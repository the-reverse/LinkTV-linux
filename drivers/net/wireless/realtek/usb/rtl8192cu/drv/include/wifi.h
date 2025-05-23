#ifndef _WIFI_H_
#define _WIFI_H_

#include <rtw_byteorder.h>

#ifdef BIT
//#error	"BIT define occurred earlier elsewhere!\n"
#undef BIT
#endif
#define BIT(x)	(1 << (x))


#define WLAN_ETHHDR_LEN		14
#define WLAN_ETHADDR_LEN	6
#define WLAN_IEEE_OUI_LEN	3
#define WLAN_ADDR_LEN		6
#define WLAN_CRC_LEN		4
#define WLAN_BSSID_LEN		6
#define WLAN_BSS_TS_LEN		8
#define WLAN_HDR_A3_LEN		24
#define WLAN_HDR_A4_LEN		30
#define WLAN_HDR_A3_QOS_LEN	26
#define WLAN_HDR_A4_QOS_LEN	32
#define WLAN_SSID_MAXLEN	32
#define WLAN_DATA_MAXLEN	2312

#define WLAN_A3_PN_OFFSET	24
#define WLAN_A4_PN_OFFSET	30

#define WLAN_MIN_ETHFRM_LEN	60
#define WLAN_MAX_ETHFRM_LEN	1514
#define WLAN_ETHHDR_LEN		14

#define P80211CAPTURE_VERSION	0x80211001

#ifdef GREEN_HILL
#pragma pack(1)
#endif

enum WIFI_FRAME_TYPE {
	WIFI_MGT_TYPE  =	(0),
	WIFI_CTRL_TYPE =	(BIT(2)),
	WIFI_DATA_TYPE =	(BIT(3)),
	WIFI_QOS_DATA_TYPE	= (BIT(7)|BIT(3)),	//!< QoS Data	
};

enum WIFI_FRAME_SUBTYPE {

    // below is for mgt frame
    WIFI_ASSOCREQ       = (0 | WIFI_MGT_TYPE),
    WIFI_ASSOCRSP       = (BIT(4) | WIFI_MGT_TYPE),
    WIFI_REASSOCREQ     = (BIT(5) | WIFI_MGT_TYPE),
    WIFI_REASSOCRSP     = (BIT(5) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_PROBEREQ       = (BIT(6) | WIFI_MGT_TYPE),
    WIFI_PROBERSP       = (BIT(6) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_BEACON         = (BIT(7) | WIFI_MGT_TYPE),
    WIFI_ATIM           = (BIT(7) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_DISASSOC       = (BIT(7) | BIT(5) | WIFI_MGT_TYPE),
    WIFI_AUTH           = (BIT(7) | BIT(5) | BIT(4) | WIFI_MGT_TYPE),
    WIFI_DEAUTH         = (BIT(7) | BIT(6) | WIFI_MGT_TYPE),
    WIFI_ACTION         = (BIT(7) | BIT(6) | BIT(4) | WIFI_MGT_TYPE),

    // below is for control frame
    WIFI_PSPOLL         = (BIT(7) | BIT(5) | WIFI_CTRL_TYPE),
    WIFI_RTS            = (BIT(7) | BIT(5) | BIT(4) | WIFI_CTRL_TYPE),
    WIFI_CTS            = (BIT(7) | BIT(6) | WIFI_CTRL_TYPE),
    WIFI_ACK            = (BIT(7) | BIT(6) | BIT(4) | WIFI_CTRL_TYPE),
    WIFI_CFEND          = (BIT(7) | BIT(6) | BIT(5) | WIFI_CTRL_TYPE),
    WIFI_CFEND_CFACK    = (BIT(7) | BIT(6) | BIT(5) | BIT(4) | WIFI_CTRL_TYPE),

    // below is for data frame
    WIFI_DATA           = (0 | WIFI_DATA_TYPE),
    WIFI_DATA_CFACK     = (BIT(4) | WIFI_DATA_TYPE),
    WIFI_DATA_CFPOLL    = (BIT(5) | WIFI_DATA_TYPE),
    WIFI_DATA_CFACKPOLL = (BIT(5) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_DATA_NULL      = (BIT(6) | WIFI_DATA_TYPE),
    WIFI_CF_ACK         = (BIT(6) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_CF_POLL        = (BIT(6) | BIT(5) | WIFI_DATA_TYPE),
    WIFI_CF_ACKPOLL     = (BIT(6) | BIT(5) | BIT(4) | WIFI_DATA_TYPE),
    WIFI_QOS_DATA_NULL	= (BIT(6) | WIFI_QOS_DATA_TYPE),
};

enum WIFI_REASON_CODE	{
	_RSON_RESERVED_					= 0,
	_RSON_UNSPECIFIED_				= 1,
	_RSON_AUTH_NO_LONGER_VALID_		= 2,
	_RSON_DEAUTH_STA_LEAVING_		= 3,
	_RSON_INACTIVITY_				= 4,
	_RSON_UNABLE_HANDLE_			= 5,
	_RSON_CLS2_						= 6,
	_RSON_CLS3_						= 7,
	_RSON_DISAOC_STA_LEAVING_		= 8,
	_RSON_ASOC_NOT_AUTH_			= 9,

	// WPA reason
	_RSON_INVALID_IE_				= 13,
	_RSON_MIC_FAILURE_				= 14,
	_RSON_4WAY_HNDSHK_TIMEOUT_		= 15,
	_RSON_GROUP_KEY_UPDATE_TIMEOUT_	= 16,
	_RSON_DIFF_IE_					= 17,
	_RSON_MLTCST_CIPHER_NOT_VALID_	= 18,
	_RSON_UNICST_CIPHER_NOT_VALID_	= 19,
	_RSON_AKMP_NOT_VALID_			= 20,
	_RSON_UNSUPPORT_RSNE_VER_		= 21,
	_RSON_INVALID_RSNE_CAP_			= 22,
	_RSON_IEEE_802DOT1X_AUTH_FAIL_	= 23,

	//belowing are Realtek definition
	_RSON_PMK_NOT_AVAILABLE_		= 24,
};

enum WIFI_STATUS_CODE {
	_STATS_SUCCESSFUL_			= 0,
	_STATS_FAILURE_				= 1,
	_STATS_CAP_FAIL_			= 10,
	_STATS_NO_ASOC_				= 11,
	_STATS_OTHER_				= 12,
	_STATS_NO_SUPP_ALG_			= 13,
	_STATS_OUT_OF_AUTH_SEQ_		= 14,
	_STATS_CHALLENGE_FAIL_		= 15,
	_STATS_AUTH_TIMEOUT_		= 16,
	_STATS_UNABLE_HANDLE_STA_	= 17,
	_STATS_RATE_FAIL_			= 18,
};

enum WIFI_REG_DOMAIN {
	DOMAIN_FCC		= 1,
	DOMAIN_IC		= 2,
	DOMAIN_ETSI		= 3,
	DOMAIN_SPAIN	= 4,
	DOMAIN_FRANCE	= 5,
	DOMAIN_MKK		= 6,
	DOMAIN_ISRAEL	= 7,
	DOMAIN_MKK1		= 8,
	DOMAIN_MKK2		= 9,
	DOMAIN_MKK3		= 10,
	DOMAIN_MAX
};

#define _TO_DS_		BIT(8)
#define _FROM_DS_	BIT(9)
#define _MORE_FRAG_	BIT(10)
#define _RETRY_		BIT(11)
#define _PWRMGT_	BIT(12)
#define _MORE_DATA_	BIT(13)
#define _PRIVACY_	BIT(14)
#define _ORDER_			BIT(15)

#define SetToDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_TO_DS_); \
	} while(0)

#define GetToDs(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_TO_DS_)) != 0)

#define ClearToDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_TO_DS_)); \
	} while(0)

#define SetFrDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_FROM_DS_); \
	} while(0)

#define GetFrDs(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_FROM_DS_)) != 0)

#define ClearFrDs(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_FROM_DS_)); \
	} while(0)

#define get_tofr_ds(pframe)	((GetToDs(pframe) << 1) | GetFrDs(pframe))


#define SetMFrag(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_MORE_FRAG_); \
	} while(0)

#define GetMFrag(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_MORE_FRAG_)) != 0)

#define ClearMFrag(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_MORE_FRAG_)); \
	} while(0)

#define SetRetry(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_RETRY_); \
	} while(0)

#define GetRetry(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_RETRY_)) != 0)

#define ClearRetry(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_RETRY_)); \
	} while(0)

#define SetPwrMgt(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_PWRMGT_); \
	} while(0)

#define GetPwrMgt(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_PWRMGT_)) != 0)

#define ClearPwrMgt(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_PWRMGT_)); \
	} while(0)

#define SetMData(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_MORE_DATA_); \
	} while(0)

#define GetMData(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_MORE_DATA_)) != 0)

#define ClearMData(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_MORE_DATA_)); \
	} while(0)

#define SetPrivacy(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_PRIVACY_); \
	} while(0)

#define GetPrivacy(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_PRIVACY_)) != 0)

#define ClearPrivacy(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) &= (~cpu_to_le16(_PRIVACY_)); \
	} while(0)


#define GetOrder(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_ORDER_)) != 0)

#define GetFrameType(pbuf)	(le16_to_cpu(*(unsigned short *)(pbuf)) & (BIT(3) | BIT(2)))

#define SetFrameType(pbuf,type)	\
	do { 	\
		*(unsigned short *)(pbuf) &= __constant_cpu_to_le16(~(BIT(3) | BIT(2))); \
		*(unsigned short *)(pbuf) |= __constant_cpu_to_le16(type); \
	} while(0)

#define GetFrameSubType(pbuf)	(cpu_to_le16(*(unsigned short *)(pbuf)) & (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2)))

#define SetFrameSubType(pbuf,type) \
	do {    \
		*(unsigned short *)(pbuf) &= cpu_to_le16(~(BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2))); \
		*(unsigned short *)(pbuf) |= cpu_to_le16(type); \
	} while(0)

#define GetSequence(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)) >> 4)

#define GetFragNum(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)) & 0x0f)

#define GetTupleCache(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 22)))

#define SetFragNum(pbuf, num) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 22) = \
			((*(unsigned short *)((unsigned int)(pbuf) + 22)) & le16_to_cpu(~(0x000f))) | \
			cpu_to_le16(0x0f & (num));     \
	} while(0)

#define SetSeqNum(pbuf, num) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 22) = \
			((*(unsigned short *)((unsigned int)(pbuf) + 22)) & le16_to_cpu((unsigned short)~0xfff0)) | \
			le16_to_cpu((unsigned short)(0xfff0 & (num << 4))); \
	} while(0)

#define SetDuration(pbuf, dur) \
	do {    \
		*(unsigned short *)((unsigned int)(pbuf) + 2) = cpu_to_le16(0xffff & (dur)); \
	} while(0)


#define SetPriority(pbuf, tid)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(tid & 0xf); \
	} while(0)

#define GetPriority(pbuf)	((le16_to_cpu(*(unsigned short *)(pbuf))) & 0xf)

#define SetEOSP(pbuf, eosp)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16( (eosp & 1) << 4); \
	} while(0)

#define SetAckpolicy(pbuf, ack)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16( (ack & 3) << 5); \
	} while(0)

#define GetAckpolicy(pbuf) (((le16_to_cpu(*(unsigned short *)pbuf)) >> 5) & 0x3)

#define GetAMsdu(pbuf) (((le16_to_cpu(*(unsigned short *)pbuf)) >> 7) & 0x1)

#define SetAMsdu(pbuf, amsdu)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16( (amsdu & 1) << 7); \
	} while(0)	

#define GetAid(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + 2)) & 0x3fff)

#define GetTid(pbuf)	(cpu_to_le16(*(unsigned short *)((unsigned int)(pbuf) + (((GetToDs(pbuf)<<1)|GetFrDs(pbuf))==3?30:24))) & 0x000f)

#define GetAddr1Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 4))

#define GetAddr2Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 10))

#define GetAddr3Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 16))

#define GetAddr4Ptr(pbuf)	((unsigned char *)((unsigned int)(pbuf) + 24))

#define MacAddr_isBcst(addr) \
( \
	( (addr[0] == 0xff) && (addr[1] == 0xff) && \
		(addr[2] == 0xff) && (addr[3] == 0xff) && \
		(addr[4] == 0xff) && (addr[5] == 0xff) )  ? _TRUE : _FALSE \
)

__inline static int IS_MCAST(unsigned char *da)
{
	if ((*da) & 0x01)
		return _TRUE;
	else
		return _FALSE;
}


__inline static unsigned char * get_da(unsigned char *pframe)
{
	unsigned char 	*da;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			da = GetAddr1Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			da = GetAddr1Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			da = GetAddr3Ptr(pframe);
			break;
		default:	// ToDs=1, FromDs=1
			da = GetAddr3Ptr(pframe);
			break;
	}

	return da;
}


__inline static unsigned char * get_sa(unsigned char *pframe)
{
	unsigned char 	*sa;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			sa = GetAddr2Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			sa = GetAddr3Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			sa = GetAddr2Ptr(pframe);
			break;
		default:	// ToDs=1, FromDs=1
			sa = GetAddr4Ptr(pframe);
			break;
	}

	return sa;
}

__inline static unsigned char * get_hdr_bssid(unsigned char *pframe)
{
	unsigned char 	*sa;
	unsigned int	to_fr_ds	= (GetToDs(pframe) << 1) | GetFrDs(pframe);

	switch (to_fr_ds) {
		case 0x00:	// ToDs=0, FromDs=0
			sa = GetAddr3Ptr(pframe);
			break;
		case 0x01:	// ToDs=0, FromDs=1
			sa = GetAddr2Ptr(pframe);
			break;
		case 0x02:	// ToDs=1, FromDs=0
			sa = GetAddr1Ptr(pframe);
			break;
		case 0x03:	// ToDs=1, FromDs=1
			sa = GetAddr1Ptr(pframe);
			break;
		default:	
			sa =NULL; //???????
			break;
	}

	return sa;
}


__inline static int IsFrameTypeCtrl(unsigned char *pframe)
{
	if(WIFI_CTRL_TYPE == GetFrameType(pframe))
		return _TRUE;
	else
		return _FALSE;
}
/*-----------------------------------------------------------------------------
			Below is for the security related definition
------------------------------------------------------------------------------*/
#define _RESERVED_FRAME_TYPE_	0
#define _SKB_FRAME_TYPE_		2
#define _PRE_ALLOCMEM_			1
#define _PRE_ALLOCHDR_			3
#define _PRE_ALLOCLLCHDR_		4
#define _PRE_ALLOCICVHDR_		5
#define _PRE_ALLOCMICHDR_		6

#define _SIFSTIME_				((priv->pmib->dot11BssType.net_work_type&WIRELESS_11A)?16:10)
#define _ACKCTSLNG_				14	//14 bytes long, including crclng
#define _CRCLNG_				4

#define _ASOCREQ_IE_OFFSET_		4	// excluding wlan_hdr
#define	_ASOCRSP_IE_OFFSET_		6
#define _REASOCREQ_IE_OFFSET_	10
#define _REASOCRSP_IE_OFFSET_	6
#define _PROBEREQ_IE_OFFSET_	0
#define	_PROBERSP_IE_OFFSET_	12
#define _AUTH_IE_OFFSET_		6
#define _DEAUTH_IE_OFFSET_		0
#define _BEACON_IE_OFFSET_		12
#define _PUBLIC_ACTION_IE_OFFSET_	8

#define _FIXED_IE_LENGTH_			_BEACON_IE_OFFSET_

#define _SSID_IE_				0
#define _SUPPORTEDRATES_IE_	1
#define _DSSET_IE_				3
#define _TIM_IE_					5
#define _IBSS_PARA_IE_			6
#define _CHLGETXT_IE_			16
#define _RSN_IE_2_				48
#define _SSN_IE_1_					221
#define _ERPINFO_IE_			42
#define _EXT_SUPPORTEDRATES_IE_	50

#define _HT_CAPABILITY_IE_			45
#define _HT_EXTRA_INFO_IE_			61
#define _HT_ADD_INFO_IE_			61 //_HT_EXTRA_INFO_IE_

#define _VENDOR_SPECIFIC_IE_		221

#define	_RESERVED47_				47


/* ---------------------------------------------------------------------------
					Below is the fixed elements...
-----------------------------------------------------------------------------*/
#define _AUTH_ALGM_NUM_			2
#define _AUTH_SEQ_NUM_			2
#define _BEACON_ITERVAL_		2
#define _CAPABILITY_			2
#define _CURRENT_APADDR_		6
#define _LISTEN_INTERVAL_		2
#define _RSON_CODE_				2
#define _ASOC_ID_				2
#define _STATUS_CODE_			2
#define _TIMESTAMP_				8

#define AUTH_ODD_TO				0
#define AUTH_EVEN_TO			1

#define WLAN_ETHCONV_ENCAP		1
#define WLAN_ETHCONV_RFC1042	2
#define WLAN_ETHCONV_8021h		3

#define cap_ESS BIT(0)
#define cap_IBSS BIT(1)
#define cap_CFPollable BIT(2)
#define cap_CFRequest BIT(3)
#define cap_Privacy BIT(4)
#define cap_ShortPremble BIT(5)
#define cap_PBCC	BIT(6)
#define cap_ChAgility	BIT(7)
#define cap_SpecMgmt	BIT(8)
#define cap_QoS	BIT(9)
#define cap_ShortSlot	BIT(10)

/*-----------------------------------------------------------------------------
				Below is the definition for 802.11i / 802.1x
------------------------------------------------------------------------------*/
#define _IEEE8021X_MGT_			1		// WPA
#define _IEEE8021X_PSK_			2		// WPA with pre-shared key

/*
#define _NO_PRIVACY_			0
#define _WEP_40_PRIVACY_		1
#define _TKIP_PRIVACY_			2
#define _WRAP_PRIVACY_			3
#define _CCMP_PRIVACY_			4
#define _WEP_104_PRIVACY_		5
#define _WEP_WPA_MIXED_PRIVACY_ 6	// WEP + WPA
*/
				
/*-----------------------------------------------------------------------------
				Below is the definition for WMM
------------------------------------------------------------------------------*/
#define _WMM_IE_Length_				7  // for WMM STA
#define _WMM_Para_Element_Length_		24


/*-----------------------------------------------------------------------------
				Below is the definition for 802.11n 
------------------------------------------------------------------------------*/

/* block-ack parameters */
#define IEEE80211_ADDBA_PARAM_POLICY_MASK 0x0002
#define IEEE80211_ADDBA_PARAM_TID_MASK 0x003C
#define IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK 0xFFA0
#define IEEE80211_DELBA_PARAM_TID_MASK 0xF000
#define IEEE80211_DELBA_PARAM_INITIATOR_MASK 0x0800

//#ifdef CONFIG_80211N_HT

#define SetOrderBit(pbuf)	\
	do	{	\
		*(unsigned short *)(pbuf) |= cpu_to_le16(_ORDER_); \
	} while(0)

#define GetOrderBit(pbuf)	(((*(unsigned short *)(pbuf)) & le16_to_cpu(_ORDER_)) != 0)


/**
 * struct ieee80211_bar - HT Block Ack Request
 *
 * This structure refers to "HT BlockAckReq" as
 * described in 802.11n draft section 7.2.1.7.1
 */
 #if defined(PLATFORM_LINUX) || defined(CONFIG_RTL8712FW)
struct ieee80211_bar {
	unsigned short frame_control;
	unsigned short duration;
	unsigned char ra[6];
	unsigned char ta[6];
	unsigned short control;
	unsigned short start_seq_num;
} __attribute__((packed));
 #endif

/* 802.11 BAR control masks */
#define IEEE80211_BAR_CTRL_ACK_POLICY_NORMAL     0x0000
#define IEEE80211_BAR_CTRL_CBMTID_COMPRESSED_BA  0x0004


 #if defined(PLATFORM_LINUX) || defined(CONFIG_RTL8712FW)

 /**
 * struct ieee80211_ht_cap - HT capabilities
 *
 * This structure refers to "HT capabilities element" as
 * described in 802.11n draft section 7.3.2.52
 */
 
struct ieee80211_ht_cap {
	unsigned short 	cap_info;
	unsigned char 	ampdu_params_info;
	unsigned char 	supp_mcs_set[16];
	unsigned short 	extended_ht_cap_info;
	unsigned int		tx_BF_cap_info;
	unsigned char	       antenna_selection_info;
} __attribute__ ((packed));

/**
 * struct ieee80211_ht_cap - HT additional information
 *
 * This structure refers to "HT information element" as
 * described in 802.11n draft section 7.3.2.53
 */
struct ieee80211_ht_addt_info {
	unsigned char 	control_chan;
	unsigned char		ht_param;
	unsigned short	operation_mode;
	unsigned short	stbc_param;
	unsigned char		basic_set[16];
} __attribute__ ((packed));


struct HT_caps_element
{
	union
	{ 
		struct 
		{ 	
			unsigned short	HT_caps_info;
			unsigned char	AMPDU_para;
			unsigned char	MCS_rate[16];
			unsigned short	HT_ext_caps;
			unsigned int	Beamforming_caps;
			unsigned char	ASEL_caps;
		} HT_cap_element;
		unsigned char HT_cap[26];
	};
} __attribute__ ((packed));

struct HT_info_element
{
	unsigned char	primary_channel;
	unsigned char	infos[5];
	unsigned char	MCS_rate[16];
}  __attribute__ ((packed));

struct AC_param
{
	unsigned char		ACI_AIFSN;
	unsigned char		CW;
	unsigned short	TXOP_limit;
}  __attribute__ ((packed));

struct WMM_para_element
{
	unsigned char		QoS_info;
	unsigned char		reserved;
	struct AC_param	ac_param[4];
}  __attribute__ ((packed));

struct ADDBA_request
{
	unsigned char		dialog_token;
	unsigned short	BA_para_set;
	unsigned short	BA_timeout_value;
	unsigned short	BA_starting_seqctrl;
}  __attribute__ ((packed));



#endif


#ifdef PLATFORM_WINDOWS

#pragma pack(1)

struct ieee80211_ht_cap {
	unsigned short 	cap_info;
	unsigned char 	ampdu_params_info;
	unsigned char 	supp_mcs_set[16];
	unsigned short 	extended_ht_cap_info;
	unsigned int		tx_BF_cap_info;
	unsigned char	       antenna_selection_info;
};


struct ieee80211_ht_addt_info {
	unsigned char 	control_chan;
	unsigned char		ht_param;
	unsigned short	operation_mode;
	unsigned short	stbc_param;
	unsigned char		basic_set[16];
};

struct HT_caps_element
{
	union
	{ 
		struct 
		{ 	
			unsigned short	HT_caps_info;
			unsigned char	AMPDU_para;
			unsigned char	MCS_rate[16];
			unsigned short	HT_ext_caps;
			unsigned int	Beamforming_caps;
			unsigned char	ASEL_caps;
		} HT_cap_element;
		unsigned char HT_cap[26];
	};
};

struct HT_info_element
{
	unsigned char	primary_channel;
	unsigned char	infos[5];
	unsigned char	MCS_rate[16];
};

struct AC_param
{
	unsigned char		ACI_AIFSN;
	unsigned char		CW;
	unsigned short	TXOP_limit;
};

struct WMM_para_element
{
	unsigned char		QoS_info;
	unsigned char		reserved;
	struct AC_param	ac_param[4];
};

struct ADDBA_request
{
	unsigned char		dialog_token;
	unsigned short	BA_para_set;
	unsigned short	BA_timeout_value;
	unsigned short	BA_starting_seqctrl;
};


#pragma pack()

#endif


/* 802.11n HT capabilities masks */
#define IEEE80211_HT_CAP_SUP_WIDTH		0x0002
#define IEEE80211_HT_CAP_SM_PS			0x000C
#define IEEE80211_HT_CAP_GRN_FLD		0x0010
#define IEEE80211_HT_CAP_SGI_20			0x0020
#define IEEE80211_HT_CAP_SGI_40			0x0040
#define IEEE80211_HT_CAP_TX_STBC			0x0080
#define IEEE80211_HT_CAP_RX_STBC		0x0300
#define IEEE80211_HT_CAP_DELAY_BA		0x0400
#define IEEE80211_HT_CAP_MAX_AMSDU		0x0800
#define IEEE80211_HT_CAP_DSSSCCK40		0x1000
/* 802.11n HT capability AMPDU settings */
#define IEEE80211_HT_CAP_AMPDU_FACTOR		0x03
#define IEEE80211_HT_CAP_AMPDU_DENSITY		0x1C
/* 802.11n HT capability MSC set */
#define IEEE80211_SUPP_MCS_SET_UEQM		4
#define IEEE80211_HT_CAP_MAX_STREAMS		4
#define IEEE80211_SUPP_MCS_SET_LEN		10
/* maximum streams the spec allows */
#define IEEE80211_HT_CAP_MCS_TX_DEFINED		0x01
#define IEEE80211_HT_CAP_MCS_TX_RX_DIFF		0x02
#define IEEE80211_HT_CAP_MCS_TX_STREAMS		0x0C
#define IEEE80211_HT_CAP_MCS_TX_UEQM		0x10
/* 802.11n HT IE masks */
#define IEEE80211_HT_IE_CHA_SEC_OFFSET		0x03
#define IEEE80211_HT_IE_CHA_SEC_NONE	 	0x00
#define IEEE80211_HT_IE_CHA_SEC_ABOVE 		0x01
#define IEEE80211_HT_IE_CHA_SEC_BELOW 		0x03
#define IEEE80211_HT_IE_CHA_WIDTH		0x04
#define IEEE80211_HT_IE_HT_PROTECTION		0x0003
#define IEEE80211_HT_IE_NON_GF_STA_PRSNT	0x0004
#define IEEE80211_HT_IE_NON_HT_STA_PRSNT	0x0010

/* block-ack parameters */
#define IEEE80211_ADDBA_PARAM_POLICY_MASK 0x0002
#define IEEE80211_ADDBA_PARAM_TID_MASK 0x003C
#define IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK 0xFFA0
#define IEEE80211_DELBA_PARAM_TID_MASK 0xF000
#define IEEE80211_DELBA_PARAM_INITIATOR_MASK 0x0800

/*
 * A-PMDU buffer sizes
 * According to IEEE802.11n spec size varies from 8K to 64K (in powers of 2)
 */
#define IEEE80211_MIN_AMPDU_BUF 0x8
#define IEEE80211_MAX_AMPDU_BUF 0x40


/* Spatial Multiplexing Power Save Modes */
#define WLAN_HT_CAP_SM_PS_STATIC		0
#define WLAN_HT_CAP_SM_PS_DYNAMIC	1
#define WLAN_HT_CAP_SM_PS_INVALID	2
#define WLAN_HT_CAP_SM_PS_DISABLED	3


//#endif

//	===============WPS Section===============
//	For WPSv1.0
#define WPSOUI							0x0050f204
//	WPS attribute ID
#define WPS_ATTR_VER1					0x104A
#define WPS_ATTR_SIMPLE_CONF_STATE	0x1044
#define WPS_ATTR_RESP_TYPE			0x103B
#define WPS_ATTR_UUID_E				0x1047
#define WPS_ATTR_MANUFACTURER		0x1021
#define WPS_ATTR_MODEL_NAME			0x1023
#define WPS_ATTR_MODEL_NUMBER		0x1024
#define WPS_ATTR_SERIAL_NUMBER		0x1042
#define WPS_ATTR_PRIMARY_DEV_TYPE	0x1054
#define WPS_ATTR_DEVICE_NAME			0x1011
#define WPS_ATTR_CONF_METHOD			0x1008
#define WPS_ATTR_RF_BANDS				0x103C
#define WPS_ATTR_DEVICE_PWID			0x1012

//	Value of WPS attribute "WPS_ATTR_DEVICE_NAME
#define WPS_MAX_DEVICE_NAME_LEN		32

//	Value of WPS Response Type Attribute
#define WPS_RESPONSE_TYPE_INFO_ONLY	0x00
#define WPS_RESPONSE_TYPE_8021X		0x01
#define WPS_RESPONSE_TYPE_REGISTRAR	0x02
#define WPS_RESPONSE_TYPE_AP			0x03

//	Value of WPS WiFi Simple Configuration State Attribute
#define WPS_WSC_STATE_NOT_CONFIG	0x01
#define WPS_WSC_STATE_CONFIG			0x02

//	Value of WPS Version Attribute
#define WPS_VERSION_1					0x10

//	Value of WPS Configuration Method Attribute
#define WPS_CONFIG_METHOD_FLASH		0x0001
#define WPS_CONFIG_METHOD_ETHERNET	0x0002
#define WPS_CONFIG_METHOD_LABEL		0x0004
#define WPS_CONFIG_METHOD_DISPLAY	0x0008
#define WPS_CONFIG_METHOD_E_NFC		0x0010
#define WPS_CONFIG_METHOD_I_NFC		0x0020
#define WPS_CONFIG_METHOD_NFC		0x0040
#define WPS_CONFIG_METHOD_PBC		0x0080
#define WPS_CONFIG_METHOD_KEYPAD	0x0100
#define WPS_CONFIG_METHOD_VPBC		0x0280
#define WPS_CONFIG_METHOD_PPBC		0x0480
#define WPS_CONFIG_METHOD_VDISPLAY	0x2008
#define WPS_CONFIG_METHOD_PDISPLAY	0x4008

//	Value of Category ID of WPS Primary Device Type Attribute
#define WPS_PDT_CID_MULIT_MEDIA		0x0008
#define WPS_PDT_CID_RTK_WIDI			0x001E

//	Value of Sub Category ID of WPS Primary Device Type Attribute
#define WPS_PDT_SCID_MEDIA_SERVER	0x0005
#define WPS_PDT_SCID_RTK_DMP			0x0001

//	Value of Device Password ID
#define WPS_DPID_PIN					0x0000
#define WPS_DPID_USER_SPEC			0x0001
#define WPS_DPID_MACHINE_SPEC			0x0002
#define WPS_DPID_REKEY					0x0003
#define WPS_DPID_PBC					0x0004
#define WPS_DPID_REGISTRAR_SPEC		0x0005


//	=====================P2P Section=====================
//	For P2P
#define	P2POUI							0x506F9A09

//	P2P Attribute ID
#define	P2P_ATTR_STATUS					0x00
#define	P2P_ATTR_MINOR_REASON_CODE		0x01
#define	P2P_ATTR_CAPABILITY				0x02
#define	P2P_ATTR_DEVICE_ID				0x03
#define	P2P_ATTR_GO_INTENT				0x04
#define	P2P_ATTR_CONF_TIMEOUT			0x05
#define	P2P_ATTR_LISTEN_CH				0x06
#define	P2P_ATTR_GROUP_BSSID				0x07
#define	P2P_ATTR_EX_LISTEN_TIMING		0x08
#define	P2P_ATTR_INTENTED_IF_ADDR		0x09
#define	P2P_ATTR_MANAGEABILITY			0x0A
#define	P2P_ATTR_CH_LIST					0x0B
#define	P2P_ATTR_NOA						0x0C
#define	P2P_ATTR_DEVICE_INFO				0x0D
#define	P2P_ATTR_GROUP_INFO				0x0E
#define	P2P_ATTR_GROUP_ID					0x0F
#define	P2P_ATTR_INTERFACE				0x10
#define	P2P_ATTR_OPERATING_CH			0x11
#define	P2P_ATTR_INVITATION_FLAGS		0x12

//	Value of Status Attribute
#define	P2P_STATUS_SUCCESS						0x00
#define	P2P_STATUS_FAIL_INFO_UNAVAILABLE		0x01
#define	P2P_STATUS_FAIL_INCOMPATIBLE_PARAM		0x02
#define	P2P_STATUS_FAIL_LIMIT_REACHED			0x03
#define	P2P_STATUS_FAIL_INVALID_PARAM			0x04
#define	P2P_STATUS_FAIL_REQUEST_UNABLE			0x05
#define	P2P_STATUS_FAIL_PREVOUS_PROTO_ERR		0x06
#define	P2P_STATUS_FAIL_NO_COMMON_CH			0x07
#define	P2P_STATUS_FAIL_UNKNOWN_P2PGROUP		0x08
#define	P2P_STATUS_FAIL_BOTH_GOINTENT_15		0x09
#define	P2P_STATUS_FAIL_INCOMPATIBLE_PROVSION	0x0A
#define	P2P_STATUS_FAIL_USER_REJECT				0x0B

//	Value of Inviation Flags Attribute
#define	P2P_INVITATION_FLAGS_PERSISTENT			BIT(0)


//	Value of Device Capability Bitmap
#define	P2P_DEVCAP_SERVICE_DISCOVERY		BIT(0)
#define	P2P_DEVCAP_CLIENT_DISCOVERABILITY	BIT(1)
#define	P2P_DEVCAP_CONCURRENT_OPERATION	BIT(2)
#define	P2P_DEVCAP_INFRA_MANAGED			BIT(3)
#define	P2P_DEVCAP_DEVICE_LIMIT				BIT(4)
#define	P2P_DEVCAP_INVITATION_PROC			BIT(5)

//	Value of Group Capability Bitmap
#define	P2P_GRPCAP_GO							BIT(0)
#define	P2P_GRPCAP_PERSISTENT_GROUP			BIT(1)
#define	P2P_GRPCAP_GROUP_LIMIT				BIT(2)
#define	P2P_GRPCAP_INTRABSS					BIT(3)
#define	P2P_GRPCAP_CROSS_CONN				BIT(4)
#define	P2P_GRPCAP_PERSISTENT_RECONN		BIT(5)
#define	P2P_GRPCAP_GROUP_FORMATION			BIT(6)

//	P2P Public Action Frame ( Management Frame )
#define	P2P_PUB_ACTION_ACTION				0x09

//	P2P Public Action Frame Type
#define	P2P_GO_NEGO_REQ						0
#define	P2P_GO_NEGO_RESP						1
#define	P2P_GO_NEGO_CONF						2
#define	P2P_INVIT_REQ							3
#define	P2P_INVIT_RESP							4
#define	P2P_DEVDISC_REQ						5
#define	P2P_DEVDISC_RESP						6
#define	P2P_PROVISION_DISC_REQ				7
#define	P2P_PROVISION_DISC_RESP				8

#define	P2P_MAX_PERSISTENT_GROUP_NUM		10

#define	P2P_PROVISIONING_SCAN_CNT			3

enum P2P_ROLE {
	P2P_ROLE_DISABLE = 0,
	P2P_ROLE_DEVICE = 1,
	P2P_ROLE_CLIENT = 2,
	P2P_ROLE_GO = 3	
};

enum P2P_STATE {
	P2P_STATE_NONE = 0,					//	P2P disable
	P2P_STATE_IDLE = 1,						//	P2P had enabled and do nothing
	P2P_STATE_LISTEN = 2,					//	In pure listen state
	P2P_STATE_SCAN = 3,					//	In scan phase
	P2P_STATE_FIND_PHASE = 4,				//	In find phase	( Listen state + search state )
	P2P_STATE_PROVISION_DIS = 5,			//	In P2P provisioning discovery
											//	todo: should separate four states here
											//	first: sending provisioning request
											//	second: receiving the provisioning response
											//	third: receiving the provisioning request
											//	fourth: sending the provisioning response
	P2P_STATE_GONEGO_ING = 6,				//	Doing the group owner negoitation handshake
	P2P_STATE_GONEGO_OK = 7,				//	finish the group negoitation handshake with success
	P2P_STATE_GONEGO_FAIL = 8,			//	finish the group negoitation handshake with failure
	P2P_STATE_RECV_INVITE_REQ = 9,		//	receiving the P2P Inviation request
	P2P_STATE_PROVISIONING_ING = 10,		//	Doing the P2P WPS
	P2P_STATE_PROVISIONING_DONE = 11,	//	Finish the P2P WPS
};
#endif // _WIFI_H_

