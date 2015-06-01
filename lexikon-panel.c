/* linux/arch/arm/mach-msm/board-lexikon-panel.c
 *
 * Copyright (C) 2008 HTC Corporation.
 * Author: Jay Tu <jay_tu@htc.com>
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <mach/msm_fb-7x30.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <mach/panel_id.h>

#include "../board-lexikon.h"
#include "../devices.h"
#include "../proc_comm.h"
#include "../../../../drivers/video/msm/mdp_hw.h"

#define DRIVER_IC_CUT2			4
#define PANEL_WHITESTONE		0
#define PANEL_LEXIKON_SHARP		1
#define PANEL_LEXIKON_SONY		2
#define PANEL_LEXIKON_SHARP_CUT2        (PANEL_LEXIKON_SHARP | DRIVER_IC_CUT2)
#define PANEL_LEXIKON_SONY_CUT2         (PANEL_LEXIKON_SONY | DRIVER_IC_CUT2)

extern int panel_type;
static struct vreg *V_LCMIO_1V8, *V_LCM_2V85;

static int mddi_novatec_power(u32 on)
{
	int rc;
	unsigned config;
	printk(KERN_DEBUG "%s(%d)\n", __func__, __LINE__);

	if (on) {
		vreg_enable(V_LCM_2V85);
		msleep(3);
		vreg_disable(V_LCM_2V85);
		msleep(50);
		vreg_enable(V_LCM_2V85);
		vreg_enable(V_LCMIO_1V8);
		msleep(2);
		gpio_set_value(LEXIKON_LCD_RSTz, 1);
		msleep(1);
		gpio_set_value(LEXIKON_LCD_RSTz, 0);
		msleep(1);
		gpio_set_value(LEXIKON_LCD_RSTz, 1);
		msleep(60);
		config = PCOM_GPIO_CFG(LEXIKON_MDDI_TE, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		config = PCOM_GPIO_CFG(LEXIKON_LCD_ID1, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		config = PCOM_GPIO_CFG(LEXIKON_LCD_ID0, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
	} else {
		config = PCOM_GPIO_CFG(LEXIKON_MDDI_TE, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		config = PCOM_GPIO_CFG(LEXIKON_LCD_ID1, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		config = PCOM_GPIO_CFG(LEXIKON_LCD_ID0, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		gpio_set_value(LEXIKON_LCD_RSTz, 0);
		msleep(10);
		vreg_disable(V_LCMIO_1V8);
		vreg_disable(V_LCM_2V85);
	}
    return 0;
}

int panel_init_power(void) 
{
	/* lcd panel power */
	/* 2.85V -- LDO20 */
	V_LCM_2V85 = vreg_get(NULL, "gp13");

	if (IS_ERR(V_LCM_2V85)) {
		pr_err("%s: LCM_2V85 get failed (%ld)\n",
			__func__, PTR_ERR(V_LCM_2V85));
		return -1;
	}
	if (sys_rev > 0)
		V_LCMIO_1V8 = vreg_get(NULL, "lvsw0");
	else
		V_LCMIO_1V8 = vreg_get(NULL, "wlan2");

	if (IS_ERR(V_LCMIO_1V8)) {
		pr_err("%s: LCMIO_1V8 get failed (%ld)\n",
		       __func__, PTR_ERR(V_LCMIO_1V8));
		return -1;
	}
}

static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
  *clk_rate *= 2;
	return 0;
}

static struct mddi_platform_data mddi_pdata = {
    .mddi_power_save = mddi_novatec_power,
	.mddi_sel_clk = msm_fb_mddi_sel_clk,
	.mddi_client_power = NULL, // mddi_novatec_power,
};

static struct msm_panel_common_pdata mdp_pdata = {
    .hw_revision_addr = 0xac001270,
    .gpio = 30,
    .mdp_max_clk = 192000000,
    .mdp_rev = MDP_REV_40,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", &mddi_pdata);
}

void __init lexikon_init_panel(unsigned int sys_rev)
{
	int rc;

    panel_init_power();
    msm_fb_add_devices();

	return 0;
}
