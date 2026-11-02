/*
 * Compact Flash Association Temperature Monitor
 *
 * Author: James Buren <braewoods+ow@braewoods.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/unaligned.h>
#include <linux/ata.h>
#include <linux/hwmon.h>
#include <linux/version.h>
#include <scsi/scsi_driver.h>

#define DRVNAME "cfatemp"

struct cfatemp_temp {
    struct mutex lock;
    union {
        struct {
            int val;
            int low;
            int high;
        };
        int raw[3];
    };
};

struct cfatemp_data {
    struct list_head list;
    struct device *dev;
    struct scsi_device *sdev;
    struct device *hwdev;
    struct cfatemp_temp temp;
    bool have_lowhigh;
};

static bool
ata_id_smart_supported(const u16 *ata_id)
{
    return !!(ata_id[ATA_ID_COMMAND_SET_1] & (1 << 0));
}

static bool
ata_id_smart_enabled(const u16 *ata_id)
{
    return !!(ata_id[ATA_ID_CFS_ENABLE_1] & (1 << 0));
}

static int
cfatemp_check_vpd_ata(struct scsi_device *sdev)
{
    const struct scsi_vpd *vpd;
    const u8 *data;
    const u16 *ata_id;
    bool ata;
    bool cfa;
    bool smart;
    int ret = -ENODEV;

    rcu_read_lock();
    vpd = rcu_dereference(sdev->vpd_pg89);
    if (!vpd || vpd->len < 572)
        goto bail;

    data = vpd->data;
    if (data[36] != 0x34 && data[56] != ATA_CMD_ID_ATA)
        goto bail;

    ata_id = (u16 *) &data[60];
    ata = ata_id_is_ata(ata_id);
    cfa = ata_id_is_cfa(ata_id);
    smart = ata_id_smart_supported(ata_id) && ata_id_smart_enabled(ata_id);

    if (!ata || !cfa || !smart)
        goto bail;

    ret = 0;
bail:
    rcu_read_unlock();
    return ret;
}

static int
cfatemp_check_libata(struct scsi_device *sdev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,17,0)
    if (!sdev->is_ata)
        return -ENODEV;
#endif

    if (!sdev->inquiry || sdev->inquiry_len < 96)
        return -ENODEV;

    if (sdev->type != TYPE_DISK && sdev->type != TYPE_ZBC)
        return -ENODEV;

    return cfatemp_check_vpd_ata(sdev);
}

static int
cfatemp_smart_read(struct scsi_device *sdev, u8 buf[static ATA_SECT_SIZE])
{
    static const u8 cmd[12] = {
        [0] = ATA_12,
        [1] = 4 << 1,
        [2] = 0x0e,
        [3] = ATA_SMART_READ_VALUES,
        [4] = 1,
        [5] = 0,
        [6] = ATA_SMART_LBAM_PASS,
        [7] = ATA_SMART_LBAH_PASS,
        [8] = 0,
        [9] = ATA_CMD_SMART,
    };
    int ret;

    ret = scsi_execute_cmd(
        sdev,
        cmd,
        REQ_OP_DRV_IN,
        buf,
        ATA_SECT_SIZE,
        10 * HZ,
        5,
        NULL
    );

    return (ret < 0) ? ret : 0;
}

static int
cfatemp_smart_csum(const u8 buf[static ATA_SECT_SIZE])
{
    u8 csum = 0;

    for (size_t i = 0; i < ATA_SECT_SIZE; i++)
        csum += buf[i];

    return (csum ? -EIO : 0);
}

static int
cfatemp_smart_attr(const u8 buf[static ATA_SECT_SIZE], const u8 **attr)
{
    for (size_t i = 2; i < 362; i += 12)
        if (buf[i] == 194) {
            *attr = &buf[i];
            return 0;
        }

    return -ENODEV;
}

static int
cfatemp_smart_temp(const u8 *attr, struct cfatemp_temp *ct)
{
    ct->val = ((s16) get_unaligned_le16(attr + 5)) * 1000;
    ct->low = ((s16) get_unaligned_le16(attr + 7)) * 1000;
    ct->high = ((s16) get_unaligned_le16(attr + 9)) * 1000;

    if (ct->low > ct->high)
        swap(ct->low, ct->high);

    if (ct->low && ct->high && (ct->low > ct->val || ct->high < ct->val))
        return -ENODEV;

    return 0;
}

static int
cfatemp_check_smart(struct scsi_device *sdev, struct cfatemp_temp *ct)
{
    u8 buf[ATA_SECT_SIZE];
    const u8 *attr;
    int ret;

    ret = cfatemp_smart_read(sdev, buf);
    if (ret)
        return ret;

    ret = cfatemp_smart_csum(buf);
    if (ret)
        return ret;

    ret = cfatemp_smart_attr(buf, &attr);
    if (ret)
        return ret;

    ret = cfatemp_smart_temp(attr, ct);
    if (ret)
        return ret;

    return 0;
}

static int
cfatemp_check_dev(struct scsi_device *sdev, struct cfatemp_temp *ct)
{
    int ret;

    ret = cfatemp_check_libata(sdev);
    if (ret)
        return ret;

    ret = cfatemp_check_smart(sdev, ct);
    if (ret)
        return ret;

    return 0;
}

static umode_t
cfatemp_is_visible(const void *data, enum hwmon_sensor_types type, u32 attr, int chan)
{
    const struct cfatemp_data *cd = data;

    if (chan != 0)
        goto hide;

    if (type != hwmon_temp)
        goto hide;

    if (attr == hwmon_temp_input)
        goto show;

    if (!cd->have_lowhigh)
        goto hide;

    if (attr == hwmon_temp_lowest)
        goto show;

    if (attr == hwmon_temp_highest)
        goto show;

hide:
    return 0000;

show:
    return 0444;
}

static int
cfatemp_read(struct device *dev, enum hwmon_sensor_types type, u32 attr, int chan, long *out)
{
    struct cfatemp_data *cd = dev_get_drvdata(dev);
    struct scsi_device *sdev = cd->sdev;
    struct cfatemp_temp *ct = &cd->temp;
    size_t index;
    int ret;

    if (chan != 0)
        goto error;

    if (type != hwmon_temp)
        goto error;

    if (attr == hwmon_temp_input) {
        index = 0;
        goto read;
    }

    if (!cd->have_lowhigh)
        goto error;

    if (attr == hwmon_temp_lowest) {
        index = 1;
        goto read;
    }

    if (attr == hwmon_temp_highest) {
        index = 2;
        goto read;
    }

error:
    return -EINVAL;

read:
    mutex_lock(&ct->lock);
    ret = (index ? 0 : cfatemp_check_smart(sdev, ct));
    *out = ct->raw[index];
    mutex_unlock(&ct->lock);
    return ret;
}

static const struct hwmon_ops cfatemp_ops = {
    .is_visible = cfatemp_is_visible,
    .read       = cfatemp_read,
};

static const struct hwmon_channel_info * const cfatemp_channel_info[] = {
    HWMON_CHANNEL_INFO(chip, HWMON_C_REGISTER_TZ),
    HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT | HWMON_T_LOWEST | HWMON_T_HIGHEST),
    NULL,
};

static const struct hwmon_chip_info cfatemp_chip_info = {
    .ops  = &cfatemp_ops,
    .info = cfatemp_channel_info,
};

static LIST_HEAD(cfatemp_devlist);

static int
cfatemp_add_dev(struct device *dev)
{
    struct device *parent = dev->parent;
    struct scsi_device *sdev = to_scsi_device(parent);
    struct cfatemp_data *cd;
    struct cfatemp_temp *ct;
    int ret;

    cd = kzalloc(sizeof(*cd), GFP_KERNEL);
    if (!cd)
        return -ENOMEM;

    ret = cfatemp_check_dev(sdev, ct = &cd->temp);
    if (ret)
        goto error;

    mutex_init(&ct->lock);

    cd->dev = dev;
    cd->sdev = sdev;
    cd->have_lowhigh = (ct->low && ct->high);

    cd->hwdev = hwmon_device_register_with_info(
        parent,
        DRVNAME,
        cd,
        &cfatemp_chip_info,
        NULL
    );
    ret = PTR_ERR_OR_ZERO(cd->hwdev);
    if (ret)
        goto error;

    list_add(&cd->list, &cfatemp_devlist);
    return 0;

error:
    kfree(cd);
    return ret;
}

static void
cfatemp_remove_dev(struct device *dev)
{
    struct cfatemp_data *cd;
    struct cfatemp_data *tmp;

    list_for_each_entry_safe(cd, tmp, &cfatemp_devlist, list) {
        if (cd->dev == dev) {
            list_del(&cd->list);
            hwmon_device_unregister(cd->hwdev);
            kfree(cd);
            break;
        }
    }
}

static struct class_interface cfatemp_interface = {
    .add_dev    = cfatemp_add_dev,
    .remove_dev = cfatemp_remove_dev,
};

static int __init
cfatemp_init(void)
{
    return scsi_register_interface(&cfatemp_interface);
}
module_init(cfatemp_init);

static void __exit
cfatemp_exit(void)
{
    scsi_unregister_interface(&cfatemp_interface);
}
module_exit(cfatemp_exit);

MODULE_AUTHOR("James Buren <braewoods+ow@braewoods.net>");
MODULE_DESCRIPTION("Compact Flash Association Temperature Monitor");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRVNAME);
