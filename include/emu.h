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
 * Bus source, F1, and F2 handling for emulator task
 *
 * $Id: emu.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_EMU_H_INCLUDED_)
#define	_EMU_H_INCLUDED_

/** @brief bus source emu: read S register */
#define	bs_emu_read_sreg	bs_task_3

/** @brief bus source emu: load S register */
#define	bs_emu_load_sreg	bs_task_4

/** @brief f1 emu (1000): switch mode; branch to ROM/RAM microcode */
#define	f1_emu_swmode		f1_task_10

/** @brief f1 emu (1001): write microcode RAM */
#define	f1_emu_wrtram		f1_task_11

/** @brief f1 emu (1010): read microcode RAM */
#define	f1_emu_rdram		f1_task_12

/** @brief f1 emu (1011): load reset mode register */
#define	f1_emu_load_rmr		f1_task_13

/** @brief f1 emu (1100): undefined */

/** @brief f1 emu (1101): load extended S register bank */
#define	f1_emu_load_esrb	f1_task_15

/** @brief f1 emu (1110): read serial number (Ethernet ID) */
#define	f1_emu_rsnf		f1_task_16

/** @brief f1 emu (1111): start I/O hardware (Ethernet) */
#define	f1_emu_startf		f1_task_17

/** @brief f2 emu (1000): branch on bus odd */
#define	f2_emu_busodd		f2_task_10

/** @brief f2 emu (1001): magic shifter (MRSH 1: shifter[15]=T[0], MLSH 1: shifter[015]) */
#define	f2_emu_magic		f2_task_11

/** @brief f2 emu (1010): do novel shift (RSH 1: shifter[15]=XC, LSH 1: shifer[0]=XC) */
#define	f2_emu_load_dns		f2_task_12

/** @brief f2 emu (1011): destination accu */
#define	f2_emu_acdest		f2_task_13

/** @brief f2 emu (1100): load instruction register and branch */
#define	f2_emu_load_ir		f2_task_14

/** @brief f2 emu (1101): load instruction displacement and branch */
#define	f2_emu_idisp		f2_task_15

/** @brief f2 emu (1110): source accu */
#define	f2_emu_acsource		f2_task_16

/* there is no f2_task_17 (1111) */

/* width,from,to of the 16 bit instruction register */
#define	IR_ARITH(ir)	ALTO_GET(ir,16, 0, 0)
#define	IR_SrcAC(ir)	ALTO_GET(ir,16, 1, 2)
#define	IR_DstAC(ir)	ALTO_GET(ir,16, 3, 4)
#define	IR_AFunc(ir)	ALTO_GET(ir,16, 5, 7)
#define	IR_SH(ir)	ALTO_GET(ir,16, 8, 9)
#define	IR_CY(ir)	ALTO_GET(ir,16,10,11)
#define	IR_NL(ir)	ALTO_GET(ir,16,12,12)
#define	IR_SK(ir)	ALTO_GET(ir,16,13,15)

#define	IR_MFunc(ir)	ALTO_GET(ir,16, 1, 2)
#define	IR_JFunc(ir)	ALTO_GET(ir,16, 3, 4)
#define	IR_I(ir)	ALTO_GET(ir,16, 5, 5)
#define	IR_X(ir)	ALTO_GET(ir,16, 6, 7)
#define	IR_DISP(ir)	ALTO_GET(ir,16, 8,15)
#define	IR_AUGFUNC(ir)	ALTO_GET(ir,16, 3, 7)

/** @brief instruction register memory function mask */
#define	op_MFUNC_MASK	0060000

/** @brief jump functions value */
#define	op_MFUNC_JUMP	0000000

/** @brief jump functions mask */
#define	op_JUMP_MASK	0014000

/** @brief jump */
#define	op_JMP		0000000

/** @brief jump to subroutine */
#define	op_JSR		0004000

/** @brief increment and skip if zero */
#define	op_ISZ		0010000

/** @brief decrement and skip if zero */
#define	op_DSZ		0014000

/** @brief load accu functions value */
#define	op_LDA		0020000

/** @brief store accu functions value */
#define	op_STA		0040000

/** @brief store accu functions value */
#define	op_AUGMENTED	0060000

/** @brief mask covering all augmented functions */
#define	op_AUGM_MASK	0077400

/** @brief augmented functions w/o displacement */
#define	op_AUGM_NODISP	0061000

/** @brief mask for augmented subfunctions in DISP */
#define	op_AUGM_SUBFUNC	0000037

/** @brief cycle AC0 */
#define	op_CYCLE	0060000

/** @brief NODISP: opcodes without displacement */
#define	op_NODISP	0061000

/** @brief disable interrupts */
#define	op_DIR		0061000

/** @brief enable interrupts */
#define	op_EIR		0061001

/** @brief branch and return from interrupt */
#define	op_BRI		0061002

/** @brief read clock to AC0, AC1 */
#define	op_RCLK		0061003

/** @brief start I/O */
#define	op_SIO		0061004

/** @brief block transfer */
#define	op_BLT		0061005

/** @brief block set value */
#define	op_BLKS		0061006

/** @brief start interval timer */
#define	op_SIT		0061007

/** @brief jump to microcode RAM (actually ROM, too) */
#define	op_JMPRAM	0061010

/** @brief read microcode RAM */
#define	op_RDRAM	0061011

/** @brief write microcode RAM */
#define	op_WRTRAM	0061012

/** @brief disable interrupts, and skip, if already disabled */
#define	op_DIRS		0061013

/** @brief get microcode version in AC0 */
#define	op_VERS		0061014

/** @brief double word read (Alto II) */
#define	op_DREAD	0061015

/** @brief double word write (Alto II) */
#define	op_DWRITE	0061016

/** @brief double word exchange (Alto II) */
#define	op_DEXCH	0061017

/** @brief unsigned multiply */
#define	op_MUL		0061020

/** @brief unsigned divide */
#define	op_DIV		0061021

/** @brief write two different accus in fast succession */
#define	op_DIAGNOSE1	0061022

/** @brief write Hamming code and memory */
#define	op_DIAGNOSE2	0061023

/** @brief bit-aligned block transfer */
#define	op_BITBLT	0061024

/** @brief load accu AC0 from extended memory (Alto II/XM) */
#define	op_XMLDA	0061025

/** @brief store accu AC0 from extended memory (Alto II/XM) */
#define	op_XMSTA	0061026

/** @brief jump to subroutine PC relative, doubly indirect */
#define	op_JSRII	0064400

/** @brief jump to subroutine AC2 relative, doubly indirect */
#define	op_JSRIS	0065000

/** @brief convert bitmapped font to bitmap */
#define	op_CONVERT	0067000

/** @brief mask for arithmetic functions */
#define	op_ARITH_MASK	0103400

/** @brief one's complement */
#define	op_COM		0100000
/** @brief two's complement */
#define	op_NEG		0100400
/** @brief accu transfer */
#define	op_MOV		0101000
/** @brief increment */
#define	op_INC		0101400
/** @brief add one's complement */
#define	op_ADC		0102000
/** @brief subtract by adding two's complement */
#define	op_SUB		0102400
/** @brief add */
#define	op_ADD		0103000
/** @brief logical and */
#define	op_AND		0103400

/** @brief effective address is direct */
#define	ea_DIRECT	0000000
/** @brief effective address is indirect */
#define	ea_INDIRECT	0002000

/** @brief mask for effective address modes */
#define	ea_MASK		0001400
/** @brief e is page 0 address */
#define	ea_PAGE0	0000000
/** @brief e is PC + signed displacement */
#define	ea_PCREL	0000400
/** @brief e is AC2 + signed displacement */
#define	ea_AC2REL	0001000
/** @brief e is AC3 + signed displacement */
#define	ea_AC3REL	0001400

/** @brief shift mode mask (do novel shifts) */
#define	sh_MASK		0000300
/** @brief rotate left through carry */
#define	sh_L		0000100
/** @brief rotate right through carry */
#define	sh_R		0000200
/** @brief swap byte halves */
#define	sh_S		0000300

/** @brief carry in mode mask */
#define	cy_MASK		0000060
/** @brief carry in is zero */
#define	cy_Z		0000020
/** @brief carry in is one */
#define	cy_O		0000040
/** @brief carry in is complemented carry */
#define	cy_C		0000060

/** @brief no-load mask */
#define	nl_MASK		0000010
/** @brief do not load DstAC nor carry */
#define	nl_		0000010

/** @brief skip mask */
#define	sk_MASK		0000007
/** @brief never skip */
#define	sk_NVR		0000000
/** @brief always skip */
#define	sk_SKP		0000001
/** @brief skip if carry result is zero */
#define	sk_SZC		0000002
/** @brief skip if carry result is non-zero */
#define	sk_SNC		0000003
/** @brief skip if 16-bit result is zero */
#define	sk_SZR		0000004
/** @brief skip if 16-bit result is non-zero */
#define	sk_SNR		0000005
/** @brief skip if either result is zero */
#define	sk_SEZ		0000006
/** @brief skip if both results are non-zero */
#define	sk_SBN		0000007

typedef struct {
	/** @brief emulator instruction register */
	int ir;

	/** @brief emulator skip */
	int skip;

	/** @brief emulator carry */
	int cy;

}	emu_t;

/** @brief emulator context */
extern emu_t emu;

/** @brief emulator task */
extern int init_emu(int task);

#endif	/* !defined(_EMU_H_INCLUDED_) */
