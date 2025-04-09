#ifndef    _VENUSFB_H_
#define    _VENUSFB_H_

#define IOA_DC_SYS_MISC ((volatile unsigned int *)0xb8008004)
#define SB2_GRAPHIC_OFFSET 0xe0000000

typedef enum _tagDisplayMode 
{
	DISPLAY_1080,
	DISPLAY_720,
	DISPLAY_576,
	DISPLAY_480
} DISPLAY_MODE;

typedef enum _PIXELFORMAT
{   
	Format_8,           //332
	Format_16,          //565
	Format_32,          //A888
	Format_32_888A,     //888A
	Format_2_IDX,
	Format_4_IDX,
	Format_8_IDX,
	Format_1555,
	Format_4444_ARGB,
	Format_4444_YUVA,
	Format_8888_YUVA,
	Format_5551,
	Format_4444_RGBA,
	NO_OF_PIXEL_FORMAT
} PIXEL_FORMAT;

/*
 * These are the bitfields for each
 * display depth that we support.
 */
struct venusfb_rgb {
	struct fb_bitfield	red;
	struct fb_bitfield	green;
	struct fb_bitfield	blue;
	struct fb_bitfield	transp;
};

typedef struct venusfb_mach_info {
	int			pixclock;
	short			xres;
	short			yres;
	int			xoffset;
	int			yoffset;
	char			bpp;			// in bits
	int			pitch;			// in bytes
	void 			*videomemory;		// virtual address (in-kernel)
	void			*phyAddr;		// physical address
	int			videomemorysize;	// in bytes
	int			videomemorysizeorder; 	// in 2^n (4KBytes unit)
	int			storageMode;            // 0 :  block mode, 1 : sequential mode
	struct venusfb_rgb	rgb_info;
#ifdef CONFIG_REALTEK_RPC
	u_int			winID;
	u_int			mixer2;
#endif
} VENUSFB_MACH_INFO;

struct venusfb_info {
	struct fb_info		fb;
	struct device		*dev;

	/*
	 * These are the addresses we mapped
	 * the framebuffer memory region to.
	 */

	char			*map_vir_addr;
	void			*map_phy_addr;
	u_int			map_size;

	VENUSFB_MACH_INFO	video_info;
	u_int			pseudo_palette[256];

	atomic_t 		ref_count;
};

#define SB2_GREG_START_0                                                             0x1801A500
#define SB2_GREG_START_1                                                             0x1801A504
#define SB2_GREG_START_2                                                             0x1801A508
#define SB2_GREG_START_3                                                             0x1801A50C
#define SB2_GREG_START_0_reg_addr                                                    "0xB801A500"
#define SB2_GREG_START_1_reg_addr                                                    "0xB801A504"
#define SB2_GREG_START_2_reg_addr                                                    "0xB801A508"
#define SB2_GREG_START_3_reg_addr                                                    "0xB801A50C"
#define SB2_GREG_START_0_reg                                                         0xB801A500
#define SB2_GREG_START_1_reg                                                         0xB801A504
#define SB2_GREG_START_2_reg                                                         0xB801A508
#define SB2_GREG_START_3_reg                                                         0xB801A50C
#define SB2_GREG_START_0_inst_adr                                                    "0x0040"
#define SB2_GREG_START_1_inst_adr                                                    "0x0041"
#define SB2_GREG_START_2_inst_adr                                                    "0x0042"
#define SB2_GREG_START_3_inst_adr                                                    "0x0043"
#define SB2_GREG_START_0_inst                                                        0x0040
#define SB2_GREG_START_1_inst                                                        0x0041
#define SB2_GREG_START_2_inst                                                        0x0042
#define SB2_GREG_START_3_inst                                                        0x0043
#define SB2_GREG_START_addr_shift                                                    (0)
#define SB2_GREG_START_addr_mask                                                     (0x0FFFFFFF)
#define SB2_GREG_START_addr(data)                                                    (0x0FFFFFFF&((data)<<0))
#define SB2_GREG_START_addr_src(data)                                                ((0x0FFFFFFF&(data))>>0)

#define SB2_GREG_END_0                                                               0x1801A510
#define SB2_GREG_END_1                                                               0x1801A514
#define SB2_GREG_END_2                                                               0x1801A518
#define SB2_GREG_END_3                                                               0x1801A51C
#define SB2_GREG_END_0_reg_addr                                                      "0xB801A510"
#define SB2_GREG_END_1_reg_addr                                                      "0xB801A514"
#define SB2_GREG_END_2_reg_addr                                                      "0xB801A518"
#define SB2_GREG_END_3_reg_addr                                                      "0xB801A51C"
#define SB2_GREG_END_0_reg                                                           0xB801A510
#define SB2_GREG_END_1_reg                                                           0xB801A514
#define SB2_GREG_END_2_reg                                                           0xB801A518
#define SB2_GREG_END_3_reg                                                           0xB801A51C
#define SB2_GREG_END_0_inst_adr                                                      "0x0044"
#define SB2_GREG_END_1_inst_adr                                                      "0x0045"
#define SB2_GREG_END_2_inst_adr                                                      "0x0046"
#define SB2_GREG_END_3_inst_adr                                                      "0x0047"
#define SB2_GREG_END_0_inst                                                          0x0044
#define SB2_GREG_END_1_inst                                                          0x0045
#define SB2_GREG_END_2_inst                                                          0x0046
#define SB2_GREG_END_3_inst                                                          0x0047
#define SB2_GREG_END_addr_shift                                                      (0)
#define SB2_GREG_END_addr_mask                                                       (0x0FFFFFFF)
#define SB2_GREG_END_addr(data)                                                      (0x0FFFFFFF&((data)<<0))
#define SB2_GREG_END_addr_src(data)                                                  ((0x0FFFFFFF&(data))>>0)

#define SB2_GREG_NO_SWAP                                                             0x1801A520
#define SB2_GREG_NO_SWAP_reg_addr                                                    "0xB801A520"
#define SB2_GREG_NO_SWAP_reg                                                         0xB801A520
#define SB2_GREG_NO_SWAP_inst_adr                                                    "0x0048"
#define SB2_GREG_NO_SWAP_inst                                                        0x0048
#define SB2_GREG_NO_SWAP_disable_shift                                               (0)
#define SB2_GREG_NO_SWAP_disable_mask                                                (0x00000001)
#define SB2_GREG_NO_SWAP_disable(data)                                               (0x00000001&((data)<<0))
#define SB2_GREG_NO_SWAP_disable_src(data)                                           ((0x00000001&(data))>>0)

/*
 * Minimum X and Y resolutions
 */

#define VENUS_FB_IOC_MAGIC		'f'
#define VENUS_FB_IOC_GET_MACHINE_INFO	_IOR(VENUS_FB_IOC_MAGIC, 1, struct venusfb_mach_info)
#define VENUS_FB_IOC_MAXNR		8

#define MIN_XRES	256
#define MIN_YRES	64

#ifdef CONFIG_REALTEK_RPC

#define S_OK		0x10000000
#define S_FAIL		0x10000000

#define RPCCMD_CONFIG_VIDEO_STANDARD	0
#define RPCCMD_CONFIG_HDMI		1
#define RPCCMD_SET_MIXER2_BUFFER	2
#define RPCCMD_CONFIG_GRAPHIC_CANVAS	3
#define RPCCMD_CREATE_GRAPHIC_WIN	4
#define RPCCMD_HIDE_GRAPHIC_WIN		5
#define RPCCMD_MODIFY_GRAPHIC_WIN	6
#define RPCCMD_DELETE_GRAPHIC_WIN	7
#define RPCCMD_DRAW_GRAPHIC_WIN		8
#define RPCCMD_DISPLAY_GRAPHIC		9
#define RPCCMD_CONFIG_OSD_PALETTE	10
#define RPCCMD_SET_OSD_WIN_PALETTE	11
#define RPCCMD_SET_BKGRND		12

enum VO_STANDARD {
	VO_STANDARD_NTSC_M		= 0,
	VO_STANDARD_NTSC_J		= 1,
	VO_STANDARD_NTSC_443		= 2,
	VO_STANDARD_PAL_B		= 3,
	VO_STANDARD_PAL_D		= 4,
	VO_STANDARD_PAL_G		= 5,
	VO_STANDARD_PAL_H		= 6,
	VO_STANDARD_PAL_I		= 7,
	VO_STANDARD_PAL_N		= 8,
	VO_STANDARD_PAL_NC		= 9,
	VO_STANDARD_PAL_M		= 10,
	VO_STANDARD_PAL_60		= 11,
	VO_STANDARD_SECAM		= 12,
	VO_STANDARD_HDTV_720P_60	= 13,
	VO_STANDARD_HDTV_720P_50	= 14,
	VO_STANDARD_HDTV_720P_30	= 15,
	VO_STANDARD_HDTV_720P_25	= 16,
	VO_STANDARD_HDTV_720P_24	= 17,
	VO_STANDARD_HDTV_1080I_60	= 18,
	VO_STANDARD_HDTV_1080I_50	= 19,
	VO_STANDARD_HDTV_1080P_30	= 20,
	VO_STANDARD_HDTV_1080P_25	= 21,
	VO_STANDARD_HDTV_1080P_24	= 22,
	VO_STANDARD_VGA			= 23,
	VO_STANDARD_SVGA		= 24,
	VO_STANDARD_HDTV_1080P_60	= 25,
	VO_STANDARD_HDTV_1080P_50	= 26,
	VO_STANDARD_HDTV_1080I_59	= 27,
	VO_STANDARD_HDTV_720P_59	= 28,
	VO_STANDARD_HDTV_1080P_23	= 29,
};
typedef enum VO_STANDARD VO_STANDARD;

enum VO_PEDESTAL_TYPE {
	VO_PEDESTAL_TYPE_300_700_ON	= 0,
	VO_PEDESTAL_TYPE_300_700_OFF	= 1,
	VO_PEDESTAL_TYPE_286_714_ON	= 2,
	VO_PEDESTAL_TYPE_286_714_OFF	= 3,
};
typedef enum VO_PEDESTAL_TYPE VO_PEDESTAL_TYPE;

enum VO_HDMI_MODE {
	VO_DVI_ON			= 0,
	VO_HDMI_ON			= 1,
	VO_HDMI_OFF			= 2,
};
typedef enum VO_HDMI_MODE VO_HDMI_MODE;

enum VO_HDMI_AUDIO_SAMPLE_FREQ {
	VO_HDMI_AUDIO_NULL		= 0,
	VO_HDMI_AUDIO_32K		= 1,
	VO_HDMI_AUDIO_44_1K		= 2,
	VO_HDMI_AUDIO_48K		= 3,
	VO_HDMI_AUDIO_88_2K		= 4,
	VO_HDMI_AUDIO_96K		= 5,
	VO_HDMI_AUDIO_176_4K		= 6,
	VO_HDMI_AUDIO_192K		= 7,
};
typedef enum VO_HDMI_AUDIO_SAMPLE_FREQ VO_HDMI_AUDIO_SAMPLE_FREQ;

enum VO_HDMI_COLOR_FMT {
	VO_HDMI_RGB444			= 0,
	VO_HDMI_YCbCr422		= 1,
	VO_HDMI_YCbCr444		= 2,
};
typedef enum VO_HDMI_COLOR_FMT VO_HDMI_COLOR_FMT;

struct VIDEO_RPC_VOUT_CONFIG_VIDEO_STANDARD {
	enum VO_STANDARD		standard;
	u_char				enProg;
	u_char				enDIF;
	u_char				enCompRGB;
	enum VO_PEDESTAL_TYPE		pedType;
	u_int				dataInt0;
	u_int				dataInt1;
};
typedef struct VIDEO_RPC_VOUT_CONFIG_VIDEO_STANDARD VIDEO_RPC_VOUT_CONFIG_VIDEO_STANDARD;

struct VIDEO_RPC_VOUT_CONFIG_HDMI {
	enum VO_HDMI_MODE		hdmiMode;
	enum VO_HDMI_AUDIO_SAMPLE_FREQ	audioSampleFreq;
	enum VO_HDMI_COLOR_FMT		colorFmt;
	u_char				audioChannelCount;
};
typedef struct VIDEO_RPC_VOUT_CONFIG_HDMI VIDEO_RPC_VOUT_CONFIG_HDMI;

struct VO_COLOR {
	u_char				c1;
	u_char				c2;
	u_char				c3;
	u_char				isRGB;
};
typedef struct VO_COLOR VO_COLOR;

struct VIDEO_RPC_VOUT_SET_BKGRND {
	struct VO_COLOR			bgColor;
	u_char				bgEnable;
};
typedef struct VIDEO_RPC_VOUT_SET_BKGRND VIDEO_RPC_VOUT_SET_BKGRND;

enum VO_GRAPHIC_PLANE {
	VO_GRAPHIC_OSD			= 0,
	VO_GRAPHIC_SUB1			= 1,
	VO_GRAPHIC_SUB2			= 2,
};
typedef enum VO_GRAPHIC_PLANE VO_GRAPHIC_PLANE;

struct VO_RECTANGLE {
	u_short				x;
	u_short				y;
	u_short				width;
	u_short				height;
};
typedef struct VO_RECTANGLE VO_RECTANGLE;

struct VIDEO_RPC_VOUT_CONFIG_GRAPHIC_CANVAS {
	enum VO_GRAPHIC_PLANE		plane;
	struct VO_RECTANGLE		srcWin;
	struct VO_RECTANGLE		dispWin;
	u_char				go;
};
typedef struct VIDEO_RPC_VOUT_CONFIG_GRAPHIC_CANVAS VIDEO_RPC_VOUT_CONFIG_GRAPHIC_CANVAS;

typedef enum
{
	VOUT_GRAPHIC_OSD		= 0,
	VOUT_GRAPHIC_SUB1		= 1,
	VOUT_GRAPHIC_SUB2		= 2,
} VOUT_GRAPHIC_PLANE;

enum VO_OSD_COLOR_FORMAT {
	VO_OSD_COLOR_FORMAT_2BIT	= 0,
	VO_OSD_COLOR_FORMAT_4BIT	= 1,
	VO_OSD_COLOR_FORMAT_8BIT	= 2,
	VO_OSD_COLOR_FORMAT_RGB332	= 3,
	VO_OSD_COLOR_FORMAT_RGB565	= 4,
	VO_OSD_COLOR_FORMAT_ARGB1555	= 5,
	VO_OSD_COLOR_FORMAT_ARGB4444	= 6,
	VO_OSD_COLOR_FORMAT_ARGB8888	= 7,
	VO_OSD_COLOR_FORMAT_Reserved0	= 8,
	VO_OSD_COLOR_FORMAT_Reserved1	= 9,
	VO_OSD_COLOR_FORMAT_Reserved2	= 10,
	VO_OSD_COLOR_FORMAT_YCBCRA4444	= 11,
	VO_OSD_COLOR_FORMAT_YCBCRA8888	= 12,
	VO_OSD_COLOR_FORMAT_RGBA5551	= 13,
	VO_OSD_COLOR_FORMAT_RGBA4444	= 14,
	VO_OSD_COLOR_FORMAT_RGBA8888	= 15,
	VO_OSD_COLOR_FORMAT_420		= 16,
	VO_OSD_COLOR_FORMAT_422		= 17,
	VO_OSD_COLOR_FORMAT_RGB323	= 18,
	VO_OSD_COLOR_FORMAT_RGB233	= 19,
	VO_OSD_COLOR_FORMAT_RGB556	= 20,
	VO_OSD_COLOR_FORMAT_RGB655	= 21,
};
typedef enum VO_OSD_COLOR_FORMAT VO_OSD_COLOR_FORMAT;

enum VO_OSD_RGB_ORDER {
	VO_OSD_COLOR_RGB		 = 0,
	VO_OSD_COLOR_BGR		 = 1,
	VO_OSD_COLOR_GRB		 = 2,
	VO_OSD_COLOR_GBR		 = 3,
	VO_OSD_COLOR_RBG		 = 4,
	VO_OSD_COLOR_BRG		 = 5,
};
typedef enum VO_OSD_RGB_ORDER VO_OSD_RGB_ORDER;

struct VIDEO_RPC_VOUT_CREATE_GRAPHIC_WIN {
	enum VO_GRAPHIC_PLANE		plane;
	struct VO_RECTANGLE		winPos;
	enum VO_OSD_COLOR_FORMAT	colorFmt;
	enum VO_OSD_RGB_ORDER		rgbOrder;
	int				colorKey;
	u_char				alpha;
	u_char				reserved;
};
typedef struct VIDEO_RPC_VOUT_CREATE_GRAPHIC_WIN VIDEO_RPC_VOUT_CREATE_GRAPHIC_WIN;

enum VO_GRAPHIC_STORAGE_MODE {
	VO_GRAPHIC_BLOCK		= 0,
	VO_GRAPHIC_SEQUENTIAL		= 1,
};
typedef enum VO_GRAPHIC_STORAGE_MODE VO_GRAPHIC_STORAGE_MODE;

struct VIDEO_RPC_VOUT_DRAW_GRAPHIC_WIN {
	enum VO_GRAPHIC_PLANE		plane;
	u_short				winID;
	enum VO_GRAPHIC_STORAGE_MODE	storageMode;
	u_char				paletteIndex;
	u_char				compressed;
	u_char				interlace_Frame;
	u_char				bottomField;
	u_short				startX[4];
	u_short				startY[4];
	u_short				imgPitch[4];
	long				pImage[4];
	u_char				reserved;
	u_char				go;
};
typedef struct VIDEO_RPC_VOUT_DRAW_GRAPHIC_WIN VIDEO_RPC_VOUT_DRAW_GRAPHIC_WIN;

struct VIDEO_RPC_VOUT_DELETE_GRAPHIC_WIN {
	enum VO_GRAPHIC_PLANE		plane;
	u_short				winID;
	u_char				go;
};
typedef struct VIDEO_RPC_VOUT_DELETE_GRAPHIC_WIN VIDEO_RPC_VOUT_DELETE_GRAPHIC_WIN;

#define VO_OSD_WIN_POSITION		0x01
#define VO_OSD_WIN_COLORFMT		0x02
#define VO_OSD_WIN_COLORKEY		0x04
#define VO_OSD_WIN_ALPHA		0x08
#define VO_OSD_WIN_SRC_LOCATION		0x10
#define VO_OSD_WIN_STORAGE_TYPE		0x20

struct VIDEO_RPC_VOUT_MODIFY_GRAPHIC_WIN {
	enum VO_GRAPHIC_PLANE		plane;
	u_char				winID;
	u_char				reqMask;
	struct VO_RECTANGLE		winPos;
	enum VO_OSD_COLOR_FORMAT	colorFmt;
	enum VO_OSD_RGB_ORDER		rgbOrder;
	int				colorKey;
	u_char				alpha;
	enum VO_GRAPHIC_STORAGE_MODE	storageMode;
	u_char				paletteIndex;
	u_char				compressed;
	u_char				interlace_Frame;
	u_char				bottomField;
	u_short				startX[4];
	u_short				startY[4];
	u_short				imgPitch[4];
	long				pImage[4];
	u_char				reserved;
	u_char				go;
};
typedef struct VIDEO_RPC_VOUT_MODIFY_GRAPHIC_WIN VIDEO_RPC_VOUT_MODIFY_GRAPHIC_WIN;

#endif

#endif

