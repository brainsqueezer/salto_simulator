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
 * RAM related task functions
 *
 * $Id: ram.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>

#include "alto.h"
#include "cpu.h"
#include "ram.h"

/**
 * @brief early: drive bus by S register or M (MYL), if rsel is = 0
 *
 * Note: RSEL == 0 can't be read, because it is decoded as
 * access to the M register (MYL latch access, LREF' in the schematics)
 */
void bs_read_sreg_0(void)
{
	int bank = cpu.s_reg_bank[cpu.task];
	int and;

	if (MIR_RSEL) {
		and = cpu.s[bank][MIR_RSEL];
		LOG((0,2,"	<-S%02o; bus &= S[%o][%02o] (%#o)\n",
			MIR_RSEL, bank, MIR_RSEL, and));
	} else {
		and = cpu.m;
		LOG((0,2,"	<-S%02o; bus &= M (%#o)\n",
			MIR_RSEL, and));
	}
	cpu.bus &= and;
}

/**
 * @brief early: load S register puts garbage on the bus
 */
void bs_load_sreg_0(void)
{
	int and = 0;	/* ??? */
	LOG((0,2,"	S%02o<- BUS &= garbage (%#o)\n",
		MIR_RSEL, and));
	cpu.bus &= and;
}

/**
 * @brief bs_load_sreg late: load S register from M
 */
void bs_load_sreg_1(void)
{
	int bank = cpu.s_reg_bank[cpu.task];
	cpu.s[bank][MIR_RSEL] = cpu.m;
	LOG((0,2,"	S%02o<- S[%o][%02o] := %#o\n",
		MIR_RSEL, bank, MIR_RSEL, cpu.m));
}

/**
 * @brief branch to ROM page
 */
static void branch_ROM(const char *from, int page)
{
	cpu.next2 = (cpu.next2 & UCODE_PAGE_MASK) + page * UCODE_PAGE_SIZE;
	LOG((0,2,"	SWMODE: branch from %s to ROM%d (%#o)\n",
		from, page, cpu.next2));
	(void)from;
}

/**
 * @brief branch to RAM page
 */
static void branch_RAM(const char *from, int page)
{
	cpu.next2 = (cpu.next2 & UCODE_PAGE_MASK) + UCODE_RAM_BASE + page * UCODE_PAGE_SIZE;
	LOG((0,2,"	SWMODE: branch from %s to RAM%d\n",
		from, page, cpu.next2));
	(void)from;
}

/**
 * @brief f1_swmode early: switch to micro program counter BUS[6-15] in other bank
 *
 * Note: Jumping to uninitialized CRAM
 *
 * When jumping to uninitialized RAM, which, because of the inverted bits of the
 * microcode words F1(0), F2(0) and LOADL, it is then read as F1=010 (SWMODE),
 * F2=010 (BUSODD) and LOADL=1, loading the M register (MYL latch), too.
 * This causes control to go back to the Emulator task at 0, because the
 * NEXT[0-9] of uninitialized RAM is 0.
 *
 */
void f1_swmode_1(void)
{
	/* currently executing in what page? */
	int current = cpu.mpc / UCODE_PAGE_SIZE;

#if	(UCODE_ROM_PAGES == 1 && UCODE_RAM_PAGES == 1)
	switch (current) {
	case 0:
		branch_RAM("ROM0", 0);
		break;
	case 1:
		branch_ROM("RAM0", 0);
		break;
	default:
		fatal(1, "Impossible current mpc %d\n", current);
	}
#elif	(UCODE_ROM_PAGES == 2 && UCODE_RAM_PAGES == 1)
	int next = ALTO_GET(cpu.next2,10,1,1);

	switch (current) {
	case 0: /* ROM0 to RAM0 or ROM1 */
		switch (next) {
		case 0:
			branch_RAM("ROM0", 0);
			break;
		case 1:
			branch_ROM("ROM0", 1);
			break;
		default:
			fatal(1, "Impossible next %d\n", next);
		}
		break;
	case 1: /* ROM1 to ROM0 or RAM0 */
		switch (next) {
		case 0:
			branch_ROM("ROM1", 0);
			break;
		case 1:
			branch_RAM("ROM1", 0);
			break;
		default:
			fatal(1, "Impossible next %d\n", next);
		}
		break;
	case 2: /* RAM0 to ROM0 or ROM1 */
		switch (next) {
		case 0:
			branch_ROM("RAM0", 0);
			break;
		case 1:
			branch_ROM("RAM0", 1);
			break;
		default:
			fatal(1, "Impossible next %d\n", next);
		}
		break;
	default:
		fatal(1, "Impossible current mpc %d\n", current);
	}
#elif	(UCODE_ROM_PAGES == 1 && UCODE_RAM_PAGES == 3)
	int next = ALTO_GET(cpu.next2,10,1,2);

	switch (current) {
	case 0: /* ROM0 to RAM0, RAM2, RAM1, RAM0 */
		switch (next) {
		case 0:
			branch_RAM("ROM0", 0);
			break;
		case 1:
			branch_RAM("ROM0", 2);
			break;
		case 2:
			branch_RAM("ROM0", 1);
			break;
		case 3:
			branch_RAM("ROM0", 0);
			break;
		default:
			fatal(1, "Impossible next %d\n", next);
		}
		break;
	case 1: /* RAM0 to ROM0, RAM2, RAM1, RAM1 */
		switch (next) {
		case 0:
			branch_ROM("RAM0", 0);
			break;
		case 1:
			branch_RAM("RAM0", 2);
			break;
		case 2:
			branch_RAM("RAM0", 1);
			break;
		case 3:
			branch_RAM("RAM0", 1);
			break;
		default:
			fatal(1, "Impossible next %d\n", next);
		}
		break;
	case 2: /* RAM1 to ROM0, RAM2, RAM0, RAM0 */
		switch (next) {
		case 0:
			branch_ROM("RAM1", 0);
			break;
		case 1:
			branch_RAM("RAM1", 2);
			break;
		case 2:
			branch_RAM("RAM1", 0);
			break;
		case 3:
			branch_RAM("RAM1", 0);
			break;
		default:
			fatal(1, "Impossible next %d\n", next);
		}
		break;
	case 3: /* RAM2 to ROM0, RAM1, RAM0, RAM0 */
		switch (next) {
		case 0:
			branch_ROM("RAM2", 0);
			break;
		case 1:
			branch_RAM("RAM2", 1);
			break;
		case 2:
			branch_RAM("RAM2", 0);
			break;
		case 3:
			branch_RAM("RAM2", 0);
			break;
		default:
			fatal(1, "Impossible next %d\n", next);
		}
		break;
	default:
		fatal(1, "Impossible current mpc %d\n", current);
	}
#else
	fatal(1, "Impossible control ROM/RAM combination %d/%d\n",
		UCODE_ROM_PAGES, UCODE_RAM_PAGES);
#endif
}

/**
 * @brief late: start WRTRAM cycle
 */
void f1_wrtram_1(void)
{
	cpu.wrtram_flag = 1;
	LOG((0,2,"	WRTRAM\n"));
}

/**
 * @brief late: start RDRAM cycle
 */
void f1_rdram_1(void)
{
	cpu.rdram_flag = 1;
	LOG((0,2,"	RDRAM\n"));
}

#if	(UCODE_RAM_PAGES == 3)

/**
 * @brief f1_load_rmr late: load the reset mode register
 *
 * F1=013 corresponds to RMR<- in the emulator. In Altos with the 3K
 * RAM option, F1=013 performs RMR<- in all RAM-related tasks, including
 * the emulator.
 */
static void f1_load_rmr_1(void)
{
	LOG((0,2,"	RMR<-; BUS (%#o)\n", cpu.bus));
	cpu.reset_mode = cpu.bus;
}

#else	/* UCODE_RAM_PAGES == 3*/

/**
 * @brief late: load the S register bank from BUS[12-14]
 */
void f1_load_srb_1(void)
{
	cpu.s_reg_bank[cpu.task] = ALTO_GET(cpu.bus,16,12,14) % SREG_BANKS;
	LOG((0,2,"	SRB<-; srb[%d] := %#o\n", cpu.task, cpu.s_reg_bank[cpu.task]));
}

#endif

/**
 * @brief RAM related task slots initialization
 */
int init_ram(int task)
{
	ram_related[task] = 1;

	SET_FN(bs, ram_read_slocation,		bs_read_sreg_0,	NULL);
	SET_FN(bs, ram_load_slocation,		bs_load_sreg_0,	bs_load_sreg_1);

	SET_FN(f1, ram_swmode,			NULL,		f1_swmode_1);
	SET_FN(f1, ram_wrtram,			NULL, 		f1_wrtram_1);
	SET_FN(f1, ram_rdram,			NULL, 		f1_rdram_1);
#if	(UCODE_RAM_PAGES == 3)
	SET_FN(f1, ram_load_srb,		NULL, 		f1_load_rmr_1);
#else
	SET_FN(f1, ram_load_srb,		NULL, 		f1_load_srb_1);
#endif

	return 0;
}
