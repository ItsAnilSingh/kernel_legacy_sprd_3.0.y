/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach/time.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <mach/hardware.h>
#include <linux/i2c.h>
#include <linux/i2c/ft5306_ts.h>
#if(defined(CONFIG_INPUT_AL3006_I2C))
#include <linux/i2c/al3006_pls.h>
#endif
#if(defined(CONFIG_INPUT_LTR558_I2C)||defined(CONFIG_INPUT_LTR558_I2C_MODULE))
#include <linux/i2c/ltr_558als.h>
#endif
#include <linux/i2c/lis3dh.h>
#if(defined(CONFIG_SENSORS_AK8975)||defined(CONFIG_SENSORS_AK8975_MODULE))
#include <linux/akm8975.h>
#endif
#if(defined(CONFIG_SENSORS_AK8963)||defined(CONFIG_SENSORS_AK8963_MODULE))
#include <linux/akm8963.h>
#endif
#include <linux/spi/spi.h>
#include <mach/adc.h>
#include <mach/globalregs.h>
#include <mach/irqs.h>
#include <mach/board.h>
#include <sound/audio_pa.h>
#include "../devices.h"
#include <mach/serial_sprd.h>
#include <gps/gpsctl.h>
#include <linux/interrupt.h>
#include <linux/headset_sprd.h>

extern void __init sc8810_reserve(void);
extern void __init sc8810_map_io(void);
extern void __init sc8810_init_irq(void);
extern void __init sc8810_timer_init(void);
extern int __init sci_regulator_init(void);
extern void __init sc8810_clock_init(void);
extern void __init sc8810_init_ana_chipid(void);
#ifdef CONFIG_ANDROID_RAM_CONSOLE
extern int __init sprd_ramconsole_init(void);
#endif
static int	cp_watchdog_flag = 0;
static struct platform_device rfkill_device;
static struct platform_device kb_backlight_device;

static struct platform_gpsctl_data pdata_gpsctl = {
	.reset_pin = GPIO_GPS_RESET,
	.onoff_pin = GPIO_GPS_ONOFF,
	.clk_type  = "clk_aux0",
	.pwr_type  = "vddwif0",
};

static struct platform_device  gpsctl_dev = {
	.name               = "gpsctl",
	.dev.platform_data  = &pdata_gpsctl,
};

#include <mach/modem_interface.h>
struct modem_intf_platform_data modem_interface = {
        .dev_type               = MODEM_DEV_SDIO,
        .modem_dev_parameter    = NULL,
        .modem_power_gpio       = GPIO_INVALID,
        .modem_boot_gpio        = GPIO_MODEM_BOOT,
        .modem_crash_gpio       = GPIO_MODEM_CRASH,
};

static struct platform_device modem_interface_device = {
       .name   = "modem_interface",
       .id     = -1,
       .dev    = {
               .platform_data  = &modem_interface,
       },
};

static struct resource ipc_sdio_resources[] = {
	[0] = {
		.start = SPRD_SPI0_PHYS,
		.end = SPRD_SPI0_PHYS + SZ_4K - 1,

	    .flags = IORESOURCE_MEM,
	       },
};

static struct platform_device ipc_sdio_device = {
	.name = "ipc_sdio",
	.id = 0,
	.num_resources = ARRAY_SIZE(ipc_sdio_resources),
	.resource = ipc_sdio_resources,
};


static struct headset_button sprd_headset_button[] = {
	{
		.adc_min			= 0x0000,
		.adc_max			= 0x00C8,
		.code			= KEY_MEDIA,
	},
	{
		.adc_min			= 0x00C9,
		.adc_max			= 0x02BC,
		.code			= KEY_VOLUMEUP,
	},
	{
		.adc_min			= 0x02BD,
		.adc_max			= 0x0514,
		.code			= KEY_VOLUMEDOWN,
	},
};

static struct sprd_headset_buttons_platform_data sprd_headset_button_data = {
	.headset_button	= sprd_headset_button,
	.nbuttons	= ARRAY_SIZE(sprd_headset_button),
};

static struct sprd_headset_detect_platform_data sprd_headset_detect_data = {
	.switch_gpio	= HEADSET_SWITCH_GPIO,
	.detect_gpio	= HEADSET_DETECT_GPIO,
	.button_gpio	= HEADSET_BUTTON_GPIO,
	.detect_active_low	= 1,
	.button_active_low	= 1,
};

static struct platform_device sprd_headset_button_device = {
	.name	= "headset-buttons",
	.id	= -1,
	.dev	= {
		.platform_data	= &sprd_headset_button_data,
	},
};

static struct platform_device sprd_headset_detect_device = {
	.name	= "headset-detect",
	.id	= -1,
	.dev	= {
		.platform_data	= &sprd_headset_detect_data,
	},
};
static struct platform_device *devices[] __initdata = {
	&sprd_serial_device0,
	&sprd_serial_device1,
	&sprd_serial_device2,
	&sprd_serial_device3,
	&sprd_device_rtc,
	&sprd_nand_device,
	&sprd_lcd_device0,
	&sprd_backlight_device,
	&sprd_i2c_device0,
	&sprd_i2c_device1,
	&sprd_i2c_device2,
	&sprd_i2c_device3,
	&sprd_spi0_device,
	&sprd_spi1_device,
	&sprd_keypad_device,
	&sprd_audio_platform_pcm_device,
	&sprd_audio_cpu_dai_vaudio_device,
	&sprd_audio_cpu_dai_vbc_device,
	&sprd_audio_codec_sprd_codec_device,
	&sprd_audio_cpu_dai_i2s_device,
	&sprd_audio_cpu_dai_i2s_device1,
	&sprd_audio_codec_null_codec_device,
	&sprd_battery_device,
#ifdef CONFIG_ANDROID_PMEM
	&sprd_pmem_device,
	&sprd_pmem_adsp_device,
#endif
#ifdef CONFIG_ION
	&sprd_ion_dev,
#endif
	//&sprd_emmc0_device,
	&sprd_sdio0_device,
	&sprd_sdio1_device,
	&sprd_sdio2_device,
	&sprd_vsp_device,
	&sprd_dcam_device,
	&sprd_scale_device,
	&sprd_rotation_device,
	&kb_backlight_device,
	&gpsctl_dev,

        &modem_interface_device,
        &ipc_sdio_device,
		&rfkill_device,
	&sprd_headset_detect_device,
	&sprd_headset_button_device,
};

/* RFKILL */
static struct resource rfkill_resources[] = {
	{
		.name   = "bt_reset",
		.start  = GPIO_BT_RESET,
		.end    = GPIO_BT_RESET,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device rfkill_device = {
     .name = "rfkill",
     .id = -1,
     .num_resources  = ARRAY_SIZE(rfkill_resources),
     .resource   = rfkill_resources,
};

/* keypad backlight */
static struct platform_device kb_backlight_device = {
	.name           = "keyboard-backlight",
	.id             =  -1,
};

static struct sys_timer sc8810_timer = {
	.init = sc8810_timer_init,
};

static int calibration_mode = false;
static int __init calibration_start(char *str)
{
	if(str)
		pr_info("modem calibartion:%s\n", str);
	calibration_mode = true;
	return 1;
}
__setup("calibration=", calibration_start);

int in_calibration(void)
{
	return (calibration_mode == true);
}

EXPORT_SYMBOL(in_calibration);

static void __init sprd_add_otg_device(void)
{
	/*
	 * if in calibrtaion mode, we do nothing, modem will handle everything
	 */
	if (calibration_mode)
		return;
	platform_device_register(&sprd_otg_device);
}

static struct serial_data plat_data0 = {
	.wakeup_type = BT_RX_WAKE_UP,
	.clk = 48000000,
};
static struct serial_data plat_data1 = {
	.wakeup_type = BT_NO_WAKE_UP,
	.clk = 26000000,
};
static struct serial_data plat_data2 = {
	.wakeup_type = BT_NO_WAKE_UP,
	.clk = 26000000,
};

static struct serial_data plat_data3 = {
	.wakeup_type = BT_NO_WAKE_UP,
	.clk = 26000000,
};

static struct ft5x0x_ts_platform_data ft5x0x_ts_info = {
	.irq_gpio_number	= GPIO_TOUCH_IRQ,
	.reset_gpio_number	= GPIO_TOUCH_RESET,
	.vdd_name			= "vdd28",
};

#if(defined(CONFIG_INPUT_AL3006_I2C))
static struct al3006_pls_platform_data al3006_pls_info = {
	.irq_gpio_number	= GPIO_PLSENSOR_IRQ,
};
#endif

#if(defined(CONFIG_INPUT_LTR558_I2C)||defined(CONFIG_INPUT_LTR558_I2C_MODULE))
static struct ltr558_pls_platform_data ltr558_pls_info = {
	.irq_gpio_number	= GPIO_PLSENSOR_IRQ,
};
#endif

static struct i2c_board_info i2c2_boardinfo[] = {
{  I2C_BOARD_INFO("BEKEN_FM", 0x70),  },
};

static struct i2c_board_info i2c3_boardinfo[] = {
	{
		I2C_BOARD_INFO(FT5206_TS_DEVICE, FT5206_TS_ADDR),
		.platform_data = &ft5x0x_ts_info,
	},
};

static struct lis3dh_acc_platform_data lis3dh_plat_data = {
	.poll_interval = 20,
	.min_interval = 10,
	.g_range = LIS3DH_ACC_G_2G,
	.axis_map_x = 1,
	.axis_map_y = 0,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 1
};

#if(defined(CONFIG_SENSORS_AK8975)||defined(CONFIG_SENSORS_AK8975_MODULE))
struct akm8975_platform_data akm8975_platform_d = {
	.mag_low_x = -20480,
	.mag_high_x = 20479,
	.mag_low_y = -20480,
	.mag_high_y = 20479,
	.mag_low_z = -20480,
	.mag_high_z = 20479,
};
#endif

#if(defined(CONFIG_SENSORS_AK8963)||defined(CONFIG_SENSORS_AK8963_MODULE))
struct akm8963_platform_data akm_platform_data_8963 = {
       .layout = 3,
       .outbit = 1,
       .gpio_DRDY = MSENSOR_DRDY_GPIO,
       .gpio_RST = MSENSOR_RSTN_GPIO,
};
#endif

static struct i2c_board_info i2c1_boardinfo[] = {
	{I2C_BOARD_INFO("sensor_main",0x3C),},
	{I2C_BOARD_INFO("sensor_sub",0x21),},
        {I2C_BOARD_INFO("nmiatv",0x60),},/*atv*/ 
};

static struct i2c_board_info i2c0_boardinfo[] = {
	{ I2C_BOARD_INFO(LIS3DH_ACC_I2C_NAME, LIS3DH_ACC_I2C_ADDR),
	  .platform_data = &lis3dh_plat_data,
	},
#if(defined(CONFIG_SENSORS_AK8975)||defined(CONFIG_SENSORS_AK8975_MODULE))
	{ I2C_BOARD_INFO(AKM8975_I2C_NAME,    AKM8975_I2C_ADDR),
	  .platform_data = &akm8975_platform_d,
	},
#endif
#if(defined(CONFIG_SENSORS_AK8963)||defined(CONFIG_SENSORS_AK8963_MODULE))
		{ I2C_BOARD_INFO(AKM8963_I2C_NAME,	  AKM8963_I2C_ADDR),
		  .platform_data = &akm_platform_data_8963,
		},
#endif
#if(defined(CONFIG_INPUT_AL3006_I2C))
	{ I2C_BOARD_INFO(AL3006_PLS_DEVICE,   AL3006_PLS_ADDRESS),
	  .platform_data = &al3006_pls_info,
	},
#endif
#if(defined(CONFIG_INPUT_LTR558_I2C)||defined(CONFIG_INPUT_LTR558_I2C_MODULE))
	{ I2C_BOARD_INFO(LTR558_I2C_NAME,  LTR558_I2C_ADDR),
	  .platform_data = &ltr558_pls_info,
	},
#endif
};

/* config I2C2 SDA/SCL to SIM2 pads */
static void sprd8810_i2c2sel_config(void)
{
	sprd_greg_set_bits(REG_TYPE_GLOBAL, PINCTRL_I2C2_SEL, GR_PIN_CTL);
}

static int sc8810_add_i2c_devices(void)
{
	sprd8810_i2c2sel_config();
	i2c_register_board_info(3, i2c3_boardinfo, ARRAY_SIZE(i2c3_boardinfo));
	i2c_register_board_info(2, i2c2_boardinfo, ARRAY_SIZE(i2c2_boardinfo));
	i2c_register_board_info(1, i2c1_boardinfo, ARRAY_SIZE(i2c1_boardinfo));
	i2c_register_board_info(0, i2c0_boardinfo, ARRAY_SIZE(i2c0_boardinfo));
	return 0;
}

struct platform_device audio_pa_amplifier_device = {
	.name = "speaker-pa",
	.id = -1,
};

static int audio_pa_amplifier_speaker(int cmd, void *data)
{
	int ret = 0;
	if (cmd < 0) {
		/* get speaker amplifier status : enabled or disabled */
		ret = 0;
	} else {
		/* set speaker amplifier */
	}
	return ret;
}

static _audio_pa_control audio_pa_control = {
	.speaker = {
		.init = NULL,
		.control = NULL,
	},
	.earpiece = {
		.init = NULL,
		.control = NULL,
	},
	.headset = {
		.init = NULL,
		.control = NULL,
	},
};

static int spi_cs_gpio_map[][2] = {
    {SPI0_CMMB_CS_GPIO,  0},
} ;

static struct spi_board_info spi_boardinfo[] = {
	{
	.modalias = "cmmb-dev",
	.bus_num = 0,
	.chip_select = 0,
	.max_speed_hz = 1000 * 1000,
	.mode = SPI_CPOL | SPI_CPHA,
	}
};

static void sprd_spi_init(void)
{
	int busnum, cs, gpio;
	int i;

	struct spi_board_info *info = spi_boardinfo;

	for (i = 0; i < ARRAY_SIZE(spi_boardinfo); i++) {
		busnum = info[i].bus_num;
		cs = info[i].chip_select;
		gpio   = spi_cs_gpio_map[busnum][cs];

		info[i].controller_data = (void *)gpio;
	}

        spi_register_board_info(info, ARRAY_SIZE(spi_boardinfo));
}

static int sc8810_add_misc_devices(void)
{
	if (audio_pa_control.speaker.control || audio_pa_control.earpiece.control || \
		audio_pa_control.headset.control) {
		platform_set_drvdata(&audio_pa_amplifier_device, &audio_pa_control);
		if (platform_device_register(&audio_pa_amplifier_device))
			pr_err("faile to install audio_pa_amplifier_device\n");
	}
	return 0;
}

static irqreturn_t cp_watchdog_handle(int irq, void *dev)
{
	extern void modem_intf_channel_indicate_message(int dir,int para,int index);
        int32_t retval = IRQ_HANDLED;
	modem_intf_channel_indicate_message(0,0,0);
	printk("CP_WATCHDOG Reset ...\n");
        return retval;
}
void unregister_cp_watchdog_handler(void)
{
	printk("%s(%d)\n",__func__,cp_watchdog_flag);
	if(cp_watchdog_flag){
		free_irq(IRQ_CP_WDG_INT,NULL);
		printk("free_irq CP watchdog reset\n");
	}
	cp_watchdog_flag = 0;
}
void register_cp_watchdog_handler(void)
{
	extern void sprd_turnon_watchdog(void);
        int retval;

	printk("%s(%d)\n",__func__,cp_watchdog_flag);
	if(cp_watchdog_flag==1)
		return;
	cp_watchdog_flag = 1;
	retval = request_irq(IRQ_CP_WDG_INT, cp_watchdog_handle,
                        IRQF_ONESHOT|IRQF_DISABLED, NULL,NULL);
        if (retval != 0) {
                pr_err("request of irq%d failed\n", IRQ_CP_WDG_INT);
        }
}
static void __init sc8810_init_machine(void)
{
	sci_adc_init((void __iomem *)ADC_REG_BASE);
	sci_regulator_init();
	sprd_add_otg_device();
	platform_device_add_data(&sprd_serial_device0,(const void*)&plat_data0,sizeof(plat_data0));
	platform_device_add_data(&sprd_serial_device1,(const void*)&plat_data1,sizeof(plat_data1));
	platform_device_add_data(&sprd_serial_device2,(const void*)&plat_data2,sizeof(plat_data2));
	platform_device_add_data(&sprd_serial_device3,(const void*)&plat_data3,sizeof(plat_data3));
	platform_add_devices(devices, ARRAY_SIZE(devices));
	sc8810_add_i2c_devices();
	sc8810_add_misc_devices();
	sprd_spi_init();
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	sprd_ramconsole_init();
#endif
	
}

static void __init sc8810_fixup(struct machine_desc *desc, struct tag *tag,
		char **cmdline, struct meminfo *mi)
{
}

static void __init sc8810_init_early(void)
{
	/* earlier init request than irq and timer */
	sc8810_clock_init();

        /* init ana chip id */
	sc8810_init_ana_chipid();
}

MACHINE_START(SC8810OPENPHONE, "sc7710g")
	.reserve	= sc8810_reserve,
	.map_io		= sc8810_map_io,
	.init_irq	= sc8810_init_irq,
	.timer		= &sc8810_timer,
	.init_machine	= sc8810_init_machine,
	.fixup		= sc8810_fixup,
	.init_early	= sc8810_init_early,
MACHINE_END
