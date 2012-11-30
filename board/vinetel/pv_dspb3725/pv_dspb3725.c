/*
 * (C) Copyright 2004-2011
 * Texas Instruments, <www.ti.com>
 *
 * Author :
 *	Manikandan Pillai <mani.pillai@ti.com>
 *
 * Derived from Beagle Board and 3430 SDP code by
 *	Richard Woodruff <r-woodruff2@ti.com>
 *	Syed Mohammed Khasim <khasim@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/arch/mem.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/gpio.h>
#include <asm/arch/ehci.h>
#include <i2c.h>
#include <asm/mach-types.h>
#include <linux/mtd/nand.h>
#include "pv_dspb3725.h"
//#if KIMBG_ADD
//#include "fpga.h"
//#endif

#if KIMBG_ADD
#include <twl4030.h>
#ifdef CONFIG_USB_EHCI
#include <usb.h>
#include <asm/ehci-omap.h>
#endif
#endif

#define OMAP3EVM_GPIO_ETH_RST_GEN1		64
#define OMAP3EVM_GPIO_ETH_RST_GEN2		7

DECLARE_GLOBAL_DATA_PTR;

static u32 omap3_evm_version;

u32 get_omap3_evm_rev(void)
{
	return omap3_evm_version;
}

static void omap3_evm_get_revision(void)
{
#if defined(CONFIG_CMD_NET)
	/*
	 * Board revision can be ascertained only by identifying
	 * the Ethernet chipset.
	 */
	unsigned int smsc_id;

	/* Ethernet PHY ID is stored at ID_REV register */
	smsc_id = readl(CONFIG_SMC911X_BASE + 0x50) & 0xFFFF0000;
	printf("Read back SMSC id 0x%x\n", smsc_id);

	switch (smsc_id) {
	/* SMSC9115 chipset */
	case 0x01150000:
		omap3_evm_version = OMAP3EVM_BOARD_GEN_1;
		break;
	/* SMSC 9220 chipset */
	case 0x92200000:
	default:
		omap3_evm_version = OMAP3EVM_BOARD_GEN_2;
       }
#else
#if defined(CONFIG_STATIC_BOARD_REV)
	/*
	 * Look for static defintion of the board revision
	 */
	omap3_evm_version = CONFIG_STATIC_BOARD_REV;
#else
	/*
	 * Fallback to the default above.
	 */
	omap3_evm_version = OMAP3EVM_BOARD_GEN_2;
#endif
#endif	/* CONFIG_CMD_NET */
}

#ifdef CONFIG_USB_OMAP3
/*
 * MUSB port on OMAP3EVM Rev >= E requires extvbus programming.
 */
u8 omap3_evm_need_extvbus(void)
{
	u8 retval = 0;

	if (get_omap3_evm_rev() >= OMAP3EVM_BOARD_GEN_2)
		retval = 1;

	return retval;
}
#endif

/*
 * Routine: board_init
 * Description: Early hardware init.
 */
int board_init(void)
{
	gpmc_init(); /* in SRAM or SDRAM, finish GPMC */
	/* board id for Linux */
	gd->bd->bi_arch_number = MACH_TYPE_OMAP3EVM;
	/* boot param addr */
	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100);

	return 0;
}

#ifdef CONFIG_SPL_BUILD
/*
 * Routine: get_board_mem_timings
 * Description: If we use SPL then there is no x-loader nor config header
 * so we have to setup the DDR timings ourself on the first bank.  This
 * provides the timing values back to the function that configures
 * the memory.
 */
void get_board_mem_timings(u32 *mcfg, u32 *ctrla, u32 *ctrlb, u32 *rfr_ctrl,
		u32 *mr)
{
#if KIMBG_CHG
		*mcfg = ( (3 << 24) | (6 << 20) | (1 << 19) | (256 << 8) | (2 << 6) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 0) );
		*ctrla = ( (21 << 27) | (10 << 22) | (7 << 18) | (3 << 15) | (3 << 12) | (2 << 9) | (3 << 6) | (6 << 0) );
		*ctrlb = ( (1 << 16) | (1 <<12) | (5  << 8) | (23 << 0) );
#else
	int pop_mfr, pop_id;

	/*
	 * We need to identify what PoP memory is on the board so that
	 * we know what timings to use.  To map the ID values please see
	 * nand_ids.c
	 */
	identify_nand_chip(&pop_mfr, &pop_id);

	if (pop_mfr == NAND_MFR_HYNIX && pop_id == 0xbc) {
		/* 256MB DDR */
		*mcfg = HYNIX_V_MCFG_200(256 << 20);
		*ctrla = HYNIX_V_ACTIMA_200;
		*ctrlb = HYNIX_V_ACTIMB_200;
	} else {
		/* 128MB DDR */
		*mcfg = MICRON_V_MCFG_165(128 << 20);
		*ctrla = MICRON_V_ACTIMA_165;
		*ctrlb = MICRON_V_ACTIMB_165;
	}
#endif
	*rfr_ctrl = SDP_3430_SDRC_RFR_CTRL_165MHz;
	*mr = MICRON_V_MR_165;
}
#endif

#if KIMBG_ADD
#ifdef CONFIG_USB_EHCI
/* Call usb_stop() before starting the kernel */
void show_boot_progress(int val)
{
	  if (val == BOOTSTAGE_ID_RUN_OS)
				usb_stop();
}

static struct omap_usbhs_board_data usbhs_bdata = {
#if 1
	  .port_mode[0] = OMAP_EHCI_PORT_MODE_PHY,
		.port_mode[1] = OMAP_USBHS_PORT_MODE_UNUSED,
		.port_mode[2] = OMAP_USBHS_PORT_MODE_UNUSED
#else
	  .port_mode[0] = OMAP_EHCI_PORT_MODE_PHY,
		.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,
		.port_mode[2] = OMAP_EHCI_PORT_MODE_PHY,

		.port_mode[0] = OMAP_USBHS_PORT_MODE_UNUSED,
		.port_mode[1] = OMAP_USBHS_PORT_MODE_UNUSED,
		.port_mode[2] = OMAP_USBHS_PORT_MODE_UNUSED
#endif
};

int ehci_hcd_init(int index, struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	return omap_ehci_hcd_init(&usbhs_bdata, hccr, hcor);
}

// int ehci_hcd_init(void)
// {
// #if KIMBG_CHG_XXX
// #define GPIO_PHY_RESET 147
// #define OMAP_USBTLL_SYSCONFIG       (0x10)
// #define OMAP_USBTLL_SYSSTATUS       (0x14)
// #define OMAP_UHH_SYSCONFIG        (0x10)
// #define OMAP_UHH_HOSTCONFIG       (0x40)
// /*
//  * Initialize the OMAP3 EHCI controller and PHY on the BeagleBoard.
//  * Based on "drivers/usb/host/ehci-omap.c" from Linux 2.6.37.
//  * See there for additional Copyrights.
//  */
// 	debug("Initializing OMAP3 ECHI\n");
// 
// 	/* Put the PHY in RESET */
// 	gpio_request(GPIO_PHY_RESET, "");
// 	gpio_direction_output(GPIO_PHY_RESET, 0);
// 	gpio_set_value(GPIO_PHY_RESET, 0);
// 
// 	/* Hold the PHY in RESET for enough time till DIR is high */
// 	/* Refer: ISSUE1 */
// 	udelay(10);
// 
// 	struct prcm *prcm_base = (struct prcm *)PRCM_BASE;
// 	/* Enable USBHOST_L3_ICLK (USBHOST_MICLK) */
// 	sr32(&prcm_base->iclken_usbhost, 0, 1, 1);
// 	/*
// 	 * Enable USBHOST_48M_FCLK (USBHOST_FCLK1)
// 	 * and USBHOST_120M_FCLK (USBHOST_FCLK2)
// 	 */
// 	sr32(&prcm_base->fclken_usbhost, 0, 2, 3);
// 	/* Enable USBTTL_ICLK */
// 	sr32(&prcm_base->iclken3_core, 2, 1, 1);
// 	/* Enable USBTTL_FCLK */
// 	sr32(&prcm_base->fclken3_core, 2, 1, 1);
// 	debug("USB clocks enabled\n");
// 
// 	/* perform TLL soft reset, and wait until reset is complete */
// 	writel(OMAP_USBTLL_SYSCONFIG_SOFTRESET,
// 		OMAP_USBTLL_BASE + OMAP_USBTLL_SYSCONFIG);
// 	/* Wait for TLL reset to complete */
// 	while (!(readl(OMAP_USBTLL_BASE + OMAP_USBTLL_SYSSTATUS)
// 			& OMAP_USBTLL_SYSSTATUS_RESETDONE));
// 	debug("TLL reset done\n");
// 
// 	writel(OMAP_USBTLL_SYSCONFIG_ENAWAKEUP |
// 		OMAP_USBTLL_SYSCONFIG_SIDLEMODE |
// 		OMAP_USBTLL_SYSCONFIG_CACTIVITY,
// 		OMAP_USBTLL_BASE + OMAP_USBTLL_SYSCONFIG);
// 
// 	/* Put UHH in NoIdle/NoStandby mode */
// 	writel(OMAP_UHH_SYSCONFIG_ENAWAKEUP
// 		| OMAP_UHH_SYSCONFIG_SIDLEMODE
// 		| OMAP_UHH_SYSCONFIG_CACTIVITY
// 		| OMAP_UHH_SYSCONFIG_MIDLEMODE,
// 		OMAP_UHH_BASE + OMAP_UHH_SYSCONFIG);
// 
// 	/* setup burst configurations */
// 	writel(OMAP_UHH_HOSTCONFIG_INCR4_BURST_EN
// 		| OMAP_UHH_HOSTCONFIG_INCR8_BURST_EN
// 		| OMAP_UHH_HOSTCONFIG_INCR16_BURST_EN,
// 		OMAP_UHH_BASE + OMAP_UHH_HOSTCONFIG);
// 
// 	/*
// 	 * Refer ISSUE1:
// 	 * Hold the PHY in RESET for enough time till
// 	 * PHY is settled and ready
// 	 */
// 	udelay(10);
// 	gpio_set_value(GPIO_PHY_RESET, 1);
// 
// 	debug("OMAP3 EHCI init done\n");
// 	return 0;
// #else
// 	  return omap_ehci_hcd_init(&usbhs_bdata);
// #endif
// }

int ehci_hcd_stop(int index)
{
	return omap_ehci_hcd_stop();
}

// int ehci_hcd_stop(void)
// {
// 	  return omap_ehci_hcd_stop();
// }

#endif /* CONFIG_USB_EHCI */
#endif

#if KIMBG_ADD
#if defined(CONFIG_FPGA)

//for download
#define FPGA_GPMC3_CONFIG1	0x00000000		//8bit
#define FPGA_GPMC3_CONFIG2	0x001e1e01
#define FPGA_GPMC3_CONFIG3	0x00080300
#define FPGA_GPMC3_CONFIG4	0x1c091c09
#define FPGA_GPMC3_CONFIG5	0x04181f1f
#define FPGA_GPMC3_CONFIG6	0x00000FCF
#define FPGA_GPMC3_CONFIG7	0x00000f64		//0x2400 0000

#define FPGA_GPMC4_CONFIG1	0x00001000		//16bit
#define FPGA_GPMC4_CONFIG2	0x001e1e01
#define FPGA_GPMC4_CONFIG3	0x00080300
#define FPGA_GPMC4_CONFIG4	0x1c091c09
#define FPGA_GPMC4_CONFIG5	0x04181f1f
#define FPGA_GPMC4_CONFIG6	0x00000FCF
#define FPGA_GPMC4_CONFIG7	0x00000f68		//0x2800 0000

//static void setup_fpga_chip(void)
//{
//	struct ctrl *ctrl_base = (struct ctrl *)OMAP34XX_CTRL_BASE;
//	int					cs;
//
//	/* Configure GPMC registers */
//	cs = 3;
//	writel(FPGA_GPMC3_CONFIG1, &gpmc_cfg->cs[cs].config1);
//	writel(FPGA_GPMC3_CONFIG2, &gpmc_cfg->cs[cs].config2);
//	writel(FPGA_GPMC3_CONFIG3, &gpmc_cfg->cs[cs].config3);
//	writel(FPGA_GPMC3_CONFIG4, &gpmc_cfg->cs[cs].config4);
//	writel(FPGA_GPMC3_CONFIG5, &gpmc_cfg->cs[cs].config5);
//	writel(FPGA_GPMC3_CONFIG6, &gpmc_cfg->cs[cs].config6);
//	writel(FPGA_GPMC3_CONFIG7, &gpmc_cfg->cs[cs].config7);
//
//	cs = 4;
//	writel(FPGA_GPMC4_CONFIG1, &gpmc_cfg->cs[cs].config1);
//	writel(FPGA_GPMC4_CONFIG2, &gpmc_cfg->cs[cs].config2);
//	writel(FPGA_GPMC4_CONFIG3, &gpmc_cfg->cs[cs].config3);
//	writel(FPGA_GPMC4_CONFIG4, &gpmc_cfg->cs[cs].config4);
//	writel(FPGA_GPMC4_CONFIG5, &gpmc_cfg->cs[cs].config5);
//	writel(FPGA_GPMC4_CONFIG6, &gpmc_cfg->cs[cs].config6);
//	writel(FPGA_GPMC4_CONFIG7, &gpmc_cfg->cs[cs].config7);
//
//#if 0
//	/* Enable off mode for NWE in PADCONF_GPMC_NWE register */
//	writew(readw(&ctrl_base ->gpmc_nwe) | 0x0E00, &ctrl_base->gpmc_nwe);
//	/* Enable off mode for NOE in PADCONF_GPMC_NADV_ALE register */
//	writew(readw(&ctrl_base->gpmc_noe) | 0x0E00, &ctrl_base->gpmc_noe);
//	/* Enable off mode for ALE in PADCONF_GPMC_NADV_ALE register */
//	writew(readw(&ctrl_base->gpmc_nadv_ale) | 0x0E00,
//		&ctrl_base->gpmc_nadv_ale);
//#endif
//}

#endif
#endif
/*
 * Routine: misc_init_r
 * Description: Init ethernet (done here so udelay works)
 */
int misc_init_r(void)
{
#ifdef CONFIG_DRIVER_OMAP34XX_I2C
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif

//#if KIMBG_ADD
//#if defined(CONFIG_FPGA)
//	setup_fpga_chip();
//#endif
//#endif

#if defined(CONFIG_CMD_NET)
	setup_net_chip();
#endif
	omap3_evm_get_revision();

#if defined(CONFIG_CMD_NET)
//KIMBG_CMT	reset_net_chip();
#endif
	dieid_num_r();

#if KIMBG_ADD
{
	unsigned int addr=0x48002520;	//PBIASLITE
	unsigned int	value;
	value = readl(addr);
	value &= ~0x1;
	value |= 0x2;
	writel(value,addr);
}
#endif
#if KIMBG_ADD
  twl4030_pmrecv_vsel_cfg(TWL4030_PM_RECEIVER_VMMC1_DEDICATED,
//		TWL4030_PM_RECEIVER_VMMC1_VSEL_32,
		0,
		TWL4030_PM_RECEIVER_VMMC1_DEV_GRP,
		TWL4030_PM_RECEIVER_DEV_GRP_P1);
//for VBUS
//		twl4030_i2c_write_u8(0x48, 0x26, 0x0a);
#endif
//#if KIMBG_ADD
//#if defined(CONFIG_FPGA)
//sio_init_fpga ();
//#endif
//#endif

	return 0;
}

/*
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers specific to the
 *		hardware. Many pins need to be moved from protect to primary
 *		mode.
 */
void set_muxconf_regs(void)
{
	MUX_EVM();
}

#ifdef CONFIG_CMD_NET
/*
 * Routine: setup_net_chip
 * Description: Setting up the configuration GPMC registers specific to the
 *		Ethernet hardware.
 */
static void setup_net_chip(void)
{
	struct ctrl *ctrl_base = (struct ctrl *)OMAP34XX_CTRL_BASE;

	/* Configure GPMC registers */
	writel(NET_GPMC_CONFIG1, &gpmc_cfg->cs[5].config1);
	writel(NET_GPMC_CONFIG2, &gpmc_cfg->cs[5].config2);
	writel(NET_GPMC_CONFIG3, &gpmc_cfg->cs[5].config3);
	writel(NET_GPMC_CONFIG4, &gpmc_cfg->cs[5].config4);
	writel(NET_GPMC_CONFIG5, &gpmc_cfg->cs[5].config5);
	writel(NET_GPMC_CONFIG6, &gpmc_cfg->cs[5].config6);
	writel(NET_GPMC_CONFIG7, &gpmc_cfg->cs[5].config7);

	/* Enable off mode for NWE in PADCONF_GPMC_NWE register */
	writew(readw(&ctrl_base ->gpmc_nwe) | 0x0E00, &ctrl_base->gpmc_nwe);
	/* Enable off mode for NOE in PADCONF_GPMC_NADV_ALE register */
	writew(readw(&ctrl_base->gpmc_noe) | 0x0E00, &ctrl_base->gpmc_noe);
	/* Enable off mode for ALE in PADCONF_GPMC_NADV_ALE register */
	writew(readw(&ctrl_base->gpmc_nadv_ale) | 0x0E00,
		&ctrl_base->gpmc_nadv_ale);
}

/**
 * Reset the ethernet chip.
 */
static void reset_net_chip(void)
{
	int ret;
	int rst_gpio;

	if (get_omap3_evm_rev() == OMAP3EVM_BOARD_GEN_1) {
		rst_gpio = OMAP3EVM_GPIO_ETH_RST_GEN1;
	} else {
		rst_gpio = OMAP3EVM_GPIO_ETH_RST_GEN2;
	}

	ret = gpio_request(rst_gpio, "");
	if (ret < 0) {
		printf("Unable to get GPIO %d\n", rst_gpio);
		return ;
	}

	/* Configure as output */
	gpio_direction_output(rst_gpio, 0);

	/* Send a pulse on the GPIO pin */
	gpio_set_value(rst_gpio, 1);
	udelay(1);
	gpio_set_value(rst_gpio, 0);
	udelay(1);
	gpio_set_value(rst_gpio, 1);
}

int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_SMC911X
#define STR_ENV_ETHADDR	"ethaddr"

	struct eth_device *dev;
	uchar eth_addr[6];

	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);

	if (!eth_getenv_enetaddr(STR_ENV_ETHADDR, eth_addr)) {
		dev = eth_get_dev_by_index(0);
		if (dev) {
			eth_setenv_enetaddr(STR_ENV_ETHADDR, dev->enetaddr);
		} else {
			printf("omap3evm: Couldn't get eth device\n");
			rc = -1;
		}
	}
#endif
	return rc;
}
#endif /* CONFIG_CMD_NET */

#if defined(CONFIG_GENERIC_MMC) && !defined(CONFIG_SPL_BUILD)
int board_mmc_init(bd_t *bis)
{
	omap_mmc_init(0, 0, 0);
	return 0;
}
#endif


// #if defined(CONFIG_GENERIC_MMC) && !defined(CONFIG_SPL_BUILD)
// int board_mmc_init(bd_t *bis)
// {
// 	omap_mmc_init(0);
// 	return 0;
// }
// #endif
