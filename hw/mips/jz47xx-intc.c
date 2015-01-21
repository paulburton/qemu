
#include "hw/hw.h"
#include "hw/mips/jz4780.h"
#include "hw/sysbus.h"

#define TYPE_JZ47XX_INTC "jz47xx-intc"
#define JZ47XX_INTC(obj) \
    OBJECT_CHECK(JZ47xxIntcInfo, (obj), TYPE_JZ47XX_INTC)

typedef struct JZ47xxIntcBankInfo {
    uint32_t source;
    uint32_t mask;
    uint32_t pending;
} JZ47xxIntcBankInfo;

typedef struct JZ47xxIntcInfo {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    uint32_t num_banks;
    JZ47xxIntcBankInfo *banks;
    qemu_irq *irqs;
    qemu_irq out_irq;
} JZ47xxIntcInfo;

enum intc_reg {
    REG_ICSR    = 0x00, /* Source */
    REG_ICMR    = 0x04, /* Mask */
    REG_ICMSR   = 0x08, /* Mask Set */
    REG_ICMCR   = 0x0c, /* Mask Clear */
    REG_ICPR    = 0x10, /* Pending */
};

static void jz47xx_intc_update(JZ47xxIntcInfo *s)
{
    JZ47xxIntcBankInfo *bank;
    uint32_t any_pending = 0;
    unsigned i;

    for (i = 0; i < s->num_banks; i++) {
        bank = &s->banks[i];
        bank->pending = bank->source & ~bank->mask;
        any_pending |= bank->pending;
    }

    if (any_pending)
        qemu_irq_raise(s->out_irq);
    else
        qemu_irq_lower(s->out_irq);
}

static uint64_t jz47xx_intc_read(void *opaque, hwaddr offset,
                                 unsigned size)
{
    JZ47xxIntcInfo *s = opaque;
    unsigned bank_idx = offset >> 5;
    enum intc_reg reg = offset & 0x1f;
    JZ47xxIntcBankInfo *bank;

    if (bank_idx >= s->num_banks) {
        fprintf(stderr, "%s: invalid bank %u\n", __func__, bank_idx);
        return 0;
    }
    bank = &s->banks[bank_idx];

    switch (reg) {
    case REG_ICSR:
        return bank->source;

    case REG_ICMR:
        return bank->mask;

    case REG_ICMSR:
    case REG_ICMCR:
        return 0;

    case REG_ICPR:
        return bank->pending;

    default:
        fprintf(stderr, "%s: unhandled register 0x%x\n", __func__, (unsigned)reg);
        return 0;
    }
}

static void jz47xx_intc_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned size)
{
    JZ47xxIntcInfo *s = opaque;
    unsigned bank_idx = offset >> 5;
    enum intc_reg reg = offset & 0x1f;
    JZ47xxIntcBankInfo *bank;

    if (bank_idx >= s->num_banks) {
        fprintf(stderr, "%s: invalid bank %u\n", __func__, bank_idx);
        return;
    }
    bank = &s->banks[bank_idx];

    switch (reg) {
    case REG_ICSR:
    case REG_ICPR:
        return;

    case REG_ICMR:
        atomic_set(&bank->mask, value);
        jz47xx_intc_update(s);
        return;

    case REG_ICMSR:
        atomic_or(&bank->mask, value);
        jz47xx_intc_update(s);
        return;

    case REG_ICMCR:
        atomic_and(&bank->mask, ~value);
        jz47xx_intc_update(s);
        return;

    default:
        fprintf(stderr, "%s: unhandled register 0x%x\n", __func__, (unsigned)reg);
        return;
    }
}

static const MemoryRegionOps jz47xx_intc_ops = {
    .read = jz47xx_intc_read,
    .write = jz47xx_intc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void jz47xx_intc_irq_request(void *opaque, int irq, int level)
{
    JZ47xxIntcInfo *s = opaque;
    unsigned bank_idx = irq / 32;
    unsigned bit_idx = irq % 32;
    JZ47xxIntcBankInfo *bank;

    if (bank_idx >= s->num_banks) {
        fprintf(stderr, "%s: invalid bank %u\n", __func__, bank_idx);
        return;
    }
    bank = &s->banks[bank_idx];

    if (level)
        atomic_or(&bank->source, 1 << bit_idx);
    else
        atomic_and(&bank->source, ~(1 << bit_idx));

    jz47xx_intc_update(s);
}

qemu_irq jz47xx_intc_irq(DeviceState *d, unsigned n)
{
    JZ47xxIntcInfo *s = JZ47XX_INTC(d);
    unsigned bank_idx = n / 32;

    if (bank_idx >= s->num_banks) {
        fprintf(stderr, "%s: invalid bank %u\n", __func__, bank_idx);
        exit(1);
    }

    return s->irqs[n];
}

static int jz47xx_intc_initfn(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    JZ47xxIntcInfo *s = JZ47XX_INTC(dev);
    unsigned i;

    s->banks = g_malloc0(sizeof(*s->banks) * s->num_banks);
    if (!s->banks)
        return -ENOMEM;

    for (i = 0; i < s->num_banks; i++)
        s->banks[i].mask = ~0;

    s->irqs = qemu_allocate_irqs(jz47xx_intc_irq_request, s, s->num_banks * 32);

    memory_region_init_io(&s->iomem, OBJECT(s), &jz47xx_intc_ops, s, "jz47xx-intc", s->num_banks * 0x20);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->out_irq);

    return 0;
}

DeviceState *jz47xx_intc_init(hwaddr base, unsigned num_banks, qemu_irq out_irq)
{
    DeviceState *dev;

    dev = qdev_create(NULL, TYPE_JZ47XX_INTC);
    qdev_prop_set_uint32(dev, "num_banks", num_banks);
    qdev_init_nofail(dev);

    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, out_irq);

    return dev;
}

static Property jz47xx_intc_properties[] = {
    DEFINE_PROP_UINT32("num_banks", JZ47xxIntcInfo, num_banks, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void jz47xx_intc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = jz47xx_intc_initfn;
    dc->desc = "jz47xx Interrupt Controller";
    dc->props = jz47xx_intc_properties;
}

static const TypeInfo jz47xx_intc_info = {
    .name          = TYPE_JZ47XX_INTC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ47xxIntcInfo),
    .class_init    = jz47xx_intc_class_init,
};

static void jz47xx_intc_register_types(void)
{
    type_register_static(&jz47xx_intc_info);
}
type_init(jz47xx_intc_register_types)
