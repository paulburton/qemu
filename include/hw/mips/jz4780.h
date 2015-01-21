/*
 * QEMU Ingenic jz4780 SoC support
 *
 * Copyright (c) 2015 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef hw_mips_jz4780_h
#define hw_mips_jz4780_h

#include "exec/hwaddr.h"
#include "hw/mips/jz47xx.h"

enum jz4780_intc_irq {
    /* Bank 0 */
    JZ4780_INTC_AIC1 = 0,
    JZ4780_INTC_AIC0,
    JZ4780_INTC_BCH,
    JZ4780_INTC_HDMI,
    JZ4780_INTC_HDMI_WAKEUP,
    JZ4780_INTC_OHCI,
    /* Reserved */
    JZ4780_INTC_SSI1 = 7,
    JZ4780_INTC_SSI0,
    JZ4780_INTC_TSSI0,
    JZ4780_INTC_PDMA,
    JZ4780_INTC_TSSI1,
    JZ4780_INTC_GPIO5,
    JZ4780_INTC_GPIO4,
    JZ4780_INTC_GPIO3,
    JZ4780_INTC_GPIO2,
    JZ4780_INTC_GPIO1,
    JZ4780_INTC_GPIO0,
    JZ4780_INTC_SADC,
    JZ4780_INTC_X2D,
    JZ4780_INTC_EHCI,
    JZ4780_INTC_OTG,
    JZ4780_INTC_IPU1,
    JZ4780_INTC_LCD1,
    JZ4780_INTC_GPS_1MS,
    JZ4780_INTC_TCU2,
    JZ4780_INTC_TCU1,
    JZ4780_INTC_TCU0,
    JZ4780_INTC_GPS,
    JZ4780_INTC_IPU,
    JZ4780_INTC_CIM,
    JZ4780_INTC_LCD,

    /* Bank 1 */
    JZ4780_INTC_RTC,
    JZ4780_INTC_OWI,
    JZ4780_INTC_UART4,
    JZ4780_INTC_MSC2,
    JZ4780_INTC_MSC1,
    JZ4780_INTC_MSC0,
    JZ4780_INTC_SCC,
    /* Reserved */
    JZ4780_INTC_PCM0 = 40,
    JZ4780_INTC_KBC,
    JZ4780_INTC_GPVLC,
    JZ4780_INTC_COMPRESS,
    JZ4780_INTC_HARB2,
    /* Reserved */
    JZ4780_INTC_HARB0 = 46,
    JZ4780_INTC_CPM,
    JZ4780_INTC_UART3,
    JZ4780_INTC_UART2,
    JZ4780_INTC_UART1,
    JZ4780_INTC_UART0,
    JZ4780_INTC_DDR,
    /* Reserved */
    JZ4780_INTC_NEMC = 54,
    JZ4780_INTC_ETHC,
    JZ4780_INTC_SMB4,
    JZ4780_INTC_SMB3,
    JZ4780_INTC_SMB2,
    JZ4780_INTC_SMB1,
    JZ4780_INTC_SMB0,
    JZ4780_INTC_PDMAM,
    JZ4780_INTC_VPU,
    JZ4780_INTC_GPU,
};

extern int jz4780_init(uint32_t ddr_size);

extern DeviceState *jz4780_cgu_init(hwaddr base);
extern DeviceState *jz4780_tcu_init(hwaddr base, qemu_irq irqs[static 3]);

#endif /* hw_mips_jz4780_h */
