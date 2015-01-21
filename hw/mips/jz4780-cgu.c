
#include "hw/hw.h"
#include "hw/mips/jz4780.h"
#include "hw/sysbus.h"

#define TYPE_JZ4780_CGU "jz4780-cgu"
#define JZ4780_CGU(obj) \
    OBJECT_CHECK(JZ4780CGUInfo, (obj), TYPE_JZ4780_CGU)

typedef struct JZ4780CGUInfo {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
} JZ4780CGUInfo;

static uint64_t jz4780_cgu_read(void *opaque, hwaddr offset,
                                unsigned size)
{
    printf("%s: 0x" TARGET_FMT_plx " sz=%u\n", __func__, offset, size);
    return 0;
}

static void jz4780_cgu_write(void *opaque, hwaddr offset,
                             uint64_t value, unsigned size)
{
    printf("%s: *0x" TARGET_FMT_plx " = 0x%" PRIx64 " sz=%u\n", __func__, offset, value, size);
}

static const MemoryRegionOps jz4780_cgu_ops = {
    .read = jz4780_cgu_read,
    .write = jz4780_cgu_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int jz4780_cgu_initfn(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    JZ4780CGUInfo *s = JZ4780_CGU(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &jz4780_cgu_ops, s, "jz4780-cgu", 0x100);
    sysbus_init_mmio(sbd, &s->iomem);

    return 0;
}

DeviceState *jz4780_cgu_init(hwaddr base)
{
    DeviceState *dev;

    dev = qdev_create(NULL, TYPE_JZ4780_CGU);
    qdev_init_nofail(dev);

    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);

    return dev;
}

static void jz4780_cgu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = jz4780_cgu_initfn;
    dc->desc = "jz4780 CGU (Clock & Power Controller)";
}

static const TypeInfo jz4780_cgu_info = {
    .name          = TYPE_JZ4780_CGU,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(JZ4780CGUInfo),
    .class_init    = jz4780_cgu_class_init,
};

static void jz4780_cgu_register_types(void)
{
    type_register_static(&jz4780_cgu_info);
}
type_init(jz4780_cgu_register_types)
