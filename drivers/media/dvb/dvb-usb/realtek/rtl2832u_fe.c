
#include "rtl2832u_fe.h"
#include "rtl2832u_io.h"
#include "rtl2832u.h"

extern int demod_default_type;
extern int dtmb_error_packet_discard;

static struct rtl2832_reg_addr rtl2832_reg_map[]= {
	/* RTD2831_RMAP_INDEX_USB_CTRL_BIT5*/			{ RTD2832U_USB, USB_CTRL, 5, 5		},
	/* RTD2831_RMAP_INDEX_USB_STAT*/				{ RTD2832U_USB, USB_STAT, 0, 7		},
	/* RTD2831_RMAP_INDEX_USB_EPA_CTL*/			{ RTD2832U_USB, USB_EPA_CTL, 0, 31	},
	/* RTD2831_RMAP_INDEX_USB_SYSCTL*/				{ RTD2832U_USB, USB_SYSCTL, 0, 31		},
	/* RTD2831_RMAP_INDEX_USB_EPA_CFG*/			{ RTD2832U_USB, USB_EPA_CFG, 0, 31	},
	/* RTD2831_RMAP_INDEX_USB_EPA_MAXPKT*/		{ RTD2832U_USB, USB_EPA_MAXPKT, 0, 31},
	/* RTD2831_RMAP_INDEX_USB_EPA_FIFO_CFG*/		{ RTD2832U_USB, USB_EPA_FIFO_CFG, 0, 31},

	/* RTD2831_RMAP_INDEX_SYS_DEMOD_CTL*/			{ RTD2832U_SYS, DEMOD_CTL, 0, 7	       },
	/* RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL*/	{ RTD2832U_SYS, GPIO_OUTPUT_VAL, 0, 7	},
	/* RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT3*/{ RTD2832U_SYS, GPIO_OUTPUT_EN, 3, 3	},
	/* RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT3*/		{ RTD2832U_SYS, GPIO_DIR, 3, 3		},
	/* RTD2831_RMAP_INDEX_SYS_GPIO_CFG0_BIT67*/	{ RTD2832U_SYS, GPIO_CFG0, 6, 7		},
	/* RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1*/		{ RTD2832U_SYS, DEMOD_CTL1, 0, 7	       },	
	/* RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT1*/{ RTD2832U_SYS, GPIO_OUTPUT_EN, 1, 1	},
	/* RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT1*/		{ RTD2832U_SYS, GPIO_DIR, 1, 1		},
	/* RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT6*/{ RTD2832U_SYS, GPIO_OUTPUT_EN, 6, 6	},	
	/* RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT6*/		{ RTD2832U_SYS, GPIO_DIR, 6, 6		},
	/* RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT5*/{ RTD2832U_SYS, GPIO_OUTPUT_EN, 5, 5},
	/* RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT5*/      { RTD2832U_SYS, GPIO_DIR, 5, 5},

#if 0
	/* RTD2831_RMAP_INDEX_SYS_GPD*/			{ RTD2832U_SYS, GPD, 0, 7		},
	/* RTD2831_RMAP_INDEX_SYS_GPOE*/			{ RTD2832U_SYS, GPOE, 0, 7	},
	/* RTD2831_RMAP_INDEX_SYS_GPO*/			{ RTD2832U_SYS, GPO, 0, 7		},
	/* RTD2831_RMAP_INDEX_SYS_SYS_0*/			{ RTD2832U_SYS, SYS_0, 0, 7	},
#endif

	/* DTMB related */

   
};                                          




static void 	
custom_wait_ms(
	BASE_INTERFACE_MODULE*	pBaseInterface,
	unsigned long				WaitTimeMs)
{
	platform_wait(WaitTimeMs);
	return;	
}


static int
custom_i2c_read(
	BASE_INTERFACE_MODULE*	pBaseInterface,
	unsigned char				DeviceAddr,
	unsigned char*			pReadingBytes,
	unsigned char				ByteNum
	)
{
	struct dvb_usb_device *d;

	pBaseInterface->GetUserDefinedDataPointer(pBaseInterface, (void **)&d);
	if ( read_rtl2832_stdi2c( d, DeviceAddr , pReadingBytes , ByteNum ) ) goto error;
	
	return 0;
error:
	return 1;
}



static int
custom_i2c_write(
	BASE_INTERFACE_MODULE*	pBaseInterface,
	unsigned char				DeviceAddr,
	const unsigned char*			pWritingBytes,
	unsigned char				ByteNum)
{
	struct dvb_usb_device *d;

	pBaseInterface->GetUserDefinedDataPointer(pBaseInterface, (void **)&d);
	if ( write_rtl2832_stdi2c( d, DeviceAddr , (unsigned char*)pWritingBytes , ByteNum ) ) goto error;
	
	return 0;
error:
	return 1;
}



static int
read_usb_sys_register(
	struct rtl2832_state*		p_state,
	rtl2832_reg_map_index		reg_map_index,
	int*						p_val)
{
	RegType			reg_type=	rtl2832_reg_map[reg_map_index].reg_type;
	unsigned short	reg_addr=	rtl2832_reg_map[reg_map_index].reg_addr;
	int				bit_low=	rtl2832_reg_map[reg_map_index].bit_low;
	int				bit_high=	rtl2832_reg_map[reg_map_index].bit_high;

	int	n_byte_read=(bit_high>> 3)+ 1;

	*p_val= 0;
	if (read_usb_sys_int_bytes(p_state->d, reg_type, reg_addr, n_byte_read, p_val)) goto error;

	*p_val= ((*p_val>> bit_low) & rtl2832_reg_mask[bit_high- bit_low]);
 
	return 0;

error:
	return 1;
}




static int
write_usb_sys_register(
	struct rtl2832_state*		p_state,
	rtl2832_reg_map_index		reg_map_index,
	int						val_write)
{
	RegType			reg_type=	rtl2832_reg_map[reg_map_index].reg_type;
	unsigned short	reg_addr=	rtl2832_reg_map[reg_map_index].reg_addr;
	int				bit_low=	rtl2832_reg_map[reg_map_index].bit_low;
	int				bit_high=	rtl2832_reg_map[reg_map_index].bit_high;
	
	int	n_byte_write=	(bit_high>> 3)+ 1;
	int	val_read= 0;
	int	new_val_write;

	if (read_usb_sys_int_bytes(p_state->d, reg_type, reg_addr, n_byte_write, &val_read)) goto error;

	new_val_write= (val_read & (~(rtl2832_reg_mask[bit_high- bit_low]<< bit_low))) | (val_write<< bit_low);

	if (write_usb_sys_int_bytes(p_state->d, reg_type, reg_addr, n_byte_write, new_val_write)) goto error;
	return 0;
	
error:
	return 1;
}



static int 
set_tuner_power(
	struct rtl2832_state*	p_state,
	unsigned char			b_gpio4, 
	unsigned char			onoff)
{

	int			data;

	deb_info(" +%s \n", __FUNCTION__);
	
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data) ) goto error;		

	if(b_gpio4)
	{
		if(onoff)		data &= ~(BIT4);   //set bit4 to 0
		else			data |= BIT4;		//set bit4 to 1		

	}
	else
	{
		if(onoff)		data &= ~(BIT3);   //set bit3 to 0
		else			data |= BIT3;		//set bit3 to 1		
	}
	
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL,data) ) goto error;

	deb_info(" -%s \n", __FUNCTION__);

	return 0;
error:
	return 1;
}


/*
static int 
set_demod_power(
	struct rtl2832_state*	p_state,
	unsigned char			onoff)
{

	int			data;

	deb_info(" +%s \n", __FUNCTION__);
	
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data) ) goto error;		
	if(onoff)		data &= ~(BIT0);   //set bit0 to 0
	else			data |= BIT0;		//set bit0 to 1	
	data &= ~(BIT0);   //3 Demod Power always ON => hw issue.	
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL,data) ) goto error;

	deb_info(" -%s \n", __FUNCTION__);
	return 0;
error:
	return 1;
}
*/


//3//////// Set GPIO3 "OUT"  => Turn ON/OFF Tuner Power
//3//////// Set GPIO3 "IN"      => Button  Wake UP (USB IF) , NO implement in rtl2832u linux driver

static int 
gpio3_out_setting(
	struct rtl2832_state*	p_state)
{
	int			data;

	deb_info(" +%s \n", __FUNCTION__);

	// GPIO3_PAD Pull-HIGH, BIT76
	data = 2;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_CFG0_BIT67,data) ) goto error;

	// GPO_GPIO3 = 1, GPIO3 output value = 1 
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data) ) goto error;		
	data |= BIT3;		//set bit3 to 1
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL,data) ) goto error;

	// GPD_GPIO3=0, GPIO3 output direction
	data = 0;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT3,data) ) goto error;

	// GPOE_GPIO3=1, GPIO3 output enable
	data = 1;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT3,data) ) goto error;

	//BTN_WAKEUP_DIS = 1
	data = 1;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_CTRL_BIT5,data) ) goto error;

	deb_info(" -%s \n", __FUNCTION__);

	return 0;
error:
	return 1;
}



int usb_epa_fifo_reset(
	struct rtl2832_state*	p_state)
{

	int					data;

	deb_info(" +%s \n", __FUNCTION__);
	
	//3 reset epa fifo:
	//3[9] Reset EPA FIFO
	//3 [5] FIFO Flush,Write 1 to flush the oldest TS packet (a 188 bytes block)

	data = 0x0210;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CTL,data) ) goto error;

	data = 0xffff;
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CTL,&data) ) goto error;

	if( (data & 0xffff) != 0x0210)
	{
		deb_info("Write error RTD2831_RMAP_INDEX_USB_EPA_CTL = 0x%x\n",data);
	 	goto error;	
	}

	data=0x0000;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CTL,data) ) goto error;

	data = 0xffff;
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CTL,&data) ) goto error;

	if( ( data  & 0xffff) != 0x0000)
	{
		deb_info("Write error RTD2831_RMAP_INDEX_USB_EPA_CTL = 0x%x\n",data);
	 	goto error;	
	}

	deb_info(" -%s \n", __FUNCTION__);

	return 0;

error:
	return 1;

}


static int 
usb_init_bulk_setting(
	struct rtl2832_state*	p_state)
{

	int					data;
	
	deb_info(" +%s \n", __FUNCTION__);
	
	//3 1.FULL packer mode(for bulk)
	//3 2.DMA enable.
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_SYSCTL, &data) ) goto error;

	data &=0xffffff00;
	data |= 0x09;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_SYSCTL, data) ) goto error;

	data=0;
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_SYSCTL, &data) ) goto error;
      
	if((data&0xff)!=0x09)  
	{
		deb_info("Open bulk FULL packet mode error!!\n");
	 	goto error;
	}

	//3check epa config,
	//3[9-8]:00, 1 transaction per microframe
	//3[7]:1, epa enable
	//3[6-5]:10, bulk mode
	//3[4]:1, device to host
	//3[3:0]:0001, endpoint number
	data = 0;
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_CFG, &data) ) goto error;                
	if((data&0x0300)!=0x0000 || (data&0xff)!=0xd1)
	{
		deb_info("Open bulk EPA config error! data=0x%x \n" , data);
	 	goto error;	
	}

	//3 EPA maxsize packet 
	//3 512:highspeedbulk, 64:fullspeedbulk. 
	//3 940:highspeediso,  940:fullspeediso.

	//3 get info :HIGH_SPEED or FULL_SPEED
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_STAT, &data) ) goto error;	
	if(data&0x01)  
	{
		data = 0x00000200;
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_MAXPKT, data) ) goto error;

		data=0;
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_MAXPKT, &data) ) goto error;
	                      
		if((data&0xffff)!=0x0200)
		{
			deb_info("Open bulk EPA max packet size error!\n");
		 	goto error;
		}

		deb_info("HIGH SPEED\n");
	}
	else 
    	{
		data = 0x00000040;
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_MAXPKT, data) ) goto error;

		data=0;
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_MAXPKT, &data) ) goto error;
	                      
		if((data&0xffff)!=0x0200)
		{
			deb_info("Open bulk EPA max packet size error!\n");
		 	goto error;
		}
		
		deb_info("FULL SPEED\n");
	}	

	deb_info(" -%s \n", __FUNCTION__);
	
	return 0;

error:	
	return 1;
}


static int 
usb_init_setting(
	struct rtl2832_state*	p_state)
{

	int					data;

	deb_info(" +%s \n", __FUNCTION__);

	if ( usb_init_bulk_setting(p_state) ) goto error;

	//3 change fifo length of EPA 
	data = 0x00000014;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_FIFO_CFG, data) ) goto error;
	data = 0xcccccccc;
	if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_USB_EPA_FIFO_CFG, &data) ) goto error;
	if( (data & 0xff) != 0x14)
	{
		deb_info("Write error RTD2831_RMAP_INDEX_USB_EPA_FIFO_CFG =0x%x\n",data);
	 	goto error;
	}

	if ( usb_epa_fifo_reset(p_state) ) goto error;

	deb_info(" -%s \n", __FUNCTION__);
	
	return 0;

error: 
	return 1;	
}



static int 
suspend_latch_setting(
	struct rtl2832_state*	p_state,
	unsigned char			resume)
{

	int					data;
	deb_info(" +%s \n", __FUNCTION__);

	if (resume)
	{
		//3 Suspend_latch_en = 0  => Set BIT4 = 0 
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1, &data) ) goto error;		
		data &= (~BIT4);	//set bit4 to 0
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1,data) ) goto error;
	}
	else
	{
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1, &data) ) goto error;		
		data |= BIT4;		//set bit4 to 1
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1,data) ) goto error;
	}

	deb_info(" -%s \n", __FUNCTION__);	

	return 0;
error:
	return 1;

}





//3////// DEMOD_CTL1  => IR Setting , IR wakeup from suspend mode
//3////// if resume =1, resume
//3////// if resume = 0, suspend


static int 
demod_ctl1_setting(
	struct rtl2832_state*	p_state,
	unsigned char			resume)
{

	int					data;

	deb_info(" +%s \n", __FUNCTION__);
	
	if(resume)
	{
		// IR_suspend	
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1, &data) ) goto error;		
		data &= (~BIT2);		//set bit2 to 0
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1,data) ) goto error;

		//Clk_400k
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1, &data) ) goto error;		
		data &= (~BIT3);		//set bit3 to 0
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1,data) ) goto error;
	}
	else
	{
		//Clk_400k
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1, &data) ) goto error;		
		data |= BIT3;		//set bit3 to 1
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1,data) ) goto error;

		// IR_suspend		
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1, &data) ) goto error;		
		data |= BIT2;		//set bit2 to 1
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL1,data) ) goto error;
	}

	deb_info(" -%s \n", __FUNCTION__);
	
	return 0;
error:
	return 1;

}




static int 
demod_ctl_setting(
	struct rtl2832_state*	p_state,
	unsigned char			resume,
	unsigned char               on)
{

	int					data;
	unsigned char			tmp;
	
	deb_info(" +%s \n", __FUNCTION__);
		
	if(resume)
	{
		// PLL setting
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
		data |= BIT7;		//set bit7 to 1
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;

		
		//2 + Begin LOCK
		// Demod  H/W Reset
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
		data &= (~BIT5);	//set bit5 to 0
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;

		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
		data |= BIT5;		//set bit5 to 1
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;
		
		//3 reset page chache to 0 		
		if ( read_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR, 0, 1, &tmp, 1 ) ) goto error;	
		//2 -End LOCK

		// delay 5ms
		platform_wait(5);


		if(on)
		{
			// ADC_Q setting on
			if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
			data |= BIT3;		//set bit3 to 1
			if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;

			// ADC_I setting on
			if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
			data |= BIT6;		//set bit3 to 1
			if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;
		}
		else
		{
			// ADC_I setting off
			if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
			data &= (~BIT6);		//set bit3 to 0
			if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;

			// ADC_Q setting off
			if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
			data &= (~BIT3);		//set bit3 to 0
			if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;		
		}
	}
	else
	{

		// ADC_I setting
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
		data &= (~BIT6);		//set bit3 to 0
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;

		// ADC_Q setting
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
		data &= (~BIT3);		//set bit3 to 0
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;

		// PLL setting
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL, &data) ) goto error;		
		data &= (~BIT7);		//set bit7 to 0
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_DEMOD_CTL,data) ) goto error;

	}

	deb_info(" -%s \n", __FUNCTION__);

	return 0;
error: 
	return 1;	

}


static int
read_tuner_id_register(
	struct rtl2832_state*	p_state,
	unsigned char			tuner_addr,
	unsigned char			tuner_offset,
	unsigned char*		id_data,
	unsigned char			length)
{
	unsigned char				i2c_repeater;	
	struct dvb_usb_device*	d = p_state->d;

	//2 + Begin LOCK
	if(read_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;
	i2c_repeater |= BIT3;	
	if(write_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;
	
	if(read_rtl2832_tuner_register(d, tuner_addr, tuner_offset, id_data, length)) goto error;

	if(read_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;
	i2c_repeater &= (~BIT3);	
	if(write_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;
	//2 - End LOCK
	return 0;
	
error:
	return 1;
}



static int
check_mxl5007t_chip_version(
	struct rtl2832_state*	p_state,
	unsigned char			*chipversion)
{

	unsigned char Buffer[LEN_2_BYTE];
	unsigned char	i2c_repeater;	

	struct dvb_usb_device*	d = p_state->d;	


	Buffer[0] = (unsigned char)MXL5007T_I2C_READING_CONST;
	Buffer[1] = (unsigned char)MXL5007T_CHECK_ADDRESS;


	if(read_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;
	i2c_repeater |= BIT3;	
	if(write_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;


	write_rtl2832_stdi2c(d, MXL5007T_BASE_ADDRESS , Buffer, LEN_2_BYTE);

	read_rtl2832_stdi2c(d, MXL5007T_BASE_ADDRESS, Buffer, LEN_1_BYTE);

	if(read_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;
	i2c_repeater &= (~BIT3);
	if(write_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR, 1, 1, &i2c_repeater, 1 )) goto error;



	switch(Buffer[0])
	{
		case MXL5007T_CHECK_VALUE: 
			*chipversion = MxL_5007T_V4;
			break;
		default: 
			*chipversion = MxL_UNKNOWN_ID;
			break;
	}	

	return 0;

error:

	return 1;

}






static int 
check_tuner_type(
	struct rtl2832_state	*p_state)
{

	unsigned char				tuner_id_data[2];

       deb_info(" +%s\n", __FUNCTION__);
	if ((!read_tuner_id_register(p_state, MT2266_TUNER_ADDR, MT2266_OFFSET,  tuner_id_data, LEN_1_BYTE)) && 
		( tuner_id_data[0] == MT2266_CHECK_VAL ))
	{
		 	p_state->tuner_type = RTL2832_TUNER_TYPE_MT2266;

			deb_info(" -%s : MT2266 tuner on board...\n", __FUNCTION__);
	}
	else
	{
		if ((!read_tuner_id_register(p_state, FC2580_TUNER_ADDR, FC2580_OFFSET,  tuner_id_data, LEN_1_BYTE)) &&
			((tuner_id_data[0]&(~BIT7)) == FC2580_CHECK_VAL ))
		{
			p_state->tuner_type = RTL2832_TUNER_TYPE_FC2580;

			deb_info(" -%s : FC2580 tuner on board...\n", __FUNCTION__);
		}
		else
		{
			if ((!read_tuner_id_register(p_state, TUA9001_TUNER_ADDR, TUA9001_OFFSET,  tuner_id_data, LEN_2_BYTE)) &&
				(((tuner_id_data[0]<<8)|tuner_id_data[1]) == TUA9001_CHECK_VAL ))
			{
				p_state->tuner_type = RTL2832_TUNER_TYPE_TUA9001;
				
				deb_info(" -%s : TUA9001 tuner on board...\n", __FUNCTION__);
			}
			else
			{
				unsigned char chip_version;
				if ((!check_mxl5007t_chip_version(p_state, &chip_version)) &&
					(chip_version == MXL5007T_CHECK_VALUE) )
				{
					p_state->tuner_type = RTL2832_TUNER_TYPE_MXL5007T;

					deb_info(" -%s : MXL5007T tuner on board...\n", __FUNCTION__);
				}
				else
				{
					if ((!read_tuner_id_register(p_state, FC0012_BASE_ADDRESS , FC0012_CHECK_ADDRESS,  tuner_id_data, LEN_1_BYTE)) &&
					(tuner_id_data[0] == FC0012_CHECK_VALUE))
					{
						p_state->tuner_type = RTL2832_TUNER_TYPE_FC0012;
						
						deb_info(" -%s : FC0012 tuner on board...\n", __FUNCTION__);	
					}
					else
					{
						if((!read_tuner_id_register(p_state, E4000_BASE_ADDRESS, E4000_CHECK_ADDRESS, tuner_id_data, LEN_1_BYTE)) && 
							(tuner_id_data[0] == E4000_CHECK_VALUE))	
						{
							p_state->tuner_type = RTL2832_TUNER_TYPE_E4000;
							deb_info(" -%s : E4000 tuner on board...\n", __FUNCTION__);
						}
						else
						{
							p_state->tuner_type = RTL2832_TUNER_TYPE_UNKNOWN;
							
							deb_info(" -%s : Unknown tuner on board...\n", __FUNCTION__);	
							goto error;
						}
					}
				}
			}	
		}

	}

	return 0;
error:
	return -1;
}

static int 
gpio1_output_enable_direction(
	struct rtl2832_state*	p_state)
{
	int data;
	// GPD_GPIO1=0, GPIO1 output direction
	data = 0;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT1,data) ) goto error;

	// GPOE_GPIO1=1, GPIO1 output enable
	data = 1;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT1,data) ) goto error;

	return 0;
error:
	return 1;
}


static int 
gpio6_output_enable_direction(
	struct rtl2832_state*	p_state)
{
	int data;
	// GPD_GPIO6=0, GPIO6 output direction
	data = 0;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT6,data) ) goto error;

	// GPOE_GPIO6=1, GPIO6 output enable
	data = 1;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT6,data) ) goto error;

	return 0;
error:
	return 1;
}


static int 
gpio5_output_enable_direction(
	struct rtl2832_state*	p_state)
{
	int data;
	// GPD_GPIO5=0, GPIO5 output direction
	data = 0;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_DIR_BIT5,data) ) goto error;

	// GPOE_GPIO5=1, GPIO5 output enable
	data = 1;
	if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_EN_BIT5,data) ) goto error;

	return 0;
error:
	return 1;
}


static int 
check_dvbt_reset_parameters(
	struct rtl2832_state*	p_state,
	unsigned long			frequency,
	enum fe_bandwidth	bandwidth,	
	int*					reset_needed)
{

	int							is_lock;	
	unsigned int					diff_ms;

	deb_info(" +%s \n", __FUNCTION__);

	*reset_needed = 1;	 //3initialize "reset_needed"
	
	if( (p_state->current_frequency == frequency) && (p_state->current_bandwidth == bandwidth) )
	{
		if( p_state->pNim->IsSignalLocked(p_state->pNim, &is_lock) ) goto error;
		diff_ms = 0;		
		
		while( !(is_lock == LOCKED || diff_ms > 200) )
		{
			platform_wait(40);
			diff_ms += 40;
			if( p_state->pNim->IsSignalLocked(p_state->pNim, &is_lock) ) goto error;
		}

	       if (is_lock==YES)		
	       {
		   *reset_needed = 0;		 //3 set "reset_needed" = 0
		   deb_info("%s : The same frequency = %d setting\n", __FUNCTION__, (int)frequency);
	       }
	}	   

	deb_info(" -%s \n", __FUNCTION__);

	return 0;

error:
	
	*reset_needed = 1; 	//3 set "reset_needed" = 1
	return 1;
}


#if UPDATE_FUNC_ENABLE_2832
void
rtl2832_update_functions(void* ptr)
{        
	struct rtl2832_state* p_state = (struct rtl2832_state*) ptr;
	//unsigned  long ber_num, ber_dem;
	//long snr_num = 0;
	//long snr_dem = 0;
	//long _snr= 0;

	if(p_state->pNim == NULL)
	{
		deb_info("%s error\n", __FUNCTION__);
		goto mutex_error;
	}


	if( down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;

	deb_info(" +%s\n", __FUNCTION__);	

	if(!p_state->is_frequency_valid)
	{
		deb_info("  %s no need \n", __FUNCTION__);
		goto advance_exit;
	}

	// Enable demod DVBT_IIC_REPEAT.
	//if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x1) != FUNCTION_SUCCESS)
	//	goto advance_exit;

	// Update tuner mode
	if( p_state->pNim->UpdateFunction(p_state->pNim))
		goto advance_exit;
		
	// Disable demod DVBT_IIC_REPEAT.
	//if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x0) != FUNCTION_SUCCESS)
	//	goto advance_exit;
	
	/* p_state->pNim->UpdateFunction(p_state->pNim);
	p_state->pNim->GetBer( p_state->pNim , &ber_num , &ber_dem);
	p_state->pNim->GetSnrDb(p_state->pNim, &snr_num, &snr_dem) ;

	_snr = snr_num / snr_dem;
	if( _snr < 0 ) _snr = 0;

	deb_info("%s : ber = %lu, snr = %lu\n", __FUNCTION__,ber_num,_snr);
	*/

advance_exit:
	up(&p_state->i2c_repeater_mutex);
	
	schedule_delayed_work(&p_state->update2832_procedure_work, UPDATE_PROCEDURE_PERIOD_2832);

	deb_info(" -%s\n", __FUNCTION__);
	
	return;

mutex_error:
	return;
	
}
#endif

#if UPDATE_FUNC_ENABLE_2836
void 
rtl2836_update_function(void* ptr)
{
	struct rtl2832_state* p_state = (struct rtl2832_state*) ptr; 
	unsigned long Per1, Per2;
	long Data1,Data2;
	unsigned char data;
	DTMB_DEMOD_MODULE * pDtmbDemod;
	
	if(p_state->pNim2836 == NULL)
	{
		deb_info("%s error\n", __FUNCTION__);
		goto mutex_error;
	}
	pDtmbDemod = p_state->pNim2836->pDemod;

	if( down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;

	deb_info(" +%s\n", __FUNCTION__);
	if(!p_state->is_frequency_valid)
	{
		deb_info(" %s no need \n", __FUNCTION__);
		goto advance_exit;
	}

	if(pDtmbDemod->UpdateFunction(pDtmbDemod))	
	{
		deb_info("%s -- UpdateFunction failed\n", __FUNCTION__);
	}

	pDtmbDemod->GetPer(pDtmbDemod,&Per1,&Per2);
	deb_info("%s -- ***GetPer= %d***\n", __FUNCTION__, (int)Per1);

	pDtmbDemod->GetSnrDb(pDtmbDemod,&Data1,&Data2);            
	deb_info("%s -- ***SNR = %d***\n",__FUNCTION__, (int)(Data1>>2));

	read_rtl2836_demod_register(p_state->d, RTL2836_DEMOD_ADDR, PAGE_6, 0xc0, &data, LEN_1_BYTE);
	deb_info("%s --***FSM = %d***\n", __FUNCTION__, (data&0x1f));

advance_exit:
	
	up(&p_state->i2c_repeater_mutex);
	
	schedule_delayed_work(&p_state->update2836_procedure_work,  UPDATE_PROCEDURE_PERIOD_2836);

	deb_info(" -%s\n", __FUNCTION__);	
	return;

mutex_error:
	return;

}
#endif

static int 
rtl2832_init(
	struct dvb_frontend*	fe)
{
	struct rtl2832_state*	p_state = fe->demodulator_priv;
       char tmp_set_tuner_power_gpio4;

	if( down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;

	deb_info(" +%s\n", __FUNCTION__);
	
	//usb_reset_device(p_state->d->udev);

	if( usb_init_setting(p_state) ) goto error;
	
	if( gpio3_out_setting(p_state) ) goto error;				//3Set GPIO3 OUT	
	
	if( demod_ctl1_setting(p_state , 1) ) goto error;		//3	DEMOD_CTL1, resume = 1

	//if( set_demod_power(p_state , 1) ) goto error;		//3	turn ON demod power

	if( suspend_latch_setting(p_state , 1) ) goto error;		//3 suspend_latch_en = 0, resume = 1 					

	if( demod_ctl_setting(p_state , 1,  1) ) goto error;		//3 DEMOD_CTL, resume =1; ADC on

	//3 Auto detect Tuner Power Pin (GPIO3 or GPIO4)	
	if( set_tuner_power(p_state , 1 , 1) ) goto error;		//3	turn ON tuner power, 1st try GPIO4

	if( check_tuner_type(p_state) )
	{
		if(p_state->tuner_type == RTL2832_TUNER_TYPE_UNKNOWN)
		{
			if( set_tuner_power(p_state , 1 , 0) )	goto error;			//3recover GPIO4 setting at 1st try
			if( set_tuner_power(p_state , 0 , 1) )	goto error;			//3 2nd try GPIO3
			if( check_tuner_type(p_state) )		goto error;	

                     tmp_set_tuner_power_gpio4 = 0;	//4 Tuner Power Pin : GPIO3		
		}
		else
		{
			goto error;
		}	
	}
	else
	{
		tmp_set_tuner_power_gpio4 = 1;//4  Tuner Power Pin : GPIO4
	}


	if( p_state->tuner_type == RTL2832_TUNER_TYPE_TUA9001)
	{	
		if( gpio1_output_enable_direction(p_state) )	goto error;	
	}
	else if( p_state->tuner_type == RTL2832_TUNER_TYPE_MXL5007T)
	{
		//3 MXL5007T : Set GPIO6 OUTPUT_EN & OUTPUT_DIR & OUTPUT_VAL for YUAN
		int	data;		
		if( gpio6_output_enable_direction(p_state) )	goto error;	

		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data) ) goto error;		
		data |= BIT6;
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL,data) ) goto error;

	}
	else if( p_state->tuner_type == RTL2832_TUNER_TYPE_FC0012)
	{
		int data;
		if( gpio5_output_enable_direction(p_state))		goto error;

		if( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data) ) goto error;
		data |= BIT5; // set GPIO5 high
		if( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, data) ) goto error;
		data &= ~(BIT5); // set GPIO5 low
		if( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, data) ) goto error;
	}
	
       //3 Check if support RTL2836 DTMB. 
	check_dtmb_support(p_state); //2836 is off in the end of check_dtmb_support()

       //3Set demod_type.
	if((p_state->demod_ask_type == RTL2836) && (p_state->demod_support_type & SUPPORT_DTMB_MODE))
	{
		p_state->demod_type = RTL2836;
	}
	else
	{
		p_state->demod_type = RTL2832;
	}
	deb_info("demod_type is %d\n", p_state->demod_type);


	//3 Build Nim Module
	build_nim_module(p_state);
	p_state->pNim->pTuner->b_set_tuner_power_gpio4 = tmp_set_tuner_power_gpio4;

       if(p_state->demod_type == RTL2836)
       {
       	if ( set_demod_2836_power(p_state,  1))  goto error;//3 2836 on

		//3 RTL2832 ADC_I& ADC_Q OFF
		if( demod_ctl_setting(p_state,  1,  0)) goto error;//3     ADC off
       }
	
	//3  RTL2832U AGC Setting, Serial Mode Switch Setting, PIP Setting based on demod_type
	if(demod_init_setting(p_state)) goto error;

	
	//3 Nim initial
	if(p_state->demod_type == RTL2832)
	{
		//3 Nim initialize for 2832
		if ( p_state->pNim->Initialize(p_state->pNim) ) goto error; 
	}
	else if(p_state->demod_type == RTL2836)
	{ 
		// Enable demod DVBT_IIC_REPEAT.
	       if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x1) )   goto error;

             //3 Nim initialize for 2836
		if ( p_state->pNim2836->Initialize(p_state->pNim2836)) goto error;

		// Disable demod DVBT_IIC_REPEAT.
	       if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x0))  goto error;

		if(dtmb_error_packet_discard)
		{
			unsigned char val;
			if(read_rtl2832_demod_register(p_state->d,  RTL2832_DEMOD_ADDR, PAGE_0, 0x21, &val,  LEN_1_BYTE)) goto error;
			val &= ~(BIT5);
			if(write_rtl2832_demod_register(p_state->d,  RTL2832_DEMOD_ADDR, PAGE_0, 0x21, &val,  LEN_1_BYTE)) goto error;
	
			if(read_rtl2836_demod_register(p_state->d,  RTL2836_DEMOD_ADDR, PAGE_4, 0x26, &val,  LEN_1_BYTE)) goto error;
			val &= ~(BIT0);
			if(write_rtl2836_demod_register(p_state->d,  RTL2836_DEMOD_ADDR, PAGE_4, 0x26, &val,  LEN_1_BYTE)) goto error;

			deb_info(" dtmb discard error packets\n");
		}
		else
		{
			deb_info(" dtmb NOT discard error packets\n");
		}

	}

	
	p_state->is_initial = 1; 
	
	deb_info(" -%s \n", __FUNCTION__);

	up(&p_state->i2c_repeater_mutex);	

#if UPDATE_FUNC_ENABLE_2836
       if(p_state->demod_type == RTL2836)
	   	schedule_delayed_work(&(p_state->update2836_procedure_work),  0);
#endif

#if UPDATE_FUNC_ENABLE_2832
       if(p_state->demod_type == RTL2832)
	   	schedule_delayed_work(&(p_state->update2832_procedure_work), 0);//3 Initialize update function
#endif

	return 0;
error:
	up(&p_state->i2c_repeater_mutex);	
	
mutex_error:
	deb_info(" -%s  error end\n", __FUNCTION__);

	return -1;
}


static void
rtl2832_release(
	struct dvb_frontend*	fe)
{
	struct rtl2832_state* p_state = fe->demodulator_priv;
	MT2266_EXTRA_MODULE*	p_mt2266_extra;	

	if( p_state->pNim== NULL)
	{
		deb_info(" %s pNim = NULL \n", __FUNCTION__);
		return;
	}

	if( p_state->is_mt2266_nim_module_built)
	{
		p_mt2266_extra = (MT2266_EXTRA_MODULE *)(p_state->pNim->pTuner->pExtra);
		p_mt2266_extra->CloseHandle(p_state->pNim->pTuner);
		p_state->is_mt2266_nim_module_built = 0;
	}

	if(p_state->is_initial)
	{
#if UPDATE_FUNC_ENABLE_2836
		if(p_state->demod_type == RTL2836)
		{
			cancel_delayed_work( &(p_state->update2836_procedure_work));
			flush_scheduled_work();
		}
#endif

#if UPDATE_FUNC_ENABLE_2832
              if(p_state->demod_type == RTL2832)
              {
	       	cancel_delayed_work( &(p_state->update2832_procedure_work) );//cancel_rearming_delayed_work
		       flush_scheduled_work();
              }
#endif
		p_state->is_initial = 0;
	}
	p_state->is_frequency_valid = 0;

	kfree(p_state);

	deb_info("  %s \n", __FUNCTION__);	

	return;
}



static int 
rtl2832_sleep(
	struct dvb_frontend*	fe)
{
	struct rtl2832_state* p_state = fe->demodulator_priv;
	MT2266_EXTRA_MODULE*	p_mt2266_extra;
	DVBT_DEMOD_MODULE	*pDemod;
	DVBT_NIM_MODULE		*pNim;
	int 			data;
       //DTMB_NIM_MODULE   * pNim2836;
       //DTMB_DEMOD_MODULE * pDemod2836;
	

#if 0 //For Debug
	int			page_no, addr_no;
	unsigned char	reg_value;
#endif

       pNim = p_state->pNim;	
       //pNim2836 = p_state->pNim2836;
	if( pNim== NULL)
	{
		deb_info(" %s pNim = NULL \n", __FUNCTION__);
		return -1;
	}

	//if((p_state->demod_support_type & SUPPORT_DTMB_MODE) && pNim2836==NULL)
	//{
	//	deb_info(" %s pNim2836 = NULL \n", __FUNCTION__);
	//	return -1;
	//}

	pDemod = pNim->pDemod;
      // pDemod2836 = pNim2836->pDemod;

	if( p_state->is_mt2266_nim_module_built)
	{
		p_mt2266_extra = (MT2266_EXTRA_MODULE *)(p_state->pNim->pTuner->pExtra);
		p_mt2266_extra->CloseHandle(p_state->pNim->pTuner);
		p_state->is_mt2266_nim_module_built = 0;
	}
	
	if( p_state->is_initial )
	{
#if UPDATE_FUNC_ENABLE_2836
		if(p_state->demod_type == RTL2836)
		{
			cancel_delayed_work( &(p_state->update2836_procedure_work));
			flush_scheduled_work();
		}
#endif

#if UPDATE_FUNC_ENABLE_2832
              if(p_state->demod_type == RTL2832)
              {
	              cancel_delayed_work( &(p_state->update2832_procedure_work));// cancel_rearming_delayed_work
	       	flush_scheduled_work();
              }
#endif		
		p_state->is_initial = 0;
	}
	p_state->is_frequency_valid = 0;

	if( down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;

	deb_info(" +%s \n", __FUNCTION__);

#if 0
	//2 For debug
	/* for( page_no = 0; page_no < 3; page_no++ )//2832 
	{
		pDemod->SetRegPage(pDemod, page_no);
		for( addr_no = 0; addr_no < 256; addr_no++ )
		{
			pDemod->GetRegBytes(pDemod, addr_no, &reg_value, 1);
			printk("0x%x, 0x%x, 0x%x\n", page_no, addr_no, reg_value);
		}
	}*/
      for( page_no = 0; page_no < 10; page_no++ )//2836 
	{
		pDemod2836->SetRegPage(pDemod2836, page_no);
		for( addr_no = 0; addr_no < 256; addr_no++ )
		{
			pDemod2836->GetRegBytes(pDemod2836, addr_no, &reg_value, 1);
			printk("0x%x, 0x%x, 0x%x\n", page_no, addr_no, reg_value);
		}
	}
#endif

	 if( p_state->tuner_type == RTL2832_TUNER_TYPE_MXL5007T)
	{
		//3 MXL5007T : Set GPIO6 OUTPUT_VAL  OFF for YUAN
		if ( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data) ) goto error;		
		data &= (~BIT6);
		if ( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL,data) ) goto error;

	}


	if( demod_ctl1_setting(p_state , 0) ) goto error;		//3	DEMOD_CTL1, resume = 0

	if( set_tuner_power(p_state, p_state->pNim->pTuner->b_set_tuner_power_gpio4, 0) ) goto error;		//3	turn OFF tuner power

	if( demod_ctl_setting(p_state , 0,  0) ) goto error;		//3 	DEMOD_CTL, resume =0	

	//if(p_state->demod_type == RTL2836)
	set_demod_2836_power(p_state,  0);   //3 RTL2836 power OFF
	
	//2 for H/W reason
	//if( suspend_latch_setting(p_state , 0) ) goto error;		//3 suspend_latch_en = 1, resume = 0					

	//if( set_demod_power(p_state , 0) ) goto error;		//3	turn OFF demod power

	deb_info(" -%s \n", __FUNCTION__);

	up(&p_state->i2c_repeater_mutex);

	return 0;
	
error:
	up(&p_state->i2c_repeater_mutex);
	
mutex_error:
	deb_info(" -%s fail\n", __FUNCTION__);	
	return 1;
}





static int
rtl2832_set_parameters(
	struct dvb_frontend*            fe,
	struct dvb_frontend_parameters*	param
	)
{
	struct rtl2832_state*       p_state = fe->demodulator_priv;
	struct dvb_ofdm_parameters*	p_ofdm_param= &param->u.ofdm;
	unsigned long               frequency = param->frequency;
	int							bandwidth_mode;
	int                         is_signal_present;
	int                         reset_needed;
	unsigned char               data;
       
	//TUNER_MODULE*               pTuner;
    DTMB_DEMOD_MODULE *         pDemod2836;
	DVBT_DEMOD_MODULE *         pDemod2832;
    //MXL5007T_EXTRA_MODULE *     pTunerExtra; 

    if( p_state->pNim == NULL)
    {
    	deb_info(" %s pNim = NULL \n", __FUNCTION__);
    	return -1;
    }
    
    if(p_state->demod_type == RTL2836 && p_state->pNim2836 == NULL )
    {
    	deb_info(" %s pNim2836 = NULL \n", __FUNCTION__);
    	return -1;
    }

	if( down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;
	
	deb_info(" +%s frequency = %lu , bandwidth = %u\n", __FUNCTION__, frequency ,p_ofdm_param->bandwidth);

	if(p_state->demod_type == RTL2832)
	{
		if (check_dvbt_reset_parameters( p_state , frequency , p_ofdm_param->bandwidth, &reset_needed) ) 
		    goto error;

		if (reset_needed == 0 )
		{
            up(&p_state->i2c_repeater_mutex);		
			return 0;
		}

		switch (p_ofdm_param->bandwidth) 
        {
		case BANDWIDTH_6_MHZ:
		    bandwidth_mode = DVBT_BANDWIDTH_6MHZ; 	
		    break;
		
		case BANDWIDTH_7_MHZ:
		    bandwidth_mode = DVBT_BANDWIDTH_7MHZ;
		    break;
		
		case BANDWIDTH_8_MHZ:
        default:
		    bandwidth_mode = DVBT_BANDWIDTH_8MHZ;	
		    break;
        }
		
		if ( p_state->pNim->SetParameters( p_state->pNim,  frequency , bandwidth_mode ) ) goto error; 

		if ( p_state->pNim->IsSignalPresent( p_state->pNim, &is_signal_present) ) goto error;
		deb_info("  %s : ****** Signal Present = %d ******\n", __FUNCTION__, is_signal_present);

		p_state->is_frequency_valid = 1;

		
	}
	else if(p_state->demod_type == RTL2836)
	{
		pDemod2836 =  p_state->pNim2836->pDemod;
		pDemod2832 = p_state->pNim->pDemod;

		//if ( check_dtmb_reset_parameters( p_state , frequency ,  &reset_needed) ) goto error;
		//if( reset_needed == 0 )
		//{
		//	mutex_unlock(&p_state->i2c_repeater_mutex);		
		//	return 0;
		//}

              deb_info(("%s:  RTL2836 Hold Stage=9\n"),__FUNCTION__);	
		if(read_rtl2836_demod_register(p_state->d, RTL2836_DEMOD_ADDR,  PAGE_3,  0xf8,  &data, LEN_1_BYTE))  goto error;
		data &=(~BIT_0_MASK);  //reset Reg_present
		data &=(~BIT_1_MASK);  //reset Reg_lock
		if(write_rtl2836_demod_register(p_state->d, RTL2836_DEMOD_ADDR,  PAGE_3,  0xf8,  &data, LEN_1_BYTE))  goto error;
		
		//3 + RTL2836 Hold Stage=9
		data = 0x29;
		if(write_rtl2836_demod_register(p_state->d, RTL2836_DEMOD_ADDR,  PAGE_3,  0x4d,  &data, LEN_1_BYTE))  goto error;
		
		data = 0xA5;
		if(write_rtl2836_demod_register(p_state->d, RTL2836_DEMOD_ADDR,  PAGE_3,  0x4e,  &data, LEN_1_BYTE))  goto error;
		
		data = 0x94;
		if(write_rtl2836_demod_register(p_state->d, RTL2836_DEMOD_ADDR,  PAGE_3,  0x4f,  &data, LEN_1_BYTE))  goto error;
		//3 -RTL2836 Hold Stage=9
	

		// Enable demod DVBT_IIC_REPEAT.
	       if(pDemod2832->SetRegBitsWithPage(pDemod2832,  DVBT_IIC_REPEAT, 0x1) )   goto error;

		if ( p_state->pNim2836->SetParameters( p_state->pNim2836,  frequency)) 	goto error; //no bandwidth setting	

		// Disable demod DVBT_IIC_REPEAT.
	       if(pDemod2832->SetRegBitsWithPage(pDemod2832, DVBT_IIC_REPEAT, 0x0))  goto error;

		if(pDemod2832->SoftwareReset(pDemod2832))//2832 swreset
			goto error;

		p_state->is_frequency_valid = 1;
		if( rtl2836_scan_procedure(p_state))  goto error; 
	}


//#if(UPDATE_FUNC_ENABLE_2832 == 0)
	//3 FC0012/E4000 update begin --
	if(p_state->demod_type == RTL2832 && (p_state->tuner_type == RTL2832_TUNER_TYPE_FC0012 || p_state->tuner_type == RTL2832_TUNER_TYPE_E4000))
	{
              // Enable demod DVBT_IIC_REPEAT.
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x1) != FUNCTION_SUCCESS)
			goto error;

		// Update tuner LNA gain with RSSI.
		// if( p_state->pNim->UpdateFunction(p_state->pNim))
		//	goto error;
		if(p_state->tuner_type == RTL2832_TUNER_TYPE_FC0012)//fc0012
		{
			if(rtl2832_fc0012_UpdateTunerLnaGainWithRssi(p_state->pNim) != FUNCTION_SUCCESS)
				goto error;
		}
		else//e4000
		{
			if(rtl2832_e4000_UpdateTunerMode(p_state->pNim) != FUNCTION_SUCCESS)
				goto error;
		}
		
		// Disable demod DVBT_IIC_REPEAT.
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x0) != FUNCTION_SUCCESS)
			goto error;
                

		deb_info("%s : fc0012/e4000 update first\n", __FUNCTION__);

		msleep(50);

		// if( p_state->pNim->UpdateFunction(p_state->pNim)) goto error;
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x1) != FUNCTION_SUCCESS)
			goto error;

		// Update tuner LNA gain with RSSI.
		if(p_state->tuner_type == RTL2832_TUNER_TYPE_FC0012)//fc0012
		{
			if(rtl2832_fc0012_UpdateTunerLnaGainWithRssi(p_state->pNim) != FUNCTION_SUCCESS)
				goto error;
		}
		else//e4000
		{
			if(rtl2832_e4000_UpdateTunerMode(p_state->pNim) != FUNCTION_SUCCESS)
				goto error;
		}
		
		// Disable demod DVBT_IIC_REPEAT.
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x0) != FUNCTION_SUCCESS)
			goto error;

		deb_info("%s : fc0012/e4000 update 2nd\n", __FUNCTION__);
	}
	//3 FC0012/E4000 update end --
      
//#endif
	
	p_state->current_frequency = frequency;	
	p_state->current_bandwidth = p_ofdm_param->bandwidth;	

	deb_info(" -%s \n", __FUNCTION__);

	up(&p_state->i2c_repeater_mutex);	

	return 0;

error:	
	up(&p_state->i2c_repeater_mutex);	
	
mutex_error:	
	p_state->current_frequency = 0;
	p_state->current_bandwidth = -1;
	p_state->is_frequency_valid = 0;
	deb_info(" -%s  error end\n", __FUNCTION__);

	return -1;
	
}



static int 
rtl2832_get_parameters(
	struct dvb_frontend*	fe,
	struct dvb_frontend_parameters*	param)
{
	//struct rtl2832_state* p_state = fe->demodulator_priv;
	return 0;
}


static int 
rtl2832_read_status(
	struct dvb_frontend*	fe,
	fe_status_t*	status)
{
	struct rtl2832_state*	p_state = fe->demodulator_priv;
	int	is_lock;			
	unsigned  long ber_num, ber_dem;
	long			snr_num, snr_dem, snr;


	if( p_state->pNim== NULL)
	{
		deb_info(" %s pNim = NULL \n", __FUNCTION__);
		return -1;
	}

	*status = 0;	//3initialize "status"
	
	if( down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;

	if(p_state->demod_type == RTL2832)
	{
	
		if( p_state->pNim->GetBer( p_state->pNim , &ber_num , &ber_dem) ) goto error;

		if (p_state->pNim->GetSnrDb(p_state->pNim, &snr_num, &snr_dem) )  goto error;
	
		if( p_state->pNim->IsSignalLocked(p_state->pNim, &is_lock) ) goto error;
	
		if( is_lock==YES ) *status|= (FE_HAS_CARRIER| FE_HAS_VITERBI| FE_HAS_LOCK| FE_HAS_SYNC| FE_HAS_SIGNAL);

		 snr = snr_num/snr_dem;

		deb_info("%s :******Signal Lock=%d******\n", __FUNCTION__, is_lock);
		deb_info("%s : ber_num = %d\n", __FUNCTION__, (unsigned int)ber_num);	
		deb_info("%s : snr = %d \n", __FUNCTION__, (int)snr);	
	}
	else if(p_state->demod_type == RTL2836)//3Need Change ?
	{
		unsigned char	val;	

		if(read_rtl2836_demod_register(p_state->d, RTL2836_DEMOD_ADDR, PAGE_3, 0xf8, &val, LEN_1_BYTE))
			goto error;
	
		if(val & BIT_1_MASK)	is_lock = YES;
		else					is_lock = NO;
              
              //if(p_state->pNim2836->pDemod->IsSignalLocked(p_state->pNim2836->pDemod, &is_lock))
		//	  	goto error;
              
		if( is_lock==YES ) *status|= (FE_HAS_CARRIER| FE_HAS_VITERBI| FE_HAS_LOCK| FE_HAS_SYNC| FE_HAS_SIGNAL);
              
		deb_info("%s :******Signal Lock=%d******\n", __FUNCTION__, is_lock);	
	}

#if(UPDATE_FUNC_ENABLE_2832 == 0)
	//3 FC0012 tuner update begin ---
	if(p_state->demod_type == RTL2832)
	{
	if( p_state->tuner_type == RTL2832_TUNER_TYPE_FC0012)
	{
		//if( p_state->pNim->UpdateFunction(p_state->pNim)) goto error;//rtl2832_fc0012_UpdateFunction()


              // Enable demod DVBT_IIC_REPEAT.
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x1) != FUNCTION_SUCCESS)
			goto error;

		// Update tuner LNA gain with RSSI.
		if( p_state->pNim->UpdateFunction(p_state->pNim))//rtl2832_fc0012_UpdateTunerLnaGainWithRssi()
			goto error;
		
		// Disable demod DVBT_IIC_REPEAT.
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x0) != FUNCTION_SUCCESS)
			goto error;
		
	}//3   FC0012 tuner update end ---
	//3  e4000 tuner update begin ---
	else if(p_state->tuner_type == RTL2832_TUNER_TYPE_E4000)
	{
	      // Enable demod DVBT_IIC_REPEAT.
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x1) != FUNCTION_SUCCESS)
			goto error;

		// Update tuner mode
		if( p_state->pNim->UpdateFunction(p_state->pNim))
			goto error;
		
		// Disable demod DVBT_IIC_REPEAT.
		if(p_state->pNim->pDemod->SetRegBitsWithPage(p_state->pNim->pDemod, DVBT_IIC_REPEAT, 0x0) != FUNCTION_SUCCESS)
			goto error;

	}//3e4000 tuner update end ---
	}
#endif

	up(&p_state->i2c_repeater_mutex);
	
	return 0;

error:
	up(&p_state->i2c_repeater_mutex);
	
mutex_error:
	return -1;
}


static int 
rtl2832_read_ber(
	struct dvb_frontend*	fe,
	u32*	ber)
{
	struct rtl2832_state* p_state = fe->demodulator_priv;
	unsigned  long ber_num, ber_dem;

	if( p_state->pNim== NULL)
	{
		deb_info(" %s pNim = NULL \n", __FUNCTION__);
		return -1;
	}

	if( down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;

	if(p_state->demod_type == RTL2832)
	{
		if( p_state->pNim->GetBer( p_state->pNim , &ber_num , &ber_dem) ) 
		{
			*ber = 19616;
			goto error;
		}
		*ber =  ber_num;
		deb_info("  %s : ber = 0x%x \n", __FUNCTION__, *ber);
	}
	else if(p_state->demod_type == RTL2836)//read PER
	{	
	       unsigned long Per1, Per2;
		if( p_state->pNim2836 == NULL)
		{
			deb_info(" %s pNim2836 = NULL \n", __FUNCTION__);
			goto error;
		}

		if( p_state->pNim2836->pDemod->GetPer(p_state->pNim2836->pDemod, &Per1, &Per2))
		{
			*ber = 19616;
			goto error;
		}
		*ber  = Per1;
		deb_info("  %s : per = 0x%x \n", __FUNCTION__, *ber);
	}

	up(&p_state->i2c_repeater_mutex);
		
	return 0;
	
error:
	up(&p_state->i2c_repeater_mutex);
	
mutex_error:
	return -1;
}


static int 
rtl2832_read_signal_strength(
	struct dvb_frontend*	fe,
	u16*	strength)
{
	struct rtl2832_state* p_state = fe->demodulator_priv;
	unsigned long		_strength;

	if( p_state->pNim== NULL)
	{
		deb_info(" %s pNim = NULL \n", __FUNCTION__);
		return -1;
	}

    if (down_interruptible(&p_state->i2c_repeater_mutex))	
        goto mutex_error;
    
    if(p_state->demod_type == RTL2832)
    {
        if( p_state->pNim->GetSignalStrength( p_state->pNim ,  &_strength) ) 
        {
        	*strength = 0;
        	goto error;
        }
        
        *strength = (_strength<<8) | _strength;
        
        deb_info("  %s : strength = 0x%x \n", __FUNCTION__, *strength);		
    }
    else if(p_state->demod_type == RTL2836)
    {
        if(p_state->pNim2836 == NULL)
        {
            deb_info(" %s pNim2836 = NULL \n", __FUNCTION__);
            goto error;
        }
        
        if(p_state->pNim2836->pDemod->GetSignalStrength( p_state->pNim2836->pDemod ,  &_strength))// if(p_state->pNim2836->GetSignalStrength( p_state->pNim2836 ,  &_strength) ) 
        {
            *strength = 0;
            goto error;
        }
    
        *strength = (_strength<<8) | _strength;
    
        deb_info("  %s : strength = 0x%x \n", __FUNCTION__, *strength);	
    }
        
    up(&p_state->i2c_repeater_mutex);
    
    return 0;
	
error:
	up(&p_state->i2c_repeater_mutex);
	
mutex_error:
	return -1;
}


static int 
rtl2832_read_signal_quality(
	struct dvb_frontend*	fe,
	u32*	quality)
{
	struct rtl2832_state* p_state = fe->demodulator_priv;
	unsigned long		_quality;

	if( p_state->pNim== NULL)
	{
		deb_info(" %s pNim = NULL \n", __FUNCTION__);
		return -1;
	}

	if (down_interruptible(&p_state->i2c_repeater_mutex))	goto mutex_error;

	if(p_state->demod_type == RTL2832)
	{
		if ( p_state->pNim->GetSignalQuality( p_state->pNim ,  &_quality) )
		{
			*quality  = 0;		
			goto error;
		}
	}
	else if(p_state->demod_type == RTL2836)
	{
		if( p_state->pNim2836 ==  NULL)
	       {
	       	deb_info(" %s pNim2836 = NULL \n", __FUNCTION__);
		       goto error;
	       }
		
		if ( p_state->pNim2836->pDemod->GetSignalQuality( p_state->pNim2836->pDemod ,  &_quality) )//if ( p_state->pNim->GetSignalQuality( p_state->pNim ,  &_quality) )
		{
			*quality  = 0;		
			goto error;
		}
	}

	*quality = _quality;
	
	deb_info("  %s : quality = 0x%x \n", __FUNCTION__, *quality);	

	up(&p_state->i2c_repeater_mutex);
	
	return 0;
	
error:
	up(&p_state->i2c_repeater_mutex);
	
mutex_error:
	return -1;	
}



static int 
rtl2832_read_snr(
	struct dvb_frontend*	fe,
	u16*	snr)
{
	struct rtl2832_state* p_state = fe->demodulator_priv;
	long snr_num = 0;
	long snr_dem = 0;
	long _snr= 0;

	if( p_state->pNim== NULL)
	{
		deb_info(" %s pNim = NULL \n", __FUNCTION__);
		return -1;
	}

	if (down_interruptible(&p_state->i2c_repeater_mutex) )	goto mutex_error;

       if(p_state->demod_type == RTL2832)
       {
		if (p_state->pNim->GetSnrDb(p_state->pNim, &snr_num, &snr_dem) ) 
		{
			*snr = 0;
			goto error;
		}

		_snr = snr_num / snr_dem;

		if( _snr < 0 ) _snr = 0;

		*snr = _snr;
        }
        else if(p_state->demod_type == RTL2836)
        {
        	if( p_state->pNim2836 == NULL)
		{
			deb_info(" %s pNim2836 = NULL \n", __FUNCTION__);
			goto error;
		}
			
		if(p_state->pNim2836->pDemod->GetSnrDb(p_state->pNim2836->pDemod, &snr_num, &snr_dem))
		{
			*snr = 0;
			goto error;
		}
		*snr = snr_num>>2;
        }  

	deb_info("  %s : snr = %d \n", __FUNCTION__, *snr);	

	up(&p_state->i2c_repeater_mutex);
	
	return 0;
	
error:
	up(&p_state->i2c_repeater_mutex);
	
mutex_error:
	return -1;
}



static int 
rtl2832_get_tune_settings(
	struct dvb_frontend*	fe,
	struct dvb_frontend_tune_settings*	fe_tune_settings)
{
	deb_info("  %s : Do Nothing\n", __FUNCTION__);	
	fe_tune_settings->min_delay_ms = 1000;
	return 0;
}



static struct dvb_frontend_ops rtl2832_ops;

struct dvb_frontend* rtl2832u_fe_attach(struct dvb_usb_device *d)
{

	struct rtl2832_state*       p_state= NULL;


	deb_info("+%s\n", __FUNCTION__);
	 
	//3 linux fe_attach  necessary setting
	/*allocate memory for the internal state */
	p_state = kmalloc(sizeof(struct rtl2832_state), GFP_KERNEL);	
	if (p_state == NULL) goto error;
	memset(p_state,0,sizeof(*p_state));

	p_state->is_mt2266_nim_module_built = 0; //initialize is_mt2266_nim_module_built 
	p_state->is_initial = 0;			//initialize is_initial 
	p_state->is_frequency_valid = 0;
	p_state->d = d;

       p_state->demod_ask_type = demod_default_type;
	p_state->demod_type = RTL2832;
	p_state->demod_support_type = 0;
    
	/* setup the state */
	p_state->ops  = rtl2832_ops;
	p_state->frontend.ops = &p_state->ops;	
	memset(&p_state->fep, 0, sizeof(struct dvb_frontend_parameters));
	
 	/* create dvb_frontend */
	p_state->frontend.demodulator_priv = p_state;

#if UPDATE_FUNC_ENABLE_2836
	INIT_WORK(&(p_state->update2836_procedure_work), rtl2836_update_function, p_state);
#endif

#if UPDATE_FUNC_ENABLE_2832
	INIT_WORK(&(p_state->update2832_procedure_work), rtl2832_update_functions, p_state);
#endif
	init_MUTEX(&p_state->i2c_repeater_mutex);
	
	deb_info("-%s\n", __FUNCTION__);
	return &p_state->frontend;

error:	
	return NULL;	


}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// rtl2832 ops
///////////////////////////////////////////////////////////////////////////////////////////////////////


static struct dvb_frontend_ops rtl2832_ops = {
    .info = {
        .name               = "Realtek RTL2832 DVB-T  RTL2836 DTMB",
        .type               = FE_OFDM,
        .frequency_min      = 174000000,
        .frequency_max      = 862000000,
        .frequency_stepsize = 166667,
        .caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 |
            FE_CAN_FEC_3_4 | FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 |
            FE_CAN_FEC_AUTO |
            FE_CAN_QPSK | FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
            FE_CAN_TRANSMISSION_MODE_AUTO | FE_CAN_GUARD_INTERVAL_AUTO |
            FE_CAN_HIERARCHY_AUTO | FE_CAN_RECOVER |
            FE_CAN_MUTE_TS
    },

    .init                 = rtl2832_init,
    .release              = rtl2832_release,
    .sleep                = rtl2832_sleep,
    .set_frontend         = rtl2832_set_parameters,
    .get_frontend         = rtl2832_get_parameters,
    .get_tune_settings    = rtl2832_get_tune_settings,
    .read_status          = rtl2832_read_status,
    .read_ber             = rtl2832_read_ber,
    .read_signal_strength =	rtl2832_read_signal_strength,
    .read_snr             = rtl2832_read_snr,
    .read_ucblocks        = rtl2832_read_signal_quality,    
};



/* DTMB related */

static int 
check_dtmb_support(
		struct rtl2832_state* p_state)
{

	int status;
	unsigned char  buf[LEN_2_BYTE];

	deb_info(" +%s \n", __FUNCTION__);

	set_demod_2836_power(p_state, 1); //on

	status = read_rtl2836_demod_register(p_state->d,  RTL2836_DEMOD_ADDR,  PAGE_5,  0x10,  buf,  LEN_2_BYTE);

	if(!status && ( buf[0]==0x04 ) && ( buf[1]==0x00 ))
	{
		p_state->demod_support_type |=  SUPPORT_DTMB_MODE;
		deb_info(" -%s  RTL2836 on broad.....\n", __FUNCTION__);
	}
	else
	{
		p_state->demod_support_type &= (~SUPPORT_DTMB_MODE);
		deb_info(" -%s  RTL2836 NOT FOUND.....\n", __FUNCTION__);
	}

       set_demod_2836_power(p_state, 0); //off
	   
	//3 Always support DVBT
	p_state->demod_support_type |= SUPPORT_DVBT_MODE;

   	return 0;
	
}



static int
set_demod_2836_power(
		struct rtl2832_state* p_state, 
		int  onoff)
{

	int data;
	unsigned char datachar;

	deb_info(" +%s  onoff = %d\n", __FUNCTION__, onoff);

	//2 First RTL2836 Power ON 
	
	if( p_state->tuner_type == RTL2832_TUNER_TYPE_MXL5007T)
	{
		//3 a. Set GPIO 0 LOW
	      if( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data)) goto error;
	      data &= ~(BIT0); // set GPIO0 low
	      if( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, data)) goto error;				
	}
	else if(p_state->tuner_type == RTL2832_TUNER_TYPE_FC2580)
	{
 					
		//3 b. Set GPIO 5 LOW
		if( gpio5_output_enable_direction(p_state))  goto error;

		if( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data) ) goto error;
		data &= ~(BIT5); // set GPIO5 low
		if( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, data) ) goto error;
	}

	if(onoff)
	{
		//3 2. RTL2836 AGC = 1 
		if ( read_rtl2836_demod_register(p_state->d,  RTL2836_DEMOD_ADDR,  PAGE_0, 0x01, &datachar,  LEN_1_BYTE)) goto error;
		datachar |=BIT_2_MASK;
		datachar &=(~BIT_3_MASK);
		if(write_rtl2836_demod_register(p_state->d,  RTL2836_DEMOD_ADDR,   PAGE_0, 0x01, &datachar, LEN_1_BYTE))  goto error;	
	}
	else
	{
		
		//3 2. RTL2836 AGC = 0

		if ( read_rtl2836_demod_register(p_state->d,  RTL2836_DEMOD_ADDR,  PAGE_0, 0x01, &datachar,  LEN_1_BYTE)) goto error;
		datachar &=(~BIT_2_MASK);
		datachar &=(~BIT_3_MASK);	
		if(write_rtl2836_demod_register(p_state->d,  RTL2836_DEMOD_ADDR,   PAGE_0, 0x01, &datachar, LEN_1_BYTE))  goto error;	
	
		
 		//3 3. RTL2836 Power OFF 
		if( p_state->tuner_type == RTL2832_TUNER_TYPE_MXL5007T)
		{
			//3 4.a. Set GPIO 0 HIGH
			if( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data)) goto error;
			data |= BIT0; // set GPIO0 high
		       if( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, data)) goto error;
		}
		else if(p_state->tuner_type == RTL2832_TUNER_TYPE_FC2580)
		{
	 					
			//3 4.b. Set GPIO 5 HIGH
			if( read_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, &data)) goto error;
			data |= BIT5; // set GPIO5 high
		       if( write_usb_sys_register(p_state, RTD2831_RMAP_INDEX_SYS_GPIO_OUTPUT_VAL, data)) goto error;
		}		
	}
	

	deb_info(" -%s  onoff = %d\n", __FUNCTION__, onoff);
	return 0;

error:
	deb_info(" -%s  onoff = %d fail\n", __FUNCTION__, onoff);
	return 1;
		
}


static int
demod_init_setting(
		struct rtl2832_state * p_state
		)
{

	unsigned char data;
	unsigned char buf[LEN_2_BYTE];
	
	deb_info(" +%s\n", __FUNCTION__);

	switch(p_state->demod_type)
	{
	case RTL2832:	
		{
			deb_info("%s for RTL2832\n", __FUNCTION__);
			//3 1. Set IF_AGC Internal    IF_AGC_MAN 0x0c
		       if(read_rtl2832_demod_register(p_state->d,  RTL2832_DEMOD_ADDR, PAGE_1, 0x0c, &data,  LEN_1_BYTE)) goto error;
			data &= (~BIT6);
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR, PAGE_1, 0x0c, &data, LEN_1_BYTE)) goto error;


		      /*if(!context->DAB_AP_running)
			{
				//3 2. Set PID filter to reject null packet(pid = 0x1fff) 
				context->pid_filter_mode = REJECT_MODE;	
				ULONG reject_pid[1] = {0x1fff}; 
				Status = PidFilterToRejectMode(context, reject_pid, 1);
				if(!NT_SUCCESS(Status))	goto error;
			}*/
			
		}
		break;		
	case RTL2836:
		{

			deb_info("%s for RTL2836\n", __FUNCTION__);
			//3 1. Set IF_AGC Manual and Set IF_AGC MAX VAL
			buf[0]=0x5F;
 			buf[1]=0xFF;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_1,  0x0c,  buf, LEN_2_BYTE)) goto error;
	
			//3 2. PIP Setting
			data = 0xe8;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x21, &data, LEN_1_BYTE)) goto error;
		
                     data = 0x60;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x61, &data, LEN_1_BYTE)) goto error;
		
			data = 0x18;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0xbc, &data, LEN_1_BYTE)) goto error;
		
			data = 0x00;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x62, &data, LEN_1_BYTE)) goto error;
		
                     data = 0x00;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x63, &data, LEN_1_BYTE)) goto error;
		
                     data = 0x00;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x64, &data, LEN_1_BYTE)) goto error;                            
			
                     data = 0x00;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x65, &data, LEN_1_BYTE)) goto error;

			//3 +PIP filter Reject = 0x1FFF
                    	data = 0x01;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x22, &data, LEN_1_BYTE)) goto error;

			data = 0x1f;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x26, &data, LEN_1_BYTE)) goto error;

			data = 0xff;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_0, 0x27, &data, LEN_1_BYTE)) goto error;
			//3 -PIP filter Reject = 0x1FFF

			data = 0x7f;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_1, 0x92, &data, LEN_1_BYTE)) goto error;

			data = 0xf7;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_1, 0x93, &data, LEN_1_BYTE)) goto error;
			
			data = 0xff;
			if(write_rtl2832_demod_register(p_state->d, RTL2832_DEMOD_ADDR,  PAGE_1, 0x94, &data, LEN_1_BYTE)) goto error;
		}
		break;
	}

	deb_info(" -%s\n", __FUNCTION__);
	return 0;
	
error:
	deb_info(" -%s error \n", __FUNCTION__);
	return 1;

}


static int 
rtl2836_scan_procedure(
	struct rtl2832_state * p_state)
{
	unsigned char val;
	int wait_num = 0;
	unsigned long Per1, Per2;
	long Data1,Data2,Snr;
	DTMB_DEMOD_MODULE	*pDtmbDemod;	
	struct dvb_usb_device* dev;


	pDtmbDemod = p_state->pNim2836->pDemod;
	dev = p_state->d;

	deb_info(" +%s\n", __FUNCTION__);

       //3 Check signal present
	wait_num = 0;
	msleep(50); // Wait 0.05s 
       if(read_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_6, 0x53, &val, LEN_1_BYTE))  goto error;
	deb_info("%s Signel Present = 0x %x\n", __FUNCTION__, val);
	while((wait_num<3)&& (!(val&BIT_0_MASK)))
	{
		msleep(50); // Wait 0.05s
		if(read_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_6, 0x53, &val, LEN_1_BYTE))  goto error;
		deb_info("%s Signel Present = 0x %x\n", __FUNCTION__, val);
		wait_num++;
	}
	
	if(val&BIT_0_MASK)
	{
		//3  Write signal present 
		if(read_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0xf8, &val, LEN_1_BYTE))  goto error;
		val |= BIT_0_MASK;  //set Reg_present
		if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0xf8, &val, LEN_1_BYTE))  goto error;

              //3+ RTL2836 Release Stage=9
		val = 0x49;
		if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0x4d, &val, LEN_1_BYTE)) goto error;
		val = 0x29;
		if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0x4e, &val, LEN_1_BYTE)) goto error;
		val = 0x95;
		if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0x4f, &val, LEN_1_BYTE)) goto error;
		//3 -RTL2836 Release Stage=9
		deb_info("%s RTL2836 Release Stage 9\n", __FUNCTION__);


		//3 Check signal lock		
		pDtmbDemod->GetPer(pDtmbDemod, &Per1, &Per2);
		deb_info("%s --***GetPer = %d***\n", __FUNCTION__, (int)Per1); 	
	
		pDtmbDemod->GetSnrDb(pDtmbDemod, &Data1, &Data2);            
		Snr = Data1>>2;
		deb_info("%s --***SNR= %d***\n", __FUNCTION__, (int)Snr); 	
		
		wait_num = 0;
		while(wait_num<30 )
		{
			if((Per1<1) && (Snr>0) && (Snr<40) )
			{
				if(read_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0xf8, &val, LEN_1_BYTE)) goto error;
				val |= BIT_1_MASK;  //set Reg_signal
    				if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0xf8, &val, LEN_1_BYTE)) goto error;
				
				deb_info("%s Signal Lock................\n", __FUNCTION__); 	
				break;
			}

			msleep(100); // Wait 0.1s
			pDtmbDemod->GetPer(pDtmbDemod,&Per1,&Per2);
			deb_info("%s --***GetPer = %d***\n", __FUNCTION__, (int)Per1); 			

			pDtmbDemod->GetSnrDb(pDtmbDemod,&Data1,&Data2);            
			Snr = Data1>>2;
			deb_info("%s --***SNR= %d***\n", __FUNCTION__, (int)Snr); 	
			wait_num++;
		}

              if(! ((Per1<1) && (Snr>0) && (Snr<40)))
              {
              	if(read_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR,  PAGE_3,  0xf8,  &val, LEN_1_BYTE))  goto error;
			val &=(~BIT_1_MASK);  //reset Reg_lock
			if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR,  PAGE_3,  0xf8,  &val, LEN_1_BYTE)) goto error;
              }		
	}
	else
	{
		if(read_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR,  PAGE_3,  0xf8,  &val, LEN_1_BYTE)) goto error;
		val &=(~BIT_0_MASK);  //reset Reg_present
		val &=(~BIT_1_MASK);  //reset Reg_lock
		if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR,  PAGE_3,  0xf8,  &val, LEN_1_BYTE)) goto error;
		
		//3 + RTL2836 Release Stage=9
		val = 0x49;
		if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0x4d, &val, LEN_1_BYTE)) goto error;
		val = 0x29;
              if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0x4e, &val, LEN_1_BYTE)) goto error;
		val = 0x95;
		if(write_rtl2836_demod_register(dev, RTL2836_DEMOD_ADDR, PAGE_3, 0x4f, &val, LEN_1_BYTE)) goto error;
		//3 -RTL2836 Release Stage=9

		deb_info("%s RTL2836 Release Stage 9\n", __FUNCTION__);	
	}

	deb_info(" -%s\n", __FUNCTION__);
	return 0;
error:

	deb_info(" +%s error\n", __FUNCTION__);
	return -1;
}


static int
build_nim_module(
		struct rtl2832_state*  p_state)
{
	MT2266_EXTRA_MODULE	*p_mt2266_extra;

      deb_info(" +%s\n", __FUNCTION__);

      //3 Buile 2832 nim module
	if( p_state->tuner_type == RTL2832_TUNER_TYPE_MT2266)
	{
		//3 Build RTL2832 MT2266 NIM module.
		BuildRtl2832Mt2266Module(
			&p_state->pNim,
			&p_state->DvbtNimModuleMemory,
			&p_state->Rtl2832Mt2266ExtraModuleMemory,

			9,											// Maximum I2C reading byte number is 9.
			8,											// Maximum I2C writing byte number is 8.
			NULL,//custom_i2c_read,							// Employ CustomI2cRead() as basic I2C reading function.
			NULL,//custom_i2c_write,							// Employ CustomI2cWrite() as basic I2C writing function.
			custom_wait_ms,							// Employ CustomWaitMs() as basic waiting function.

			&p_state->Rtl2832ExtraModuleMemory,			// Employ RTL2832 extra module for RTL2832 module.
			RTL2832_DEMOD_ADDR,						// The RTL2832 I2C device address is 0x20 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ,					// The RTL2832 crystal frequency is 28.8 MHz.
			RTL2832_APPLICATION_DONGLE,					// The RTL2832 application mode is STB mode.
			TS_INTERFACE_PARALLEL,
			200,											// The RTL2832 update function reference period is 200 millisecond
			OFF,											// The RTL2832 Function 1 enabling status is on.

			&p_state->Mt2266ExtraModuleMemory,			// Employ MT2266 extra module for MT2266 module.
			MT2266_TUNER_ADDR							// The MT2266 I2C device address is 0xc0 in 8-bit format.
			);

		p_mt2266_extra = (MT2266_EXTRA_MODULE *)(p_state->pNim->pTuner->pExtra);

		if(p_mt2266_extra->OpenHandle(p_state->pNim->pTuner))
			deb_info("%s : MT2266 Open Handle Failed....\n", __FUNCTION__);
		
		p_state->is_mt2266_nim_module_built = 1;
	
	}
	else if( p_state->tuner_type == RTL2832_TUNER_TYPE_FC2580)
	{

		//3Build RTL2832 FC2580 NIM module.
		BuildRtl2832Fc2580Module(
			&p_state->pNim,
			&p_state->DvbtNimModuleMemory,

			9,											// Maximum I2C reading byte number is 9.
			8,											// Maximum I2C writing byte number is 8.
			custom_i2c_read,							// Employ CustomI2cRead() as basic I2C reading function.
			custom_i2c_write,							// Employ CustomI2cWrite() as basic I2C writing function.
			custom_wait_ms,							// Employ CustomWaitMs() as basic waiting function.

			&p_state->Rtl2832ExtraModuleMemory,			// Employ RTL2832 extra module for RTL2832 module.
			RTL2832_DEMOD_ADDR,						// The RTL2832 I2C device address is 0x20 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ,					// The RTL2832 crystal frequency is 28.8 MHz.		
			RTL2832_APPLICATION_DONGLE,					// The RTL2832 application mode is STB mode.
			TS_INTERFACE_PARALLEL,
			200,											// The RTL2832 update function reference period is 200 millisecond
			OFF,											// The RTL2832 Function 1 enabling status is on.

			&p_state->Fc2580ExtraModuleMemory,			// Employ FC2580 extra module for FC2580 module.
			FC2580_TUNER_ADDR,							// The FC2580 I2C device address is 0xac in 8-bit format.
			CRYSTAL_FREQ_16384000HZ,					// The FC2580 crystal frequency is 16.384 MHz.			
			FC2580_AGC_INTERNAL						// The FC2580 AGC mode is external AGC mode.
			);
		
	}
	else if( p_state->tuner_type == RTL2832_TUNER_TYPE_TUA9001)
	{

		//3Build RTL2832 TUA9001 NIM module.
		BuildRtl2832Tua9001Module(
			&p_state->pNim,
			&p_state->DvbtNimModuleMemory,

			9,											// Maximum I2C reading byte number is 9.
			8,											// Maximum I2C writing byte number is 8.
			custom_i2c_read,							// Employ CustomI2cRead() as basic I2C reading function.
			custom_i2c_write,							// Employ CustomI2cWrite() as basic I2C writing function.
			custom_wait_ms,							// Employ CustomWaitMs() as basic waiting function.

			&p_state->Rtl2832ExtraModuleMemory,			// Employ RTL2832 extra module for RTL2832 module.
			RTL2832_DEMOD_ADDR,						// The RTL2832 I2C device address is 0x20 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ,					// The RTL2832 crystal frequency is 28.8 MHz.		
			RTL2832_APPLICATION_DONGLE,					// The RTL2832 application mode is STB mode.
			TS_INTERFACE_PARALLEL,
			200,											// The RTL2832 update function reference period is 200 millisecond
			OFF,											// The RTL2832 Function 1 enabling status is on.

			&p_state->TUA9001ExtraModuleMemory,			// Employ TUA9001 extra module for FC2580 module.
			TUA9001_TUNER_ADDR							// The TUA9001 I2C device address is 0xc0 in 8-bit format.
			);
		
	}
	else if( p_state->tuner_type == RTL2832_TUNER_TYPE_MXL5007T)
	{

		//3Build RTL2832 MXL5007 NIM module.
		BuildRtl2832Mxl5007tModule(
			&p_state->pNim,
			&p_state->DvbtNimModuleMemory,

			9,											// Maximum I2C reading byte number is 9.
			8,											// Maximum I2C writing byte number is 8.
			custom_i2c_read,							// Employ CustomI2cRead() as basic I2C reading function.
			custom_i2c_write,							// Employ CustomI2cWrite() as basic I2C writing function.
			custom_wait_ms,							// Employ CustomWaitMs() as basic waiting function.

			&p_state->Rtl2832ExtraModuleMemory,			// Employ RTL2832 extra module for RTL2832 module.
			RTL2832_DEMOD_ADDR,						// The RTL2832 I2C device address is 0x20 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ,					// The RTL2832 crystal frequency is 28.8 MHz.		
			RTL2832_APPLICATION_DONGLE,					// The RTL2832 application mode is STB mode.
			TS_INTERFACE_PARALLEL,
			200,											// The RTL2832 update function reference period is 200 millisecond
			OFF,											// The RTL2832 Function 1 enabling status is on.

			&p_state->MXL5007TExtraModuleMemory,			// Employ MxL5007T  extra module for MxL5007T  module.
			MXL5007T_BASE_ADDRESS,										// The MxL5007T I2C device address is 0xc0 in 8-bit format.
			CRYSTAL_FREQ_16000000HZ,					// The MxL5007T Crystal frequency is 16.0 MHz.
                     MXL5007T_LOOP_THROUGH_DISABLE,        	// The MxL5007T loop-through mode is disabled.
		       MXL5007T_CLK_OUT_DISABLE,					// The MxL5007T clock output mode is disabled.
			MXL5007T_CLK_OUT_AMP_0						// The MxL5007T clock output amplitude is 0.
			);		
	}
	else if(p_state->tuner_type == RTL2832_TUNER_TYPE_FC0012)
	{
		//3Build RTL2832 FC0012 NIM module.
		BuildRtl2832Fc0012Module(
			&p_state->pNim,
			&p_state->DvbtNimModuleMemory,
			&p_state->Rtl2832FC0012ExtraModuleMemory,
			
			9,								// Maximum I2C reading byte number is 9.
			8,								// Maximum I2C writing byte number is 8.
			custom_i2c_read,					// Employ CustomI2cRead() as basic I2C reading function.
			custom_i2c_write,					// Employ CustomI2cWrite() as basic I2C writing function.
			custom_wait_ms,					// Employ CustomWaitMs() as basic waiting function.
			
			&p_state->Rtl2832ExtraModuleMemory,		// Employ RTL2832 extra module for RTL2832 module.
			RTL2832_DEMOD_ADDR,				// The RTL2832 I2C device address is 0x20 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ,		// The RTL2832 crystal frequency is 28.8 MHz.
			RTL2832_APPLICATION_DONGLE,		// The RTL2832 application mode is STB mode.
			TS_INTERFACE_PARALLEL,			// The RTL2832 TS interface mode is serial.
			200,//1000,						// The RTL2832 update function reference period is 50 millisecond
			OFF,							// The RTL2832 Function 1 enabling status is on.
			
			&p_state->FC0012ExtraModuleMemory,		// Employ FC0012 extra module for FC0012 module.
			FC0012_BASE_ADDRESS,			// The FC0012 I2C device address is 0xc6 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ       //CRYSTAL_FREQ_36000000HZ			// The FC0012 crystal frequency is 36.0 MHz.
			);

	}
	else if(p_state->tuner_type == RTL2832_TUNER_TYPE_E4000)
	{
		//3 Build RTL2832 E4000 NIM module
		BuildRtl2832E4000Module(
			&p_state->pNim,
			&p_state->DvbtNimModuleMemory,
			&p_state->Rtl2832E4000ExtraModuleMemory,		// Employ RTL2832 E4000 extra module.

			9,									// Maximum I2C reading byte number is 9.
			8,									// Maximum I2C writing byte number is 8.
			custom_i2c_read,						// Employ CustomI2cRead() as basic I2C reading function.
			custom_i2c_write,						// Employ CustomI2cWrite() as basic I2C writing function.
			custom_wait_ms,						// Employ CustomWaitMs() as basic waiting function.

			&p_state->Rtl2832ExtraModuleMemory,			// Employ RTL2832 extra module.
			RTL2832_DEMOD_ADDR,						// The RTL2832 I2C device address is 0x20 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ,			// The RTL2832 crystal frequency is 28.8 MHz.
			RTL2832_APPLICATION_DONGLE,			// The RTL2832 application mode is STB mode.
			TS_INTERFACE_PARALLEL,				// The RTL2832 TS interface mode is serial.
			200, //500, //1000									// The RTL2832 update function reference period is 50 millisecond
			OFF,									// The RTL2832 Function 1 enabling status is on.

			&p_state->E4000ExtraModuleMemory,			// Employ E4000 extra module.
			E4000_BASE_ADDRESS,                             // The E4000 I2C device address is 0xc8 in 8-bit format.
			CRYSTAL_FREQ_28800000HZ //CRYSTAL_FREQ_26000000HZ				// The E4000 crystal frequency is 26 MHz.
		);		
	}
	else
	{
		deb_info(" -%s : Unknown tuner on board...\n", __FUNCTION__);		
		goto error;
	}
	//Set user defined data pointer of base interface structure for custom basic functions.
       p_state->pNim->pBaseInterface->SetUserDefinedDataPointer(p_state->pNim->pBaseInterface, p_state->d );

	
	//3 Build 2836 nim module
	if(p_state->demod_support_type & SUPPORT_DTMB_MODE)
	{
		if( p_state->tuner_type == RTL2832_TUNER_TYPE_FC2580)
	       {
	       	//3Build RTL2836 FC2580 NIM module.
	              BuildRtl2836Fc2580Module(
	              	&p_state->pNim2836,
	              	&p_state->DtmbNimModuleMemory,
				9,								// Maximum I2C reading byte number is 9.
				8,								// Maximum I2C writing byte number is 8.
				custom_i2c_read,					// Employ CustomI2cRead() as basic I2C reading function.
				custom_i2c_write,					// Employ CustomI2cWrite() as basic I2C writing function.
				custom_wait_ms,					// Employ CustomWaitMs() as basic waiting function.

				&p_state->Rtl2836ExtraModuleMemory,		// Employ RTL2836 extra module for RTL2836 module.
				RTL2836_DEMOD_ADDR,					// The RTL2836 I2C device address is 0x3e in 8-bit format.
				CRYSTAL_FREQ_27000000HZ,		// The RTL2836 crystal frequency is 27.0 MHz.
				TS_INTERFACE_PARALLEL,			// The RTL2836 TS interface mode is serial.
				50,								// The RTL2836 update function reference period is 50 millisecond
				ON,								// The RTL2836 Function 1 enabling status is on.

				&p_state->Fc2580ExtraModuleMemory,		// Employ FC2580 extra module for FC2580 module.
				FC2580_TUNER_ADDR,				// The FC2580 I2C device address is 0xac in 8-bit format.
				CRYSTAL_FREQ_16384000HZ,		// The FC2580 crystal frequency is 16.384 MHz.
				FC2580_AGC_INTERNAL				// The FC2580 AGC mode is internal AGC mode.
		         );
		}
		else if(p_state->tuner_type == RTL2832_TUNER_TYPE_MXL5007T)
		{
			//3 Build RTL2836 MXL5007T NIM module
			BuildRtl2836Mxl5007tModule(
				&p_state->pNim2836,
				&p_state->DtmbNimModuleMemory,
				9,								// Maximum I2C reading byte number is 9.
				8,								// Maximum I2C writing byte number is 8.
				custom_i2c_read,				// Employ CustomI2cRead() as basic I2C reading function.
				custom_i2c_write,				// Employ CustomI2cWrite() as basic I2C writing function.
				custom_wait_ms,				// Employ CustomWaitMs() as basic waiting function.

				&p_state->Rtl2836ExtraModuleMemory,	// Employ RTL2836 extra module for RTL2836 module.
				RTL2836_DEMOD_ADDR,				// The RTL2836 I2C device address is 0x3e in 8-bit format.
				CRYSTAL_FREQ_27000000HZ,		// The RTL2836 crystal frequency is 27.0 MHz.
				TS_INTERFACE_SERIAL,			// The RTL2836 TS interface mode is serial.
				50,								// The RTL2836 update function reference period is 50 millisecond
				ON,								// The RTL2836 Function 1 enabling status is on.

				&p_state->MXL5007TExtraModuleMemory,	// Employ MxL5007T extra module for MxL5007T module.
				MXL5007T_BASE_ADDRESS,			// The MxL5007T I2C device address is 0xc0 in 8-bit format.
				CRYSTAL_FREQ_16000000HZ,		// The MxL5007T Crystal frequency is 16.0 MHz.
				MXL5007T_LOOP_THROUGH_DISABLE,	// The MxL5007T loop-through mode is disabled.
				MXL5007T_CLK_OUT_DISABLE,		// The MxL5007T clock output mode is disabled.
				MXL5007T_CLK_OUT_AMP_0			// The MxL5007T clock output amplitude is 0.
			);
		}
		else
		{
			deb_info(" -%s : Unknown tuner on board...\n", __FUNCTION__);		
		       goto error;
		}

             //Set user defined data pointer of base interface structure for custom basic functions.
             p_state->pNim2836->pBaseInterface->SetUserDefinedDataPointer(p_state->pNim2836->pBaseInterface, p_state->d );
	 }

        deb_info(" -%s\n", __FUNCTION__);
	return 0;
	
error:
	 return 1;
}





