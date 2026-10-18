/*
 * Datto DNA Platform Driver
 *
 * Author: James Buren <braewoods+ow@braewoods.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/property.h>
#include <linux/gpio/property.h>
#include <linux/gpio/machine.h>
#include <linux/platform_device.h>
#include <linux/input.h>

#define DRVNAME "datto-dna"

static unsigned int datto_dna_code = KEY_RESTART;

static const struct software_node datto_dna_led_gpio_node = {
	.name = "gpio-nct5104d-1",
};

static const struct software_node datto_dna_leds_node = {
	.name = DRVNAME "-leds",
};

static const struct property_entry datto_dna_led_cloud_props[] = {
	PROPERTY_ENTRY_STRING("label", DRVNAME ":green:cloud"),
	PROPERTY_ENTRY_GPIO("gpios", &datto_dna_led_gpio_node, 0, GPIO_ACTIVE_LOW),
	{},
};

static const struct software_node datto_dna_led_cloud_node = {
	.name = DRVNAME "-led-cloud",
	.parent = &datto_dna_leds_node,
	.properties = datto_dna_led_cloud_props,
};

static const struct property_entry datto_dna_led_wwan_props[] = {
	PROPERTY_ENTRY_STRING("label", DRVNAME ":green:wwan"),
	PROPERTY_ENTRY_GPIO("gpios", &datto_dna_led_gpio_node, 1, GPIO_ACTIVE_LOW),
	{},
};

static const struct software_node datto_dna_led_wwan_node = {
	.name = DRVNAME "-led-wwan",
	.parent = &datto_dna_leds_node,
	.properties = datto_dna_led_wwan_props,
};

static const struct software_node datto_dna_key_gpio_node = {
	.name = "gpio_ich",
};

static const struct property_entry datto_dna_keys_props[] = {
	PROPERTY_ENTRY_U32("poll-interval", 100),
	PROPERTY_ENTRY_STRING("label", "Datto DNA Keys"),
	{},
};

static const struct software_node datto_dna_keys_node = {
	.name = DRVNAME "-keys",
	.properties = datto_dna_keys_props,
};

// It is not const so the event key code can be set at runtime.
static struct property_entry datto_dna_key_event_props[] = {
	PROPERTY_ENTRY_U32("linux,code", KEY_RESERVED),
	PROPERTY_ENTRY_STRING("label", "Datto DNA Key Event"),
	PROPERTY_ENTRY_GPIO("gpios", &datto_dna_key_gpio_node, 49, GPIO_ACTIVE_LOW),
	PROPERTY_ENTRY_U32("linux,input-type", EV_KEY),
	PROPERTY_ENTRY_U32("debounce-interval", 10),
	{},
};

static const struct software_node datto_dna_key_event_node = {
	.name = DRVNAME "-key-event",
	.parent = &datto_dna_keys_node,
	.properties = datto_dna_key_event_props,
};

static const struct software_node *datto_dna_swnodes[] = {
	&datto_dna_led_gpio_node,
	&datto_dna_leds_node,
	&datto_dna_led_cloud_node,
	&datto_dna_led_wwan_node,
	&datto_dna_key_gpio_node,
	&datto_dna_keys_node,
	&datto_dna_key_event_node,
	NULL,
};

static struct platform_device *datto_dna_led_pdev;
static struct platform_device *datto_dna_key_pdev;

static int __init
datto_dna_code_verify(void)
{
	if (datto_dna_code == KEY_RESERVED || datto_dna_code > KEY_MAX) {
		pr_err(
			DRVNAME ": key code must not be %u or greater than %u\n",
			KEY_RESERVED,
			KEY_MAX
		);
		return -EINVAL;
	}

	datto_dna_key_event_props[0].value.u32_data[0] = datto_dna_code;
	return 0;
}

static int __init
datto_dna_init(void)
{
	struct platform_device_info led_pdi = {
		.name = "leds-gpio",
		.id = PLATFORM_DEVID_NONE,
	};
	struct platform_device_info key_pdi = {
		.name = "gpio-keys-polled",
		.id = PLATFORM_DEVID_NONE,
	};
	int ret = 0;

	ret = datto_dna_code_verify();
	if (ret)
		goto bail;

	ret = software_node_register_node_group(datto_dna_swnodes);
	if (ret) {
		pr_err(DRVNAME ": failed to register software nodes: %d\n", ret);
		goto bail;
	}

	led_pdi.fwnode = software_node_fwnode(&datto_dna_leds_node);
	datto_dna_led_pdev = platform_device_register_full(&led_pdi);
	ret = PTR_ERR_OR_ZERO(datto_dna_led_pdev);
	if (ret) {
		pr_err(DRVNAME ": failed to register LED device: %d\n", ret);
		goto bail_swnodes;
	}

	key_pdi.fwnode = software_node_fwnode(&datto_dna_keys_node);
	datto_dna_key_pdev = platform_device_register_full(&key_pdi);
	ret = PTR_ERR_OR_ZERO(datto_dna_key_pdev);
	if (ret) {
		pr_err(DRVNAME ": failed to register key device: %d\n", ret);
		goto bail_led;
	}

	return ret;

bail_led:
	platform_device_unregister(datto_dna_led_pdev);
bail_swnodes:
	software_node_unregister_node_group(datto_dna_swnodes);
bail:
	return ret;
}

static void __exit
datto_dna_exit(void)
{
	platform_device_unregister(datto_dna_key_pdev);
	platform_device_unregister(datto_dna_led_pdev);
	software_node_unregister_node_group(datto_dna_swnodes);
}

module_param_named(code, datto_dna_code, uint, 0444);
MODULE_PARM_DESC(code, "key code to emit when button is pressed");

module_init(datto_dna_init);
module_exit(datto_dna_exit);

MODULE_AUTHOR("James Buren <braewoods+ow@braewoods.net>");
MODULE_DESCRIPTION("Datto DNA Platform Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRVNAME);
