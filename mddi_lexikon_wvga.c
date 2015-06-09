/* 
 * adapted from arch/arm/mach-msm/board-lexikon-panel.c
 * and drivers/video/msm/lcdc_spade_wvga.c
 *
 * Lexikon MDDI Panel Driver for "new" framebuffer
 *
 * Copyright (c) 2009 Google Inc.
 * Copyright (c) 2009 HTC.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 */

#include <linux/interrupt.h>
#include <linux/wakelock.h> 
#include <linux/leds.h> 
#include <linux/delay.h> 
#include <linux/init.h> 
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <mach/panel_id.h> 
#include "msm_fb.h" 
#include "mddihost.h" 
#include "mddihosti.h"

#define write_client_reg(val, reg) mddi_queue_register_write(reg, val, FALSE, 0);

#define PWM_USER_DEF	 		143
#define PWM_USER_MIN			30
#define PWM_USER_MAX			255

#define PWM_PANEL_DEF			135
#define PWM_PANEL_MIN			9
#define PWM_PANEL_MAX			255

#define DEFAULT_BRIGHTNESS_LEVEL PWM_USER_DEF

static int lexikon_adjust_backlight(enum led_brightness val);

extern int panel_type;
static DEFINE_MUTEX(panel_lock);
static atomic_t bl_ready = ATOMIC_INIT(1);
static uint8_t last_val = BRIGHTNESS_DEFAULT_LEVEL;
static bool screen_on = true;
/* use one flag to have better backlight on/off performance */
static int lexikon_set_dim = 1;

#if 0
static int client_auto_hibernate(int on) {
    mddi_host_reg_out(CMD, MDDI_CMD_HIBERNATE | !!on);
    return 0;
}
#endif

enum {
    PANEL_LEXIKON_SHARP,
    PANEL_LEXIKON_SHARP_CUT2,
    PANEL_LEXIKON_SONY,
    PANEL_LEXIKON_SHARP_CUT2,
    PANEL_UNKNOWN
};

#define REG_WAIT (0xffff)

static struct nov_regs {
    unsigned reg;
    unsigned val;
};

static struct nov_regs sharp_init_seq[] = {
	{0x1100, 0x00},
	{REG_WAIT, 120},
	{0x89C3, 0x80},
	{0x92C2, 0x08},
	{0x0180, 0x14},
	{0x0280, 0x11},
	{0x0380, 0x33},
	{0x0480, 0x63},
	{0x0580, 0x63},
	{0x0680, 0x63},
	{0x0780, 0x00},
	{0x0880, 0x44},
	{0x0980, 0x54},
	{0x0A80, 0x10},
	{0x0B80, 0x55},
	{0x0C80, 0x55},
	{0x0D80, 0x30},
	{0x0E80, 0x44},
	{0x0F80, 0x54},
	{0x1080, 0x30},
	{0x1280, 0x77},
	{0x1380, 0x21},
	{0x1480, 0x0E},
	{0x1580, 0x98},
	{0x1680, 0xCC},
	{0x1780, 0x00},
	{0x1880, 0x00},
	{0x1980, 0x00},
	{0x1C80, 0x00},
	{0x1F80, 0x05},
	{0x2480, 0x1A},
	{0x2580, 0x1F},
	{0x2680, 0x2D},
	{0x2780, 0x3E},
	{0x2880, 0x0D},
	{0x2980, 0x21},
	{0x2A80, 0x58},
	{0x2B80, 0x2A},
	{0x2D80, 0x20},
	{0x2F80, 0x27},
	{0x3080, 0x61},
	{0x3180, 0x17},
	{0x3280, 0x37},
	{0x3380, 0x53},
	{0x3480, 0x5A},
	{0x3580, 0x8E},
	{0x3680, 0xA7},
	{0x3780, 0x3E},
	{0x3880, 0x2B},
	{0x3980, 0x2E},
	{0x3A80, 0x36},
	{0x3B80, 0x41},
	{0x3D80, 0x1A},
	{0x3F80, 0x2D},
	{0x4080, 0x5D},
	{0x4180, 0x3D},
	{0x4280, 0x20},
	{0x4380, 0x27},
	{0x4480, 0x76},
	{0x4580, 0x17},
	{0x4680, 0x39},
	{0x4780, 0x55},
	{0x4880, 0x71},
	{0x4980, 0xA6},
	{0x4A80, 0xBF},
	{0x4B80, 0x55},
	{0x4C80, 0x55},
	{0x4D80, 0x58},
	{0x4E80, 0x5F},
	{0x4F80, 0x66},
	{0x5080, 0x18},
	{0x5180, 0x26},
	{0x5280, 0x57},
	{0x5380, 0x3D},
	{0x5480, 0x1E},
	{0x5580, 0x26},
	{0x5680, 0x6B},
	{0x5780, 0x17},
	{0x5880, 0x3B},
	{0x5980, 0x4F},
	{0x5A80, 0x5A},
	{0x5B80, 0x8E},
	{0x5C80, 0xA7},
	{0x5D80, 0x3E},
	{0x5E80, 0x66},
	{0x5F80, 0x68},
	{0x5500, 0x02},
	{0x5E00, 0x09},
	{0x1DC0, 0x27},
	{0x6080, 0x6C},
	{0x6180, 0x6E},
	{0x6280, 0x16},
	{0x6380, 0x2A},
	{0x6480, 0x59},
	{0x6580, 0x4C},
	{0x6680, 0x1E},
	{0x6780, 0x25},
	{0x6880, 0x7B},
	{0x6980, 0x17},
	{0x6A80, 0x3A},
	{0x6B80, 0x53},
	{0x6C80, 0x71},
	{0x6D80, 0xA6},
	{0x6E80, 0xBF},
	{0x6F80, 0x55},
	{0x7080, 0x63},
	{0x7180, 0x66},
	{0x7280, 0x70},
	{0x7380, 0x76},
	{0x7480, 0x18},
	{0x7580, 0x27},
	{0x7680, 0x58},
	{0x7780, 0x47},
	{0x7880, 0x1E},
	{0x7980, 0x25},
	{0x7A80, 0x72},
	{0x7B80, 0x18},
	{0x7C80, 0x3B},
	{0x7D80, 0x4C},
	{0x7E80, 0x5A},
	{0x7F80, 0x8E},
	{0x8080, 0xA7},
	{0x8180, 0x3E},
	{0x8280, 0x75},
	{0x8380, 0x77},
	{0x8480, 0x7C},
	{0x8580, 0x7E},
	{0x8680, 0x16},
	{0x8780, 0x2C},
	{0x8880, 0x5C},
	{0x8980, 0x55},
	{0x8A80, 0x1F},
	{0x8B80, 0x24},
	{0x8C80, 0x82},
	{0x8D80, 0x15},
	{0x8E80, 0x38},
	{0x8F80, 0x50},
	{0x9080, 0x71},
	{0x9180, 0xA6},
	{0x9280, 0xBF},
	{0x9380, 0x55},
	{0x9480, 0xB5},
	{0x9580, 0x04},
	{0x9680, 0x18},
	{0x9780, 0xB0},
	{0x9880, 0x20},
	{0x9980, 0x28},
	{0x9A80, 0x08},
	{0x9B80, 0x04},
	{0x9C80, 0x12},
	{0x9D80, 0x00},
	{0x9E80, 0x00},
	{0x9F80, 0x12},
	{0xA080, 0x00},
	{0xA280, 0x00},
	{0xA380, 0x3C},
	{0xA480, 0x01},
	{0xA580, 0xC0},
	{0xA680, 0x01},
	{0xA780, 0x00},
	{0xA980, 0x00},
	{0xAA80, 0x00},
	{0xAB80, 0x70},
	{0xE780, 0x11},
	{0xE880, 0x11},
	{0xED80, 0x0A},
	{0xEE80, 0x80},
	{0xF780, 0x0D},
	{0x2900, 0x00},
	{0x3500, 0x00},
	{0x4400, 0x02},
	{0x4401, 0x58},
};

static struct nov_regs sony_init_seq[] = {
	{0x1100, 0x00},
	{REG_WAIT, 120},
	{0x0480, 0x63},
	{0x0580, 0x63},
	{0x0680, 0x63},
	{0x2480, 0x52},
	{0x2580, 0x56},
	{0x2680, 0x62},
	{0x2780, 0x6B},
	{0x2880, 0x17},
	{0x2980, 0x2A},
	{0x2A80, 0x5B},
	{0x2B80, 0x6B},
	{0x2D80, 0x1E},
	{0x2F80, 0x25},
	{0x3080, 0xB2},
	{0x3180, 0x1C},
	{0x3280, 0x48},
	{0x3380, 0x5E},
	{0x3480, 0xC2},
	{0x3580, 0xD2},
	{0x3680, 0xEF},
	{0x3780, 0x7F},
	{0x3880, 0x52},
	{0x3980, 0x56},
	{0x3A80, 0x62},
	{0x3B80, 0x6B},
	{0x3D80, 0x17},
	{0x3F80, 0x2A},
	{0x4080, 0x5B},
	{0x4180, 0x6B},
	{0x4280, 0x1E},
	{0x4380, 0x25},
	{0x4480, 0xB2},
	{0x4580, 0x1C},
	{0x4680, 0x48},
	{0x4780, 0x5E},
	{0x4880, 0xC2},
	{0x4980, 0xD2},
	{0x4A80, 0xEF},
	{0x4B80, 0x7F},
	{0x4C80, 0x3F},
	{0x4D80, 0x44},
	{0x4E80, 0x53},
	{0x4F80, 0x60},
	{0x5080, 0x17},
	{0x5180, 0x2A},
	{0x5280, 0x5C},
	{0x5380, 0x68},
	{0x5480, 0x1F},
	{0x5580, 0x25},
	{0x5680, 0xB1},
	{0x5780, 0x1D},
	{0x5880, 0x4C},
	{0x5980, 0x63},
	{0x5A80, 0xBA},
	{0x5B80, 0xC8},
	{0x5C80, 0xFA},
	{0x5D80, 0x7F},
	{0x5E80, 0x3F},
	{0x5F80, 0x44},
	{0x5500, 0x02},
	{0x5E00, 0x09},
	{0x1DC0, 0x27},
	{0x6080, 0x53},
	{0x6180, 0x60},
	{0x6280, 0x17},
	{0x6380, 0x2A},
	{0x6480, 0x5C},
	{0x6580, 0x68},
	{0x6680, 0x1F},
	{0x6780, 0x25},
	{0x6880, 0xB1},
	{0x6980, 0x1D},
	{0x6A80, 0x4C},
	{0x6B80, 0x63},
	{0x6C80, 0xBA},
	{0x6D80, 0xC8},
	{0x6E80, 0xFA},
	{0x6F80, 0x7F},
	{0x7080, 0x00},
	{0x7180, 0x0A},
	{0x7280, 0x25},
	{0x7380, 0x3B},
	{0x7480, 0x1D},
	{0x7580, 0x30},
	{0x7680, 0x60},
	{0x7780, 0x63},
	{0x7880, 0x1F},
	{0x7980, 0x26},
	{0x7A80, 0xB2},
	{0x7B80, 0x1B},
	{0x7C80, 0x47},
	{0x7D80, 0x5E},
	{0x7E80, 0xBC},
	{0x7F80, 0xCD},
	{0x8080, 0xF8},
	{0x8180, 0x7F},
	{0x8280, 0x00},
	{0x8380, 0x0A},
	{0x8480, 0x25},
	{0x8580, 0x3B},
	{0x8680, 0x1D},
	{0x8780, 0x30},
	{0x8880, 0x60},
	{0x8980, 0x63},
	{0x8A80, 0x1F},
	{0x8B80, 0x26},
	{0x8C80, 0xB2},
	{0x8D80, 0x1B},
	{0x8E80, 0x47},
	{0x8F80, 0x5E},
	{0x9080, 0xBC},
	{0x9180, 0xCD},
	{0x9280, 0xF8},
	{0x9380, 0x7F},
	{0x2900, 0x00},
	{0x3500, 0x00},
	{0x4400, 0x02},
	{0x4401, 0x58},
};

static int write_seq(struct nov_regs *cmd_table, unsigned array_size)
{
    int i;

    for (i = 0; i < array_size; i++)
    {
        if (cmd_table[i].reg == REG_WAIT)
        {
            msleep(cmd_table[i].val);
            continue;
        } 
        write_client_reg(cmd_table[i].val, cmd_table[i].reg);
        if (reg == 0x1100)
        mddi_host_reg_out(CMD, MDDI_CMD_POWERDOWN);
    }
    return 0;
}

static int lexikon_panel_init(void)
{

    mutex_lock(&panel_lock);

    switch (panel_type) 
    {
        case PANEL_LEXIKON_SHARP:
        case PANEL_LEXIKON_SHARP_CUT2:
            write_seq(sharp_init_seq, ARRAY_SIZE(sharp_init_seq));
        break;
        case PANEL_LEXIKON_SONY:
        case PANEL_LEXIKON_SONY_CUT2:
            write_seq(sony_init_seq, ARRAY_SIZE(sony_init_seq));
        break;
        default:
            printk(KERN_ERR "%s Unknown panel! \n", __func__);
    }

    mutex_unlock(&panel_lock);

    return 0;
}

static int mddi_lexikon_panel_on(struct platform_device *pdev)
{
    int ret;
    printk(KERN_DEBUG "[BL] Turning on\n");
    screen_on = true;

    // resume before unblank
    lexikon_panel_init();
    atomic_set(&bl_ready, 1);

    /* Enable dim */
    write_client_reg(0x24, 0x5300);
    write_client_reg(0x0A, 0x22C0);
    msleep(30);
    lexikon_adjust_backlight(last_val);

    return 0;
}

static int mddi_lexikon_panel_off(struct platform_device *pdev)
{
    printk(KERN_DEBUG "[BL] Turning off\n");
    screen_on = false;

    /* set dim off for performance */
    write_client_reg(0x0, 0x5300);
    lexikon_adjust_backlight(LED_OFF);
    write_client_reg(0, 0x2800);
    write_client_reg(0, 0x1000);
    atomic_set(&bl_ready, 0);
    return 0;
}

static int lexikon_adjust_backlight(enum led_brightness val)
{
	unsigned int shrink_br;

	if (atomic_read(&bl_ready) == 0)
		return;

    if (val == 0)
        shrink_br = 0;
    else if (val < 30)
        shrink_br = 9;
    else if ((val >= 30) && (val < 83))
        shrink_br = 46 * (val - 30) / 53 + 9;
    else if ((val >= 83) && (val < 142))
        shrink_br = 54 * (val - 83) / 59 + 55;
    else
        shrink_br = 146 * (val - 142) / 113 + 109;

    if (last_val == shrink_br)
    {
        printk(KERN_DEBUG "[BL] Skipping identical br. %d\n", shrink_br);
        return;
    }

	mutex_lock(&panel_lock);

    if (lexikon_set_dim == 1)
    {
        write_client_reg(0x2C, 0x5300);
        /* we dont need set dim again */
        lexikon_set_dim = 0;
    }

    printk(KERN_DEBUG "[BL] Setting bl to %d\n", shrink_br);    

    write_client_reg(0x00, 0x5500);
    write_client_reg(shrink_br, 0x5100);
    last_val = shrink_br;

    mutex_unlock(&panel_lock);

    return shrink_br;
}

static enum led_brightness
lexikon_get_brightness(struct led_classdev *led_cdev)
{
    return last_val;
}

static void lexikon_brightness_set(struct led_classdev *led_cdev,
    enum led_brightness val)
{
    if (!screen_on)
    {
        printk(KERN_DEBUG "[BL] Screen is off, ignoring val=%d \n", val);
        return;
    }

    led_cdev->brightness = lexikon_adjust_brightness(val);
    /* set next backlight value with dim */
    lexikon_set_dim = 1;
}

static struct led_classdev lexikon_backlight_led = {
    .name           = "lcd-backlight",
    .brightness_get = lexikon_brightness_get,
    .brightness_set = lexikon_brightness_set,
};

static int lexikon_backlight_probe(struct platform_device *pdev)
{
    int rc;

    rc = led_classdev_register(&pdev->dev, &lexikon_backlight_led);
    if (rc)
    { 
        printk(KERN_ERR "%s +\n", __func__);
        led_classdev_unregister(&lexikon_backlight_led);
        return rc;
    }
    return 0;
}

static struct platform_device lexikon_backlight = {
    .name = "lexikon-backlight",
};

static struct platform_driver lexikon_backlight_driver = {
    .probe      = lexikon_backlight_probe,
    .driver     = {
        .name   = "lexikon-backlight",
        .owner  = THIS_MODULE,
    },
};

static int lexikonwvga_probe(struct platform_device *pdev)
{
    pr_info("%s: id=%d\n", __func__, pdev->id);

    int ret;
    struct msm_panel_common_pdata *mddi_lexikonwvga_pdata;

    wake_lock_init(&panel_idle_lock, WAKE_LOCK_SUSPEND,
            "backlight_present");

    ret = platform_device_register(&lexikon_backlight);
    if (ret)
    {
        printk(KERN_ERR "%s fail %d\n", __func__, rc);
        platform_device_unregister(&lexikon_backlight);
        return ret;
    }

    msm_fb_add_device(pdev);
    return 0;
}

static struct platform_driver this_driver = {
    .probe = lexikonwvga_probe,
    .driver = {
        .name = "mddi_lexikon_wvga"
    },
};

static struct msm_fb_panel_data lexikonwvga_panel_data= {
    .on = mddi_lexikon_panel_on,
    .off = mddi_lexikon_panel_off,
};

static struct platform_device this_device = {
    .name	= "mddi_lexikon_wvga",
    .id	= 1,
    .dev	= {
        .platform_data = &lexikonwvga_panel_data,
    },
};

static int __init lexikonwvga_init(void)
{
    int ret;
    struct msm_panel_info *pinfo;

    u32 id;
    ret = msm_fb_detect_client("mddi_lexikon_wvga");
    if (ret == -ENODEV)
        return 0;

    ret = platform_driver_register(&this_driver);
    if (ret) 
    {
        pr_err("%s: driver register failed, rc=%d\n", __func__, ret);
        return ret;
    }

    pinfo = &lexikonwvga_panel_data.panel_info;
    pinfo->xres = 480;
    pinfo->yres = 800;
    pinfo->type = MDDI_PANEL;
    pinfo->pdest = DISPLAY_1;
    pinfo->mddi.vdopkt = MDDI_DEFAULT_PRIM_PIX_ATTR;
    pinfo->wait_cycle = 0;
    pinfo->bpp = 16; // as defined by HTC
    pinfo->fb_num = 2; // or 3?
    pinfo->clk_rate = 192000000;
    pinfo->clk_min  = 192000000; // when scaled, is 384000000
    pinfo->clk_max  = 200000000;
    pinfo->lcd.v_back_porch = 4; // vivo sony
    pinfo->lcd.v_front_porch = 2; // vivo sony
    pinfo->lcd.v_pulse_width = 4; // vivo sony

    ret = platform_device_register(&this_device);
	if (ret) 
    {
		printk(KERN_ERR "%s not able to register the device\n",
			__func__);
		platform_driver_unregister(&this_driver);
	}

	return ret;
}

static int __init lexikon_backlight_init(void)
{
    int ret;
    ret = platform_driver_register(&lexikon_backlight_driver);
    if (ret) 
    {
        printk(KERN_ERR "%s not able to register the device\n",
            __func__);
        platform_device_unregister(&lexikon_backlight_driver);
        return ret;
    }
    return 0;
}

device_initcall(lexikonwvga_init);
module_init(lexikon_backlight_init);
