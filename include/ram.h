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
 * Bus source and F1 handling for RAM related tasks.
 *
 * $Id: ram.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_RAM_H_INCLUDED_)
#define	_RAM_H_INCLUDED_

#include "alto.h"
#include "cpu.h"

/** @brief bs task specific: ram related */
#define	bs_ram_read_slocation	bs_task_3
#define	bs_ram_load_slocation	bs_task_4

/** @brief f1 task specific: ram related */
#define	f1_ram_swmode		f1_task_10
#define	f1_ram_wrtram		f1_task_11
#define	f1_ram_rdram		f1_task_12
#define	f1_ram_load_srb		f1_task_13

/**
 * @brief bs_read_sreg early: drive bus by S register or M, if rsel is = 0
 *
 * Note: M is just another name for s[s_reg_bank[task]][0]
 */
void bs_read_sreg_0(void);

/**
 * @brief bs_load_sreg early: load S register puts garbage on the bus
 */
void bs_load_sreg_0(void);

/**
 * @brief bs_read_sreg late: load S register from M
 *
 * Note: M is just another name for s[s_reg_bank[task]][0]
 */
void bs_load_sreg_1(void);

/**
 * @brief f1_swmode late: switch to micro program count CRAM (?)
 */
void f1_swmode_1(void);

/**
 * @brief f1_wrtram late: start WRTRAM cycle
 */
void f1_wrtram_1(void);

/**
 * @brief f1_rdram late: start RDRAM cycle
 */
void f1_rdram_1(void);

/**
 * @brief f1_load_srb late: load the S register bank from BUS[12-14]
 */
void f1_load_srb_1(void);

/** @brief initialize ram related task */
extern int init_ram(int task);

#endif	/* !defined(_RAM_H_INCLUDED_) */

