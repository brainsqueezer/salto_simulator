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
 * $Id: emu.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "alto.h"
#include "timer.h"
#include "cpu.h"
#include "emu.h"
#include "memory.h"
#include "ram.h"
#include "ether.h"
#include "debug.h"

/** @brief CTL2K_U3 address line for F2 function */
#define	CTL2K_U3(f2) (f2 == f2_emu_idisp ? 0x80 : 0x00)

/** @brief set non-zero to use expressions after the schematics for RSEL[3-4] */
#define	USE_SCHEMATICS_RSEL	0

/** @brief emulator context */
emu_t emu;

/**
 * @brief register selection
 *
 * <PRE>
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
 * </PRE>
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
		and = ((signed char)and) & 0177777;
	}
	LOG((0,2, "	<-DISP (%06o)\n", and));
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
#elif	0
	fatal(1, "Emulator task want's to BLOCK.\n" \
		"%s-%04o: r:%02o af:%02o bs:%02o f1:%02o f2:%02o" \
		" t:%o l:%o next:%05o next2:%05o cycle:%lld\n",
		task_name[cpu.task], cpu.mpc,
		cpu.rsel, MIR_ALUF, MIR_BS,
		MIR_F1, MIR_F2,
		MIR_T, MIR_L,
		MIR_NEXT, cpu.next2,
		ntime() / CPU_MICROCYCLE_TIME);
#else
	/* just ignore (?) */
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
	LOG((0,2,"	ESRB<-; BUS[12-14] (%#o)\n", cpu.bus));
	printf("	ESRB<-; BUS[12-14] (%#o)\n", cpu.bus);
	cpu.s_reg_bank[cpu.task] = ALTO_GET(cpu.bus,16,12,14);
}

/**
 * @brief f1_rsnf early: drive the bus from the Ethernet node ID
 *
 * TODO: move this to the Ethernet code? It's really a emulator
 * specific function that is decoded by the Ethernet card.
 */
static void f1_rsnf_0(void)
{
	int and = 0177400 | ether_id;
	LOG((0,2,"	<-RSNF; (%#o)\n", and));
	cpu.bus &= and;
}

/**
 * @brief f1_startf early: defines commands for for I/O hardware, including Ethernet
 * <PRE>
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
 * </PRE>
 *
 * TODO: move this to the Ethernet code? It's really a emulator
 * specific function that is decoded by the Ethernet card.
 */
static void f1_startf_0(void)
{
	LOG((0,2,"	STARTF (BUS is %06o)\n", cpu.bus));
	/* TODO: what do we do here? reset the CPU on bit 0? */
	if (ALTO_BIT(cpu.bus,16,0)) {
		LOG((0,2,"****	Software boot feature\n"));
		alto_soft_reset();
	} else {
		PUT_ETH_ICMD(eth.status, ALTO_BIT(cpu.bus,16,14));
		PUT_ETH_OCMD(eth.status, ALTO_BIT(cpu.bus,16,15));
		if (GET_ETH_ICMD(eth.status) || GET_ETH_OCMD(eth.status))
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

#if	0
/**
 * @brief f2_magic late: shift and use T
 */
static void f2_magic_1(void)
{
	int XC;
	switch (MIR_F1) {
	case f1_l_lsh_1:	/* <-L MLSH 1 */
		XC = (cpu.t >> 15) & 1;
		cpu.shifter = (cpu.l << 1) & 0177777;
		cpu.shifter |= XC;
		LOG((0,2,"	<-L MLSH 1 (shifer:%06o XC:%o)", cpu.shifter, XC));
		break;
	case f1_l_rsh_1:	/* <-L MRSH 1 */
		XC = cpu.t & 1;
		cpu.shifter = cpu.l >> 1;
		cpu.shifter |= XC << 15;
		LOG((0,2,"	<-L MRSH 1 (shifter:%06o XC:%o)", cpu.shifer, XC));
		break;
	case f1_l_lcy_8:	/* <-L LCY 8 */
	default:		/* other */
		break;
	}
}
#endif

/**
 * @brief dns early: modify RESELECT with DstAC = (3 - IR[3-4])
 */
static void f2_load_dns_0(void)
{
#if	USE_SCHEMATICS_RSEL
	ALTO_PUT(cpu.rsel, 5, 3, 3,
		RA3(f2_emu_load_dns, emu.ir, cpu.rsel));
	ALTO_PUT(cpu.rsel, 5, 4, 4,
		RA4(f2_emu_load_dns, emu.ir, cpu.rsel));
#else
	ALTO_PUT(cpu.rsel, 5, 3, 4, IR_DstAC(emu.ir) ^ 3);
#endif
	LOG((0,2,"	DNS<-; rsel := DstAC (%#o %s)\n",
		cpu.rsel, r_name[cpu.rsel]));
}

/**
 * @brief late: do novel shifts
 *
 * <PRE>
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
 *            = ORA ^ IR11 ^ 1
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
 * </PRE>
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
	uint8_t CARRY = emu.cy ^ 1;
	uint8_t ORA = (exorB | CARRY) ^ 1;
	uint8_t exorC = ORA ^ (IR11 ^ 1);
	uint8_t exorD = exorC ^ cpu.laluc0;
	uint8_t XC = exorD;
	uint8_t NEWCARRY;
	uint8_t DCARRY;
	uint8_t DSKIP;
	uint8_t SHZERO;

	switch (MIR_F1) {
	case f1_l_rsh_1:	/* <-L RSH 1 */
		NEWCARRY = cpu.l & 1;
		cpu.shifter = ((cpu.l >> 1) | (XC << 15)) & 0177777;
		LOG((0,2,"	DNS; <-L RSH 1 (shifter:%06o XC:%o NEWCARRY:%o)",
			cpu.shifter, XC, NEWCARRY));
		break;
	case f1_l_lsh_1:	/* <-L LSH 1 */
		NEWCARRY = (cpu.l >> 15) & 1;
		cpu.shifter = ((cpu.l << 1) | XC) & 0177777;
		LOG((0,2,"	DNS; <-L LSH 1 (shifter:%06o XC:%o NEWCARRY:%o)",
			cpu.shifter, XC, NEWCARRY));
		break;
	case f1_l_lcy_8:	/* <-L LCY 8 */
	default:		/* other */
		NEWCARRY = XC;
		LOG((0,2,"	DNS; (shifter:%06o NEWCARRY:%o)",
			cpu.shifter, NEWCARRY));
		break;
	}
	SHZERO = (cpu.shifter == 0);
	DCARRY = (((IR12 ^ 1) & NEWCARRY) | (IR12 & CARRY)) ^ 1;
	DSKIP = (((NEWCARRY ^ 1) & IR14) | (SHZERO & IR13)) ^ IR15;

	/* DCARRY is latched as new emu.cy */
	emu.cy = DCARRY;

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
	ALTO_PUT(cpu.rsel, 5, 3, 3, RA3(f2_emu_acdest, emu.ir, cpu.rsel));
	ALTO_PUT(cpu.rsel, 5, 4, 4, RA4(f2_emu_acdest, emu.ir, cpu.rsel));
#else
	ALTO_PUT(cpu.rsel, 5, 3, 4, IR_DstAC(emu.ir) ^ 3);
#endif
	LOG((0,2,"	ACDEST<-; mux (rsel:%#o %s)\n",
		cpu.rsel, r_name[cpu.rsel]));
}

#if	DEBUG
static void bitblt_info(void)
{
	static const char *type_name[4] = {"bitmap","complement","and gray","gray"};
	static const char *oper_name[4] = {"replace","paint","invert","erase"};
	int bbt = cpu.r[rsel_ac2];
	int val = debug_read_mem(bbt);

	LOG((0,3,"	BITBLT AC1:%06o AC2:%06o\n",
		cpu.r[rsel_ac1], cpu.r[rsel_ac2]));
	LOG((0,3,"		function  : %06o\n", val));
	LOG((0,3,"			src extRAM: %o\n", ALTO_BIT(val,16,10)));
	LOG((0,3,"			dst extRAM: %o\n", ALTO_BIT(val,16,11)));
	LOG((0,3,"			src type  : %o (%s)\n",
		ALTO_GET(val,16,12,13), type_name[ALTO_GET(val,16,12,13)]));
	LOG((0,3,"			operation : %o (%s)\n",
		ALTO_GET(val,16,14,15), oper_name[ALTO_GET(val,16,14,15)]));
	val = debug_read_mem(bbt+1);
	LOG((0,3,"		unused AC2: %06o (%d)\n", val, val));
	val = debug_read_mem(bbt+2);
	LOG((0,3,"		DBCA      : %06o (%d)\n", val, val));
	val = debug_read_mem(bbt+3);
	LOG((0,3,"		DBMR      : %06o (%d words)\n", val, val));
	val = debug_read_mem(bbt+4);
	LOG((0,3,"		DLX       : %06o (%d bits)\n", val, val));
	val = debug_read_mem(bbt+5);
	LOG((0,3,"		DTY       : %06o (%d scanlines)\n", val, val));
	val = debug_read_mem(bbt+6);
	LOG((0,3,"		DW        : %06o (%d bits)\n", val, val));
	val = debug_read_mem(bbt+7);
	LOG((0,3,"		DH        : %06o (%d scanlines)\n", val, val));
	val = debug_read_mem(bbt+8);
	LOG((0,3,"		SBCA      : %06o (%d)\n", val, val));
	val = debug_read_mem(bbt+9);
	LOG((0,3,"		SBMR      : %06o (%d words)\n", val, val));
	val = debug_read_mem(bbt+10);
	LOG((0,3,"		SLX       : %06o (%d bits)\n", val, val));
	val = debug_read_mem(bbt+11);
	LOG((0,3,"		STY       : %06o (%d scanlines)\n", val, val));
	LOG((0,3,"		GRAY0-3   : %06o %06o %06o %06o\n",
		debug_read_mem(bbt+12), debug_read_mem(bbt+13),
		debug_read_mem(bbt+14), debug_read_mem(bbt+15)));
}
#endif	/* DEBUG */

/**
 * @brief f2_load_ir late: load instruction register IR and branch on IR[0,5-7]
 *
 * Loading the IR clears the skip latch.
 */
static void f2_load_ir_1(void)
{
	int or = (ALTO_GET(cpu.bus,16,0,0) << 3) | ALTO_GET(cpu.bus,16,5,7);

#if	DEBUG
	if (ll[task_emu].level > 1) {
		char dasm[64];
		dbg_dasm(dasm, sizeof(dasm), 0, cpu.r[6], cpu.bus);
		LOG((0,2,"	IR<-; IR = %06o, branch on IR[0,5-7] (%#o|%#o)\n",
			cpu.bus, cpu.next2, or));
		/* disassembled instruction */
		LOG((0,2,"		%06o: %06o %s\n",
			mem.mar, cpu.bus, dasm));
	}
#endif	/* DEBUG */

	/* special logging of some opcodes */
	switch (cpu.bus) {
	case op_CYCLE:
		LOG((0,3,"	CYCLE AC0:#o\n",
			cpu.r[rsel_ac0]));
		break;
	case op_CYCLE + 1: case op_CYCLE + 2: case op_CYCLE + 3: case op_CYCLE + 4:
	case op_CYCLE + 5: case op_CYCLE + 6: case op_CYCLE + 7: case op_CYCLE + 8:
	case op_CYCLE + 9: case op_CYCLE +10: case op_CYCLE +11: case op_CYCLE +12:
	case op_CYCLE +13: case op_CYCLE +14: case op_CYCLE +15:
		LOG((0,3,"	CYCLE %#o\n",
			cpu.bus - op_CYCLE));
#if	(DEBUG && 0)
		paused = 1;
		debug_view(1);
#endif
		break;
	case op_BLT:
		LOG((0,3,"	BLT dst:%#o src:%#o size:%#o\n",
			(cpu.r[rsel_ac1] + cpu.r[rsel_ac3] + 1) & 0177777,
			(cpu.r[rsel_ac0] + 1) & 017777,
			-cpu.r[rsel_ac3] & 0177777));
		break;
	case op_BLKS:
		LOG((0,3,"	BLKS dst:%#o val:%#o size:%#o\n",
			(cpu.r[rsel_ac1] + cpu.r[rsel_ac3] + 1) & 0177777,
			cpu.r[rsel_ac0],
			-cpu.r[rsel_ac3] & 0177777));
		break;
	case op_DIAGNOSE1:
		LOG((0,3,"	DIAGNOSE1 AC0:%06o AC1:%06o AC2:%06o AC3:%06o\n",
			cpu.r[rsel_ac0], cpu.r[rsel_ac1],
			cpu.r[rsel_ac2], cpu.r[rsel_ac3]));
		break;
	case op_DIAGNOSE2:
		LOG((0,3,"	DIAGNOSE2 AC0:%06o AC1:%06o AC2:%06o AC3:%06o\n",
			cpu.r[rsel_ac0], cpu.r[rsel_ac1],
			cpu.r[rsel_ac2], cpu.r[rsel_ac3]));
		break;
	case op_BITBLT:
#if	DEBUG
		bitblt_info();
#endif
		break;
	case op_RDRAM:
		LOG((0,3,"	RDRAM addr:%#o\n", cpu.r[rsel_ac1]));
#if	(DEBUG && 0)
		paused = 1;
		debug_view(1);
#endif
		break;
	case op_WRTRAM:
		LOG((0,3,"	WRTAM addr:%#o upper:%06o lower:%06o\n",
			cpu.r[rsel_ac1], cpu.r[rsel_ac0], cpu.r[rsel_ac3]));
		break;
	case op_JMPRAM:
		LOG((0,3,"	JMPRAM addr:%#o\n", cpu.r[rsel_ac1]));
#if	(DEBUG && 0)
		paused = 1;
		debug_view(1);
#endif
		break;
	case op_XMLDA:
		LOG((0,3,"	XMLDA AC0 = [bank:%o AC1:#o]\n",
			cpu.bank_reg[cpu.task] & 3, cpu.r[rsel_ac1]));
		break;
	case op_XMSTA:
		LOG((0,3,"	XMSTA [bank:%o AC1:#o] = AC0 (%#o)\n",
			cpu.bank_reg[cpu.task] & 3, cpu.r[rsel_ac1],
			cpu.r[rsel_ac1]));
#if	(DEBUG && 1)
		paused = 1;
		debug_view(1);
#endif
		break;
	}
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
	ALTO_PUT(cpu.rsel, 5, 3, 3, RA3(f2_emu_acsource, emu.ir, cpu.rsel));
	ALTO_PUT(cpu.rsel, 5, 4, 4, RA4(f2_emu_acsource, emu.ir, cpu.rsel));
#else
	ALTO_PUT(cpu.rsel, 5, 3, 4, IR_SrcAC(emu.ir) ^ 3);
#endif
	LOG((0,2,"	<-ACSOURCE; rsel := SrcAC (%#o %s)\n",
		cpu.rsel, r_name[cpu.rsel]));
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

/**
 * @brief Initialize the emulator task BUS sources, F1, and F2 functions
 */
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
	/* F1 014 is undefined (?) */
	SET_FN(f1, task_14,			NULL,		f1_load_srb_1);
	SET_FN(f1, emu_load_esrb,		NULL,		f1_load_esrb_1);
	SET_FN(f1, emu_rsnf,			f1_rsnf_0,	NULL);
	SET_FN(f1, emu_startf,			f1_startf_0,	NULL);

	SET_FN(f2, emu_busodd,			NULL,		f2_busodd_1);
#if	0
	SET_FN(f2, emu_magic,			NULL,		f2_magic_1);
#else
	SET_FN(f2, emu_magic,			NULL,		NULL);
#endif
	SET_FN(f2, emu_load_dns,		f2_load_dns_0,	f2_load_dns_1);
	SET_FN(f2, emu_acdest,			f2_acdest_0,	NULL);
	SET_FN(f2, emu_load_ir,			NULL,		f2_load_ir_1);
	SET_FN(f2, emu_idisp,			NULL,		f2_idisp_1);
	SET_FN(f2, emu_acsource,		f2_acsource_0,	f2_acsource_1);

	return 0;
}
