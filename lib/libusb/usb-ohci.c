/*****************************************************************************
 * Copyright (c) 2013 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <string.h>
#include "usb.h"
#include "usb-core.h"
#include "usb-ohci.h"

#undef OHCI_DEBUG
//#define OHCI_DEBUG
#ifdef OHCI_DEBUG
#define dprintf(_x ...) printf(_x)
#else
#define dprintf(_x ...)
#endif

/*
 * Dump OHCI register
 *
 * @param   - ohci_hcd
 * @return  -
 */
static void ohci_dump_regs(struct ohci_regs *regs)
{
	dprintf("\n - HcRevision         %08X", read_reg(&regs->rev));
	dprintf("   - HcControl          %08X", read_reg(&regs->control));
	dprintf("\n - HcCommandStatus    %08X", read_reg(&regs->cmd_status));
	dprintf("   - HcInterruptStatus  %08X", read_reg(&regs->intr_status));
	dprintf("\n - HcInterruptEnable  %08X", read_reg(&regs->intr_enable));
	dprintf("   - HcInterruptDisable %08X", read_reg(&regs->intr_disable));
	dprintf("\n - HcHCCA             %08X", read_reg(&regs->hcca));
	dprintf("   - HcPeriodCurrentED  %08X", read_reg(&regs->period_curr_ed));
	dprintf("\n - HcControlHeadED    %08X", read_reg(&regs->cntl_head_ed));
	dprintf("   - HcControlCurrentED %08X", read_reg(&regs->cntl_curr_ed));
	dprintf("\n - HcBulkHeadED       %08X", read_reg(&regs->bulk_head_ed));
	dprintf("   - HcBulkCurrentED    %08X", read_reg(&regs->bulk_curr_ed));
	dprintf("\n - HcDoneHead         %08X", read_reg(&regs->done_head));
	dprintf("   - HcFmInterval       %08X", read_reg(&regs->fm_interval));
	dprintf("\n - HcFmRemaining      %08X", read_reg(&regs->fm_remaining));
	dprintf("   - HcFmNumber         %08X", read_reg(&regs->fm_num));
	dprintf("\n - HcPeriodicStart    %08X", read_reg(&regs->period_start));
	dprintf("   - HcLSThreshold      %08X", read_reg(&regs->ls_threshold));
	dprintf("\n - HcRhDescriptorA    %08X", read_reg(&regs->rh_desc_a));
	dprintf("   - HcRhDescriptorB    %08X", read_reg(&regs->rh_desc_b));
	dprintf("\n - HcRhStatus         %08X", read_reg(&regs->rh_status));
	dprintf("\n");
}

/*
 * OHCI Spec 5.1.1
 * OHCI Spec 7.4 Root Hub Partition
 */
static int ohci_hcd_reset(struct ohci_regs *regs)
{
	uint32_t time;
	time = SLOF_GetTimer() + 20;
	write_reg(&regs->cmd_status, OHCI_CMD_STATUS_HCR);
	while ((time > SLOF_GetTimer()) &&
		(read_reg(&regs->cmd_status) & OHCI_CMD_STATUS_HCR))
		cpu_relax();
	if (read_reg(&regs->cmd_status) & OHCI_CMD_STATUS_HCR) {
		printf(" ** HCD Reset failed...");
		return -1;
	}

	write_reg(&regs->rh_desc_a, RHDA_PSM_INDIVIDUAL | RHDA_OCPM_PERPORT);
	write_reg(&regs->rh_desc_b, RHDB_PPCM_PORT_POWER);
	write_reg(&regs->fm_interval, FRAME_INTERVAL);
	write_reg(&regs->period_start, PERIODIC_START);
	return 0;
}

static int ohci_hcd_init(struct ohci_hcd *ohcd)
{
	struct ohci_regs *regs;
	struct ohci_ed *ed;
	long ed_phys = 0;
	unsigned int i;
	uint32_t oldrwc;

	if (!ohcd)
		return -1;

	dprintf("%s: HCCA memory %p\n", __func__, ohcd->hcca);
	dprintf("%s: OHCI Regs   %p\n", __func__, regs);

	ed = SLOF_dma_alloc(sizeof(struct ohci_ed));
	if (!ed) {
		printf("usb-ohci: Unable to allocate memory - ed\n");
		return -1;
	}
	ed_phys = SLOF_dma_map_in(ed, sizeof(struct ohci_ed), true);

	/*
	 * OHCI Spec 4.4: Host Controller Communications Area
	 */
	memset(ohcd->hcca, 0, HCCA_SIZE);
	memset(ed, 0, sizeof(struct ohci_ed));
	write_reg(&ed->attr, EDA_SKIP);
	for (i = 0; i < HCCA_INTR_NUM; i++)
		write_reg(&ohcd->hcca->intr_table[i], ed_phys);

	write_reg(&regs->hcca, ohcd->hcca_phys);
	write_reg(&regs->cntl_head_ed, 0);
	write_reg(&regs->bulk_head_ed, 0);

	/* OHCI Spec 7.1.2 HcControl Register */
	oldrwc = read_reg(&regs->control) & OHCI_CTRL_RWC;
	write_reg(&regs->control, (OHCI_CTRL_CBSR | OHCI_CTRL_CLE
					| OHCI_CTRL_PLE | OHCI_USB_OPER | oldrwc));
	ohci_dump_regs(regs);
	return 0;
}

/*
 * OHCI Spec 7.4 Root Hub Partition
 */
static void ohci_hub_check_ports(struct ohci_hcd *ohcd)
{
	struct ohci_regs *regs;
	unsigned int ports, i, port_status, port_clear = 0;

	regs = ohcd->regs;
	ports = read_reg(&regs->rh_desc_a) & RHDA_NDP;
	write_reg(&regs->rh_status, RH_STATUS_LPSC);
	SLOF_msleep(5);
	dprintf("usb-ohci: ports connected %d\n", ports);
	for (i = 0; i < ports; i++) {
		dprintf("usb-ohci: ports scanning %d\n", i);
		port_status = read_reg(&regs->rh_ps[i]);
		if (port_status & RH_PS_CSC) {
			if (port_status & RH_PS_CCS) {
				write_reg(&regs->rh_ps[i], RH_PS_PRS);
				port_clear |= RH_PS_CSC;
				dprintf("Start enumerating device\n");
				SLOF_msleep(10);
			} else
				dprintf("Start removing device\n");
		}
		port_status = read_reg(&regs->rh_ps[i]);
		if (port_status & RH_PS_PRSC) {
			port_clear |= RH_PS_PRSC;
			dprintf("Device reset\n");
		}
		if (port_status & RH_PS_PESC) {
			port_clear |= RH_PS_PESC;
			if (port_status & RH_PS_PES)
				dprintf("enabled\n");
			else
				dprintf("disabled\n");
		}
		if (port_status & RH_PS_PSSC) {
			port_clear |= RH_PS_PESC;
			dprintf("suspended\n");
		}
		port_clear &= 0xFFFF0000;
		if (port_clear)
			write_reg(&regs->rh_ps[i], port_clear);
	}
}

static void ohci_init(struct usb_hcd_dev *hcidev)
{
	struct ohci_hcd *ohcd;

	printf("  OHCI: initializing\n");
	dprintf("%s: device base address %p\n", __func__, hcidev->base);

	ohcd = SLOF_dma_alloc(sizeof(struct ohci_hcd));
	if (!ohcd) {
		printf("usb-ohci: Unable to allocate memory\n");
		goto out;
	}

	/* Addressing BusID - 7:5, Device ID: 4:0 */
	hcidev->nextaddr = (hcidev->num << 5) | 1;
	hcidev->priv = ohcd;
	ohcd->hcidev = hcidev;
	ohcd->regs = (struct ohci_regs *)(hcidev->base);
	ohcd->hcca = SLOF_dma_alloc(sizeof(struct ohci_hcca));
	if (!ohcd->hcca || PTR_U32(ohcd->hcca) & HCCA_ALIGN) {
		printf("usb-ohci: Unable to allocate/unaligned HCCA memory %p\n",
			ohcd->hcca);
		goto out_free_hcd;
	}
	ohcd->hcca_phys = SLOF_dma_map_in(ohcd->hcca,
					sizeof(struct ohci_hcca), true);
	dprintf("usb-ohci: HCCA memory %p HCCA-dev memory %08lx\n",
		ohcd->hcca, ohcd->hcca_phys);

	ohci_hcd_reset(ohcd->regs);
	ohci_hcd_init(ohcd);
	ohci_hub_check_ports(ohcd);
	return;

out_free_hcd:
	SLOF_dma_free(ohcd->hcca, sizeof(struct ohci_hcca));
	SLOF_dma_free(ohcd, sizeof(struct ohci_hcd));
out:
	return;
}

static void ohci_detect(void)
{

}

static void ohci_disconnect(void)
{

}

struct usb_hcd_ops ohci_ops = {
	.name        = "ohci-hcd",
	.init        = ohci_init,
	.detect      = ohci_detect,
	.disconnect  = ohci_disconnect,
	.usb_type    = USB_OHCI,
	.next        = NULL,
};

void usb_ohci_register(void)
{
	usb_hcd_register(&ohci_ops);
}