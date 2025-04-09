#ifndef __INC_HAL8192DETEST_FW_IMG_H
#define __INC_HAL8192DETEST_FW_IMG_H

#include <basic_types.h>

/*Created on  2010/ 5/27,  9:49*/

#define Rtl8192DTestImgArrayLength 15054
extern u8 Rtl8192DTestFwImgArray[Rtl8192DTestImgArrayLength];
#define Rtl8192DTestMainArrayLength 1
extern u8 Rtl8192DTestFwMainArray[Rtl8192DTestMainArrayLength];
#define Rtl8192DTestDataArrayLength 1
extern u8 Rtl8192DTestFwDataArray[Rtl8192DTestDataArrayLength];
#define Rtl8192DTestPHY_REG_2TArrayLength 376
extern u32 Rtl8192DTestPHY_REG_2TArray[Rtl8192DTestPHY_REG_2TArrayLength];
#define Rtl8192DTestPHY_REG_1TArrayLength 1
extern u32 Rtl8192DTestPHY_REG_1TArray[Rtl8192DTestPHY_REG_1TArrayLength];
#define Rtl8192DTestPHY_REG_Array_PGLength 1
extern u32 Rtl8192DTestPHY_REG_Array_PG[Rtl8192DTestPHY_REG_Array_PGLength];
#define Rtl8192DTestRadioA_2TArrayLength 340
extern u32 Rtl8192DTestRadioA_2TArray[Rtl8192DTestRadioA_2TArrayLength];
#define Rtl8192DTestRadioB_2TArrayLength 340
extern u32 Rtl8192DTestRadioB_2TArray[Rtl8192DTestRadioB_2TArrayLength];
#define Rtl8192DTestRadioA_1TArrayLength 1
extern u32 Rtl8192DTestRadioA_1TArray[Rtl8192DTestRadioA_1TArrayLength];
#define Rtl8192DTestRadioB_1TArrayLength 1
extern u32 Rtl8192DTestRadioB_1TArray[Rtl8192DTestRadioB_1TArrayLength];
#define Rtl8192DTestMAC_2TArrayLength 174
extern u32 Rtl8192DTestMAC_2TArray[Rtl8192DTestMAC_2TArrayLength];
#define Rtl8192DTestAGCTAB_5GArrayLength 514
extern u32 Rtl8192DTestAGCTAB_5GArray[Rtl8192DTestAGCTAB_5GArrayLength];
#define Rtl8192DTestAGCTAB_2GArrayLength 514
extern u32 Rtl8192DTestAGCTAB_2GArray[Rtl8192DTestAGCTAB_2GArrayLength];

#define RTL8192D_TEST_FW_IMG_FILE			"rtl8192DU\\rtl8192dfw_test.bin"
#define RTL8192D_TEST_PHY_REG_PG_FILE		"rtl8192DU\\PHY_REG_PG_test.txt"

#define RTL8192D_TEST_PHY_REG_FILE			"rtl8192DU\\PHY_REG_test.txt"	
#define RTL8192D_TEST_PHY_RADIO_A_FILE		"rtl8192DU\\radio_a_test.txt"
#define RTL8192D_TEST_PHY_RADIO_B_FILE		"rtl8192DU\\radio_b_test.txt"	
#define RTL8192D_TEST_AGC_TAB_2G			"rtl8192DU\\AGC_TAB_2G_test.txt"
#define RTL8192D_TEST_AGC_TAB_5G			"rtl8192DU\\AGC_TAB_5G_test.txt"
#define RTL8192D_TEST_MAC_REG_FILE			"rtl8192DU\\MAC_REG_test.txt"

#endif //__INC_HAL8192CU_FW_IMG_H
