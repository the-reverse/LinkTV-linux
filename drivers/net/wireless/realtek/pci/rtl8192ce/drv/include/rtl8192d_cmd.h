#ifndef __RTL8192D_CMD_H_
#define __RTL8192D_CMD_H_


//--------------------------------------------
//3				Host Message Box 
//--------------------------------------------

// User Define Message [31:8]

//_SETPWRMODE_PARM
#define SET_H2CCMD_PWRMODE_PARM_MODE(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE(__pH2CCmd, 0, 8, __Value)
#define SET_H2CCMD_PWRMODE_PARM_SMART_PS(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_PWRMODE_PARM_BCN_PASS_TIME(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)

//JOINBSSRPT_PARM
#define SET_H2CCMD_JOINBSSRPT_PARM_OPMODE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE(__pH2CCmd, 0, 8, __Value)

//_RSVDPAGE_LOC
#define SET_H2CCMD_RSVDPAGE_LOC_PROBE_RSP(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE(__pH2CCmd, 0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_LOC_PSPOLL(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_LOC_NULL_DATA(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)


// Description: Determine the types of H2C commands that are the same in driver and Fw.
// Fisrt constructed by tynli. 2009.10.09.
typedef enum _RTL8192D_H2C_CMD 
{
	H2C_AP_OFFLOAD = 0,		/*0*/
	H2C_SETPWRMODE = 1,		/*1*/
	H2C_JOINBSSRPT = 2,		/*2*/
	H2C_RSVDPAGE = 3,
	H2C_RSSI_REPORT = 5,
	H2C_RA_MASK = 6,
	H2C_MAC_MODE_SEL = 9,
	H2C_PWRM=15,
	H2C_CMD_MAX
}RTL8192D_H2C_CMD;

struct cmd_msg_parm {
	u8 eid; //element id
	u8 sz; // sz
	u8 buf[6];
};

void FillH2CCmd92D(_adapter* padapter, u8 ElementID, u32 CmdLen, u8* pCmdBuffer);

void rtl8192d_set_FwPwrMode_cmd(_adapter*padapter, u8 Mode);
void rtl8192d_set_FwJoinBssReport_cmd(_adapter* padapter, u8 mstatus);

#endif


