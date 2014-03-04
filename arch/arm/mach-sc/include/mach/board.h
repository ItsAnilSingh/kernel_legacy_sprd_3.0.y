/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_BOARD_H
#define __ASM_ARCH_BOARD_H

#ifdef	CONFIG_MACH_SP8825EA
#include "__board-sp8825ea.h"
#endif

#ifdef	CONFIG_MACH_SPX35EA
#include "__board-sp8830ea.h"
#endif

#ifdef	CONFIG_MACH_SPX35EB
#include "__board-sp8830eb.h"
#endif

#ifdef	CONFIG_MACH_SP8835EB
#include "__board-sp8835eb.h"
#endif

#ifdef	CONFIG_MACH_SPX35EC
#include "__board-sp8830ec.h"
#endif

#ifdef	CONFIG_MACH_SP8830SSW
#include "__board-sp8830ssw.h"
#endif

#ifdef	CONFIG_MACH_SP7730EC
#include "__board-sp7730ec.h"
#endif

#ifdef	CONFIG_MACH_SP7730ECTRISIM
#include "__board-sp7730ectrisim.h"
#endif

#ifdef	CONFIG_MACH_SP5735
#include "__board-sp5735.h"
#endif

#ifdef	CONFIG_MACH_SP5735PAD
#include "__board-sp5735pad.h"
#endif

#if defined (CONFIG_MACH_SPX35FPGA) || defined (CONFIG_MACH_SPX15FPGA)
#include "__board-sp8830fpga.h"
#endif

#ifdef	CONFIG_MACH_SP7735EC
#include "__board-sp7735ec.h"
#endif

#include <asm/sizes.h>


#ifdef CONFIG_ION
    #if defined (CONFIG_ARCH_SC8825)
        #if defined(CONFIG_CAMERA_8M)
            #define SPRD_ION_SIZE	(23*1024*1024)
        #elif defined(CONFIG_CAMERA_5M)
            #define SPRD_ION_SIZE	(19*1024*1024)
        #elif defined(CONFIG_CAMERA_3M)
            #define SPRD_ION_SIZE	(13*1024*1024)
        #elif defined(CONFIG_CAMERA_2M)
            #ifdef CONFIG_CAMERA_ROTATION
                #define SPRD_ION_SIZE	(13*1024*1024)
            #else
                #define SPRD_ION_SIZE	(8*1024*1024)
            #endif
        #else
	    #define SPRD_ION_SIZE	(CONFIG_SPRD_ION_SIZE * SZ_1M)
        #endif
    #elif defined (CONFIG_ARCH_SCX35)
        #if defined(CONFIG_CMA)
            #define SPRD_ION_SIZE	(20*1024*1024)
        #else
            #ifndef SPRD_ION_SIZE	/* should be defined in "__board-sp**.h" already */
            #define SPRD_ION_SIZE	(76 * SZ_1M)
            #endif
        #endif
    #else
        #define SPRD_ION_SIZE	(CONFIG_SPRD_ION_SIZE*1024*1024)
    #endif


    #if defined (CONFIG_ARCH_SCX35)
        #if defined(CONFIG_CMA)
            #define SPRD_ION_OVERLAY_SIZE   (0 * SZ_1M)
        #else
            #ifndef SPRD_ION_OVERLAY_SIZE	/* should be defined in "__board-sp**.h" already */
            #define SPRD_ION_OVERLAY_SIZE	(12 * SZ_1M)
            #endif
        #endif
    #else
        #define SPRD_ION_OVERLAY_SIZE   (CONFIG_SPRD_ION_OVERLAY_SIZE * SZ_1M)
    #endif

#else /* !ION */
#define SPRD_ION_SIZE           (0 * SZ_1M)
#define SPRD_ION_OVERLAY_SIZE   (0 * SZ_1M)
#endif

#define SPRD_IO_MEM_SIZE	(SPRD_ION_SIZE + SPRD_ION_OVERLAY_SIZE)
#define SPRD_IO_MEM_BASE	\
	((CONFIG_PHYS_OFFSET & (~(SZ_512M - 1))) + SZ_512M - SPRD_IO_MEM_SIZE)

#define SPRD_ION_BASE		(SPRD_IO_MEM_BASE)
#define SPRD_ION_OVERLAY_BASE   (SPRD_ION_BASE + SPRD_ION_SIZE)

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define SPRD_RAM_CONSOLE_SIZE	0x20000
#define SPRD_RAM_CONSOLE_START	(SPRD_IO_MEM_BASE - SPRD_RAM_CONSOLE_SIZE)
#endif

#ifdef CONFIG_FB_LCD_RESERVE_MEM

#if defined(CONFIG_SPRD_FB_SIZE)
#define SPRD_FB_MEM_SIZE	(CONFIG_SPRD_FB_SIZE * SZ_1M)
#else
#define SPRD_FB_MEM_SIZE	SZ_8M
#endif

#define SPRD_FB_MEM_BASE	(SPRD_RAM_CONSOLE_START - SPRD_FB_MEM_SIZE)
#endif

#ifdef CONFIG_SPRD_IQ
#define SPRD_IQ_SIZE SZ_128M
#endif

#define SPRD_SYSDUMP_MAGIC	(SPRD_IO_MEM_BASE + SPRD_IO_MEM_SIZE - SZ_1M)

struct sysdump_mem {
	unsigned long paddr;
	unsigned long vaddr;
	unsigned long soff;
	size_t size;
	int type;
};

enum sysdump_type {
	SYSDUMP_RAM,
	SYSDUMP_MODEM,
	SYSDUMP_IOMEM,
};

extern int sprd_dump_mem_num;
extern struct sysdump_mem sprd_dump_mem[];

#endif
