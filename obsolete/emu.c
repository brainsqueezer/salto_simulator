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
 * Emulator task bus source, F1, and F2 functions
 *
 * $Id: emu.c,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include "alto.h"
#include "cpu.h"
#include "emu.h"
#include "memory.h"
#include "ram.h"
#include "ether.h"
#include "debug.h"

/** @brief CTL2K_U3 address line for F2 function */
#define	CTL2K_U3(f2) (f2 == f2_emu_idisp ? 0x80 : 0x00)

/** @brief set non-zero to use expressions after the schematics for RSEL[3-4] */
#define	USE_SCHEMATICS_RSEL	1

/** @brief emulator context */
emu_t emu;

/**
 * @brief register selection
 *
 * From the schematics: 08_ALU, page 6 (PDF page 4)
 *
 * EMACT		emulator task active
 * F2[0-2]=111b		<-ACSOURCE and F2_17
 * F2[0-2]=101b		DNS<- and ACDEST<-
 *
 * 	u49 (8 input NAND 74S30)
 * 	----------------------------------------------
 * 	F2[0] & F2[2] & F2[1]' & IR[03]' & EMACT
 *
 *		F2[0-2]	IR[03]	EMACT	output u49pin8
 *		--------------------------------------
 *		101	0	1	0
 *		all others		1
 *
 *
 * 	u59 (8 input NAND 74S30)
 * 	----------------------------------------------
 * 	F2[0] & F2[2] & F2[1] & IR[01]' & EMACT
 *
 *		F2[0-2]	IR[01]	EMACT	output u59pin8
 *		--------------------------------------
 *		111	0	1	0
 *		all others		1
 *
 * 	u70d (2 input NOR 74S02 used as inverter)
 * 	---------------------------------------------
 * 	RSEL3 -> RSEL3'
 *
 *		u79b (3 input NAND 74S10)
 *		---------------------------------------------
 *			u49pin8	u59pin8	RSEL3'	output 6RA3
 *			-------------------------------------
 *			1	1	1	0
 *			0	x	x	1
 *			x	0	x	1
 *			x	x	0	1
 *
 *
 * 	u60 (8 input NAND 74S30)
 * 	----------------------------------------------
 * 	F2[0] & F2[2] & F2[1]' & IR[02]' & EMACT
 *
 *		F2[0-2]	IR[02]	EMACT	output u60pin8
 *		--------------------------------------
 *		101	0	1	0
 *		all others		1
 *
 * 	u50 (8 input NAND 74S30)
 * 	----------------------------------------------
 * 	F2[0] & F2[2] & F2[1] & IR[04]' & EMACT
 *
 *		F2[0-2]	IR[04]	EMACT	output u50pin8
 *		--------------------------------------
 *		111	0	1	0
 *		all others		1
 *
 * 	u70c (2 input NOR 74S02 used as inverter)
 * 	---------------------------------------------
 * 	RSEL4 -> RSEL4'
 *
 *
 *		u79c (3 input NAND 74S10)
 *		---------------------------------------------
 *			u60pin8	u50pin8	RSEL4'	output 8RA4
 *			-------------------------------------
 *			1	1	1	0
 *			0	x	x	1
 *			x	0	x	1
 *			x	x	0	1
 *
 * BUG?: schematics seem to have swapped IR(04)' and IR(02)' inputs for the
 * RA4 decoding, because SrcAC is selected from IR[1-2]?
 */

#if	USE_SCHEMATICS_RSEL
#define	RA3(f2,ir,rs)	(\
	(((ALTO_GET(f2,4,0,2)==5 && ALTO_GET(ir,16,3,3)==0) ? 0 : 1) && \
	((ALTO_GET(f2,4,0,2)==7 && ALTO_GET(ir,16,1,1)==0) ? 0 : 1) && \
	(ALTO_GET(rs,5,3,3)==0)) ? 0 : 1)

#define	RA4(f2,ir,rs)	(\
	(((ALTO_GET(f2,4,0,2)==5 && ALTO_GET(ir,16,4,4)==0) ? 0 : 1) && \
	((ALTO_GET(f2,4,0,2)==7 && ALTO_GET(ir,16,2,2)==0) ? 0 : 1) && \
	(ALTO_GET(rs,5,4,4)==0)) ? 0 : 1)
#endif	/* USE_SCHEMATICS_RSEL */

/**
 * @brief early: drive bus by IR[8-15], possibly sign extended
 *
 * The high order bits of IR cannot be read directly, but the
 * displacement field of IR (8 low order bits) may be read with
 * the <-DISP bus source. If the X field of the instruction is
 * zero (i.e., it specifies page 0 addressing), then the DISP
 * field of the instruction is put on BUS[8-15] and BUS[0-7]
 * is zeroed. If the X field of the instruction is non-zero
 * (i.e. it specifies PC-relative or base-register addressing)
 * then the DISP field is sign-extended and put on the bus.
 *
 */
static void bs_disp_0(void)
{
	int and = IR_DISP(emu.ir);
	if (IR_X(emu.ir)) {
		if (ALTO_GET(and,16,8,8))
			ALTO_PUT(and,16,0,7,-1);
		LOG((0,2, "	<-DISP (%06o)\n", and));
	} else {
		LOG((0,2, "	<-DISP (%06o)\n", and));
	}
	cpu.bus &= and;
}

/**
 * @brief f1_block early: block task
 *
 * The task request for the active task is cleared
 */
static void f1_block_0(void)
{
#if	0
	CPU_CLR_TASK_WAKEUP(cpu.task);
	LOG((0,2, "	BLOCK %02o:%s\n", cpu.task, task_name[cpu.task]));
#else
	fatal(1, "Emulator task want's to BLOCK. mpc:%05o %11o\n",
		cpu.mpc, cpu.mir);
#endif
}

/**
 * @brief f1_load_rmr late: load the reset mode register
 */
static void f1_load_rmr_1(void)
{
	LOG((0,2,"	RMR<-; BUS (%#o)\n", cpu.bus));
	cpu.reset_mode = cpu.bus;
}

/**
 * @brief f1_load_esrb late: load the extended S register bank from BUS[12-14]
 */
static void f1_load_esrb_1(void)
{
	int val = ALTO_GET(cpu.bus,16,12,14);
	LOG((0,2,"	ESRB<-; BUS[12-14] (%#o)\n", val));
	cpu.s_reg_bank[cpu.task] = val;
}

/**
 * @brief f1_rsnf early: drive the bus from the Ethernet node ID
 */
static void f1_rsnf_0(void)
{
	int and = 0177400 | ETHERNET_NODE_ID;
	LOG((0,2,"	<-RSNF; (%#o)\n", and));
	cpu.bus &= and;
}

/**
 * @brief f1_startf early: defines commands for for I/O hardware, including Ethernet
 *
 * (SIO) Start I/O is included to facilitate I/O control, It places the contents of
 * AC0 on the processor bus and executes the STARTF function (F1 = 17B). By convention,
 * bits of AC0 must be "1" in order to signal devices. See Appendix C for a summary of
 * assigned bits.
 *    Bit 0  100000B   Standard Alto: Software boot feature
 *    Bit 14 000002B   Standard Alto: Ethernet
 *    Bit 15 000001B   Standard Alto: Ethernet
 * If bit 0 of AC0 is 1, and if an Ethernet board is plugged into the Alto, the machine
 * will boot, just as if the "boot button" were pressed (see sections 3.4, 8.4 and 9.2.2
 * for discussions of bootstrapping).
 *
 * SIO also returns a result in AC0. If the Ethernet hardware is installed, the serial
 * number and/or Ethernet host address of the machine (0-377B) is loaded into AC0[8-15].
 * (On Alto I, the serial number and Ethernet host address are equivalent; on Alto II,
 * the value loaded into AC0 is the Ethernet host address only.) If Ethernet hardware
 * is missing, AC0[8-15] = 377B. Microcode installed after June 1976, which this manual
 * describes, returns AC0[0] = 0. Microcode installed prior to June 1976 returns
 * AC0[0] = 1; this is a quick way to acquire the approximate vintage of a machine's
 * microcode.
 */
static void f1_startf_0(void)
{
	LOG((0,2,"	STARTF (BUS is %06o)\n", cpu.bus));
	/* TODO: what do we do here? reset the CPU on bit 0? */
	if (ALTO_GET(cpu.bus,16,0,0)) {
		LOG((0,2,"****	Software boot feature\n"));
		alto_soft_reset();
	}
	if (ALTO_GET(cpu.bus,16,14,15)) {
		/* anything else to do? */
		CPU_SET_TASK_WAKEUP(task_ether);
	}
}

/**
 * @brief f2_busodd late: branch on odd bus
 */
static void f2_busodd_1(void)
{
	int or = cpu.bus & 1;
	LOG((0,2,"	BUSODD; %sbranch (%#o|%#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief f2_magic late: shift and use T
 */
static void f2_magic_1(void)
{
	int XC;
	switch (MIR_F1(cpu.mir)) {
	case f1_l_lsh_1:	/* <-L MLSH 1 */
		XC = cpu.t >> 15;
		cpu.shifter |= XC;
		LOG((0,2,"	<-L MLSH 1 (XC %o)", XC));
		break;
	case f1_l_rsh_1:	/* <-L MRSH 1 */
		XC = cpu.t & 1;
		cpu.shifter |= XC << 15;
		LOG((0,2,"	<-L MRSH 1 (XC %o)", XC));
		break;
	}
}

/**
 * @brief dns early: modify RESELECT with DstAC = (3 - IR[3-4])
 */
static void f2_load_dns_0(void)
{
#if	USE_SCHEMATICS_RSEL
	ALTO_PUT(cpu.rsel,5,3,3,RA3(f2_emu_load_dns,emu.ir,cpu.rsel));
	ALTO_PUT(cpu.rsel,5,4,4,RA4(f2_emu_load_dns,emu.ir,cpu.rsel));
#else
	cpu.rsel = (cpu.rsel & ~3) | (3 - IR_DstAC(emu.ir));
#endif
	LOG((0,2,"	DNS<-; rsel := DstAC (%#o %s)\n", cpu.rsel, r_name[cpu.rsel]));
}

/**
 * @brief late: do novel shifts
 *
 * New emulator carry is selected by instruction register
 * bits CY = IR[10-11]. R register and emulator carry are
 * loaded only if NL = IR[12] is 0.
 * SKIP is set according to SK = IR[13-15].
 *
 *  CARRY     = !emu.cy
 *  exorB     = IR11 ^ IR10
 *  ORA       = !(exorB | CARRY)
 *            = (exorB | CARRY) ^ 1
 *  exorC     = ORA ^ !IR11
 *            = ORA ^ (IR11 ^ 1)
 *  exorD     = exorC ^ LALUC0
 *  XC        = !(!(DNS & exorD) & !(MAGIC & OUTza))
 *            = (DNS & exorD) | (MAGIC & OUTza)
 *            = exorD, because this is DNS
 *  NEWCARRY  = [XC, L(00), L(15), XC] for F1 = no shift, <-L RSH 1, <-L LSH 1, LCY 8
 *  SHZERO    = shifter == 0
 *  DCARRY    = !((!IR12 & NEWCARRY) | (IR12 & CARRY))
 *            = (((IR12 ^ 1) & NEWCARRY) | (IR12 & CARRY)) ^ 1
 *  DSKIP     = !((!NEWCARRY & IR14) | (SHZERO & IR13)) ^ !IR15
 *            = ((((NEWCARRY ^ 1) & IR14) | (SHZERO & IR13)) ^ 1) ^ (IR15 ^ 1)
 *            = (((NEWCARRY ^ 1) & IR14) | (SHZERO & IR13)) ^ IR15
 */
static void f2_load_dns_1(void)
{
	uint8_t IR10 = ALTO_GET(emu.ir,16,10,10);
	uint8_t IR11 = ALTO_GET(emu.ir,16,11,11);
	uint8_t IR12 = ALTO_GET(emu.ir,16,12,12);
	uint8_t IR13 = ALTO_GET(emu.ir,16,13,13);
	uint8_t IR14 = ALTO_GET(emu.ir,16,14,14);
	uint8_t IR15 = ALTO_GET(emu.ir,16,15,15);
	uint8_t exorB = IR11 ^ IR10;
	uint8_t CARRY = emu.cy;
	uint8_t ORA = (exorB | CARRY) ^ 1;
	uint8_t exorC = ORA ^ (IR11 ^ 1);
	uint8_t exorD = exorC ^ cpu.laluc0;
	uint8_t XC = exorD;
	uint8_t NEWCARRY;
	uint8_t DCARRY;
	uint8_t DSKIP;
	uint8_t SHZERO;

	switch (MIR_F1(cpu.mir)) {
	case f1_l_rsh_1:	/* <-L RSH 1 */
		cpu.shifter |= XC << 15;
		NEWCARRY = cpu.l & 1;
		break;
	case f1_l_lsh_1:	/* <-L LSH 1 */
		cpu.shifter |= XC;
		NEWCARRY = cpu.l >> 15;
		break;
	default:
		NEWCARRY = XC;
		break;
	}
	SHZERO = (cpu.shifter == 0) ? 1 : 0;

	DCARRY = (((IR12 ^ 1) & NEWCARRY) | (IR12 & CARRY)) ^ 1;

	DSKIP = (((NEWCARRY ^ 1) & IR14) | (SHZERO & IR13)) ^ IR15;

	/* DCARRY is latched as new emu.cy */
	emu.cy = DCARRY ^ 1;

	/* DSKIP is latched as new emu.skip */
	emu.skip = DSKIP;

	/* !(IR12 & DNS) -> WR' = 0 for the register file */
	if (!IR12) {
		cpu.r[cpu.rsel] = cpu.shifter;
	}
}

/**
 * @brief f2_acdest early: modify RSELECT with DstAC = (3 - IR[3-4])
 */
static void f2_acdest_0(void)
{
#if	USE_SCHEMATICS_RSEL
	ALTO_PUT(cpu.rsel,5,3,3,RA3(f2_emu_acdest,emu.ir,cpu.rsel));
	ALTO_PUT(cpu.rsel,5,4,4,RA4(f2_emu_acdest,emu.ir,cpu.rsel));
#else
	cpu.rsel = (cpu.rsel & ~3) | (3 - IR_DstAC(emu.ir));
#endif
	LOG((0,2,"	ACDEST<-; mux (rsel:%#o %s)\n", cpu.rsel, r_name[cpu.rsel]));
}

/**
 * @brief f2_load_ir late: load instruction register IR and branch on IR[0,5-7]
 *
 * Loading the IR clears the skip latch.
 */
static void f2_load_ir_1(void)
{
	int or = (ALTO_GET(cpu.bus,16,0,0) << 3) | ALTO_GET(cpu.bus,16,5,7);
#if	DEBUG
	if (loglevel[task_emu] > 1) {
		char dasm[64];
		dbg_dasm(dasm, sizeof(dasm), 0, cpu.r[6], cpu.bus);
		LOG((0,2,"	IR<-; IR = %06o, branch on IR[0,5-7] (%#o|%#o)\n",
			cpu.bus, cpu.next2, or));
		/* disassembled instruction */
		LOG((0,2,"		%06o: %06o %s\n",
			mem.mar, cpu.bus, dasm));
	}
	/* special logging of some opcodes */
	switch (cpu.bus) {
	case op_BLT:
		if (cpu.r[rsel_ac3] != 0) {
			LOG((0,0,"	BLT dst:%#o src:%#o size:%#o\n",
				(cpu.r[rsel_ac1] + cpu.r[rsel_ac3] + 1) & 0177777,
				(cpu.r[rsel_ac0] + 1) & 017777,
				-cpu.r[rsel_ac3] & 0177777));
		}
		break;
	case op_BLKS:
		LOG((0,0,"	BLKS dst:%#o val:%#o size:%#o\n",
			(cpu.r[rsel_ac1] + cpu.r[rsel_ac3] + 1) & 0177777,
			cpu.r[rsel_ac0],
			-cpu.r[rsel_ac3] & 0177777));
		break;
	case op_DIAGNOSE1:
		LOG((0,0,"	DIAGNOSE1 AC0:%06o AC1:%06o AC2:%06o AC3:%06o\n",
			cpu.r[rsel_ac0], cpu.r[rsel_ac1],
			cpu.r[rsel_ac2], cpu.r[rsel_ac3]));
		break;
	case op_DIAGNOSE2:
		LOG((0,0,"	DIAGNOSE2 AC0:%06o AC1:%06o AC2:%06o AC3:%06o\n",
			cpu.r[rsel_ac0], cpu.r[rsel_ac1],
			cpu.r[rsel_ac2], cpu.r[rsel_ac3]));
		break;
	case op_BITBLT:
		{
			int bbt = cpu.r[rsel_ac2];
			LOG((0,0,"	BITBLT AC1:%06o AC2:%06o\n",
				cpu.r[rsel_ac1], cpu.r[rsel_ac2]));
			LOG((0,0,"		function  : %06o\n", debug_read_mem(bbt)));
			LOG((0,0,"		unused    : %06o\n", debug_read_mem(bbt+1)));
			LOG((0,0,"		DBCA      : %06o\n", debug_read_mem(bbt+2)));
			LOG((0,0,"		DBMR      : %06o\n", debug_read_mem(bbt+3)));
			LOG((0,0,"		DLX       : %06o\n", debug_read_mem(bbt+4)));
			LOG((0,0,"		DTY       : %06o\n", debug_read_mem(bbt+5)));
			LOG((0,0,"		DW        : %06o\n", debug_read_mem(bbt+6)));
			LOG((0,0,"		DH        : %06o\n", debug_read_mem(bbt+7)));
			LOG((0,0,"		SBCA      : %06o\n", debug_read_mem(bbt+8)));
			LOG((0,0,"		SBMR      : %06o\n", debug_read_mem(bbt+9)));
			LOG((0,0,"		SLX       : %06o\n", debug_read_mem(bbt+10)));
			LOG((0,0,"		STY       : %06o\n", debug_read_mem(bbt+11)));
			LOG((0,0,"		GRAY0-3   : %06o %06o %06o %06o\n",
				debug_read_mem(bbt+12), debug_read_mem(bbt+13),
				debug_read_mem(bbt+14), debug_read_mem(bbt+15)));
		}
		paused = 1;
		break;
	}
#endif	/* DEBUG */
	emu.ir = cpu.bus;
	emu.skip = 0;
	CPU_BRANCH(or);
}


/**
 * @brief f2_idisp late: branch on: arithmetic IR_SH, others PROM ctl2k_u3[IR[1-7]]
 */
static void f2_idisp_1(void)
{
	int or;

	if (IR_ARITH(emu.ir)) {
		/* 1xxxxxxxxxxxxxxx */
		or = IR_SH(emu.ir) ^ 3;			/* complement of SH */
		LOG((0,2,"	IDISP<-; branch on SH^3 (%#o|%#o)\n",
			cpu.next2, or));
	} else {
		int addr = CTL2K_U3(f2_emu_idisp) + ALTO_GET(emu.ir,16,1,7);
		/* 0???????xxxxxxxx */
		or = ctl2k_u3[addr];
		LOG((0,2,"	IDISP<-; IR (%#o) branch on PROM ctl2k_u3[%03o] (%#o|%#o)\n",
			emu.ir, addr, cpu.next2, or));
	}

	CPU_BRANCH(or);
}

/**
 * @brief f2_acsource early: modify RSELECT with SrcAC = (3 - IR[1-2])
 */
static void f2_acsource_0(void)
{
#if	USE_SCHEMATICS_RSEL
	ALTO_PUT(cpu.rsel,5,3,3,RA3(f2_emu_acsource,emu.ir,cpu.rsel));
	ALTO_PUT(cpu.rsel,5,4,4,RA4(f2_emu_acsource,emu.ir,cpu.rsel));
#else
	cpu.rsel = (cpu.rsel & ~3) | (3 - IR_SrcAC(emu.ir));
#endif
	LOG((0,2,"	<-ACSOURCE; rsel := SrcAC (%#o %s)\n", cpu.rsel, r_name[cpu.rsel]));
}

/**
 * @brief f2_acsource late: branch on: arithmetic IR_SH, others PROM ctl2k_u3[IR[1-7]]
 */
static void f2_acsource_1(void)
{
	int or;

	if (IR_ARITH(emu.ir)) {
		/* 1xxxxxxxxxxxxxxx */
		or = IR_SH(emu.ir) ^ 3;			/* complement of SH */
		LOG((0,2,"	<-ACSOURCE; branch on SH^3 (%#o|%#o)\n",
			cpu.next2, or));
	} else {
		int addr = CTL2K_U3(f2_emu_acsource) + ALTO_GET(emu.ir,16,1,7);
		/* 0???????xxxxxxxx */
		or = ctl2k_u3[addr];
		LOG((0,2,"	<-ACSOURCE; branch on PROM ctl2k_u3[%03o] (%#o|%#o)\n",
			addr, cpu.next2, or));
	}
	CPU_BRANCH(or);
}

int init_emu(int task)
{
	init_ram(task);

	SET_FN(bs, emu_read_sreg,		bs_read_sreg_0,	NULL);
	SET_FN(bs, emu_load_sreg,		bs_load_sreg_0,	bs_load_sreg_1);
	SET_FN(bs, disp,			bs_disp_0,	NULL);

	/* catch the emulator task wanting to block (wrong branch) */
	SET_FN(f1, block,			f1_block_0,	NULL);

	SET_FN(f1, emu_swmode,			NULL,		f1_swmode_1);
	SET_FN(f1, emu_wrtram,			NULL,		f1_wrtram_1);
	SET_FN(f1, emu_rdram,			NULL,		f1_rdram_1);
	SET_FN(f1, emu_load_rmr,		NULL,		f1_load_rmr_1);
	SET_FN(f1, emu_load_esrb,		NULL,		f1_load_esrb_1);
	SET_FN(f1, emu_rsnf,			f1_rsnf_0,	NULL);
	SET_FN(f1, emu_startf,			f1_startf_0,	NULL);

	SET_FN(f2, emu_busodd,			NULL,		f2_busodd_1);
	SET_FN(f2, emu_magic,			NULL,		f2_magic_1);
	SET_FN(f2, emu_load_dns,		f2_load_dns_0,	f2_load_dns_1);
	SET_FN(f2, emu_acdest,			f2_acdest_0,	NULL);
	SET_FN(f2, emu_load_ir,			NULL,		f2_load_ir_1);
	SET_FN(f2, emu_idisp,			NULL,		f2_idisp_1);
	SET_FN(f2, emu_acsource,		f2_acsource_0,	f2_acsource_1);

	return 0;
}
