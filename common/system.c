/*
 * Copyright © 2017-2020 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#include <counter.h>
#include <css.h>
#include <debug.h>
#include <delay.h>
#include <device.h>
#include <exception.h>
#include <irq.h>
#include <pmic.h>
#include <regulator.h>
#include <regulator_list.h>
#include <scpi.h>
#include <serial.h>
#include <simple_device.h>
#include <stdbool.h>
#include <stddef.h>
#include <system.h>
#include <util.h>
#include <watchdog.h>
#include <clock/ccu.h>
#include <gpio/sunxi-gpio.h>
#include <msgbox/sunxi-msgbox.h>
#include <watchdog/sunxi-twd.h>

#if CONFIG(DEBUG_HWSPINLOCK_TEST)
#include <timeout.h>
#include <platform/devices.h>

#define TIMEOUT_VAL	CONFIG_DEBUG_HWSPINLOCK_TEST_TIMEOUT /* us */
#define VALUE_TO_STR(x)	#x
#define STR(x)		VALUE_TO_STR(x)
#pragma message		("WARNING: hwspinlock loop is enabled - timing: " STR(TIMEOUT_VAL) "us")

#define readl(addr) (*((volatile unsigned long *)(addr)))
#define writel(v, addr) (*((volatile unsigned long *)(addr)) = (unsigned long)(v))

static uint32_t timeout;
static uint16_t lock;
#endif

/* This variable is persisted across exception restarts. */
static uint8_t system_state = SYSTEM_BOOT;

uint8_t
get_system_state(void)
{
	return system_state;
}

bool
system_can_wake(void)
{
	return system_state == SYSTEM_INACTIVE ||
	       system_state == SYSTEM_OFF;
}

bool
system_is_running(void)
{
	return system_state == SYSTEM_ACTIVE;
}

noreturn void
system_state_machine(uint32_t exception)
{
	const struct device *mailbox, *pmic, *watchdog;
	uint8_t initial_state = system_state;

	if (initial_state > SYSTEM_ACTIVE) {
		/*
		 * If the firmware started in any state other than the initial
		 * state or SYSTEM_ACTIVE, assume the system is off. It could
		 * be suspended, but resetting the board after an IRQ is safer
		 * than attempting to resume in an unpredictable environment.
		 */
		system_state = SYSTEM_OFF;

		/* Clear out inactive references. */
		watchdog = NULL;
		mailbox  = NULL;
	} else {
		/* Otherwise, prepare the SYSTEM_ACTIVE state. */
		system_state = SYSTEM_ACTIVE;

		/* First, enable watchdog protection. */
		watchdog = device_get_or_null(&r_twd.dev);

		/* Perform one-time device driver initialization. */
		counter_init();
		r_ccu_init();
		ccu_init();
		css_init();

		/* Acquire runtime-only devices. */
		mailbox = device_get_or_null(&msgbox.dev);
	}

	/*
	 * Initialize the serial port. Unless a preinitialized port (UART0) is
	 * selected, errors occurring before this function call will not be
	 * logged, and exceptions (such as those caused by assertion failures)
	 * could result in a silent infinite loop.
	 *
	 * Serial communication needs accurate clock frequencies. Specifically,
	 * OSC16M must be calibrated before initializing R_UART, and R_UART
	 * will only work after a SYSTEM_INACTIVE/OFF exception restart if the
	 * calibrated OSC16M frequency is retained across the restart.
	 */
	serial_init();

	/* Log startup messages. */
	report_exception(exception);
	debug_print_sprs();

	/*
	 * If the firmware started in the initial state, assume the secure
	 * monitor is waiting for an SCP_READY message.  Otherwise, assume
	 * nothing is listening, and skip sending the SCP_READY message to
	 * avoid filling up the mailbox.
	 */
	if (mailbox && initial_state == SYSTEM_BOOT) {
		scpi_create_message(mailbox, SCPI_CLIENT_EL3,
		                    SCPI_CMD_SCP_READY);
	}

#if CONFIG(DEBUG_HWSPINLOCK_TEST)
	timeout = timeout_set(TIMEOUT_VAL);
	lock = 0;
#endif

	for (;;) {
		switch (system_state) {
		case SYSTEM_ACTIVE:
			/* Poll runtime devices. */
			if (watchdog)
				watchdog_restart(watchdog);

			/* Poll runtime services. */
			if (mailbox)
				scpi_poll(mailbox);

#if CONFIG(DEBUG_HWSPINLOCK_TEST)
			if (timeout_expired(timeout)) {
				writel(0, DEV_SPINLOCK + DEV_SPINLOCK_LOCK_REGN + lock * 4);

				timeout = timeout_set(TIMEOUT_VAL);

				++lock;
				if (lock >= DEV_SPINLOCK_MAX_LOCKS)
					lock = 0;
				readl(DEV_SPINLOCK + DEV_SPINLOCK_LOCK_REGN + lock * 4);
			}
#endif

			/*
			 * Skip debugging hooks while the system is active,
			 * even after triggering a state transition. This
			 * avoids clobbering device state before the suspend
			 * procedure runs.
			 */
			continue;
		case SYSTEM_SUSPEND:
		case SYSTEM_SHUTDOWN:
			debug("Suspending...");

			int (*pmic_action)(const struct device *);
			const struct regulator_list *regulators;
			uint8_t next_state;

			if (system_state == SYSTEM_SUSPEND) {
				pmic_action = pmic_suspend;
				regulators  = &inactive_list;
				next_state  = SYSTEM_INACTIVE;
			} else {
				pmic_action = pmic_shutdown;
				regulators  = &off_list;
				next_state  = SYSTEM_OFF;
			}

			/* Release runtime-only devices. */
			device_put(mailbox), mailbox = NULL;

			/* Synchronize device state with Linux. */
			simple_device_sync(&pio);
			simple_device_sync(&r_pio);

			/* Configure the CCU for minimal power consumption. */
			ccu_suspend();

			/*
			 * Disable watchdog protection. Once devices outside
			 * the SoC (oscillators and regulators) are disabled,
			 * the watchdog cannot successfully reset the SoC.
			 */
			device_put(watchdog), watchdog = NULL;

			/* Gate the rest of the SoC before removing power. */
			r_ccu_suspend();

			/* Perform PMIC-specific actions. */
			if ((pmic = pmic_get()))
				pmic_action(pmic);

			/* Turn off all unnecessary power domains. */
			regulator_bulk_disable(regulators);

			/*
			 * The regulator provider is often part of the same
			 * device as the PMIC. Reduce churn by doing both PMIC
			 * and regulator actions before releasing the PMIC.
			 */
			device_put(pmic);

			debug("Suspend complete!");

			/* The system is now inactive or off. */
			system_state = next_state;
			break;
		case SYSTEM_INACTIVE:
			/* Poll wakeup sources. Resume on wakeup. */
			if (!irq_poll())
				break;
			fallthrough;
		case SYSTEM_RESUME:
			debug("Resuming...");

			/*
			 * Perform PMIC-specific resume actions.
			 * The PMIC is expected to restore regulator state.
			 * If it fails, manually turn the regulators back on.
			 */
			if (!(pmic = pmic_get()) || pmic_resume(pmic))
				regulator_bulk_enable(&inactive_list);
			device_put(pmic);

			/* Give regulator outputs time to rise. */
			udelay(5000);

			/* Restore SoC-internal power domains. */
			r_ccu_resume();

			/* Enable watchdog protection. */
			watchdog = device_get_or_null(&r_twd.dev);

			/* Configure the CCU for increased performance. */
			ccu_resume();

			/* Acquire runtime-only devices. */
			mailbox = device_get_or_null(&msgbox.dev);

			/* Resume execution on the first CPU in the CSS. */
			css_set_power_state(0, 0, SCPI_CSS_ON,
			                    SCPI_CSS_ON, SCPI_CSS_ON);

			debug("Resume complete!");

			/* The system is now active. */
			system_state = SYSTEM_ACTIVE;
			break;
		case SYSTEM_OFF:
			/* Poll wakeup sources. Reboot on wakeup. */
			if (!irq_poll())
				break;
			fallthrough;
		case SYSTEM_REBOOT:
			/* Attempt to reset the board using the PMIC. */
			if ((pmic = pmic_get())) {
				pmic_reset(pmic);
				device_put(pmic);
			}

			/* Continue through to resetting the SoC. */
			fallthrough;
		case SYSTEM_RESET:
		default:
			/* Turn on regulators required to boot. */
			regulator_bulk_enable(&off_list);

			/* Give regulator outputs time to rise. */
			udelay(5000);

			/* Restore SoC-internal power domains. */
			r_ccu_resume();

			/* Attempt to reset the SoC using the watchdog. */
			if (!watchdog)
				watchdog = device_get_or_null(&r_twd.dev);
			if (watchdog)
				watchdog_set_timeout(watchdog, 1);

			/* Continue making reset attempts each iteration. */
			system_state = SYSTEM_RESET;
			break;
		}

		debug_monitor();
		debug_print_battery();
		debug_print_latency();
	}
}

void
system_reboot(void)
{
	system_state = SYSTEM_REBOOT;
}

void
system_reset(void)
{
	system_state = SYSTEM_RESET;
}

void
system_shutdown(void)
{
	assert(system_state == SYSTEM_ACTIVE);

	system_state = SYSTEM_SHUTDOWN;
}

void
system_suspend(void)
{
	assert(system_state == SYSTEM_ACTIVE);

	system_state = SYSTEM_SUSPEND;
}

void
system_wakeup(void)
{
	if (system_state == SYSTEM_INACTIVE)
		system_state = SYSTEM_RESUME;
	else
		system_state = SYSTEM_REBOOT;
}
