#ifndef __MMC_OPS_H
#define __MMC_OPS_H

static const unsigned short mmc_ocr_bit_to_vdd[] = {
    150,    155,    160,    165,    170,    180,    190,    200,
    210,    220,    230,    240,    250,    260,    270,    280,
    290,    300,    310,    320,    330,    340,    350,    360
};

static const unsigned int tran_exp[] = {
    10000,      100000,     1000000,    10000000,
    0,      0,      0,      0
};

static const unsigned char tran_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

static const unsigned int tacc_exp[] = {
    1,  10, 100,    1000,   10000,  100000, 1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};


void rsp_rtk2mmc(u32 *raw);

#endif
