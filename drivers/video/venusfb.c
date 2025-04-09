/*
 * linux/drivers/video/venusfb.c - 
 * 
 *
 * Modes / Resolution:
 *
 *      | 720x480  720x576  1280x720 1920x1080
 *  ----+-------------------------------------
 *  8bit|  0x101    0x201    0x401    0x801   
 * 16bit|  0x102    0x202    0x402    0x802   
 * 32bit|  0x103    0x204    0x404    0x804   
 *
 * args example:  video=venusfb:0x202 (this is default setting)
 *
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <venus.h>
#include <linux/radix-tree.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/blkpg.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/sched.h>
#include <linux/dcache.h>
#include <linux/auth.h>
#ifdef CONFIG_DEVFS_FS
	#include <linux/devfs_fs_kernel.h>
#endif
#ifdef CONFIG_REALTEK_RPC
	#include <linux/RPCDriver.h>
#endif

#include <platform.h>
#include "venusfb.h"

#define TMPBUFLEN   2048

#define DBG_PRINT(s, args...) printk(s, ##args)

static VENUSFB_MACH_INFO venus_video_info = {
	.pixclock		= 720*480*60,
	.xres			= 720,
	.yres			= 480,
	.xoffset		= 0,
	.yoffset		= 0,
	.bpp			= 16,
	.pitch			= 768*2,
	.videomemory		= NULL,
	.phyAddr		= NULL,
	.videomemorysize	= 768*2*480,
	.videomemorysizeorder	= 8, // 2^8 * 4KB = 1MB
	.storageMode		= 0,
#ifdef CONFIG_REALTEK_RPC
	.winID			= 0,
	.mixer2			= 0,
#endif
};

static struct fb_monspecs monspecs __initdata = {
	.hfmin			= 30000,
	.hfmax			= 70000,
	.vfmin			= 50,
	.vfmax			= 65,
};

static struct fb_ops venusfb_ops;

static void reset_video_info(struct venusfb_info *fbi)
{
	const struct venusfb_rgb rgb_8 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 0,  .length = 8, },
		.blue	= { .offset = 0,  .length = 8, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_16 = {
		.red	= { .offset = 11, .length = 5, },
		.green	= { .offset = 5,  .length = 6, },
		.blue	= { .offset = 0,  .length = 5, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_32 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 8,  .length = 8, },
		.blue	= { .offset = 16, .length = 8, },
		.transp = { .offset = 24, .length = 8, },
	};

	fbi->video_info 		= venus_video_info;

	// fill [struct fb_fix_screeninfo] : Non-changeable properties 
	strcpy(fbi->fb.fix.id, "venusfb");
	fbi->fb.fix.type		= FB_TYPE_PACKED_PIXELS;
	fbi->fb.fix.type_aux		= 0;
	fbi->fb.fix.visual		= FB_VISUAL_TRUECOLOR;
	fbi->fb.fix.xpanstep		= 1;
	fbi->fb.fix.ypanstep		= 1;
	fbi->fb.fix.ywrapstep		= 1;
	fbi->fb.fix.line_length 	= venus_video_info.pitch;
	fbi->fb.fix.accel		= FB_ACCEL_NONE;

	// fill [struct fb_var_screeninfo] : This is used to set "feature"
	fbi->fb.var.xres		= venus_video_info.xres;
	fbi->fb.var.xres_virtual	= venus_video_info.xres;
	fbi->fb.var.yres		= venus_video_info.yres;
	fbi->fb.var.yres_virtual	= venus_video_info.yres;
	fbi->fb.var.xoffset		= 0;
	fbi->fb.var.yoffset		= 0;
	fbi->fb.var.bits_per_pixel	= venus_video_info.bpp;
	fbi->fb.var.grayscale		= 0;

	switch(fbi->fb.var.bits_per_pixel) {
	case 8:
		fbi->fb.var.red		= rgb_8.red;
		fbi->fb.var.green	= rgb_8.green;
		fbi->fb.var.blue 	= rgb_8.blue;
		fbi->fb.var.transp 	= rgb_8.transp;
		break;
	default:
	case 16:
		fbi->fb.var.red		= rgb_16.red;
		fbi->fb.var.green	= rgb_16.green;
		fbi->fb.var.blue 	= rgb_16.blue;
		fbi->fb.var.transp 	= rgb_16.transp;
		break;
	case 32:
		fbi->fb.var.red		= rgb_32.red;
		fbi->fb.var.green	= rgb_32.green;
		fbi->fb.var.blue 	= rgb_32.blue;
		fbi->fb.var.transp 	= rgb_32.transp;
		break;
	}

	fbi->fb.var.pixclock		= 1000*1000*1000 / venus_video_info.pixclock; // in pico-second unit
	fbi->fb.var.left_margin		= 0;
	fbi->fb.var.right_margin	= 0;
	fbi->fb.var.upper_margin	= 0;
	fbi->fb.var.lower_margin	= 0;
	fbi->fb.var.hsync_len		= 0;
	fbi->fb.var.vsync_len		= 0;
	fbi->fb.var.sync		= FB_SYNC_EXT;
	fbi->fb.var.nonstd		= 0;
	fbi->fb.var.activate		= FB_ACTIVATE_NOW;
	fbi->fb.var.height		= 1;
	fbi->fb.var.width		= 1;
	fbi->fb.var.accel_flags		= 0;
	fbi->fb.var.vmode		= FB_VMODE_NONINTERLACED;

	// fbops for character device support
	fbi->fb.fbops			= &venusfb_ops;
	fbi->fb.flags			= FBINFO_DEFAULT;
	fbi->fb.monspecs		= monspecs;
	fbi->fb.pseudo_palette		= (void *)fbi->pseudo_palette;
}

static int video_allocMemory(struct venusfb_info *fbi)
{
	if (fbi->video_info.videomemory != 0)
		BUG();

	if (!(fbi->video_info.videomemory = dvr_malloc(PAGE_SIZE << fbi->video_info.videomemorysizeorder))) {
		printk("VenusFB: dvr_malloc(2^%d) failed ..\n", fbi->video_info.videomemorysizeorder);
		return 1;
	}
	printk(" ### alloc Memory %p (2^%d)...\n", fbi->video_info.videomemory, fbi->video_info.videomemorysizeorder);

	if (is_mars_cpu() || is_jupiter_cpu())
		fbi->video_info.phyAddr = (void *)virt_to_phys(fbi->video_info.videomemory);
	else
		fbi->video_info.phyAddr = (void *)(virt_to_phys(fbi->video_info.videomemory) | SB2_GRAPHIC_OFFSET);
	memset(fbi->video_info.videomemory, 0, fbi->video_info.videomemorysize);
	dma_cache_wback_inv((unsigned long)fbi->video_info.videomemory, fbi->video_info.videomemorysize);

	fbi->map_vir_addr               = fbi->video_info.videomemory;
	fbi->map_phy_addr               = fbi->video_info.phyAddr;
	fbi->map_size                   = fbi->video_info.videomemorysize;

	fbi->fb.fix.smem_start          = (unsigned long)fbi->video_info.phyAddr;
	fbi->fb.fix.smem_len            = PAGE_ALIGN(fbi->video_info.videomemorysize);

	fbi->fb.screen_base             = (char __iomem *)(UNCAC_ADDR((unsigned int)fbi->video_info.videomemory));
	fbi->fb.screen_size             = fbi->video_info.videomemorysize;

	// enable the SB2 graphic region
//	*(volatile unsigned long *)SB2_GREG_START_0_reg = fbi->map_phy_addr;
//	*(volatile unsigned long *)SB2_GREG_END_0_reg = fbi->map_phy_addr+fbi->map_size;
//	*(volatile unsigned long *)SB2_GREG_NO_SWAP_reg = SB2_GREG_NO_SWAP_disable(0);

	return 0;
}

static int video_freeMemory(struct venusfb_info *fbi)
{
	if (fbi->video_info.videomemory == 0)
		BUG();

	printk(" ### free Memory %p...\n", fbi->video_info.videomemory);

	// disable the SB2 graphic region
//	*(volatile unsigned long *)SB2_GREG_NO_SWAP_reg = SB2_GREG_NO_SWAP_disable(1);
//	*(volatile unsigned long *)SB2_GREG_START_0_reg = 1;
//	*(volatile unsigned long *)SB2_GREG_END_0_reg = 0;

	dvr_free(fbi->video_info.videomemory);
	fbi->video_info.videomemory = 0;

	return 0;
}

#ifdef CONFIG_REALTEK_RPC
static int video_initDisplayArea(struct venusfb_info *fbi)
{
	VIDEO_RPC_VOUT_CONFIG_VIDEO_STANDARD	*structConfigVideoStandard;
	VIDEO_RPC_VOUT_CONFIG_HDMI		*structConfigHDMI;
	VIDEO_RPC_VOUT_SET_BKGRND		*structSetBKGrnd; 
	VIDEO_RPC_VOUT_CONFIG_GRAPHIC_CANVAS	*structConfigGraphicCanvas;
	struct page *page = 0;
	unsigned long ret;

	printk(" #@# Init Display Area...\n");

	page = alloc_page(GFP_KERNEL);
	printk(" #@# IBuffer: %p...\n", page_address(page));

	// set mixer2 buffer (if needed)...
	if (fbi->video_info.xres != 720) {
		if (fbi->video_info.mixer2 == 0) {
			fbi->video_info.mixer2 = (unsigned long)dvr_malloc(720*576*6);
		
			if (send_rpc_command(RPC_VIDEO, RPCCMD_SET_MIXER2_BUFFER, fbi->video_info.mixer2, 0, &ret))
				goto fail;
			printk(" #@# set mixer2 buffer %x, ret = %lx \n", fbi->video_info.mixer2, ret);
		
			if (ret != S_OK) {
				printk("VenusFB: Set mixer2 buffer fail...\n");
				goto fail;
			}
		} else {
			printk(" #@# use mixer2 buffer %x \n", fbi->video_info.mixer2);
		}
	}

	// config video standard...
	structConfigVideoStandard = (VIDEO_RPC_VOUT_CONFIG_VIDEO_STANDARD *)page_address(page);
	if (fbi->video_info.xres == 720)
		structConfigVideoStandard->standard = htonl(VO_STANDARD_NTSC_M);
	else if (fbi->video_info.xres == 1280)
		structConfigVideoStandard->standard = htonl(VO_STANDARD_HDTV_720P_60);
	else if (fbi->video_info.xres == 1920)
		structConfigVideoStandard->standard = htonl(VO_STANDARD_HDTV_1080I_60);
	else
		BUG();
	structConfigVideoStandard->enProg = 0;
	structConfigVideoStandard->enDIF = 0;
	structConfigVideoStandard->enCompRGB = 0;
	structConfigVideoStandard->pedType = 0;
	structConfigVideoStandard->dataInt0 = htonl(0);
	structConfigVideoStandard->dataInt1 = htonl(0);

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_CONFIG_VIDEO_STANDARD, UNCAC_BASE|(unsigned long)page_address(page), 0, &ret))
		goto fail;
	printk(" #@# config video standard, ret = %lx \n", ret);

	if (ret != S_OK) {
		printk("VenusFB: Config video standard fail...\n");
		goto fail;
	}

	// config hdmi...
	structConfigHDMI = (VIDEO_RPC_VOUT_CONFIG_HDMI *)page_address(page);
	structConfigHDMI->hdmiMode = htonl(VO_HDMI_ON);
	structConfigHDMI->audioSampleFreq = htonl(VO_HDMI_AUDIO_32K);
	structConfigHDMI->colorFmt = htonl(VO_HDMI_RGB444 << 5);
	structConfigHDMI->audioChannelCount = 2;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_CONFIG_HDMI, UNCAC_BASE|(unsigned long)page_address(page), 0, &ret))
		goto fail;
	printk(" #@# config hdmi, ret = %lx \n", ret);

	if (ret != S_OK) {
		printk("VenusFB: Config hdmi fail...\n");
		goto fail;
	}

	// set background...
	structSetBKGrnd = (VIDEO_RPC_VOUT_SET_BKGRND *)page_address(page);
	structSetBKGrnd->bgColor.c1 = 16;
	structSetBKGrnd->bgColor.c2 = 128;
	structSetBKGrnd->bgColor.c3 = 128;
	structSetBKGrnd->bgColor.isRGB = 0;
	structSetBKGrnd->bgEnable = 1;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_SET_BKGRND, UNCAC_BASE|(unsigned long)page_address(page), 0, &ret))
		goto fail;
	printk(" #@# set background, ret = %lx \n", ret);

	if (ret != S_OK) {
		printk("VenusFB: Set background fail...\n");
		goto fail;
	}

	// config graphic canvas...
	structConfigGraphicCanvas = (VIDEO_RPC_VOUT_CONFIG_GRAPHIC_CANVAS *)page_address(page);
	structConfigGraphicCanvas->plane = htonl(VOUT_GRAPHIC_OSD);
	structConfigGraphicCanvas->srcWin.x = 0;
	structConfigGraphicCanvas->srcWin.y = 0;
	structConfigGraphicCanvas->srcWin.width = htons(fbi->video_info.xres);
	structConfigGraphicCanvas->srcWin.height = htons(fbi->video_info.yres);
	structConfigGraphicCanvas->dispWin.x = htons(40);
	structConfigGraphicCanvas->dispWin.y = htons(20);
	structConfigGraphicCanvas->dispWin.width = htons(fbi->video_info.xres-80);
	structConfigGraphicCanvas->dispWin.height = htons(fbi->video_info.yres-40);
	structConfigGraphicCanvas->go = 0;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_CONFIG_GRAPHIC_CANVAS, UNCAC_BASE|(unsigned long)page_address(page), 0, &ret))
		goto fail;
	printk(" #@# config graphic canvas, ret = %lx \n", ret);

	if (ret != S_OK) {
		printk("VenusFB: Config graphic canvas fail...\n");
		goto fail;
	}

	__free_page(page);

	// free mixer2 buffer (if needed)...
	if (fbi->video_info.xres == 720) {
		// we do not need the mixer2 buffer now...
		if (fbi->video_info.mixer2) {
			printk(" #@# free mixer2 buffer %x \n", fbi->video_info.mixer2);
			dvr_free((void *)fbi->video_info.mixer2);
			fbi->video_info.mixer2 = 0;
		}
	}

	return 0;

fail:
	if (page)
		__free_page(page);

	if (fbi->video_info.mixer2) {
		dvr_free((void *)fbi->video_info.mixer2);
		fbi->video_info.mixer2 = 0;
	}

	return 1;
}

static int video_clrDisplayArea(struct venusfb_info *fbi);

static int video_drawDisplayArea(struct venusfb_info *fbi)
{
	VIDEO_RPC_VOUT_CREATE_GRAPHIC_WIN	*structCreateGraphicWin;
	VIDEO_RPC_VOUT_DRAW_GRAPHIC_WIN		*structDrawGraphicWin;
	struct page *page = 0;
	unsigned long winID, ret;

	printk(" #@# Draw Display Area...\n");

	page = alloc_page(GFP_KERNEL);
	printk(" #@# DBuffer: %p...\n", page_address(page));

	// create window...
	structCreateGraphicWin = (VIDEO_RPC_VOUT_CREATE_GRAPHIC_WIN *)page_address(page);
	structCreateGraphicWin->plane = htonl(VOUT_GRAPHIC_OSD);
	structCreateGraphicWin->winPos.x = 0;
	structCreateGraphicWin->winPos.y = 0;
	structCreateGraphicWin->winPos.width = htons(fbi->video_info.xres);
	structCreateGraphicWin->winPos.height = htons(fbi->video_info.yres);
	switch (fbi->video_info.bpp) {
		case 8:
			structCreateGraphicWin->colorFmt = htonl(VO_OSD_COLOR_FORMAT_RGB332);
			structCreateGraphicWin->rgbOrder = htonl((VO_OSD_RGB_ORDER)VO_OSD_COLOR_RGB);
			break;
		default:
		case 16:
			structCreateGraphicWin->colorFmt = htonl(VO_OSD_COLOR_FORMAT_RGB565);
			structCreateGraphicWin->rgbOrder = htonl((VO_OSD_RGB_ORDER)VO_OSD_COLOR_RGB);
			break;
		case 32:
			//+JT - provide ARGB support
			printk(" #@# transp.offset: %d...\n", fbi->video_info.rgb_info.transp.offset);
			if (fbi->video_info.rgb_info.transp.offset == 24) {
				printk(" #@# red.offset: %d...\n", fbi->video_info.rgb_info.red.offset);
				if(fbi->video_info.rgb_info.red.offset == 16) {
					structCreateGraphicWin->colorFmt = htonl(VO_OSD_COLOR_FORMAT_RGBA8888);
					structCreateGraphicWin->rgbOrder = htonl((VO_OSD_RGB_ORDER)VO_OSD_COLOR_BGR);
				} else {        //+JT - EJ 's original setting
					structCreateGraphicWin->colorFmt = htonl(VO_OSD_COLOR_FORMAT_RGBA8888);
					structCreateGraphicWin->rgbOrder = htonl((VO_OSD_RGB_ORDER)VO_OSD_COLOR_RGB);
				}
			} else {
				structCreateGraphicWin->colorFmt = htonl(VO_OSD_COLOR_FORMAT_ARGB8888);
				structCreateGraphicWin->rgbOrder = htonl((VO_OSD_RGB_ORDER)VO_OSD_COLOR_RGB);
			}
			break;
	}
	structCreateGraphicWin->colorKey = htonl(0xFFFFFFFF);
	structCreateGraphicWin->alpha = 0xff;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_CREATE_GRAPHIC_WIN, UNCAC_BASE|(unsigned long)page_address(page), 0, &winID))
		goto fail;
	printk(" #@# create window, ret = %lx \n", winID);

	if (winID == 0xffffffff) {
		printk("VenusFB: Create window fail...\n");
		goto fail;
	}
	fbi->video_info.winID = winID;

	// draw window...
	structDrawGraphicWin = (VIDEO_RPC_VOUT_DRAW_GRAPHIC_WIN *)page_address(page);
	structDrawGraphicWin->plane = htonl(VOUT_GRAPHIC_OSD);
	structDrawGraphicWin->winID = htons((u_short)winID);
	structDrawGraphicWin->storageMode = htonl(VO_GRAPHIC_SEQUENTIAL);
	structDrawGraphicWin->paletteIndex = 0xff;
	structDrawGraphicWin->compressed = 0;
	structDrawGraphicWin->interlace_Frame = 0;
	structDrawGraphicWin->bottomField = 0;
	structDrawGraphicWin->startX[0] = 0;
	structDrawGraphicWin->startY[0] = 0;
	structDrawGraphicWin->imgPitch[0] = htons((u_short)fbi->video_info.pitch);
	structDrawGraphicWin->pImage[0] = htonl((long)fbi->video_info.phyAddr);
	structDrawGraphicWin->go = 1;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_DRAW_GRAPHIC_WIN, UNCAC_BASE|(unsigned long)page_address(page), 0, &ret))
		goto fail;
	printk(" #@# draw window, ret = %lx \n", ret);

	if (ret != S_OK) {
		printk("VenusFB: Draw window fail...\n");
		video_clrDisplayArea(fbi);
		goto fail;
	}

	__free_page(page);

	return 0;

fail:
	if (page)
		__free_page(page);

	return 1;
}

static int video_clrDisplayArea(struct venusfb_info *fbi)
{
	VIDEO_RPC_VOUT_DELETE_GRAPHIC_WIN	*structDeleteGraphicWin;
	struct page *page = 0;
	unsigned long ret;

	printk(" #@# Clear Display Area...\n");

	page = alloc_page(GFP_KERNEL);
	printk(" #@# CBuffer: %p...\n", page_address(page));

	// delete window...
	structDeleteGraphicWin = (VIDEO_RPC_VOUT_DELETE_GRAPHIC_WIN *)page_address(page);
	structDeleteGraphicWin->plane = htonl(VOUT_GRAPHIC_OSD);
	structDeleteGraphicWin->winID = htons((u_short)(fbi->video_info.winID));
	structDeleteGraphicWin->go = 1;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_DELETE_GRAPHIC_WIN, UNCAC_BASE|(unsigned long)page_address(page), 0, &ret))
		goto fail;
	printk(" #@# delete window, ret = %lx \n", ret);

	if (ret != S_OK) {
		printk("VenusFB: Delete window fail...\n");
		goto fail;
	}

	// clear the window ID...
	fbi->video_info.winID = 0;

	__free_page(page);

	return 0;

fail:
	if (page)
		__free_page(page);

	return 1;
}
#endif

/*
 * Internal routines
 */

static void __init setup_venus_video_info(char *options) {
	char *this_opt;
	uint32_t MIN_TILE_HEIGHT;
	short height;

	// retrieve MIN_TILE_HEIGHT
	MIN_TILE_HEIGHT = 8 << ((readl(IOA_DC_SYS_MISC) >> 16) & 0x3);

	// default settings
	venus_video_info.xres = 1280;
	venus_video_info.yres = 720;
	venus_video_info.xoffset = 0;
	venus_video_info.yoffset = 0;
	venus_video_info.bpp = 32;

	while((this_opt = strsep(&options, ",")) != NULL) {
		if(!*this_opt)
			continue;
		if(strncmp(this_opt, "0x101", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 480;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x102", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 480;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x104", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 480;
			venus_video_info.bpp = 32;
		}
		else if(strncmp(this_opt, "0x201", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 576;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x202", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 576;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x204", 5) == 0) {
			venus_video_info.xres = 720;
			venus_video_info.yres = 576;
			venus_video_info.bpp = 32;
		}
		else if(strncmp(this_opt, "0x401", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 720;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x402", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 720;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x404", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 720;
			venus_video_info.bpp = 32;
		}
		else if(strncmp(this_opt, "0x801", 5) == 0) {
			venus_video_info.xres = 1920;
			venus_video_info.yres = 1080;
			venus_video_info.bpp = 8;
		}
		else if(strncmp(this_opt, "0x802", 5) == 0) {
			venus_video_info.xres = 1920;
			venus_video_info.yres = 1080;
			venus_video_info.bpp = 16;
		}
		else if(strncmp(this_opt, "0x804", 5) == 0) {
			venus_video_info.xres = 1280;
			venus_video_info.yres = 960;
			venus_video_info.bpp = 32;
		}
	}

	if (is_mars_cpu() || is_jupiter_cpu()) {
		venus_video_info.storageMode = 1; // sequential mode

		// tune height
		height = venus_video_info.yres;
	
		// adjust pitch to the multiple of 16
		venus_video_info.pitch = ((venus_video_info.xres * (venus_video_info.bpp / 8) + 0x0F) >> 4 ) << 4;
	} else {
		uint32_t ii;

		venus_video_info.storageMode = 0; // block mode

		// tune height
		height = ((venus_video_info.yres - 1) / MIN_TILE_HEIGHT + 1) * MIN_TILE_HEIGHT;
	
		// adjust pitch to the multiple of 256
		venus_video_info.pitch = ((venus_video_info.xres * (venus_video_info.bpp / 8) + 0xFF) >> 8 ) << 8;

		// adjust pitch to power of 2 in 256 bytes unit
		for(ii=256; ; ii*=2) {
			if(venus_video_info.pitch <= ii) {
				venus_video_info.pitch = ii;
				break;
			}
		}
	}

	// calculate pixel clock
	venus_video_info.pixclock = venus_video_info.xres * venus_video_info.yres * 60;

	// calculate required video memory size
	venus_video_info.videomemorysize =  venus_video_info.pitch * height;

	// calculate required videomemorysizeorder
	if(venus_video_info.videomemorysize == (1 << (fls(venus_video_info.videomemorysize) - 1)))
		venus_video_info.videomemorysizeorder = fls(venus_video_info.videomemorysize) - 13;
	else
		venus_video_info.videomemorysizeorder = fls(venus_video_info.videomemorysize) - 12;
}

//
// Frame Buffer Operation Functions 
//

static int
venusfb_open(struct fb_info *info, int user) 
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;
	if (atomic_inc_return(&fbi->ref_count) == 1) {
		if (video_allocMemory(fbi))
			return -ENOMEM;
#ifdef CONFIG_REALTEK_RPC
		video_initDisplayArea(fbi);
		video_drawDisplayArea(fbi);
#endif
	}

	return 0;
}

static int
venusfb_release(struct fb_info *info, int user)
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;
	if (atomic_dec_return(&fbi->ref_count) == 0) {
#ifdef CONFIG_REALTEK_RPC
		video_clrDisplayArea(fbi);
#endif
		if (video_freeMemory(fbi))
			return -EFAULT;
		reset_video_info(fbi);
	}

	return 0;
}

#if 0
static ssize_t
venusfb_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
};

static ssize_t 
venusfb_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
};

#endif

static int 
venusfb_ioctl(struct inode *inode, struct file *filp,
				unsigned int cmd, unsigned long arg, struct fb_info *info)
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;

	int err = 0;
	int retval = 0;

	if (_IOC_TYPE(cmd) != VENUS_FB_IOC_MAGIC) 
		return -ENOTTY;
	else if (_IOC_NR(cmd) > VENUS_FB_IOC_MAXNR) 
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) 
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	switch(cmd) {
	case VENUS_FB_IOC_GET_MACHINE_INFO:
		if(copy_to_user((void *)arg, &(fbi->video_info), sizeof(VENUSFB_MACH_INFO)) != 0)
			return -EFAULT;
		retval = 0;
		break;
	default:
		retval = -ENOIOCTLCMD;
	}

	return retval;
}

/*
 * Pan or Wrap the Display
 *
 * This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
 */

static int venusfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
#ifdef CONFIG_REALTEK_RPC
	VIDEO_RPC_VOUT_MODIFY_GRAPHIC_WIN	*structModifyGraphicWin;
	struct page *page = 0;
	unsigned long ret;
	struct venusfb_info *fbi = (struct venusfb_info *) info;

	if ((var->xoffset == fbi->video_info.xoffset) && (var->yoffset == fbi->video_info.yoffset)) {
		printk(" #@# DONT PAN...\n");
		return 0;
	}
	fbi->video_info.xoffset = var->xoffset;
	fbi->video_info.yoffset = var->yoffset;
	printk(" #@# Pan Display Area...%d %d \n", var->xoffset, var->yoffset);

	page = alloc_page(GFP_KERNEL);
	printk(" #@# PBuffer: %p...\n", page_address(page));

	// modify window...
	structModifyGraphicWin = (VIDEO_RPC_VOUT_MODIFY_GRAPHIC_WIN *)page_address(page);
	structModifyGraphicWin->plane = htonl(VOUT_GRAPHIC_OSD);
	structModifyGraphicWin->winID = htons((u_short)(fbi->video_info.winID));
	structModifyGraphicWin->reqMask = VO_OSD_WIN_SRC_LOCATION;
	structModifyGraphicWin->startX[0] = htons(var->xoffset);
	structModifyGraphicWin->startY[0] = htons(var->yoffset);
	structModifyGraphicWin->imgPitch[0] = htons((u_short)fbi->video_info.pitch);
	structModifyGraphicWin->pImage[0] = htonl((long)fbi->video_info.phyAddr);
	structModifyGraphicWin->go = 1;

#ifdef CONFIG_REALTEK_PREVENT_DC_ALIAS
	flush_dcache_page_alias(page);
#else
	__flush_dcache_page(page);
#endif
	if (send_rpc_command(RPC_VIDEO, RPCCMD_MODIFY_GRAPHIC_WIN, UNCAC_BASE|(unsigned long)page_address(page), 0, &ret))
		goto fail;
	printk(" #@# modify window, ret = %lx \n", ret);

	if (ret != S_OK) {
		printk("VenusFB: Modify window fail...\n");
		goto fail;
	}

	__free_page(page);

	return 0;

fail:
	if (page)
		__free_page(page);

	return -EINVAL;
#else
	return 0;
#endif
}

/*
 * Set a single color register. The values supplied are already
 * rounded down to the hardware's capabilities (according to the
 * entries in the var structure). Return != 0 for invalid regno.
 */

static int venusfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int transp, struct fb_info *info)
{
//	printk(" #@# %d, r: %d, g: %d, b: %d, %x \n", regno, red, green, blue, info->pseudo_palette);
//	if (regno == 15)
//		dump_stack();

	if (regno >= 256)       /* no. of hw registers */
		return 1;
	/*
	 * Program hardware... do anything you want with transp
	 */

	/* grayscale works only partially under directcolor */
	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
	}

	/* Directcolor:
	 *   var->{color}.offset contains start of bitfield
	 *   var->{color}.length contains length of bitfield
	 *   {hardwarespecific} contains width of RAMDAC
	 *   cmap[X] is programmed to (X << red.offset) | (X << green.offset) | (X << blue.offset)
	 *   RAMDAC[X] is programmed to (red, green, blue)
	 *
	 * Pseudocolor:
	 *   uses offset = 0 && length = RAMDAC register width.
	 *   var->{color}.offset is 0
	 *   var->{color}.length contains widht of DAC
	 *   cmap is not used
	 *   RAMDAC[X] is programmed to (red, green, blue)
	 *
	 * Truecolor:
	 *   does not use DAC. Usually 3 are present.
	 *   var->{color}.offset contains start of bitfield
	 *   var->{color}.length contains length of bitfield
	 *   cmap is programmed to (red << red.offset) | (green << green.offset) |
	 *                     (blue << blue.offset) | (transp << transp.offset)
	 *   RAMDAC does not exist
	 */
#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
	switch (info->fix.visual) {
		case FB_VISUAL_TRUECOLOR:
		case FB_VISUAL_PSEUDOCOLOR:
			red = CNVT_TOHW(red, info->var.red.length);
			green = CNVT_TOHW(green, info->var.green.length);
			blue = CNVT_TOHW(blue, info->var.blue.length);
			transp = CNVT_TOHW(transp, info->var.transp.length);
			break;
		case FB_VISUAL_DIRECTCOLOR:
			red = CNVT_TOHW(red, 8);        /* expect 8 bit DAC */
			green = CNVT_TOHW(green, 8);
			blue = CNVT_TOHW(blue, 8);
			/* hey, there is bug in transp handling... */
			transp = CNVT_TOHW(transp, 8);
			break;
	}
#undef CNVT_TOHW
	/* Truecolor has hardware independent palette */
	if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
		u32 v;

		if (regno >= 16)
			return 1;

		v = (red << info->var.red.offset) |
			(green << info->var.green.offset) |
			(blue << info->var.blue.offset) |
			(transp << info->var.transp.offset);
		switch (info->var.bits_per_pixel) {
			case 8:
				break;
			case 16:
				((u32 *) (info->pseudo_palette))[regno] = v;
				break;
			case 24:
			case 32:
				((u32 *) (info->pseudo_palette))[regno] = v;
				break;
		}
		return 0;
	}
	return 0;
}

/*
 * Note that we are entered with the kernel locked.
 */

static int venusfb_mmap(struct fb_info *info, struct file *file, struct vm_area_struct *vma)
{
	unsigned long prot;
	unsigned long off;
	unsigned long start;
	u32 len;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	off = vma->vm_pgoff << PAGE_SHIFT;

	/* frame buffer memory */
	start = info->fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	if (off >= len) {
		/* memory mapped io */
		off -= len;
		if (info->var.accel_flags)
			return -EINVAL;
		start = info->fix.mmio_start;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.mmio_len);
	}
	start &= PAGE_MASK;
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	prot = pgprot_val(vma->vm_page_prot);
	prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
	if (prot & _PAGE_WRITE)
		prot = prot | _PAGE_FILE | _PAGE_VALID | _PAGE_DIRTY;
	else
		prot = prot | _PAGE_FILE | _PAGE_VALID;
	prot &= ~_PAGE_PRESENT;
	vma->vm_page_prot = __pgprot(prot);

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			     vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

// sanity check
static int
venusfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct venusfb_info *fbi = (struct venusfb_info *) info;
	u32 pitch, isResetVideoStandard = 0;

//	if (var->bits_per_pixel != 32) {
//		printk(" #@# Error BPP value %d (only 32-bits)\n", var->bits_per_pixel);
//		return -EINVAL;
//	}

	const struct venusfb_rgb rgb_8 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 0,  .length = 8, },
		.blue	= { .offset = 0,  .length = 8, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_16 = {
		.red	= { .offset = 11, .length = 5, },
		.green	= { .offset = 5,  .length = 6, },
		.blue	= { .offset = 0,  .length = 5, },
		.transp = { .offset = 0,  .length = 0, },
	};

	const struct venusfb_rgb rgb_32 = {
		.red	= { .offset = 0,  .length = 8, },
		.green	= { .offset = 8,  .length = 8, },
		.blue	= { .offset = 16, .length = 8, },
		.transp = { .offset = 24, .length = 8, },
	};

	// resolution check
	if (var->xres == 720) {
		if(var->yres != 480 && var->yres != 576)
			var->yres = 480;
	}
	else if(var->xres == 1280) {
		if(var->yres != 720)
			var->yres = 720;
	}
	else if(var->xres == 1920) {
		if(var->yres != 1080)
			var->yres = 1080;
	}
	else {
		var->xres = 720;
		var->yres = 480;
	}

	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;

	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

	// bpp check
	if (var->bits_per_pixel <= 8)
		var->bits_per_pixel = 8;
	else if (var->bits_per_pixel <= 16)
		var->bits_per_pixel = 16;
	else if (var->bits_per_pixel <= 32)
		var->bits_per_pixel = 32;
	else
		return -EINVAL;

//+JT - keep application's request
#if 1
	fbi->video_info.rgb_info.red = var->red;
	fbi->video_info.rgb_info.green = var->green;
	fbi->video_info.rgb_info.blue = var->blue;
	fbi->video_info.rgb_info.transp = var->transp;
#else
	// RGB offset calibration
	switch(var->bits_per_pixel) {
	case 8:
		var->red		= rgb_8.red;
		var->green		= rgb_8.green;
		var->blue 		= rgb_8.blue;
		var->transp		= rgb_8.transp;
		break;
	default:
	case 16:
		var->red		= rgb_16.red;
		var->green		= rgb_16.green;
		var->blue 		= rgb_16.blue;
		var->transp		= rgb_16.transp;
		break;
	case 32:
		var->red		= rgb_32.red;
		var->green		= rgb_32.green;
		var->blue 		= rgb_32.blue;
		var->transp		= rgb_32.transp;
		break;
	}
#endif

//	var->xoffset			= 0;
//	var->yoffset			= 0;
	var->grayscale			= 0;

	var->pixclock			= 1000*1000*1000 / fbi->video_info.pixclock; // in pico-second unit
	var->left_margin		= 0;
	var->right_margin		= 0;
	var->upper_margin		= 0;
	var->lower_margin		= 0;
	var->hsync_len			= 0;
	var->vsync_len			= 0;
	var->sync			= FB_SYNC_EXT;
	var->nonstd			= 0;
	var->activate			= FB_ACTIVATE_NOW;
	var->height			= 1;
	var->width			= 1;
	var->accel_flags		= 0;
	var->vmode			= FB_VMODE_NONINTERLACED;

	// check if we need to reset video standard
	if ((fbi->video_info.xres != var->xres) || (fbi->video_info.yres != var->yres))
		isResetVideoStandard = 1;

	// sync with fbi->video_info
	fbi->video_info.pixclock	= var->xres * var->yres * 60;
	fbi->video_info.xres 		= var->xres;
	fbi->video_info.yres 		= var->yres;
	fbi->video_info.bpp		= var->bits_per_pixel;

	if (is_mars_cpu() || is_jupiter_cpu())
		fbi->video_info.pitch 	= ((fbi->video_info.xres * (fbi->video_info.bpp / 8) + 0x0F) >> 4 ) << 4;
	else {
		uint32_t ii;
		fbi->video_info.pitch 	= ((fbi->video_info.xres * (fbi->video_info.bpp / 8) + 0xFF) >> 8 ) << 8;

		// adjust pitch to power of 2 in 256 bytes unit
		for(ii=256; ; ii*=2) {
			if(fbi->video_info.pitch <= ii) {
				fbi->video_info.pitch = ii;
				break;
			}
		}
	}

	fbi->fb.fix.line_length = fbi->video_info.pitch;

	// check if we need to re-allocate memory
	if (is_mars_cpu() || is_jupiter_cpu())
		pitch 	= ((var->xres_virtual * (fbi->video_info.bpp / 8) + 0x0F) >> 4 ) << 4;
	else {
		uint32_t ii;
		pitch 	= ((var->xres_virtual * (fbi->video_info.bpp / 8) + 0xFF) >> 8 ) << 8;

		// adjust pitch to power of 2 in 256 bytes unit
		for(ii=256; ; ii*=2) {
			if(pitch <= ii) {
				pitch = ii;
				break;
			}
		}
	}
	printk(" @@@@@ %d %d %d \n", pitch, var->yres_virtual, fbi->video_info.videomemorysize);
	if ((pitch * var->yres_virtual) > fbi->video_info.videomemorysize) {
#ifdef CONFIG_REALTEK_RPC
		video_clrDisplayArea(fbi);
#endif
		video_freeMemory(fbi);
		fbi->video_info.videomemorysize = pitch * var->yres_virtual;
		// calculate required videomemorysizeorder
		if (fbi->video_info.videomemorysize == (1 << (fls(fbi->video_info.videomemorysize) - 1)))
			fbi->video_info.videomemorysizeorder = fls(fbi->video_info.videomemorysize) - 13;
		else
			fbi->video_info.videomemorysizeorder = fls(fbi->video_info.videomemorysize) - 12;
		video_allocMemory(fbi);
#ifdef CONFIG_REALTEK_RPC
		if (isResetVideoStandard)
			video_initDisplayArea(fbi);
		video_drawDisplayArea(fbi);
#endif
	} else if (isResetVideoStandard) {
#ifdef CONFIG_REALTEK_RPC
		video_clrDisplayArea(fbi);
		video_initDisplayArea(fbi);
		video_drawDisplayArea(fbi);
#endif
	} else if (fbi->fb.var.bits_per_pixel != var->bits_per_pixel) {
#ifdef CONFIG_REALTEK_RPC
		video_clrDisplayArea(fbi);
		if (isResetVideoStandard)
			video_initDisplayArea(fbi);
		video_drawDisplayArea(fbi);
#endif
	}

	return 0;
}

static int venusfb_set_par(struct fb_info *info)
{
	return 0;
}

/* ------------ Interfaces to hardware functions ------------ */

static struct fb_ops venusfb_ops = {
	.owner			= THIS_MODULE,
	.fb_open		= venusfb_open,
	.fb_release		= venusfb_release,
	.fb_read		= NULL, // venusfb_read # enable if mmap doesn't work
	.fb_write		= NULL, // venusfb_write # enable if mmap doesn't work
	.fb_check_var		= venusfb_check_var,
	.fb_set_par		= venusfb_set_par,
	.fb_ioctl		= venusfb_ioctl,
	.fb_pan_display		= venusfb_pan_display,
	.fb_setcolreg		= venusfb_setcolreg,
	.fb_fillrect		= cfb_fillrect,
	.fb_copyarea		= cfb_copyarea,
	.fb_imageblit		= cfb_imageblit,
	.fb_cursor		= soft_cursor,
	.fb_mmap		= venusfb_mmap,
};

/*
 *  Initialization
 */
static struct venusfb_info *  __init venusfb_init_fbinfo(struct device *dev)
{
	struct venusfb_info *fbi = NULL;

	// fill venusfb_info
	fbi = (struct venusfb_info *)framebuffer_alloc(sizeof(struct venusfb_info) - sizeof(struct fb_info), dev);

	if (!fbi) {
		printk("VenusFB: framebuffer_alloc(%d) failed ..\n", sizeof(struct venusfb_info) - sizeof(struct fb_info));
		goto err;
	}

	memset(fbi, 0, sizeof(struct venusfb_info));
	fbi->dev = dev;
	reset_video_info(fbi);

err:
	return fbi;
}

static int __init venusfb_probe(struct device *dev)
{
	struct venusfb_info *fbi = NULL;
	int ret;

	/* initialize fbi */
	fbi = venusfb_init_fbinfo(dev);
	if (!fbi){
		printk("VenusFB: error: can't allocate memory for fbi\n");
		return -ENOMEM;
	}

//	venusfb_check_var(&fbi->fb.var, &fbi->fb);

	// framebuffer device registeration
	ret = register_framebuffer(&fbi->fb);
	if (ret < 0)
		goto failed;

	printk("VenusFB: load successful (%dx%dx%dbbp)\n", fbi->video_info.xres, fbi->video_info.yres, fbi->video_info.bpp);

	return 0;

failed:

	framebuffer_release((struct fb_info *)fbi);
	return ret;
}

static int venusfb_remove(struct device *device)
{
	struct fb_info *info = dev_get_drvdata(device);

	printk("VenusFB: removing..\n");
	if (info) {
		unregister_framebuffer(info);
		framebuffer_release(info);
	}
	return 0;
}

static struct device_driver venusfb_driver = {
	.name	= "venusfb",
	.bus	= &platform_bus_type,
	.probe	= venusfb_probe,
	.remove = venusfb_remove,
};

static struct platform_device venusfb_device = {
	.name	= "venusfb",
	.id	= 0,
};

/*
 * Initialization
 */
static int __init venusfb_init(void)
{
	int ret;
	char *options = NULL;

	printk("VenusFB: Framebuffer device driver for Realtek Media Processors\n");

	if (fb_get_options("venusfb", &options))
		return -ENODEV;

	setup_venus_video_info(options);

	ret = driver_register(&venusfb_driver);

	if (!ret) {
		ret = platform_device_register(&venusfb_device);
		if (ret)
			driver_unregister(&venusfb_driver);
	}

	return ret;
}

/*
 * Cleanup
 */
static void __exit venusfb_cleanup(void)
{
	platform_device_unregister(&venusfb_device);
	driver_unregister(&venusfb_driver);
}

module_init(venusfb_init);
module_exit(venusfb_cleanup);

MODULE_AUTHOR("Chih-pin Wu, Realtek Semiconductor");
MODULE_DESCRIPTION("Framebuffer driver for Venus/Neptune/Mars");
MODULE_LICENSE("GPL v2");
