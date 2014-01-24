/*
 *  kernel/driver/char/modem_interface/modem_gpio_drv.c
 *
 *  Generic modem interface gpio handling.
 *
 *  Author:     Jiayong Yang(Jiayong.Yang@spreadtrum.com)
 *  Created:    Jul 27, 2012
 *  Copyright:  Spreadtrum Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/mutex.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <mach/modem_interface.h>
#include <mach/globalregs.h>
#include "modem_buffer.h"
#include "modem_interface_driver.h"
#include <mach/hardware.h>
#include <mach/pinmap.h>

#define GPIO_INVALID 0xFFFFFFFF

extern void modem_intf_send_GPIO_message(int gpio_no,int status,int index);
static unsigned int cp_alive_gpio_irq;
static unsigned int cp_crash_gpio_irq;
static unsigned int modem_power_gpio;
static int    cp_alive_gpio ;
static int    cp_crash_gpio;
static int    modem_is_poweron = 0;
extern void   register_cp_watchdog_handler(void);
extern void   unregister_cp_watchdog_handler(void);
int get_alive_status(void)
{
	int status = gpio_get_value(cp_alive_gpio);
	if(status != 0)
		return 1;
	return 0;
}
int get_assert_status(void)
{
	int status = gpio_get_value(cp_crash_gpio);
	if(status != 0)
		return 1;
	return 0;
}

/*
 * All rfctl pins that are not configured as gpio functionality were
 * pulled down during kernel initialization, so we have to call this
 * function to recover the default configuration defined in
 * rfctl-pinmap-board.h after modem become alive.
 * @see: pull_down_rfctl_nongpio_pins() defined in
 *       arch/arm/mach-sc8810/pinmap.c
 */
static int recover_rfctl_pinmap()
{
	pinmap_t tmp_rfctl_pinmap[] = {
		#include <rfctl-pinmap-board.h>
	};
        int i;
        for (i = 0; i < ARRAY_SIZE(tmp_rfctl_pinmap); i++) {
                __raw_writel(tmp_rfctl_pinmap[i].val, CTL_PIN_BASE
			+ tmp_rfctl_pinmap[i].reg);
        }
	return 0;
}

static irqreturn_t cp_alive_gpio_handle(int irq, void *handle)
{
	int status ;

	/* Use this flag to make sure that we call
	    recover_rfctl_pinmap() only once. */
	static bool rfctl_is_recovered = false;

	if (modem_is_poweron == 0)
		return IRQ_HANDLED;
	status = gpio_get_value(cp_alive_gpio);
	if (status == 0) {
		modem_intf_send_GPIO_message(cp_alive_gpio,status,0);
	} else {
		if (!rfctl_is_recovered) {
			/* Recover the default configuration after
			    modem become alive. */
			recover_rfctl_pinmap();
			rfctl_is_recovered = true;
		}
		modem_intf_send_GPIO_message(cp_alive_gpio,1,0);
	}
	return IRQ_HANDLED;
}

static irqreturn_t cp_crash_gpio_handle(int irq, void *handle)
{
	int status ;

	if(modem_is_poweron==0)
		return IRQ_HANDLED;
	status = gpio_get_value(cp_crash_gpio);
	printk(KERN_WARNING"cp_crash_irq: %d \n",status);
	if(status == 0)
		modem_intf_send_GPIO_message(cp_crash_gpio,status,0);
	else
		modem_intf_send_GPIO_message(cp_crash_gpio,1,0);
	return IRQ_HANDLED;
}
int modem_gpio_init(void *para)
{
	struct modem_intf_platform_data *modem_config = (struct nodem_intf_platform_data *)para;
	int    error = 0;
	int    status = 0;
    
	pr_debug("modem_gpio_init start!!! \n");
	cp_alive_gpio = modem_config->modem_boot_gpio;
	cp_crash_gpio = modem_config->modem_crash_gpio;
        modem_power_gpio = modem_config->modem_power_gpio;

	if(GPIO_INVALID != modem_power_gpio){
		gpio_request(modem_power_gpio, "modem_power_gpio");
		gpio_export(modem_power_gpio,0);
	}
	error = gpio_request(cp_alive_gpio, "modem_boot_gpio");
	if (error) {
		pr_err("Cannot request GPIO %d\n", cp_alive_gpio);
		gpio_free(cp_alive_gpio);
        	return error;
	}
	
	gpio_direction_input(cp_alive_gpio);
	gpio_export(cp_alive_gpio,0);
	cp_alive_gpio_irq = gpio_to_irq(cp_alive_gpio);
	if (cp_alive_gpio_irq < 0)
		return -1;
	error = request_threaded_irq(cp_alive_gpio_irq, cp_alive_gpio_handle, 
		NULL, IRQF_DISABLED|IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING, "modem_boot", para);
	if (error) {
		printk("lee :cannot alloc cp_alive_gpio_irq, err %d\r\n", error);
		gpio_free(cp_alive_gpio);
		return error;
	}

//////////////////////////////////////////////////
        error = gpio_request(cp_crash_gpio, "modem_crash_gpio");
        if (error) {
                pr_err("Cannot request GPIO %d\n", cp_crash_gpio);
                gpio_free(cp_crash_gpio);
                return error;
        }
        gpio_export(cp_crash_gpio,0);
        gpio_direction_input(cp_crash_gpio);

        cp_crash_gpio_irq = gpio_to_irq(cp_crash_gpio);
        if (cp_crash_gpio_irq < 0)
                return -1;
        error = request_threaded_irq(cp_crash_gpio_irq, cp_crash_gpio_handle,
                NULL, IRQF_DISABLED|IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING, "modem_crash", para);
        if (error) {
                printk("lee :cannot alloc cp_crash irq, err %d\r\n", error);
                gpio_free(cp_crash_gpio);
                return error;
        }

/////////////////////////////////////////////////
        status = gpio_get_value(cp_alive_gpio);
        if(status == 0)
                modem_intf_send_GPIO_message(cp_alive_gpio,status,0);
        else
                modem_intf_send_GPIO_message(cp_alive_gpio,1,0);

	return 0;
}

void modem_poweron(void)
{
	printk("modem power on!!! \n");
	if(GPIO_INVALID != modem_power_gpio){
		gpio_direction_output(modem_power_gpio, 1); //2012.1.10
		gpio_set_value(modem_power_gpio, 1);
	} else {
		sprd_greg_clear_bits(REG_TYPE_GLOBAL, 0x00000040, 0x4C);
	}
	register_cp_watchdog_handler();
	modem_is_poweron = 1;
}
void modem_poweroff(void)
{
	printk("modem power off!!! \n");
	modem_is_poweron = 0;
	unregister_cp_watchdog_handler();
	if (GPIO_INVALID != modem_power_gpio){
		gpio_direction_output(modem_power_gpio, 0); //2012.1.10
		gpio_set_value(modem_power_gpio, 0);
	} else {
		sprd_greg_set_bits(REG_TYPE_GLOBAL, 0x00000040, 0x4C);
	}
}
