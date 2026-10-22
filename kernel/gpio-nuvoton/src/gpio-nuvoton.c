/*
 * GPIO driver for Nuvoton Super-I/O Chips
 *
 * Author: James Buren <braewoods+ow@braewoods.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/gpio/driver.h>
#include <linux/platform_device.h>

#define DRVNAME "gpio-nuvoton"

#define SIO_ENTER        0x87
#define SIO_EXIT         0xaa
#define SIO_LDSEL        0x07
#define SIO_CHIPID       0x20

#define SIO_NCT6106D_ID  0xc452
#define SIO_NOCHIP       0xffff

#define NUVOTON_REG_DIR  0
#define NUVOTON_REG_DAT  1
#define NUVOTON_REG_INV  2
#define NUVOTON_REG_PPOD 3
#define NUVOTON_REG_EN   4

#define NUVOTON_CHIP(_ngpio, _label)                      \
        {                                                 \
            .label            = DRVNAME "-" _label,       \
            .owner            = THIS_MODULE,              \
            .get_direction    = nuvoton_get_direction,    \
            .direction_input  = nuvoton_direction_input,  \
            .direction_output = nuvoton_direction_output, \
            .get              = nuvoton_get,              \
            .set              = nuvoton_set,              \
            .set_config       = nuvoton_set_config,       \
            .base             = -1,                       \
            .ngpio            = _ngpio,                   \
        }

enum nuvoton_chip : u16 {
    nct6106d,
};

struct nuvoton_sio {
    // Super-I/O Chip Address
    u16 addr;

    // Nuvoton Chip Type
    enum nuvoton_chip chip;
};

struct nuvoton_bank {
    // Generic GPIO Structure
    struct gpio_chip chip;

    // Super-I/O Chip Address
    u16 addr;

    // The location of this bank's enable register.
    struct {
        u8 ld;
        u8 reg;
        u8 val;
    } en;

    // The location of this bank's group select register.
    struct {
        u8 reg;
        u8 val;
    } gs;

    // The location of this bank's direction, data, and inversion registers.
    struct {
        u8 ld;
        u8 reg;
    } dir;

    // The location of this bank's pull-drain or open-drain register.
    struct {
        u8 ld;
        u8 reg;
    } ppod;

    // The masks for each bank pin.
    u8 pins[8];
};

static char const * const nuvoton_chip_names[] = {
    [nct6106d] = "nct6106d",
};

static int
sio_request(u16 addr)
{
    if (!request_muxed_region(addr, 2, DRVNAME)) {
        pr_err(
            DRVNAME ": IO port %#02x could not be reserved\n",
            addr
        );
        return -EBUSY;
    }

    outb(SIO_ENTER, addr);
    outb(SIO_ENTER, addr);
    return 0;
}

static void
sio_release(u16 addr)
{
    outb(SIO_EXIT, addr);
    release_region(addr, 2);
}

static inline void
sio_outb(u16 addr, u8 reg, u8 val)
{
    outb(reg, addr);
    outb(val, addr + 1);
}

static inline u8
sio_inb(u16 addr, u8 reg)
{
    outb(reg, addr);
    return inb(addr + 1);
}

static inline u16
sio_inw(u16 addr, u8 reg)
{
    u16 val;
    val = sio_inb(addr, reg) << 8;
    val |= sio_inb(addr, reg + 1);
    return val;
}

static inline void
sio_select(u16 addr, u8 ld)
{
    sio_outb(addr, SIO_LDSEL, ld);
}

static inline void
sio_set_bits(u16 addr, u8 reg, u8 mask, int on)
{
    u8 val;
    val = sio_inb(addr, reg);
    if (on)
        val |= mask;
    else
        val &= ~mask;
    outb(val, addr + 1);
}

static inline int
nuvoton_request(const struct nuvoton_bank *bank)
{
    return sio_request(bank->addr);
}

static inline void
nuvoton_release(const struct nuvoton_bank *bank)
{
    sio_release(bank->addr);
}

static inline void
nuvoton_select(const struct nuvoton_bank *bank, int reg)
{
    const auto gs = &bank->gs;
    const auto dir = &bank->dir;
    const auto ppod = &bank->ppod;

    switch (reg) {
        case NUVOTON_REG_DIR:
        case NUVOTON_REG_DAT:
        case NUVOTON_REG_INV:
            sio_select(bank->addr, dir->ld);
            if (gs->reg)
                sio_outb(bank->addr, gs->reg, gs->val);
            return;

        case NUVOTON_REG_PPOD:
            sio_select(bank->addr, ppod->ld);
            if (gs->reg)
                sio_outb(bank->addr, gs->reg, gs->val);
            return;
    }
}

static inline void
nuvoton_get_bits(const struct nuvoton_bank *bank, int reg, unsigned int off, u8 *bits)
{
    const auto dir = &bank->dir;
    const auto ppod = &bank->ppod;
    const u8 mask = bank->pins[off];

    switch (reg) {
        case NUVOTON_REG_DIR:
        case NUVOTON_REG_DAT:
        case NUVOTON_REG_INV:
            *bits = (sio_inb(bank->addr, dir->reg + reg) & mask);
            return;

        case NUVOTON_REG_PPOD:
            *bits = (sio_inb(bank->addr, ppod->reg) & mask);
            return;
    }
}

static inline void
nuvoton_set_bits(const struct nuvoton_bank *bank, int reg, unsigned int off, int on)
{
    const auto dir = &bank->dir;
    const auto ppod = &bank->ppod;
    const u8 mask = bank->pins[off];

    switch (reg) {
        case NUVOTON_REG_DIR:
        case NUVOTON_REG_DAT:
        case NUVOTON_REG_INV:
            sio_set_bits(bank->addr, dir->reg + reg, mask, on);
            return;

        case NUVOTON_REG_PPOD:
            sio_set_bits(bank->addr, ppod->reg, mask, on);
            return;
    }
}

static int
nuvoton_get_direction(struct gpio_chip *gc, unsigned int off)
{
    const struct nuvoton_bank *bank = gpiochip_get_data(gc);
    int ret;
    u8 bits;

    ret = nuvoton_request(bank);
    if (ret)
        return ret;

    nuvoton_select(bank, NUVOTON_REG_DIR);
    nuvoton_get_bits(bank, NUVOTON_REG_DIR, off, &bits);
    nuvoton_release(bank);
    return !!bits;
}

static int
nuvoton_direction_input(struct gpio_chip *gc, unsigned int off)
{
    const struct nuvoton_bank *bank = gpiochip_get_data(gc);
    int ret;

    ret = nuvoton_request(bank);
    if (ret)
        return ret;

    nuvoton_select(bank, NUVOTON_REG_DIR);
    nuvoton_set_bits(bank, NUVOTON_REG_DIR, off, 1);
    nuvoton_release(bank);
    return 0;
}

static int
nuvoton_direction_output(struct gpio_chip *gc, unsigned int off, int on)
{
    const struct nuvoton_bank *bank = gpiochip_get_data(gc);
    int ret;

    ret = nuvoton_request(bank);
    if (ret)
        return ret;

    nuvoton_select(bank, NUVOTON_REG_DIR);
    nuvoton_set_bits(bank, NUVOTON_REG_DAT, off, on);
    nuvoton_set_bits(bank, NUVOTON_REG_DIR, off, 0);
    nuvoton_release(bank);
    return 0;
}

static int
nuvoton_get(struct gpio_chip *gc, unsigned int off)
{
    const struct nuvoton_bank *bank = gpiochip_get_data(gc);
    int ret;
    u8 bits;

    ret = nuvoton_request(bank);
    if (ret)
        return ret;

    nuvoton_select(bank, NUVOTON_REG_DAT);
    nuvoton_get_bits(bank, NUVOTON_REG_DAT, off, &bits);
    nuvoton_release(bank);
    return !!bits;
}

static void
nuvoton_set(struct gpio_chip *gc, unsigned int off, int on)
{
    const struct nuvoton_bank *bank = gpiochip_get_data(gc);
    int ret;

    ret = nuvoton_request(bank);
    if (ret)
        return;

    nuvoton_select(bank, NUVOTON_REG_DAT);
    nuvoton_set_bits(bank, NUVOTON_REG_DAT, off, on);
    nuvoton_release(bank);
    return;
}

static int
nuvoton_set_config(struct gpio_chip *gc, unsigned int off, unsigned long cfg)
{
    const struct nuvoton_bank *bank = gpiochip_get_data(gc);
    const enum pin_config_param parm = pinconf_to_config_param(cfg);
    int on;
    int ret;

    if (parm == PIN_CONFIG_DRIVE_OPEN_DRAIN)
        on = 1;
    else if (parm == PIN_CONFIG_DRIVE_PUSH_PULL)
        on = 0;
    else
        return -ENOTSUPP;

    ret = nuvoton_request(bank);
    if (ret)
        return ret;

    nuvoton_select(bank, NUVOTON_REG_PPOD);
    nuvoton_set_bits(bank, NUVOTON_REG_PPOD, off, on);
    nuvoton_release(bank);
    return 0;
}

static const struct nuvoton_bank nct6106d_banks[] __initconst = {
    {
        .chip = NUVOTON_CHIP(8, "0"),
        .en   = {0x07, 0x30, 0x01},
        .dir  = {0x07, 0xe0},
        .ppod = {0x0f, 0xe0},
        .pins = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
    },
    {
        .chip = NUVOTON_CHIP(8, "1"),
        .en   = {0x07, 0x30, 0x02},
        .dir  = {0x07, 0xe4},
        .ppod = {0x0f, 0xe1},
        .pins = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
    },
    {
        .chip = NUVOTON_CHIP(8, "2"),
        .en   = {0x07, 0x30, 0x04},
        .dir  = {0x07, 0xe8},
        .ppod = {0x0f, 0xe2},
        .pins = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
    },
    {
        .chip = NUVOTON_CHIP(8, "3"),
        .en   = {0x07, 0x30, 0x08},
        .dir  = {0x07, 0xec},
        .ppod = {0x0f, 0xe3},
        .pins = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
    },
    {
        .chip = NUVOTON_CHIP(8, "4"),
        .en   = {0x07, 0x30, 0x10},
        .dir  = {0x07, 0xf0},
        .ppod = {0x0f, 0xe4},
        .pins = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
    },
    {
        .chip = NUVOTON_CHIP(8, "5"),
        .en   = {0x07, 0x30, 0x20},
        .dir  = {0x07, 0xf4},
        .ppod = {0x0f, 0xe5},
        .pins = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
    },
    {
        .chip = NUVOTON_CHIP(8, "6"),
        .en   = {0x07, 0x30, 0x40},
        .dir  = {0x07, 0xf8},
        .ppod = {0x0f, 0xe6},
        .pins = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
    },
    {
        .chip = NUVOTON_CHIP(1, "7"),
        .en   = {0x07, 0x30, 0x80},
        .dir  = {0x07, 0xfc},
        .ppod = {0x0f, 0xe7},
        .pins = {0x02},
    },
};

static int __init
nuvoton_probe_chip(u16 addr, struct nuvoton_sio *sio)
{
    int ret;
    u16 chip;

    ret = sio_request(addr);
    if (ret)
        return ret;

    ret = -ENODEV;

    chip = sio_inw(addr, SIO_CHIPID);
    switch (chip) {
        case SIO_NCT6106D_ID:
            sio->chip = nct6106d;
            break;

        case SIO_NOCHIP:
            pr_info(
                DRVNAME ": no chip found at IO port %#02x\n",
                addr
            );
            goto bail;

        default:
            pr_info(
                DRVNAME ": unknown chip (%#04x) found at IO port %#02x\n",
                chip,
                addr
            );
            goto bail;
    }

    ret = 0;

    sio->addr = addr;

    pr_info(
        DRVNAME ": %s chip (%#04x) at IO port %#02x\n",
        nuvoton_chip_names[sio->chip],
        chip,
        addr
    );

bail:
    sio_release(addr);
    return ret;
}

static int __init
nuvoton_find_chip(struct nuvoton_sio *sio)
{
    if (!nuvoton_probe_chip(0x2e, sio))
        return 0;

    if (!nuvoton_probe_chip(0x4e, sio))
        return 0;

    pr_err(DRVNAME ": no supported chips found\n");
    return -ENODEV;
}

static int __init
nuvoton_probe_bank(struct device *dev, const struct nuvoton_sio *sio, const struct nuvoton_bank *bank)
{
    const auto en = &bank->en;
    struct nuvoton_bank *copy;
    struct gpio_chip *chip;
    int ret;
    u8 val;

    ret = sio_request(sio->addr);
    if (ret)
        return ret;

    sio_select(sio->addr, en->ld);
    val = sio_inb(sio->addr, en->reg);
    sio_release(sio->addr);

    if (!(val & en->val))
        return 0;

    copy = devm_kmemdup(dev, bank, sizeof(*bank), GFP_KERNEL);
    if (!copy) {
        pr_err(DRVNAME ": failed to copy bank tables\n");
        return -ENOMEM;
    }

    copy->addr = sio->addr;
    chip = &copy->chip;

    ret = devm_gpiochip_add_data(dev, chip, copy);
    if (ret) {
        pr_err(
            DRVNAME ": failed to register gpiochip: %d\n",
            ret
        );
        return ret;
    }

    pr_info(
        "%s: registered bank with %d pin%c\n",
        chip->label,
        chip->ngpio,
        (chip->ngpio > 1) ? 's' : '\0'
    );

    return 0;
}

static int __init
nuvoton_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const struct nuvoton_sio *sio = dev_get_platdata(dev);
    const struct nuvoton_bank *banks;
    size_t nr_banks;
    int ret;

    switch (sio->chip) {
        case nct6106d:
            banks = nct6106d_banks;
            nr_banks = ARRAY_SIZE(nct6106d_banks);
            break;

        default:
            return -ENODEV;
    }

    for (size_t i = 0; i < nr_banks; i++) {
        ret = nuvoton_probe_bank(dev, sio, &banks[i]);
        if (ret)
            break;
    }

    return ret;
}

static struct platform_device *nuvoton_pdev;

static struct platform_driver nuvoton_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = DRVNAME,
    },
};

static int __init
nuvoton_init(void)
{
    int ret;
    struct nuvoton_sio sio;

    ret = nuvoton_find_chip(&sio);
    if (ret)
        return ret;

    struct platform_device_info pdi = {
        .name      = DRVNAME,
        .id        = PLATFORM_DEVID_NONE,
        .data      = &sio,
        .size_data = sizeof(sio),
    };

    nuvoton_pdev = platform_device_register_full(&pdi);
    ret = PTR_ERR_OR_ZERO(nuvoton_pdev);
    if (ret) {
        pr_err(
            DRVNAME ": failed to register platform device: %d\n",
            ret
        );
        return ret;
    }

    ret = platform_driver_probe(&nuvoton_driver, nuvoton_probe);
    if (ret) {
        pr_err(
            DRVNAME ": failed to register platform driver: %d\n",
            ret
        );
        platform_device_unregister(nuvoton_pdev);
        return ret;
    }

    return 0;
}
subsys_initcall(nuvoton_init);

static void __exit
nuvoton_exit(void)
{
    platform_device_unregister(nuvoton_pdev);
    platform_driver_unregister(&nuvoton_driver);
}
module_exit(nuvoton_exit);

MODULE_AUTHOR("James Buren <braewoods+ow@braewoods.net>");
MODULE_DESCRIPTION("GPIO driver for Nuvoton Super-I/O Chips");
MODULE_LICENSE("GPL");
