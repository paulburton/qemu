/*
 * QEMU Mips Creator CI20 board support
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

#include "elf.h"
#include "hw/boards.h"
#include "hw/hw.h"
#include "hw/loader.h"
#include "hw/mips/cpudevs.h"
#include "hw/mips/jz4780.h"
#include "sysemu/char.h"
#include "sysemu/sysemu.h"

static int64_t kernel_entry;

static void cpu0_reset(void *opaque)
{
    MIPSCPU *cpu = opaque;
    CPUMIPSState *env = &cpu->env;

    env->active_tc.PC = kernel_entry;
}

static void ci20_init(MachineState *machine)
{
    MIPSCPU *cpu;
    int64_t kernel_high;
    int i, err;

    if (!machine->kernel_filename) {
        fprintf(stderr, "No kernel provided\n");
        exit(1);
    }

    for (i = 0; i < 5; i++) {
        if (!serial_hds[i]) {
            char label[32];
            snprintf(label, sizeof(label), "serial%d", i);
            serial_hds[i] = qemu_chr_find(label);
            if (!serial_hds[i])
                serial_hds[i] = qemu_chr_new(label, "null", NULL);
        }
    }

    err = jz4780_init(1 << 30);
    if (err) {
        fprintf(stderr, "Unable to initialize jz4780 SoC support\n");
        exit(1);
    }

    cpu = MIPS_CPU(first_cpu);
    qemu_register_reset(cpu0_reset, cpu);

    if (load_elf(machine->kernel_filename, cpu_mips_kseg0_to_phys, NULL,
                 (uint64_t *)&kernel_entry, NULL, (uint64_t *)&kernel_high,
                 0, ELF_MACHINE, 1) < 0) {
        fprintf(stderr, "qemu: could not load kernel '%s'\n",
                machine->kernel_filename);
        exit(1);
    }
}

static QEMUMachine ci20_machine = {
    .name = "ci20",
    .desc = "MIPS Creator CI20",
    .init = ci20_init,
    .max_cpus = 2,
};

static void ci20_machine_init(void)
{
    qemu_register_machine(&ci20_machine);
}
machine_init(ci20_machine_init);
