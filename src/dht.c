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
 * Display horizontal task functions
 *
 * $Id: dht.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "display.h"
#include "dht.h"

/* @brief dht block; disable the display word task */
static void f1_block_0(void)
{
	dsp.dht_blocks = 1;
	/* clear the wakeup for the display horizontal task */
	CPU_CLR_TASK_WAKEUP(cpu.task);
	LOG((0,2,"	BLOCK %s\n", task_name[cpu.task]));
}

/**
 * @brief set the next scanline's mode inverse and half clock and branch
 *
 * BUS[0] selects the pixel clock (0), or half pixel clock (1)
 * BUS[1] selects normal mode (0), or inverse mode (1)
 *
 * The current BUS[0] drives the NEXT[09] line, i.e. branches to 0 or 1
 */
static void f2_setmode_1(void)
{
	int or = GET_SETMODE_SPEEDY(cpu.bus);
	dsp.setmode = cpu.bus;
	LOG((0,2,"	SETMODE<- BUS (%#o), branch on BUS[0] (%#o | %#o)\n",
		cpu.bus, cpu.next2, or));
	CPU_BRANCH(or);
}

/** @brief called by the CPU when the display horizontal task becomes active */
static void activate(void)
{
	/* TODO: what do we do here? */
	CPU_CLR_TASK_WAKEUP(cpu.task);
}

/**
 * @brief initialize the display horizontal task
 *
 * @param task task number
 * @result returns 0 on success
 */
int init_dht(int task)
{
	SET_FN(f1, block,			f1_block_0,	NULL);
	SET_FN(f2, dht_evenfield,		NULL,		f2_evenfield_1);
	SET_FN(f2, dht_setmode,			NULL,		f2_setmode_1);

	CPU_SET_ACTIVATE_CB(task, activate);

	return 0;
}
