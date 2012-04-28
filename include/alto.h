/*****************************************************************************
 * SALTO - Xerox Alto I/II Simulator.
 *
 * Copyright (C) 2007 by Juergen Buchmueller <pullmoll@t-online.de>
 * Partially based on info found in Eric Smith's Alto simulator: Altogether
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Alto contants and macros
 *
 * $Id: alto.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_ALTO_H_INCLUDED_)
#define	_ALTO_H_INCLUDED_

/** @brief get the integer type definitions */
#include "altoint.h"

/** @brief type to hold nano seconds */
typedef int64_t	ntime_t;

/** @brief time in nano seconds for a CPU microcycle */
#define	CPU_MICROCYCLE_TIME	170

#ifndef	CRAM_CONFIG
#define	CRAM_CONFIG	2
#endif

#if	(CRAM_CONFIG==1)

/** @brief number of microcode ROM pages */
#define	UCODE_ROM_PAGES	1
/** @brief number of microcode RAM pages */
#define	UCODE_RAM_PAGES	1

#elif	(CRAM_CONFIG==2)

/** @brief number of microcode ROM pages */
#define	UCODE_ROM_PAGES	2
/** @brief number of microcode RAM pages */
#define	UCODE_RAM_PAGES	1

#elif	(CRAM_CONFIG==3)

/** @brief number of microcode ROM pages */
#define	UCODE_ROM_PAGES	1
/** @brief number of microcode RAM pages */
#define	UCODE_RAM_PAGES	3

#else

#error	"Undefined CROM/CRAM configuration"

#endif

/** @brief number of S register banks
 * depends on the number of RAM pages
 * 8 pages in 3K CRAM configuration
 * 1 page in 1K CRAM configurations
 */
#if	(UCODE_RAM_PAGES==3)
#define	SREG_BANKS	8
#else
#define	SREG_BANKS	1
#endif

/** @brief number of words of microcode */
#define	UCODE_PAGE_SIZE	1024

/** @brief mask for microcode ROM/RAM address */
#define	UCODE_PAGE_MASK	(UCODE_PAGE_SIZE-1)

/** @brief total number of words of microcode */
#define	UCODE_SIZE	((UCODE_ROM_PAGES+UCODE_RAM_PAGES)*UCODE_PAGE_SIZE)

/** @brief base offset for the RAM page(s) */
#define	UCODE_RAM_BASE	(UCODE_ROM_PAGES*UCODE_PAGE_SIZE)

/** @brief number words in the constant ROM */
#define	CONST_SIZE	256

/** @brief size of main memory */
#define	RAM_SIZE	262144

#if	DEBUG

/** @brief non zero if simualtion shall shut down */
extern int halted;

/** @brief non zero if simualtion shall pause */
extern int paused;

/** @brief non zero next step in pause mode */
extern int step;

/**
 * @brief switch views between Alto surface (0) or debug surface (1)
 */
extern void debug_view(int which);

/**
 * @brief log print a format string
 *
 * @param task 0: use cpu.task as index for loglevel; >0: use other logelevels
 * @param level level of importance (higher values are less important)
 * @result returns size of string written
 */
extern int logprintf(int task, int level, const char *fmt, ...);

/**
 * @brief per task and some extra loglevels
 *
 * This is actually in debug.c
 */
extern int loglevel[];
#define	LOG(x)	logprintf x

#else	/* DEBUG */

#define	LOG(x)	/* x */

#endif	/* !DEBUG */

/** @brief LED icon identifiers */
typedef enum {
	led_blank,
	led_0,
	led_1,
	led_2,
	led_3,
	led_4,
	led_5,
	led_6,
	led_7,
	led_8,
	led_9,
	led_green_off,
	led_green_on,
	led_red_off,
	led_red_on,
	led_yellow_off,
	led_yellow_on,
	mouse_off,
	mouse_on,
	mng_off,
	mng_on0,
	mng_on1,
	mng_on2,
	mng_on3,
	led_count
}	led_t;


/** @brief structure of the (internal) character generator data */
typedef struct {
	/** @brief width of characters in this generator */
	int width;
	/** @brief height of characters in this generator */
	int height;
	/** @brief character code and bitmap data */
	const uint8_t *data;
	/** @brief number of bytes of dtaa */
	size_t size;
}	chargen_t;

/** @brief make a RGB color word from red, green, and blue */
#define	sdl_rgb(r,g,b) (((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|((uint32_t)(b)))
/** @brief get red from a RGB color word */
#define	sdl_get_r(rgb)	((uint8_t)((rgb) >> 16))
/** @brief get green from a RGB color word */
#define	sdl_get_g(rgb)	((uint8_t)((rgb) >>  8))
/** @brief get blue from a RGB color word */
#define	sdl_get_b(rgb)	((uint8_t)((rgb) >>  0))

/** @brief write a character to the surface border */
extern int border_putch(int x, int y, uint8_t ch);

/** @brief print a string to the surface border */
extern int border_printf(int x, int y, const char *fmt, ...);

/** @brief write 16 pixels to x, y into the alto surface */
extern int sdl_write(int x, int y, uint32_t pixel);

/** @brief draw a LED icon to the SDL surface border */
extern int sdl_draw_icon(int x0, int y0, int type);

/** @brief render character generator bitmaps for one color */
extern int sdl_chargen_alpha(const chargen_t *cg, int col, uint32_t rgb);

/** @brief write charmap bitmap for char ch, color to x, y into the debug surface */
extern int sdl_debug(int x, int y, int ch, int color);

/** @brief update pending changes to the screen */
extern int sdl_update(int full);

extern void fatal(int exitcode, const char *fmt, ...);

/**
 * @brief initialize the machine, load ROMs and PROMs
 *
 * @param rompath base directory name where to find ROMs
 */
extern int alto_init(const char *rompath);

#endif	/* !defined(_ALTO_H_INCLUDED_) */
