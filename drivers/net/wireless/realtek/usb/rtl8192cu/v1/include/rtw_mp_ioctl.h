#ifndef _RTW_MP_IOCTL_H_
#define _RTW_MP_IOCTL_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <mp_custom_oid.h>
#include <rtw_ioctl.h>
#include <rtw_ioctl_rtl.h>
#include <rtw_efuse.h>


#define TESTFWCMDNUMBER			1000000
#define TEST_H2CINT_WAIT_TIME		500
#define TEST_C2HINT_WAIT_TIME		500
#define HCI_TEST_SYSCFG_HWMASK		1
#define _BUSCLK_40M			(4 << 2)

//------------------------------------------------------------------------------
typedef struct CFG_DBG_MSG_STRUCT {
	u32 DebugLevel;
	u32 DebugComponent_H32;
	u32 DebugComponent_L32;
}CFG_DBG_MSG_STRUCT,*PCFG_DBG_MSG_STRUCT;

typedef struct _RW_REG {
	uint offset;
	uint width;
	u32 value;
}mp_rw_reg,RW_Reg, *pRW_Reg;

//for OID_RT_PRO_READ16_EEPROM & OID_RT_PRO_WRITE16_EEPROM
typedef struct _EEPROM_RW_PARAM {
	uint offset;
	u16 value;
}eeprom_rw_param,EEPROM_RWParam, *pEEPROM_RWParam;

typedef struct _EFUSE_ACCESS_STRUCT_ {
	u16	start_addr;
	u16	cnts;
	u8	data[0];
}EFUSE_ACCESS_STRUCT, *PEFUSE_ACCESS_STRUCT;

typedef struct _BURST_RW_REG {
	uint offset;
	uint len;
	u8 Data[256];
}burst_rw_reg,Burst_RW_Reg, *pBurst_RW_Reg;

typedef struct _USB_VendorReq{
	u8	bRequest;
	u16	wValue;
	u16	wIndex;
	u16	wLength;
	u8	u8Dir;//0:OUT, 1:IN
	u8	u8InData;
}usb_vendor_req, USB_VendorReq, *pUSB_VendorReq;

typedef struct _DR_VARIABLE_STRUCT_ {
	u8 offset;
	u32 variable;
}DR_VARIABLE_STRUCT;

int mp_start_joinbss(_adapter *padapter, NDIS_802_11_SSID *pssid);

void _irqlevel_changed_(_irqL *irqlevel, /*BOOLEAN*/unsigned char  bLower);


// oid_rtl_seg_87_11_00
NDIS_STATUS oid_rt_pro8711_join_bss_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_read_register_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_write_register_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_burst_read_register_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_burst_write_register_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_write_txcmd_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_read16_eeprom_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_write16_eeprom_hdl (struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro8711_wi_poll_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro8711_pkt_loss_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_rd_attrib_mem_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_wr_attrib_mem_hdl (struct oid_par_priv* poid_par_priv);
NDIS_STATUS  oid_rt_pro_set_rf_intfs_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_poll_rx_status_hdl(struct oid_par_priv* poid_par_priv);
// oid_rtl_seg_87_11_20
NDIS_STATUS oid_rt_pro_cfg_debug_message_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_data_rate_ex_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_basic_rate_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_read_tssi_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_power_tracking_hdl(struct oid_par_priv* poid_par_priv);
//oid_rtl_seg_87_11_50
NDIS_STATUS oid_rt_pro_qry_pwrstate_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_pwrstate_hdl(struct oid_par_priv* poid_par_priv);
//oid_rtl_seg_87_11_F0
NDIS_STATUS oid_rt_pro_h2c_set_rate_table_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_h2c_get_rate_table_hdl(struct oid_par_priv* poid_par_priv);


//oid_rtl_seg_81_80_00
NDIS_STATUS oid_rt_pro_set_data_rate_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_start_test_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_stop_test_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_channel_direct_call_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_antenna_bb_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_tx_power_control_hdl(struct oid_par_priv* poid_par_priv);
//oid_rtl_seg_81_80_20
NDIS_STATUS oid_rt_pro_query_tx_packet_sent_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_query_rx_packet_received_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_query_rx_packet_crc32_error_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_pro_reset_tx_packet_sent_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_reset_rx_packet_received_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_modulation_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_pro_set_continuous_tx_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_single_carrier_tx_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_carrier_suppression_tx_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_single_tone_tx_hdl(struct oid_par_priv* poid_par_priv);


//oid_rtl_seg_81_87
NDIS_STATUS oid_rt_pro_write_bb_reg_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_read_bb_reg_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_pro_write_rf_reg_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_read_rf_reg_hdl(struct oid_par_priv* poid_par_priv);


//oid_rtl_seg_81_85
NDIS_STATUS oid_rt_wireless_mode_hdl(struct oid_par_priv* poid_par_priv);

//oid_rtl_seg_87_12_00
NDIS_STATUS oid_rt_pro_encryption_ctrl_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_add_sta_info_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_dele_sta_info_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_query_dr_variable_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_rx_packet_type_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_pro_read_efuse_hdl(struct oid_par_priv *poid_par_priv);
NDIS_STATUS oid_rt_pro_write_efuse_hdl(struct oid_par_priv *poid_par_priv);
NDIS_STATUS oid_rt_pro_rw_efuse_pgpkt_hdl(struct oid_par_priv *poid_par_priv);
NDIS_STATUS oid_rt_get_efuse_current_size_hdl(struct oid_par_priv *poid_par_priv);
NDIS_STATUS oid_rt_pro_efuse_hdl(struct oid_par_priv *poid_par_priv);
NDIS_STATUS oid_rt_pro_efuse_map_hdl(struct oid_par_priv *poid_par_priv);

NDIS_STATUS oid_rt_set_bandwidth_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_set_crystal_cap_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_set_rx_packet_type_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_get_efuse_max_size_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_pro_set_tx_agc_offset_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_pro_set_pkt_test_mode_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_get_thermal_meter_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_reset_phy_rx_packet_count_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_get_phy_rx_packet_received_hdl(struct oid_par_priv* poid_par_priv);
NDIS_STATUS oid_rt_get_phy_rx_packet_crc32_error_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_set_power_down_hdl(struct oid_par_priv* poid_par_priv);

NDIS_STATUS oid_rt_get_power_mode_hdl(struct oid_par_priv* poid_par_priv);

#ifdef _RTL871X_MP_IOCTL_C_

const struct oid_obj_priv oid_rtl_seg_81_80_00[] =
{
	{1, &oid_null_function},			//0x00	OID_RT_PRO_RESET_DUT
	{1, &oid_rt_pro_set_data_rate_hdl},		//0x01
	{1, &oid_rt_pro_start_test_hdl},		//0x02
	{1, &oid_rt_pro_stop_test_hdl},			//0x03
	{1, &oid_null_function},			//0x04	OID_RT_PRO_SET_PREAMBLE
	{1, &oid_null_function},			//0x05	OID_RT_PRO_SET_SCRAMBLER
	{1, &oid_null_function},			//0x06	OID_RT_PRO_SET_FILTER_BB
	{1, &oid_null_function},			//0x07	OID_RT_PRO_SET_MANUAL_DIVERSITY_BB
	{1, &oid_rt_pro_set_channel_direct_call_hdl},	//0x08
	{1, &oid_null_function},			//0x09	OID_RT_PRO_SET_SLEEP_MODE_DIRECT_CALL
	{1, &oid_null_function},			//0x0A	OID_RT_PRO_SET_WAKE_MODE_DIRECT_CALL
	{1, &oid_rt_pro_set_continuous_tx_hdl},		//0x0B	OID_RT_PRO_SET_TX_CONTINUOUS_DIRECT_CALL
	{1, &oid_rt_pro_set_single_carrier_tx_hdl},	//0x0C	OID_RT_PRO_SET_SINGLE_CARRIER_TX_CONTINUOUS
	{1, &oid_null_function},			//0x0D	OID_RT_PRO_SET_TX_ANTENNA_BB
	{1, &oid_rt_pro_set_antenna_bb_hdl},		//0x0E
	{1, &oid_null_function},			//0x0F	OID_RT_PRO_SET_CR_SCRAMBLER
	{1, &oid_null_function},			//0x10	OID_RT_PRO_SET_CR_NEW_FILTER
	{1, &oid_rt_pro_set_tx_power_control_hdl},	//0x11	OID_RT_PRO_SET_TX_POWER_CONTROL
	{1, &oid_null_function},			//0x12	OID_RT_PRO_SET_CR_TX_CONFIG
	{1, &oid_null_function},			//0x13	OID_RT_PRO_GET_TX_POWER_CONTROL
	{1, &oid_null_function},			//0x14	OID_RT_PRO_GET_CR_SIGNAL_QUALITY
	{1, &oid_null_function},			//0x15	OID_RT_PRO_SET_CR_SETPOINT
	{1, &oid_null_function},			//0x16	OID_RT_PRO_SET_INTEGRATOR
	{1, &oid_null_function},			//0x17	OID_RT_PRO_SET_SIGNAL_QUALITY
	{1, &oid_null_function},			//0x18	OID_RT_PRO_GET_INTEGRATOR
	{1, &oid_null_function},			//0x19	OID_RT_PRO_GET_SIGNAL_QUALITY
	{1, &oid_null_function},			//0x1A	OID_RT_PRO_QUERY_EEPROM_TYPE
	{1, &oid_null_function},			//0x1B	OID_RT_PRO_WRITE_MAC_ADDRESS
	{1, &oid_null_function},			//0x1C	OID_RT_PRO_READ_MAC_ADDRESS
	{1, &oid_null_function},			//0x1D	OID_RT_PRO_WRITE_CIS_DATA
	{1, &oid_null_function},			//0x1E	OID_RT_PRO_READ_CIS_DATA
	{1, &oid_null_function}				//0x1F	OID_RT_PRO_WRITE_POWER_CONTROL

};

const struct oid_obj_priv oid_rtl_seg_81_80_20[] =
{
	{1, &oid_null_function},			//0x20	OID_RT_PRO_READ_POWER_CONTROL
	{1, &oid_null_function},			//0x21	OID_RT_PRO_WRITE_EEPROM
	{1, &oid_null_function},			//0x22	OID_RT_PRO_READ_EEPROM
	{1, &oid_rt_pro_reset_tx_packet_sent_hdl},	//0x23
	{1, &oid_rt_pro_query_tx_packet_sent_hdl},	//0x24
	{1, &oid_rt_pro_reset_rx_packet_received_hdl},	//0x25
	{1, &oid_rt_pro_query_rx_packet_received_hdl},	//0x26
	{1, &oid_rt_pro_query_rx_packet_crc32_error_hdl},	//0x27
	{1, &oid_null_function},			//0x28	OID_RT_PRO_QUERY_CURRENT_ADDRESS
	{1, &oid_null_function},			//0x29	OID_RT_PRO_QUERY_PERMANENT_ADDRESS
	{1, &oid_null_function},			//0x2A	OID_RT_PRO_SET_PHILIPS_RF_PARAMETERS
	{1, &oid_rt_pro_set_carrier_suppression_tx_hdl},//0x2B	OID_RT_PRO_SET_CARRIER_SUPPRESSION_TX
	{1, &oid_null_function},			//0x2C	OID_RT_PRO_RECEIVE_PACKET
	{1, &oid_null_function},			//0x2D	OID_RT_PRO_WRITE_EEPROM_BYTE
	{1, &oid_null_function},			//0x2E	OID_RT_PRO_READ_EEPROM_BYTE
	{1, &oid_rt_pro_set_modulation_hdl}		//0x2F

};

const struct oid_obj_priv oid_rtl_seg_81_80_40[] =
{
	{1, &oid_null_function},			//0x40
	{1, &oid_null_function},			//0x41
	{1, &oid_null_function},			//0x42
	{1, &oid_rt_pro_set_single_tone_tx_hdl},	//0x43
	{1, &oid_null_function},			//0x44
	{1, &oid_null_function}				//0x45
};

const struct oid_obj_priv oid_rtl_seg_81_80_80[] =
{
	{1, &oid_null_function},			//0x80	OID_RT_DRIVER_OPTION
	{1, &oid_null_function},			//0x81	OID_RT_RF_OFF
	{1, &oid_null_function}				//0x82	OID_RT_AUTH_STATUS

};

const struct oid_obj_priv oid_rtl_seg_81_85[] =
{
	{1, &oid_rt_wireless_mode_hdl}			//0x00	OID_RT_WIRELESS_MODE
};

struct oid_obj_priv oid_rtl_seg_81_87[] =
{
	{1, &oid_null_function},			//0x80	OID_RT_PRO8187_WI_POLL
	{1, &oid_rt_pro_write_bb_reg_hdl},		//0x81
	{1, &oid_rt_pro_read_bb_reg_hdl},		//0x82
	{1, &oid_rt_pro_write_rf_reg_hdl},		//0x82
	{1, &oid_rt_pro_read_rf_reg_hdl}		//0x83
};

struct oid_obj_priv oid_rtl_seg_87_11_00[] =
{
	{1, &oid_rt_pro8711_join_bss_hdl},		//0x00  //S
	{1, &oid_rt_pro_read_register_hdl},		//0x01
	{1, &oid_rt_pro_write_register_hdl},		//0x02
	{1, &oid_rt_pro_burst_read_register_hdl},	//0x03
	{1, &oid_rt_pro_burst_write_register_hdl},	//0x04
	{1, &oid_rt_pro_write_txcmd_hdl},		//0x05
	{1, &oid_rt_pro_read16_eeprom_hdl},		//0x06
	{1, &oid_rt_pro_write16_eeprom_hdl},		//0x07
	{1, &oid_null_function},			//0x08	OID_RT_PRO_H2C_SET_COMMAND
	{1, &oid_null_function},			//0x09	OID_RT_PRO_H2C_QUERY_RESULT
	{1, &oid_rt_pro8711_wi_poll_hdl},		//0x0A
	{1, &oid_rt_pro8711_pkt_loss_hdl},		//0x0B
	{1, &oid_rt_rd_attrib_mem_hdl},			//0x0C
	{1, &oid_rt_wr_attrib_mem_hdl},			//0x0D
	{1, &oid_null_function},			//0x0E
	{1, &oid_null_function},			//0x0F
	{1, &oid_null_function},			//0x10	OID_RT_PRO_H2C_CMD_MODE
	{1, &oid_null_function},			//0x11	OID_RT_PRO_H2C_CMD_RSP_MODE
	{1, &oid_null_function},			//0X12	OID_RT_PRO_WAIT_C2H_EVENT
	{1, &oid_null_function},			//0X13	OID_RT_PRO_RW_ACCESS_PROTOCOL_TEST
	{1, &oid_null_function},			//0X14	OID_RT_PRO_SCSI_ACCESS_TEST
	{1, &oid_null_function},			//0X15	OID_RT_PRO_SCSI_TCPIPOFFLOAD_OUT
	{1, &oid_null_function},			//0X16	OID_RT_PRO_SCSI_TCPIPOFFLOAD_IN
	{1, &oid_null_function},			//0X17	OID_RT_RRO_RX_PKT_VIA_IOCTRL
	{1, &oid_null_function},			//0X18	OID_RT_RRO_RX_PKTARRAY_VIA_IOCTRL
	{1, &oid_null_function},			//0X19	OID_RT_RPO_SET_PWRMGT_TEST
	{1, &oid_null_function},			//0X1A
	{1, &oid_null_function},			//0X1B	OID_RT_PRO_QRY_PWRMGT_TEST
	{1, &oid_null_function},			//0X1C	OID_RT_RPO_ASYNC_RWIO_TEST
	{1, &oid_null_function},			//0X1D	OID_RT_RPO_ASYNC_RWIO_POLL
	{1, &oid_rt_pro_set_rf_intfs_hdl},		//0X1E
	{1, &oid_rt_poll_rx_status_hdl}			//0X1F
};

struct oid_obj_priv oid_rtl_seg_87_11_20[] =
{
	{1, &oid_rt_pro_cfg_debug_message_hdl},		//0x20
	{1, &oid_rt_pro_set_data_rate_ex_hdl},		//0x21
	{1, &oid_rt_pro_set_basic_rate_hdl},		//0x22
	{1, &oid_rt_pro_read_tssi_hdl},			//0x23
	{1, &oid_rt_pro_set_power_tracking_hdl}		//0x24
};


struct oid_obj_priv oid_rtl_seg_87_11_50[] =
{
	{1, &oid_rt_pro_qry_pwrstate_hdl},		//0x50
	{1, &oid_rt_pro_set_pwrstate_hdl}		//0x51
};

struct oid_obj_priv oid_rtl_seg_87_11_80[] =
{
	{1, &oid_null_function}				//0x80
};

struct oid_obj_priv oid_rtl_seg_87_11_B0[] =
{
	{1, &oid_null_function}				//0xB0
};

struct oid_obj_priv oid_rtl_seg_87_11_F0[] =
{
	{1, &oid_null_function},			//0xF0
	{1, &oid_null_function},			//0xF1
	{1, &oid_null_function},			//0xF2
	{1, &oid_null_function},			//0xF3
	{1, &oid_null_function},			//0xF4
	{1, &oid_null_function},			//0xF5
	{1, &oid_null_function},			//0xF6
	{1, &oid_null_function},			//0xF7
	{1, &oid_null_function},			//0xF8
	{1, &oid_null_function},			//0xF9
	{1, &oid_null_function},			//0xFA
	{1, &oid_rt_pro_h2c_set_rate_table_hdl},	//0xFB
	{1, &oid_rt_pro_h2c_get_rate_table_hdl},	//0xFC
	{1, &oid_null_function},			//0xFD
	{1, &oid_null_function},			//0xFE	OID_RT_PRO_H2C_C2H_LBK_TEST
	{1, &oid_null_function}				//0xFF

};

struct oid_obj_priv oid_rtl_seg_87_12_00[]=
{
	{1, &oid_rt_pro_encryption_ctrl_hdl},		//0x00	Q&S
	{1, &oid_rt_pro_add_sta_info_hdl},		//0x01	S
	{1, &oid_rt_pro_dele_sta_info_hdl},		//0x02	S
	{1, &oid_rt_pro_query_dr_variable_hdl},		//0x03	Q
	{1, &oid_rt_pro_rx_packet_type_hdl},		//0x04	Q,S
	{1, &oid_rt_pro_read_efuse_hdl},		//0x05	Q	OID_RT_PRO_READ_EFUSE
	{1, &oid_rt_pro_write_efuse_hdl},		//0x06	S	OID_RT_PRO_WRITE_EFUSE
	{1, &oid_rt_pro_rw_efuse_pgpkt_hdl},		//0x07	Q,S
	{1, &oid_rt_get_efuse_current_size_hdl},	//0x08 	Q
	{1, &oid_rt_set_bandwidth_hdl},			//0x09
	{1, &oid_rt_set_crystal_cap_hdl},		//0x0a
	{1, &oid_rt_set_rx_packet_type_hdl},		//0x0b	S
	{1, &oid_rt_get_efuse_max_size_hdl},		//0x0c
	{1, &oid_rt_pro_set_tx_agc_offset_hdl},		//0x0d
	{1, &oid_rt_pro_set_pkt_test_mode_hdl},		//0x0e
	{1, &oid_null_function},			//0x0f		OID_RT_PRO_FOR_EVM_TEST_SETTING
	{1, &oid_rt_get_thermal_meter_hdl},		//0x10	Q	OID_RT_PRO_GET_THERMAL_METER
	{1, &oid_rt_reset_phy_rx_packet_count_hdl},	//0x11	S	OID_RT_RESET_PHY_RX_PACKET_COUNT
	{1, &oid_rt_get_phy_rx_packet_received_hdl},	//0x12	Q	OID_RT_GET_PHY_RX_PACKET_RECEIVED
	{1, &oid_rt_get_phy_rx_packet_crc32_error_hdl},	//0x13	Q	OID_RT_GET_PHY_RX_PACKET_CRC32_ERROR
	{1, &oid_rt_set_power_down_hdl},		//0x14	Q	OID_RT_SET_POWER_DOWN
	{1, &oid_rt_get_power_mode_hdl}			//0x15	Q	OID_RT_GET_POWER_MODE
};

#else /* _RTL871X_MP_IOCTL_C_ */

extern struct oid_obj_priv oid_rtl_seg_81_80_00[32];
extern struct oid_obj_priv oid_rtl_seg_81_80_20[16];
extern struct oid_obj_priv oid_rtl_seg_81_80_40[6];
extern struct oid_obj_priv oid_rtl_seg_81_80_80[3];

extern struct oid_obj_priv oid_rtl_seg_81_85[1];
extern struct oid_obj_priv oid_rtl_seg_81_87[5];

extern struct oid_obj_priv oid_rtl_seg_87_11_00[32];
extern struct oid_obj_priv oid_rtl_seg_87_11_20[5];
extern struct oid_obj_priv oid_rtl_seg_87_11_50[2];
extern struct oid_obj_priv oid_rtl_seg_87_11_80[1];
extern struct oid_obj_priv oid_rtl_seg_87_11_B0[1];
extern struct oid_obj_priv oid_rtl_seg_87_11_F0[16];

extern struct oid_obj_priv oid_rtl_seg_87_12_00[32];

#endif /* _RTL871X_MP_IOCTL_C_ */


enum MP_MODE {
	MP_START_MODE,
	MP_STOP_MODE,
	MP_ERR_MODE
};

struct rwreg_param{
	unsigned int offset;
	unsigned int width;
	unsigned int value;
};

struct bbreg_param{
	unsigned int offset;
	unsigned int phymask;
	unsigned int value;
};
/*
struct rfchannel_param{
	unsigned int ch;
	unsigned int modem;
};
*/
struct txpower_param{
	unsigned int pwr_index;
};


struct datarate_param{
	unsigned int rate_index;
};


struct rfintfs_parm {
	unsigned int rfintfs;
};

struct mp_xmit_packet {
	unsigned int len;
	unsigned int mem[MAX_MP_XMITBUF_SZ >> 2];
};

struct psmode_param {
	unsigned int ps_mode;
	unsigned int smart_ps;
};

//for OID_RT_PRO_READ16_EEPROM & OID_RT_PRO_WRITE16_EEPROM
struct eeprom_rw_param {
	unsigned int offset;
	unsigned short value;
};

struct mp_ioctl_handler {
	unsigned int paramsize;
	unsigned int (*handler)(struct oid_par_priv* poid_par_priv);
	unsigned int oid;
};

struct mp_ioctl_param{
	unsigned int subcode;
	unsigned int len;
	unsigned char data[0];
};

#define GEN_MP_IOCTL_SUBCODE(code) _MP_IOCTL_ ## code ## _CMD_

enum RTL871X_MP_IOCTL_SUBCODE {
	GEN_MP_IOCTL_SUBCODE(MP_START), 		/*0*/
	GEN_MP_IOCTL_SUBCODE(MP_STOP), 			/*1*/
	GEN_MP_IOCTL_SUBCODE(READ_REG), 		/*2*/
	GEN_MP_IOCTL_SUBCODE(WRITE_REG),
	GEN_MP_IOCTL_SUBCODE(SET_CHANNEL),		/*4*/
	GEN_MP_IOCTL_SUBCODE(SET_TXPOWER),		/*5*/
	GEN_MP_IOCTL_SUBCODE(SET_DATARATE),		/*6*/
	GEN_MP_IOCTL_SUBCODE(READ_BB_REG),		/*7*/
	GEN_MP_IOCTL_SUBCODE(WRITE_BB_REG),
	GEN_MP_IOCTL_SUBCODE(READ_RF_REG),		/*9*/
	GEN_MP_IOCTL_SUBCODE(WRITE_RF_REG),
	GEN_MP_IOCTL_SUBCODE(SET_RF_INTFS),
	GEN_MP_IOCTL_SUBCODE(IOCTL_XMIT_PACKET),	/*12*/
	GEN_MP_IOCTL_SUBCODE(PS_STATE),			/*13*/
	GEN_MP_IOCTL_SUBCODE(READ16_EEPROM),		/*14*/
	GEN_MP_IOCTL_SUBCODE(WRITE16_EEPROM),		/*15*/
	GEN_MP_IOCTL_SUBCODE(SET_PTM),			/*16*/
	GEN_MP_IOCTL_SUBCODE(READ_TSSI),		/*17*/
	GEN_MP_IOCTL_SUBCODE(CNTU_TX),			/*18*/
	GEN_MP_IOCTL_SUBCODE(SET_BANDWIDTH),		/*19*/
	GEN_MP_IOCTL_SUBCODE(SET_RX_PKT_TYPE),		/*20*/
	GEN_MP_IOCTL_SUBCODE(RESET_PHY_RX_PKT_CNT),	/*21*/
	GEN_MP_IOCTL_SUBCODE(GET_PHY_RX_PKT_RECV),	/*22*/
	GEN_MP_IOCTL_SUBCODE(GET_PHY_RX_PKT_ERROR),	/*23*/
	GEN_MP_IOCTL_SUBCODE(SET_POWER_DOWN),		/*24*/
	GEN_MP_IOCTL_SUBCODE(GET_THERMAL_METER),	/*25*/
	GEN_MP_IOCTL_SUBCODE(GET_POWER_MODE),		/*26*/
	GEN_MP_IOCTL_SUBCODE(EFUSE),			/*27*/
	GEN_MP_IOCTL_SUBCODE(EFUSE_MAP),		/*28*/
	GEN_MP_IOCTL_SUBCODE(GET_EFUSE_MAX_SIZE),	/*29*/
	GEN_MP_IOCTL_SUBCODE(GET_EFUSE_CURRENT_SIZE),	/*30*/
	GEN_MP_IOCTL_SUBCODE(SC_TX),			/*31*/
	GEN_MP_IOCTL_SUBCODE(CS_TX),			/*32*/
	GEN_MP_IOCTL_SUBCODE(ST_TX),			/*33*/
	GEN_MP_IOCTL_SUBCODE(TRIGGER_GPIO),		/*34*/
	GEN_MP_IOCTL_SUBCODE(SET_DM_BT),		/*35*/
	GEN_MP_IOCTL_SUBCODE(DEL_BA),			/*36*/
	MAX_MP_IOCTL_SUBCODE,
};

unsigned int mp_ioctl_xmit_packet_hdl(struct oid_par_priv* poid_par_priv);

#ifdef _RTL871X_MP_IOCTL_C_

#define GEN_MP_IOCTL_HANDLER(sz, hdl, oid) {sz, hdl, oid},

#define EXT_MP_IOCTL_HANDLER(sz, subcode, oid) {sz, &mp_ioctl_ ## subcode ## _hdl, oid},


struct mp_ioctl_handler mp_ioctl_hdl[] = {

	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_start_test_hdl, OID_RT_PRO_START_TEST)/*0*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_stop_test_hdl, OID_RT_PRO_STOP_TEST)/*1*/

	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), oid_rt_pro_read_register_hdl, OID_RT_PRO_READ_REGISTER)/*2*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), oid_rt_pro_write_register_hdl, OID_RT_PRO_WRITE_REGISTER)
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_set_channel_direct_call_hdl, OID_RT_PRO_SET_CHANNEL_DIRECT_CALL)
	GEN_MP_IOCTL_HANDLER(sizeof(struct txpower_param), oid_rt_pro_set_tx_power_control_hdl, OID_RT_PRO_SET_TX_POWER_CONTROL)
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_set_data_rate_hdl, OID_RT_PRO_SET_DATA_RATE)

	GEN_MP_IOCTL_HANDLER(sizeof(struct bb_reg_param), oid_rt_pro_read_bb_reg_hdl, OID_RT_PRO_READ_BB_REG)/*7*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct bb_reg_param), oid_rt_pro_write_bb_reg_hdl, OID_RT_PRO_WRITE_BB_REG)

	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), oid_rt_pro_read_rf_reg_hdl, OID_RT_PRO_RF_READ_REGISTRY)/*9*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), oid_rt_pro_write_rf_reg_hdl, OID_RT_PRO_RF_WRITE_REGISTRY)

	GEN_MP_IOCTL_HANDLER(sizeof(struct rfintfs_parm), NULL, 0)

	EXT_MP_IOCTL_HANDLER(0, xmit_packet, 0)/*12*/

	GEN_MP_IOCTL_HANDLER(sizeof(struct psmode_param), NULL, 0)/*13*/

	GEN_MP_IOCTL_HANDLER(sizeof(struct eeprom_rw_param), NULL, 0)/*14*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct eeprom_rw_param), NULL, 0)/*15*/

	GEN_MP_IOCTL_HANDLER(sizeof(unsigned char), NULL, 0)/*16*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), NULL, 0)/*17*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_set_continuous_tx_hdl, OID_RT_PRO_SET_CONTINUOUS_TX)/*18*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_set_bandwidth_hdl, OID_RT_SET_BANDWIDTH)/*19*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_set_rx_packet_type_hdl, OID_RT_SET_RX_PACKET_TYPE)/*20*/
	GEN_MP_IOCTL_HANDLER(0, oid_rt_reset_phy_rx_packet_count_hdl, OID_RT_RESET_PHY_RX_PACKET_COUNT)/*21*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_get_phy_rx_packet_received_hdl, OID_RT_GET_PHY_RX_PACKET_RECEIVED)/*22*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_get_phy_rx_packet_crc32_error_hdl, OID_RT_GET_PHY_RX_PACKET_CRC32_ERROR)/*23*/
	GEN_MP_IOCTL_HANDLER(sizeof(unsigned char), oid_rt_set_power_down_hdl, OID_RT_SET_POWER_DOWN)/*24*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_get_thermal_meter_hdl, OID_RT_PRO_GET_THERMAL_METER)/*25*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_get_power_mode_hdl, OID_RT_GET_POWER_MODE)/*26*/
	GEN_MP_IOCTL_HANDLER(sizeof(EFUSE_ACCESS_STRUCT), oid_rt_pro_efuse_hdl, OID_RT_PRO_EFUSE)/*27*/
	GEN_MP_IOCTL_HANDLER(EFUSE_MAP_MAX_SIZE, oid_rt_pro_efuse_map_hdl, OID_RT_PRO_EFUSE_MAP)/*28*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_get_efuse_max_size_hdl, OID_RT_GET_EFUSE_MAX_SIZE)/*29*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_get_efuse_current_size_hdl, OID_RT_GET_EFUSE_CURRENT_SIZE)/*30*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_set_single_carrier_tx_hdl, OID_RT_PRO_SET_SINGLE_CARRIER_TX)/*31*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_set_carrier_suppression_tx_hdl, OID_RT_PRO_SET_CARRIER_SUPPRESSION_TX)/*32*/
	GEN_MP_IOCTL_HANDLER(sizeof(u32), oid_rt_pro_set_single_tone_tx_hdl, OID_RT_PRO_SET_SINGLE_TONE_TX)/*33*/
	GEN_MP_IOCTL_HANDLER(0, oid_rt_pro_trigger_gpio_hdl, 0)/*34*/
};

#else /* _RTL871X_MP_IOCTL_C_ */

extern struct mp_ioctl_handler mp_ioctl_hdl[];

#endif /* _RTL871X_MP_IOCTL_C_ */

#endif

