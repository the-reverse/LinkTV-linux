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

/*
 * 82562G 10/100 Network Connection
 * 82562G-2 10/100 Network Connection
 * 82562GT 10/100 Network Connection
 * 82562GT-2 10/100 Network Connection
 * 82562V 10/100 Network Connection
 * 82562V-2 10/100 Network Connection
 * 82566DC-2 Gigabit Network Connection
 * 82566DC Gigabit Network Connection
 * 82566DM-2 Gigabit Network Connection
 * 82566DM Gigabit Network Connection
 * 82566MC Gigabit Network Connection
 * 82566MM Gigabit Network Connection
 * 82567LM Gigabit Network Connection
 * 82567LF Gigabit Network Connection
 * 82567V Gigabit Network Connection
 * 82567LM-2 Gigabit Network Connection
 * 82567LF-2 Gigabit Network Connection
 * 82567V-2 Gigabit Network Connection
 * 82567LF-3 Gigabit Network Connection
 * 82567LM-3 Gigabit Network Connection
 * 82567LM-4 Gigabit Network Connection
 * 82577LM Gigabit Network Connection
 * 82577LC Gigabit Network Connection
 * 82578DM Gigabit Network Connection
 * 82578DC Gigabit Network Connection
 */

#include "e1000.h"

static s32  e1000_init_phy_params_ich8lan(struct e1000_hw *hw);
static s32 e1000_init_phy_params_pchlan(struct e1000_hw *hw);
static s32  e1000_init_nvm_params_ich8lan(struct e1000_hw *hw);
static s32  e1000_init_mac_params_ich8lan(struct e1000_hw *hw);
static s32  e1000_acquire_swflag_ich8lan(struct e1000_hw *hw);
static void e1000_release_swflag_ich8lan(struct e1000_hw *hw);
static bool e1000_check_mng_mode_ich8lan(struct e1000_hw *hw);
static s32  e1000_check_reset_block_ich8lan(struct e1000_hw *hw);
static s32  e1000_phy_hw_reset_ich8lan(struct e1000_hw *hw);
static s32  e1000_get_phy_info_ich8lan(struct e1000_hw *hw);
static s32  e1000_set_d0_lplu_state_ich8lan(struct e1000_hw *hw,
                                            bool active);
static s32  e1000_set_d3_lplu_state_ich8lan(struct e1000_hw *hw,
                                            bool active);
static s32  e1000_read_nvm_ich8lan(struct e1000_hw *hw, u16 offset,
                                   u16 words, u16 *data);
static s32  e1000_write_nvm_ich8lan(struct e1000_hw *hw, u16 offset,
                                    u16 words, u16 *data);
static s32  e1000_validate_nvm_checksum_ich8lan(struct e1000_hw *hw);
static s32  e1000_update_nvm_checksum_ich8lan(struct e1000_hw *hw);
static s32  e1000_valid_led_default_ich8lan(struct e1000_hw *hw,
                                            u16 *data);
static s32 e1000_id_led_init_pchlan(struct e1000_hw *hw);
static s32  e1000_get_bus_info_ich8lan(struct e1000_hw *hw);
static s32  e1000_reset_hw_ich8lan(struct e1000_hw *hw);
static s32  e1000_init_hw_ich8lan(struct e1000_hw *hw);
static s32  e1000_setup_link_ich8lan(struct e1000_hw *hw);
static s32  e1000_setup_copper_link_ich8lan(struct e1000_hw *hw);
static s32  e1000_get_link_up_info_ich8lan(struct e1000_hw *hw,
                                           u16 *speed, u16 *duplex);
static s32  e1000_cleanup_led_ich8lan(struct e1000_hw *hw);
static s32  e1000_led_on_ich8lan(struct e1000_hw *hw);
static s32  e1000_led_off_ich8lan(struct e1000_hw *hw);
static s32  e1000_setup_led_pchlan(struct e1000_hw *hw);
static s32  e1000_cleanup_led_pchlan(struct e1000_hw *hw);
static s32  e1000_led_on_pchlan(struct e1000_hw *hw);
static s32  e1000_led_off_pchlan(struct e1000_hw *hw);
static void e1000_clear_hw_cntrs_ich8lan(struct e1000_hw *hw);
static s32  e1000_erase_flash_bank_ich8lan(struct e1000_hw *hw, u32 bank);
static s32  e1000_flash_cycle_ich8lan(struct e1000_hw *hw, u32 timeout);
static s32  e1000_flash_cycle_init_ich8lan(struct e1000_hw *hw);
static s32  e1000_get_phy_info_ife_ich8lan(struct e1000_hw *hw);
static void e1000_initialize_hw_bits_ich8lan(struct e1000_hw *hw);
static s32  e1000_kmrn_lock_loss_workaround_ich8lan(struct e1000_hw *hw);
static s32  e1000_read_flash_byte_ich8lan(struct e1000_hw *hw,
                                          u32 offset, u8 *data);
static s32  e1000_read_flash_data_ich8lan(struct e1000_hw *hw, u32 offset,
                                          u8 size, u16 *data);
static s32  e1000_read_flash_word_ich8lan(struct e1000_hw *hw,
                                          u32 offset, u16 *data);
static s32  e1000_retry_write_flash_byte_ich8lan(struct e1000_hw *hw,
                                                 u32 offset, u8 byte);
static s32  e1000_write_flash_byte_ich8lan(struct e1000_hw *hw,
                                           u32 offset, u8 data);
static s32  e1000_write_flash_data_ich8lan(struct e1000_hw *hw, u32 offset,
                                           u8 size, u16 data);
static s32 e1000_get_cfg_done_ich8lan(struct e1000_hw *hw);
static void e1000_power_down_phy_copper_ich8lan(struct e1000_hw *hw);
static s32 e1000_check_for_copper_link_ich8lan(struct e1000_hw *hw);

/* ICH GbE Flash Hardware Sequencing Flash Status Register bit breakdown */
/* Offset 04h HSFSTS */
union ich8_hws_flash_status {
	struct ich8_hsfsts {
		u16 flcdone    :1; /* bit 0 Flash Cycle Done */
		u16 flcerr     :1; /* bit 1 Flash Cycle Error */
		u16 dael       :1; /* bit 2 Direct Access error Log */
		u16 berasesz   :2; /* bit 4:3 Sector Erase Size */
		u16 flcinprog  :1; /* bit 5 flash cycle in Progress */
		u16 reserved1  :2; /* bit 13:6 Reserved */
		u16 reserved2  :6; /* bit 13:6 Reserved */
		u16 fldesvalid :1; /* bit 14 Flash Descriptor Valid */
		u16 flockdn    :1; /* bit 15 Flash Config Lock-Down */
	} hsf_status;
	u16 regval;
};

/* ICH GbE Flash Hardware Sequencing Flash control Register bit breakdown */
/* Offset 06h FLCTL */
union ich8_hws_flash_ctrl {
	struct ich8_hsflctl {
		u16 flcgo      :1;   /* 0 Flash Cycle Go */
		u16 flcycle    :2;   /* 2:1 Flash Cycle */
		u16 reserved   :5;   /* 7:3 Reserved  */
		u16 fldbcount  :2;   /* 9:8 Flash Data Byte Count */
		u16 flockdn    :6;   /* 15:10 Reserved */
	} hsf_ctrl;
	u16 regval;
};

/* ICH Flash Region Access Permissions */
union ich8_hws_flash_regacc {
	struct ich8_flracc {
		u32 grra      :8; /* 0:7 GbE region Read Access */
		u32 grwa      :8; /* 8:15 GbE region Write Access */
		u32 gmrag     :8; /* 23:16 GbE Master Read Access Grant */
		u32 gmwag     :8; /* 31:24 GbE Master Write Access Grant */
	} hsf_flregacc;
	u16 regval;
};

/**
 *  e1000_init_phy_params_pchlan - Initialize PHY function pointers
 *  @hw: pointer to the HW structure
 *
 *  Initialize family-specific PHY parameters and function pointers.
 **/
static s32 e1000_init_phy_params_pchlan(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	s32 ret_val = E1000_SUCCESS;

	phy->addr                     = 1;
	phy->reset_delay_us           = 100;

	phy->ops.acquire              = e1000_acquire_swflag_ich8lan;
	phy->ops.check_polarity       = e1000_check_polarity_ife;
	phy->ops.check_reset_block    = e1000_check_reset_block_ich8lan;
	phy->ops.force_speed_duplex   = e1000_phy_force_speed_duplex_ife;
	phy->ops.get_cable_length     = e1000e_get_cable_length_igp_2;
	phy->ops.get_cfg_done         = e1000_get_cfg_done_ich8lan;
	phy->ops.get_info             = e1000_get_phy_info_ich8lan;
	phy->ops.read_reg             = e1000_read_phy_reg_hv;
	phy->ops.release              = e1000_release_swflag_ich8lan;
	phy->ops.reset                = e1000_phy_hw_reset_ich8lan;
	phy->ops.set_d0_lplu_state    = e1000_set_d0_lplu_state_ich8lan;
	phy->ops.set_d3_lplu_state    = e1000_set_d3_lplu_state_ich8lan;
	phy->ops.write_reg            = e1000_write_phy_reg_hv;
	phy->ops.power_up             = e1000_power_up_phy_copper;
	phy->ops.power_down           = e1000_power_down_phy_copper_ich8lan;
	phy->autoneg_mask             = AUTONEG_ADVERTISE_SPEED_DEFAULT;

	phy->id = e1000_phy_unknown;
	e1000e_get_phy_id(hw);
	phy->type = e1000e_get_phy_type_from_id(phy->id);

	if (phy->type == e1000_phy_82577) {
		phy->ops.check_polarity = e1000_check_polarity_82577;
		phy->ops.force_speed_duplex =
			e1000_phy_force_speed_duplex_82577;
		phy->ops.get_cable_length   = e1000_get_cable_length_82577;
		phy->ops.get_info = e1000_get_phy_info_82577;
		phy->ops.commit = e1000e_phy_sw_reset;
	}

	return ret_val;
}

/**
 *  e1000_init_phy_params_ich8lan - Initialize PHY function pointers
 *  @hw: pointer to the HW structure
 *
 *  Initialize family-specific PHY parameters and function pointers.
 **/
static s32 e1000_init_phy_params_ich8lan(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	s32 ret_val = E1000_SUCCESS;
	u16 i = 0;

	phy->addr                     = 1;
	phy->reset_delay_us           = 100;

	phy->ops.acquire              = e1000_acquire_swflag_ich8lan;
	phy->ops.check_polarity       = e1000_check_polarity_ife;
	phy->ops.check_reset_block    = e1000_check_reset_block_ich8lan;
	phy->ops.force_speed_duplex   = e1000_phy_force_speed_duplex_ife;
	phy->ops.get_cable_length     = e1000e_get_cable_length_igp_2;
	phy->ops.get_cfg_done         = e1000_get_cfg_done_ich8lan;
	phy->ops.get_info             = e1000_get_phy_info_ich8lan;
	phy->ops.read_reg             = e1000e_read_phy_reg_igp;
	phy->ops.release              = e1000_release_swflag_ich8lan;
	phy->ops.reset                = e1000_phy_hw_reset_ich8lan;
	phy->ops.set_d0_lplu_state    = e1000_set_d0_lplu_state_ich8lan;
	phy->ops.set_d3_lplu_state    = e1000_set_d3_lplu_state_ich8lan;
	phy->ops.write_reg            = e1000e_write_phy_reg_igp;
	phy->ops.power_up             = e1000_power_up_phy_copper;
	phy->ops.power_down           = e1000_power_down_phy_copper_ich8lan;

	/*
	 * We may need to do this twice - once for IGP and if that fails,
	 * we'll set BM func pointers and try again
	 */
	ret_val = e1000e_determine_phy_address(hw);
	if (ret_val) {
		phy->ops.write_reg = e1000e_write_phy_reg_bm;
		phy->ops.read_reg  = e1000e_read_phy_reg_bm;
		ret_val = e1000e_determine_phy_address(hw);
		if (ret_val) {
			e_dbg("Cannot determine PHY addr. Erroring out\n");
			goto out;
		}
	}

	phy->id = 0;
	while ((e1000_phy_unknown == e1000e_get_phy_type_from_id(phy->id)) &&
	       (i++ < 100)) {
		msleep(1);
		ret_val = e1000e_get_phy_id(hw);
		if (ret_val)
			goto out;
	}

	/* Verify phy id */
	switch (phy->id) {
	case IGP03E1000_E_PHY_ID:
		phy->type = e1000_phy_igp_3;
		phy->autoneg_mask = AUTONEG_ADVERTISE_SPEED_DEFAULT;
		break;
	case IFE_E_PHY_ID:
	case IFE_PLUS_E_PHY_ID:
	case IFE_C_E_PHY_ID:
		phy->type = e1000_phy_ife;
		phy->autoneg_mask = E1000_ALL_NOT_GIG;
		break;
	case BME1000_E_PHY_ID:
		phy->type = e1000_phy_bm;
		phy->autoneg_mask = AUTONEG_ADVERTISE_SPEED_DEFAULT;
		phy->ops.read_reg = e1000e_read_phy_reg_bm;
		phy->ops.write_reg = e1000e_write_phy_reg_bm;
		phy->ops.commit = e1000e_phy_sw_reset;
		break;
	default:
		ret_val = -E1000_ERR_PHY;
		goto out;
	}

out:
	return ret_val;
}

/**
 *  e1000_init_nvm_params_ich8lan - Initialize NVM function pointers
 *  @hw: pointer to the HW structure
 *
 *  Initialize family-specific NVM parameters and function
 *  pointers.
 **/
static s32 e1000_init_nvm_params_ich8lan(struct e1000_hw *hw)
{
	struct e1000_nvm_info *nvm = &hw->nvm;
	struct e1000_dev_spec_ich8lan *dev_spec = &hw->dev_spec.ich8lan;
	union ich8_hws_flash_status hsfsts;
	u32 gfpreg, sector_base_addr, sector_end_addr;
	s32 ret_val = E1000_SUCCESS;
	u16 i;

	/* Can't read flash registers if the register set isn't mapped. */
	if (!hw->flash_address) {
		e_dbg("ERROR: Flash registers not mapped\n");
		ret_val = -E1000_ERR_CONFIG;
		goto out;
	}

	nvm->type = e1000_nvm_flash_sw;

	gfpreg = er32flash(ICH_FLASH_GFPREG);

	/*
	 * sector_X_addr is a "sector"-aligned address (4096 bytes)
	 * Add 1 to sector_end_addr since this sector is included in
	 * the overall size.
	 */
	sector_base_addr = gfpreg & FLASH_GFPREG_BASE_MASK;
	sector_end_addr = ((gfpreg >> 16) & FLASH_GFPREG_BASE_MASK) + 1;

	/* flash_base_addr is byte-aligned */
	nvm->flash_base_addr = sector_base_addr << FLASH_SECTOR_ADDR_SHIFT;

	/*
	 * find total size of the NVM, then cut in half since the total
	 * size represents two separate NVM banks.
	 */
	nvm->flash_bank_size = (sector_end_addr - sector_base_addr)
	                          << FLASH_SECTOR_ADDR_SHIFT;
	nvm->flash_bank_size /= 2;
	/* Adjust to word count */
	nvm->flash_bank_size /= sizeof(u16);

	/*
	 * Make sure the flash bank size does not overwrite the 4k
	 * sector ranges. We may have 64k allotted to us but we only care
	 * about the first 2 4k sectors. Therefore, if we have anything less
	 * than 64k set in the HSFSTS register, we will reduce the bank size
	 * down to 4k and let the rest remain unused. If berasesz == 3, then
	 * we are working in 64k mode. Otherwise we are not.
	 */
	if (nvm->flash_bank_size > E1000_ICH8_SHADOW_RAM_WORDS) {
		hsfsts.regval = er16flash(ICH_FLASH_HSFSTS);
		if (hsfsts.hsf_status.berasesz != 3)
			nvm->flash_bank_size = E1000_ICH8_SHADOW_RAM_WORDS;
	}

	nvm->word_size = E1000_ICH8_SHADOW_RAM_WORDS;

	/* Clear shadow ram */
	for (i = 0; i < nvm->word_size; i++) {
		dev_spec->shadow_ram[i].modified = false;
		dev_spec->shadow_ram[i].value    = 0xFFFF;
	}

	/* Function Pointers */
	nvm->ops.acquire       = e1000_acquire_swflag_ich8lan;
	nvm->ops.read          = e1000_read_nvm_ich8lan;
	nvm->ops.release       = e1000_release_swflag_ich8lan;
	nvm->ops.update        = e1000_update_nvm_checksum_ich8lan;
	nvm->ops.valid_led_default = e1000_valid_led_default_ich8lan;
	nvm->ops.validate      = e1000_validate_nvm_checksum_ich8lan;
	nvm->ops.write         = e1000_write_nvm_ich8lan;

out:
	return ret_val;
}

/**
 *  e1000_init_mac_params_ich8lan - Initialize MAC function pointers
 *  @hw: pointer to the HW structure
 *
 *  Initialize family-specific MAC parameters and function
 *  pointers.
 **/
static s32 e1000_init_mac_params_ich8lan(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;

	/* Set media type function pointer */
	hw->phy.media_type = e1000_media_type_copper;

	/* Set mta register count */
	mac->mta_reg_count = 32;
	/* Set rar entry count */
	mac->rar_entry_count = E1000_ICH_RAR_ENTRIES;
	if (mac->type == e1000_ich8lan)
		mac->rar_entry_count--;
	/* Set if part includes ASF firmware */
	mac->asf_firmware_present = true;
	/* Set if manageability features are enabled. */
	mac->arc_subsystem_valid = true;

	/* Function pointers */

	/* bus type/speed/width */
	mac->ops.get_bus_info = e1000_get_bus_info_ich8lan;
	/* function id */
	mac->ops.set_lan_id = e1000_set_lan_id_single_port;
	/* reset */
	mac->ops.reset_hw = e1000_reset_hw_ich8lan;
	/* hw initialization */
	mac->ops.init_hw = e1000_init_hw_ich8lan;
	/* link setup */
	mac->ops.setup_link = e1000_setup_link_ich8lan;
	/* physical interface setup */
	mac->ops.setup_physical_interface = e1000_setup_copper_link_ich8lan;
	/* check for link */
	mac->ops.check_for_link = e1000_check_for_copper_link_ich8lan;
	/* check management mode */
	mac->ops.check_mng_mode = e1000_check_mng_mode_ich8lan;
	/* link info */
	mac->ops.get_link_up_info = e1000_get_link_up_info_ich8lan;
	/* multicast address update */
	mac->ops.update_mc_addr_list = e1000e_update_mc_addr_list_generic;
	/* setting MTA */
	mac->ops.mta_set = e1000_mta_set_generic;
	/* clear hardware counters */
	mac->ops.clear_hw_cntrs = e1000_clear_hw_cntrs_ich8lan;

	/* LED operations */
	switch (mac->type) {
	case e1000_ich8lan:
	case e1000_ich9lan:
	case e1000_ich10lan:
		/* ID LED init */
		mac->ops.id_led_init = e1000e_id_led_init;
		/* blink LED */
		mac->ops.blink_led = e1000e_blink_led;
		/* setup LED */
		mac->ops.setup_led = e1000_setup_led_generic;
		/* cleanup LED */
		mac->ops.cleanup_led = e1000_cleanup_led_ich8lan;
		/* turn on/off LED */
		mac->ops.led_on = e1000_led_on_ich8lan;
		mac->ops.led_off = e1000_led_off_ich8lan;
		break;
	case e1000_pchlan:
		/* ID LED init */
		mac->ops.id_led_init = e1000_id_led_init_pchlan;
		/* setup LED */
		mac->ops.setup_led = e1000_setup_led_pchlan;
		/* cleanup LED */
		mac->ops.cleanup_led = e1000_cleanup_led_pchlan;
		/* turn on/off LED */
		mac->ops.led_on = e1000_led_on_pchlan;
		mac->ops.led_off = e1000_led_off_pchlan;
		break;
	default:
		break;
	}

	/* Enable PCS Lock-loss workaround for ICH8 */
	if (mac->type == e1000_ich8lan)
		e1000e_set_kmrn_lock_loss_workaround_ich8lan(hw, true);


	return E1000_SUCCESS;
}

/**
 *  e1000_check_for_copper_link_ich8lan - Check for link (Copper)
 *  @hw: pointer to the HW structure
 *
 *  Checks to see of the link status of the hardware has changed.  If a
 *  change in link status has been detected, then we read the PHY registers
 *  to get the current speed/duplex if link exists.
 **/
static s32 e1000_check_for_copper_link_ich8lan(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	s32 ret_val;
	bool link;

	/*
	 * We only want to go out to the PHY registers to see if Auto-Neg
	 * has completed and/or if our link status has changed.  The
	 * get_link_status flag is set upon receiving a Link Status
	 * Change or Rx Sequence Error interrupt.
	 */
	if (!mac->get_link_status) {
		ret_val = E1000_SUCCESS;
		goto out;
	}

	if (hw->mac.type == e1000_pchlan) {
		ret_val = e1000e_write_kmrn_reg(hw,
						   E1000_KMRNCTRLSTA_K1_CONFIG,
						   E1000_KMRNCTRLSTA_K1_ENABLE);
		if (ret_val)
			goto out;
	}

	/*
	 * First we want to see if the MII Status Register reports
	 * link.  If so, then we want to get the current speed/duplex
	 * of the PHY.
	 */
	ret_val = e1000e_phy_has_link_generic(hw, 1, 0, &link);
	if (ret_val)
		goto out;

	if (!link)
		goto out; /* No link detected */

	mac->get_link_status = false;

	if (hw->phy.type == e1000_phy_82578) {
		ret_val = e1000_link_stall_workaround_hv(hw);
		if (ret_val)
			goto out;
	}

	/*
	 * Check if there was DownShift, must be checked
	 * immediately after link-up
	 */
	e1000e_check_downshift(hw);

	/*
	 * If we are forcing speed/duplex, then we simply return since
	 * we have already determined whether we have link or not.
	 */
	if (!mac->autoneg) {
		ret_val = -E1000_ERR_CONFIG;
		goto out;
	}

	/*
	 * Auto-Neg is enabled.  Auto Speed Detection takes care
	 * of MAC speed/duplex configuration.  So we only need to
	 * configure Collision Distance in the MAC.
	 */
	e1000e_config_collision_dist(hw);

	/*
	 * Configure Flow Control now that Auto-Neg has completed.
	 * First, we need to restore the desired flow control
	 * settings because we may have had to re-autoneg with a
	 * different link partner.
	 */
	ret_val = e1000e_config_fc_after_link_up(hw);
	if (ret_val)
		e_dbg("Error configuring flow control\n");

out:
	return ret_val;
}

/**
 *  e1000_init_function_pointers_ich8lan - Initialize ICH8 function pointers
 *  @hw: pointer to the HW structure
 *
 *  Initialize family-specific function pointers for PHY, MAC, and NVM.
 **/
void e1000_init_function_pointers_ich8lan(struct e1000_hw *hw)
{
	e1000_init_mac_ops_generic(hw);
	e1000_init_nvm_ops_generic(hw);
	hw->mac.ops.init_params = e1000_init_mac_params_ich8lan;
	hw->nvm.ops.init_params = e1000_init_nvm_params_ich8lan;
	switch (hw->mac.type) {
	case e1000_ich8lan:
	case e1000_ich9lan:
	case e1000_ich10lan:
		hw->phy.ops.init_params = e1000_init_phy_params_ich8lan;
		break;
	case e1000_pchlan:
		hw->phy.ops.init_params = e1000_init_phy_params_pchlan;
		break;
	default:
		break;
	}
}

static DEFINE_MUTEX(nvm_mutex);

/**
 *  e1000_acquire_swflag_ich8lan - Acquire software control flag
 *  @hw: pointer to the HW structure
 *
 *  Acquires the software control flag for performing NVM and PHY
 *  operations.  This is a function pointer entry point only called by
 *  read/write routines for the PHY and NVM parts.
 **/
static s32 e1000_acquire_swflag_ich8lan(struct e1000_hw *hw)
{
	u32 extcnf_ctrl, timeout = PHY_CFG_TIMEOUT;
	s32 ret_val = E1000_SUCCESS;

	might_sleep();

	mutex_lock(&nvm_mutex);

	while (timeout) {
		extcnf_ctrl = er32(EXTCNF_CTRL);

		if (!(extcnf_ctrl & E1000_EXTCNF_CTRL_SWFLAG)) {
			extcnf_ctrl |= E1000_EXTCNF_CTRL_SWFLAG;
			ew32(EXTCNF_CTRL, extcnf_ctrl);

			extcnf_ctrl = er32(EXTCNF_CTRL);
			if (extcnf_ctrl & E1000_EXTCNF_CTRL_SWFLAG)
				break;
		}
		mdelay(1);
		timeout--;
	}

	if (!timeout) {
		e_dbg("SW/FW/HW has locked the resource for too long.\n");
		extcnf_ctrl &= ~E1000_EXTCNF_CTRL_SWFLAG;
		ew32(EXTCNF_CTRL, extcnf_ctrl);
		mutex_unlock(&nvm_mutex);
		ret_val = -E1000_ERR_CONFIG;
		goto out;
	}

out:
	return ret_val;
}

/**
 *  e1000_release_swflag_ich8lan - Release software control flag
 *  @hw: pointer to the HW structure
 *
 *  Releases the software control flag for performing NVM and PHY operations.
 *  This is a function pointer entry point only called by read/write
 *  routines for the PHY and NVM parts.
 **/
static void e1000_release_swflag_ich8lan(struct e1000_hw *hw)
{
	u32 extcnf_ctrl;

	extcnf_ctrl = er32(EXTCNF_CTRL);
	extcnf_ctrl &= ~E1000_EXTCNF_CTRL_SWFLAG;
	ew32(EXTCNF_CTRL, extcnf_ctrl);

	mutex_unlock(&nvm_mutex);
}

/**
 *  e1000_check_mng_mode_ich8lan - Checks management mode
 *  @hw: pointer to the HW structure
 *
 *  This checks if the adapter has manageability enabled.
 *  This is a function pointer entry point only called by read/write
 *  routines for the PHY and NVM parts.
 **/
static bool e1000_check_mng_mode_ich8lan(struct e1000_hw *hw)
{
	u32 fwsm;

	fwsm = er32(FWSM);
	return (fwsm & E1000_FWSM_MODE_MASK) ==
	        (E1000_ICH_MNG_IAMT_MODE << E1000_FWSM_MODE_SHIFT);
}
/**
 *  e1000_check_reset_block_ich8lan - Check if PHY reset is blocked
 *  @hw: pointer to the HW structure
 *
 *  Checks if firmware is blocking the reset of the PHY.
 *  This is a function pointer entry point only called by
 *  reset routines.
 **/
static s32 e1000_check_reset_block_ich8lan(struct e1000_hw *hw)
{
	u32 fwsm;

	fwsm = er32(FWSM);
	return (fwsm & E1000_ICH_FWSM_RSPCIPHY) ? E1000_SUCCESS
	                                        : E1000_BLK_PHY_RESET;
}

/**
 *  e1000_hv_phy_workarounds_ich8lan - A series of Phy workarounds to be
 *  done after every PHY reset.
 **/
static s32 e1000_hv_phy_workarounds_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;

	if (hw->mac.type != e1000_pchlan)
		return ret_val;

	if (((hw->phy.type == e1000_phy_82577) &&
	     ((hw->phy.revision == 1) || (hw->phy.revision == 2))) ||
	    ((hw->phy.type == e1000_phy_82578) && (hw->phy.revision == 1))) {
		/* Disable generation of early preamble */
		ret_val = e1e_wphy(hw, PHY_REG(769, 25), 0x4431);
		if (ret_val)
			return ret_val;

		/* Preamble tuning for SSC */
		ret_val = e1e_wphy(hw, PHY_REG(770, 16), 0xA204);
		if (ret_val)
			return ret_val;
	}

	if (hw->phy.type == e1000_phy_82578) {
		/*
		 * Return registers to default by doing a soft reset then
		 * writing 0x3140 to the control register.
		 */
		if (hw->phy.revision < 2) {
			e1000e_phy_sw_reset(hw);
			ret_val = e1e_wphy(hw, PHY_CONTROL,
			                                0x3140);
		}
	}

	/* Select page 0 */
	ret_val = hw->phy.ops.acquire(hw);
	if (ret_val)
		return ret_val;
	hw->phy.addr = 1;
	e1000e_write_phy_reg_mdic(hw, IGP01E1000_PHY_PAGE_SELECT, 0);
	hw->phy.ops.release(hw);

	return ret_val;
}

/**
 *  e1000_lan_init_done_ich8lan - Check for PHY config completion
 *  @hw: pointer to the HW structure
 *
 *  Check the appropriate indication the MAC has finished configuring the
 *  PHY after a software reset.
 **/
static void e1000_lan_init_done_ich8lan(struct e1000_hw *hw)
{
	u32 data, loop = E1000_ICH8_LAN_INIT_TIMEOUT;

	/* Wait for basic configuration completes before proceeding */
	do {
		data = er32(STATUS);
		data &= E1000_STATUS_LAN_INIT_DONE;
		udelay(100);
	} while ((!data) && --loop);

	/*
	 * If basic configuration is incomplete before the above loop
	 * count reaches 0, loading the configuration from NVM will
	 * leave the PHY in a bad state possibly resulting in no link.
	 */
	if (loop == 0)
		e_dbg("LAN_INIT_DONE not set, increase timeout\n");

	/* Clear the Init Done bit for the next init event */
	data = er32(STATUS);
	data &= ~E1000_STATUS_LAN_INIT_DONE;
	ew32(STATUS, data);
}

/**
 *  e1000_phy_hw_reset_ich8lan - Performs a PHY reset
 *  @hw: pointer to the HW structure
 *
 *  Resets the PHY
 *  This is a function pointer entry point called by drivers
 *  or other shared routines.
 **/
static s32 e1000_phy_hw_reset_ich8lan(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	u32 i, data, cnf_size, cnf_base_addr, sw_cfg_mask;
	s32 ret_val;
	u16 word_addr, reg_data, reg_addr, phy_page = 0;

	ret_val = e1000e_phy_hw_reset_generic(hw);
	if (ret_val)
		goto out;

	/* Allow time for h/w to get to a quiescent state after reset */
	msleep(10);

	if (hw->mac.type == e1000_pchlan) {
		ret_val = e1000_hv_phy_workarounds_ich8lan(hw);
		if (ret_val)
			goto out;
	}

	/*
	 * Initialize the PHY from the NVM on ICH platforms.  This
	 * is needed due to an issue where the NVM configuration is
	 * not properly autoloaded after power transitions.
	 * Therefore, after each PHY reset, we will load the
	 * configuration data out of the NVM manually.
	 */
	if (hw->mac.type == e1000_ich8lan && phy->type == e1000_phy_igp_3) {
		/* Check if SW needs configure the PHY */
		if ((hw->device_id == E1000_DEV_ID_ICH8_IGP_M_AMT) ||
		    (hw->device_id == E1000_DEV_ID_ICH8_IGP_M))
			sw_cfg_mask = E1000_FEXTNVM_SW_CONFIG_ICH8M;
		else
			sw_cfg_mask = E1000_FEXTNVM_SW_CONFIG;

		data = er32(FEXTNVM);
		if (!(data & sw_cfg_mask))
			goto out;

		/* Wait for basic configuration completes before proceeding */
		e1000_lan_init_done_ich8lan(hw);

		/*
		 * Make sure HW does not configure LCD from PHY
		 * extended configuration before SW configuration
		 */
		data = er32(EXTCNF_CTRL);
		if (data & E1000_EXTCNF_CTRL_LCD_WRITE_ENABLE)
			goto out;

		cnf_size = er32(EXTCNF_SIZE);
		cnf_size &= E1000_EXTCNF_SIZE_EXT_PCIE_LENGTH_MASK;
		cnf_size >>= E1000_EXTCNF_SIZE_EXT_PCIE_LENGTH_SHIFT;
		if (!cnf_size)
			goto out;

		cnf_base_addr = data & E1000_EXTCNF_CTRL_EXT_CNF_POINTER_MASK;
		cnf_base_addr >>= E1000_EXTCNF_CTRL_EXT_CNF_POINTER_SHIFT;

		/* Configure LCD from extended configuration region. */

		/* cnf_base_addr is in DWORD */
		word_addr = (u16)(cnf_base_addr << 1);

		for (i = 0; i < cnf_size; i++) {
			ret_val = e1000_read_nvm(hw, (word_addr + i * 2), 1,
			                           &reg_data);
			if (ret_val)
				goto out;

			ret_val = e1000_read_nvm(hw, (word_addr + i * 2 + 1),
			                           1, &reg_addr);
			if (ret_val)
				goto out;

			/* Save off the PHY page for future writes. */
			if (reg_addr == IGP01E1000_PHY_PAGE_SELECT) {
				phy_page = reg_data;
				continue;
			}

			reg_addr |= phy_page;

			ret_val = e1e_wphy(hw, (u32)reg_addr, reg_data);
			if (ret_val)
				goto out;
		}
	}

out:
	return ret_val;
}

/**
 *  e1000_get_phy_info_ich8lan - Calls appropriate PHY type get_phy_info
 *  @hw: pointer to the HW structure
 *
 *  Wrapper for calling the get_phy_info routines for the appropriate phy type.
 **/
static s32 e1000_get_phy_info_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = -E1000_ERR_PHY_TYPE;

	switch (hw->phy.type) {
	case e1000_phy_ife:
		ret_val = e1000_get_phy_info_ife_ich8lan(hw);
		break;
	case e1000_phy_igp_3:
	case e1000_phy_bm:
	case e1000_phy_82578:
	case e1000_phy_82577:
		ret_val = e1000e_get_phy_info_igp(hw);
		break;
	default:
		break;
	}

	return ret_val;
}

/**
 *  e1000_get_phy_info_ife_ich8lan - Retrieves various IFE PHY states
 *  @hw: pointer to the HW structure
 *
 *  Populates "phy" structure with various feature states.
 *  This function is only called by other family-specific
 *  routines.
 **/
static s32 e1000_get_phy_info_ife_ich8lan(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	s32 ret_val;
	u16 data;
	bool link;

	ret_val = e1000e_phy_has_link_generic(hw, 1, 0, &link);
	if (ret_val)
		goto out;

	if (!link) {
		e_dbg("Phy info is only valid if link is up\n");
		ret_val = -E1000_ERR_CONFIG;
		goto out;
	}

	ret_val = e1e_rphy(hw, IFE_PHY_SPECIAL_CONTROL, &data);
	if (ret_val)
		goto out;
	phy->polarity_correction = (data & IFE_PSC_AUTO_POLARITY_DISABLE)
	                           ? false : true;

	if (phy->polarity_correction) {
		ret_val = e1000_check_polarity_ife(hw);
		if (ret_val)
			goto out;
	} else {
		/* Polarity is forced */
		phy->cable_polarity = (data & IFE_PSC_FORCE_POLARITY)
		                      ? e1000_rev_polarity_reversed
		                      : e1000_rev_polarity_normal;
	}

	ret_val = e1e_rphy(hw, IFE_PHY_MDIX_CONTROL, &data);
	if (ret_val)
		goto out;

	phy->is_mdix = (data & IFE_PMC_MDIX_STATUS) ? true : false;

	/* The following parameters are undefined for 10/100 operation. */
	phy->cable_length = E1000_CABLE_LENGTH_UNDEFINED;
	phy->local_rx = e1000_1000t_rx_status_undefined;
	phy->remote_rx = e1000_1000t_rx_status_undefined;

out:
	return ret_val;
}

/**
 *  e1000_set_d0_lplu_state_ich8lan - Set Low Power Linkup D0 state
 *  @hw: pointer to the HW structure
 *  @active: true to enable LPLU, false to disable
 *
 *  Sets the LPLU D0 state according to the active flag.  When
 *  activating LPLU this function also disables smart speed
 *  and vice versa.  LPLU will not be activated unless the
 *  device autonegotiation advertisement meets standards of
 *  either 10 or 10/100 or 10/100/1000 at all duplexes.
 *  This is a function pointer entry point only called by
 *  PHY setup routines.
 **/
static s32 e1000_set_d0_lplu_state_ich8lan(struct e1000_hw *hw, bool active)
{
	struct e1000_phy_info *phy = &hw->phy;
	u32 phy_ctrl;
	s32 ret_val = E1000_SUCCESS;
	u16 data;

	if (phy->type == e1000_phy_ife)
		goto out;

	phy_ctrl = er32(PHY_CTRL);

	if (active) {
		phy_ctrl |= E1000_PHY_CTRL_D0A_LPLU;
		ew32(PHY_CTRL, phy_ctrl);

		if (phy->type != e1000_phy_igp_3)
			goto out;

		/*
		 * Call gig speed drop workaround on LPLU before accessing
		 * any PHY registers
		 */
		if (hw->mac.type == e1000_ich8lan)
			e1000e_gig_downshift_workaround_ich8lan(hw);

		/* When LPLU is enabled, we should disable SmartSpeed */
		ret_val = e1e_rphy(hw,
		                            IGP01E1000_PHY_PORT_CONFIG,
		                            &data);
		data &= ~IGP01E1000_PSCFR_SMART_SPEED;
		ret_val = e1e_wphy(hw,
		                             IGP01E1000_PHY_PORT_CONFIG,
		                             data);
		if (ret_val)
			goto out;
	} else {
		phy_ctrl &= ~E1000_PHY_CTRL_D0A_LPLU;
		ew32(PHY_CTRL, phy_ctrl);

		if (phy->type != e1000_phy_igp_3)
			goto out;

		/*
		 * LPLU and SmartSpeed are mutually exclusive.  LPLU is used
		 * during Dx states where the power conservation is most
		 * important.  During driver activity we should enable
		 * SmartSpeed, so performance is maintained.
		 */
		if (phy->smart_speed == e1000_smart_speed_on) {
			ret_val = e1e_rphy(hw,
			                            IGP01E1000_PHY_PORT_CONFIG,
			                            &data);
			if (ret_val)
				goto out;

			data |= IGP01E1000_PSCFR_SMART_SPEED;
			ret_val = e1e_wphy(hw,
			                             IGP01E1000_PHY_PORT_CONFIG,
			                             data);
			if (ret_val)
				goto out;
		} else if (phy->smart_speed == e1000_smart_speed_off) {
			ret_val = e1e_rphy(hw,
			                            IGP01E1000_PHY_PORT_CONFIG,
			                            &data);
			if (ret_val)
				goto out;

			data &= ~IGP01E1000_PSCFR_SMART_SPEED;
			ret_val = e1e_wphy(hw,
			                             IGP01E1000_PHY_PORT_CONFIG,
			                             data);
			if (ret_val)
				goto out;
		}
	}

out:
	return ret_val;
}

/**
 *  e1000_set_d3_lplu_state_ich8lan - Set Low Power Linkup D3 state
 *  @hw: pointer to the HW structure
 *  @active: true to enable LPLU, false to disable
 *
 *  Sets the LPLU D3 state according to the active flag.  When
 *  activating LPLU this function also disables smart speed
 *  and vice versa.  LPLU will not be activated unless the
 *  device autonegotiation advertisement meets standards of
 *  either 10 or 10/100 or 10/100/1000 at all duplexes.
 *  This is a function pointer entry point only called by
 *  PHY setup routines.
 **/
static s32 e1000_set_d3_lplu_state_ich8lan(struct e1000_hw *hw, bool active)
{
	struct e1000_phy_info *phy = &hw->phy;
	u32 phy_ctrl;
	s32 ret_val = E1000_SUCCESS;
	u16 data;

	phy_ctrl = er32(PHY_CTRL);

	if (!active) {
		phy_ctrl &= ~E1000_PHY_CTRL_NOND0A_LPLU;
		ew32(PHY_CTRL, phy_ctrl);

		if (phy->type != e1000_phy_igp_3)
			goto out;

		/*
		 * LPLU and SmartSpeed are mutually exclusive.  LPLU is used
		 * during Dx states where the power conservation is most
		 * important.  During driver activity we should enable
		 * SmartSpeed, so performance is maintained.
		 */
		if (phy->smart_speed == e1000_smart_speed_on) {
			ret_val = e1e_rphy(hw,
			                            IGP01E1000_PHY_PORT_CONFIG,
			                            &data);
			if (ret_val)
				goto out;

			data |= IGP01E1000_PSCFR_SMART_SPEED;
			ret_val = e1e_wphy(hw,
			                             IGP01E1000_PHY_PORT_CONFIG,
			                             data);
			if (ret_val)
				goto out;
		} else if (phy->smart_speed == e1000_smart_speed_off) {
			ret_val = e1e_rphy(hw,
			                            IGP01E1000_PHY_PORT_CONFIG,
			                            &data);
			if (ret_val)
				goto out;

			data &= ~IGP01E1000_PSCFR_SMART_SPEED;
			ret_val = e1e_wphy(hw,
			                             IGP01E1000_PHY_PORT_CONFIG,
			                             data);
			if (ret_val)
				goto out;
		}
	} else if ((phy->autoneg_advertised == E1000_ALL_SPEED_DUPLEX) ||
	           (phy->autoneg_advertised == E1000_ALL_NOT_GIG) ||
	           (phy->autoneg_advertised == E1000_ALL_10_SPEED)) {
		phy_ctrl |= E1000_PHY_CTRL_NOND0A_LPLU;
		ew32(PHY_CTRL, phy_ctrl);

		if (phy->type != e1000_phy_igp_3)
			goto out;

		/*
		 * Call gig speed drop workaround on LPLU before accessing
		 * any PHY registers
		 */
		if (hw->mac.type == e1000_ich8lan)
			e1000e_gig_downshift_workaround_ich8lan(hw);

		/* When LPLU is enabled, we should disable SmartSpeed */
		ret_val = e1e_rphy(hw,
		                            IGP01E1000_PHY_PORT_CONFIG,
		                            &data);
		if (ret_val)
			goto out;

		data &= ~IGP01E1000_PSCFR_SMART_SPEED;
		ret_val = e1e_wphy(hw,
		                             IGP01E1000_PHY_PORT_CONFIG,
		                             data);
	}

out:
	return ret_val;
}

/**
 *  e1000_valid_nvm_bank_detect_ich8lan - finds out the valid bank 0 or 1
 *  @hw: pointer to the HW structure
 *  @bank:  pointer to the variable that returns the active bank
 *
 *  Reads signature byte from the NVM using the flash access registers.
 *  Word 0x13 bits 15:14 = 10b indicate a valid signature for that bank.
 **/
static s32 e1000_valid_nvm_bank_detect_ich8lan(struct e1000_hw *hw, u32 *bank)
{
	u32 eecd;
	struct e1000_nvm_info *nvm = &hw->nvm;
	u32 bank1_offset = nvm->flash_bank_size * sizeof(u16);
	u32 act_offset = E1000_ICH_NVM_SIG_WORD * 2 + 1;
	u8 sig_byte = 0;
	s32 ret_val = E1000_SUCCESS;

	switch (hw->mac.type) {
	case e1000_ich8lan:
	case e1000_ich9lan:
		eecd = er32(EECD);
		if ((eecd & E1000_EECD_SEC1VAL_VALID_MASK) ==
		    E1000_EECD_SEC1VAL_VALID_MASK) {
			if (eecd & E1000_EECD_SEC1VAL)
				*bank = 1;
			else
				*bank = 0;

			goto out;
		}
		e_dbg("Unable to determine valid NVM bank via EEC - "
		         "reading flash signature\n");
		/* fall-thru */
	default:
		/* set bank to 0 in case flash read fails */
		*bank = 0;

		/* Check bank 0 */
		ret_val = e1000_read_flash_byte_ich8lan(hw, act_offset,
		                                        &sig_byte);
		if (ret_val)
			goto out;
		if ((sig_byte & E1000_ICH_NVM_VALID_SIG_MASK) ==
			E1000_ICH_NVM_SIG_VALUE) {
			*bank = 0;
			goto out;
		}

		/* Check bank 1 */
		ret_val = e1000_read_flash_byte_ich8lan(hw, act_offset +
		                                        bank1_offset,
					                &sig_byte);
		if (ret_val)
			goto out;
		if ((sig_byte & E1000_ICH_NVM_VALID_SIG_MASK) ==
			E1000_ICH_NVM_SIG_VALUE) {
			*bank = 1;
			goto out;
		}

		e_dbg("ERROR: No valid NVM bank present\n");
		ret_val = -E1000_ERR_NVM;
		break;
	}
out:
	return ret_val;
}

/**
 *  e1000_read_nvm_ich8lan - Read word(s) from the NVM
 *  @hw: pointer to the HW structure
 *  @offset: The offset (in bytes) of the word(s) to read.
 *  @words: Size of data to read in words
 *  @data: Pointer to the word(s) to read at offset.
 *
 *  Reads a word(s) from the NVM using the flash access registers.
 **/
static s32 e1000_read_nvm_ich8lan(struct e1000_hw *hw, u16 offset, u16 words,
                                  u16 *data)
{
	struct e1000_nvm_info *nvm = &hw->nvm;
	struct e1000_dev_spec_ich8lan *dev_spec = &hw->dev_spec.ich8lan;
	u32 act_offset;
	s32 ret_val = E1000_SUCCESS;
	u32 bank = 0;
	u16 i, word;

	if ((offset >= nvm->word_size) || (words > nvm->word_size - offset) ||
	    (words == 0)) {
		e_dbg("nvm parameter(s) out of bounds\n");
		ret_val = -E1000_ERR_NVM;
		goto out;
	}

	ret_val = nvm->ops.acquire(hw);
	if (ret_val)
		goto out;

	ret_val = e1000_valid_nvm_bank_detect_ich8lan(hw, &bank);
	if (ret_val != E1000_SUCCESS)
		goto release;

	act_offset = (bank) ? nvm->flash_bank_size : 0;
	act_offset += offset;

	for (i = 0; i < words; i++) {
		if ((dev_spec->shadow_ram) &&
		    (dev_spec->shadow_ram[offset+i].modified)) {
			data[i] = dev_spec->shadow_ram[offset+i].value;
		} else {
			ret_val = e1000_read_flash_word_ich8lan(hw,
			                                        act_offset + i,
			                                        &word);
			if (ret_val)
				break;
			data[i] = word;
		}
	}

release:
	nvm->ops.release(hw);

out:
	if (ret_val)
		e_dbg("NVM read error: %d\n", ret_val);

	return ret_val;
}

/**
 *  e1000_flash_cycle_init_ich8lan - Initialize flash
 *  @hw: pointer to the HW structure
 *
 *  This function does initial flash setup so that a new read/write/erase cycle
 *  can be started.
 **/
static s32 e1000_flash_cycle_init_ich8lan(struct e1000_hw *hw)
{
	union ich8_hws_flash_status hsfsts;
	s32 ret_val = -E1000_ERR_NVM;
	s32 i = 0;

	hsfsts.regval = er16flash(ICH_FLASH_HSFSTS);

	/* Check if the flash descriptor is valid */
	if (hsfsts.hsf_status.fldesvalid == 0) {
		e_dbg("Flash descriptor invalid.  "
		         "SW Sequencing must be used.");
		goto out;
	}

	/* Clear FCERR and DAEL in hw status by writing 1 */
	hsfsts.hsf_status.flcerr = 1;
	hsfsts.hsf_status.dael = 1;

	ew16flash(ICH_FLASH_HSFSTS, hsfsts.regval);

	/*
	 * Either we should have a hardware SPI cycle in progress
	 * bit to check against, in order to start a new cycle or
	 * FDONE bit should be changed in the hardware so that it
	 * is 1 after hardware reset, which can then be used as an
	 * indication whether a cycle is in progress or has been
	 * completed.
	 */

	if (hsfsts.hsf_status.flcinprog == 0) {
		/*
		 * There is no cycle running at present,
		 * so we can start a cycle.
		 * Begin by setting Flash Cycle Done.
		 */
		hsfsts.hsf_status.flcdone = 1;
		ew16flash(ICH_FLASH_HSFSTS, hsfsts.regval);
		ret_val = E1000_SUCCESS;
	} else {
		/*
		 * Otherwise poll for sometime so the current
		 * cycle has a chance to end before giving up.
		 */
		for (i = 0; i < ICH_FLASH_READ_COMMAND_TIMEOUT; i++) {
			hsfsts.regval = er16flash(
			                                      ICH_FLASH_HSFSTS);
			if (hsfsts.hsf_status.flcinprog == 0) {
				ret_val = E1000_SUCCESS;
				break;
			}
			udelay(1);
		}
		if (ret_val == E1000_SUCCESS) {
			/*
			 * Successful in waiting for previous cycle to timeout,
			 * now set the Flash Cycle Done.
			 */
			hsfsts.hsf_status.flcdone = 1;
			ew16flash(ICH_FLASH_HSFSTS,
			                        hsfsts.regval);
		} else {
			e_dbg("Flash controller busy, cannot get access");
		}
	}

out:
	return ret_val;
}

/**
 *  e1000_flash_cycle_ich8lan - Starts flash cycle (read/write/erase)
 *  @hw: pointer to the HW structure
 *  @timeout: maximum time to wait for completion
 *
 *  This function starts a flash cycle and waits for its completion.
 **/
static s32 e1000_flash_cycle_ich8lan(struct e1000_hw *hw, u32 timeout)
{
	union ich8_hws_flash_ctrl hsflctl;
	union ich8_hws_flash_status hsfsts;
	s32 ret_val = -E1000_ERR_NVM;
	u32 i = 0;

	/* Start a cycle by writing 1 in Flash Cycle Go in Hw Flash Control */
	hsflctl.regval = er16flash(ICH_FLASH_HSFCTL);
	hsflctl.hsf_ctrl.flcgo = 1;
	ew16flash(ICH_FLASH_HSFCTL, hsflctl.regval);

	/* wait till FDONE bit is set to 1 */
	do {
		hsfsts.regval = er16flash(ICH_FLASH_HSFSTS);
		if (hsfsts.hsf_status.flcdone == 1)
			break;
		udelay(1);
	} while (i++ < timeout);

	if (hsfsts.hsf_status.flcdone == 1 && hsfsts.hsf_status.flcerr == 0)
		ret_val = E1000_SUCCESS;

	return ret_val;
}

/**
 *  e1000_read_flash_word_ich8lan - Read word from flash
 *  @hw: pointer to the HW structure
 *  @offset: offset to data location
 *  @data: pointer to the location for storing the data
 *
 *  Reads the flash word at offset into data.  Offset is converted
 *  to bytes before read.
 **/
static s32 e1000_read_flash_word_ich8lan(struct e1000_hw *hw, u32 offset,
                                         u16 *data)
{
	s32 ret_val;

	if (!data) {
		ret_val = -E1000_ERR_NVM;
		goto out;
	}

	/* Must convert offset into bytes. */
	offset <<= 1;

	ret_val = e1000_read_flash_data_ich8lan(hw, offset, 2, data);

out:
	return ret_val;
}

/**
 *  e1000_read_flash_byte_ich8lan - Read byte from flash
 *  @hw: pointer to the HW structure
 *  @offset: The offset of the byte to read.
 *  @data: Pointer to a byte to store the value read.
 *
 *  Reads a single byte from the NVM using the flash access registers.
 **/
static s32 e1000_read_flash_byte_ich8lan(struct e1000_hw *hw, u32 offset,
                                         u8 *data)
{
	s32 ret_val = E1000_SUCCESS;
	u16 word = 0;

	ret_val = e1000_read_flash_data_ich8lan(hw, offset, 1, &word);
	if (ret_val)
		goto out;

	*data = (u8)word;

out:
	return ret_val;
}

/**
 *  e1000_read_flash_data_ich8lan - Read byte or word from NVM
 *  @hw: pointer to the HW structure
 *  @offset: The offset (in bytes) of the byte or word to read.
 *  @size: Size of data to read, 1=byte 2=word
 *  @data: Pointer to the word to store the value read.
 *
 *  Reads a byte or word from the NVM using the flash access registers.
 **/
static s32 e1000_read_flash_data_ich8lan(struct e1000_hw *hw, u32 offset,
                                         u8 size, u16 *data)
{
	union ich8_hws_flash_status hsfsts;
	union ich8_hws_flash_ctrl hsflctl;
	u32 flash_linear_addr;
	u32 flash_data = 0;
	s32 ret_val = -E1000_ERR_NVM;
	u8 count = 0;

	if (size < 1  || size > 2 || offset > ICH_FLASH_LINEAR_ADDR_MASK)
		goto out;
	flash_linear_addr = (ICH_FLASH_LINEAR_ADDR_MASK & offset) +
	                    hw->nvm.flash_base_addr;

	do {
		udelay(1);
		/* Steps */
		ret_val = e1000_flash_cycle_init_ich8lan(hw);
		if (ret_val != E1000_SUCCESS)
			break;

		hsflctl.regval = er16flash(ICH_FLASH_HSFCTL);
		/* 0b/1b corresponds to 1 or 2 byte size, respectively. */
		hsflctl.hsf_ctrl.fldbcount = size - 1;
		hsflctl.hsf_ctrl.flcycle = ICH_CYCLE_READ;
		ew16flash(ICH_FLASH_HSFCTL, hsflctl.regval);

		ew32flash(ICH_FLASH_FADDR, flash_linear_addr);

		ret_val = e1000_flash_cycle_ich8lan(hw,
		                                ICH_FLASH_READ_COMMAND_TIMEOUT);

		/*
		 * Check if FCERR is set to 1, if set to 1, clear it
		 * and try the whole sequence a few more times, else
		 * read in (shift in) the Flash Data0, the order is
		 * least significant byte first msb to lsb
		 */
		if (ret_val == E1000_SUCCESS) {
			flash_data = er32flash(ICH_FLASH_FDATA0);
			if (size == 1)
				*data = (u8)(flash_data & 0x000000FF);
			else if (size == 2)
				*data = (u16)(flash_data & 0x0000FFFF);
			break;
		} else {
			/*
			 * If we've gotten here, then things are probably
			 * completely hosed, but if the error condition is
			 * detected, it won't hurt to give it another try...
			 * ICH_FLASH_CYCLE_REPEAT_COUNT times.
			 */
			hsfsts.regval = er16flash(
			                                      ICH_FLASH_HSFSTS);
			if (hsfsts.hsf_status.flcerr == 1) {
				/* Repeat for some time before giving up. */
				continue;
			} else if (hsfsts.hsf_status.flcdone == 0) {
				e_dbg("Timeout error - flash cycle "
				         "did not complete.");
				break;
			}
		}
	} while (count++ < ICH_FLASH_CYCLE_REPEAT_COUNT);

out:
	return ret_val;
}

/**
 *  e1000_write_nvm_ich8lan - Write word(s) to the NVM
 *  @hw: pointer to the HW structure
 *  @offset: The offset (in bytes) of the word(s) to write.
 *  @words: Size of data to write in words
 *  @data: Pointer to the word(s) to write at offset.
 *
 *  Writes a byte or word to the NVM using the flash access registers.
 **/
static s32 e1000_write_nvm_ich8lan(struct e1000_hw *hw, u16 offset, u16 words,
                                   u16 *data)
{
	struct e1000_nvm_info *nvm = &hw->nvm;
	struct e1000_dev_spec_ich8lan *dev_spec = &hw->dev_spec.ich8lan;
	s32 ret_val = E1000_SUCCESS;
	u16 i;

	if ((offset >= nvm->word_size) || (words > nvm->word_size - offset) ||
	    (words == 0)) {
		e_dbg("nvm parameter(s) out of bounds\n");
		ret_val = -E1000_ERR_NVM;
		goto out;
	}

	ret_val = nvm->ops.acquire(hw);
	if (ret_val)
		goto out;

	for (i = 0; i < words; i++) {
		dev_spec->shadow_ram[offset+i].modified = true;
		dev_spec->shadow_ram[offset+i].value = data[i];
	}

	nvm->ops.release(hw);

out:
	return ret_val;
}

/**
 *  e1000_update_nvm_checksum_ich8lan - Update the checksum for NVM
 *  @hw: pointer to the HW structure
 *
 *  The NVM checksum is updated by calling the generic update_nvm_checksum,
 *  which writes the checksum to the shadow ram.  The changes in the shadow
 *  ram are then committed to the EEPROM by processing each bank at a time
 *  checking for the modified bit and writing only the pending changes.
 *  After a successful commit, the shadow ram is cleared and is ready for
 *  future writes.
 **/
static s32 e1000_update_nvm_checksum_ich8lan(struct e1000_hw *hw)
{
	struct e1000_nvm_info *nvm = &hw->nvm;
	struct e1000_dev_spec_ich8lan *dev_spec = &hw->dev_spec.ich8lan;
	u32 i, act_offset, new_bank_offset, old_bank_offset, bank;
	s32 ret_val;
	u16 data;

	ret_val = e1000e_update_nvm_checksum_generic(hw);
	if (ret_val)
		goto out;

	if (nvm->type != e1000_nvm_flash_sw)
		goto out;

	ret_val = nvm->ops.acquire(hw);
	if (ret_val)
		goto out;

	/*
	 * We're writing to the opposite bank so if we're on bank 1,
	 * write to bank 0 etc.  We also need to erase the segment that
	 * is going to be written
	 */
	ret_val =  e1000_valid_nvm_bank_detect_ich8lan(hw, &bank);
	if (ret_val != E1000_SUCCESS) {
		nvm->ops.release(hw);
		goto out;
	}

	if (bank == 0) {
		new_bank_offset = nvm->flash_bank_size;
		old_bank_offset = 0;
		ret_val = e1000_erase_flash_bank_ich8lan(hw, 1);
		if (ret_val) {
			nvm->ops.release(hw);
			goto out;
		}
	} else {
		old_bank_offset = nvm->flash_bank_size;
		new_bank_offset = 0;
		ret_val = e1000_erase_flash_bank_ich8lan(hw, 0);
		if (ret_val) {
			nvm->ops.release(hw);
			goto out;
		}
	}

	for (i = 0; i < E1000_ICH8_SHADOW_RAM_WORDS; i++) {
		/*
		 * Determine whether to write the value stored
		 * in the other NVM bank or a modified value stored
		 * in the shadow RAM
		 */
		if (dev_spec->shadow_ram[i].modified) {
			data = dev_spec->shadow_ram[i].value;
		} else {
			ret_val = e1000_read_flash_word_ich8lan(hw, i +
			                                        old_bank_offset,
			                                        &data);
			if (ret_val)
				break;
		}

		/*
		 * If the word is 0x13, then make sure the signature bits
		 * (15:14) are 11b until the commit has completed.
		 * This will allow us to write 10b which indicates the
		 * signature is valid.  We want to do this after the write
		 * has completed so that we don't mark the segment valid
		 * while the write is still in progress
		 */
		if (i == E1000_ICH_NVM_SIG_WORD)
			data |= E1000_ICH_NVM_SIG_MASK;

		/* Convert offset to bytes. */
		act_offset = (i + new_bank_offset) << 1;

		udelay(100);
		/* Write the bytes to the new bank. */
		ret_val = e1000_retry_write_flash_byte_ich8lan(hw,
		                                               act_offset,
		                                               (u8)data);
		if (ret_val)
			break;

		udelay(100);
		ret_val = e1000_retry_write_flash_byte_ich8lan(hw,
		                                          act_offset + 1,
		                                          (u8)(data >> 8));
		if (ret_val)
			break;
	}

	/*
	 * Don't bother writing the segment valid bits if sector
	 * programming failed.
	 */
	if (ret_val) {
		e_dbg("Flash commit failed.\n");
		nvm->ops.release(hw);
		goto out;
	}

	/*
	 * Finally validate the new segment by setting bit 15:14
	 * to 10b in word 0x13 , this can be done without an
	 * erase as well since these bits are 11 to start with
	 * and we need to change bit 14 to 0b
	 */
	act_offset = new_bank_offset + E1000_ICH_NVM_SIG_WORD;
	ret_val = e1000_read_flash_word_ich8lan(hw, act_offset, &data);
	if (ret_val) {
		nvm->ops.release(hw);
		goto out;
	}
	data &= 0xBFFF;
	ret_val = e1000_retry_write_flash_byte_ich8lan(hw,
	                                               act_offset * 2 + 1,
	                                               (u8)(data >> 8));
	if (ret_val) {
		nvm->ops.release(hw);
		goto out;
	}

	/*
	 * And invalidate the previously valid segment by setting
	 * its signature word (0x13) high_byte to 0b. This can be
	 * done without an erase because flash erase sets all bits
	 * to 1's. We can write 1's to 0's without an erase
	 */
	act_offset = (old_bank_offset + E1000_ICH_NVM_SIG_WORD) * 2 + 1;
	ret_val = e1000_retry_write_flash_byte_ich8lan(hw, act_offset, 0);
	if (ret_val) {
		nvm->ops.release(hw);
		goto out;
	}

	/* Great!  Everything worked, we can now clear the cached entries. */
	for (i = 0; i < E1000_ICH8_SHADOW_RAM_WORDS; i++) {
		dev_spec->shadow_ram[i].modified = false;
		dev_spec->shadow_ram[i].value = 0xFFFF;
	}

	nvm->ops.release(hw);

	/*
	 * Reload the EEPROM, or else modifications will not appear
	 * until after the next adapter reset.
	 */
	nvm->ops.reload(hw);
	msleep(10);

out:
	if (ret_val)
		e_dbg("NVM update error: %d\n", ret_val);

	return ret_val;
}

/**
 *  e1000_validate_nvm_checksum_ich8lan - Validate EEPROM checksum
 *  @hw: pointer to the HW structure
 *
 *  Check to see if checksum needs to be fixed by reading bit 6 in word 0x19.
 *  If the bit is 0, that the EEPROM had been modified, but the checksum was not
 *  calculated, in which case we need to calculate the checksum and set bit 6.
 **/
static s32 e1000_validate_nvm_checksum_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;
	u16 data;

	/*
	 * Read 0x19 and check bit 6.  If this bit is 0, the checksum
	 * needs to be fixed.  This bit is an indication that the NVM
	 * was prepared by OEM software and did not calculate the
	 * checksum...a likely scenario.
	 */
	ret_val = e1000_read_nvm(hw, 0x19, 1, &data);
	if (ret_val)
		goto out;

	if ((data & 0x40) == 0) {
		data |= 0x40;
		ret_val = e1000_write_nvm(hw, 0x19, 1, &data);
		if (ret_val)
			goto out;
		ret_val = e1000e_update_nvm_checksum(hw);
		if (ret_val)
			goto out;
	}

	ret_val = e1000e_validate_nvm_checksum_generic(hw);

out:
	return ret_val;
}

/**
 *  e1000_write_flash_data_ich8lan - Writes bytes to the NVM
 *  @hw: pointer to the HW structure
 *  @offset: The offset (in bytes) of the byte/word to read.
 *  @size: Size of data to read, 1=byte 2=word
 *  @data: The byte(s) to write to the NVM.
 *
 *  Writes one/two bytes to the NVM using the flash access registers.
 **/
static s32 e1000_write_flash_data_ich8lan(struct e1000_hw *hw, u32 offset,
                                          u8 size, u16 data)
{
	union ich8_hws_flash_status hsfsts;
	union ich8_hws_flash_ctrl hsflctl;
	u32 flash_linear_addr;
	u32 flash_data = 0;
	s32 ret_val = -E1000_ERR_NVM;
	u8 count = 0;

	if (size < 1 || size > 2 || data > size * 0xff ||
	    offset > ICH_FLASH_LINEAR_ADDR_MASK)
		goto out;

	flash_linear_addr = (ICH_FLASH_LINEAR_ADDR_MASK & offset) +
	                    hw->nvm.flash_base_addr;

	do {
		udelay(1);
		/* Steps */
		ret_val = e1000_flash_cycle_init_ich8lan(hw);
		if (ret_val != E1000_SUCCESS)
			break;

		hsflctl.regval = er16flash(ICH_FLASH_HSFCTL);
		/* 0b/1b corresponds to 1 or 2 byte size, respectively. */
		hsflctl.hsf_ctrl.fldbcount = size - 1;
		hsflctl.hsf_ctrl.flcycle = ICH_CYCLE_WRITE;
		ew16flash(ICH_FLASH_HSFCTL, hsflctl.regval);

		ew32flash(ICH_FLASH_FADDR, flash_linear_addr);

		if (size == 1)
			flash_data = (u32)data & 0x00FF;
		else
			flash_data = (u32)data;

		ew32flash(ICH_FLASH_FDATA0, flash_data);

		/*
		 * check if FCERR is set to 1 , if set to 1, clear it
		 * and try the whole sequence a few more times else done
		 */
		ret_val = e1000_flash_cycle_ich8lan(hw,
		                               ICH_FLASH_WRITE_COMMAND_TIMEOUT);
		if (ret_val == E1000_SUCCESS)
			break;

		/*
		 * If we're here, then things are most likely
		 * completely hosed, but if the error condition
		 * is detected, it won't hurt to give it another
		 * try...ICH_FLASH_CYCLE_REPEAT_COUNT times.
		 */
		hsfsts.regval = er16flash(ICH_FLASH_HSFSTS);
		if (hsfsts.hsf_status.flcerr == 1) {
			/* Repeat for some time before giving up. */
			continue;
		} else if (hsfsts.hsf_status.flcdone == 0) {
			e_dbg("Timeout error - flash cycle "
				 "did not complete.");
			break;
		}
	} while (count++ < ICH_FLASH_CYCLE_REPEAT_COUNT);

out:
	return ret_val;
}

/**
 *  e1000_write_flash_byte_ich8lan - Write a single byte to NVM
 *  @hw: pointer to the HW structure
 *  @offset: The index of the byte to read.
 *  @data: The byte to write to the NVM.
 *
 *  Writes a single byte to the NVM using the flash access registers.
 **/
static s32 e1000_write_flash_byte_ich8lan(struct e1000_hw *hw, u32 offset,
                                          u8 data)
{
	u16 word = (u16)data;

	return e1000_write_flash_data_ich8lan(hw, offset, 1, word);
}

/**
 *  e1000_retry_write_flash_byte_ich8lan - Writes a single byte to NVM
 *  @hw: pointer to the HW structure
 *  @offset: The offset of the byte to write.
 *  @byte: The byte to write to the NVM.
 *
 *  Writes a single byte to the NVM using the flash access registers.
 *  Goes through a retry algorithm before giving up.
 **/
static s32 e1000_retry_write_flash_byte_ich8lan(struct e1000_hw *hw,
                                                u32 offset, u8 byte)
{
	s32 ret_val;
	u16 program_retries;

	ret_val = e1000_write_flash_byte_ich8lan(hw, offset, byte);
	if (ret_val == E1000_SUCCESS)
		goto out;

	for (program_retries = 0; program_retries < 100; program_retries++) {
		e_dbg("Retrying Byte %2.2X at offset %u\n", byte, offset);
		udelay(100);
		ret_val = e1000_write_flash_byte_ich8lan(hw, offset, byte);
		if (ret_val == E1000_SUCCESS)
			break;
	}
	if (program_retries == 100) {
		ret_val = -E1000_ERR_NVM;
		goto out;
	}

out:
	return ret_val;
}

/**
 *  e1000_erase_flash_bank_ich8lan - Erase a bank (4k) from NVM
 *  @hw: pointer to the HW structure
 *  @bank: 0 for first bank, 1 for second bank, etc.
 *
 *  Erases the bank specified. Each bank is a 4k block. Banks are 0 based.
 *  bank N is 4096 * N + flash_reg_addr.
 **/
static s32 e1000_erase_flash_bank_ich8lan(struct e1000_hw *hw, u32 bank)
{
	struct e1000_nvm_info *nvm = &hw->nvm;
	union ich8_hws_flash_status hsfsts;
	union ich8_hws_flash_ctrl hsflctl;
	u32 flash_linear_addr;
	/* bank size is in 16bit words - adjust to bytes */
	u32 flash_bank_size = nvm->flash_bank_size * 2;
	s32 ret_val = E1000_SUCCESS;
	s32 count = 0;
	s32 j, iteration, sector_size;

	hsfsts.regval = er16flash(ICH_FLASH_HSFSTS);

	/*
	 * Determine HW Sector size: Read BERASE bits of hw flash status
	 * register
	 * 00: The Hw sector is 256 bytes, hence we need to erase 16
	 *     consecutive sectors.  The start index for the nth Hw sector
	 *     can be calculated as = bank * 4096 + n * 256
	 * 01: The Hw sector is 4K bytes, hence we need to erase 1 sector.
	 *     The start index for the nth Hw sector can be calculated
	 *     as = bank * 4096
	 * 10: The Hw sector is 8K bytes, nth sector = bank * 8192
	 *     (ich9 only, otherwise error condition)
	 * 11: The Hw sector is 64K bytes, nth sector = bank * 65536
	 */
	switch (hsfsts.hsf_status.berasesz) {
	case 0:
		/* Hw sector size 256 */
		sector_size = ICH_FLASH_SEG_SIZE_256;
		iteration = flash_bank_size / ICH_FLASH_SEG_SIZE_256;
		break;
	case 1:
		sector_size = ICH_FLASH_SEG_SIZE_4K;
		iteration = 1;
		break;
	case 2:
		if (hw->mac.type == e1000_ich9lan) {
			sector_size = ICH_FLASH_SEG_SIZE_8K;
			iteration = flash_bank_size / ICH_FLASH_SEG_SIZE_8K;
		} else {
			ret_val = -E1000_ERR_NVM;
			goto out;
		}
		break;
	case 3:
		sector_size = ICH_FLASH_SEG_SIZE_64K;
		iteration = 1;
		break;
	default:
		ret_val = -E1000_ERR_NVM;
		goto out;
	}

	/* Start with the base address, then add the sector offset. */
	flash_linear_addr = hw->nvm.flash_base_addr;
	flash_linear_addr += (bank) ? (sector_size * iteration) : 0;

	for (j = 0; j < iteration ; j++) {
		do {
			/* Steps */
			ret_val = e1000_flash_cycle_init_ich8lan(hw);
			if (ret_val)
				goto out;

			/*
			 * Write a value 11 (block Erase) in Flash
			 * Cycle field in hw flash control
			 */
			hsflctl.regval = er16flash(
			                                      ICH_FLASH_HSFCTL);
			hsflctl.hsf_ctrl.flcycle = ICH_CYCLE_ERASE;
			ew16flash(ICH_FLASH_HSFCTL,
			                        hsflctl.regval);

			/*
			 * Write the last 24 bits of an index within the
			 * block into Flash Linear address field in Flash
			 * Address.
			 */
			flash_linear_addr += (j * sector_size);
			ew32flash(ICH_FLASH_FADDR,
			                      flash_linear_addr);

			ret_val = e1000_flash_cycle_ich8lan(hw,
			                       ICH_FLASH_ERASE_COMMAND_TIMEOUT);
			if (ret_val == E1000_SUCCESS)
				break;

			/*
			 * Check if FCERR is set to 1.  If 1,
			 * clear it and try the whole sequence
			 * a few more times else Done
			 */
			hsfsts.regval = er16flash(
						      ICH_FLASH_HSFSTS);
			if (hsfsts.hsf_status.flcerr == 1)
				/* repeat for some time before giving up */
				continue;
			else if (hsfsts.hsf_status.flcdone == 0)
				goto out;
		} while (++count < ICH_FLASH_CYCLE_REPEAT_COUNT);
	}

out:
	return ret_val;
}

/**
 *  e1000_valid_led_default_ich8lan - Set the default LED settings
 *  @hw: pointer to the HW structure
 *  @data: Pointer to the LED settings
 *
 *  Reads the LED default settings from the NVM to data.  If the NVM LED
 *  settings is all 0's or F's, set the LED default to a valid LED default
 *  setting.
 **/
static s32 e1000_valid_led_default_ich8lan(struct e1000_hw *hw, u16 *data)
{
	s32 ret_val;

	ret_val = e1000_read_nvm(hw, NVM_ID_LED_SETTINGS, 1, data);
	if (ret_val) {
		e_dbg("NVM Read Error\n");
		goto out;
	}

	if (*data == ID_LED_RESERVED_0000 ||
	    *data == ID_LED_RESERVED_FFFF)
		*data = ID_LED_DEFAULT_ICH8LAN;

out:
	return ret_val;
}

/**
 *  e1000_id_led_init_pchlan - store LED configurations
 *  @hw: pointer to the HW structure
 *
 *  PCH does not control LEDs via the LEDCTL register, rather it uses
 *  the PHY LED configuration register.
 *
 *  PCH also does not have an "always on" or "always off" mode which
 *  complicates the ID feature.  Instead of using the "on" mode to indicate
 *  in ledctl_mode2 the LEDs to use for ID (see e1000e_id_led_init()),
 *  use "link_up" mode.  The LEDs will still ID on request if there is no
 *  link based on logic in e1000_led_[on|off]_pchlan().
 **/
static s32 e1000_id_led_init_pchlan(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	s32 ret_val;
	const u32 ledctl_on = E1000_LEDCTL_MODE_LINK_UP;
	const u32 ledctl_off = E1000_LEDCTL_MODE_LINK_UP | E1000_PHY_LED0_IVRT;
	u16 data, i, temp, shift;

	/* Get default ID LED modes */
	ret_val = hw->nvm.ops.valid_led_default(hw, &data);
	if (ret_val)
		goto out;

	mac->ledctl_default = er32(LEDCTL);
	mac->ledctl_mode1 = mac->ledctl_default;
	mac->ledctl_mode2 = mac->ledctl_default;

	for (i = 0; i < 4; i++) {
		temp = (data >> (i << 2)) & E1000_LEDCTL_LED0_MODE_MASK;
		shift = (i * 5);
		switch (temp) {
		case ID_LED_ON1_DEF2:
		case ID_LED_ON1_ON2:
		case ID_LED_ON1_OFF2:
			mac->ledctl_mode1 &= ~(E1000_PHY_LED0_MASK << shift);
			mac->ledctl_mode1 |= (ledctl_on << shift);
			break;
		case ID_LED_OFF1_DEF2:
		case ID_LED_OFF1_ON2:
		case ID_LED_OFF1_OFF2:
			mac->ledctl_mode1 &= ~(E1000_PHY_LED0_MASK << shift);
			mac->ledctl_mode1 |= (ledctl_off << shift);
			break;
		default:
			/* Do nothing */
			break;
		}
		switch (temp) {
		case ID_LED_DEF1_ON2:
		case ID_LED_ON1_ON2:
		case ID_LED_OFF1_ON2:
			mac->ledctl_mode2 &= ~(E1000_PHY_LED0_MASK << shift);
			mac->ledctl_mode2 |= (ledctl_on << shift);
			break;
		case ID_LED_DEF1_OFF2:
		case ID_LED_ON1_OFF2:
		case ID_LED_OFF1_OFF2:
			mac->ledctl_mode2 &= ~(E1000_PHY_LED0_MASK << shift);
			mac->ledctl_mode2 |= (ledctl_off << shift);
			break;
		default:
			/* Do nothing */
			break;
		}
	}

out:
	return ret_val;
}

/**
 *  e1000_get_bus_info_ich8lan - Get/Set the bus type and width
 *  @hw: pointer to the HW structure
 *
 *  ICH8 use the PCI Express bus, but does not contain a PCI Express Capability
 *  register, so the the bus width is hard coded.
 **/
static s32 e1000_get_bus_info_ich8lan(struct e1000_hw *hw)
{
	struct e1000_bus_info *bus = &hw->bus;
	s32 ret_val;

	ret_val = e1000e_get_bus_info_pcie(hw);

	/*
	 * ICH devices are "PCI Express"-ish.  They have
	 * a configuration space, but do not contain
	 * PCI Express Capability registers, so bus width
	 * must be hardcoded.
	 */
	if (bus->width == e1000_bus_width_unknown)
		bus->width = e1000_bus_width_pcie_x1;

	return ret_val;
}

/**
 *  e1000_reset_hw_ich8lan - Reset the hardware
 *  @hw: pointer to the HW structure
 *
 *  Does a full reset of the hardware which includes a reset of the PHY and
 *  MAC.
 **/
static s32 e1000_reset_hw_ich8lan(struct e1000_hw *hw)
{
	u32 ctrl, icr, kab;
	s32 ret_val;

	/*
	 * Prevent the PCI-E bus from sticking if there is no TLP connection
	 * on the last TLP read/write transaction when MAC is reset.
	 */
	ret_val = e1000e_disable_pcie_master(hw);
	if (ret_val)
		e_dbg("PCI-E Master disable polling has failed.\n");

	e_dbg("Masking off all interrupts\n");
	ew32(IMC, 0xffffffff);

	/*
	 * Disable the Transmit and Receive units.  Then delay to allow
	 * any pending transactions to complete before we hit the MAC
	 * with the global reset.
	 */
	ew32(RCTL, 0);
	ew32(TCTL, E1000_TCTL_PSP);
	e1e_flush();

	msleep(10);

	/* Workaround for ICH8 bit corruption issue in FIFO memory */
	if (hw->mac.type == e1000_ich8lan) {
		/* Set Tx and Rx buffer allocation to 8k apiece. */
		ew32(PBA, E1000_PBA_8K);
		/* Set Packet Buffer Size to 16k. */
		ew32(PBS, E1000_PBS_16K);
	}

	ctrl = er32(CTRL);

	if (!e1000_check_reset_block(hw) && !hw->phy.reset_disable) {
		/* Clear PHY Reset Asserted bit */
		if (hw->mac.type >= e1000_pchlan) {
			u32 status = er32(STATUS);
			ew32(STATUS, status &
			                ~E1000_STATUS_PHYRA);
		}

		/*
		 * PHY HW reset requires MAC CORE reset at the same
		 * time to make sure the interface between MAC and the
		 * external PHY is reset.
		 */
		ctrl |= E1000_CTRL_PHY_RST;
	}
	ret_val = e1000_acquire_swflag_ich8lan(hw);
	e_dbg("Issuing a global reset to ich8lan\n");
	ew32(CTRL, (ctrl | E1000_CTRL_RST));
	msleep(20);

	if (!ret_val)
		e1000_release_swflag_ich8lan(hw);

	if (ctrl & E1000_CTRL_PHY_RST)
		ret_val = hw->phy.ops.get_cfg_done(hw);

	if (hw->mac.type >= e1000_ich10lan) {
		e1000_lan_init_done_ich8lan(hw);
	} else {
		if (!ret_val) {
		/* release the swflag because it is not reset by
		 * hardware reset
		 */
		e1000_release_swflag_ich8lan(hw);
	}

	ret_val = e1000e_get_auto_rd_done(hw);
		if (ret_val) {
			/*
			 * When auto config read does not complete, do not
			 * return with an error. This can happen in situations
			 * where there is no eeprom and prevents getting link.
			 */
			e_dbg("Auto Read Done did not complete\n");
		}
	}

	/*
	 * For PCH, this write will make sure that any noise
	 * will be detected as a CRC error and be dropped rather than show up
	 * as a bad packet to the DMA engine.
	 */
	if (hw->mac.type == e1000_pchlan)
		ew32(CRC_OFFSET, 0x65656565);

	ew32(IMC, 0xffffffff);
	icr = er32(ICR);

	kab = er32(KABGTXD);
	kab |= E1000_KABGTXD_BGSQLBIAS;
	ew32(KABGTXD, kab);

	if (hw->mac.type == e1000_pchlan)
		ret_val = e1000_hv_phy_workarounds_ich8lan(hw);

	return ret_val;
}

/**
 *  e1000_init_hw_ich8lan - Initialize the hardware
 *  @hw: pointer to the HW structure
 *
 *  Prepares the hardware for transmit and receive by doing the following:
 *   - initialize hardware bits
 *   - initialize LED identification
 *   - setup receive address registers
 *   - setup flow control
 *   - setup transmit descriptors
 *   - clear statistics
 **/
static s32 e1000_init_hw_ich8lan(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	u32 ctrl_ext, txdctl, snoop;
	s32 ret_val;
	u16 i;

	e1000_initialize_hw_bits_ich8lan(hw);

	/* Initialize identification LED */
	ret_val = mac->ops.id_led_init(hw);
	if (ret_val)
		/* This is not fatal and we should not stop init due to this */
		e_dbg("Error initializing identification LED\n");

	/* Setup the receive address. */
	e1000e_init_rx_addrs(hw, mac->rar_entry_count);

	/* Zero out the Multicast HASH table */
	e_dbg("Zeroing the MTA\n");
	for (i = 0; i < mac->mta_reg_count; i++)
		E1000_WRITE_REG_ARRAY(hw, E1000_MTA, i, 0);

	/*
	 * The 82578 Rx buffer will stall if wakeup is enabled in host and
	 * the ME.  Reading the BM_WUC register will clear the host wakeup bit.
	 * Reset the phy after disabling host wakeup to reset the Rx buffer.
	 */
	if (hw->phy.type == e1000_phy_82578) {
		e1e_rphy(hw, BM_WUC, &i);
		ret_val = e1000_phy_hw_reset_ich8lan(hw);
		if (ret_val)
			return ret_val;
	}

	/* Setup link and flow control */
	ret_val = mac->ops.setup_link(hw);

	/* Set the transmit descriptor write-back policy for both queues */
	txdctl = er32(TXDCTL(0));
	txdctl = (txdctl & ~E1000_TXDCTL_WTHRESH) |
		 E1000_TXDCTL_FULL_TX_DESC_WB;
	txdctl = (txdctl & ~E1000_TXDCTL_PTHRESH) |
	         E1000_TXDCTL_MAX_TX_DESC_PREFETCH;
	ew32(TXDCTL(0), txdctl);
	txdctl = er32(TXDCTL(1));
	txdctl = (txdctl & ~E1000_TXDCTL_WTHRESH) |
		 E1000_TXDCTL_FULL_TX_DESC_WB;
	txdctl = (txdctl & ~E1000_TXDCTL_PTHRESH) |
	         E1000_TXDCTL_MAX_TX_DESC_PREFETCH;
	ew32(TXDCTL(1), txdctl);

	/*
	 * ICH8 has opposite polarity of no_snoop bits.
	 * By default, we should use snoop behavior.
	 */
	if (mac->type == e1000_ich8lan)
		snoop = PCIE_ICH8_SNOOP_ALL;
	else
		snoop = (u32)~(PCIE_NO_SNOOP_ALL);
	e1000e_set_pcie_no_snoop(hw, snoop);

	ctrl_ext = er32(CTRL_EXT);
	ctrl_ext |= E1000_CTRL_EXT_RO_DIS;
	ew32(CTRL_EXT, ctrl_ext);

	/*
	 * Clear all of the statistics registers (clear on read).  It is
	 * important that we do this after we have tried to establish link
	 * because the symbol error count will increment wildly if there
	 * is no link.
	 */
	e1000_clear_hw_cntrs_ich8lan(hw);

	return ret_val;
}
/**
 *  e1000_initialize_hw_bits_ich8lan - Initialize required hardware bits
 *  @hw: pointer to the HW structure
 *
 *  Sets/Clears required hardware bits necessary for correctly setting up the
 *  hardware for transmit and receive.
 **/
static void e1000_initialize_hw_bits_ich8lan(struct e1000_hw *hw)
{
	u32 reg;

	/* Extended Device Control */
	reg = er32(CTRL_EXT);
	reg |= (1 << 22);
	/* Enable PHY low-power state when MAC is at D3 w/o WoL */
	if (hw->mac.type >= e1000_pchlan)
		reg |= E1000_CTRL_EXT_PHYPDEN;
	ew32(CTRL_EXT, reg);

	/* Transmit Descriptor Control 0 */
	reg = er32(TXDCTL(0));
	reg |= (1 << 22);
	ew32(TXDCTL(0), reg);

	/* Transmit Descriptor Control 1 */
	reg = er32(TXDCTL(1));
	reg |= (1 << 22);
	ew32(TXDCTL(1), reg);

	/* Transmit Arbitration Control 0 */
	reg = er32(TARC(0));
	if (hw->mac.type == e1000_ich8lan)
		reg |= (1 << 28) | (1 << 29);
	reg |= (1 << 23) | (1 << 24) | (1 << 26) | (1 << 27);
	ew32(TARC(0), reg);

	/* Transmit Arbitration Control 1 */
	reg = er32(TARC(1));
	if (er32(TCTL) & E1000_TCTL_MULR)
		reg &= ~(1 << 28);
	else
		reg |= (1 << 28);
	reg |= (1 << 24) | (1 << 26) | (1 << 30);
	ew32(TARC(1), reg);

	/* Device Status */
	if (hw->mac.type == e1000_ich8lan) {
		reg = er32(STATUS);
		reg &= ~(1 << 31);
		ew32(STATUS, reg);
	}

	return;
}

/**
 *  e1000_setup_link_ich8lan - Setup flow control and link settings
 *  @hw: pointer to the HW structure
 *
 *  Determines which flow control settings to use, then configures flow
 *  control.  Calls the appropriate media-specific link configuration
 *  function.  Assuming the adapter has a valid link partner, a valid link
 *  should be established.  Assumes the hardware has previously been reset
 *  and the transmitter and receiver are not enabled.
 **/
static s32 e1000_setup_link_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;

	if (e1000_check_reset_block(hw))
		goto out;

	/*
	 * ICH parts do not have a word in the NVM to determine
	 * the default flow control setting, so we explicitly
	 * set it to full.
	 */
	if (hw->fc.requested_mode == e1000_fc_default)
		hw->fc.requested_mode = e1000_fc_full;

	/*
	 * Save off the requested flow control mode for use later.  Depending
	 * on the link partner's capabilities, we may or may not use this mode.
	 */
	hw->fc.current_mode = hw->fc.requested_mode;

	e_dbg("After fix-ups FlowControl is now = %x\n",
		hw->fc.current_mode);

	/* Continue to configure the copper link. */
	ret_val = hw->mac.ops.setup_physical_interface(hw);
	if (ret_val)
		goto out;

	ew32(FCTTV, hw->fc.pause_time);
	if ((hw->phy.type == e1000_phy_82578) ||
	    (hw->phy.type == e1000_phy_82577)) {
		ret_val = e1e_wphy(hw,
		                             PHY_REG(BM_PORT_CTRL_PAGE, 27),
		                             hw->fc.pause_time);
		if (ret_val)
			goto out;
	}

	ret_val = e1000e_set_fc_watermarks(hw);

out:
	return ret_val;
}

/**
 *  e1000_setup_copper_link_ich8lan - Configure MAC/PHY interface
 *  @hw: pointer to the HW structure
 *
 *  Configures the kumeran interface to the PHY to wait the appropriate time
 *  when polling the PHY, then call the generic setup_copper_link to finish
 *  configuring the copper link.
 **/
static s32 e1000_setup_copper_link_ich8lan(struct e1000_hw *hw)
{
	u32 ctrl;
	s32 ret_val;
	u16 reg_data;

	ctrl = er32(CTRL);
	ctrl |= E1000_CTRL_SLU;
	ctrl &= ~(E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
	ew32(CTRL, ctrl);

	/*
	 * Set the mac to wait the maximum time between each iteration
	 * and increase the max iterations when polling the phy;
	 * this fixes erroneous timeouts at 10Mbps.
	 */
	ret_val = e1000e_write_kmrn_reg(hw,
	                                       E1000_KMRNCTRLSTA_TIMEOUTS,
	                                       0xFFFF);
	if (ret_val)
		goto out;
	ret_val = e1000e_read_kmrn_reg(hw,
	                                      E1000_KMRNCTRLSTA_INBAND_PARAM,
	                                      &reg_data);
	if (ret_val)
		goto out;
	reg_data |= 0x3F;
	ret_val = e1000e_write_kmrn_reg(hw,
	                                       E1000_KMRNCTRLSTA_INBAND_PARAM,
	                                       reg_data);
	if (ret_val)
		goto out;

	switch (hw->phy.type) {
	case e1000_phy_igp_3:
		ret_val = e1000e_copper_link_setup_igp(hw);
		if (ret_val)
			goto out;
		break;
	case e1000_phy_bm:
	case e1000_phy_82578:
		ret_val = e1000e_copper_link_setup_m88(hw);
		if (ret_val)
			goto out;
		break;
	case e1000_phy_82577:
		ret_val = e1000_copper_link_setup_82577(hw);
		if (ret_val)
			goto out;
		break;
	case e1000_phy_ife:
		ret_val = e1e_rphy(hw, IFE_PHY_MDIX_CONTROL,
		                               &reg_data);
		if (ret_val)
			goto out;

		reg_data &= ~IFE_PMC_AUTO_MDIX;

		switch (hw->phy.mdix) {
		case 1:
			reg_data &= ~IFE_PMC_FORCE_MDIX;
			break;
		case 2:
			reg_data |= IFE_PMC_FORCE_MDIX;
			break;
		case 0:
		default:
			reg_data |= IFE_PMC_AUTO_MDIX;
			break;
		}
		ret_val = e1e_wphy(hw, IFE_PHY_MDIX_CONTROL,
		                                reg_data);
		if (ret_val)
			goto out;
		break;
	default:
		break;
	}
	ret_val = e1000e_setup_copper_link(hw);

out:
	return ret_val;
}

/**
 *  e1000_get_link_up_info_ich8lan - Get current link speed and duplex
 *  @hw: pointer to the HW structure
 *  @speed: pointer to store current link speed
 *  @duplex: pointer to store the current link duplex
 *
 *  Calls the generic get_speed_and_duplex to retrieve the current link
 *  information and then calls the Kumeran lock loss workaround for links at
 *  gigabit speeds.
 **/
static s32 e1000_get_link_up_info_ich8lan(struct e1000_hw *hw, u16 *speed,
                                          u16 *duplex)
{
	s32 ret_val;

	ret_val = e1000e_get_speed_and_duplex_copper(hw, speed, duplex);
	if (ret_val)
		goto out;

	if ((hw->mac.type == e1000_pchlan) && (*speed == SPEED_1000)) {
		ret_val = e1000e_write_kmrn_reg(hw,
						  E1000_KMRNCTRLSTA_K1_CONFIG,
						  E1000_KMRNCTRLSTA_K1_DISABLE);
		if (ret_val)
			goto out;
	}

	if ((hw->mac.type == e1000_ich8lan) &&
	    (hw->phy.type == e1000_phy_igp_3) &&
	    (*speed == SPEED_1000)) {
		ret_val = e1000_kmrn_lock_loss_workaround_ich8lan(hw);
	}

out:
	return ret_val;
}

/**
 *  e1000_kmrn_lock_loss_workaround_ich8lan - Kumeran workaround
 *  @hw: pointer to the HW structure
 *
 *  Work-around for 82566 Kumeran PCS lock loss:
 *  On link status change (i.e. PCI reset, speed change) and link is up and
 *  speed is gigabit-
 *    0) if workaround is optionally disabled do nothing
 *    1) wait 1ms for Kumeran link to come up
 *    2) check Kumeran Diagnostic register PCS lock loss bit
 *    3) if not set the link is locked (all is good), otherwise...
 *    4) reset the PHY
 *    5) repeat up to 10 times
 *  Note: this is only called for IGP3 copper when speed is 1gb.
 **/
static s32 e1000_kmrn_lock_loss_workaround_ich8lan(struct e1000_hw *hw)
{
	struct e1000_dev_spec_ich8lan *dev_spec = &hw->dev_spec.ich8lan;
	u32 phy_ctrl;
	s32 ret_val = E1000_SUCCESS;
	u16 i, data;
	bool link;

	if (!(dev_spec->kmrn_lock_loss_workaround_enabled))
		goto out;

	/*
	 * Make sure link is up before proceeding.  If not just return.
	 * Attempting this while link is negotiating fouled up link
	 * stability
	 */
	ret_val = e1000e_phy_has_link_generic(hw, 1, 0, &link);
	if (!link) {
		ret_val = E1000_SUCCESS;
		goto out;
	}

	for (i = 0; i < 10; i++) {
		/* read once to clear */
		ret_val = e1e_rphy(hw, IGP3_KMRN_DIAG, &data);
		if (ret_val)
			goto out;
		/* and again to get new status */
		ret_val = e1e_rphy(hw, IGP3_KMRN_DIAG, &data);
		if (ret_val)
			goto out;

		/* check for PCS lock */
		if (!(data & IGP3_KMRN_DIAG_PCS_LOCK_LOSS)) {
			ret_val = E1000_SUCCESS;
			goto out;
		}

		/* Issue PHY reset */
		e1000_phy_hw_reset(hw);
		mdelay(5);
	}
	/* Disable GigE link negotiation */
	phy_ctrl = er32(PHY_CTRL);
	phy_ctrl |= (E1000_PHY_CTRL_GBE_DISABLE |
	             E1000_PHY_CTRL_NOND0A_GBE_DISABLE);
	ew32(PHY_CTRL, phy_ctrl);

	/*
	 * Call gig speed drop workaround on Gig disable before accessing
	 * any PHY registers
	 */
	e1000e_gig_downshift_workaround_ich8lan(hw);

	/* unable to acquire PCS lock */
	ret_val = -E1000_ERR_PHY;

out:
	return ret_val;
}

/**
 *  e1000e_set_kmrn_lock_loss_workaround_ich8lan - Set Kumeran workaround state
 *  @hw: pointer to the HW structure
 *  @state: boolean value used to set the current Kumeran workaround state
 *
 *  If ICH8, set the current Kumeran workaround state (enabled - true
 *  /disabled - false).
 **/
void e1000e_set_kmrn_lock_loss_workaround_ich8lan(struct e1000_hw *hw,
                                                 bool state)
{
	struct e1000_dev_spec_ich8lan *dev_spec = &hw->dev_spec.ich8lan;

	if (hw->mac.type != e1000_ich8lan) {
		e_dbg("Workaround applies to ICH8 only.\n");
		return;
	}

	dev_spec->kmrn_lock_loss_workaround_enabled = state;

	return;
}

/**
 *  e1000_ipg3_phy_powerdown_workaround_ich8lan - Power down workaround on D3
 *  @hw: pointer to the HW structure
 *
 *  Workaround for 82566 power-down on D3 entry:
 *    1) disable gigabit link
 *    2) write VR power-down enable
 *    3) read it back
 *  Continue if successful, else issue LCD reset and repeat
 **/
void e1000e_igp3_phy_powerdown_workaround_ich8lan(struct e1000_hw *hw)
{
	u32 reg;
	u16 data;
	u8  retry = 0;

	if (hw->phy.type != e1000_phy_igp_3)
		goto out;

	/* Try the workaround twice (if needed) */
	do {
		/* Disable link */
		reg = er32(PHY_CTRL);
		reg |= (E1000_PHY_CTRL_GBE_DISABLE |
		        E1000_PHY_CTRL_NOND0A_GBE_DISABLE);
		ew32(PHY_CTRL, reg);

		/*
		 * Call gig speed drop workaround on Gig disable before
		 * accessing any PHY registers
		 */
		if (hw->mac.type == e1000_ich8lan)
			e1000e_gig_downshift_workaround_ich8lan(hw);

		/* Write VR power-down enable */
		e1e_rphy(hw, IGP3_VR_CTRL, &data);
		data &= ~IGP3_VR_CTRL_DEV_POWERDOWN_MODE_MASK;
		e1e_wphy(hw, IGP3_VR_CTRL,
		                   data | IGP3_VR_CTRL_MODE_SHUTDOWN);

		/* Read it back and test */
		e1e_rphy(hw, IGP3_VR_CTRL, &data);
		data &= IGP3_VR_CTRL_DEV_POWERDOWN_MODE_MASK;
		if ((data == IGP3_VR_CTRL_MODE_SHUTDOWN) || retry)
			break;

		/* Issue PHY reset and repeat at most one more time */
		reg = er32(CTRL);
		ew32(CTRL, reg | E1000_CTRL_PHY_RST);
		retry++;
	} while (retry);

out:
	return;
}

/**
 *  e1000e_gig_downshift_workaround_ich8lan - WoL from S5 stops working
 *  @hw: pointer to the HW structure
 *
 *  Steps to take when dropping from 1Gb/s (eg. link cable removal (LSC),
 *  LPLU, Gig disable, MDIC PHY reset):
 *    1) Set Kumeran Near-end loopback
 *    2) Clear Kumeran Near-end loopback
 *  Should only be called for ICH8[m] devices with IGP_3 Phy.
 **/
void e1000e_gig_downshift_workaround_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;
	u16 reg_data;

	if ((hw->mac.type != e1000_ich8lan) ||
	    (hw->phy.type != e1000_phy_igp_3))
		goto out;

	ret_val = e1000e_read_kmrn_reg(hw, E1000_KMRNCTRLSTA_DIAG_OFFSET,
	                                      &reg_data);
	if (ret_val)
		goto out;
	reg_data |= E1000_KMRNCTRLSTA_DIAG_NELPBK;
	ret_val = e1000e_write_kmrn_reg(hw,
	                                       E1000_KMRNCTRLSTA_DIAG_OFFSET,
	                                       reg_data);
	if (ret_val)
		goto out;
	reg_data &= ~E1000_KMRNCTRLSTA_DIAG_NELPBK;
	ret_val = e1000e_write_kmrn_reg(hw,
	                                       E1000_KMRNCTRLSTA_DIAG_OFFSET,
	                                       reg_data);
out:
	return;
}

/**
 *  e1000e_disable_gig_wol_ich8lan - disable gig during WoL
 *  @hw: pointer to the HW structure
 *
 *  During S0 to Sx transition, it is possible the link remains at gig
 *  instead of negotiating to a lower speed.  Before going to Sx, set
 *  'LPLU Enabled' and 'Gig Disable' to force link speed negotiation
 *  to a lower speed.
 *
 *  Should only be called for applicable parts.
 **/
void e1000e_disable_gig_wol_ich8lan(struct e1000_hw *hw)
{
	u32 phy_ctrl;

	switch (hw->mac.type) {
	case e1000_ich9lan:
	case e1000_ich10lan:
	case e1000_pchlan:
		phy_ctrl = er32(PHY_CTRL);
		phy_ctrl |= E1000_PHY_CTRL_D0A_LPLU |
		            E1000_PHY_CTRL_GBE_DISABLE;
		ew32(PHY_CTRL, phy_ctrl);

		/* Workaround SWFLAG unexpectedly set during S0->Sx */
		if (hw->mac.type == e1000_pchlan)
			udelay(500);
	default:
		break;
	}

	return;
}

/**
 *  e1000_cleanup_led_ich8lan - Restore the default LED operation
 *  @hw: pointer to the HW structure
 *
 *  Return the LED back to the default configuration.
 **/
static s32 e1000_cleanup_led_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;

	if (hw->phy.type == e1000_phy_ife)
		ret_val = e1e_wphy(hw, IFE_PHY_SPECIAL_CONTROL_LED,
		                              0);
	else
		ew32(LEDCTL, hw->mac.ledctl_default);

	return ret_val;
}

/**
 *  e1000_led_on_ich8lan - Turn LEDs on
 *  @hw: pointer to the HW structure
 *
 *  Turn on the LEDs.
 **/
static s32 e1000_led_on_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;

	if (hw->phy.type == e1000_phy_ife)
		ret_val = e1e_wphy(hw, IFE_PHY_SPECIAL_CONTROL_LED,
		                (IFE_PSCL_PROBE_MODE | IFE_PSCL_PROBE_LEDS_ON));
	else
		ew32(LEDCTL, hw->mac.ledctl_mode2);

	return ret_val;
}

/**
 *  e1000_led_off_ich8lan - Turn LEDs off
 *  @hw: pointer to the HW structure
 *
 *  Turn off the LEDs.
 **/
static s32 e1000_led_off_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;

	if (hw->phy.type == e1000_phy_ife)
		ret_val = e1e_wphy(hw,
		               IFE_PHY_SPECIAL_CONTROL_LED,
		               (IFE_PSCL_PROBE_MODE | IFE_PSCL_PROBE_LEDS_OFF));
	else
		ew32(LEDCTL, hw->mac.ledctl_mode1);

	return ret_val;
}

/**
 *  e1000_setup_led_pchlan - Configures SW controllable LED
 *  @hw: pointer to the HW structure
 *
 *  This prepares the SW controllable LED for use.
 **/
static s32 e1000_setup_led_pchlan(struct e1000_hw *hw)
{
	return e1e_wphy(hw, HV_LED_CONFIG,
					(u16)hw->mac.ledctl_mode1);
}

/**
 *  e1000_cleanup_led_pchlan - Restore the default LED operation
 *  @hw: pointer to the HW structure
 *
 *  Return the LED back to the default configuration.
 **/
static s32 e1000_cleanup_led_pchlan(struct e1000_hw *hw)
{
	return e1e_wphy(hw, HV_LED_CONFIG,
					(u16)hw->mac.ledctl_default);
}

/**
 *  e1000_led_on_pchlan - Turn LEDs on
 *  @hw: pointer to the HW structure
 *
 *  Turn on the LEDs.
 **/
static s32 e1000_led_on_pchlan(struct e1000_hw *hw)
{
	u16 data = (u16)hw->mac.ledctl_mode2;
	u32 i, led;

	/*
	 * If no link, then turn LED on by setting the invert bit
	 * for each LED that's mode is "link_up" in ledctl_mode2.
	 */
	if (!(er32(STATUS) & E1000_STATUS_LU)) {
		for (i = 0; i < 3; i++) {
			led = (data >> (i * 5)) & E1000_PHY_LED0_MASK;
			if ((led & E1000_PHY_LED0_MODE_MASK) !=
			    E1000_LEDCTL_MODE_LINK_UP)
				continue;
			if (led & E1000_PHY_LED0_IVRT)
				data &= ~(E1000_PHY_LED0_IVRT << (i * 5));
			else
				data |= (E1000_PHY_LED0_IVRT << (i * 5));
		}
	}

	return e1e_wphy(hw, HV_LED_CONFIG, data);
}

/**
 *  e1000_led_off_pchlan - Turn LEDs off
 *  @hw: pointer to the HW structure
 *
 *  Turn off the LEDs.
 **/
static s32 e1000_led_off_pchlan(struct e1000_hw *hw)
{
	u16 data = (u16)hw->mac.ledctl_mode1;
	u32 i, led;

	/*
	 * If no link, then turn LED off by clearing the invert bit
	 * for each LED that's mode is "link_up" in ledctl_mode1.
	 */
	if (!(er32(STATUS) & E1000_STATUS_LU)) {
		for (i = 0; i < 3; i++) {
			led = (data >> (i * 5)) & E1000_PHY_LED0_MASK;
			if ((led & E1000_PHY_LED0_MODE_MASK) !=
			    E1000_LEDCTL_MODE_LINK_UP)
				continue;
			if (led & E1000_PHY_LED0_IVRT)
				data &= ~(E1000_PHY_LED0_IVRT << (i * 5));
			else
				data |= (E1000_PHY_LED0_IVRT << (i * 5));
		}
	}

	return e1e_wphy(hw, HV_LED_CONFIG, data);
}

/**
 *  e1000_get_cfg_done_ich8lan - Read config done bit
 *  @hw: pointer to the HW structure
 *
 *  Read the management control register for the config done bit for
 *  completion status.  NOTE: silicon which is EEPROM-less will fail trying
 *  to read the config done bit, so an error is *ONLY* logged and returns
 *  E1000_SUCCESS.  If we were to return with error, EEPROM-less silicon
 *  would not be able to be reset or change link.
 **/
static s32 e1000_get_cfg_done_ich8lan(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;
	u32 bank = 0;

	if (hw->mac.type >= e1000_pchlan) {
		u32 status = er32(STATUS);

		if (status & E1000_STATUS_PHYRA) {
			ew32(STATUS, status &
			                ~E1000_STATUS_PHYRA);
		} else
			e_dbg("PHY Reset Asserted not set - needs delay\n");
	}

	e1000e_get_cfg_done(hw);

	/* If EEPROM is not marked present, init the IGP 3 PHY manually */
	if ((hw->mac.type != e1000_ich10lan) &&
	    (hw->mac.type != e1000_pchlan)) {
		if (((er32(EECD) & E1000_EECD_PRES) == 0) &&
		    (hw->phy.type == e1000_phy_igp_3)) {
			e1000_phy_init_script_igp3(hw);
		}
	} else {
		if (e1000_valid_nvm_bank_detect_ich8lan(hw, &bank)) {
			/* Maybe we should do a basic PHY config */
			e_dbg("EEPROM not present\n");
			ret_val = -E1000_ERR_CONFIG;
		}
	}

	return ret_val;
}

/**
 * e1000_power_down_phy_copper_ich8lan - Remove link during PHY power down
 * @hw: pointer to the HW structure
 *
 * In the case of a PHY power down to save power, or to turn off link during a
 * driver unload, or wake on lan is not enabled, remove the link.
 **/
static void e1000_power_down_phy_copper_ich8lan(struct e1000_hw *hw)
{
	/* If the management interface is not enabled, then power down */
	if (!(hw->mac.ops.check_mng_mode(hw) ||
	      e1000_check_reset_block(hw)))
		e1000_power_down_phy_copper(hw);

	return;
}

/**
 *  e1000_clear_hw_cntrs_ich8lan - Clear statistical counters
 *  @hw: pointer to the HW structure
 *
 *  Clears hardware counters specific to the silicon family and calls
 *  clear_hw_cntrs_generic to clear all general purpose counters.
 **/
static void e1000_clear_hw_cntrs_ich8lan(struct e1000_hw *hw)
{
	u16 phy_data;

	e1000e_clear_hw_cntrs_base(hw);

	er32(ALGNERRC);
	er32(RXERRC);
	er32(TNCRS);
	er32(CEXTERR);
	er32(TSCTC);
	er32(TSCTFC);

	er32(MGTPRC);
	er32(MGTPDC);
	er32(MGTPTC);

	er32(IAC);
	er32(ICRXOC);

	/* Clear PHY statistics registers */
	if ((hw->phy.type == e1000_phy_82578) ||
	    (hw->phy.type == e1000_phy_82577)) {
		e1e_rphy(hw, HV_SCC_UPPER, &phy_data);
		e1e_rphy(hw, HV_SCC_LOWER, &phy_data);
		e1e_rphy(hw, HV_ECOL_UPPER, &phy_data);
		e1e_rphy(hw, HV_ECOL_LOWER, &phy_data);
		e1e_rphy(hw, HV_MCC_UPPER, &phy_data);
		e1e_rphy(hw, HV_MCC_LOWER, &phy_data);
		e1e_rphy(hw, HV_LATECOL_UPPER, &phy_data);
		e1e_rphy(hw, HV_LATECOL_LOWER, &phy_data);
		e1e_rphy(hw, HV_COLC_UPPER, &phy_data);
		e1e_rphy(hw, HV_COLC_LOWER, &phy_data);
		e1e_rphy(hw, HV_DC_UPPER, &phy_data);
		e1e_rphy(hw, HV_DC_LOWER, &phy_data);
		e1e_rphy(hw, HV_TNCRS_UPPER, &phy_data);
		e1e_rphy(hw, HV_TNCRS_LOWER, &phy_data);
	}
}

