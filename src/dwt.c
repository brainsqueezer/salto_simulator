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
 * Display word task functions
 *
 * $Id: dwt.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "debug.h"
#include "display.h"
#include "dwt.h"

/** @brief block the display word task */
static void f1_block_0(void)
{
	dsp.dwt_blocks = 1;

	/* clear the wakeup for the display word task */
	CPU_CLR_TASK_WAKEUP(cpu.task);
	LOG((0,2,"	BLOCK %s\n", task_name[cpu.task]));

	/* wakeup the display horizontal task, if it didn't block itself */
	if (!dsp.dht_blocks)
		CPU_SET_TASK_WAKEUP(task_dht);
}

/** @brief load the display data register */
void load_ddr_1(void)
{
	LOG((0,2,"	DDR<- BUS (%#o)\n", cpu.bus));
	dsp.fifo[dsp.fifo_wr] = cpu.bus;
	if (++dsp.fifo_wr == DISPLAY_FIFO_SIZE)
		dsp.fifo_wr = 0;
	if (FIFO_STOPWAKE_0 == 0)
		CPU_CLR_TASK_WAKEUP(task_dwt);
	LOG((log_DSP,2, "	DWT push %04x into FIFO[%02o]%s\n",
		cpu.bus, (dsp.fifo_wr - 1) & (DISPLAY_FIFO_SIZE - 1),
		FIFO_STOPWAKE_0 == 0 ? " STOPWAKE" : ""));
}

int init_dwt(int task)
{
	SET_FN(f1, block,		f1_block_0,	NULL);
	SET_FN(f2, dwt_load_ddr,	NULL,		load_ddr_1);

	return 0;
}
