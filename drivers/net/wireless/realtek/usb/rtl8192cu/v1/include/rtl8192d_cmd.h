#ifndef __RTL8192D_CMD_H_
#define __RTL8192D_CMD_H_


enum cmd_msg_element_id
{	
	NONE_CMDMSG_EID,
	AP_OFFLOAD_EID=0,
	SET_PWRMODE_EID=1,
	JOINBSS_RPT_EID=2,
	RSVD_PAGE_EID=3,
	RSSI_4_EID = 4,
	RSSI_SETTING_EID=5,
	MACID_CONFIG_EID=6,
	MACID_PS_MODE_EID=7,
	P2P_PS_OFFLOAD_EID=8,
	MAX_CMDMSG_EID	 
};

struct cmd_msg_parm {
	u8 eid; //element id
	u8 sz; // sz
	u8 buf[6];
};

typedef struct _SETPWRMODE_PARM{
	u8 	Mode;
	u8 	SmartPS;
	u8	BcnPassTime;	// unit: 100ms
}SETPWRMODE_PARM, *PSETPWRMODE_PARM;

typedef struct JOINBSSRPT_PARM{
	u8	OpMode;	// RT_MEDIA_STATUS
}JOINBSSRPT_PARM, *PJOINBSSRPT_PARM;

typedef struct _RSVDPAGE_LOC{
	u8 	LocProbeRsp;
	u8 	LocPsPoll;
	u8	LocNullData;
}RSVDPAGE_LOC, *PRSVDPAGE_LOC;

void rtl8192d_set_FwPwrMode_cmd(_adapter*padapter, u8 Mode);
void rtl8192d_set_FwJoinBssReport_cmd(_adapter* padapter, u8 mstatus);

// host message to firmware cmd
u8 rtl8192d_set_rssi_cmd(_adapter*padapter, u8 *param);
u8 rtl8192d_set_raid_cmd(_adapter*padapter, u32 mask, u8 arg);


#endif


