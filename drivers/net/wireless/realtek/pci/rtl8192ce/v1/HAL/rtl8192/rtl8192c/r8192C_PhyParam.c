/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/
/*Created on  2009/ 7/23,  5: 4*/

#ifdef RTL8192CE
#include "r8192C_PhyParam.h"

u32 AGC_TAB_ARRY[] = {
0xc78,	0x7F000001,  
0xc78,	0x7F010001,
0xc78,	0x7F020001,
0xc78,	0x7F030001,
0xc78,	0x7E040001,
0xc78,	0x7D050001, 
0xc78,	0x7C060001,
0xc78,	0x7B070001,
0xc78,	0x7A080001,
0xc78,	0x79090001,
0xc78,	0x780A0001,  
0xc78,	0x770B0001,
0xc78,	0x760C0001,
0xc78,	0x750D0001,
0xc78,	0x740E0001,
0xc78,	0x730F0001,  
0xc78,	0x72100001,
0xc78,	0x71110001,
0xc78,	0x70120001,
0xc78,	0x6F130001,
0xc78,	0x6E140001, 
0xc78,	0x6D150001,         
0xc78,	0x6C160001,        
0xc78,	0x6B170001,         
0xc78,	0x6A180001,        
0xc78,	0x69190001,  
0xc78,	0x681A0001,         
0xc78,	0x671B0001,
0xc78,	0x661C0001,
0xc78,	0x651D0001,
0xc78,	0x641E0001, 
0xc78,	0x631F0001,
0xc78,	0x62200001,
0xc78,	0x61210001,
0xc78,	0x4A220001,
0xc78,	0x49230001,  
0xc78,	0x48240001,
0xc78,	0x47250001,
0xc78,	0x46260001,
0xc78,	0x45270001,
0xc78,	0x2B280001,  
0xc78,	0x2A290001,              
0xc78,	0x292A0001,               
0xc78,	0x282B0001,              
0xc78,	0x272C0001,               
0xc78,	0x262D0001,  
0xc78,	0x252E0001,
0xc78,	0x242F0001,
0xc78,	0x23300001,
0xc78,	0x08310001,
0xc78,	0x07320001, 
0xc78,	0x06330001,
0xc78,	0x05340001,
0xc78,	0x04350001,
0xc78,	0x03360001,
0xc78,	0x02370001,  
0xc78,	0x01380001,
0xc78,	0x00390001,
0xc78,	0x003A0001,
0xc78,	0x003B0001,
0xc78,	0x003C0001,  
0xc78,	0x003D0001,
0xc78,	0x003E0001,
0xc78,	0x003F0001,  
0xc78,	0x7F400001, 
0xc78,	0x7F410001,       
0xc78,	0x7F420001,        
0xc78,	0x7F430001,       
0xc78,	0x7E440001,        
0xc78,	0x7D450001, 
0xc78,	0x7C460001,        
0xc78,	0x7B470001,        
0xc78,	0x7A480001,        
0xc78,	0x79490001,        
0xc78,	0x784a0001, 
0xc78,	0x774b0001,        
0xc78,	0x764c0001,        
0xc78,	0x754d0001,        
0xc78,	0x744e0001,        
0xc78,	0x734f0001, 
0xc78,	0x72500001,        
0xc78,	0x71510001,        
0xc78,	0x70520001,        
0xc78,	0x6F530001,        
0xc78,	0x6E540001, 
0xc78,	0x6D550001,        
0xc78,	0x6C560001,        
0xc78,	0x6B570001,        
0xc78,	0x6A580001,        
0xc78,	0x69590001, 
0xc78,	0x685a0001,        
0xc78,	0x675b0001,        
0xc78,	0x665c0001,        
0xc78,	0x655d0001,        
0xc78,	0x645e0001, 
0xc78,	0x635f0001,        
0xc78,	0x62600001,        
0xc78,	0x61610001,        
0xc78,	0x4A620001,        
0xc78,	0x49630001, 
0xc78,	0x48640001,        
0xc78,	0x47650001,        
0xc78,	0x46660001,        
0xc78,	0x45670001,        
0xc78,	0x2B680001, 
0xc78,	0x2A690001,        
0xc78,	0x296a0001,        
0xc78,	0x286b0001,        
0xc78,	0x276c0001,        
0xc78,	0x266d0001, 
0xc78,	0x256e0001,        
0xc78,	0x246f0001,        
0xc78,	0x23700001,        
0xc78,	0x08710001,        
0xc78,	0x07720001, 
0xc78,	0x06730001,        
0xc78,	0x05740001,        
0xc78,	0x04750001,        
0xc78,	0x03760001,        
0xc78,	0x02770001, 
0xc78,	0x01780001,        
0xc78,	0x00790001,        
0xc78,	0x007a0001,        
0xc78,	0x007b0001,        
0xc78,	0x007c0001, 
0xc78,	0x007d0001,        
0xc78,	0x007e0001,        
0xc78,	0x007f0001, 
0xc78,	0x3800001e,	
0xc78,	0x3801001e,	
0xc78,	0x3802001e,	
0xc78,	0x3803001e,	
0xc78,	0x3804001e,	
0xc78,	0x3805001e,          	
0xc78,	0x3806001e,	
0xc78,	0x3807001e,     	
0xc78,	0x3C08001e,  
0xc78,	0x4009001e,       
0xc78,	0x440A001e,  	
0xc78,	0x460B001e,  
0xc78,	0x4A0C001e,  	
0xc78,	0x4E0D001e,       	
0xc78,	0x500E001e,  
0xc78,	0x540F001e,  	
0xc78,	0x5610001e,  		 
0xc78,	0x5A11001e,  
0xc78,	0x5C12001e,  	 
0xc78,	0x5E13001e,  
0xc78,	0x6414001e,  
0xc78,	0x6C15001e,  
0xc78,	0x6C16001e,  	
0xc78,	0x6C17001e,	
0xc78,	0x6C18001e,	
0xc78,	0x6C19001e,	
0xc78,	0x6C1A001e,	
0xc78,	0x6C1B001e,	
0xc78,	0x6C1C001e,	
0xc78,	0x681D001e,	
0xc78,	0x681E001e,	
0xc78,	0x681F001e,	
0xff,
};

u32 PHY_REG_ARRY[] = {
0x024, 0x11800f,	
0x028, 0xffdb83,	
0x800, 0x00040000, 
0x804, 0x00000003,
0x808, 0x0000fc00,
0x80c, 0x0000000A,  
0x810, 0x10005088,
0x814, 0x020c3d10,
0x818, 0x02200385, 
0x81c, 0x00000000, 
0x820, 0x01000100, 
0x824, 0x00390004, 
0x828, 0x01000100, 
0x82c, 0x00390004, 
0x830, 0x30333333, 
0x834, 0x2a2d2e2f,  
0x838, 0x32323232,  
0x83c, 0x30333333,  
0x840, 0x00010000,  
0x844, 0x00010000,  
0x848, 0x2a2d2e2f,  
0x84c, 0x30333333,  
0x850, 0x00000000,  
0x854, 0x00000000,  
0x858, 0x65a965a9,
0x85c, 0x0c1b25a4,  
0x860, 0x0f7f0130, 
0x864, 0x0f7f0130, 
0x868, 0x2a2d2e2f,  
0x86c, 0x32323232,  
0x870, 0x07000700,  
0x874, 0x00004000,  
0x878, 0x08080808,  
0x87c, 0x00000000,   
0x880, 0xc0083070,  
0x884, 0x000004d5,  
0x888, 0x00000000,  
0x88c, 0x88000004,  
0x890, 0x00000000,
0x894, 0xfffffffe, 
0x898, 0x40302010, 
0x89c, 0x00706050, 
0x900, 0x00000000,
0x904, 0x00000023, 
0x908, 0x00000000,  
0x90c, 0x81121313, 
0xa00, 0x00d047c8,
0xa04, 0xc1ff000c,
0xa08, 0x8ccd8300, 
0xa0c, 0x2e62120f,
0xa10, 0x95009b78, 
0xa14, 0x11144028,
0xa18, 0x00881117,
0xa1c, 0x89140f00,
0xa20, 0x1a1b0000,
0xa24, 0x090e1317, 
0xa28, 0x00000204,
0xa2c, 0x00d30000,
0xa70, 0x001fbf80,
0xa74, 0x00000007,
0xc00, 0x40071d40, 
0xc04, 0x03a05633,  
0xc08, 0x000000e4,  
0xc0c, 0x6c6c6c6c, 
0xc10, 0x08800000, 
0xc14, 0x40000100,
0xc18, 0x08800000,
0xc1c, 0x40000100,
0xc20, 0x00000000,  
0xc24, 0x00000000,  
0xc28, 0x00000000,  
0xc2c, 0x00000000,  
0xc30, 0x69e9ab44,  
0xc34, 0x469652cf,   
0xc38, 0x49795994,  
0xc3c, 0x0a979760,  
0xc40, 0x1f7c403f,  
0xc44, 0x000100b7,  
0xc48, 0xec020107, 
0xc4c, 0x007f037f, 
0xc50, 0x69543420, 
0xc54, 0x433c0094, 
0xc58, 0x69543420, 
0xc5c, 0x433c0094,
0xc60, 0x00000000, 
0xc64, 0x5116848b, 
0xc68, 0x47c00bff, 
0xc6c, 0x00000036, 
0xc70, 0x2c7f000d, 
0xc74, 0x0186109b, 
0xc78, 0x0000001f,
0xc7c, 0x00b91612,  
0xc80, 0x40000100, 
0xc84, 0x20f60000, 
0xc88, 0x40000100, 
0xc8c, 0x20200000, 
0xc90, 0x00000000, 
0xc94, 0x00000000, 	
0xc98, 0x00000000, 
0xc9c, 0x00007f7f, 
0xca0, 0x00000000, 
0xca4, 0x00000000, 
0xca8, 0x00000000, 
0xcac, 0x00000000, 
0xcb0, 0x00000000, 
0xcb4, 0x00000000, 
0xcb8, 0x00000000, 
0xcbc, 0x28000000,
0xcc0, 0x00000000, 
0xcc4, 0x00000000, 
0xcc8, 0x00000000, 
0xccc, 0x00000000, 
0xcd0, 0x00000000, 
0xcd4, 0x00000000, 
0xcd8, 0x64b22427, 
0xcdc, 0x00766932, 
0xce0, 0x00222222,  
0xce4, 0x00000000,
0xce8, 0x37644302,
0xcec, 0x2f97d40c,
0xd00, 0x00080740, 
0xd04, 0x00020403,  
0xd08, 0x0000907f,  
0xd0c, 0x20010201, 
0xd10, 0xa0633333,  
0xd14, 0x33333c63,  
0xd18, 0x6a8f5b6b,
0xd2c, 0xcc979975,  
0xd30, 0x00000000,
0xd34, 0x80608000,
0xd38, 0x00000000,
0xd3c, 0x00027293,
0xd40, 0x00000000,
0xd44, 0x00000000,
0xd48, 0x00000000,
0xd4c, 0x00000000,
0xd50, 0x6437140a,
0xd54, 0x00000000, 
0xd58, 0x00000000,
0xd5c, 0x30032064, 
0xd60, 0x4653de68,
0xd64, 0x04518a3c, 
0xd68, 0x00002101,
0xd6c, 0x2a201c16,  
0xd70, 0x1812362e,  
0xd74, 0x322c2220,  
0xd78, 0x000e3c24,  
0xe00, 0x30333333,	
0xe04, 0x2a2d2e2f,	
0xe08, 0x00003232,	
0xe10, 0x30333333,	
0xe14, 0x2a2d2e2f,	
0xe18, 0x30333333,	
0xe1c, 0x2a2d2e2f,	
0xe28, 0x00000000,        
0xe30, 0x1000dc1f,	
0xe34, 0x10008c1f,
0xe38, 0x02140102,
0xe3C, 0x681604c2,
0xe40, 0x01007c00,
0xe44, 0x01004800,
0xe48, 0xfb000000,
0xe4c, 0x000028d1,
0xe50, 0x1000dc1f,
0xe54, 0x10008c1f,
0xe58, 0x02140102,
0xe5C, 0x28160d05,
0xe6c, 0x63db25a4,	
0xe70, 0x63db25a4,	
0xe74, 0x0c1b25a4,	
0xe78, 0x0c1b25a4,	
0xe7c, 0x0c1b25a4,	
0xe80, 0x0c1b25a4,	
0xe84, 0x63db25a4,	
0xe88, 0x0c1b25a4,	
0xe8c, 0x63db25a4,	
0xed0, 0x63db25a4,	
0xed4, 0x63db25a4,	
0xed8, 0x63db25a4,	
0xedc, 0x001b25a4,	
0xee0, 0x001b25a4,	
0xeec, 0x6fdb25a4,	
0xf14, 0x00000003, 
0xf4c, 0x00000000, 
0xf00, 0x00000300, 
0xff,
};

u32 phy_to1T1R_a_ARRY[] = {
0x844, 0xffffffff, 0x00010000,
0x804, 0x0000000f, 0x1, 
0x824, 0x00f0000f, 0x300004, 
0x82c, 0x00f0000f, 0x100002, 
0x870, 0x04000000, 0x1, 
0x864, 0x00000400, 0x0, 
0x878, 0x000f000f, 0x00002, 
0xe74, 0x0f000000, 0x2, 
0xe78, 0x0f000000, 0x2, 
0xe7c, 0x0f000000, 0x2, 
0xe80, 0x0f000000, 0x2, 
0x90c, 0x000000ff, 0x11, 
0xc04, 0x000000ff, 0x11, 
0xd04, 0x0000000f, 0x1, 

0x1f4, 0xffff0000, 0x7777, 
0x234, 0xf8000000, 0xa,

0xff,
};

u32 phy_to1T2R_ARRY[] = {
0x804, 0x0000000f, 0x3, 
0x824, 0x00f0000f, 0x300004, 
0x82c, 0x00f0000f, 0x300002, 
0x870, 0x04000000, 0x1, 
0x864, 0x00000400, 0x0, 
0x878, 0x000f000f, 0x00002, 
0xe74, 0x0f000000, 0x2, 
0xe78, 0x0f000000, 0x2, 
0xe7c, 0x0f000000, 0x2, 
0xe80, 0x0f000000, 0x2, 
0x90c, 0x000000ff, 0x11, 
0xc04, 0x000000ff, 0x33, 
0xd04, 0x0000000f, 0x3, 

0x1f4, 0xffff0000, 0x7777, 
0x234, 0xf8000000, 0xa,

0xff,
};

u32 radio_a_ARRY[] = {
0x00, 0x30159,	
0x01, 0x31288,	
0x02, 0x28000,	
0X09, 0X2044f,
0x0a, 0x1adb0,
0x0b, 0x54867,
0x0c, 0x8992c,
0x0d, 0x0c52c,

0x0e, 0x21084,
0x0f, 0x00451,

0x19, 0x00000,
0x1a, 0x12255,  
0x1b, 0x60a00,	
0x1c, 0xfc300,
0x1d, 0xa1250,
0x1e, 0x44009,	
0x1f, 0x80441,	
0x20, 0x0b614,
0x21, 0x6c000,	
0x22, 0x00000,
0x23, 0x01558, 
0x24, 0x00060,	
0x25, 0x00583,
0x26, 0x0f400,	
0x27, 0xc8799, 
0x28, 0x55540,  
0x29, 0x04582,
0x2a, 0x00001,  
0x2b, 0x21334,

0x2a, 0x00000,
0x2b, 0x00054,
0x2a, 0x00001,
0x2b, 0x00808,
0x2b, 0x53333,
0x2c, 0x0000c,
0x2a, 0x00002,
0x2b, 0x00808,
0x2b, 0x5b333,
0x2c, 0x0000d,
0x2a, 0x00003,
0x2b, 0x00808,
0x2b, 0x63333,
0x2c, 0x0000d,
0x2a, 0x00004,
0x2b, 0x00808,
0x2b, 0x6b333,
0x2c, 0x0000d,
0x2a, 0x00005,
0x2b, 0x00709,
0x2b, 0x53333,
0x2c, 0x0000d,
0x2a, 0x00006,
0x2b, 0x00709,
0x2b, 0x5b333,
0x2c, 0x0000d,
0x2a, 0x00007,
0x2b, 0x00709,
0x2b, 0x63333,
0x2c, 0x0000d,
0x2a, 0x00008,
0x2b, 0x00709,
0x2b, 0x6b333,
0x2c, 0x0000d,
0x2a, 0x00009,
0x2b, 0x0060a,
0x2b, 0x53333,
0x2c, 0x0000d,
0x2a, 0x0000a,
0x2b, 0x0060a,
0x2b, 0x5b333,
0x2c, 0x0000d,
0x2a, 0x0000b,
0x2b, 0x0060a,
0x2b, 0x63333,
0x2c, 0x0000d,
0x2a, 0x0000c,
0x2b, 0x0060a,
0x2b, 0x6b333,
0x2c, 0x0000d,
0x2a, 0x0000d,
0x2b, 0x0050b,
0x2b, 0x53333,
0x2c, 0x0000d,
0x2a, 0x0000e,
0x2b, 0x0050b,
0x2b, 0x66623,
0x2c, 0x0001a,

0x2a, 0xe0000,

0x10, 0x4000f,	
0x11, 0x231fc,	
0x10, 0x6000f,	
0x11, 0x3f9f8,	
0x10, 0x2000f,	
0x11, 0x203f9,	



0x13, 0x287b3,
0x13, 0x247a7,
0x13, 0x205a3,
0x13, 0x1c3a6,
0x13, 0x182a9,
0x13, 0x142ac,
0x13, 0x101b0,
0x13, 0x0c1a4,
0x13, 0x080b0,
0x13, 0x040a4,
0x13, 0x0001c,

0x14, 0x1944c,
0x14, 0x59444,
0x14, 0x9944c,
0x14, 0xd9444,

0x15, 0x0f433,
0x15, 0x4f433,
0x15, 0x8f433,

0x16, 0xe0330,
0x16, 0xa0330,
0x16, 0x60330,
0x16, 0x20330,


0x00, 0x10159, 
0x18, 0x0f401,  
0xfe, 0x00000,
0xfe, 0x00000,
0x1f, 0x80443,  
0xfe, 0x00000,
0xfe, 0x00000,
0x00, 0x30159, 

0xff, 0xffff,

};

u32 radio_b_ARRY[] = {
0x00, 0x30159,	
0x01, 0x30001,	
0x02, 0x11000,	
0X09, 0X2044f,
0x0a, 0x00fb0,
0x0b, 0xd4c0b,
0x0c, 0x8992c,
0x0d, 0x0c52e,
0x0e, 0x42108,
0x0f, 0x00451,

0x13, 0x28fb3,
0x13, 0x247a7,
0x13, 0x205a3,
0x13, 0x1c3a6,
0x13, 0x182a9,
0x13, 0x142ac,
0x13, 0x101b0,
0x13, 0x0c1a4,
0x13, 0x080b0,
0x13, 0x040a4,
0x13, 0x0001c,


0x15, 0x0f433,

0x16, 0xe0336,
0x16, 0xe033a,

0xff, 0xffff,	
};

u32 radio_b_gm_ARRY[] = {
0x00, 0x30159,
0x01, 0x01041,  
0x02, 0x11000,  
0x05, 0x80fc0,
0x07, 0xfc803,



0xff, 0xffff,
};

#endif 
