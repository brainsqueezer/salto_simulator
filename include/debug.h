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
 * Debugger
 *
 * $Id: debug.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_DEBUGGER_H_INCLUDED_)
#define	_DEBUGGER_H_INCLUDED_

/* need to know DISPLAY_WIDTH and DISPLAY_HEIGHT */
#include "display.h"

#if	DEBUG
/** @brief log levels for CPU tasks and non-tasks (outside CPU) */
typedef enum {
	/** @brief log_CPU level uses cpu.task as index to loglevels */
	log_CPU,
	/** @brief use task switcher loglevel to gate messages */
	log_TSW = task_COUNT,
	/** @brief use memory loglevel to gate messages */
	log_MEM,
	/** @brief use timer loglevel to gate messages */
	log_TMR,
	/** @brief use display loglevel to gate messages */
	log_DSP,
	/** @brief use disk loglevel to gate messages */
	log_DSK,
	/** @brief use drive loglevel to gate messages */
	log_DRV,
	/** @brief use loglevel for miscellaneous functions to gate messages */
	log_MISC,
	/** @brief number of additional loglevels */
	log_COUNT
}	log_t;

/** @brief structure of the loglevel table */
typedef struct {
	int level;
	const char *name;
	const char *description;
}	loglevel_t;

extern loglevel_t ll[log_COUNT+1];

#endif	/* DEBUG */

/** @brief the available number bases for debugger outputs */
typedef enum {
	base_OCT,
	base_HEX,
	base_DEC,
	base_COUNT
}	base_t;

/** @brief number of words in the debugger dump */
#define	DBG_DUMP_WORDS	0200

/** @brief debugger font width */
#define	DBG_FONT_W	6

/** @brief debugger font height */
#define	DBG_FONT_H	10

/** @brief number of colors */
#define	DBG_COLORS	8

/** @brief debugger text map width in character cells */
#define	DBG_TEXTMAP_W	(DISPLAY_WIDTH/DBG_FONT_W)

/** @brief debugger text map height in character cells */
#define	DBG_TEXTMAP_H	(DISPLAY_HEIGHT/DBG_FONT_H)

/** @brief debugger scrolling window width */
#define	DBG_WIN_W	DBG_TEXTMAP_W

/** @brief debugger scrolling window height */
#define	DBG_WIN_H	16

/** @brief debugger scrolling window left */
#define	DBG_WIN_X	0

/** @brief debugger scrolling window top */
#define	DBG_WIN_Y	(DBG_TEXTMAP_H-DBG_WIN_H)

/** @brief arbitrary value for the max. width of bits info output */
#define	DBG_BITS_W	DBG_TEXTMAP_W

/** @brief arbitrary value for the max. height of bits info output */
#define	DBG_BITS_H	12

/** @brief number of entries in the follow/return address stack */
#define	DBG_STACK_SIZE	16

/** @brief structure of the debugger context */
typedef struct {
	/** @brief visible flag; if non-zero, the debugger SDL surface is visible */
	int visible;

	/** @brief lock flag; if non-zero, memory access comes from the debugger */
	int lock;

	/** @brief current debugger window text */
	int textmap[DBG_TEXTMAP_W * DBG_TEXTMAP_H];

	/** @brief recent debugger window text */
	int backmap[DBG_TEXTMAP_W * DBG_TEXTMAP_H];

	/** @brief number base for outputs */
	base_t base;

	/** @brief non zero: display memory dump, zero: display drive bits */
	int memory;

	/** @brief non zero: display ASCII dump with memory, zero: display memory check */
	int ascii;

	/** @brief memory address for the dump */
	int memaddr;

	/** @brief cursor offset for the dump */
	int memoffs;

	/** @brief drive unit for the sector dump */
	int drive;

	/** @brief drive page for the sector dump */
	int page;

	/** @brief sector bit address sector dump */
	int secaddr;

	/** @brief sector bit offset sector dump */
	int secoffs;

	/** @brief return address stack for follow (SPACE), return (BACKSPACE) */
	int retaddr[DBG_STACK_SIZE];

	/** @brief return address stack pointer */
	int stackptr;

	/** @brief text color */
	int color;

	/** @brief scrolling range cursor x */
	int cx;

	/** @brief scrolling range cursor y */
	int cy;

	/** @brief memory write watch addresses */
	int ww_addr[DBG_STACK_SIZE];

	/** @brief memory write watch count */
	int ww_count;

	/** @brief memory read watch addresses */
	int wr_addr[DBG_STACK_SIZE];

	/** @brief memory read watch count */
	int wr_count;

}	debug_t;

/**
 * @brief debugger context
 */
extern debug_t dbg;

/**
 * @brief disassemble one emulator instruction
 *
 * @param buff pointer to a string buffer to receive the disassembly
 * @param size max. size of the buffer
 * @param indent indentation wanted (non-zero after skip in previous inst.)
 * @param pc current program counter (e.g. R06)
 * @param ir instruction register, i.e. opcode
 * @result returns 1 if instruction may skip, 0 otherwise
 */
extern int dbg_dasm(char *buff, size_t size, int indent, int pc, unsigned int ir);

/**
 * @brief dump R and S[0] registers, tasks, 2 pages of memory ...
 */
extern void dbg_dump_regs(void);

/**
 * @brief debug key event, called by sdl_update
 *
 * @param sym pointer to a key symbol (SDL_keysym)
 * @param down non-zero if key is pressed, zero if released
 */
extern void dbg_key(void *sym, int down);

/**
 * @brief print a formatted string to the scrollable range
 */
extern int dbg_printf(const char *fmt, ...);

/**
 * @brief pass command line switches down to the debugger
 *
 * @param arg a command line switch, like "-all" or "+tsw"
 * @result returns 0 if arg accepted, -1 otherwise
 */
extern int dbg_args(const char *arg);

/**
 * @brief print usage info for the debugger switches
 *
 * @param argc argument count
 * @param argv argument list
 * @result returns 0 (why should it fail?)
 */
extern int dbg_usage(int argc, char **argv);

/**
 * @brief initialize the debugger
 */
extern int debug_init(void);

#endif	/* !defined(_DEBUGGER_H_INCLUDED_) */

