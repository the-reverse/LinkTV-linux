/*******************************************************************************

  Intel PRO/1000 Linux driver
  Copyright(c) 1999 - 2009 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Linux NICS <linux.nics@intel.com>
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

#ifndef _E1000_80003ES2LAN_H_
#define _E1000_80003ES2LAN_H_

#define E1000_KMRNCTRLSTA_OFFSET_FIFO_CTRL       0x00
#define E1000_KMRNCTRLSTA_OFFSET_INB_CTRL        0x02
#define E1000_KMRNCTRLSTA_OFFSET_HD_CTRL         0x10
#define E1000_KMRNCTRLSTA_OFFSET_MAC2PHY_OPMODE  0x1F

#define E1000_KMRNCTRLSTA_FIFO_CTRL_RX_BYPASS    0x0008
#define E1000_KMRNCTRLSTA_FIFO_CTRL_TX_BYPASS    0x0800
#define E1000_KMRNCTRLSTA_INB_CTRL_DIS_PADDING   0x0010

#define E1000_KMRNCTRLSTA_HD_CTRL_10_100_DEFAULT 0x0004
#define E1000_KMRNCTRLSTA_HD_CTRL_1000_DEFAULT   0x0000
#define E1000_KMRNCTRLSTA_OPMODE_E_IDLE          0x2000

#define E1000_TCTL_EXT_GCEX_MASK 0x000FFC00 /* Gigabit Carry Extend Padding */
#define DEFAULT_TCTL_EXT_GCEX_80003ES2LAN        0x00010000

#define DEFAULT_TIPG_IPGT_1000_80003ES2LAN       0x8
#define DEFAULT_TIPG_IPGT_10_100_80003ES2LAN     0x9

/* GG82563 PHY Specific Status Register (Page 0, Register 16 */
#define GG82563_PSCR_POLARITY_REVERSAL_DISABLE  0x0002 /* 1=Reversal Disabled */
#define GG82563_PSCR_CROSSOVER_MODE_MASK        0x0060
#define GG82563_PSCR_CROSSOVER_MODE_MDI         0x0000 /* 00=Manual MDI */
#define GG82563_PSCR_CROSSOVER_MODE_MDIX        0x0020 /* 01=Manual MDIX */
#define GG82563_PSCR_CROSSOVER_MODE_AUTO        0x0060 /* 11=Auto crossover */

/* PHY Specific Control Register 2 (Page 0, Register 26) */
#define GG82563_PSCR2_REVERSE_AUTO_NEG          0x2000
                                               /* 1=Reverse Auto-Negotiation */

/* MAC Specific Control Register (Page 2, Register 21) */
/* Tx clock speed for Link Down and 1000BASE-T for the following speeds */
#define GG82563_MSCR_TX_CLK_MASK                0x0007
#define GG82563_MSCR_TX_CLK_10MBPS_2_5          0x0004
#define GG82563_MSCR_TX_CLK_100MBPS_25          0x0005
#define GG82563_MSCR_TX_CLK_1000MBPS_2_5        0x0006
#define GG82563_MSCR_TX_CLK_1000MBPS_25         0x0007

#define GG82563_MSCR_ASSERT_CRS_ON_TX           0x0010 /* 1=Assert */

/* DSP Distance Register (Page 5, Register 26) */
/*
 * 0 = <50M
 * 1 = 50-80M
 * 2 = 80-100M
 * 3 = 110-140M
 * 4 = >140M
 */
#define GG82563_DSPD_CABLE_LENGTH               0x0007

/* Kumeran Mode Control Register (Page 193, Register 16) */
#define GG82563_KMCR_PASS_FALSE_CARRIER         0x0800

/* Max number of times Kumeran read/write should be validated */
#define GG82563_MAX_KMRN_RETRY                  0x5

/* Power Management Control Register (Page 193, Register 20) */
#define GG82563_PMCR_ENABLE_ELECTRICAL_IDLE     0x0001
                                          /* 1=Enable SERDES Electrical Idle */

/* In-Band Control Register (Page 194, Register 18) */
#define GG82563_ICR_DIS_PADDING                 0x0010 /* Disable Padding */

#endif
