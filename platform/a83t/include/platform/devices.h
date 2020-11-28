/*
 * Copyright © 2017-2020 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#ifndef PLATFORM_DEVICES_H
#define PLATFORM_DEVICES_H

#define DEV_DE          0x01000000
#define DEV_DEBUG       0x01400000
#define DEV_CPU_BIST_C0 0x01502000
#define DEV_CPU_BIST_C1 0x01602000
#define DEV_CPUCFG      0x01700000
#define DEV_TIMESTAMP   0x01710000
#define DEV_CCI         0x01790000
#define DEV_SYSCON      0x01c00000
#define DEV_DMA         0x01c02000
#define DEV_NAND        0x01c03000
#define DEV_KEYMEM      0x01c0b000
#define DEV_TCON0       0x01c0c000
#define DEV_TCON1       0x01c0d000
#define DEV_VE          0x01c0e000
#define DEV_MMC0        0x01c0f000
#define DEV_MMC1        0x01c10000
#define DEV_MMC2        0x01c11000
#define DEV_SID         0x01c14000
#define DEV_SS          0x01c15000
#define DEV_MSGBOX      0x01c17000
#define DEV_SPINLOCK    0x01c18000
#define DEV_SPINLOCK_STATUS_OFFSET  0x0010
#define DEV_SPINLOCK_LOCK_REGN      0x0100
#define DEV_SPINLOCK_MAX_LOCKS      32
#define DEV_USBOTG      0x01c19000
#define DEV_USB0        0x01c1a000
#define DEV_USB1        0x01c1b000
#define DEV_TZASC       0x01c1e000
#define DEV_CCU         0x01c20000
#define DEV_PIO         0x01c20800
#define DEV_TIMER       0x01c20c00
#define DEV_SPDIF       0x01c21000
#define DEV_PWM         0x01c21400
#define DEV_I2S0        0x01c22000
#define DEV_I2S1        0x01c22400
#define DEV_I2S2        0x01c22800
#define DEV_TDM         0x01c23000
#define DEV_SPC         0x01c23400
#define DEV_DSI         0x01c26000
#define DEV_UART0       0x01c28000
#define DEV_UART1       0x01c28400
#define DEV_UART2       0x01c28800
#define DEV_UART3       0x01c28c00
#define DEV_UART4       0x01c29000
#define DEV_I2C0        0x01c2ac00
#define DEV_I2C1        0x01c2b000
#define DEV_I2C2        0x01c2b400
#define DEV_EMAC        0x01c30000
#define DEV_GPU         0x01c40000
#define DEV_HSTIMER     0x01c60000
#define DEV_DRAMCOM     0x01c62000
#define DEV_DRAMCTL     0x01c63000
#define DEV_DRAMPHY     0x01c65000
#define DEV_SPI0        0x01c68000
#define DEV_SPI1        0x01c69000
#define DEV_GIC         0x01c80000
#define DEV_GICD        0x01c81000
#define DEV_GICC        0x01c82000
#define DEV_CSI         0x01cb0000
#define DEV_HDMI        0x01ee0000
#define DEV_R_TIMER     0x01f00800
#define DEV_R_INTC      0x01f00c00
#define DEV_R_WDOG      0x01f01000
#define DEV_R_PRCM      0x01f01400
#define DEV_R_TWD       0x01f01800
#define DEV_R_CPUCFG    0x01f01c00
#define DEV_R_CIR       0x01f02000
#define DEV_R_I2C       0x01f02400
#define DEV_R_UART      0x01f02800
#define DEV_R_PIO       0x01f02c00
#define DEV_R_RSB       0x01f03400
#define DEV_R_PWM       0x01f03800
#define DEV_R_LRADC     0x01f03c00
#define DEV_R_THS       0x01f04000

#endif /* PLATFORM_DEVICES_H */
