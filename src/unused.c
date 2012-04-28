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
 * Handling for unused tasks
 *
 * $Id: unused.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>

#include "ram.h"

/** @brief fatal exit on unitialized dynamic phase BUS source */
static void bs_unused_0(void)
{
	fatal(9,"fatal: bad early bus source pointer for task %s, mpc:%05o bs:%s\n",
		task_name[cpu.task], cpu.mpc, bs_name[MIR_BS]);
}

/** @brief fatal exit on unitialized latching phase BUS source */
static void bs_unused_1(void)
{
	fatal(9,"fatal: bad late bus source pointer for task %s, mpc:%05o bs: %s\n",
		task_name[cpu.task], cpu.mpc, bs_name[MIR_BS]);
}

/** @brief fatal exit on unitialized dynamic phase F1 function */
static void f1_unused_0(void)
{
	fatal(9,"fatal: bad early f1 function pointer for task %s, mpc:%05o f1: %s\n",
		task_name[cpu.task], cpu.mpc, f1_name[MIR_F1]);
}

/** @brief fatal exit on unitialized latching phase F1 function */
static void f1_unused_1(void)
{
	fatal(9,"fatal: bad late f1 function pointer for task %s, mpc:%05o f1: %s\n",
		task_name[cpu.task], cpu.mpc, f1_name[MIR_F1]);
}

/** @brief fatal exit on unitialized dynamic phase F2 function */
static void f2_unused_0(void)
{
	fatal(9,"fatal: bad early f2 function pointer for task %s, mpc:%05o f2: %s\n",
		task_name[cpu.task], cpu.mpc, f2_name[MIR_F2]);
}

/** @brief fatal exit on unitialized latching phase F2 function */
static void f2_unused_1(void)
{
	fatal(9,"fatal: bad late f2 function pointer for task %s, mpc:%05o f2: %s\n",
		task_name[cpu.task], cpu.mpc, f2_name[MIR_F2]);
}


/**
 * @brief unused task slots initialization
 */
int init_unused(int task)
{
	SET_FN(bs, task_3,	bs_unused_0,	bs_unused_1);
	SET_FN(bs, task_4,	bs_unused_0,	bs_unused_1);

	SET_FN(f1, task_10,	f1_unused_0,	f1_unused_1);
	SET_FN(f1, task_11,	f1_unused_0,	f1_unused_1);
	SET_FN(f1, task_12,	f1_unused_0,	f1_unused_1);
	SET_FN(f1, task_13,	f1_unused_0,	f1_unused_1);
	SET_FN(f1, task_14,	f1_unused_0,	f1_unused_1);
	SET_FN(f1, task_15,	f1_unused_0,	f1_unused_1);
	SET_FN(f1, task_16,	f1_unused_0,	f1_unused_1);
	SET_FN(f1, task_17,	f1_unused_0,	f1_unused_1);

	SET_FN(f2, task_10,	f2_unused_0,	f2_unused_1);
	SET_FN(f2, task_11,	f2_unused_0,	f2_unused_1);
	SET_FN(f2, task_12,	f2_unused_0,	f2_unused_1);
	SET_FN(f2, task_13,	f2_unused_0,	f2_unused_1);
	SET_FN(f2, task_14,	f2_unused_0,	f2_unused_1);
	SET_FN(f2, task_15,	f2_unused_0,	f2_unused_1);
	SET_FN(f2, task_16,	f2_unused_0,	f2_unused_1);
	SET_FN(f2, task_17,	f2_unused_0,	f2_unused_1);

	return 0;
}
