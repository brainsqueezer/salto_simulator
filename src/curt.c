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
 * Cursor task functions
 *
 * $Id: curt.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>

#include "alto.h"
#include "cpu.h"
#include "display.h"
#include "curt.h"

/**
 * @brief update the cursor word and bitmask
 *
 * Shift CSR to xpreg % 16 position to make it easier to
 * to handle the word xor in unload_word().
 * <PRE>
 * xpreg % 16   cursor bits
 *              [ first word   ][  second word ]
 * ----------------------------------------------
 *     0        xxxxxxxxxxxxxxxx0000000000000000
 *     1        0xxxxxxxxxxxxxxxx000000000000000
 *     2        00xxxxxxxxxxxxxxxx00000000000000
 * ...
 *    14        00000000000000xxxxxxxxxxxxxxxx00
 *    15        000000000000000xxxxxxxxxxxxxxxx0
 * </PRE>
 */
#define	UPDATE_CURSOR do { \
	int x = 1023 - dsp.xpreg; \
	dsp.curdata = dsp.csr << (16 - (x & 15)); \
	dsp.curword = x / 16; \
} while (0)


/* @brief curt block; disable the cursor task and set the curt_blocks flag */
static void f1_block_0(void)
{
	CPU_CLR_TASK_WAKEUP(cpu.task);
	LOG((0,2,"	BLOCK %s\n", task_name[cpu.task]));
	dsp.curt_blocks = 1;
}


/** @brief load the x position register from BUS[6-15] */
static void load_xpreg_1(void)
{
	dsp.xpreg = ALTO_GET(cpu.bus,16,6,15);
	LOG((0,2,"	XPREG<- BUS[6-15] (%#o)\n", dsp.xpreg));
}

/** @brief load the cursor shift register from BUS[0-15] */
static void load_csr_1(void)
{
	dsp.csr = cpu.bus;
	LOG((0,2,"	CSR<- BUS (%#o)\n", dsp.csr));
	UPDATE_CURSOR;
}

/** @brief called by the CPU when the cursor task becomes active */
static void activate(void)
{
	CPU_CLR_TASK_WAKEUP(cpu.task);
	dsp.curt_wakeup = 0;
}

/** @brief initialize the cursor task F1 and F2 functions */
int init_curt(int task)
{
	SET_FN(f1, block,			f1_block_0,	NULL);
	SET_FN(f2, curt_load_xpreg,		NULL,		load_xpreg_1);
	SET_FN(f2, curt_load_csr,		NULL,		load_csr_1);

	CPU_SET_ACTIVATE_CB(task, activate);
	return 0;
}
