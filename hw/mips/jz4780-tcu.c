
#include "hw/hw.h"
#include "hw/mips/jz4780.h"
#include "hw/sysbus.h"

#define TYPE_JZ4780_TCU "jz4780-tcu"
#define JZ4780_TCU(obj) \
    OBJECT_CHECK(JZ4780TCUInfo, (obj), TYPE_JZ4780_TCU)

struct JZ4780TCUInfo;

typedef struct JZ4780TCUChannelInfo {
    struct JZ4780TCUInfo *tcu;
    unsigned idx;
    uint32_t half, full, count, ctrl;
    QEMUTimer *timer;
} JZ4780TCUChannelInfo;

typedef struct JZ4780TCUInfo {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq irqs[3];
    JZ4780TCUChannelInfo *channels[16];
    uint32_t enable, flag, stop, mask;
} JZ4780TCUInfo;

enum tcu_reg {
    REG_TER     = 0x10, /* Enable */
    REG_TESR    = 0x14, /* Enable Set */
    REG_TECR    = 0x18, /* Enable Clear */
    REG_TSR     = 0x1c, /* Stop */
    REG_TFR     = 0x20, /* Flag */
    REG_TFSR    = 0x24, /* Flag Set */
    REG_TFCR    = 0x28, /* Flag Clear */
    REG_TSSR    = 0x2c, /* Stop Set */
    REG_TMR     = 0x30, /* Mask */
    REG_TMSR    = 0x34, /* Mask Set */
    REG_TMCR    = 0x38, /* Mask Clear */
    REG_TSCR    = 0x3c, /* Stop Clear */
    REG_TDFR0   = 0x40, /* Channel 0 Data Full */
    REG_TDHR0   = 0x44, /* Channel 0 Data Half */
    REG_TCNT0   = 0x48, /* Channel 0 Counter */
    REG_TCSR0   = 0x4c, /* Channel 0 Control */
    /* ... */
    REG_TDFR7   = 0xb0, /* Channel 7 Data Full */
    REG_TDHR7   = 0xb4, /* Channel 7 Data Half */
    REG_TCNT7   = 0xb8, /* Channel 7 Counter */
    REG_TCSR7   = 0xbc, /* Channel 7 Control */
};

enum tcu_timer_reg {
    REG_TDFR    = 0x00, /* Data Full */
    REG_TDHR    = 0x04, /* Data Half */
    REG_TCNT    = 0x08, /* Counter */
    REG_TCSR    = 0x0c, /* Control */
};

static void jz4780_tcu_update_irqs(JZ4780TCUInfo *s)
{
    uint32_t pending = s->flag & ~s->mask;

    /* OS timer uses IRQ0 */
    if (pending & 0x00008000)
        qemu_irq_raise(s->irqs[0]);
    else
        qemu_irq_lower(s->irqs[0]);

    /* Timer 5 uses IRQ1 */
    if (pending & 0x00200020)
        qemu_irq_raise(s->irqs[1]);
    else
        qemu_irq_lower(s->irqs[1]);

    /* Timers 0-4,6,7 use IRQ2 */
    if (pending & 0x00df00df)
        qemu_irq_raise(s->irqs[2]);
    else
        qemu_irq_lower(s->irqs[2]);
}

static void jz4780_tcu_update(JZ4780TCUInfo *s, unsigned channel_idx)
{
    JZ4780TCUChannelInfo *c = s->channels[channel_idx];

    if (channel_idx != 15 && c->count >= c->half)
        atomic_or(&s->flag, 1 << (16 + channel_idx));

    if (c->count >= c->full) {
        atomic_or(&s->flag, 1 << channel_idx);
        c->count = 0;
    }
}

static void jz4780_tcu_update_all(JZ4780TCUInfo *s)
{
    unsigned i;

    for (i = 0; i < 8; i++)
        jz4780_tcu_update(s, i);

    jz4780_tcu_update(s, 15);
}

static void jz4780_tcu_update_many(JZ4780TCUInfo *s, uint32_t channels)
{
    /* TODO */
    jz4780_tcu_update_all(s);
}

static uint64_t jz4780_tcu_read(void *opaque, hwaddr offset,
                                unsigned size)
{
    JZ4780TCUInfo *s = opaque;
    enum tcu_reg reg = offset;
    JZ4780TCUChannelInfo *c;
    unsigned channel_idx;

    switch (reg) {
    case REG_TDFR0 ... REG_TCSR7:
        channel_idx = (offset - REG_TDFR0) / 0x10;
        c = s->channels[channel_idx];

        switch (offset & 0xf) {
        case REG_TCNT:
            return c->count;

        default:
            fprintf(stderr, "%s: unhandled register 0x" TARGET_FMT_plx "\n", __func__, offset);
        }
        return 0;

    default:
        fprintf(stderr, "%s: unhandled register 0x" TARGET_FMT_plx "\n", __func__, offset);
        return 0;
    }
}

static void jz4780_tcu_write(void *opaque, hwaddr offset,
                             uint64_t value, unsigned size)
{
    JZ4780TCUInfo *s = opaque;
    enum tcu_reg reg = offset;
    JZ4780TCUChannelInfo *c;
    unsigned channel_idx;

    switch (reg) {
    case REG_TER:
        atomic_set(&s->enable, value);
        jz4780_tcu_update_all(s);
        break;

    case REG_TESR:
        atomic_or(&s->enable, value);
        jz4780_tcu_update_many(s, value);
        break;

    case REG_TECR:
        atomic_and(&s->enable, ~value);
        jz4780_tcu_update_many(s, value);
        break;

    case REG_TFR:
        atomic_set(&s->flag, value);
        break;

    case REG_TFSR:
        atomic_or(&s->flag, value);
        break;

    case REG_TFCR:
        atomic_and(&s->flag, ~value);
        break;

    case REG_TSR:
        atomic_set(&s->stop, value);
        jz4780_tcu_update_all(s);
        break;

    case REG_TSSR:
        atomic_or(&s->stop, value);
        jz4780_tcu_update_many(s, value);
        break;

    case REG_TMR:
        atomic_set(&s->mask, value);
        jz4780_tcu_update_all(s);
        break;

    case REG_TMSR:
        atomic_or(&s->mask, value);
        jz4780_tcu_update_many(s, value);
        break;

    case REG_TMCR:
        atomic_and(&s->mask, ~value);
        jz4780_tcu_update_many(s, value);
        break;

    case REG_TSCR:
        atomic_and(&s->stop, ~value);
        jz4780_tcu_update_many(s, value);
        break;

    case REG_TDFR0 ... REG_TCSR7:
        channel_idx = (offset - REG_TDFR0) / 0x10;
        c = s->channels[channel_idx];

        switch (offset & 0xf) {
        case REG_TDFR:
            c->full = value & 0xffff;
            break;

        case REG_TDHR:
            c->half = value & 0xffff;
            break;

        case REG_TCNT:
            c->count = value;
            break;

        case REG_TCSR:
            c->ctrl = value;
            break;

        default:
            fprintf(stderr, "%s: unhandled register 0x" TARGET_FMT_plx "\n", __func__, offset);
            return;
        }

        jz4780_tcu_update(s, channel_idx);
        break;

    default:
        fprintf(stderr, "%s: unhandled register 0x" TARGET_FMT_plx "\n", __func__, offset);
        return;
    }

    jz4780_tcu_update_irqs(s);
}

static const MemoryRegionOps jz4780_tcu_ops = {
    .read = jz4780_tcu_read,
    .write = jz4780_tcu_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void jz4780_tcu_cb(void *opaque)
{
    JZ4780TCUChannelInfo *c = opaque;

    if (c->tcu->enable & (1 << c->idx)) {
        //c->count++;
        c->count += 10000;
        jz4780_tcu_update(c->tcu, c->idx);
        jz4780_tcu_update_irqs(c->tcu);
    }

    timer_mod(c->timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 1000000);
}

static JZ4780TCUChannelInfo *jz4780_tcu_channel_init(JZ4780TCUInfo *tcu, unsigned idx)
{
    JZ4780TCUChannelInfo *c;

    c = g_malloc0(sizeof(*c));
    if (!c)
        return NULL;

    c->tcu = tcu;
    c->idx = idx;
    c->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, &jz4780_tcu_cb, c);
    c->half = 0x7fff;
    c->full = 0xffff;

    timer_mod(c->timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 1000);
    return c;
}

static int jz4780_tcu_initfn(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    JZ4780TCUInfo *s = JZ4780_TCU(dev);
    unsigned i;

    for (i = 0; i < 8; i++)
        s->channels[i] = jz4780_tcu_channel_init(s, i);
    s->channels[15] = jz4780_tcu_channel_init(s, 15);

    s->enable = 0;
    s->flag = 0;
    s->stop = ~0;
    s->mask = ~0;

    memory_region_init_io(&s->iomem, OBJECT(s), &jz4780_tcu_ops, s, "jz4780-tcu", 0x200);
    sysbus_init_mmio(sbd, &s->iomem);

    for (i = 0; i < 3; i++)
        sysbus_init_irq(sbd, &s->irqs[i]);

    return 0;
}

DeviceState *jz4780_tcu_init(hwaddr base, qemu_irq irqs[static 3])
{
    DeviceState *dev;
    unsigned i;

    dev = qdev_create(NULL, TYPE_JZ4780_TCU);
    qdev_init_nofail(dev);

    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);

    for (i = 0; i < 3; i++)
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), i, irqs[i]);

    return dev;
}

static void jz4780_tcu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = jz4780_tcu_initfn;
    dc->desc = "jz4780 TCU (Timer & Counter Unit)";
}

static const TypeInfo jz4780_tcu_info = {
    .name          = TYPE_JZ4780_TCU,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4780TCUInfo),
    .class_init    = jz4780_tcu_class_init,
};

static void jz4780_tcu_register_types(void)
{
    type_register_static(&jz4780_tcu_info);
}
type_init(jz4780_tcu_register_types)
