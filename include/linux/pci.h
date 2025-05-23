/*
 *	pci.h
 *
 *	PCI defines and function prototypes
 *	Copyright 1994, Drew Eckhardt
 *	Copyright 1997--1999 Martin Mares <mj@ucw.cz>
 *
 *	For more information, please consult the following manuals (look at
 *	http://www.pcisig.com/ for how to get them):
 *
 *	PCI BIOS Specification
 *	PCI Local Bus Specification
 *	PCI to PCI Bridge Specification
 *	PCI System Design Guide
 */

#ifndef LINUX_PCI_H
#define LINUX_PCI_H

#include <linux/mod_devicetable.h>

/*
 * Under PCI, each device has 256 bytes of configuration address space,
 * of which the first 64 bytes are standardized as follows:
 */
#define PCI_VENDOR_ID		0x00	/* 16 bits */
#define PCI_DEVICE_ID		0x02	/* 16 bits */
#define PCI_COMMAND		0x04	/* 16 bits */
#define  PCI_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY	0x2	/* Enable response in Memory space */
#define  PCI_COMMAND_MASTER	0x4	/* Enable bus mastering */
#define  PCI_COMMAND_SPECIAL	0x8	/* Enable response to special cycles */
#define  PCI_COMMAND_INVALIDATE	0x10	/* Use memory write and invalidate */
#define  PCI_COMMAND_VGA_PALETTE 0x20	/* Enable palette snooping */
#define  PCI_COMMAND_PARITY	0x40	/* Enable parity checking */
#define  PCI_COMMAND_WAIT 	0x80	/* Enable address/data stepping */
#define  PCI_COMMAND_SERR	0x100	/* Enable SERR */
#define  PCI_COMMAND_FAST_BACK	0x200	/* Enable back-to-back writes */
#define  PCI_COMMAND_INTX_DISABLE 0x400 /* INTx Emulation Disable */

#define PCI_STATUS		0x06	/* 16 bits */
#define  PCI_STATUS_CAP_LIST	0x10	/* Support Capability List */
#define  PCI_STATUS_66MHZ	0x20	/* Support 66 Mhz PCI 2.1 bus */
#define  PCI_STATUS_UDF		0x40	/* Support User Definable Features [obsolete] */
#define  PCI_STATUS_FAST_BACK	0x80	/* Accept fast-back to back */
#define  PCI_STATUS_PARITY	0x100	/* Detected parity error */
#define  PCI_STATUS_DEVSEL_MASK	0x600	/* DEVSEL timing */
#define  PCI_STATUS_DEVSEL_FAST		0x000
#define  PCI_STATUS_DEVSEL_MEDIUM	0x200
#define  PCI_STATUS_DEVSEL_SLOW		0x400
#define  PCI_STATUS_SIG_TARGET_ABORT	0x800 /* Set on target abort */
#define  PCI_STATUS_REC_TARGET_ABORT	0x1000 /* Master ack of " */
#define  PCI_STATUS_REC_MASTER_ABORT	0x2000 /* Set on master abort */
#define  PCI_STATUS_SIG_SYSTEM_ERROR	0x4000 /* Set when we drive SERR */
#define  PCI_STATUS_DETECTED_PARITY	0x8000 /* Set on parity error */

#define PCI_CLASS_REVISION	0x08	/* High 24 bits are class, low 8 revision */
#define PCI_REVISION_ID		0x08	/* Revision ID */
#define PCI_CLASS_PROG		0x09	/* Reg. Level Programming Interface */
#define PCI_CLASS_DEVICE	0x0a	/* Device class */

#define PCI_CACHE_LINE_SIZE	0x0c	/* 8 bits */
#define PCI_LATENCY_TIMER	0x0d	/* 8 bits */
#define PCI_HEADER_TYPE		0x0e	/* 8 bits */
#define  PCI_HEADER_TYPE_NORMAL		0
#define  PCI_HEADER_TYPE_BRIDGE		1
#define  PCI_HEADER_TYPE_CARDBUS	2

#define PCI_BIST		0x0f	/* 8 bits */
#define  PCI_BIST_CODE_MASK	0x0f	/* Return result */
#define  PCI_BIST_START		0x40	/* 1 to start BIST, 2 secs or less */
#define  PCI_BIST_CAPABLE	0x80	/* 1 if BIST capable */

/*
 * Base addresses specify locations in memory or I/O space.
 * Decoded size can be determined by writing a value of
 * 0xffffffff to the register, and reading it back.  Only
 * 1 bits are decoded.
 */
#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */
#define PCI_BASE_ADDRESS_1	0x14	/* 32 bits [htype 0,1 only] */
#define PCI_BASE_ADDRESS_2	0x18	/* 32 bits [htype 0 only] */
#define PCI_BASE_ADDRESS_3	0x1c	/* 32 bits */
#define PCI_BASE_ADDRESS_4	0x20	/* 32 bits */
#define PCI_BASE_ADDRESS_5	0x24	/* 32 bits */
#define  PCI_BASE_ADDRESS_SPACE		0x01	/* 0 = memory, 1 = I/O */
#define  PCI_BASE_ADDRESS_SPACE_IO	0x01
#define  PCI_BASE_ADDRESS_SPACE_MEMORY	0x00
#define  PCI_BASE_ADDRESS_MEM_TYPE_MASK	0x06
#define  PCI_BASE_ADDRESS_MEM_TYPE_32	0x00	/* 32 bit address */
#define  PCI_BASE_ADDRESS_MEM_TYPE_1M	0x02	/* Below 1M [obsolete] */
#define  PCI_BASE_ADDRESS_MEM_TYPE_64	0x04	/* 64 bit address */
#define  PCI_BASE_ADDRESS_MEM_PREFETCH	0x08	/* prefetchable? */
#define  PCI_BASE_ADDRESS_MEM_MASK	(~0x0fUL)
#define  PCI_BASE_ADDRESS_IO_MASK	(~0x03UL)
/* bit 1 is reserved if address_space = 1 */

/* Header type 0 (normal devices) */
#define PCI_CARDBUS_CIS		0x28
#define PCI_SUBSYSTEM_VENDOR_ID	0x2c
#define PCI_SUBSYSTEM_ID	0x2e
#define PCI_ROM_ADDRESS		0x30	/* Bits 31..11 are address, 10..1 reserved */
#define  PCI_ROM_ADDRESS_ENABLE	0x01
#define PCI_ROM_ADDRESS_MASK	(~0x7ffUL)

#define PCI_CAPABILITY_LIST	0x34	/* Offset of first capability list entry */

/* 0x35-0x3b are reserved */
#define PCI_INTERRUPT_LINE	0x3c	/* 8 bits */
#define PCI_INTERRUPT_PIN	0x3d	/* 8 bits */
#define PCI_MIN_GNT		0x3e	/* 8 bits */
#define PCI_MAX_LAT		0x3f	/* 8 bits */

/* Header type 1 (PCI-to-PCI bridges) */
#define PCI_PRIMARY_BUS		0x18	/* Primary bus number */
#define PCI_SECONDARY_BUS	0x19	/* Secondary bus number */
#define PCI_SUBORDINATE_BUS	0x1a	/* Highest bus number behind the bridge */
#define PCI_SEC_LATENCY_TIMER	0x1b	/* Latency timer for secondary interface */
#define PCI_IO_BASE		0x1c	/* I/O range behind the bridge */
#define PCI_IO_LIMIT		0x1d
#define  PCI_IO_RANGE_TYPE_MASK	0x0fUL	/* I/O bridging type */
#define  PCI_IO_RANGE_TYPE_16	0x00
#define  PCI_IO_RANGE_TYPE_32	0x01
#define  PCI_IO_RANGE_MASK	(~0x0fUL)
#define PCI_SEC_STATUS		0x1e	/* Secondary status register, only bit 14 used */
#define PCI_MEMORY_BASE		0x20	/* Memory range behind */
#define PCI_MEMORY_LIMIT	0x22
#define  PCI_MEMORY_RANGE_TYPE_MASK 0x0fUL
#define  PCI_MEMORY_RANGE_MASK	(~0x0fUL)
#define PCI_PREF_MEMORY_BASE	0x24	/* Prefetchable memory range behind */
#define PCI_PREF_MEMORY_LIMIT	0x26
#define  PCI_PREF_RANGE_TYPE_MASK 0x0fUL
#define  PCI_PREF_RANGE_TYPE_32	0x00
#define  PCI_PREF_RANGE_TYPE_64	0x01
#define  PCI_PREF_RANGE_MASK	(~0x0fUL)
#define PCI_PREF_BASE_UPPER32	0x28	/* Upper half of prefetchable memory range */
#define PCI_PREF_LIMIT_UPPER32	0x2c
#define PCI_IO_BASE_UPPER16	0x30	/* Upper half of I/O addresses */
#define PCI_IO_LIMIT_UPPER16	0x32
/* 0x34 same as for htype 0 */
/* 0x35-0x3b is reserved */
#define PCI_ROM_ADDRESS1	0x38	/* Same as PCI_ROM_ADDRESS, but for htype 1 */
/* 0x3c-0x3d are same as for htype 0 */
#define PCI_BRIDGE_CONTROL	0x3e
#define  PCI_BRIDGE_CTL_PARITY	0x01	/* Enable parity detection on secondary interface */
#define  PCI_BRIDGE_CTL_SERR	0x02	/* The same for SERR forwarding */
#define  PCI_BRIDGE_CTL_NO_ISA	0x04	/* Disable bridging of ISA ports */
#define  PCI_BRIDGE_CTL_VGA	0x08	/* Forward VGA addresses */
#define  PCI_BRIDGE_CTL_MASTER_ABORT	0x20  /* Report master aborts */
#define  PCI_BRIDGE_CTL_BUS_RESET	0x40	/* Secondary bus reset */
#define  PCI_BRIDGE_CTL_FAST_BACK	0x80	/* Fast Back2Back enabled on secondary interface */

/* Header type 2 (CardBus bridges) */
#define PCI_CB_CAPABILITY_LIST	0x14
/* 0x15 reserved */
#define PCI_CB_SEC_STATUS	0x16	/* Secondary status */
#define PCI_CB_PRIMARY_BUS	0x18	/* PCI bus number */
#define PCI_CB_CARD_BUS		0x19	/* CardBus bus number */
#define PCI_CB_SUBORDINATE_BUS	0x1a	/* Subordinate bus number */
#define PCI_CB_LATENCY_TIMER	0x1b	/* CardBus latency timer */
#define PCI_CB_MEMORY_BASE_0	0x1c
#define PCI_CB_MEMORY_LIMIT_0	0x20
#define PCI_CB_MEMORY_BASE_1	0x24
#define PCI_CB_MEMORY_LIMIT_1	0x28
#define PCI_CB_IO_BASE_0	0x2c
#define PCI_CB_IO_BASE_0_HI	0x2e
#define PCI_CB_IO_LIMIT_0	0x30
#define PCI_CB_IO_LIMIT_0_HI	0x32
#define PCI_CB_IO_BASE_1	0x34
#define PCI_CB_IO_BASE_1_HI	0x36
#define PCI_CB_IO_LIMIT_1	0x38
#define PCI_CB_IO_LIMIT_1_HI	0x3a
#define  PCI_CB_IO_RANGE_MASK	(~0x03UL)
/* 0x3c-0x3d are same as for htype 0 */
#define PCI_CB_BRIDGE_CONTROL	0x3e
#define  PCI_CB_BRIDGE_CTL_PARITY	0x01	/* Similar to standard bridge control register */
#define  PCI_CB_BRIDGE_CTL_SERR		0x02
#define  PCI_CB_BRIDGE_CTL_ISA		0x04
#define  PCI_CB_BRIDGE_CTL_VGA		0x08
#define  PCI_CB_BRIDGE_CTL_MASTER_ABORT	0x20
#define  PCI_CB_BRIDGE_CTL_CB_RESET	0x40	/* CardBus reset */
#define  PCI_CB_BRIDGE_CTL_16BIT_INT	0x80	/* Enable interrupt for 16-bit cards */
#define  PCI_CB_BRIDGE_CTL_PREFETCH_MEM0 0x100	/* Prefetch enable for both memory regions */
#define  PCI_CB_BRIDGE_CTL_PREFETCH_MEM1 0x200
#define  PCI_CB_BRIDGE_CTL_POST_WRITES	0x400
#define PCI_CB_SUBSYSTEM_VENDOR_ID	0x40
#define PCI_CB_SUBSYSTEM_ID		0x42
#define PCI_CB_LEGACY_MODE_BASE		0x44	/* 16-bit PC Card legacy mode base address (ExCa) */
/* 0x48-0x7f reserved */

/* Capability lists */

#define PCI_CAP_LIST_ID		0	/* Capability ID */
#define  PCI_CAP_ID_PM		0x01	/* Power Management */
#define  PCI_CAP_ID_AGP		0x02	/* Accelerated Graphics Port */
#define  PCI_CAP_ID_VPD		0x03	/* Vital Product Data */
#define  PCI_CAP_ID_SLOTID	0x04	/* Slot Identification */
#define  PCI_CAP_ID_MSI		0x05	/* Message Signalled Interrupts */
#define  PCI_CAP_ID_CHSWP	0x06	/* CompactPCI HotSwap */
#define  PCI_CAP_ID_PCIX	0x07	/* PCI-X */
#define  PCI_CAP_ID_HT		0x08	/* HyperTransport */
#define  PCI_CAP_ID_VNDR	0x09	/* Vendor specific */
#define  PCI_CAP_ID_DBG		0x0A	/* Debug port */
#define  PCI_CAP_ID_CCRC	0x0B	/* CompactPCI Central Resource Control */
#define  PCI_CAP_ID_SHPC 	0x0C	/* PCI Standard Hot-Plug Controller */
#define  PCI_CAP_ID_SSVID	0x0D	/* Bridge subsystem vendor/device ID */
#define  PCI_CAP_ID_AGP3	0x0E	/* AGP Target PCI-PCI bridge */
#define  PCI_CAP_ID_EXP 	0x10	/* PCI Express */
#define  PCI_CAP_ID_MSIX	0x11	/* MSI-X */
#define  PCI_CAP_ID_AF		0x13	/* PCI Advanced Features */
#define PCI_CAP_LIST_NEXT	1	/* Next capability in the list */
#define PCI_CAP_FLAGS		2	/* Capability defined flags (16 bits) */
#define PCI_CAP_SIZEOF		4

/* Power Management Registers */

#define PCI_PM_PMC		2	/* PM Capabilities Register */
#define  PCI_PM_CAP_VER_MASK	0x0007	/* Version */
#define  PCI_PM_CAP_PME_CLOCK	0x0008	/* PME clock required */
#define  PCI_PM_CAP_RESERVED    0x0010  /* Reserved field */
#define  PCI_PM_CAP_DSI		0x0020	/* Device specific initialization */
#define  PCI_PM_CAP_AUX_POWER	0x01C0	/* Auxilliary power support mask */
#define  PCI_PM_CAP_D1		0x0200	/* D1 power state support */
#define  PCI_PM_CAP_D2		0x0400	/* D2 power state support */
#define  PCI_PM_CAP_PME		0x0800	/* PME pin supported */
#define  PCI_PM_CAP_PME_MASK	0xF800	/* PME Mask of all supported states */
#define  PCI_PM_CAP_PME_D0	0x0800	/* PME# from D0 */
#define  PCI_PM_CAP_PME_D1	0x1000	/* PME# from D1 */
#define  PCI_PM_CAP_PME_D2	0x2000	/* PME# from D2 */
#define  PCI_PM_CAP_PME_D3	0x4000	/* PME# from D3 (hot) */
#define  PCI_PM_CAP_PME_D3cold	0x8000	/* PME# from D3 (cold) */
#define  PCI_PM_CAP_PME_SHIFT	11	/* Start of the PME Mask in PMC */
#define PCI_PM_CTRL		4	/* PM control and status register */
#define  PCI_PM_CTRL_STATE_MASK	0x0003	/* Current power state (D0 to D3) */
#define  PCI_PM_CTRL_NO_SOFT_RESET	0x0008	/* No reset for D3hot->D0 */
#define  PCI_PM_CTRL_PME_ENABLE	0x0100	/* PME pin enable */
#define  PCI_PM_CTRL_DATA_SEL_MASK	0x1e00	/* Data select (??) */
#define  PCI_PM_CTRL_DATA_SCALE_MASK	0x6000	/* Data scale (??) */
#define  PCI_PM_CTRL_PME_STATUS	0x8000	/* PME pin status */
#define PCI_PM_PPB_EXTENSIONS	6	/* PPB support extensions (??) */
#define  PCI_PM_PPB_B2_B3	0x40	/* Stop clock when in D3hot (??) */
#define  PCI_PM_BPCC_ENABLE	0x80	/* Bus power/clock control enable (??) */
#define PCI_PM_DATA_REGISTER	7	/* (??) */
#define PCI_PM_SIZEOF		8

/* AGP registers */

#define PCI_AGP_VERSION		2	/* BCD version number */
#define PCI_AGP_RFU		3	/* Rest of capability flags */
#define PCI_AGP_STATUS		4	/* Status register */
#define  PCI_AGP_STATUS_RQ_MASK	0xff000000	/* Maximum number of requests - 1 */
#define  PCI_AGP_STATUS_SBA	0x0200	/* Sideband addressing supported */
#define  PCI_AGP_STATUS_64BIT	0x0020	/* 64-bit addressing supported */
#define  PCI_AGP_STATUS_FW	0x0010	/* FW transfers supported */
#define  PCI_AGP_STATUS_RATE4	0x0004	/* 4x transfer rate supported */
#define  PCI_AGP_STATUS_RATE2	0x0002	/* 2x transfer rate supported */
#define  PCI_AGP_STATUS_RATE1	0x0001	/* 1x transfer rate supported */
#define PCI_AGP_COMMAND		8	/* Control register */
#define  PCI_AGP_COMMAND_RQ_MASK 0xff000000  /* Master: Maximum number of requests */
#define  PCI_AGP_COMMAND_SBA	0x0200	/* Sideband addressing enabled */
#define  PCI_AGP_COMMAND_AGP	0x0100	/* Allow processing of AGP transactions */
#define  PCI_AGP_COMMAND_64BIT	0x0020 	/* Allow processing of 64-bit addresses */
#define  PCI_AGP_COMMAND_FW	0x0010 	/* Force FW transfers */
#define  PCI_AGP_COMMAND_RATE4	0x0004	/* Use 4x rate */
#define  PCI_AGP_COMMAND_RATE2	0x0002	/* Use 2x rate */
#define  PCI_AGP_COMMAND_RATE1	0x0001	/* Use 1x rate */
#define PCI_AGP_SIZEOF		12

/* Vital Product Data */

#define PCI_VPD_ADDR		2	/* Address to access (15 bits!) */
#define  PCI_VPD_ADDR_MASK	0x7fff	/* Address mask */
#define  PCI_VPD_ADDR_F		0x8000	/* Write 0, 1 indicates completion */
#define PCI_VPD_DATA		4	/* 32-bits of data returned here */

/* Slot Identification */

#define PCI_SID_ESR		2	/* Expansion Slot Register */
#define  PCI_SID_ESR_NSLOTS	0x1f	/* Number of expansion slots available */
#define  PCI_SID_ESR_FIC	0x20	/* First In Chassis Flag */
#define PCI_SID_CHASSIS_NR	3	/* Chassis Number */

/* Message Signalled Interrupts registers */

#define PCI_MSI_FLAGS		2	/* Various flags */
#define  PCI_MSI_FLAGS_64BIT	0x80	/* 64-bit addresses allowed */
#define  PCI_MSI_FLAGS_QSIZE	0x70	/* Message queue size configured */
#define  PCI_MSI_FLAGS_QMASK	0x0e	/* Maximum queue size available */
#define  PCI_MSI_FLAGS_ENABLE	0x01	/* MSI feature enabled */
#define  PCI_MSI_FLAGS_MASKBIT	0x100	/* 64-bit mask bits allowed */
#define PCI_MSI_RFU		3	/* Rest of capability flags */
#define PCI_MSI_ADDRESS_LO	4	/* Lower 32 bits */
#define PCI_MSI_ADDRESS_HI	8	/* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set) */
#define PCI_MSI_DATA_32		8	/* 16 bits of data for 32-bit devices */
#define PCI_MSI_MASK_32		12	/* Mask bits register for 32-bit devices */
#define PCI_MSI_DATA_64		12	/* 16 bits of data for 64-bit devices */
#define PCI_MSI_MASK_64		16	/* Mask bits register for 64-bit devices */

/* MSI-X registers (these are at offset PCI_MSIX_FLAGS) */
#define PCI_MSIX_FLAGS		2
#define  PCI_MSIX_FLAGS_QSIZE	0x7FF
#define  PCI_MSIX_FLAGS_ENABLE	(1 << 15)
#define  PCI_MSIX_FLAGS_MASKALL	(1 << 14)
#define PCI_MSIX_FLAGS_BIRMASK	(7 << 0)

/* CompactPCI Hotswap Register */

#define PCI_CHSWP_CSR		2	/* Control and Status Register */
#define  PCI_CHSWP_DHA		0x01	/* Device Hiding Arm */
#define  PCI_CHSWP_EIM		0x02	/* ENUM# Signal Mask */
#define  PCI_CHSWP_PIE		0x04	/* Pending Insert or Extract */
#define  PCI_CHSWP_LOO		0x08	/* LED On / Off */
#define  PCI_CHSWP_PI		0x30	/* Programming Interface */
#define  PCI_CHSWP_EXT		0x40	/* ENUM# status - extraction */
#define  PCI_CHSWP_INS		0x80	/* ENUM# status - insertion */

/* PCI Advanced Feature registers */

#define PCI_AF_LENGTH		2
#define PCI_AF_CAP		3
#define  PCI_AF_CAP_TP		0x01
#define  PCI_AF_CAP_FLR		0x02
#define PCI_AF_CTRL		4
#define  PCI_AF_CTRL_FLR	0x01
#define PCI_AF_STATUS		5
#define  PCI_AF_STATUS_TP	0x01

/* PCI-X registers */

#define PCI_X_CMD		2	/* Modes & Features */
#define  PCI_X_CMD_DPERR_E	0x0001	/* Data Parity Error Recovery Enable */
#define  PCI_X_CMD_ERO		0x0002	/* Enable Relaxed Ordering */
#define  PCI_X_CMD_READ_512	0x0000	/* 512 byte maximum read byte count */
#define  PCI_X_CMD_READ_1K	0x0004	/* 1Kbyte maximum read byte count */
#define  PCI_X_CMD_READ_2K	0x0008	/* 2Kbyte maximum read byte count */
#define  PCI_X_CMD_READ_4K	0x000c	/* 4Kbyte maximum read byte count */
#define  PCI_X_CMD_MAX_READ	0x000c	/* Max Memory Read Byte Count */
				/* Max # of outstanding split transactions */
#define  PCI_X_CMD_SPLIT_1	0x0000	/* Max 1 */
#define  PCI_X_CMD_SPLIT_2	0x0010	/* Max 2 */
#define  PCI_X_CMD_SPLIT_3	0x0020	/* Max 3 */
#define  PCI_X_CMD_SPLIT_4	0x0030	/* Max 4 */
#define  PCI_X_CMD_SPLIT_8	0x0040	/* Max 8 */
#define  PCI_X_CMD_SPLIT_12	0x0050	/* Max 12 */
#define  PCI_X_CMD_SPLIT_16	0x0060	/* Max 16 */
#define  PCI_X_CMD_SPLIT_32	0x0070	/* Max 32 */
#define  PCI_X_CMD_MAX_SPLIT	0x0070	/* Max Outstanding Split Transactions */
#define  PCI_X_CMD_VERSION(x) 	(((x) >> 12) & 3) /* Version */
#define PCI_X_STATUS		4	/* PCI-X capabilities */
#define  PCI_X_STATUS_DEVFN	0x000000ff	/* A copy of devfn */
#define  PCI_X_STATUS_BUS	0x0000ff00	/* A copy of bus nr */
#define  PCI_X_STATUS_64BIT	0x00010000	/* 64-bit device */
#define  PCI_X_STATUS_133MHZ	0x00020000	/* 133 MHz capable */
#define  PCI_X_STATUS_SPL_DISC	0x00040000	/* Split Completion Discarded */
#define  PCI_X_STATUS_UNX_SPL	0x00080000	/* Unexpected Split Completion */
#define  PCI_X_STATUS_COMPLEX	0x00100000	/* Device Complexity */
#define  PCI_X_STATUS_MAX_READ	0x00600000	/* Designed Max Memory Read Count */
#define  PCI_X_STATUS_MAX_SPLIT	0x03800000	/* Designed Max Outstanding Split Transactions */
#define  PCI_X_STATUS_MAX_CUM	0x1c000000	/* Designed Max Cumulative Read Size */
#define  PCI_X_STATUS_SPL_ERR	0x20000000	/* Rcvd Split Completion Error Msg */
#define  PCI_X_STATUS_266MHZ	0x40000000	/* 266 MHz capable */
#define  PCI_X_STATUS_533MHZ	0x80000000	/* 533 MHz capable */

/* PCI Express capability registers */

#define PCI_EXP_FLAGS		2	/* Capabilities register */
#define PCI_EXP_FLAGS_VERS	0x000f	/* Capability version */
#define PCI_EXP_FLAGS_TYPE	0x00f0	/* Device/Port type */
#define  PCI_EXP_TYPE_ENDPOINT	0x0	/* Express Endpoint */
#define  PCI_EXP_TYPE_LEG_END	0x1	/* Legacy Endpoint */
#define  PCI_EXP_TYPE_ROOT_PORT 0x4	/* Root Port */
#define  PCI_EXP_TYPE_UPSTREAM	0x5	/* Upstream Port */
#define  PCI_EXP_TYPE_DOWNSTREAM 0x6	/* Downstream Port */
#define  PCI_EXP_TYPE_PCI_BRIDGE 0x7	/* PCI/PCI-X Bridge */
#define  PCI_EXP_TYPE_RC_END	0x9	/* Root Complex Integrated Endpoint */
#define  PCI_EXP_TYPE_RC_EC	0x10	/* Root Complex Event Collector */
#define PCI_EXP_FLAGS_SLOT	0x0100	/* Slot implemented */
#define PCI_EXP_FLAGS_IRQ	0x3e00	/* Interrupt message number */
#define PCI_EXP_DEVCAP		4	/* Device capabilities */
#define  PCI_EXP_DEVCAP_PAYLOAD	0x07	/* Max_Payload_Size */
#define  PCI_EXP_DEVCAP_PHANTOM	0x18	/* Phantom functions */
#define  PCI_EXP_DEVCAP_EXT_TAG	0x20	/* Extended tags */
#define  PCI_EXP_DEVCAP_L0S	0x1c0	/* L0s Acceptable Latency */
#define  PCI_EXP_DEVCAP_L1	0xe00	/* L1 Acceptable Latency */
#define  PCI_EXP_DEVCAP_ATN_BUT	0x1000	/* Attention Button Present */
#define  PCI_EXP_DEVCAP_ATN_IND	0x2000	/* Attention Indicator Present */
#define  PCI_EXP_DEVCAP_PWR_IND	0x4000	/* Power Indicator Present */
#define  PCI_EXP_DEVCAP_RBER	0x8000	/* Role-Based Error Reporting */
#define  PCI_EXP_DEVCAP_PWR_VAL	0x3fc0000 /* Slot Power Limit Value */
#define  PCI_EXP_DEVCAP_PWR_SCL	0xc000000 /* Slot Power Limit Scale */
#define  PCI_EXP_DEVCAP_FLR     0x10000000 /* Function Level Reset */
#define PCI_EXP_DEVCTL		8	/* Device Control */
#define  PCI_EXP_DEVCTL_CERE	0x0001	/* Correctable Error Reporting En. */
#define  PCI_EXP_DEVCTL_NFERE	0x0002	/* Non-Fatal Error Reporting Enable */
#define  PCI_EXP_DEVCTL_FERE	0x0004	/* Fatal Error Reporting Enable */
#define  PCI_EXP_DEVCTL_URRE	0x0008	/* Unsupported Request Reporting En. */
#define  PCI_EXP_DEVCTL_RELAX_EN 0x0010 /* Enable relaxed ordering */
#define  PCI_EXP_DEVCTL_PAYLOAD	0x00e0	/* Max_Payload_Size */
#define  PCI_EXP_DEVCTL_EXT_TAG	0x0100	/* Extended Tag Field Enable */
#define  PCI_EXP_DEVCTL_PHANTOM	0x0200	/* Phantom Functions Enable */
#define  PCI_EXP_DEVCTL_AUX_PME	0x0400	/* Auxiliary Power PM Enable */
#define  PCI_EXP_DEVCTL_NOSNOOP_EN 0x0800  /* Enable No Snoop */
#define  PCI_EXP_DEVCTL_READRQ	0x7000	/* Max_Read_Request_Size */
#define  PCI_EXP_DEVCTL_BCR_FLR 0x8000  /* Bridge Configuration Retry / FLR */
#define PCI_EXP_DEVSTA		10	/* Device Status */
#define  PCI_EXP_DEVSTA_CED	0x01	/* Correctable Error Detected */
#define  PCI_EXP_DEVSTA_NFED	0x02	/* Non-Fatal Error Detected */
#define  PCI_EXP_DEVSTA_FED	0x04	/* Fatal Error Detected */
#define  PCI_EXP_DEVSTA_URD	0x08	/* Unsupported Request Detected */
#define  PCI_EXP_DEVSTA_AUXPD	0x10	/* AUX Power Detected */
#define  PCI_EXP_DEVSTA_TRPND	0x20	/* Transactions Pending */
#define PCI_EXP_LNKCAP		12	/* Link Capabilities */
#define  PCI_EXP_LNKCAP_SLS	0x0000000f /* Supported Link Speeds */
#define  PCI_EXP_LNKCAP_MLW	0x000003f0 /* Maximum Link Width */
#define  PCI_EXP_LNKCAP_ASPMS	0x00000c00 /* ASPM Support */
#define  PCI_EXP_LNKCAP_L0SEL	0x00007000 /* L0s Exit Latency */
#define  PCI_EXP_LNKCAP_L1EL	0x00038000 /* L1 Exit Latency */
#define  PCI_EXP_LNKCAP_CLKPM	0x00040000 /* L1 Clock Power Management */
#define  PCI_EXP_LNKCAP_SDERC	0x00080000 /* Suprise Down Error Reporting Capable */
#define  PCI_EXP_LNKCAP_DLLLARC	0x00100000 /* Data Link Layer Link Active Reporting Capable */
#define  PCI_EXP_LNKCAP_LBNC	0x00200000 /* Link Bandwidth Notification Capability */
#define  PCI_EXP_LNKCAP_PN	0xff000000 /* Port Number */
#define PCI_EXP_LNKCTL		16	/* Link Control */
#define  PCI_EXP_LNKCTL_ASPMC	0x0003	/* ASPM Control */
#define  PCI_EXP_LNKCTL_RCB	0x0008	/* Read Completion Boundary */
#define  PCI_EXP_LNKCTL_LD	0x0010	/* Link Disable */
#define  PCI_EXP_LNKCTL_RL	0x0020	/* Retrain Link */
#define  PCI_EXP_LNKCTL_CCC	0x0040	/* Common Clock Configuration */
#define  PCI_EXP_LNKCTL_ES	0x0080	/* Extended Synch */
#define  PCI_EXP_LNKCTL_CLKREQ_EN 0x100	/* Enable clkreq */
#define  PCI_EXP_LNKCTL_HAWD	0x0200	/* Hardware Autonomous Width Disable */
#define  PCI_EXP_LNKCTL_LBMIE	0x0400	/* Link Bandwidth Management Interrupt Enable */
#define  PCI_EXP_LNKCTL_LABIE	0x0800	/* Lnk Autonomous Bandwidth Interrupt Enable */
#define PCI_EXP_LNKSTA		18	/* Link Status */
#define  PCI_EXP_LNKSTA_CLS	0x000f	/* Current Link Speed */
#define  PCI_EXP_LNKSTA_NLW	0x03f0	/* Nogotiated Link Width */
#define  PCI_EXP_LNKSTA_LT	0x0800	/* Link Training */
#define  PCI_EXP_LNKSTA_SLC	0x1000	/* Slot Clock Configuration */
#define  PCI_EXP_LNKSTA_DLLLA	0x2000	/* Data Link Layer Link Active */
#define  PCI_EXP_LNKSTA_LBMS	0x4000	/* Link Bandwidth Management Status */
#define  PCI_EXP_LNKSTA_LABS	0x8000	/* Link Autonomous Bandwidth Status */
#define PCI_EXP_SLTCAP		20	/* Slot Capabilities */
#define  PCI_EXP_SLTCAP_ABP	0x00000001 /* Attention Button Present */
#define  PCI_EXP_SLTCAP_PCP	0x00000002 /* Power Controller Present */
#define  PCI_EXP_SLTCAP_MRLSP	0x00000004 /* MRL Sensor Present */
#define  PCI_EXP_SLTCAP_AIP	0x00000008 /* Attention Indicator Present */
#define  PCI_EXP_SLTCAP_PIP	0x00000010 /* Power Indicator Present */
#define  PCI_EXP_SLTCAP_HPS	0x00000020 /* Hot-Plug Surprise */
#define  PCI_EXP_SLTCAP_HPC	0x00000040 /* Hot-Plug Capable */
#define  PCI_EXP_SLTCAP_SPLV	0x00007f80 /* Slot Power Limit Value */
#define  PCI_EXP_SLTCAP_SPLS	0x00018000 /* Slot Power Limit Scale */
#define  PCI_EXP_SLTCAP_EIP	0x00020000 /* Electromechanical Interlock Present */
#define  PCI_EXP_SLTCAP_NCCS	0x00040000 /* No Command Completed Support */
#define  PCI_EXP_SLTCAP_PSN	0xfff80000 /* Physical Slot Number */
#define PCI_EXP_SLTCTL		24	/* Slot Control */
#define  PCI_EXP_SLTCTL_ABPE	0x0001	/* Attention Button Pressed Enable */
#define  PCI_EXP_SLTCTL_PFDE	0x0002	/* Power Fault Detected Enable */
#define  PCI_EXP_SLTCTL_MRLSCE	0x0004	/* MRL Sensor Changed Enable */
#define  PCI_EXP_SLTCTL_PDCE	0x0008	/* Presence Detect Changed Enable */
#define  PCI_EXP_SLTCTL_CCIE	0x0010	/* Command Completed Interrupt Enable */
#define  PCI_EXP_SLTCTL_HPIE	0x0020	/* Hot-Plug Interrupt Enable */
#define  PCI_EXP_SLTCTL_AIC	0x00c0	/* Attention Indicator Control */
#define  PCI_EXP_SLTCTL_PIC	0x0300	/* Power Indicator Control */
#define  PCI_EXP_SLTCTL_PCC	0x0400	/* Power Controller Control */
#define  PCI_EXP_SLTCTL_EIC	0x0800	/* Electromechanical Interlock Control */
#define  PCI_EXP_SLTCTL_DLLSCE	0x1000	/* Data Link Layer State Changed Enable */
#define PCI_EXP_SLTSTA		26	/* Slot Status */
#define  PCI_EXP_SLTSTA_ABP	0x0001	/* Attention Button Pressed */
#define  PCI_EXP_SLTSTA_PFD	0x0002	/* Power Fault Detected */
#define  PCI_EXP_SLTSTA_MRLSC	0x0004	/* MRL Sensor Changed */
#define  PCI_EXP_SLTSTA_PDC	0x0008	/* Presence Detect Changed */
#define  PCI_EXP_SLTSTA_CC	0x0010	/* Command Completed */
#define  PCI_EXP_SLTSTA_MRLSS	0x0020	/* MRL Sensor State */
#define  PCI_EXP_SLTSTA_PDS	0x0040	/* Presence Detect State */
#define  PCI_EXP_SLTSTA_EIS	0x0080	/* Electromechanical Interlock Status */
#define  PCI_EXP_SLTSTA_DLLSC	0x0100	/* Data Link Layer State Changed */
#define PCI_EXP_RTCTL		28	/* Root Control */
#define  PCI_EXP_RTCTL_SECEE	0x01	/* System Error on Correctable Error */
#define  PCI_EXP_RTCTL_SENFEE	0x02	/* System Error on Non-Fatal Error */
#define  PCI_EXP_RTCTL_SEFEE	0x04	/* System Error on Fatal Error */
#define  PCI_EXP_RTCTL_PMEIE	0x08	/* PME Interrupt Enable */
#define  PCI_EXP_RTCTL_CRSSVE	0x10	/* CRS Software Visibility Enable */
#define PCI_EXP_RTCAP		30	/* Root Capabilities */
#define PCI_EXP_RTSTA		32	/* Root Status */
#define PCI_EXP_DEVCAP2		36	/* Device Capabilities 2 */
#define  PCI_EXP_DEVCAP2_ARI	0x20	/* Alternative Routing-ID */
#define PCI_EXP_DEVCTL2		40	/* Device Control 2 */
#define  PCI_EXP_DEVCTL2_ARI	0x20	/* Alternative Routing-ID */
#define PCI_EXP_LNKCTL2		48	/* Link Control 2 */
#define PCI_EXP_SLTCTL2		56	/* Slot Control 2 */

/* Extended Capabilities (PCI-X 2.0 and Express) */
#define PCI_EXT_CAP_ID(header)		(header & 0x0000ffff)
#define PCI_EXT_CAP_VER(header)		((header >> 16) & 0xf)
#define PCI_EXT_CAP_NEXT(header)	((header >> 20) & 0xffc)

#define PCI_EXT_CAP_ID_ERR	1
#define PCI_EXT_CAP_ID_VC	2
#define PCI_EXT_CAP_ID_DSN	3
#define PCI_EXT_CAP_ID_PWR	4
#define PCI_EXT_CAP_ID_ARI	14
#define PCI_EXT_CAP_ID_ATS	15
#define PCI_EXT_CAP_ID_SRIOV	16

/* Advanced Error Reporting */
#define PCI_ERR_UNCOR_STATUS	4	/* Uncorrectable Error Status */
#define  PCI_ERR_UNC_TRAIN	0x00000001	/* Training */
#define  PCI_ERR_UNC_DLP	0x00000010	/* Data Link Protocol */
#define  PCI_ERR_UNC_POISON_TLP	0x00001000	/* Poisoned TLP */
#define  PCI_ERR_UNC_FCP	0x00002000	/* Flow Control Protocol */
#define  PCI_ERR_UNC_COMP_TIME	0x00004000	/* Completion Timeout */
#define  PCI_ERR_UNC_COMP_ABORT	0x00008000	/* Completer Abort */
#define  PCI_ERR_UNC_UNX_COMP	0x00010000	/* Unexpected Completion */
#define  PCI_ERR_UNC_RX_OVER	0x00020000	/* Receiver Overflow */
#define  PCI_ERR_UNC_MALF_TLP	0x00040000	/* Malformed TLP */
#define  PCI_ERR_UNC_ECRC	0x00080000	/* ECRC Error Status */
#define  PCI_ERR_UNC_UNSUP	0x00100000	/* Unsupported Request */
#define PCI_ERR_UNCOR_MASK	8	/* Uncorrectable Error Mask */
	/* Same bits as above */
#define PCI_ERR_UNCOR_SEVER	12	/* Uncorrectable Error Severity */
	/* Same bits as above */
#define PCI_ERR_COR_STATUS	16	/* Correctable Error Status */
#define  PCI_ERR_COR_RCVR	0x00000001	/* Receiver Error Status */
#define  PCI_ERR_COR_BAD_TLP	0x00000040	/* Bad TLP Status */
#define  PCI_ERR_COR_BAD_DLLP	0x00000080	/* Bad DLLP Status */
#define  PCI_ERR_COR_REP_ROLL	0x00000100	/* REPLAY_NUM Rollover */
#define  PCI_ERR_COR_REP_TIMER	0x00001000	/* Replay Timer Timeout */
#define PCI_ERR_COR_MASK	20	/* Correctable Error Mask */
	/* Same bits as above */
#define PCI_ERR_CAP		24	/* Advanced Error Capabilities */
#define  PCI_ERR_CAP_FEP(x)	((x) & 31)	/* First Error Pointer */
#define  PCI_ERR_CAP_ECRC_GENC	0x00000020	/* ECRC Generation Capable */
#define  PCI_ERR_CAP_ECRC_GENE	0x00000040	/* ECRC Generation Enable */
#define  PCI_ERR_CAP_ECRC_CHKC	0x00000080	/* ECRC Check Capable */
#define  PCI_ERR_CAP_ECRC_CHKE	0x00000100	/* ECRC Check Enable */
#define PCI_ERR_HEADER_LOG	28	/* Header Log Register (16 bytes) */
#define PCI_ERR_ROOT_COMMAND	44	/* Root Error Command */
/* Correctable Err Reporting Enable */
#define PCI_ERR_ROOT_CMD_COR_EN		0x00000001
/* Non-fatal Err Reporting Enable */
#define PCI_ERR_ROOT_CMD_NONFATAL_EN	0x00000002
/* Fatal Err Reporting Enable */
#define PCI_ERR_ROOT_CMD_FATAL_EN	0x00000004
#define PCI_ERR_ROOT_STATUS	48
#define PCI_ERR_ROOT_COR_RCV		0x00000001	/* ERR_COR Received */
/* Multi ERR_COR Received */
#define PCI_ERR_ROOT_MULTI_COR_RCV	0x00000002
/* ERR_FATAL/NONFATAL Recevied */
#define PCI_ERR_ROOT_UNCOR_RCV		0x00000004
/* Multi ERR_FATAL/NONFATAL Recevied */
#define PCI_ERR_ROOT_MULTI_UNCOR_RCV	0x00000008
#define PCI_ERR_ROOT_FIRST_FATAL	0x00000010	/* First Fatal */
#define PCI_ERR_ROOT_NONFATAL_RCV	0x00000020	/* Non-Fatal Received */
#define PCI_ERR_ROOT_FATAL_RCV		0x00000040	/* Fatal Received */
#define PCI_ERR_ROOT_COR_SRC	52
#define PCI_ERR_ROOT_SRC	54

/* Virtual Channel */
#define PCI_VC_PORT_REG1	4
#define PCI_VC_PORT_REG2	8
#define PCI_VC_PORT_CTRL	12
#define PCI_VC_PORT_STATUS	14
#define PCI_VC_RES_CAP		16
#define PCI_VC_RES_CTRL		20
#define PCI_VC_RES_STATUS	26

/* Power Budgeting */
#define PCI_PWR_DSR		4	/* Data Select Register */
#define PCI_PWR_DATA		8	/* Data Register */
#define  PCI_PWR_DATA_BASE(x)	((x) & 0xff)	    /* Base Power */
#define  PCI_PWR_DATA_SCALE(x)	(((x) >> 8) & 3)    /* Data Scale */
#define  PCI_PWR_DATA_PM_SUB(x)	(((x) >> 10) & 7)   /* PM Sub State */
#define  PCI_PWR_DATA_PM_STATE(x) (((x) >> 13) & 3) /* PM State */
#define  PCI_PWR_DATA_TYPE(x)	(((x) >> 15) & 7)   /* Type */
#define  PCI_PWR_DATA_RAIL(x)	(((x) >> 18) & 7)   /* Power Rail */
#define PCI_PWR_CAP		12	/* Capability */
#define  PCI_PWR_CAP_BUDGET(x)	((x) & 1)	/* Included in system budget */

/*
 * Hypertransport sub capability types
 *
 * Unfortunately there are both 3 bit and 5 bit capability types defined
 * in the HT spec, catering for that is a little messy. You probably don't
 * want to use these directly, just use pci_find_ht_capability() and it
 * will do the right thing for you.
 */
#define HT_3BIT_CAP_MASK	0xE0
#define HT_CAPTYPE_SLAVE	0x00	/* Slave/Primary link configuration */
#define HT_CAPTYPE_HOST		0x20	/* Host/Secondary link configuration */

#define HT_5BIT_CAP_MASK	0xF8
#define HT_CAPTYPE_IRQ		0x80	/* IRQ Configuration */
#define HT_CAPTYPE_REMAPPING_40	0xA0	/* 40 bit address remapping */
#define HT_CAPTYPE_REMAPPING_64 0xA2	/* 64 bit address remapping */
#define HT_CAPTYPE_UNITID_CLUMP	0x90	/* Unit ID clumping */
#define HT_CAPTYPE_EXTCONF	0x98	/* Extended Configuration Space Access */
#define HT_CAPTYPE_MSI_MAPPING	0xA8	/* MSI Mapping Capability */
#define  HT_MSI_FLAGS		0x02		/* Offset to flags */
#define  HT_MSI_FLAGS_ENABLE	0x1		/* Mapping enable */
#define  HT_MSI_FLAGS_FIXED	0x2		/* Fixed mapping only */
#define  HT_MSI_FIXED_ADDR	0x00000000FEE00000ULL	/* Fixed addr */
#define  HT_MSI_ADDR_LO		0x04		/* Offset to low addr bits */
#define  HT_MSI_ADDR_LO_MASK	0xFFF00000	/* Low address bit mask */
#define  HT_MSI_ADDR_HI		0x08		/* Offset to high addr bits */
#define HT_CAPTYPE_DIRECT_ROUTE	0xB0	/* Direct routing configuration */
#define HT_CAPTYPE_VCSET	0xB8	/* Virtual Channel configuration */
#define HT_CAPTYPE_ERROR_RETRY	0xC0	/* Retry on error configuration */
#define HT_CAPTYPE_GEN3		0xD0	/* Generation 3 hypertransport configuration */
#define HT_CAPTYPE_PM		0xE0	/* Hypertransport powermanagement configuration */

/* Alternative Routing-ID Interpretation */
#define PCI_ARI_CAP		0x04	/* ARI Capability Register */
#define  PCI_ARI_CAP_MFVC	0x0001	/* MFVC Function Groups Capability */
#define  PCI_ARI_CAP_ACS	0x0002	/* ACS Function Groups Capability */
#define  PCI_ARI_CAP_NFN(x)	(((x) >> 8) & 0xff) /* Next Function Number */
#define PCI_ARI_CTRL		0x06	/* ARI Control Register */
#define  PCI_ARI_CTRL_MFVC	0x0001	/* MFVC Function Groups Enable */
#define  PCI_ARI_CTRL_ACS	0x0002	/* ACS Function Groups Enable */
#define  PCI_ARI_CTRL_FG(x)	(((x) >> 4) & 7) /* Function Group */

/* Address Translation Service */
#define PCI_ATS_CAP		0x04	/* ATS Capability Register */
#define  PCI_ATS_CAP_QDEP(x)	((x) & 0x1f)	/* Invalidate Queue Depth */
#define  PCI_ATS_MAX_QDEP	32	/* Max Invalidate Queue Depth */
#define PCI_ATS_CTRL		0x06	/* ATS Control Register */
#define  PCI_ATS_CTRL_ENABLE	0x8000	/* ATS Enable */
#define  PCI_ATS_CTRL_STU(x)	((x) & 0x1f)	/* Smallest Translation Unit */
#define  PCI_ATS_MIN_STU	12	/* shift of minimum STU block */

/* Single Root I/O Virtualization */
#define PCI_SRIOV_CAP		0x04	/* SR-IOV Capabilities */
#define  PCI_SRIOV_CAP_VFM	0x01	/* VF Migration Capable */
#define  PCI_SRIOV_CAP_INTR(x)	((x) >> 21) /* Interrupt Message Number */
#define PCI_SRIOV_CTRL		0x08	/* SR-IOV Control */
#define  PCI_SRIOV_CTRL_VFE	0x01	/* VF Enable */
#define  PCI_SRIOV_CTRL_VFM	0x02	/* VF Migration Enable */
#define  PCI_SRIOV_CTRL_INTR	0x04	/* VF Migration Interrupt Enable */
#define  PCI_SRIOV_CTRL_MSE	0x08	/* VF Memory Space Enable */
#define  PCI_SRIOV_CTRL_ARI	0x10	/* ARI Capable Hierarchy */
#define PCI_SRIOV_STATUS	0x0a	/* SR-IOV Status */
#define  PCI_SRIOV_STATUS_VFM	0x01	/* VF Migration Status */
#define PCI_SRIOV_INITIAL_VF	0x0c	/* Initial VFs */
#define PCI_SRIOV_TOTAL_VF	0x0e	/* Total VFs */
#define PCI_SRIOV_NUM_VF	0x10	/* Number of VFs */
#define PCI_SRIOV_FUNC_LINK	0x12	/* Function Dependency Link */
#define PCI_SRIOV_VF_OFFSET	0x14	/* First VF Offset */
#define PCI_SRIOV_VF_STRIDE	0x16	/* Following VF Stride */
#define PCI_SRIOV_VF_DID	0x1a	/* VF Device ID */
#define PCI_SRIOV_SUP_PGSIZE	0x1c	/* Supported Page Sizes */
#define PCI_SRIOV_SYS_PGSIZE	0x20	/* System Page Size */
#define PCI_SRIOV_BAR		0x24	/* VF BAR0 */
#define  PCI_SRIOV_NUM_BARS	6	/* Number of VF BARs */
#define PCI_SRIOV_VFM		0x3c	/* VF Migration State Array Offset*/
#define  PCI_SRIOV_VFM_BIR(x)	((x) & 7)	/* State BIR */
#define  PCI_SRIOV_VFM_OFFSET(x) ((x) & ~7)	/* State Offset */
#define  PCI_SRIOV_VFM_UA	0x0	/* Inactive.Unavailable */
#define  PCI_SRIOV_VFM_MI	0x1	/* Dormant.MigrateIn */
#define  PCI_SRIOV_VFM_MO	0x2	/* Active.MigrateOut */
#define  PCI_SRIOV_VFM_AV	0x3	/* Active.Available */


/* Include the ID list */

#include <linux/pci_ids.h>

/*
 * The PCI interface treats multi-function devices as independent
 * devices.  The slot/function address of each device is encoded
 * in a single byte as follows:
 *
 *	7:3 = slot
 *	2:0 = function
 */
#define PCI_DEVFN(slot,func)	((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)		(((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)		((devfn) & 0x07)

/* Ioctls for /proc/bus/pci/X/Y nodes. */
#define PCIIOC_BASE		('P' << 24 | 'C' << 16 | 'I' << 8)
#define PCIIOC_CONTROLLER	(PCIIOC_BASE | 0x00)	/* Get controller for PCI device. */
#define PCIIOC_MMAP_IS_IO	(PCIIOC_BASE | 0x01)	/* Set mmap state to I/O space. */
#define PCIIOC_MMAP_IS_MEM	(PCIIOC_BASE | 0x02)	/* Set mmap state to MEM space. */
#define PCIIOC_WRITE_COMBINE	(PCIIOC_BASE | 0x03)	/* Enable/disable write-combining. */

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/config.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/device.h>

/* File state for mmap()s on /proc/bus/pci/X/Y */
enum pci_mmap_state {
	pci_mmap_io,
	pci_mmap_mem
};

/* This defines the direction arg to the DMA mapping routines. */
#define PCI_DMA_BIDIRECTIONAL	0
#define PCI_DMA_TODEVICE	1
#define PCI_DMA_FROMDEVICE	2
#define PCI_DMA_NONE		3

#define DEVICE_COUNT_COMPATIBLE	4
#define DEVICE_COUNT_RESOURCE	12

typedef int __bitwise pci_power_t;

#define PCI_D0	((pci_power_t __force) 0)
#define PCI_D1	((pci_power_t __force) 1)
#define PCI_D2	((pci_power_t __force) 2)
#define PCI_D3hot	((pci_power_t __force) 3)
#define PCI_D3cold	((pci_power_t __force) 4)
#define PCI_POWER_ERROR	((pci_power_t __force) -1)

/*
 * The pci_dev structure is used to describe PCI devices.
 */
struct pci_dev {
	struct list_head global_list;	/* node in list of all PCI devices */
	struct list_head bus_list;	/* node in per-bus list */
	struct pci_bus	*bus;		/* bus this device is on */
	struct pci_bus	*subordinate;	/* bus this device bridges to */

	void		*sysdata;	/* hook for sys-specific extension */
	struct proc_dir_entry *procent;	/* device entry in /proc/bus/pci */

	unsigned int	devfn;		/* encoded device & function index */
	unsigned short	vendor;
	unsigned short	device;
	unsigned short	subsystem_vendor;
	unsigned short	subsystem_device;
	unsigned int	class;		/* 3 bytes: (base,sub,prog-if) */
	u8		hdr_type;	/* PCI header type (`multi' flag masked out) */
	u8		rom_base_reg;	/* which config register controls the ROM */

	struct pci_driver *driver;	/* which driver has allocated this device */
	u64		dma_mask;	/* Mask of the bits of bus address this
					   device implements.  Normally this is
					   0xffffffff.  You only need to change
					   this if your device has broken DMA
					   or supports 64-bit transfers.  */

	pci_power_t     current_state;  /* Current operating state. In ACPI-speak,
					   this is D0-D3, D0 being fully functional,
					   and D3 being off. */

	struct	device	dev;		/* Generic device interface */

	/* device is compatible with these IDs */
	unsigned short vendor_compatible[DEVICE_COUNT_COMPATIBLE];
	unsigned short device_compatible[DEVICE_COUNT_COMPATIBLE];

	int		cfg_size;	/* Size of configuration space */

	/*
	 * Instead of touching interrupt line and base address registers
	 * directly, use the values stored here. They might be different!
	 */
	unsigned int	irq;
	struct resource resource[DEVICE_COUNT_RESOURCE]; /* I/O and memory regions + expansion ROMs */

	/* These fields are used by common fixups */
	unsigned int	transparent:1;	/* Transparent PCI bridge */
	unsigned int	multifunction:1;/* Part of multi-function device */
	/* keep track of device state */
	unsigned int	is_enabled:1;	/* pci_enable_device has been called */
	unsigned int	is_busmaster:1; /* device is busmaster */
	
	u32		saved_config_space[16]; /* config space saved at suspend time */
	struct bin_attribute *rom_attr; /* attribute descriptor for sysfs ROM entry */
	int rom_attr_enabled;		/* has display of the rom attribute been enabled? */
	struct bin_attribute *res_attr[DEVICE_COUNT_RESOURCE]; /* sysfs file for resources */
#ifdef CONFIG_PCI_NAMES
#define PCI_NAME_SIZE	255
#define PCI_NAME_HALF	__stringify(43)	/* less than half to handle slop */
	char		pretty_name[PCI_NAME_SIZE];	/* pretty name for users to see */
#endif
};

#define pci_dev_g(n) list_entry(n, struct pci_dev, global_list)
#define pci_dev_b(n) list_entry(n, struct pci_dev, bus_list)
#define	to_pci_dev(n) container_of(n, struct pci_dev, dev)
#define for_each_pci_dev(d) while ((d = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, d)) != NULL)

/*
 *  For PCI devices, the region numbers are assigned this way:
 *
 *	0-5	standard PCI regions
 *	6	expansion ROM
 *	7-10	bridges: address space assigned to buses behind the bridge
 */

#define PCI_ROM_RESOURCE 6
#define PCI_BRIDGE_RESOURCES 7
#define PCI_NUM_RESOURCES 11

#ifndef PCI_BUS_NUM_RESOURCES
#define PCI_BUS_NUM_RESOURCES 4
#endif
  
#define PCI_REGION_FLAG_MASK 0x0fU	/* These bits of resource flags tell us the PCI region flags */

struct pci_bus {
	struct list_head node;		/* node in list of buses */
	struct pci_bus	*parent;	/* parent bus this bridge is on */
	struct list_head children;	/* list of child buses */
	struct list_head devices;	/* list of devices on this bus */
	struct pci_dev	*self;		/* bridge device as seen by parent */
	struct resource	*resource[PCI_BUS_NUM_RESOURCES];
					/* address space routed to this bus */

	struct pci_ops	*ops;		/* configuration access functions */
	void		*sysdata;	/* hook for sys-specific extension */
	struct proc_dir_entry *procdir;	/* directory entry in /proc/bus/pci */

	unsigned char	number;		/* bus number */
	unsigned char	primary;	/* number of primary bridge */
	unsigned char	secondary;	/* number of secondary bridge */
	unsigned char	subordinate;	/* max number of subordinate buses */

	char		name[48];

	unsigned short  bridge_ctl;	/* manage NO_ISA/FBB/et al behaviors */
	unsigned short  pad2;
	struct device		*bridge;
	struct class_device	class_dev;
	struct bin_attribute	*legacy_io; /* legacy I/O for this bus */
	struct bin_attribute	*legacy_mem; /* legacy mem */
};

#define pci_bus_b(n)	list_entry(n, struct pci_bus, node)
#define to_pci_bus(n)	container_of(n, struct pci_bus, class_dev)

/*
 * Error values that may be returned by PCI functions.
 */
#define PCIBIOS_SUCCESSFUL		0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED	0x81
#define PCIBIOS_BAD_VENDOR_ID		0x83
#define PCIBIOS_DEVICE_NOT_FOUND	0x86
#define PCIBIOS_BAD_REGISTER_NUMBER	0x87
#define PCIBIOS_SET_FAILED		0x88
#define PCIBIOS_BUFFER_TOO_SMALL	0x89

/* Low-level architecture-dependent routines */

struct pci_ops {
	int (*read)(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val);
	int (*write)(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val);
};

struct pci_raw_ops {
	int (*read)(unsigned int domain, unsigned int bus, unsigned int devfn,
		    int reg, int len, u32 *val);
	int (*write)(unsigned int domain, unsigned int bus, unsigned int devfn,
		     int reg, int len, u32 val);
};

extern struct pci_raw_ops *raw_pci_ops;

struct pci_bus_region {
	unsigned long start;
	unsigned long end;
};

struct pci_dynids {
	spinlock_t lock;            /* protects list, index */
	struct list_head list;      /* for IDs added at runtime */
	unsigned int use_driver_data:1; /* pci_driver->driver_data is used */
};

struct module;
struct pci_driver {
	struct list_head node;
	char *name;
	struct module *owner;
	const struct pci_device_id *id_table;	/* must be non-NULL for probe to be called */
	int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);	/* New device inserted */
	void (*remove) (struct pci_dev *dev);	/* Device removed (NULL if not a hot-plug capable driver) */
	int  (*suspend) (struct pci_dev *dev, pm_message_t state);	/* Device suspended */
	int  (*resume) (struct pci_dev *dev);	                /* Device woken up */
	int  (*enable_wake) (struct pci_dev *dev, pci_power_t state, int enable);   /* Enable wake event */
	void (*shutdown) (struct pci_dev *dev);

	struct device_driver	driver;
	struct pci_dynids dynids;
};

#define	to_pci_driver(drv) container_of(drv,struct pci_driver, driver)

/**
 * PCI_DEVICE - macro used to describe a specific pci device
 * @vend: the 16 bit PCI Vendor ID
 * @dev: the 16 bit PCI Device ID
 *
 * This macro is used to create a struct pci_device_id that matches a
 * specific device.  The subvendor and subdevice fields will be set to
 * PCI_ANY_ID.
 */
#define PCI_DEVICE(vend,dev) \
	.vendor = (vend), .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID

/**
 * PCI_DEVICE_CLASS - macro used to describe a specific pci device class
 * @dev_class: the class, subclass, prog-if triple for this device
 * @dev_class_mask: the class mask for this device
 *
 * This macro is used to create a struct pci_device_id that matches a
 * specific PCI class.  The vendor, device, subvendor, and subdevice 
 * fields will be set to PCI_ANY_ID.
 */
#define PCI_DEVICE_CLASS(dev_class,dev_class_mask) \
	.class = (dev_class), .class_mask = (dev_class_mask), \
	.vendor = PCI_ANY_ID, .device = PCI_ANY_ID, \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID

/* 
 * pci_module_init is obsolete, this stays here till we fix up all usages of it
 * in the tree.
 */
#define pci_module_init	pci_register_driver

/* these external functions are only available when PCI support is enabled */
#ifdef CONFIG_PCI

extern struct bus_type pci_bus_type;

/* Do NOT directly access these two variables, unless you are arch specific pci
 * code, or pci core code. */
extern struct list_head pci_root_buses;	/* list of all known PCI buses */
extern struct list_head pci_devices;	/* list of all devices */

void pcibios_fixup_bus(struct pci_bus *);
int pcibios_enable_device(struct pci_dev *, int mask);
char *pcibios_setup (char *str);

/* Used only when drivers/pci/setup.c is used */
void pcibios_align_resource(void *, struct resource *,
			    unsigned long, unsigned long);
void pcibios_update_irq(struct pci_dev *, int irq);

/* Generic PCI functions used internally */

extern struct pci_bus *pci_find_bus(int domain, int busnr);
struct pci_bus *pci_scan_bus_parented(struct device *parent, int bus, struct pci_ops *ops, void *sysdata);
static inline struct pci_bus *pci_scan_bus(int bus, struct pci_ops *ops, void *sysdata)
{
	return pci_scan_bus_parented(NULL, bus, ops, sysdata);
}
int pci_scan_slot(struct pci_bus *bus, int devfn);
struct pci_dev * pci_scan_single_device(struct pci_bus *bus, int devfn);
unsigned int pci_scan_child_bus(struct pci_bus *bus);
void pci_bus_add_device(struct pci_dev *dev);
void pci_bus_add_devices(struct pci_bus *bus);
void pci_name_device(struct pci_dev *dev);
char *pci_class_name(u32 class);
void pci_read_bridge_bases(struct pci_bus *child);
struct resource *pci_find_parent_resource(const struct pci_dev *dev, struct resource *res);
int pci_get_interrupt_pin(struct pci_dev *dev, struct pci_dev **bridge);
extern struct pci_dev *pci_dev_get(struct pci_dev *dev);
extern void pci_dev_put(struct pci_dev *dev);
extern void pci_remove_bus(struct pci_bus *b);
extern void pci_remove_bus_device(struct pci_dev *dev);

/* Generic PCI functions exported to card drivers */

struct pci_dev *pci_find_device (unsigned int vendor, unsigned int device, const struct pci_dev *from);
struct pci_dev *pci_find_device_reverse (unsigned int vendor, unsigned int device, const struct pci_dev *from);
struct pci_dev *pci_find_slot (unsigned int bus, unsigned int devfn);
int pci_find_capability (struct pci_dev *dev, int cap);
int pci_find_ext_capability (struct pci_dev *dev, int cap);
struct pci_bus * pci_find_next_bus(const struct pci_bus *from);

struct pci_dev *pci_get_device (unsigned int vendor, unsigned int device, struct pci_dev *from);
struct pci_dev *pci_get_subsys (unsigned int vendor, unsigned int device,
				unsigned int ss_vendor, unsigned int ss_device,
				struct pci_dev *from);
struct pci_dev *pci_get_slot (struct pci_bus *bus, unsigned int devfn);
struct pci_dev *pci_get_class (unsigned int class, struct pci_dev *from);
int pci_dev_present(const struct pci_device_id *ids);

int pci_bus_read_config_byte (struct pci_bus *bus, unsigned int devfn, int where, u8 *val);
int pci_bus_read_config_word (struct pci_bus *bus, unsigned int devfn, int where, u16 *val);
int pci_bus_read_config_dword (struct pci_bus *bus, unsigned int devfn, int where, u32 *val);
int pci_bus_write_config_byte (struct pci_bus *bus, unsigned int devfn, int where, u8 val);
int pci_bus_write_config_word (struct pci_bus *bus, unsigned int devfn, int where, u16 val);
int pci_bus_write_config_dword (struct pci_bus *bus, unsigned int devfn, int where, u32 val);

static inline int pci_read_config_byte(struct pci_dev *dev, int where, u8 *val)
{
	return pci_bus_read_config_byte (dev->bus, dev->devfn, where, val);
}
static inline int pci_read_config_word(struct pci_dev *dev, int where, u16 *val)
{
	return pci_bus_read_config_word (dev->bus, dev->devfn, where, val);
}
static inline int pci_read_config_dword(struct pci_dev *dev, int where, u32 *val)
{
	return pci_bus_read_config_dword (dev->bus, dev->devfn, where, val);
}
static inline int pci_write_config_byte(struct pci_dev *dev, int where, u8 val)
{
	return pci_bus_write_config_byte (dev->bus, dev->devfn, where, val);
}
static inline int pci_write_config_word(struct pci_dev *dev, int where, u16 val)
{
	return pci_bus_write_config_word (dev->bus, dev->devfn, where, val);
}
static inline int pci_write_config_dword(struct pci_dev *dev, int where, u32 val)
{
	return pci_bus_write_config_dword (dev->bus, dev->devfn, where, val);
}

int pci_enable_device(struct pci_dev *dev);
int pci_enable_device_bars(struct pci_dev *dev, int mask);
void pci_disable_device(struct pci_dev *dev);
void pci_set_master(struct pci_dev *dev);
#define HAVE_PCI_SET_MWI
int pci_set_mwi(struct pci_dev *dev);
void pci_clear_mwi(struct pci_dev *dev);
int pci_set_dma_mask(struct pci_dev *dev, u64 mask);
int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask);
int pci_assign_resource(struct pci_dev *dev, int i);

/* ROM control related routines */
void __iomem *pci_map_rom(struct pci_dev *pdev, size_t *size);
void __iomem *pci_map_rom_copy(struct pci_dev *pdev, size_t *size);
void pci_unmap_rom(struct pci_dev *pdev, void __iomem *rom);
void pci_remove_rom(struct pci_dev *pdev);

/* Power management related routines */
int pci_save_state(struct pci_dev *dev);
int pci_restore_state(struct pci_dev *dev);
int pci_set_power_state(struct pci_dev *dev, pci_power_t state);
pci_power_t pci_choose_state(struct pci_dev *dev, pm_message_t state);
int pci_enable_wake(struct pci_dev *dev, pci_power_t state, int enable);

/* Helper functions for low-level code (drivers/pci/setup-[bus,res].c) */
void pci_bus_assign_resources(struct pci_bus *bus);
void pci_bus_size_bridges(struct pci_bus *bus);
int pci_claim_resource(struct pci_dev *, int);
void pci_assign_unassigned_resources(void);
void pdev_enable_device(struct pci_dev *);
void pdev_sort_resources(struct pci_dev *, struct resource_list *);
void pci_fixup_irqs(u8 (*)(struct pci_dev *, u8 *),
		    int (*)(struct pci_dev *, u8, u8));
#define HAVE_PCI_REQ_REGIONS	2
int pci_request_regions(struct pci_dev *, char *);
void pci_release_regions(struct pci_dev *);
int pci_request_region(struct pci_dev *, int, char *);
void pci_release_region(struct pci_dev *, int);

/* drivers/pci/bus.c */
int pci_bus_alloc_resource(struct pci_bus *bus, struct resource *res,
			   unsigned long size, unsigned long align,
			   unsigned long min, unsigned int type_mask,
			   void (*alignf)(void *, struct resource *,
					  unsigned long, unsigned long),
			   void *alignf_data);
void pci_enable_bridges(struct pci_bus *bus);

/* New-style probing supporting hot-pluggable devices */
int pci_register_driver(struct pci_driver *);
void pci_unregister_driver(struct pci_driver *);
void pci_remove_behind_bridge(struct pci_dev *);
struct pci_driver *pci_dev_driver(const struct pci_dev *);
const struct pci_device_id *pci_match_device(const struct pci_device_id *ids, const struct pci_dev *dev);
int pci_scan_bridge(struct pci_bus *bus, struct pci_dev * dev, int max, int pass);

/* kmem_cache style wrapper around pci_alloc_consistent() */

#include <linux/dmapool.h>

#define	pci_pool dma_pool
#define pci_pool_create(name, pdev, size, align, allocation) \
		dma_pool_create(name, &pdev->dev, size, align, allocation)
#define	pci_pool_destroy(pool) dma_pool_destroy(pool)
#define	pci_pool_alloc(pool, flags, handle) dma_pool_alloc(pool, flags, handle)
#define	pci_pool_free(pool, vaddr, addr) dma_pool_free(pool, vaddr, addr)

#if defined(CONFIG_ISA) || defined(CONFIG_EISA)
extern struct pci_dev *isa_bridge;
#endif

struct msix_entry {
	u16 	vector;	/* kernel uses to write allocated vector */
	u16	entry;	/* driver uses to specify entry, OS writes */
};

#ifndef CONFIG_PCI_MSI
static inline void pci_scan_msi_device(struct pci_dev *dev) {}
static inline int pci_enable_msi(struct pci_dev *dev) {return -1;}
static inline void pci_disable_msi(struct pci_dev *dev) {}
static inline int pci_enable_msix(struct pci_dev* dev,
	struct msix_entry *entries, int nvec) {return -1;}
static inline void pci_disable_msix(struct pci_dev *dev) {}
static inline void msi_remove_pci_irq_vectors(struct pci_dev *dev) {}
#else
extern void pci_scan_msi_device(struct pci_dev *dev);
extern int pci_enable_msi(struct pci_dev *dev);
extern void pci_disable_msi(struct pci_dev *dev);
extern int pci_enable_msix(struct pci_dev* dev,
	struct msix_entry *entries, int nvec);
extern void pci_disable_msix(struct pci_dev *dev);
extern void msi_remove_pci_irq_vectors(struct pci_dev *dev);
#endif

#endif /* CONFIG_PCI */

/* Include architecture-dependent settings and functions */

#include <asm/pci.h>

/*
 *  If the system does not have PCI, clearly these return errors.  Define
 *  these as simple inline functions to avoid hair in drivers.
 */

#ifndef CONFIG_PCI
#define _PCI_NOP(o,s,t) \
	static inline int pci_##o##_config_##s (struct pci_dev *dev, int where, t val) \
		{ return PCIBIOS_FUNC_NOT_SUPPORTED; }
#define _PCI_NOP_ALL(o,x)	_PCI_NOP(o,byte,u8 x) \
				_PCI_NOP(o,word,u16 x) \
				_PCI_NOP(o,dword,u32 x)
_PCI_NOP_ALL(read, *)
_PCI_NOP_ALL(write,)

static inline struct pci_dev *pci_find_device(unsigned int vendor, unsigned int device, const struct pci_dev *from)
{ return NULL; }

static inline struct pci_dev *pci_find_slot(unsigned int bus, unsigned int devfn)
{ return NULL; }

static inline struct pci_dev *pci_get_device (unsigned int vendor, unsigned int device, struct pci_dev *from)
{ return NULL; }

static inline struct pci_dev *pci_get_subsys (unsigned int vendor, unsigned int device,
unsigned int ss_vendor, unsigned int ss_device, struct pci_dev *from)
{ return NULL; }

static inline struct pci_dev *pci_get_class(unsigned int class, struct pci_dev *from)
{ return NULL; }

#define pci_dev_present(ids)	(0)
#define pci_dev_put(dev)	do { } while (0)

static inline void pci_set_master(struct pci_dev *dev) { }
static inline int pci_enable_device(struct pci_dev *dev) { return -EIO; }
static inline void pci_disable_device(struct pci_dev *dev) { }
static inline int pci_set_dma_mask(struct pci_dev *dev, u64 mask) { return -EIO; }
static inline int pci_assign_resource(struct pci_dev *dev, int i) { return -EBUSY;}
static inline int pci_register_driver(struct pci_driver *drv) { return 0;}
static inline void pci_unregister_driver(struct pci_driver *drv) { }
static inline int pci_find_capability (struct pci_dev *dev, int cap) {return 0; }
static inline int pci_find_ext_capability (struct pci_dev *dev, int cap) {return 0; }
static inline const struct pci_device_id *pci_match_device(const struct pci_device_id *ids, const struct pci_dev *dev) { return NULL; }

/* Power management related routines */
static inline int pci_save_state(struct pci_dev *dev) { return 0; }
static inline int pci_restore_state(struct pci_dev *dev) { return 0; }
static inline int pci_set_power_state(struct pci_dev *dev, pci_power_t state) { return 0; }
static inline pci_power_t pci_choose_state(struct pci_dev *dev, pm_message_t state) { return PCI_D0; }
static inline int pci_enable_wake(struct pci_dev *dev, pci_power_t state, int enable) { return 0; }

#define	isa_bridge	((struct pci_dev *)NULL)

#else

/*
 * PCI domain support.  Sometimes called PCI segment (eg by ACPI),
 * a PCI domain is defined to be a set of PCI busses which share
 * configuration space.
 */
#ifndef CONFIG_PCI_DOMAINS
static inline int pci_domain_nr(struct pci_bus *bus) { return 0; }
static inline int pci_proc_domain(struct pci_bus *bus)
{
	return 0;
}
#endif

#endif /* !CONFIG_PCI */

/* these helpers provide future and backwards compatibility
 * for accessing popular PCI BAR info */
#define pci_resource_start(dev,bar)   ((dev)->resource[(bar)].start)
#define pci_resource_end(dev,bar)     ((dev)->resource[(bar)].end)
#define pci_resource_flags(dev,bar)   ((dev)->resource[(bar)].flags)
#define pci_resource_len(dev,bar) \
	((pci_resource_start((dev),(bar)) == 0 &&	\
	  pci_resource_end((dev),(bar)) ==		\
	  pci_resource_start((dev),(bar))) ? 0 :	\
	  						\
	 (pci_resource_end((dev),(bar)) -		\
	  pci_resource_start((dev),(bar)) + 1))

/* Similar to the helpers above, these manipulate per-pci_dev
 * driver-specific data.  They are really just a wrapper around
 * the generic device structure functions of these calls.
 */
static inline void *pci_get_drvdata (struct pci_dev *pdev)
{
	return dev_get_drvdata(&pdev->dev);
}

static inline void pci_set_drvdata (struct pci_dev *pdev, void *data)
{
	dev_set_drvdata(&pdev->dev, data);
}

/* If you want to know what to call your pci_dev, ask this function.
 * Again, it's a wrapper around the generic device.
 */
static inline char *pci_name(struct pci_dev *pdev)
{
	return pdev->dev.bus_id;
}

/* Some archs want to see the pretty pci name, so use this macro */
#ifdef CONFIG_PCI_NAMES
#define pci_pretty_name(dev) ((dev)->pretty_name)
#else
#define pci_pretty_name(dev) ""
#endif

/*
 *  The world is not perfect and supplies us with broken PCI devices.
 *  For at least a part of these bugs we need a work-around, so both
 *  generic (drivers/pci/quirks.c) and per-architecture code can define
 *  fixup hooks to be called for particular buggy devices.
 */

struct pci_fixup {
	u16 vendor, device;	/* You can use PCI_ANY_ID here of course */
	void (*hook)(struct pci_dev *dev);
};

enum pci_fixup_pass {
	pci_fixup_early,	/* Before probing BARs */
	pci_fixup_header,	/* After reading configuration header */
	pci_fixup_final,	/* Final phase of device fixups */
	pci_fixup_enable,	/* pci_enable_device() time */
};

/* Anonymous variables would be nice... */
#define DECLARE_PCI_FIXUP_SECTION(section, name, vendor, device, hook)	\
	static const struct pci_fixup __pci_fixup_##name __attribute_used__ \
	__attribute__((__section__(#section))) = { vendor, device, hook };
#define DECLARE_PCI_FIXUP_EARLY(vendor, device, hook)			\
	DECLARE_PCI_FIXUP_SECTION(.pci_fixup_early,			\
			vendor##device##hook, vendor, device, hook)
#define DECLARE_PCI_FIXUP_HEADER(vendor, device, hook)			\
	DECLARE_PCI_FIXUP_SECTION(.pci_fixup_header,			\
			vendor##device##hook, vendor, device, hook)
#define DECLARE_PCI_FIXUP_FINAL(vendor, device, hook)			\
	DECLARE_PCI_FIXUP_SECTION(.pci_fixup_final,			\
			vendor##device##hook, vendor, device, hook)
#define DECLARE_PCI_FIXUP_ENABLE(vendor, device, hook)			\
	DECLARE_PCI_FIXUP_SECTION(.pci_fixup_enable,			\
			vendor##device##hook, vendor, device, hook)


void pci_fixup_device(enum pci_fixup_pass pass, struct pci_dev *dev);

extern int pci_pci_problems;
#define PCIPCI_FAIL		1
#define PCIPCI_TRITON		2
#define PCIPCI_NATOMA		4
#define PCIPCI_VIAETBF		8
#define PCIPCI_VSFX		16
#define PCIPCI_ALIMAGIK		32

#endif /* __KERNEL__ */
#endif /* LINUX_PCI_H */
