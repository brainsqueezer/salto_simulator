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
 * Alto CPU emulation
 *
 * $Id: cpu.c,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alto.h"
#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include "debug.h"
#include "keyboard.h"
#include "display.h"
#include "mouse.h"

/* task headers */
#include "emu.h"
#include "ksec.h"
#include "ether.h"
#include "mrt.h"
#include "dwt.h"
#include "curt.h"
#include "dht.h"
#include "dvt.h"
#include "part.h"
#include "ram.h"
#include "kwd.h"
#include "unused.h"

#ifndef	DEBUG_CPU_TIMESLICES
#define	DEBUG_TIMESLICES	0
#endif

/** @brief set non-zero to double check the ALUF results */
#define	ASSERT_ALUF	1

#define	GET_BANK_NORMAL(breg)	ALTO_GET(breg,16,12,13)
#define	GET_BANK_EXTENDED(breg)	ALTO_GET(breg,16,14,15)

#define	GET_CRAM_IGNORE(addr)	ALTO_GET(addr,16,0,1)
#define	GET_CRAM_BANKSEL(addr)	ALTO_GET(addr,16,2,3)
#define	GET_CRAM_RAMROM(addr)	ALTO_GET(addr,16,4,4)
#define	GET_CRAM_HALFSEL(addr)	ALTO_GET(addr,16,5,5)
#define	GET_CRAM_WORDADDR(addr)	ALTO_GET(addr,16,6,15)

/** @brief raw microcode words */
uint32_t ucode_raw[UCODE_SIZE];

/** @brief constant PROM decoded */
uint32_t const_prom[256];

/** @brief per task bus source function pointers, phases 0 and 1 */
void (*fn_bs[p_COUNT][task_COUNT][bs_COUNT])(void);

/** @brief per task f1 function pointers, early and late */
void (*fn_f1[p_COUNT][task_COUNT][f1_COUNT])(void);

/** @brief per task f2 function pointers, early and late */
void (*fn_f2[p_COUNT][task_COUNT][f2_COUNT])(void);

/** @brief set when task is RAM related */
char ram_related [task_COUNT];

/** @brief the current CPU context */
cpu_t cpu;

/** @brief number of nanoseconds left to execute in the current slice */
ntime_t alto_ntime;

/** @brief flag set by timer.c if alto_execute() shall leave its loop */
int alto_leave;

const char *task_name[task_COUNT] = {
	"emu",		"task01",	"task02",	"task03",
	"ksec",		"task05",	"task06",	"ether",
	"mrt",		"dwt",		"curt",		"dht",
	"dvt",		"part",		"kwd",		"task17"
};

const char *r_name[rsel_COUNT] = {
	"ac3",		"ac2",		"ac1",		"ac0",
	"nww",		"r05",		"pc",		"r07",
	"xh",		"r11",		"ecntr",	"epntr",
	"r14",		"r15",		"r16",		"r17",
	"curx",		"curdata",	"cba",		"aecl",
	"slc",		"mtemp",	"htab",		"ypos",
	"dwa",		"kwdctw",	"cksumrw",	"knmarw",
	"dcbr",		"dwax",		"mask",		"r37"
};

const char *bs_name[bs_COUNT] = {
	"read_r",	"load_r",	"no_source",	"task_3",
	"task_4",	"read_md",	"mouse",	"disp"
};

const char *f1_name[f1_COUNT] = {
	"nop",		"load_mar",	"task",		"block",
	"l_lsh_1",	"l_rsh_1",	"l_lcy_8",	"const",
	"task_10",	"task_11",	"task_12",	"task_13",
	"task_14",	"task_15",	"task_16",	"task_17"
};

const char *f2_name[f2_COUNT] = {
	"nop",		"bus=0",	"shifter<0",	"shifter=0",
	"bus",		"alucy",	"load_md",	"const",
	"task_10",	"task_11",	"task_12",	"task_13",
	"task_14",	"task_15",	"task_16",	"task_17"
};


/**
 * @brief 2k control PROM u3 - 256x4
 *
 * PROM u3 is 256x4 type 3601-1, looks like SN74387, and it
 * controls NEXT[6-9]', i.e. the outputs are wire-AND to NEXT
 *
 *           SN74387
 *         +---+-+---+
 *    A6  -|1  +-+ 16|-  Vcc
 *    A5  -|2      15|-  A7
 *    A4  -|3      14|-  FE1'
 *    A3  -|4      13|-  FE2'
 *    A0  -|5      12|-  D0
 *    A1  -|6      11|-  D1
 *    A2  -|7      10|-  D2
 *   GND  -|8       9|-  D3
 *         +---------+
 *
 *
 * It is enabled whenever the Emulator task is active and:
 *	both F2[0] and F[1] are 1	F2 functions 014, 015, 016, 017
 *	F2=14 is 0			not for F2 = 14 (load IR<-)
 *	IR[0] is 0			not for arithmetic group
 *
 * This means it controls the F2 functions 015:IDISP<- and 016:<-ACSOURCE
 *
 * Its address lines are:
 *	line	pin		connected to		load swap
 *	-------------------------------------------------------------------
 *	A0	5		F2[2] (i.e. MIR[18])	IR[07]
 *	A1	6		IR[01]			IR[06]
 *	A2	7		IR[02]			IR[05]
 *	A3	4		IR[03]			IR[04]
 *	A4	3		IR[04]			IR[03]
 *	A5	2		IR[05]			IR[02]
 *	A6	1		IR[06]			IR[01]
 *	A7	15		IR[07]			F2[2]
 *
 * Its data lines are:
 *	line	pin		connected to		load
 *	-------------------------------------------------------------------
 *	D3	9		NEXT[06]'		NEXT[06]
 *	D2	10		NEXT[07]'		NEXT[07]
 *	D1	11		NEXT[08]'		NEXT[08]
 *	D0	12		NEXT[09]'		NEXT[09]
 * 
 * Its address lines are reversed at load time to make it easier to
 * access it. Also both, address and data lines, are inverted.
 *
 * 0000: 000,000,013,016,012,016,014,016,000,001,013,016,012,016,016,016,
 * 0020: 010,001,013,016,012,016,015,016,010,001,013,016,012,016,017,016,
 * 0040: 004,001,013,017,012,017,014,017,004,001,013,017,012,017,016,017,
 * 0060: 014,001,013,017,012,017,015,017,014,001,013,017,012,017,017,017,
 * 0100: 002,001,013,016,012,016,014,016,002,001,013,016,012,016,016,016,
 * 0120: 012,001,013,016,012,016,015,016,012,001,013,016,012,016,017,016,
 * 0140: 006,001,013,017,012,017,014,017,006,013,013,017,012,017,016,017,
 * 0160: 017,001,013,017,012,017,015,017,017,001,013,017,012,017,017,017,
 * 0200: 011,001,013,016,012,016,014,016,011,016,013,016,012,016,016,016,
 * 0220: 001,001,013,016,012,016,015,016,001,001,013,016,012,016,017,016,
 * 0240: 005,001,013,017,012,017,014,017,005,013,013,017,012,017,016,017,
 * 0260: 015,001,013,017,012,017,015,017,015,014,013,017,012,017,017,017,
 * 0300: 003,001,013,016,012,016,014,016,003,001,013,016,012,016,016,016,
 * 0320: 013,001,013,016,012,016,015,016,013,001,013,016,012,016,017,016,
 * 0340: 007,001,013,017,012,017,014,017,007,001,013,017,012,017,016,017,
 * 0360: 016,001,013,017,012,017,015,017,016,015,013,017,012,017,017,017
 */
uint8_t ctl2k_u3[256];

/**
 * @brief 2k control PROM u38 - 32x8
 *
 * 0000: 0367,0353,0323,0315,0265,0251,0221,0216,
 * 0010: 0166,0152,0122,0114,0064,0050,0020,0017,
 * 0020: 0000,0000,0000,0000,0000,0000,0000,0000,
 * 0030: 0000,0000,0000,0000,0000,0000,0000,0000
 */
uint8_t ctl2k_u38[32];

/**
 * @brief 2k control PROM u76 - 256x4
 *
 * 0000: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0020: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0040: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0060: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0100: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0120: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0140: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0160: 000,014,000,000,000,000,000,000,014,000,000,000,000,000,000,000,
 * 0200: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0220: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0240: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0260: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0300: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0320: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0340: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000,
 * 0360: 000,014,000,000,000,000,000,000,000,014,000,000,000,000,000,000
 */
uint8_t ctl2k_u76[256];


/**
 * @brief 3k CRAM PROM a37
 *
 * 0000: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0020: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0040: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0060: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0100: 014,014,014,014,014,014,014,014,014,014,014,014,014,014,014,014,
 * 0120: 014,014,014,016,014,014,014,004,014,014,014,010,014,014,014,015,
 * 0140: 015,015,015,015,015,015,015,015,015,015,015,015,015,015,015,015,
 * 0160: 015,015,015,017,015,015,015,005,015,015,015,011,015,015,015,014,
 * 0200: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0220: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0240: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0260: 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
 * 0300: 014,014,014,014,014,014,014,014,014,014,014,014,014,014,014,014,
 * 0320: 014,014,014,016,014,014,014,014,014,014,014,014,014,014,014,015,
 * 0340: 015,015,015,015,015,015,015,015,015,015,015,015,015,015,015,015,
 * 0360: 015,015,015,017,015,015,015,015,015,015,015,015,015,015,015,014
 */
uint8_t cram3k_a37[256];

/**
 * @brief memory addressing PROM a64
 *
 * This dump is from PROM madr.a64:
 * 0000: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0020: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0040: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0060: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0100: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0120: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0140: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0160: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0200: 004,004,004,004,004,004,000,000,004,004,004,004,004,004,004,004,
 * 0220: 004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,004,
 * 0240: 004,004,004,004,004,004,000,000,004,004,004,004,004,004,004,004,
 * 0260: 004,004,004,004,004,004,004,014,004,004,004,004,004,005,004,004,
 * 0300: 004,004,004,004,004,004,000,000,004,004,004,004,004,004,004,004,
 * 0320: 004,004,004,004,004,004,006,006,004,004,004,004,004,004,004,004,
 * 0340: 004,004,004,004,004,004,000,000,004,004,004,004,004,004,004,004,
 * 0360: 004,004,004,004,004,004,004,004,004,004,004,004,004,005,004,004
 */
uint8_t madr_a64[256];

/**
 * @brief memory addressing PROM a65
 *
 * This dump is from PROM madr.a65:
 * 0000: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0020: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0040: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0060: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0100: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0120: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0140: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0160: 007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,007,
 * 0200: 007,007,007,007,007,017,007,017,007,007,007,007,007,014,007,014,
 * 0220: 007,007,007,007,007,015,007,015,007,007,007,007,007,013,007,016,
 * 0240: 007,007,007,007,007,017,007,017,007,007,007,007,007,014,007,014,
 * 0260: 007,007,007,007,007,007,007,007,007,007,007,007,007,011,007,016,
 * 0300: 007,007,007,007,007,017,007,017,007,007,007,007,007,014,007,014,
 * 0320: 007,007,007,007,007,007,007,007,007,007,007,007,007,012,007,016,
 * 0340: 007,007,007,007,007,017,007,017,007,007,007,007,007,014,007,014,
 * 0360: 007,007,007,007,007,007,007,007,007,007,007,007,007,010,007,016
 */
uint8_t madr_a65[256];

static void fn_bs_bad_0(void)
{
	fatal(9,"fatal: bad early bus source pointer for task %s, mpc:%05o bs:%s\n",
		task_name[cpu.task], cpu.mpc, bs_name[MIR_BS(cpu.mir)]);
}

static void fn_bs_bad_1(void)
{
	fatal(9,"fatal: bad late bus source pointer for task %s, mpc:%05o bs: %s\n",
		task_name[cpu.task], cpu.mpc, bs_name[MIR_BS(cpu.mir)]);
}

static void fn_f1_bad_0(void)
{
	fatal(9,"fatal: bad early f1 function pointer for task %s, mpc:%05o f1: %s\n",
		task_name[cpu.task], cpu.mpc, f1_name[MIR_F1(cpu.mir)]);
}

static void fn_f1_bad_1(void)
{
	fatal(9,"fatal: bad late f1 function pointer for task %s, mpc:%05o f1: %s\n",
		task_name[cpu.task], cpu.mpc, f1_name[MIR_F1(cpu.mir)]);
}

static void fn_f2_bad_0(void)
{
	fatal(9,"fatal: bad early f2 function pointer for task %s, mpc:%05o f2: %s\n",
		task_name[cpu.task], cpu.mpc, f2_name[MIR_F2(cpu.mir)]);
}

static void fn_f2_bad_1(void)
{
	fatal(9,"fatal: bad late f2 function pointer for task %s, mpc:%05o f2: %s\n",
		task_name[cpu.task], cpu.mpc, f2_name[MIR_F2(cpu.mir)]);
}

/**
 * @brief read bank register in memory mapped I/O range
 *
 * The bank registers are stored in a 16x4-bit RAM 74S189.
 */
static int bank_reg_r(int address)
{
	int task = address & 017;
	int bank = cpu.bank_reg[task] | 0177760;
	return bank;
}

/**
 * @brief write bank register in memory mapped I/O range
 *
 * The bank registers are stored in a 16x4-bit RAM 74S189.
 */
static void bank_reg_w(int address, int data)
{
	int task = address & 017;
	cpu.bank_reg[task] = data & 017;
	LOG((0,0,"	write bank[%02o]=%#o normal:%o extended:%o (%s)\n",
		task, data,
		GET_BANK_NORMAL(data),
		GET_BANK_EXTENDED(data),
		task_name[task]));
}

/**
 * @brief switch to cpu.next_task now
 */
static void switch_task(void)
{
	if (!cpu.task_switch)
		return;
	/* save this task's mpc */
	cpu.task_mpc[cpu.task] = cpu.mpc;
	cpu.task = cpu.next_task;
	cpu.task_switch = 0;
	LOG((log_TSW,1, "task switch to %02o:%s (cycle %lld)\n",
		cpu.task, task_name[cpu.task], ntime() / CPU_MICROCYCLE_TIME));
	cpu.mpc = cpu.task_mpc[cpu.task];
	cpu.next2 = 0;
}

/**
 * @brief bs_read_r early: drive bus by R register
 */
static void bs_read_r_0(void)
{
	int and = cpu.r[cpu.rsel];
	LOG((0,2,"	<-R%02o; %s (%#o)\n",
		cpu.rsel, r_name[cpu.rsel], and));
	cpu.bus &= and;
}

/**
 * @brief bs_load_r early: load R places 0 on the BUS
 */
static void bs_load_r_0(void)
{
	int and = 0;
	LOG((0,2,"	R%02o<-; %s (BUS&=0)\n",
		cpu.rsel, r_name[cpu.rsel]));
	cpu.bus &= and;
}

/**
 * @brief bs_load_r late: load R from SHIFTER
 */
static void bs_load_r_1(void)
{
	cpu.r[cpu.rsel] = cpu.shifter;
	LOG((0,2,"	R%02o<-; %s = SHIFTER (%#o)\n",
		cpu.rsel, r_name[cpu.rsel], cpu.r[cpu.rsel]));
}

/**
 * @brief bs_read_md early: drive BUS from read memory data
 */
static void bs_read_md_0(void)
{
#if	DEBUG
	int mar = mem.mar;
#endif
	int md = read_mem();
	LOG((0,2,"	<-MD; BUS&=MD (%#o=[%#o])\n", md, mar));
	cpu.bus &= md;
}

/**
 * @brief bs_mouse early: drive bus by mouse
 */
static void bs_mouse_0(void)
{
	int and = mouse_read();
	LOG((0,2,"	<-MOUSE; BUS&=MOUSE (%#o)\n", and));
	cpu.bus &= and;
}

/**
 * @brief early: drive bus by displacement (which?)
 */
static void bs_disp_0(void)
{
	int and = 0177777;
	fprintf(stderr, "BS <-DISP not handled by task %s mpc:%04x\n",
		task_name[cpu.task], cpu.mpc);
	LOG((0,2,"	<-DISP; BUS&=DISP ?? (%#o)\n", and));
	cpu.bus &= and;
}

/**
 * @brief f1_load_mar late: load memory address register
 *
 * Load memory address register from the ALU output;
 * start main memory reference (see section 2.3).
 */
static void f1_load_mar_1(void)
{
	int bank = cpu.bank_reg[cpu.task];
	int msb;
	if (MIR_F2(cpu.mir) == f2_load_md) {
		msb = GET_BANK_EXTENDED(bank) << 16;
		LOG((0,0, "	XMAR %#o\n", msb | cpu.alu));
	} else {
		msb = GET_BANK_NORMAL(bank) << 16;

	}
	load_mar(cpu.rsel, msb | cpu.alu);
}

/**
 * @brief f1_task early: task switch
 *
 * The priority encoder finds the highest task requesting service
 * and switches the task number after the next cycle.
 */
static void f1_task_0(void)
{
	int i;
	LOG((0,2, "	TASK %02o:%s", cpu.task, task_name[cpu.task]));
	for (i = 15; i >= 0; i--) {
		if (CPU_GET_TASK_WAKEUP(i)) {
			cpu.next2_task = i;
			if (CPU_GET_TASK_AUTOBLOCK(i))
				CPU_CLR_TASK_WAKEUP(i);
			if (cpu.next2_task != cpu.next_task) {
				LOG((0,2, " switch to %02o:%s\n", cpu.next2_task, task_name[cpu.next2_task]));
				cpu.task_switch = 1;
			} else {
				LOG((0,2, " high prio\n"));
			}
			return;
		}
	}
	fatal(3, "no tasks requesting service\n");
}

#ifdef	f1_block0_unused
/**
 * @brief f1_block early: block task
 *
 * The task request for the active task is cleared
 */
static void f1_block_0(void)
{
	CPU_CLR_TASK_WAKEUP(cpu.task);
	LOG((0,2, "	BLOCK %02o:%s\n", cpu.task, task_name[cpu.task]));
}
#endif

/**
 * @brief f2_bus_eq_0 late: branch on bus equals zero
 */
static void f2_bus_eq_0_1(void)
{
	int or = cpu.bus == 0 ? 1 : 0;
	LOG((0,2, "	BUS=0; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_shifter_lt_0 late: branch on shifter less than zero
 */
static void f2_shifter_lt_0_1(void)
{
	int or = cpu.shifter & 0100000 ? 1 : 0;
	LOG((0,2, "	SH<0; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_shifter_eq_0 late: branch on shifter equals zero
 */
static void f2_shifter_eq_0_1(void)
{
	int or = cpu.shifter == 0 ? 1 : 0;
	LOG((0,2, "	SH=0; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_bus late: branch on bus bits BUS[6-15]
 */
static void f2_bus_1(void)
{
	int or = ALTO_GET(cpu.bus,16,6,15);
	LOG((0,2, "	BUS; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_alucy late: branch on latched alu carry output
 */
static void f2_alucy_1(void)
{
	int or = cpu.laluc0 ^ 1;
	LOG((0,2, "	ALUCY; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_load_md late: load memory data
 *
 * Deliver BUS data to memory.
 */
static void f2_load_md_1(void)
{
#if	DEBUG
	int mar = mem.mar;
#endif
	if (MIR_F1(cpu.mir) == f1_load_mar) {
		/* part of an XMAR */
		LOG((0,0, "	XMAR %#o (%#o)\n", mar, cpu.bus));
	} else {
		write_mem(cpu.bus);
		LOG((0,2, "	MD<- BUS ([%#o]=%#o)\n", mar, cpu.bus));
	}
}

/**
 * @brief read the microcode ROM/RAM halfword
 */
static void rdram(void)
{
	int addr, val;

	if (GET_CRAM_RAMROM(cpu.cram_addr)) {
		/* read ROM 0 at current mpc */
		addr = cpu.mpc & 01777;
	} else {
		/* read RAM 0,1,2 */
		addr = UCODE_RAM_BASE +
			GET_CRAM_WORDADDR(cpu.cram_addr) +
			GET_CRAM_BANKSEL(cpu.cram_addr) * UCODE_PAGE_SIZE;
	}
	if (addr < UCODE_SIZE) {
		val = ucode_raw[addr];
		printf("rdram: ucode[%#x]=%#x\n", addr, val);
		if (GET_CRAM_HALFSEL(cpu.cram_addr)) {
			val = (val ^ UCODE_INVERTED_BITS) >> 16;
			LOG((0,2,"	RDRAM; BUS &= ucode[%#o] high (%#o)\n", addr, val));
		} else {
			val = (val ^ UCODE_INVERTED_BITS) & 0177777;
			LOG((0,2,"	RDRAM; BUS &= ucode[%#o] low (%#o)\n", addr, val));
		}
	} else {
		val = 0177777 ^ UCODE_INVERTED_BITS;	/* ??? */
		LOG((0,2,"	RDRAM; CRAM address error: address:#0, data:%#o\n", addr, val));
	}
	cpu.rdram_flag = 0;
	cpu.bus &= val;
}

/**
 * @brief write the microcode RAM from the L register and ALU
 */
static void wrtram (void)
{
	int addr, val;

	/* write RAM 0,1,2 */
	addr = UCODE_RAM_BASE +
		GET_CRAM_WORDADDR(cpu.cram_addr) +
		GET_CRAM_BANKSEL(cpu.cram_addr) * UCODE_PAGE_SIZE;
	val = ((cpu.l << 16) | cpu.alu) ^ UCODE_INVERTED_BITS;
	if (addr < UCODE_SIZE) {
		printf("wrtram: ucode[%#x]=%#x\n", addr, val);
		LOG((0,2,"	WRTRAM; ucode[%#o] = l,alu (%#o)\n", addr, val));
		ucode_raw[addr] = val;
	} else {
		LOG((0,0,"	WRTRAM; CRAM address error: address:%#o, data:%#o\n", addr, val));
	}
	cpu.wrtram_flag = 0;
}


/** @brief software initiated reset (STARTF) */
int alto_soft_reset(void)
{
	int task;

	for (task = 0; task < task_COUNT; task++) {
		/* every task starts at mpc = task number, in either ROM0 or RAM0 */
		if (cpu.reset_mode & (1 << task))
			cpu.task_mpc[task] = task;
		else
			cpu.task_mpc[task] = UCODE_RAM_BASE + task;
	}
	/* swithc to task 0 */
	cpu.next2_task = 0;
	cpu.task_switch = 1;

	/* all tasks start in ROM0 again */
	cpu.reset_mode = 0xffff;

	/* reset the display state machine values */
	cpu.dsp_counter = 0;
	cpu.dsp_state = 020;

	/* return next task (?) */
	return cpu.next_task;
}


/** @brief reset the various registers */
int alto_reset(void)
{
	int task;

	memset(&cpu, 0, sizeof(cpu));

	cpu.dsp_counter = 0;
	cpu.dsp_state = 020;

	/* all tasks start in ROM0 */
	cpu.reset_mode = 0xffff;

	memset(&ram_related, 0, sizeof(ram_related));

	/* install standard handlers in all tasks */
	for (task = 0; task < task_COUNT; task++) {
		int i;

		/* every task starts at mpc = task number, in either ROM0 or RAM0 */
		if (cpu.reset_mode & (1 << task))
			cpu.task_mpc[task] = task;
		else
			cpu.task_mpc[task] = UCODE_RAM_BASE + task;

		for (i = 0; i < bs_COUNT; i++) {
			fn_bs[0][task][i]		= fn_bs_bad_0;
			fn_bs[1][task][i]		= fn_bs_bad_1;
		}
		for (i = 0; i < f1_COUNT; i++) {
			fn_f1[0][task][i]		= fn_f1_bad_0;
			fn_f1[1][task][i]		= fn_f1_bad_1;
		}
		for (i = 0; i < f2_COUNT; i++) {
			fn_f2[0][task][i]		= fn_f2_bad_0;
			fn_f2[1][task][i]		= fn_f2_bad_1;
		}

		SET_FN(bs, read_r,		bs_read_r_0,	NULL);
		SET_FN(bs, load_r,		bs_load_r_0,	bs_load_r_1);
		SET_FN(bs, no_source,		NULL,		NULL);
		/* bs_task_3 and bs_task_4 are task specific */
#if	1
		SET_FN(bs, task_3,		NULL,		NULL);
		SET_FN(bs, task_4,		NULL,		NULL);
#endif
		SET_FN(bs, read_md,		bs_read_md_0,	NULL);
		SET_FN(bs, mouse,		bs_mouse_0,	NULL);
		SET_FN(bs, disp,		bs_disp_0,	NULL);

		SET_FN(f1, nop,			NULL,		NULL);
		SET_FN(f1, load_mar,		NULL,		f1_load_mar_1);
		SET_FN(f1, task,		f1_task_0,	NULL);
#if	0
		/* not all tasks have the BLOCK f1 */
		SET_FN(f1, block,		f1_block_0,	NULL);
#endif
		SET_FN(f1, l_lsh_1,		NULL,		NULL);
		SET_FN(f1, l_rsh_1,		NULL,		NULL);
		SET_FN(f1, l_lcy_8,		NULL,		NULL);
		SET_FN(f1, const,		NULL,		NULL);
		/* f1_task_10 to f1_task_17 are task specific */

		SET_FN(f2, nop,			NULL,		NULL);
		SET_FN(f2, bus_eq_0,		NULL,		f2_bus_eq_0_1);
		SET_FN(f2, shifter_lt_0,	NULL,		f2_shifter_lt_0_1);
		SET_FN(f2, shifter_eq_0,	NULL,		f2_shifter_eq_0_1);
		SET_FN(f2, bus,			NULL,		f2_bus_1);
		SET_FN(f2, alucy,		NULL,		f2_alucy_1);
		SET_FN(f2, load_md,		NULL,		f2_load_md_1);
		SET_FN(f2, const,		NULL,		NULL);
		/* f2_task_10 to f2_task_17 are task specific */
	}

	init_emu(task_emu);
	init_unused(task_1);
	init_unused(task_2);
	init_unused(task_3);
	init_ksec(task_ksec);
	init_unused(task_5);
	init_unused(task_6);
	init_ether(task_ether);
	init_mrt(task_mrt);
	init_dwt(task_dwt);
	init_curt(task_curt);
	init_dht(task_dht);
	init_dvt(task_dvt);
	init_part(task_part);
	init_kwd(task_kwd);
	init_unused(task_17);

	install_mmio_fn(0177740, 0177757, bank_reg_r, bank_reg_w);

	/* start with task 0 */
	cpu.task = 0;
	CPU_SET_TASK_WAKEUP(cpu.task);

	return 0;
}

/**
 * @brief call the display state machine on appropriate invtervals
 *
 * On every seventh CPU cycle call the display_state_machine()
 * This happens every 7*170ns = 1190ns, which is close enough
 * to 24 * the display's pixel clock rate.
 */
static void cpu_display_state_machine(void)
{
	if (--cpu.dsp_counter < 0) {
		cpu.dsp_state = display_state_machine(cpu.dsp_state);
		cpu.dsp_counter = 7;
	}
}

/**
 * @brief Functional description of the 4-bit ALU 74181
 *
 * The 74181 is a 4-bit high speed parallel Arithmetic Logic Unit (ALU).
 * Controlled by four Function Select inputs (S0-S3) and the Mode Control
 * input (M), it can perform all the 16 possible logic operations or 16
 * different arithmetic operations on active HIGH or active LOW operands.
 * The Function Table lists these operations.
 *
 * When the Mode Control input (M) is HIGH, all internal carries are
 * inhibited and the device performs logic operations on the individual
 * bits as listed. When the Mode Control input is LOW, the carries are
 * enabled and the device performs arithmetic operations on the two 4-bit
 * words. The device incorporates full internal carry lookahead and
 * provides for either ripple carry between devices using the Cn+4 output,
 * or for carry lookahead between packages using the signals P' (Carry
 * Propagate) and G' (Carry Generate). In the ADD mode, P' indicates that
 * F' is 15 or more, while G' indicates that F' is 16 or more. In the
 * SUBTRACT mode, P' indicates that F' is zero or less, while G' indicates
 * that F' is less than zero. P' and G' are not affected by carry in.
 * When speed requirements are not stringent, it can be used in a simple
 * ripple carry mode by connecting the Carry output (Cn+4) signal to the
 * Carry input (Cn) of the next unit. For high speed operation the device
 * is used in conjunction with the 74182 carry lookahead circuit. One
 * carry lookahead package is required for each group of four 74181 devices.
 * Carry lookahead can be provided at various levels and offers high speed
 * capability over extremely long word lengths.
 *
 * The A=B output from the device goes HIGH when all four F' outputs are
 * HIGH and can be used to indicate logic equivalence over four bits when
 * the unit is in the subtract mode. The A=B output is open collector and
 * can be wired-AND with other A=B outputs to give a comparison for more
 * than four bits. The A=B signal can also be used with the Cn+4 signal
 * to indicated A>B and A<B.
 *
 * The Function Table lists the arithmetic operations that are performed
 * without a carry in. An incoming carry adds a one to each operation.
 * Thus, select code 0110 generates A minus B minus 1 (2's complement
 * notation) without a carry in and generates A minus B when a carry is
 * applied. Because subtraction is actually performed by the complementary
 * addition (1's complement), a carry out means borrow; thus a carry is
 * generated when there is no underflow and no carry is generated when
 * there is underflow. As indicated, this device can be used with either
 * active LOW inputs producing active LOW outputs or with active HIGH
 * inputs producing active HIGH outputs. For either case the table lists
 * the operations that are performed to the operands labeled inside the
 * logic symbol.
 *
 * The AltoI/II use four 74181s and a 74182 carry lookahead circuit,
 * and the inputs and outputs are all active HIGH.
 *
 * Active HIGH operands:
 *
 * +-------------------+-------------+------------------------+------------------------+
 * |    Mode Select    |   Logic     | Arithmetic w/o carry[2]| Arithmetic w/ carry [2]|
 * |      Inputs       |             |                        |                        |
 * |  S3  S2  S1  S0   |   (M=1)     | (M=0) (Cn=1)           | (M=0) (Cn=0)           |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   0   0   | A'          | A                      | A + 1                  |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   0   1   | A' | B'     | A | B                  | (A | B) + 1            |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   1   0   | A' & B      | A | B'                 | (A | B') + 1           |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   1   1   | logic 0     | - 1                    | 0                      |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   0   0   | (A & B)'    | A + (A & B')           | A + (A & B') + 1       |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   0   1   | B'          | (A | B) + (A & B')     | (A | B) + (A & B') + 1 |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   1   0   | A ^ B       | A - B - 1              | A - B                  |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   1   1   | A & B'      | (A & B) - 1            | A & B                  |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   0   0   | A' | B      | A + (A & B)            | A + (A & B) + 1        |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   0   1   | A' ^ B'     | A + B                  | A + B + 1              |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   1   0   | B           | (A | B') + (A & B)     | (A | B') + (A & B) + 1 |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   1   1   | A & B       | (A & B) - 1            | A & B                  |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   0   0   | logic 1     | A + A               [1]| A + A + 1           [1]|
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   0   1   | A | B'      | (A | B) + A            | (A | B) + A + 1        |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   1   0   | A | B       | (A | B') + A           | (A | B') + A + 1       |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   1   1   | A           | A - 1                  | A                      |
 * +-------------------+-------------+------------------------+------------------------+
 *  
 * [1] Each bit is shifted to the next most significant position.
 * [2] Arithmetic operations expressed in 2's complement notation.
 */
#define	SMC(s3,s2,s1,s0,m,c) (32*(s3)+16*(s2)+8*(s1)+4*(s0)+2*(m)+(c))

static __inline int alu_74181(int smc)
{
	register uint32_t a = cpu.bus;
	register uint32_t b = cpu.t;
	register uint32_t f = 0;

	if (smc & 2) {
		/* Mode Select 1: Logic */
		switch (smc / 4) {
		case  0: f = ~a;			break;	/* 0000: A'       */
		case  1: f = ~a | ~b;			break;	/* 0001: A' | B'  */
		case  2: f = ~a & b;			break;	/* 0010: A' & B   */
		case  3: f = 0;				break;	/* 0011: logic 0  */
		case  4: f = ~(a & b);			break;	/* 0100: (A & B)' */
		case  5: f = ~b;			break;	/* 0101: B'       */
		case  6: f = a ^ b;			break;	/* 0110: A ^ B    */
		case  7: f = a & ~b;			break;	/* 0111: A & B'   */
		case  8: f = ~a | b;			break;	/* 1000: A' | B   */
		case  9: f = ~a ^ ~b;			break;	/* 1001: A' ^ B'  */
		case 10: f = b;				break;	/* 1010: B        */
		case 11: f = a & b;			break;	/* 1011: A & B    */
		case 12: f = ~0;			break;	/* 1100: logic 1  */
		case 13: f = a | ~b;			break;	/* 1101: A | B'   */
		case 14: f = a | b;			break;	/* 1110: A | B    */
		case 15: f = a;				break;	/* 1111: A        */
		}
		/* Cn+z = no carry */
		cpu.aluc0 = 1;
		return f & 0177777;
	}
	/* Mode Select 0: Arithmetic */
	if (smc & 1) {
		/* Cn = 1: Arithmetic w/o carry */
		switch (smc / 4) {
		case  0: f = a;				break;	/* 0000: A                      */
		case  1: f = a | b;			break;	/* 0001: A | B                  */
		case  2: f = a | ~b;			break;	/* 0010: A | B'                 */
		case  3: f = ~0;			break;	/* 0011: -1                     */
		case  4: f = a + (a & ~b);		break;	/* 0100: A + (A & B')           */
		case  5: f = (a | b) + (a & ~b);	break;	/* 0101: (A | B) + (A & B')     */
		case  6: f = a + ~b;			break;	/* 0110: A - B - 1              */
		case  7: f = (a & b) + ~0;		break;	/* 0111: (A & B) - 1            */
		case  8: f = a + (a & b);		break;	/* 1000: A + (A & B)            */
		case  9: f = a + b;			break;	/* 1001: A + B                  */
		case 10: f = (a | ~b) + (a & b);	break;	/* 1010: (A | B') + (A & B)     */
		case 11: f = (a & b) + ~0;		break;	/* 1011: (A & B) - 1            */
		case 12: f = a + a;			break;	/* 1100: A + A                  */
		case 13: f = (a | b) + a;		break;	/* 1101: (A | B) + A            */
		case 14: f = (a | ~b) + a;		break;	/* 1110: (A | B') + A           */
		case 15: f = a + ~0;			break;	/* 1111: A - 1                  */
		}
	} else {
		/* Cn = 0: Arithmetic with carry */
		switch (smc / 4) {
		case  0: f = a + 1;			break;	/* 0000: A + 1                  */
		case  1: f = (a | b) + 1;		break;	/* 0001: (A | B) + 1            */
		case  2: f = (a | ~b) + 1;		break;	/* 0010: (A | B') + 1           */
		case  3: f = 0;				break;	/* 0011: 0                      */
		case  4: f = a + (a & ~b) + 1;		break;	/* 0100: A + (A & B') + 1       */
		case  5: f = (a | b) + (a & ~b) + 1;	break;	/* 0101: (A | B) + (A & B') + 1 */
		case  6: f = a + ~b + 1;		break;	/* 0110: A - B                  */
		case  7: f = a & b;			break;	/* 0111: A & B                  */
		case  8: f = a + (a & b) + 1;		break;	/* 1000: A + (A & B) + 1        */
		case  9: f = a + b + 1;			break;	/* 1001: A + B + 1              */
		case 10: f = (a | ~b) + (a & b) + 1;	break;	/* 1010: (A | B') + (A & B) + 1 */
		case 11: f = a & b;			break;	/* 1011: A & B                  */
		case 12: f = a + a + 1;			break;	/* 1100: A + A + 1              */
		case 13: f = (a | b) + a + 1;		break;	/* 1101: (A | B) + A + 1        */
		case 14: f = (a | ~b) + a + 1;		break;	/* 1110: (A | B') + A + 1       */
		case 15: f = a;				break;	/* 1111: A                      */
		}
	}
	/* Cn+z = carry */
	cpu.aluc0 = (~f >> 16) & 1;
	return f & 0177777;
}

/** @brief flag that tells whether to load the T register from BUS or ALU */
#define	TSELECT	1

/** @brief flag the inverted operation M=1 (logic) or M=0 (arithmetic) */
#define	ALUM2	2

/**
 * @brief compute the ALU function, and return the TSELECT and ALUM2 flags
 */
static __inline int alto_aluf(int aluf)
{
	register int flags;

	switch (aluf) {
	/**
	 * 00: ALU <- BUS
	 * PROM data for S3-0:1111 M:1 C:0
	 * 74181 function F=A
	 * T source is ALU
	 */
	case aluf_bus__alut:
		cpu.alu = alu_74181(SMC(1,1,1,1, 1, 0));
		flags = TSELECT;
		LOG((0,2,"	ALU<- BUS (%#o)\n", cpu.alu));
#if	ASSERT_ALUF
		if (cpu.alu != cpu.bus)
			fatal(1,"aluf 0000");
#endif
		break;
	/**
	 * 01: ALU <- T
	 * PROM data for S3-0:1010 M:1 C:0
	 * 74181 function F=B
	 * T source is BUS
	 */
	case aluf_treg:
		cpu.alu = alu_74181(SMC(1,0,1,0, 1, 0));
		flags = 0;
		LOG((0,2,"	ALU<- T (%#o)\n", cpu.alu));
#if	ASSERT_ALUF
		if (cpu.alu != cpu.t)
			fatal(1,"aluf 0001");
#endif
		break;
	/**
	 * 02: ALU <- BUS | T
	 * PROM data for S3-0:1110 M:1 C:0
	 * 74181 function F=A|B
	 * T source is ALU
	 */
	case aluf_bus_or_t__alut:
		cpu.alu = alu_74181(SMC(1,1,1,0, 1, 0));
		flags = TSELECT;
		LOG((0,2,"	ALU<- BUS OR T (%#o := %#o | %#o)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != (cpu.bus | cpu.t))
			fatal(1,"aluf 0010");
#endif
		break;
	/**
	 * 03: ALU <- BUS & T
	 * PROM data for S3-0:1011 M:1 C:0
	 * 74181 function F=A&B
	 * T source is BUS
	 */
	case aluf_bus_and_t:
		cpu.alu = alu_74181(SMC(1,0,1,1, 1, 0));
		flags = 0;
		LOG((0,2,"	ALU<- BUS AND T (%#o := %#o & %#o)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != (cpu.bus & cpu.t))
			fatal(1,"aluf 0011");
#endif
		break;
	/**
	 * 04: ALU <- BUS ^ T
	 * PROM data for S3-0:0110 M:1 C:0
	 * 74181 function F=A^B
	 * T source is BUS
	 */
	case aluf_bus_xor_t:
		cpu.alu = alu_74181(SMC(0,1,1,0, 1, 0));
		flags = 0;
		LOG((0,2,"	ALU<- BUS XOR T (%#o := %#o ^ %#o)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != (cpu.bus ^ cpu.t))
			fatal(1,"aluf 0100");
#endif
		break;
	/**
	 * 05: ALU <- BUS + 1
	 * PROM data for S3-0:0000 M:0 C:0
	 * 74181 function F=A+1
	 * T source is ALU
	 */
	case aluf_bus_plus_1__alut:
		cpu.alu = alu_74181(SMC(0,0,0,0, 0, 0));
		flags = ALUM2 | TSELECT;
		LOG((0,2,"	ALU<- BUS + 1 (%#o := %#o + 1)\n",
			cpu.alu, cpu.bus));
#if	ASSERT_ALUF
		if (cpu.alu != ((cpu.bus + 1) & 0177777))
			fatal(1,"aluf 0101");
#endif
		break;
	/**
	 * 06: ALU <- BUS - 1
	 * PROM data for S3-0:1111 M:0 C:1
	 * 74181 function F=A-1
	 * T source is ALU
	 */
	case aluf_bus_minus_1__alut:
		cpu.alu = alu_74181(SMC(1,1,1,1, 0, 1));
		flags = ALUM2 | TSELECT;
		LOG((0,2,"	ALU<- BUS - 1 (%#o := %#o - 1)\n",
			cpu.alu, cpu.bus));
#if	ASSERT_ALUF
		if (cpu.alu != ((cpu.bus - 1) & 0177777))
			fatal(1,"aluf 0110");
#endif
		break;
	/**
	 * 07: ALU <- BUS + T
	 * PROM data for S3-0:1001 M:0 C:1
	 * 74181 function F=A+B
	 * T source is BUS
	 */
	case aluf_bus_plus_t:
		cpu.alu = alu_74181(SMC(1,0,0,1, 0, 1));
		flags = ALUM2;
		LOG((0,2,"	ALU<- BUS + T (%#o := %#o + %#o)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != ((cpu.bus + cpu.t) & 0177777))
			fatal(1,"aluf 0111");
#endif
		break;
	/**
	 * 10: ALU <- BUS - T
	 * PROM data for S3-0:0110 M:0 C:0
	 * 74181 function F=A-B
	 * T source is BUS
	 */
	case aluf_bus_minus_t:
		cpu.alu = alu_74181(SMC(0,1,1,0, 0, 0));
		flags = ALUM2;
		LOG((0,2,"	ALU<- BUS - T (%#o := %#o - %#o)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != ((cpu.bus - cpu.t) & 0177777))
			fatal(1,"aluf 1000");
#endif
		break;
	/**
	 * 11: ALU <- BUS - T - 1
	 * PROM data for S3-0:0110 M:0 C:1
	 * 74181 function F=A-B-1
	 * T source is BUS
	 */
	case aluf_bus_minus_t_minus_1:
		cpu.alu = alu_74181(SMC(0,1,1,0, 0, 1));
		flags = ALUM2;
		LOG((0,2,"	ALU<- BUS - T - 1 (%#o := %#o - %#o - 1)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != ((cpu.bus - cpu.t - 1) & 0177777))
			fatal(1,"aluf 1001");
#endif
		break;
	/**
	 * 12: ALU <- BUS + T + 1
	 * PROM data for S3-0:1001 M:0 C:0
	 * 74181 function F=A+B+1
	 * T source is ALU
	 */
	case aluf_bus_plus_t_plus_1__alut:
		cpu.alu = alu_74181(SMC(1,0,0,1, 0, 0));
		flags = ALUM2 | TSELECT;
		LOG((0,2,"	ALU<- BUS + T + 1 (%#o := %#o + %#o + 1)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != ((cpu.bus + cpu.t + 1) & 0177777))
			fatal(1,"aluf 1010");
#endif
		break;
	/**
	 * 13: ALU <- BUS + SKIP
	 * PROM data for S3-0:0000 M:0 C:SKIP
	 * 74181 function F=A (SKIP=1) or F=A+1 (SKIP=0)
	 * T source is ALU
	 */
	case aluf_bus_plus_skip__alut:
		cpu.alu = alu_74181(SMC(0,0,0,0, 0, emu.skip^1));
		flags = ALUM2 | TSELECT;
		LOG((0,2,"	ALU<- BUS + SKIP (%#o := %#o + %#o)\n",
			cpu.alu, cpu.bus, emu.skip));
#if	ASSERT_ALUF
		if (cpu.alu != ((cpu.bus + emu.skip) & 0177777))
			fatal(1,"aluf 1011");
#endif
		break;
	/**
	 * 14: ALU <- BUS & T
	 * PROM data for S3-0:1011 M:1 C:0
	 * 74181 function F=A&B
	 * T source is ALU
	 */
	case aluf_bus_and_t__alut:
		cpu.alu = alu_74181(SMC(1,0,1,1, 1, 0));
		flags = TSELECT;
		LOG((0,2,"	ALU<- BUS AND T (%#o := %#o & %#o)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != (cpu.bus & cpu.t))
			fatal(1,"aluf 1100");
#endif
		break;
	/**
	 * 15: ALU <- BUS & ~T
	 * PROM data for S3-0:0111 M:1 C:0
	 * 74181 function F=A&~B
	 * T source is BUS
	 */
	case aluf_bus_and_not_t:
		cpu.alu = alu_74181(SMC(0,1,1,1, 1, 0));
		flags = 0;
		LOG((0,2,"	ALU<- BUS AND NOT T (%#o := %#o & ~%#o)\n",
			cpu.alu, cpu.bus, cpu.t));
#if	ASSERT_ALUF
		if (cpu.alu != (cpu.bus & ~cpu.t))
			fatal(1,"aluf 1101");
#endif
		break;
	/**
	 * 16: ALU <- ???
	 * PROM data for S3-0:??? M:? C:?
	 * 74181 perhaps F=0 (0011/0/0)
	 * T source is BUS
	 */
	default:
		cpu.alu = alu_74181(SMC(0,0,1,1, 0, 0));
		flags = ALUM2;
		LOG((0,0,"	ALU<- 0 (illegal aluf in task %s, mpc:%05o aluf:%02o)\n",
			task_name[cpu.task], cpu.mpc, aluf));
#if	ASSERT_ALUF
		if (cpu.alu != 0)
			fatal(1,"aluf 111x");
#endif
	}
	return flags;
}

/**
 * @brief compute the shifter effects
 *
 * Note: The shifter MSB or LSB are modified by the Emualtor F2s
 * MAGIC and DNS. The shifter does not, however, change the ALUC0.
 */
__inline static void alto_shifter(int f1)
{
	switch (f1) {
	case f1_l_lsh_1: /* L LSH 1 */
		cpu.shifter = (cpu.l << 1) & 0177777;
		LOG((0,2,"	SHIFTER <-L LSH 1 (%#o := %#o<<1)\n",
			cpu.shifter, cpu.l));
		break;
	case f1_l_rsh_1: /* L RSH 1 */
		cpu.shifter = cpu.l >> 1;
		LOG((0,2,"	SHIFTER <-L RSH 1 (%#o := %#o>>1)\n",
			cpu.shifter, cpu.l));
		break;
	case f1_l_lcy_8: /* L LCY 8 */
		cpu.shifter = ((cpu.l >> 8) | (cpu.l << 8)) & 0177777;
		LOG((0,2,"	SHIFTER <-L LCY 8 (%#o := bswap %#o)\n",
			cpu.shifter, cpu.l));
		break;
	default:	 /* no shift, just pass */
		cpu.shifter = cpu.l;
		LOG((0,7,"	SHIFTER <-L (%#o)\n",
			cpu.shifter));
		break;
	}
}

/** @brief execute the CPU for at most nsecs nano seconds */
ntime_t alto_execute(ntime_t nsecs)
{
	alto_leave = 0;
	alto_ntime = nsecs;

#if	DEBUG_TIMESLICES
	printf("@%lld: run timeslice for %lldns\n", ntime(), alto_ntime);
#endif

	cpu.mpc = cpu.task_mpc[cpu.task];

	do {
		int do_bs, aluf, bs, f1, f2, next, flags;

		/* nano seconds per cycle */
		alto_ntime -= CPU_MICROCYCLE_TIME;
		cpu.task_ntime[cpu.task] += CPU_MICROCYCLE_TIME;

		cpu.mir	= ucode_raw[cpu.mpc];
		cpu.rsel= MIR_RSEL(cpu.mir);
		aluf	= MIR_ALUF(cpu.mir);
		bs	= MIR_BS(cpu.mir);
		f1	= MIR_F1(cpu.mir);
		f2	= MIR_F2(cpu.mir);
		next	= MIR_NEXT(cpu.mir) | cpu.next2;
		cpu.next2 = MIR_NEXT(ucode_raw[next]);

		cpu_display_state_machine();

		/*
		 * This bus source decoding is not performed if f1 = 7 or f2 = 7.
		 * These functions use the BS field to provide part of the address
		 * to the constant ROM
		 */
		do_bs = !(f1 == f1_const || f2 == f2_const);

		LOG((0,2,"\n%s-%04o: r:%02o af:%02o bs:%02o f1:%02o f2:%02o t:%o l:%o next:%04o cycle:%lld\n",
			task_name[cpu.task], cpu.mpc,
			cpu.rsel, aluf, bs, f1, f2,
			MIR_T(cpu.mir), MIR_L(cpu.mir), next,
			ntime() / CPU_MICROCYCLE_TIME));

		if (do_bs && bs == bs_read_md) {
			while (check_mem_read_stall()) {
				LOG((0,3, "	memory read stall\n"));
				cpu_display_state_machine();
				alto_ntime -= CPU_MICROCYCLE_TIME;
				cpu.task_ntime[cpu.task] += CPU_MICROCYCLE_TIME;
			}
		}

		if (f1 != f1_load_mar && f2 == f2_load_md) {
			while (check_mem_write_stall()) {
				LOG((0,3, "	memory write stall\n"));
				cpu_display_state_machine();
				alto_ntime -= CPU_MICROCYCLE_TIME;
				cpu.task_ntime[cpu.task] += CPU_MICROCYCLE_TIME;
			}
		}

		cpu.bus = 0177777;
		if (cpu.rdram_flag)
			rdram();

		/*
		 * The constant memory is gated to the bus by F1 = 7, F2 = 7, or BS >= 4
		 */
		if (!do_bs || bs >= 4) {
			int addr = 8 * cpu.rsel + bs;
			LOG((0,2,"	%#o; BUS &= CONST[%03o]\n", const_prom[addr], addr));
			cpu.bus &= const_prom[addr];
		}

		/*
		 * early f2 has to be done before early bs, because the
		 * emulator f2 acsource or acdest may change rsel
		 */
		CALL_FN(f2, 0);

		/*
		 * early bs can be done now
		 */
		if (do_bs || cpu.rsel == 037)
			CALL_FN(bs, 0);

		/*
		 * early f1
		 */
		CALL_FN(f1, 0);

		/*
		 * compute the ALU function
		 */
		flags = alto_aluf(aluf);

		/* WRTRAM uses last L register, must do this before updating L */
		if (cpu.wrtram_flag)
			wrtram();

		/*
		 * compute the SHIFTER function
		 */
		alto_shifter(f1);

		/* late bs is done now, if no constant was put on the bus */
		if (do_bs)
			CALL_FN(bs, 1);

		/* late f1 is done now, if any */
		CALL_FN(f1, 1);

		/* late f2 is done now, if any */
		CALL_FN(f2, 1);

		/*
		 * update L register and LALUC0, and also M register, if RAM related task
		 */
		if (MIR_L(cpu.mir)) {
			cpu.l = cpu.alu;
			if (flags & ALUM2) {
				cpu.laluc0 = cpu.aluc0;
				LOG((0,2, "	L<- ALU (%#o); LALUC0<- ALUC0 (%o)\n",
					cpu.alu, cpu.laluc0));
			} else {
				cpu.laluc0 = 0;
				LOG((0,2, "	L<- ALU (%#o); LALUC0<- %o\n",
					cpu.alu, cpu.laluc0));
			}
			if (ram_related[cpu.task]) {
				cpu.s[cpu.s_reg_bank[cpu.task]][0] = cpu.alu;
				LOG((0,2, "	M<- ALU (%#o)\n",
					cpu.alu));
			}
		}

		/*
		 * update T register
		 */
		if (MIR_T(cpu.mir)) {
			cpu.cram_addr = cpu.alu;
			if (flags & TSELECT) {
				LOG((0,2, "	T<- ALU (%#o)\n", cpu.alu));
				cpu.t = cpu.alu;
			} else {
				LOG((0,2, "	T<- BUS (%#o)\n", cpu.bus));
				cpu.t = cpu.bus;
			}
		}

		/* make next instruction's mpc */
		cpu.mpc = (cpu.mpc & ~01777) | next;

		if (cpu.task_switch) {
			/* switch now? */
			if (cpu.task != cpu.next_task) {
				switch_task();
			} else {
				/* one more microinstruction */
				cpu.next_task = cpu.next2_task;
			}
		}
	} while (!alto_leave && alto_ntime > CPU_MICROCYCLE_TIME);

	/* save this task's mpc, next, and next2 */
	cpu.task_mpc[cpu.task] = cpu.mpc;

#if	DEBUG_TIMESLICES
	if (alto_ntime > 0) {
		printf("@%lld: leave timeslice; %lldns (%lldcyc) left; ran %lldns (=%lldcyc)\n",
			ntime(),
			alto_ntime,
			alto_ntime / CPU_MICROCYCLE_TIME,
			nsecs - alto_ntime,
			(nsecs - alto_ntime) / CPU_MICROCYCLE_TIME);
	} else {
		printf("@%lld: completed timeslice %lldns (%lldcyc)\n",
			ntime(),
			nsecs,
			nsecs / CPU_MICROCYCLE_TIME);
	}
#endif

	nsecs -= alto_ntime;
	alto_ntime = 0;

	return nsecs;
}
