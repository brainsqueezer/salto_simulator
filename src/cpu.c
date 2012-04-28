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
 * $Id: cpu.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
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
#include "disk.h"

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
/** @brief set non-zero to enable huge amouts of debug output to analyze CPU time slices */
#define	DEBUG_TIMESLICES	0
#endif

#ifndef	USE_ALU_74181
/** @brief use a full blown ALU 74181 emulation to map the ALU functions */
#define	USE_ALU_74181		1
#endif

#ifndef	USE_PRIO_F9318
/** @brief use task priority decoder with F9318s and 2KCTL u38 PROM (broken) */
#define	USE_PRIO_F9318		0
#endif

/** @brief set non-zero to do extra sanity check on the ALU functions */
#define	ALUF_ASSERTION		0

/** @brief get the normally accessed bank number from a bank register */
#define	GET_BANK_NORMAL(breg)	ALTO_GET(breg,16,12,13)

/** @brief get the extended bank number (accessed via XMAR) from a bank register */
#define	GET_BANK_EXTENDED(breg)	ALTO_GET(breg,16,14,15)

/** @brief get an ignored bit field from a control RAM address */
#define	GET_CRAM_IGNORE(addr)	ALTO_GET(addr,16,0,1)

/** @brief get the bank select bit field from a control RAM address */
#define	GET_CRAM_BANKSEL(addr)	ALTO_GET(addr,16,2,3)

/** @brief get the ROM/RAM flag from a control RAM address */
#define	GET_CRAM_RAMROM(addr)	ALTO_GET(addr,16,4,4)

/** @brief get the half select flag from a control RAM address */
#define	GET_CRAM_HALFSEL(addr)	ALTO_GET(addr,16,5,5)

/** @brief get the word address bit field from a control RAM address */
#define	GET_CRAM_WORDADDR(addr)	ALTO_GET(addr,16,6,15)

/** @brief call a BUS SOURCE, F1, or F2 function */
#define	CALL_FN(type,val,p) do { \
	if (NULL != fn_##type[p][cpu.task][val]) \
		fn_##type[p][cpu.task][val](); \
}	while (0)


/** @brief raw microcode words, decoded */
uint32_t ucode_raw[UCODE_SIZE];

/** @brief constant PROM, decoded */
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

/** @brief task names */
const char *task_name[task_COUNT] = {
	"emu",		"task01",	"task02",	"task03",
	"ksec",		"task05",	"task06",	"ether",
	"mrt",		"dwt",		"curt",		"dht",
	"dvt",		"part",		"kwd",		"task17"
};

/** @brief register names (as used by the microcode) */
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

/** @brief ALU function names */
const char *aluf_name[aluf_COUNT] = {
	"bus",		"t",		"bus or t",	"bus and t",
	"bus xor t",	"bus + 1",	"bus - 1",	"bus + t",
	"bus - t",	"bus - t - 1",	"bus + t + 1",	"bus + skip",
	"bus,t",	"bus and not t","0 (undef)",	"0 (undef)"
};

/** @brief BUS source names */
const char *bs_name[bs_COUNT] = {
	"read_r",	"load_r",	"no_source",	"task_3",
	"task_4",	"read_md",	"mouse",	"disp"
};

/** @brief F1 function names */
const char *f1_name[f1_COUNT] = {
	"nop",		"load_mar",	"task",		"block",
	"l_lsh_1",	"l_rsh_1",	"l_lcy_8",	"const",
	"task_10",	"task_11",	"task_12",	"task_13",
	"task_14",	"task_15",	"task_16",	"task_17"
};

/** @brief F2 function names */
const char *f2_name[f2_COUNT] = {
	"nop",		"bus=0",	"shifter<0",	"shifter=0",
	"bus",		"alucy",	"load_md",	"const",
	"task_10",	"task_11",	"task_12",	"task_13",
	"task_14",	"task_15",	"task_16",	"task_17"
};


/**
 * @brief 2KCTL PROM u3 - 256x4
 * <PRE>
 * PROM u3 is 256x4 type 3601-1, looks like SN74387, and it
 * controls NEXT[6-9]', i.e. the outputs are wire-AND to NEXT
 *
 *           SN74387
 *         +---+-+---+
 *         |   +-+   |
 *    A6  -|1      16|-  Vcc
 *         |         |
 *    A5  -|2      15|-  A7
 *         |         |
 *    A4  -|3      14|-  FE1'
 *         |         |
 *    A3  -|4      13|-  FE2'
 *         |         |
 *    A0  -|5      12|-  D0
 *         |         |
 *    A1  -|6      11|-  D1
 *         |         |
 *    A2  -|7      10|-  D2
 *         |         |
 *   GND  -|8       9|-  D3
 *         |         |
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
 * </PRE>
 */
uint8_t ctl2k_u3[256];

/**
 * @brief 2KCTL PROM u38; 82S23; 32x8 bit
 * <PRE>
 *
 *            82S23
 *         +---+-+---+
 *         |   +-+   |
 *    B0  -|1      16|-  Vcc
 *         |         |
 *    B1  -|2      15|-  EN'
 *         |         |
 *    B2  -|3      14|-  A4
 *         |         |
 *    B3  -|4      13|-  A3
 *         |         |
 *    B4  -|5      12|-  A2
 *         |         |
 *    B5  -|6      11|-  A1
 *         |         |
 *    B6  -|7      10|-  A0
 *         |         |
 *   GND  -|8       9|-  B7
 *         |         |
 *         +---------+
 *
 * Task priority encoder
 *
 * 	line	pin	signal
 *	-------------------------------
 *	A0	10	CT1 (current task LSB)
 *	A1	11	CT2
 *	A2	12	CT4
 *	A3	13	CT8 (current task MSB)
 *	A4	14	0 (GND)
 *
 *	line	pin	signal
 *	-------------------------------
 *	B0	1	RDCT8'
 *	B1	2	RDCT4'
 *	B2	3	RDCT2'
 *	B3	4	RDCT1'
 *	B4	5	NEXT[09]'
 *	B5	6	NEXT[08]'
 *	B6	7	NEXT[07]'
 *	B7	9	NEXT[06]'
 *
 * This dump is from PROM 2kctl.u38:
 * 0000: 0367,0353,0323,0315,0265,0251,0221,0216,
 * 0010: 0166,0152,0122,0114,0064,0050,0020,0017,
 * 0020: 0000,0000,0000,0000,0000,0000,0000,0000,
 * 0030: 0000,0000,0000,0000,0000,0000,0000,0000
 * </PRE>
 */
uint8_t ctl2k_u38[32];

/** @brief output lines of the 2KCTL U38 PROM */
typedef enum {
	U38_RDCT8,
	U38_RDCT4,
	U38_RDCT2,
	U38_RDCT1,
	U38_NEXT09,
	U38_NEXT08,
	U38_NEXT07,
	U38_NEXT06
}	u38_out_t;

/**
 * @brief 2KCTL PROM u76; P3601-1; 256x4; PC0I and PC1I decoding
 * <PRE>
 * Replacement for u51, which is used in 1KCTL
 *
 *           SN74387
 *         +---+-+---+
 *         |   +-+   |
 *    A6  -|1      16|-  Vcc
 *         |         |
 *    A5  -|2      15|-  A7
 *         |         |
 *    A4  -|3      14|-  FE1'
 *         |         |
 *    A3  -|4      13|-  FE2'
 *         |         |
 *    A0  -|5      12|-  D0
 *         |         |
 *    A1  -|6      11|-  D1
 *         |         |
 *    A2  -|7      10|-  D2
 *         |         |
 *   GND  -|8       9|-  D3
 *         |         |
 *         +---------+
 *
 *	input line	signal
 *	----------------------------
 *	A7 15		EMACT'
 *	A6  1		F1(0)
 *	A5  2		F1(1)'
 *	A4  3		F1(2)'
 *	A3  4		F1(3)'
 *	A2  7		0 (GND)
 *	A1  6		PC1O
 *	A0  6		PC0O
 *
 *	output line	signal
 *	----------------------------
 *	D0 12		PC1T
 *	D1 11		PC1F
 *	D2 10		PC0T
 *	D3  9		PC0F
 *
 * The outputs are connected to a dual 4:1 demultiplexer 74S153, so that
 * depending on NEXT01' and RESET the following signals are passed through:
 *
 *	RESET	NEXT[01]'	PC0I	PC1I
 *	--------------------------------------
 *	0	0		PC0T	PC1T
 *	0	1		PC0F	PC1F
 *	1	0		PC0I4	T14 (?)
 *	1	1		-"-	-"-
 *
 * This selects the microcode "page" to jump to on SWMODE (F1 = 010)
 * depending on the current NEXT[01]' level.
 *
 *
 * This dump is from PROM 2kctl.u76:
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
 * </PRE>
 */
uint8_t ctl2k_u76[256];


/**
 * @brief 3k CRAM PROM a37
 * <PRE>
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
 * </PRE>
 */
uint8_t cram3k_a37[256];

/**
 * @brief memory addressing PROM a64
 * <PRE>
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
 * </PRE>
 */
uint8_t madr_a64[256];

/**
 * @brief memory addressing PROM a65
 * <PRE>
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
 * </PRE>
 */
uint8_t madr_a65[256];

/** @brief fatal exit on unitialized dynamic phase BUS source */
static void fn_bs_bad_0(void)
{
	fatal(9,"fatal: bad early bus source pointer for task %s, mpc:%05o bs:%s\n",
		task_name[cpu.task], cpu.mpc, bs_name[MIR_BS]);
}

/** @brief fatal exit on unitialized latching phase BUS source */
static void fn_bs_bad_1(void)
{
	fatal(9,"fatal: bad late bus source pointer for task %s, mpc:%05o bs: %s\n",
		task_name[cpu.task], cpu.mpc, bs_name[MIR_BS]);
}

/** @brief fatal exit on unitialized dynamic phase F1 function */
static void fn_f1_bad_0(void)
{
	fatal(9,"fatal: bad early f1 function pointer for task %s, mpc:%05o f1: %s\n",
		task_name[cpu.task], cpu.mpc, f1_name[MIR_F1]);
}

/** @brief fatal exit on unitialized latching phase F1 function */
static void fn_f1_bad_1(void)
{
	fatal(9,"fatal: bad late f1 function pointer for task %s, mpc:%05o f1: %s\n",
		task_name[cpu.task], cpu.mpc, f1_name[MIR_F1]);
}

/** @brief fatal exit on unitialized dynamic phase F2 function */
static void fn_f2_bad_0(void)
{
	fatal(9,"fatal: bad early f2 function pointer for task %s, mpc:%05o f2: %s\n",
		task_name[cpu.task], cpu.mpc, f2_name[MIR_F2]);
}

/** @brief fatal exit on unitialized latching phase F2 function */
static void fn_f2_bad_1(void)
{
	fatal(9,"fatal: bad late f2 function pointer for task %s, mpc:%05o f2: %s\n",
		task_name[cpu.task], cpu.mpc, f2_name[MIR_F2]);
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
	if (MIR_F2 != f2_emu_load_dns) {
		cpu.r[cpu.rsel] = cpu.shifter;
		LOG((0,2,"	R%02o<-; %s = SHIFTER (%#o)\n",
			cpu.rsel, r_name[cpu.rsel],
			cpu.shifter));
		/* HACK: programs writing r37 with xxx3 make the cursor
		 * display go nuts. Until I found the real reason for this
		 * obviously buggy display, I just clear the two
		 * least significant bits of r37 if they are set at once.
		 */
		if (cpu.rsel == 037 && ((cpu.shifter & 3) == 3)) {
			printf("writing r37 = %#o\n", cpu.shifter);
			cpu.r[037] &= ~3;
		}
	}
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
	LOG((0,0,"BS <-DISP not handled by task %s mpc:%04x\n",
		task_name[cpu.task], cpu.mpc));
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
	if (MIR_F2 == f2_load_md) {
		msb = GET_BANK_EXTENDED(bank) << 16;
		LOG((0,7, "	XMAR %#o\n", msb | cpu.alu));
	} else {
		msb = GET_BANK_NORMAL(bank) << 16;

	}
	load_mar(cpu.rsel, msb | cpu.alu);
}

#if	USE_PRIO_F9318
/** @brief F9318 input lines */
typedef enum {
	PRIO_IN_EI = (1<<8),
	PRIO_IN_I7 = (1<<7),
	PRIO_IN_I6 = (1<<6),
	PRIO_IN_I5 = (1<<5),
	PRIO_IN_I4 = (1<<4),
	PRIO_IN_I3 = (1<<3),
	PRIO_IN_I2 = (1<<2),
	PRIO_IN_I1 = (1<<1),
	PRIO_IN_I0 = (1<<0),
	/* masks */
	PRIO_I7 = PRIO_IN_I7,
	PRIO_I6_I7 = (PRIO_IN_I6 | PRIO_I7),
	PRIO_I5_I7 = (PRIO_IN_I5 | PRIO_I6_I7),
	PRIO_I4_I7 = (PRIO_IN_I4 | PRIO_I5_I7),
	PRIO_I3_I7 = (PRIO_IN_I3 | PRIO_I4_I7),
	PRIO_I2_I7 = (PRIO_IN_I2 | PRIO_I3_I7),
	PRIO_I1_I7 = (PRIO_IN_I1 | PRIO_I2_I7),
	PRIO_I0_I7 = (PRIO_IN_I0 | PRIO_I1_I7),
}	f9318_in_t;

/** @brief F9318 output lines */
typedef enum {
	PRIO_OUT_Q0 = (1<<0),
	PRIO_OUT_Q1 = (1<<1),
	PRIO_OUT_Q2 = (1<<2),
	PRIO_OUT_EO = (1<<3),
	PRIO_OUT_GS = (1<<4),
	/* masks */
	PRIO_OUT_QZ = (PRIO_OUT_Q0 | PRIO_OUT_Q1 | PRIO_OUT_Q2)
}	f9318_out_t;

/**
 * @brief F9318 priority encoder 8 to 3-bit
 *
 * Emulation of the F9318 chip (pin compatible with 74348).
 *
 * <PRE>
 *            F9318
 *         +---+-+---+	
 *         |   +-+   |         +---------------------------------+----------------+
 *    I4' -|1      16|-  Vcc   |              input              |     output     |
 *         |         |         +---------------------------------+----------------+
 *    I5' -|2      15|-  EO'   |      EI I0 I1 I2 I3 I4 I5 I6 I7 | GS Q0 Q1 Q2 EO |
 *         |         |         +---------------------------------+----------------+
 *    I6' -|3      14|-  GS'   | (a)  H  x  x  x  x  x  x  x  x  | H  H  H  H  H  |
 *         |         |         | (b)  L  H  H  H  H  H  H  H  H  | H  H  H  H  L  |
 *    I7' -|4      13|-  I3'   +---------------------------------+----------------+
 *         |         |         | (c)  L  x  x  x  x  x  x  x  L  | L  L  L  L  H  |
 *    EI' -|5      12|-  I2'   | (d)  L  x  x  x  x  x  x  L  H  | L  H  L  L  H  |
 *         |         |         | (e)  L  x  x  x  x  x  L  H  H  | L  L  H  L  H  |
 *    Q2' -|6      11|-  I1'   | (f)  L  x  x  x  x  L  H  H  H  | L  H  H  L  H  |
 *         |         |         | (g)  L  x  x  x  L  H  H  H  H  | L  L  L  H  H  |
 *    Q1' -|7      10|-  I0'   | (h)  L  x  x  L  H  H  H  H  H  | L  H  L  H  H  |
 *         |         |         | (i)  L  x  L  H  H  H  H  H  H  | L  L  H  H  H  |
 *   GND  -|8       9|-  Q0'   | (j)  L  L  H  H  H  H  H  H  H  | L  H  H  H  H  |
 *         |         |         +---------------------------------+----------------+
 *         +---------+
 * </PRE>
 */
static __inline f9318_out_t f9318(f9318_in_t in)
{
	f9318_out_t out;

	if (in & PRIO_IN_EI) {
		out = PRIO_OUT_EO | PRIO_OUT_GS | PRIO_OUT_QZ;
		LOG((0,2,"	f9318 case (a) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (0 == (in & PRIO_I7)) {
		out = PRIO_OUT_EO;
		LOG((0,2,"	f9318 case (c) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (PRIO_I7 == (in & PRIO_I6_I7)) {
		out = PRIO_OUT_EO | PRIO_OUT_Q0;
		LOG((0,2,"	f9318 case (d) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (PRIO_I6_I7 == (in & PRIO_I5_I7)) {
		out = PRIO_OUT_EO | PRIO_OUT_Q1;
		LOG((0,2,"	f9318 case (e) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (PRIO_I5_I7 == (in & PRIO_I4_I7)) {
		out = PRIO_OUT_EO | PRIO_OUT_Q0 | PRIO_OUT_Q1;
		LOG((0,2,"	f9318 case (f) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (PRIO_I4_I7 == (in & PRIO_I3_I7)) {
		out = PRIO_OUT_EO | PRIO_OUT_Q2;
		LOG((0,2,"	f9318 case (g) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (PRIO_I3_I7 == (in & PRIO_I2_I7)) {
		out = PRIO_OUT_EO | PRIO_OUT_Q0 | PRIO_OUT_Q2;
		LOG((0,2,"	f9318 case (h) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (PRIO_I2_I7 == (in & PRIO_I1_I7)) {
		out = PRIO_OUT_EO | PRIO_OUT_Q1 | PRIO_OUT_Q2;
		LOG((0,2,"	f9318 case (i) in:%#o out:%#o\n", in, out));
		return out;
	}

	if (PRIO_I1_I7 == (in & PRIO_I0_I7)) {
		out = PRIO_OUT_EO | PRIO_OUT_Q0 | PRIO_OUT_Q1 | PRIO_OUT_Q2;
		LOG((0,2,"	f9318 case (j) in:%#o out:%#o\n", in, out));
		return out;
	}

	out = PRIO_OUT_QZ | PRIO_OUT_GS;
	LOG((0,2,"	f9318 case (b) in:%#o out:%#o\n", in, out));
	return out;
}
#endif

/**
 * @brief f1_task early: task switch
 *
 * The priority encoder finds the highest task requesting service
 * and switches the task number after the next cycle.
 *
 * <PRE>
 *	CT       PROM    NEXT'     RDCT'
 *	1 2 4 8  DATA   6 7 8 9   1 2 4 8
 *	---------------------------------
 *	0 0 0 0  0367   1 1 1 1   0 1 1 1
 *	1 0 0 0  0353   1 1 1 0   1 0 1 1
 *	0 1 0 0  0323   1 1 0 1   0 0 1 1
 *	1 1 0 0  0315   1 1 0 0   1 1 0 1
 *	0 0 1 0  0265   1 0 1 1   0 1 0 1
 *	1 0 1 0  0251   1 0 1 0   1 0 0 1
 *	0 1 1 0  0221   1 0 0 1   0 0 0 1
 *	1 1 1 0  0216   1 0 0 0   1 1 1 0
 *	0 0 0 1  0166   0 1 1 1   0 1 1 0
 *	1 0 0 1  0152   0 1 1 0   1 0 1 0
 *	0 1 0 1  0122   0 1 0 1   0 0 1 0
 *	1 1 0 1  0114   0 1 0 0   1 1 0 0
 *	0 0 1 1  0064   0 0 1 1   0 1 0 0
 *	1 0 1 1  0050   0 0 1 0   1 0 0 0
 *	0 1 1 1  0020   0 0 0 1   0 0 0 0
 *	1 1 1 1  0017   0 0 0 0   1 1 1 1
 *
 * The various task wakeups are encoded using two 8:3-bit priority encoders F9318,
 * which are pin-compatible to the 74348 (inverted inputs and outputs).
 * Their part numbers are U1 and U2.
 * The two encoders are chained (EO of U1 goes to EI of U2):
 *
 * The outputs are fed into some NAND gates (74H10 and 74H00) to decode
 * the task number to latch (CT1-CT4) after a F1 TASK. The case where all
 * of RDCT1' to RDCT8' are high (1) is decoded as RESET'.
 *
 * signal	function
 * --------------------------------------------------
 * CT1		(U1.Q0' & U2.Q0' & RDCT1')'
 * CT2		(U1.Q1' & U2.Q1' & RDCT2')'
 * CT4		(U1.Q2' & U2.Q2' & RDCT4')'
 * CT8		(U1.GS' & RDCT8')'
 * RESET'	RDCT1' & RDCT2' & RDCT4' & RDCT8'
 *
 * In the tables below "x" is RDCTx' of current task
 *
 * signal          input   output, if first 0        CT1  CT2  CT4  CT8
 * ----------------------------------------------------------------------------------------
 * WAKE17' (T19?)   4 I7   Q2:0 Q1:0 Q0:0 GS:0 EO:1  1    1    1    1
 * WAKEKWDT'        3 I6   Q2:0 Q1:0 Q0:1 GS:0 EO:1  x    1    1    1
 * WAKEPART'        2 I5   Q2:0 Q1:1 Q0:0 GS:0 EO:1  1    x    1    1
 * WAKEDVT'         1 I4   Q2:0 Q1:1 Q0:1 GS:0 EO:1  x    x    1    1
 * WAKEDHT'        13 I3   Q2:1 Q1:0 Q0:0 GS:0 EO:1  1    1    x    1
 * WAKECURT'       12 I2   Q2:1 Q1:0 Q0:1 GS:0 EO:1  x    1    x    1
 * WAKEDWT'        11 I1   Q2:1 Q1:1 Q0:0 GS:0 EO:1  1    x    x    1
 * WAKEMRT'        10 I0   Q2:1 Q1:1 Q0:1 GS:0 EO:1  x    x    x    1
 * otherwise               Q2:1 Q1:1 Q0:1 GS:1 EO:0  x    x    x    x
 *
 * signal          input   output, if first 0
 * ----------------------------------------------------------------------------------------
 * WAKEET'          4 I7   Q2:0 Q1:0 Q0:0 GS:0 EO:1  1    1    1    x
 * WAKE6'           3 I6   Q2:0 Q1:0 Q0:1 GS:0 EO:1  x    1    1    x
 * WAKE5'           2 I5   Q2:0 Q1:1 Q0:0 GS:0 EO:1  1    x    1    x
 * WAKEKST'         1 I4   Q2:0 Q1:1 Q0:1 GS:0 EO:1  x    x    1    x
 * WAKE3' (T23?)   13 I3   Q2:1 Q1:0 Q0:0 GS:0 EO:1  1    1    x    x
 * WAKE2'          12 I2   Q2:1 Q1:0 Q0:1 GS:0 EO:1  x    1    x    x
 * WAKE1'          11 I1   Q2:1 Q1:1 Q0:0 GS:0 EO:1  1    x    x    x
 * 0 (GND)         10 I0   Q2:1 Q1:1 Q0:1 GS:0 EO:1  x    x    x    x
 * </PRE>
 */
static void f1_task_0(void)
{
#if	USE_PRIO_F9318
	/* Doesn't work yet */
	register f9318_in_t wakeup_hi;
	register f9318_out_t u1;
	register f9318_in_t wakeup_lo;
	register f9318_out_t u2;
	register int addr = 017;
	register int rdct1, rdct2, rdct4, rdct8;
	register int ct1, ct2, ct4, ct8;
	register int wakeup, ct;

	LOG((0,2, "	TASK %02o:%s\n", cpu.task, task_name[cpu.task]));

	if (cpu.task > task_emu && CPU_GET_TASK_WAKEUP(cpu.task))
		addr = cpu.task;
	LOG((0,2,"	ctl2k_u38[%02o] = %04o\n", addr, ctl2k_u38[addr] & 017));

	rdct1 = (ctl2k_u38[addr] >> U38_RDCT1) & 1;
	rdct2 = (ctl2k_u38[addr] >> U38_RDCT2) & 1;
	rdct4 = (ctl2k_u38[addr] >> U38_RDCT4) & 1;
	rdct8 = (ctl2k_u38[addr] >> U38_RDCT8) & 1;

	/* wakeup signals are active low */
	wakeup = ~cpu.task_wakeup;

	/* U1
	 * task wakeups 017 to 010 on I7 to I0
	 * EI is 0 (would be 1 at reset)
	 */
	wakeup_hi = (wakeup >> 8) & PRIO_I0_I7;
	u1 = f9318(wakeup_hi);

	/* U2
	 * task wakeups 007 to 001 on I7 to I1, I0 is 0
	 * EO of U1 chained to EI
	 */
	wakeup_lo = wakeup & PRIO_I0_I7;
	if (u1 & PRIO_OUT_EO)
		wakeup_lo |= PRIO_IN_EI;
	u2 = f9318(wakeup_lo);

	/* CT1 = (U1.Q0' & U2.Q0' & RDCT1')' */
	ct1 = !((u1 & PRIO_OUT_Q0) && (u2 & PRIO_OUT_Q0) && rdct1);
	LOG((0,2,"	  CT1:%o U1.Q0':%o U2.Q0':%o RDCT1':%o\n",
		ct1, (u1 & PRIO_OUT_Q0)?1:0, (u2 & PRIO_OUT_Q0)?1:0, rdct1));
	/* CT2 = (U1.Q1' & U2.Q1' & RDCT2')' */
	ct2 = !((u1 & PRIO_OUT_Q1) && (u2 & PRIO_OUT_Q1) && rdct2);
	LOG((0,2,"	  CT2:%o U1.Q1':%o U2.Q1':%o RDCT2':%o\n",
		ct2, (u1 & PRIO_OUT_Q1)?1:0, (u2 & PRIO_OUT_Q1)?1:0, rdct2));
	/* CT4 = (U1.Q2' & U2.Q2' & RDCT4')' */
	ct4 = !((u1 & PRIO_OUT_Q2) && (u2 & PRIO_OUT_Q2) && rdct4);
	LOG((0,2,"	  CT4:%o U1.Q2':%o U2.Q2':%o RDCT4':%o\n",
		ct4, (u1 & PRIO_OUT_Q2)?1:0, (u2 & PRIO_OUT_Q2)?1:0, rdct4));
	/* CT8 */
	ct8 = !((u1 & PRIO_OUT_GS) && rdct8);
	LOG((0,2,"	  CT8:%o U1.GS':%o RDCT8':%o\n",
		ct8, (u1 & PRIO_OUT_GS)?1:0, rdct8));

	ct = 8*ct8 + 4*ct4 + 2*ct2 + ct1;

	if (ct != cpu.next_task) {
		LOG((0,2, "		switch to %02o\n", ct));
		cpu.next2_task = ct;
	} else {
		LOG((0,2, "		no switch\n"));
	}	
#else	/* USE_PRIO_F9318 */
	int i;

	LOG((0,2, "	TASK %02o:%s", cpu.task, task_name[cpu.task]));
	for (i = 15; i >= 0; i--) {
		if (CPU_GET_TASK_WAKEUP(i)) {
			cpu.next2_task = i;
			if (cpu.next2_task != cpu.next_task) {
				LOG((0,2, " switch to %02o:%s\n",
					cpu.next2_task, task_name[cpu.next2_task]));
			} else {
				LOG((0,2, " no switch\n"));
			}
			return;
		}
	}
	fatal(3, "no tasks requesting service\n");
#endif	/* !USE_PRIO_F9318 */
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
 * @brief f2_bus_eq_zero late: branch on bus equals zero
 */
static void f2_bus_eq_zero_1(void)
{
	int or = cpu.bus == 0 ? 1 : 0;
	LOG((0,2, "	BUS=0; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_shifter_lt_zero late: branch on shifter less than zero
 */
static void f2_shifter_lt_zero_1(void)
{
	int or = (cpu.shifter & 0100000) ? 1 : 0;
	LOG((0,2, "	SH<0; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_shifter_eq_zero late: branch on shifter equals zero
 */
static void f2_shifter_eq_zero_1(void)
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
 * @brief f2_alucy late: branch on latched ALU carry
 */
static void f2_alucy_1(void)
{
	int or = cpu.laluc0;
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
	if (MIR_F1 == f1_load_mar) {
		/* part of an XMAR */
		LOG((0,2, "	XMAR %#o (%#o)\n", mar, cpu.bus));
	} else {
		write_mem(cpu.bus);
		LOG((0,2, "	MD<- BUS ([%#o]=%#o)\n", mar, cpu.bus));
	}
}

/**
 * @brief read the microcode ROM/RAM halfword
 *
 * Note: HALFSEL is selecting the even (0) or odd (1) half of the
 * microcode RAM 32-bit word. Here's how the demultiplexers (74298)
 * u8, u18, u28 and u38 select the bits:
 *
 *           SN74298
 *         +---+-+---+
 *         |   +-+   |
 *    B2  -|1      16|-  Vcc
 *         |         |
 *    A2  -|2      15|-  QA
 *         |         |
 *    A1  -|3      14|-  QB
 *         |         |
 *    B1  -|4      13|-  QC
 *         |         |
 *    C2  -|5      12|-  QD
 *         |         |
 *    D2  -|6      11|-  CLK
 *         |         |
 *    D1  -|7      10|-  SEL
 *         |         |
 *   GND  -|8       9|-  C1
 *         |         |
 *         +---------+
 *
 *	chip	out pin	BUS	in pin HSEL=0		in pin HSEL=1
 *	--------------------------------------------------------------
 *	u8	QA  15	0	A1   3 DRSEL(0)'	A2   2 DF2(0)
 *	u8	QB  14	1	B1   4 DRSEL(1)'	B2   1 DF2(1)'
 *	u8	QC  13	2	C1   9 DRSEL(2)'	C2   5 DF2(2)'
 *	u8	QD  12	3	D1   7 DRSEL(3)'	D2   6 DF2(3)'
 *
 *	u18	QA  15	4	A1   3 DRSEL(4)'	A2   2 LOADT'
 *	u18	QB  14	5	B1   4 DALUF(0)'	B2   1 LOADL
 *	u18	QC  13	6	C1   9 DALUF(1)'	C2   5 NEXT(00)'
 *	u18	QD  12	7	D1   7 DALUF(2)'	D2   6 NEXT(01)'
 *
 *	u28	QA  15	8	A1   3 DALUF(3)'	A2   2 NEXT(02)'
 *	u28	QB  14	9	B1   4 DBS(0)'		B2   1 NEXT(03)'
 *	u28	QC  13	10	C1   9 DBS(1)'		C2   5 NEXT(04)'
 *	u28	QD  12	11	D1   7 DBS(2)'		D2   6 NEXT(05)'
 *
 *	u38	QA  15	12	A1   3 DF1(0)		A2   2 NEXT(06)'
 *	u38	QB  14	13	B1   4 DF1(1)'		B2   1 NEXT(07)'
 *	u38	QC  13	14	C1   9 DF1(2)'		C2   5 NEXT(08)'
 *	u38	QD  12	15	D1   7 DF1(3)'		D2   6 NEXT(09)'
 *
 * The HALFSEL signal to the demultiplexers is the inverted bit BUS(5):
 * BUS(5)=1, HALFSEL=0, A1,B1,C1,D1 inputs, upper half of the 32-bit word
 * BUS(5)=0, HALFSEL=1, A2,B2,C2,D2 inputs, lower half of the 32-bit word
 */
static void rdram(void)
{
	uint32_t addr, val;
	uint32_t bank = GET_CRAM_BANKSEL(cpu.cram_addr) % UCODE_RAM_PAGES;
	uint32_t wordaddr = GET_CRAM_WORDADDR(cpu.cram_addr);

	cpu.rdram_flag = 0;
	if (GET_CRAM_RAMROM(cpu.cram_addr)) {
		/* read ROM 0 at current mpc */
		addr = cpu.mpc & 01777;
		LOG((0,0,"	rdram: ROM [%05o] ", addr));
	} else {
		/* read RAM 0,1,2 */
		addr = UCODE_RAM_BASE + bank * UCODE_PAGE_SIZE + wordaddr;
			;
		LOG((0,0,"	rdram: RAM%d [%04o] ",
			bank, wordaddr));
	}

	if (addr >= UCODE_SIZE) {
		val = 0177777;	/* ??? */
		LOG((0,0,"invalid address (%06o)\n", val));
		return;
	}
	val = ucode_raw[addr] ^ UCODE_INVERTED;
	if (GET_CRAM_HALFSEL(cpu.cram_addr)) {
		val = val >> 16;
		LOG((0,0,"upper:%06o\n", val));
	} else {
		val = val & 0177777;
		LOG((0,0,"lower:%06o\n", val));
	}
	cpu.bus &= val;
}

/**
 * @brief write the microcode RAM from M register and ALU
 *
 * Note: M is a latch (MYL, i.e. memory L) on the CRAM board that latches
 * the ALU whenever LOADL and GOODTASK are met. GOODTASK is the Emulator
 * task and something I have not yet found out about: TASKA' and TASKB'.
 *
 * There's also an undumped PROM u21 which is addressed by GOODTASK and
 * 7 other signals...
 */
static void wrtram(void)
{
	uint32_t addr;
	uint32_t bank = GET_CRAM_BANKSEL(cpu.cram_addr) % UCODE_RAM_PAGES;
	uint32_t wordaddr = GET_CRAM_WORDADDR(cpu.cram_addr);

	cpu.wrtram_flag = 0;

	/* write RAM 0,1,2 */
	addr = UCODE_RAM_BASE + bank * UCODE_PAGE_SIZE + wordaddr;
	LOG((0,0,"	wrtram: RAM%d [%04o] upper:%06o lower:%06o",
		bank, wordaddr, cpu.m, cpu.alu));
	if (addr >= UCODE_SIZE) {
		LOG((0,0," invalid address\n"));
		return;
	}
	LOG((0,0,"\n"));
	ucode_raw[addr] = ((cpu.m << 16) | cpu.alu) ^ UCODE_INVERTED;
}

static __inline void cpu_display_state_machine(void)
{
	/*
	 * Subtract the microcycle time from the display time accu.
	 * If it underflows call the display state machine and add
	 * the time for 24 pixel clocks to the accu.
	 * This is very close to every seventh CPU cycle.
	 */
	cpu.dsp_time -= CPU_MICROCYCLE_TIME;
	if (cpu.dsp_time < 0) {
		cpu.dsp_state = display_state_machine(cpu.dsp_state);
		cpu.dsp_time += DISPLAY_BITTIME(24);
	}
	if (cpu.unload_time >= 0) {
		/*
		 * Subtract the microcycle time from the unload time accu.
		 * If it underflows call the unload word function which adds
		 * the time for 16 or 32 pixel clocks to the accu, or ends
		 * the unloading by leaving unload_time at -1.
		 */
		cpu.unload_time -= CPU_MICROCYCLE_TIME;
		if (cpu.unload_time < 0) {
			cpu.unload_word = unload_word(cpu.unload_word);
		}
	}
}

#if	USE_ALU_74181
/**
 * <PRE>
 * Functional description of the 4-bit ALU 74181
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
 * |    Mode Select    |   Logic     | Arithmetic w/o carry   | Arithmetic w/ carry    |
 * |      Inputs       |             |                        |                        |
 * |  S3  S2  S1  S0   |   (M=1)     | (M=0) (Cn=1)           | (M=0) (Cn=0)           |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   0   0   | A'          | A                      | A + 1                  |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   0   1   | A' | B'     | A | B                  | (A | B) + 1            |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   1   0   | A' & B      | A | B'                 | (A | B') + 1           |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   0   1   1   | logic 0     | - 1                    | -1 + 1                 |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   0   0   | (A & B)'    | A + (A & B')           | A + (A & B') + 1       |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   0   1   | B'          | (A | B) + (A & B')     | (A | B) + (A & B') + 1 |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   1   0   | A ^ B       | A - B - 1              | A - B - 1 + 1          |
 * +-------------------+-------------+------------------------+------------------------+
 * |   0   1   1   1   | A & B'      | (A & B) - 1            | (A & B) - 1 + 1        |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   0   0   | A' | B      | A + (A & B)            | A + (A & B) + 1        |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   0   1   | A' ^ B'     | A + B                  | A + B + 1              |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   1   0   | B           | (A | B') + (A & B)     | (A | B') + (A & B) + 1 |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   0   1   1   | A & B       | (A & B) - 1            | (A & B) - 1 + 1        |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   0   0   | logic 1     | A + A                  | A + A + 1              |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   0   1   | A | B'      | (A | B) + A            | (A | B) + A + 1        |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   1   0   | A | B       | (A | B') + A           | (A | B') + A + 1       |
 * +-------------------+-------------+------------------------+------------------------+
 * |   1   1   1   1   | A           | A - 1                  | A - 1 + 1              |
 * +-------------------+-------------+------------------------+------------------------+
 * </PRE>
 */
#define	SMC(s3,s2,s1,s0,m,c) (32*(s3)+16*(s2)+8*(s1)+4*(s0)+2*(m)+(c))

static __inline int alu_74181(int smc)
{
	register uint32_t a = cpu.bus;
	register uint32_t b = cpu.t;
	register uint32_t c = ~smc & 1;
	register uint32_t s = 0;
	register uint32_t f = 0;

	if (smc & 2) {
		/* Mode Select 1: Logic */
		switch (smc / 4) {
		case  0:	/* 0000: A'       */
			f = ~a;
			break;
		case  1:	/* 0001: A' | B'  */
			f = ~a | ~b;
			break;
		case  2:	/* 0010: A' & B   */
			f = ~a & b;
			break;
		case  3:	/* 0011: logic 0  */
			f = 0;
			break;
		case  4:	/* 0100: (A & B)' */
			f = ~(a & b);
			break;
		case  5:	/* 0101: B'       */
			f = ~b;
			break;
		case  6:	/* 0110: A ^ B    */
			f = a ^ b;
			break;
		case  7:	/* 0111: A & B'   */
			f = a & ~b;
			break;
		case  8:	/* 1000: A' | B   */
			f = ~a | b;
			break;
		case  9:	/* 1001: A' ^ B'  */
			f = ~a ^ ~b;
			break;
		case 10:	/* 1010: B        */
			f = b;
			break;
		case 11:	/* 1011: A & B    */
			f = a & b;
			break;
		case 12:	/* 1100: logic 1  */
			f = ~0;
			break;
		case 13:	/* 1101: A | B'   */
			f = a | ~b;
			break;
		case 14:	/* 1110: A | B    */
			f = a | b;
			break;
		case 15:	/* 1111: A        */
			f = a;
			break;
		}
		cpu.aluc0 = 1;
		return f;
	}

	/* Mode Select 0: Arithmetic */
	switch (smc / 4) {
	case  0:	/* 0000: A                      */
		s = 0;
		f = a + c;
		break;
	case  1:	/* 0001: A | B                  */
		s = 0;
		f = (a | b) + c;
		break;
	case  2:	/* 0010: A | B'                 */
		s = 0;
		f = (a | ~b) + c;
		break;
	case  3:	/* 0011: -1                     */
		s = 1;
		f = -1 + c;
		break;
	case  4:	/* 0100: A + (A & B')           */
		s = 0;
		f = a + (a & ~b) + c;
		break;
	case  5:	/* 0101: (A | B) + (A & B')     */
		s = 0;
		f = (a | b) + (a & ~b) + c;
		break;
	case  6:	/* 0110: A - B - 1              */
		s = 1;
		f = a - b - 1 + c;
		break;
	case  7:	/* 0111: (A & B) - 1            */
		s = 1;
		f = (a & b) - 1 + c;
		break;
	case  8:	/* 1000: A + (A & B)            */
		s = 0;
		f = a + (a & b) + c;
		break;
	case  9:	/* 1001: A + B                  */
		s = 0;
		f = a + b + c;
		break;
	case 10:	/* 1010: (A | B') + (A & B)     */
		s = 0;
		f = (a | ~b) + (a & b) + c;
		break;
	case 11:	/* 1011: (A & B) - 1            */
		s = 1;
		f = (a & b) - 1 + c;
		break;
	case 12:	/* 1100: A + A                  */
		s = 0;
		f = a + a + c;
		break;
	case 13:	/* 1101: (A | B) + A            */
		s = 0;
		f = (a | b) + a + c;
		break;
	case 14:	/* 1110: (A | B') + A           */
		s = 0;
		f = (a | ~b) + a + c;
		break;
	case 15:	/* 1111: A - 1                  */
		s = 1;
		f = a - 1 + c;
		break;
	}
	cpu.aluc0 = ((f >> 16) ^ s) & 1;
	return f & 0177777;
}
#endif

/** @brief flag that tells whether to load the T register from BUS or ALU */
#define	TSELECT	1

/** @brief flag that tells wheter operation was 0: logic (M=1) or 1: arithmetic (M=0) */
#define	ALUM2	2

/** @brief execute the CPU for at most nsecs nano seconds */
ntime_t alto_execute(ntime_t nsecs)
{
	alto_leave = 0;
	alto_ntime = nsecs;

#if	DEBUG_TIMESLICES
	printf("@%lld: run timeslice for %lldns\n", ntime(), alto_ntime);
#endif

	/* get current task's next mpc and address modifier */
	cpu.next = cpu.task_mpc[cpu.task];
	cpu.next2 = cpu.task_next2[cpu.task];

	for (;;) {
		int do_bs, flags;

		if (alto_leave || alto_ntime < CPU_MICROCYCLE_TIME)
			break;

		cpu_display_state_machine();

		/* nano seconds per cycle */
		alto_ntime -= CPU_MICROCYCLE_TIME;
		cpu.task_ntime[cpu.task] += CPU_MICROCYCLE_TIME;

		/* next instruction's mpc */
		cpu.mpc = cpu.next;
		cpu.mir	= ucode_raw[cpu.mpc];
		cpu.rsel = MIR_RSEL;
		cpu.next = MIR_NEXT | cpu.next2;
		cpu.next2 = ALTO_GET(ucode_raw[cpu.next], 32, NEXT0, NEXT9) |
			(cpu.next2 & ~UCODE_PAGE_MASK);
		LOG((0,2,"\n%s-%04o: r:%02o af:%02o bs:%02o f1:%02o f2:%02o" \
			" t:%o l:%o next:%05o next2:%05o cycle:%lld\n",
			task_name[cpu.task], cpu.mpc,
			cpu.rsel, MIR_ALUF, MIR_BS, MIR_F1, MIR_F2,
			MIR_T, MIR_L, cpu.next, cpu.next2, cycle()));

		/*
		 * This bus source decoding is not performed if f1 = 7 or f2 = 7.
		 * These functions use the BS field to provide part of the address
		 * to the constant ROM
		 */
		do_bs = !(MIR_F1 == f1_const || MIR_F2 == f2_const);

		if (MIR_F1 == f1_load_mar) {
			if (check_mem_load_mar_stall(cpu.rsel)) {
				LOG((0,3, "	MAR<- stall\n"));
				cpu.next2 = cpu.next;
				cpu.next = cpu.mpc;
				continue;
			}
		} else if (MIR_F2 == f2_load_md) {
			if (check_mem_write_stall()) {
				LOG((0,3, "	MD<- stall\n"));
				cpu.next2 = cpu.next;
				cpu.next = cpu.mpc;
				continue;
			}
		}
		if (do_bs && MIR_BS == bs_read_md) {
			if (check_mem_read_stall()) {
				LOG((0,3, "	<-MD stall\n"));
				cpu.next2 = cpu.next;
				cpu.next = cpu.mpc;
				continue;
			}
		}

		cpu.bus = 0177777;

		if (cpu.rdram_flag)
			rdram();

		/*
		 * The constant memory is gated to the bus by F1 = 7, F2 = 7, or BS >= 4
		 */
		if (!do_bs || MIR_BS >= 4) {
			int addr = 8 * cpu.rsel + MIR_BS;
			LOG((0,2,"	%#o; BUS &= CONST[%03o]\n", const_prom[addr], addr));
			cpu.bus &= const_prom[addr];
		}

		/*
		 * early f2 has to be done before early bs, because the
		 * emulator f2 acsource or acdest may change rsel
		 */
		CALL_FN(f2, MIR_F2, 0);

		/*
		 * early bs can be done now
		 */
		if (do_bs)
			CALL_FN(bs, MIR_BS, 0);

		/*
		 * early f1
		 */
		CALL_FN(f1, MIR_F1, 0);

		/* compute the ALU function */
		switch (MIR_ALUF) {
		/**
		 * 00: ALU <- BUS
		 * PROM data for S3-0:1111 M:1 C:0
		 * 74181 function F=A
		 * T source is ALU
		 */
		case aluf_bus__alut:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,1,1,1, 1, 0));
#else
			cpu.alu = cpu.bus;
			cpu.aluc0 = 1;
#endif
			flags = TSELECT;
			LOG((0,2,"	ALU<- BUS (%#o := %#o)\n",
				cpu.alu, cpu.bus));
#if	ALUF_ASSERTION
			if (cpu.alu != cpu.bus)
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 01: ALU <- T
		 * PROM data for S3-0:1010 M:1 C:0
		 * 74181 function F=B
		 * T source is BUS
		 */
		case aluf_treg:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,0,1,0, 1, 0));
#else
			cpu.alu = cpu.t;
			cpu.aluc0 = 1;
#endif
			flags = 0;
			LOG((0,2,"	ALU<- T (%#o := %#o)\n",
				cpu.alu, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != cpu.t)
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 02: ALU <- BUS | T
		 * PROM data for S3-0:1110 M:1 C:0
		 * 74181 function F=A|B
		 * T source is ALU
		 */
		case aluf_bus_or_t__alut:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,1,1,0, 1, 0));
#else
			cpu.alu = cpu.bus | cpu.t;
			cpu.aluc0 = 1;
#endif
			flags = TSELECT;
			LOG((0,2,"	ALU<- BUS OR T (%#o := %#o | %#o)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != (cpu.bus | cpu.t))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 03: ALU <- BUS & T
		 * PROM data for S3-0:1011 M:1 C:0
		 * 74181 function F=A&B
		 * T source is BUS
		 */
		case aluf_bus_and_t:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,0,1,1, 1, 0));
#else
			cpu.alu = cpu.bus & cpu.t;
			cpu.aluc0 = 1;
#endif
			flags = 0;
			LOG((0,2,"	ALU<- BUS AND T (%#o := %#o & %#o)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != (cpu.bus & cpu.t))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 04: ALU <- BUS ^ T
		 * PROM data for S3-0:0110 M:1 C:0
		 * 74181 function F=A^B
		 * T source is BUS
		 */
		case aluf_bus_xor_t:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,1,1,0, 1, 0));
#else
			cpu.alu = cpu.bus ^ cpu.t;
			cpu.aluc0 = 1;
#endif
			flags = 0;
			LOG((0,2,"	ALU<- BUS XOR T (%#o := %#o ^ %#o)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != (cpu.bus ^ cpu.t))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 05: ALU <- BUS + 1
		 * PROM data for S3-0:0000 M:0 C:0
		 * 74181 function F=A+1
		 * T source is ALU
		 */
		case aluf_bus_plus_1__alut:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,0,0,0, 0, 0));
#else
			cpu.alu = cpu.bus + 1;
			cpu.aluc0 = (cpu.alu >> 16) & 1;
			cpu.alu &= 0177777;
#endif
			flags = ALUM2 | TSELECT;
			LOG((0,2,"	ALU<- BUS + 1 (%#o := %#o + 1)\n",
				cpu.alu, cpu.bus));
#if	ALUF_ASSERTION
			if (cpu.alu != ((cpu.bus + 1) & 0177777))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 06: ALU <- BUS - 1
		 * PROM data for S3-0:1111 M:0 C:1
		 * 74181 function F=A-1
		 * T source is ALU
		 */
		case aluf_bus_minus_1__alut:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,1,1,1, 0, 1));
#else
			cpu.alu = cpu.bus + 0177777;
			cpu.aluc0 = (~cpu.alu >> 16) & 1;
			cpu.alu &= 0177777;
#endif
			flags = ALUM2 | TSELECT;
			LOG((0,2,"	ALU<- BUS - 1 (%#o := %#o - 1)\n",
				cpu.alu, cpu.bus));
#if	ALUF_ASSERTION
			if (cpu.alu != ((cpu.bus - 1) & 0177777))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 07: ALU <- BUS + T
		 * PROM data for S3-0:1001 M:0 C:1
		 * 74181 function F=A+B
		 * T source is BUS
		 */
		case aluf_bus_plus_t:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,0,0,1, 0, 1));
#else
			cpu.alu = cpu.bus + cpu.t;
			cpu.aluc0 = (cpu.alu >> 16) & 1;
			cpu.alu &= 0177777;
#endif
			flags = ALUM2;
			LOG((0,2,"	ALU<- BUS + T (%#o := %#o + %#o)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != ((cpu.bus + cpu.t) & 0177777))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 10: ALU <- BUS - T
		 * PROM data for S3-0:0110 M:0 C:0
		 * 74181 function F=A-B
		 * T source is BUS
		 */
		case aluf_bus_minus_t:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,1,1,0, 0, 0));
#else
			cpu.alu = cpu.bus + ~cpu.t + 1;
			cpu.aluc0 = (~cpu.alu >> 16) & 1;
			cpu.alu &= 0177777;
#endif
			flags = ALUM2;
			LOG((0,2,"	ALU<- BUS - T (%#o := %#o - %#o)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != ((cpu.bus - cpu.t) & 0177777))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 11: ALU <- BUS - T - 1
		 * PROM data for S3-0:0110 M:0 C:1
		 * 74181 function F=A-B-1
		 * T source is BUS
		 */
		case aluf_bus_minus_t_minus_1:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,1,1,0, 0, 1));
#else
			cpu.alu = cpu.bus + ~cpu.t;
			cpu.aluc0 = (~cpu.alu >> 16) & 1;
			cpu.alu &= 0177777;
#endif
			flags = ALUM2;
			LOG((0,2,"	ALU<- BUS - T - 1 (%#o := %#o - %#o - 1)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != ((cpu.bus - cpu.t - 1) & 0177777))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 12: ALU <- BUS + T + 1
		 * PROM data for S3-0:1001 M:0 C:0
		 * 74181 function F=A+B+1
		 * T source is ALU
		 */
		case aluf_bus_plus_t_plus_1__alut:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,0,0,1, 0, 0));
#else
			cpu.alu = cpu.bus + cpu.t + 1;
			cpu.aluc0 = (cpu.alu >> 16) & 1;
			cpu.alu &= 0177777;
#endif
			flags = ALUM2 | TSELECT;
			LOG((0,2,"	ALU<- BUS + T + 1 (%#o := %#o + %#o + 1)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != ((cpu.bus + cpu.t + 1) & 0177777))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 13: ALU <- BUS + SKIP
		 * PROM data for S3-0:0000 M:0 C:SKIP
		 * 74181 function F=A (SKIP=1) or F=A+1 (SKIP=0)
		 * T source is ALU
		 */
		case aluf_bus_plus_skip__alut:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,0,0,0, 0, emu.skip^1));
#else
			cpu.alu = cpu.bus + emu.skip;
			cpu.aluc0 = (cpu.alu >> 16) & 1;
			cpu.alu &= 0177777;
#endif
			flags = ALUM2 | TSELECT;
			LOG((0,2,"	ALU<- BUS + SKIP (%#o := %#o + %#o)\n",
				cpu.alu, cpu.bus, emu.skip));
#if	ALUF_ASSERTION
			if (cpu.alu != ((cpu.bus + emu.skip) & 0177777))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 14: ALU <- BUS,T
		 * PROM data for S3-0:1011 M:1 C:0
		 * 74181 function F=A&B
		 * T source is ALU
		 */
		case aluf_bus_and_t__alut:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(1,0,1,1, 1, 0));
#else
			cpu.alu = cpu.bus & cpu.t;
			cpu.aluc0 = 1;
#endif
			flags = TSELECT;
			LOG((0,2,"	ALU<- BUS,T (%#o := %#o & %#o)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != (cpu.bus & cpu.t))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 15: ALU <- BUS & ~T
		 * PROM data for S3-0:0111 M:1 C:0
		 * 74181 function F=A&~B
		 * T source is BUS
		 */
		case aluf_bus_and_not_t:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,1,1,1, 1, 0));
#else
			cpu.alu = cpu.bus & ~cpu.t;
			cpu.aluc0 = 1;
#endif
			flags = 0;
			LOG((0,2,"	ALU<- BUS AND NOT T (%#o := %#o & ~%#o)\n",
				cpu.alu, cpu.bus, cpu.t));
#if	ALUF_ASSERTION
			if (cpu.alu != (cpu.bus & ~cpu.t))
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
			break;

		/**
		 * 16: ALU <- ???
		 * PROM data for S3-0:??? M:? C:?
		 * 74181 perhaps F=0 (0011/0/0)
		 * T source is BUS
		 */
		case aluf_undef_16:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,0,1,1, 0, 0));
#else
			cpu.alu = 0;
			cpu.aluc0 = 1;
#endif
			flags = ALUM2;
			LOG((0,0,"	ALU<- 0 (illegal aluf in task %s, mpc:%05o aluf:%02o)\n",
				task_name[cpu.task], cpu.mpc, MIR_ALUF));
#if	ALUF_ASSERTION
			if (cpu.alu != 0)
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif

		/**
		 * 17: ALU <- ???
		 * PROM data for S3-0:??? M:? C:?
		 * 74181 perhaps F=~0 (0011/0/1)
		 * T source is BUS
		 */
		case aluf_undef_17:
		default:
#if	USE_ALU_74181
			cpu.alu = alu_74181(SMC(0,0,1,1, 0, 1));
#else
			cpu.alu = 0177777;
			cpu.aluc0 = 1;
#endif
			flags = ALUM2;
			LOG((0,0,"	ALU<- 0 (illegal aluf in task %s, mpc:%05o aluf:%02o)\n",
				task_name[cpu.task], cpu.mpc, MIR_ALUF));
#if	ALUF_ASSERTION
			if (cpu.alu != 0)
				fatal(1,"aluf %s", aluf_name[MIR_ALUF]);
#endif
		}

		/* WRTRAM now, before L is changed */
		if (cpu.wrtram_flag)
			wrtram();

		switch (MIR_F1) {
		case f1_l_lsh_1:
			if (cpu.task == task_emu) {
				if (MIR_F2 == f2_emu_magic) {
					cpu.shifter = ((cpu.l << 1) | (cpu.t >> 15)) & 0177777;
					LOG((0,2,"	SHIFTER <-L MLSH 1 (%#o := %#o<<1|%#o)\n",
						cpu.shifter, cpu.l, cpu.t >> 15));
					break;
				}
				if (MIR_F2 == f2_emu_load_dns) {
					/* shifter is done in F2 */
					break;
				}
			}
			cpu.shifter = (cpu.l << 1) & 0177777;
			LOG((0,2,"	SHIFTER <-L LSH 1 (%#o := %#o<<1)\n",
				cpu.shifter, cpu.l));
			break;

		case f1_l_rsh_1:
			if (cpu.task == task_emu) {
				if (MIR_F2 == f2_emu_magic) {
					cpu.shifter = ((cpu.l >> 1) | (cpu.t << 15)) & 0177777;
					LOG((0,2,"	SHIFTER <-L MRSH 1 (%#o := %#o>>1|%#o)\n",
						cpu.shifter, cpu.l, (cpu.t << 15) & 0100000));
					break;
				}
				if (MIR_F2 == f2_emu_load_dns) {
					/* shifter is done in F2 */
					break;
				}
			}
			cpu.shifter = cpu.l >> 1;
			LOG((0,2,"	SHIFTER <-L RSH 1 (%#o := %#o>>1)\n",
				cpu.shifter, cpu.l));
			break;

		case f1_l_lcy_8:
			cpu.shifter = ((cpu.l >> 8) | (cpu.l << 8)) & 0177777;
			LOG((0,2,"	SHIFTER <-L LCY 8 (%#o := bswap %#o)\n",
				cpu.shifter, cpu.l));
			break;
		default:
			/* shifter passes L, if F1 is not one of L LSH 1, L RSH 1 or L LCY 8 */
			cpu.shifter = cpu.l;
		}

		/* late F1 is done now, if any */
		CALL_FN(f1, MIR_F1, 1);

		/* late F2 is done now, if any */
		CALL_FN(f2, MIR_F2, 1);

		/* late BS is done now, if no constant was put on the bus */
		if (do_bs)
			CALL_FN(bs, MIR_BS, 1);

		/*
		 * update L register and LALUC0, and also M register,
		 * if a RAM related task is active
		 */
		if (MIR_L) {
			/* load L from ALU */
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
				/* load M from ALU, if 'GOODTASK' */
				cpu.m = cpu.alu;
				/* also writes to S[bank][0], which can't be read */
				cpu.s[cpu.s_reg_bank[cpu.task]][0] = cpu.alu;
				LOG((0,2, "	M<- ALU (%#o)\n",
					cpu.alu));
			}
		}

		/* update T register, if LOADT is set */
		if (MIR_T) {
			cpu.cram_addr = cpu.alu;
			if (flags & TSELECT) {
				LOG((0,2, "	T<- ALU (%#o)\n", cpu.alu));
				cpu.t = cpu.alu;
			} else {
				LOG((0,2, "	T<- BUS (%#o)\n", cpu.bus));
				cpu.t = cpu.bus;
			}
		}

		if (cpu.task != cpu.next2_task) {
			/* switch now? */
			if (cpu.task == cpu.next_task) {
				/* one more microinstruction */
				cpu.next_task = cpu.next2_task;
			} else {
				/* save this task's mpc */
				cpu.task_mpc[cpu.task] = cpu.next;
				cpu.task_next2[cpu.task] = cpu.next2;
				cpu.task = cpu.next_task;
				LOG((log_TSW,1, "task switch to %02o:%s (cycle %lld)\n",
					cpu.task, task_name[cpu.task], cycle()));
				/* get new task's mpc */
				cpu.next = cpu.task_mpc[cpu.task];
				/* get address modifier after task switch (?) */
				cpu.next2 = cpu.task_next2[cpu.task];

				if (cpu.active_callback[cpu.task]) {
					/*
					 * let the task know it becomes active now
					 * and (most probably) reset the wakeup
					 */
					(*cpu.active_callback[cpu.task])();
				}
			}
		}
	}

	/* save this task's mpc and address modifier */
	cpu.task_mpc[cpu.task] = cpu.next;
	cpu.task_next2[cpu.task] = cpu.next2;

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


/** @brief software initiated reset (STARTF) */
int alto_soft_reset(void)
{
	int task;

	for (task = 0; task < task_COUNT; task++) {
		/* every task starts at mpc = task number, in either ROM0 or RAM0 */
		cpu.task_mpc[task] = (ctl2k_u38[task] >> 4) ^ 017;
		if (0 == (cpu.reset_mode & (1 << task)))
			cpu.task_mpc[task] |= UCODE_RAM_BASE;
	}
	/* switch to task 0 */
	cpu.next2_task = 0;

	/* all tasks start in ROM0 again */
	cpu.reset_mode = 0xffff;

	/* reset the display state machine values */
	cpu.dsp_time = 0;
	cpu.dsp_state = 020;

	/* return next task (?) */
	return cpu.next_task;
}


/** @brief reset the various registers */
int alto_reset(void)
{
	int task;

	memset(&cpu, 0, sizeof(cpu));
	
	cpu.dsp_time = 0;
	cpu.dsp_state = 020;

	/* all tasks start in ROM0 */
	cpu.reset_mode = 0xffff;

	memset(&ram_related, 0, sizeof(ram_related));

	/* install standard handlers in all tasks */
	for (task = 0; task < task_COUNT; task++) {
		int i;

		/* every task starts at mpc = task number, in either ROM0 or RAM0 */
		cpu.task_mpc[task] = (ctl2k_u38[task] >> 4) ^ 017;
		if (0 == (cpu.reset_mode & (1 << task)))
			cpu.task_mpc[task] |= UCODE_RAM_BASE;

		for (i = 0; i < bs_COUNT; i++) {
			fn_bs[0][task][i] = fn_bs_bad_0;
			fn_bs[1][task][i] = fn_bs_bad_1;
		}
		for (i = 0; i < f1_COUNT; i++) {
			fn_f1[0][task][i] = fn_f1_bad_0;
			fn_f1[1][task][i] = fn_f1_bad_1;
		}
		for (i = 0; i < f2_COUNT; i++) {
			fn_f2[0][task][i] = fn_f2_bad_0;
			fn_f2[1][task][i] = fn_f2_bad_1;
		}

		SET_FN(bs, read_r,		bs_read_r_0,	NULL);
		SET_FN(bs, load_r,		bs_load_r_0,	bs_load_r_1);
		SET_FN(bs, no_source,		NULL,		NULL);
		/* bs_task_3 and bs_task_4 are task specific */
		SET_FN(bs, read_md,		bs_read_md_0,	NULL);
		SET_FN(bs, mouse,		bs_mouse_0,	NULL);
		SET_FN(bs, disp,		bs_disp_0,	NULL);

		SET_FN(f1, nop,			NULL,		NULL);
		SET_FN(f1, load_mar,		NULL,		f1_load_mar_1);
		SET_FN(f1, task,		f1_task_0,	NULL);
		/* not all tasks have the f1_block */
		SET_FN(f1, l_lsh_1,		NULL,		NULL);	/* inlined */
		SET_FN(f1, l_rsh_1,		NULL,		NULL);	/* inlined */
		SET_FN(f1, l_lcy_8,		NULL,		NULL);	/* inlined */
		SET_FN(f1, const,		NULL,		NULL);
		/* f1_task_10 to f1_task_17 are task specific */

		SET_FN(f2, nop,			NULL,		NULL);
		SET_FN(f2, bus_eq_zero,		NULL,		f2_bus_eq_zero_1);
		SET_FN(f2, shifter_lt_zero,	NULL,		f2_shifter_lt_zero_1);
		SET_FN(f2, shifter_eq_zero,	NULL,		f2_shifter_eq_zero_1);
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
	CPU_SET_TASK_WAKEUP(0);

	return 0;
}
