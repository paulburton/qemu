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

#include "exec/address-spaces.h"
#include "hw/char/serial.h"
#include "hw/hw.h"
#include "hw/mips/cpudevs.h"
#include "hw/mips/jz4780.h"
#include "qemu-common.h"
#include "sysemu/sysemu.h"

static void cpux_reset(void *opaque)
{
    MIPSCPU *cpu = opaque;
    CPUMIPSState *env = &cpu->env;

    cpu_reset(CPU(cpu));

    env->CP0_Status &= ~((1 << CP0St_BEV) | (1 << CP0St_ERL));
    env->active_tc.PC = 0xf0000000;
}

int jz4780_init(uint32_t ddr_size)
{
    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *ddr_low, *ddr_high;
    uint32_t ddr_low_size, ddr_low_max = 256 << 20;
    uint32_t ddr_high_size, ddr_high_max = 768 << 20;
    qemu_irq tcu_irqs[3];
    CPUMIPSState *env;
    DeviceState *intc;
    MIPSCPU *cpu;
    unsigned i;

    /* init CPUs */
    for (i = 0; i < smp_cpus; i++) {
        cpu = cpu_mips_init("xburst-jz4780");
        if (cpu == NULL) {
            fprintf(stderr, "Unable to find CPU definition\n");
            exit(1);
        }
        env = &cpu->env;

        /* Init internal devices */
        cpu_mips_irq_init_cpu(env);
        cpu_mips_clock_init(env);
        qemu_register_reset(cpux_reset, cpu);
    }
    cpu = MIPS_CPU(first_cpu);
    env = &cpu->env;

    ddr_low_size = MIN(ddr_size, ddr_low_max);
    ddr_high_size = ddr_size - ddr_low_size;
    if (ddr_high_size > ddr_high_max)
        return -EINVAL;

    ddr_low = g_new(MemoryRegion, 1);
    memory_region_init_ram(ddr_low, NULL, "ddr-low.ram", ddr_low_size, &error_abort);
    vmstate_register_ram_global(ddr_low);
    memory_region_add_subregion(system_memory, 0x00000000, ddr_low);

    if (ddr_high_size) {
        ddr_high = g_new(MemoryRegion, 1);
        memory_region_init_ram(ddr_high, NULL, "ddr-high.ram", ddr_high_size, &error_abort);
        vmstate_register_ram_global(ddr_high);
        memory_region_add_subregion(system_memory, 0x30000000, ddr_high);
    }

    jz4780_cgu_init(0x10000000);
    intc = jz47xx_intc_init(0x10001000, 2, env->irq[2]);

    tcu_irqs[0] = jz47xx_intc_irq(intc, JZ4780_INTC_TCU0);
    tcu_irqs[1] = jz47xx_intc_irq(intc, JZ4780_INTC_TCU1);
    tcu_irqs[2] = jz47xx_intc_irq(intc, JZ4780_INTC_TCU2);
    jz4780_tcu_init(0x10002000, tcu_irqs);

    serial_mm_init(system_memory, 0x10030000, 2,
                   jz47xx_intc_irq(intc, JZ4780_INTC_UART0),
                   115200, serial_hds[0], DEVICE_NATIVE_ENDIAN);
    serial_mm_init(system_memory, 0x10031000, 2,
                   jz47xx_intc_irq(intc, JZ4780_INTC_UART1),
                   115200, serial_hds[1], DEVICE_NATIVE_ENDIAN);
    serial_mm_init(system_memory, 0x10032000, 2,
                   jz47xx_intc_irq(intc, JZ4780_INTC_UART2),
                   115200, serial_hds[2], DEVICE_NATIVE_ENDIAN);
    serial_mm_init(system_memory, 0x10033000, 2,
                   jz47xx_intc_irq(intc, JZ4780_INTC_UART3),
                   115200, serial_hds[3], DEVICE_NATIVE_ENDIAN);
    serial_mm_init(system_memory, 0x10034000, 2,
                   jz47xx_intc_irq(intc, JZ4780_INTC_UART4),
                   115200, serial_hds[4], DEVICE_NATIVE_ENDIAN);

    return 0;
}
