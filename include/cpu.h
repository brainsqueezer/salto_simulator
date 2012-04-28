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
 * $Id: cpu.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_CPU_H_INCLUDED_)
#define	_CPU_H_INCLUDED_

#define	SET_FN(type,name,fn_0,fn_1) do { \
	fn_##type[0][task][type##_##name] = fn_0; \
	fn_##type[1][task][type##_##name] = fn_1; \
}	while (0)

#define	CPU_SET_TASK_WAKEUP(task) do { \
	cpu.task_wakeup |= (1 << (task)); \
} while (0)

#define	CPU_CLR_TASK_WAKEUP(task) do { \
	cpu.task_wakeup &= ~(1 << (task)); \
} while (0)

#define	CPU_GET_TASK_WAKEUP(task) ((cpu.task_wakeup >> (task)) & 1)

#define	CPU_SET_ACTIVATE_CB(task, callback) do { \
	cpu.active_callback[task] = callback; \
} while (0)

#define	CPU_BRANCH(value) do { \
	cpu.next2 |= (value); \
} while (0)

/** @brief enumeration of the microcode word bits, left to right */
typedef enum {
	DRSEL0,DRSEL1,DRSEL2,DRSEL3,DRSEL4,
	DALUF0,DALUF1,DALUF2,DALUF3,
	DBS0,DBS1,DBS2,
	DF1_0,DF1_1,DF1_2,DF1_3,
	DF2_0,DF2_1,DF2_2,DF2_3,
	LOADT,LOADL,
	NEXT0,NEXT1,NEXT2,NEXT3,NEXT4,NEXT5,NEXT6,NEXT7,NEXT8,NEXT9
}	mir_bits_t;

/** @brief inverted bits in the microinstruction word */
#define	UCODE_INVERTED	((1<<(31-DF1_0))|(1<<(31-DF2_0))|(1<<(31-LOADL)))

/** @brief get RSELECT bit field from a microinstruction */
#define	MIR_RSEL	ALTO_GET(cpu.mir, 32, DRSEL0, DRSEL4)

/** @brief get ALUF bit field from a microinstruction */
#define	MIR_ALUF	ALTO_GET(cpu.mir, 32, DALUF0, DALUF3)

/** @brief get BUS SOURCE bit field from a microinstruction */
#define	MIR_BS		ALTO_GET(cpu.mir, 32, DBS0, DBS2)

/** @brief get F1 bit field from a microinstruction */
#define	MIR_F1		ALTO_GET(cpu.mir, 32, DF1_0, DF1_3)

/** @brief get F2 bit field from a microinstruction */
#define	MIR_F2		ALTO_GET(cpu.mir, 32, DF2_0, DF2_3)

/** @brief get T flag from a microinstruction */
#define	MIR_T		ALTO_GET(cpu.mir, 32, LOADT, LOADT)

/** @brief get L flag from a microinstruction */
#define	MIR_L		ALTO_GET(cpu.mir, 32, LOADL, LOADL)

/** @brief get NEXT bit field from a microinstruction */
#define	MIR_NEXT	ALTO_GET(cpu.mir, 32, NEXT0, NEXT9)

/** @brief task numbers */
typedef enum {
	/** @brief emulator task */
	task_emu,

	/** @brief unused */
	task_1,

	/** @brief unused */
	task_2,

	/** @brief unused */
	task_3,

	/** @brief disk sector task */
	task_ksec,

	/** @brief unused */
	task_5,

	/** @brief unused */
	task_6,

	/** @brief ethernet task */
	task_ether,

	/** @brief memory refresh task */
	task_mrt,

	/** @brief display word task */
	task_dwt,

	/** @brief cursor task */
	task_curt,

	/** @brief display horizontal task */
	task_dht,

	/** @brief display vertical task */
	task_dvt,

	/** @brief parity task */
	task_part,

	/** @brief disk word task */
	task_kwd,

	/** @brief unused task slot 017 */
	task_17,

	/** @brief number of tasks */
	task_COUNT

}	task_t;

/** @brief register select values accessing R */
typedef enum {
	/**
	 * @brief AC3 used by emulator as accu 3
	 * Also used by Mesa emulator to keep bytecode to execute after breakpoint
	 */
	rsel_ac3,
	/**
	 * @brief AC2 used by emulator as accu 2
	 * Also used by Mesa emulator as x register for xfer
	 */
	rsel_ac2,
	/**
	 * @brief AC1 used by emulator as accu 1
	 * Also used by Mesa emulator as r-temporary for return indices and values
	 */
	rsel_ac1,
	/**
	 * @brief AC0 used by emulator as accu 0
	 * Also used by Mesa emulator as new field bits for WF and friends
	 */
	rsel_ac0,
	/**
	 * @brief NWW state of the interrupt system
	 */
	rsel_r04,
	/**
	 * @brief SAD
	 * Also used by Mesa emulator as scratch R-register for counting
	 */
	rsel_r05,
	/**
	 * @brief PC used by emulator as program counter
	 */
	rsel_pc,
	/**
	 * @brief XREG
	 * Also used by Mesa emulator as task hole,
	 * i.e. pigeonhole for saving things across tasks.
	 */
	rsel_r07,
	/**
	 * @brief XH
	 * Also used by Mesa emulator as instruction byte register
	 */
	rsel_r10,
	/**
	 * @brief CLOCKTEMP - used in the MRT
	 */
	rsel_r11,
	/**
	 * @brief ECNTR remaining words in buffer - ETHERNET
	 */
	rsel_r12,
	/**
	 * @brief EPNTR points BEFORE next word in buffer - ETHERNET
	 */
	rsel_r13,
	rsel_r14,
	/**
	 * @brief MPC
	 * Used by the Mesa emulator as program counter
	 */
	rsel_r15,
	/**
	 * @brief STKP
	 * Used by the Mesa emulator as stack pointer [0-10] 0 empty, 10 full
	 */
	rsel_r16,
	/**
	 * @brief XTSreg
	 * Used by the Mesa emulator to xfer trap state
	 */
	rsel_r17,
	/**
	 * @brief CURX
	 * Holds cursor X; used by the cursor task
	 */
	rsel_r20,
	/**
	 * @brief CURDATA
	 * Holds the cursor data; used by the cursor task
	 */
	rsel_r21,
	/**
	 * @brief CBA
	 * Holds the address of the currently active DCB+1
	 */
	rsel_r22,
	/**
	 * @brief AECL
	 * Holds the address of the end of the current scanline's bitmap
	 */
	rsel_r23,
	/**
	 * @brief SLC
	 * Holds the number of scanlines remaining in currently active DCB
	 */
	rsel_r24,
	/**
	 * @brief MTEMP
	 * Holds the temporary cell
	 */
	rsel_r25,
	/**
	 * @brief HTAB
	 * Holds the number of tab words remaining on current scanline
	 */
	rsel_r26,
	/**
	 * @brief YPOS
	 */
	rsel_r27,
	/**
	 * @brief DWA
	 * Holds the address of the bit map doubleword currently being fetched
	 * for transmission to the hardware buffer.
	 */
	rsel_r30,
	/**
	 * @brief KWDCT
	 * Used by the disk tasks as word counter
	 */
	rsel_r31,
	/**
	 * @brief CKSUMR
	 * Used by the disk tasks as checksum register (and *amble counter?)
	 */
	rsel_r32,
	/**
	 * @brief KNMAR
	 * Used by the disk tasks as transfer memory address register
	 */
	rsel_r33,
	/**
	 * @brief DCBR
	 * Used by the disk tasks to keep the current device control block
	 */
	rsel_r34,
	/**
	 * @brief TEMP
	 * Used by the Mesa emulator, and also by BITBLT
	 */
	rsel_r35,
	/**
	 * @brief TEMP2
	 * Used by the Mesa emulator, and also by BITBLT
	 */
	rsel_r36,
	/**
	 * @brief CLOCKREG low order bits of the real time clock
	 */
	rsel_r37,
	/** @brief number of R registers */
	rsel_COUNT
}	rsel_t;

/** @brief ALU function numbers */
typedef enum {
	/**
	 * @brief 00: ALU <- BUS
	 * PROM data for S3-0,M,C: 1111/1/0
	 * function F=A
	 * T source is ALU
	 */
	aluf_bus__alut,
	/**
	 * @brief 01: ALU <- T
	 * PROM data for S3-0,M,C: 1010/1/0
	 * function F=B
	 * T source is BUS
	 */
	aluf_treg,
	/**
	 * @brief 02: ALU <- BUS | T
	 * PROM data for S3-0,M,C: 1110/1/0
	 * function F=A|B
	 * T source is ALU
	 */
	aluf_bus_or_t__alut,
	/**
	 * @brief 03: ALU <- BUS & T
	 * PROM data for S3-0,M,C: 1011/1/0
	 * function F=A&B
	 * T source is BUS
	 */
	aluf_bus_and_t,
	/**
	 * @brief 04: ALU <- BUS ^ T
	 * PROM data for S3-0,M,C: 0110/1/0
	 * function F=A^B
	 * T source is BUS
	 */
	aluf_bus_xor_t,
	/**
	 * @brief 05: ALU <- BUS + 1
	 * PROM data for S3-0,M,C: 0000/0/0
	 * function F=A+1
	 * T source is ALU
	 */
	aluf_bus_plus_1__alut,
	/**
	 * @brief 06: ALU <- BUS - 1
	 * PROM data for S3-0,M,C: 1111/0/1
	 * function F=A-1
	 * T source is ALU
	 */
	aluf_bus_minus_1__alut,
	/**
	 * @brief 07: ALU <- BUS + T
	 * PROM data for S3-0,M,C: 1001/0/1
	 * function F=A+B
	 * T source is BUS
	 */
	aluf_bus_plus_t,
	/**
	 * @brief 10: ALU <- BUS - T
	 * PROM data for S3-0,M,C: 0110/0/0
	 * function F=A-B
	 * T source is BUS
	 */
	aluf_bus_minus_t,
	/**
	 * @brief 11: ALU <- BUS - T - 1
	 * PROM data for S3-0,M,C: 0110/0/1
	 * function F=A-B-1
	 * T source is BUS
	 */
	aluf_bus_minus_t_minus_1,
	/**
	 * @brief 12: ALU <- BUS + T + 1
	 * PROM data for S3-0,M,C: 1001/0/0
	 * function F=A+B+1
	 * T source is ALU
	 */
	aluf_bus_plus_t_plus_1__alut,
	/**
	 * @brief 13: ALU <- BUS + SKIP
	 * PROM data for S3-0,M,C: 0000/0/SKIP
	 * function F=A (SKIP=1) or F=A+1 (SKIP=0)
	 * T source is ALU
	 */
	aluf_bus_plus_skip__alut,
	/**
	 * @brief 14: ALU <- BUS & T
	 * PROM data for S3-0,M,C: 1011/1/0
	 * function F=A&B
	 * T source is ALU
	 */
	aluf_bus_and_t__alut,
	/**
	 * @brief 15: ALU <- BUS & ~T
	 * PROM data for S3-0,M,C: 0111/1/0
	 * function F=A&~B
	 * T source is BUS
	 */
	aluf_bus_and_not_t,
	/**
	 * @brief 16: ALU <- ???
	 * PROM data for S3-0,M,C: ????/?/?
	 * perhaps F=0 (0011/0/0)
	 * T source is BUS
	 */
	aluf_undef_16,
	/**
	 * @brief 17: ALU <- ???
	 * PROM data for S3-0,M,C: ????/?/?
	 * perhaps F=0 (0011/0/0)
	 * T source is BUS
	 */
	aluf_undef_17,
	/** @brief number of ALU functions */
	aluf_COUNT
}	aluf_t;

/** @brief BUS select numbers */
typedef enum {
	/** @brief BUS source is R register */
	bs_read_r,

	/** @brief load R register from BUS */
	bs_load_r,

	/** @brief BUS is open (0177777) */
	bs_no_source,

	/** @brief BUS source is task specific */
	bs_task_3,

	/** @brief BUS source is task specific */
	bs_task_4,

	/** @brief BUS source is memory data */
	bs_read_md,

	/** @brief BUS source is mouse data */
	bs_mouse,

	/** @brief BUS source displacement */
	bs_disp,

	/** @brief number of BUS sources */
	bs_COUNT

}	bs_t;

/** @brief function 1 numbers */
typedef enum {
	/** @brief F1 no operation */
	f1_nop,

	/** @brief F1 load memory address register */
	f1_load_mar,

	/** @brief F1 task switch */
	f1_task,

	/** @brief F1 block task */
	f1_block,

	/** @brief F1 left shift L once */
	f1_l_lsh_1,

	/** @brief F1 right shift L once */
	f1_l_rsh_1,

	/** @brief F1 cycle L 8 times */
	f1_l_lcy_8,

	/** @brief F1 constant */
	f1_const,

	/** @brief F1 10 task specific */
	f1_task_10,

	/** @brief F1 11 task specific */
	f1_task_11,

	/** @brief F1 12 task specific */
	f1_task_12,

	/** @brief F1 13 task specific */
	f1_task_13,

	/** @brief F1 14 task specific */
	f1_task_14,

	/** @brief F1 15 task specific */
	f1_task_15,

	/** @brief F1 16 task specific */
	f1_task_16,

	/** @brief F1 17 task specific */
	f1_task_17,

	/** @brief number of F1 functions */
	f1_COUNT

}	f1_t;

/** @brief function 2 numbers */
typedef enum {
	/** @brief F2 no operation */
	f2_nop,

	/** @brief F2 branch on bus equals 0 */
	f2_bus_eq_zero,

	/** @brief F2 branch on shifter less than 0 */
	f2_shifter_lt_zero,

	/** @brief F2 branch on shifter equals 0 */
	f2_shifter_eq_zero,

	/** @brief F2 branch on BUS[6-15] */
	f2_bus,

	/** @brief F2 branch on (latched) ALU carry */
	f2_alucy,

	/** @brief F2 load memory data */
	f2_load_md,

	/** @brief F2 constant */
	f2_const,

	/** @brief F2 10 task specific */
	f2_task_10,

	/** @brief F2 11 task specific */
	f2_task_11,

	/** @brief F2 12 task specific */
	f2_task_12,

	/** @brief F2 13 task specific */
	f2_task_13,

	/** @brief F2 14 task specific */
	f2_task_14,

	/** @brief F2 15 task specific */
	f2_task_15,

	/** @brief F2 16 task specific */
	f2_task_16,

	/** @brief F2 17 task specific */
	f2_task_17,

	/** @brief number of F2 functions */
	f2_COUNT

}	f2_t;

typedef enum {
	/** @brief dynamic functions (e.g. ALU and SHIFTER operations) */
	p_dynamic,
	/** @brief latching function (T, L, M, R, etc. register latch) */
	p_latches,
	/** @brief number of CPU cycle phases */
	p_COUNT
}	phase_t;

/** @brief Structure of the CPU context */
typedef struct {
	/** @brief per task micro program counter */
	int task_mpc[task_COUNT];

	/** @brief per task address modifier */
	int task_next2[task_COUNT];

	/** @brief per task nano seconds executed */
	ntime_t task_ntime[task_COUNT];

	/** @brief currently active task */
	task_t task;

	/** @brief next microinstruction's task */
	task_t next_task;

	/** @brief next but one microinstruon's task */
	task_t next2_task;

	/** @brief current micro program counter */
	int mpc;

	/** @brief current micro instruction */
	uint32_t mir;

	/**
	 * @brief current micro instruction's register selection
	 * The emulator F2s ACSOURCE and ACDEST modify this.
	 * Note: S registers are addressed by the original RSEL[0-4],
	 * even when the the emulator modifies this.
	 */
	uint32_t rsel;

	/** @brief current microinstruction's next */
	int next;

	/** @brief next microinstruction's next */
	int next2;

	/** @brief R register file */
	int r[rsel_COUNT];

	/** @brief S register file(s) */
	int s[SREG_BANKS][rsel_COUNT];

	/** @brief wired-AND bus */
	int bus;

	/** @brief T register */
	int t;

	/** @brief the current ALU */
	int alu;

	/** @brief the current ALU carry output */
	int aluc0;

	/** @brief L register */
	int l;

	/** @brief shifter output */
	int shifter;

	/** @brief the latched ALU carry output */
	int laluc0;

	/** @brief M register of RAM related tasks (MYL latch in the schematics) */
	int m;

	/** @brief constant RAM address */
	int cram_addr;

	/** @brief task wakeup: bit 1<<n set if task n requesting service */
	int task_wakeup;

	/** @brief task activation callbacks */
	void (*active_callback[task_COUNT])(void);

	/** @brief reset mode register: bit 1<<n set if task n starts in ROM */
	int reset_mode;

	/** @brief set by rdram, action happens on next cycle */
	int rdram_flag;

	/** @brief set by wrtram, action happens on next cycle */
	int wrtram_flag;

	/** @brief active S register bank per task */
	int s_reg_bank[task_COUNT];

	/** @brief normal and extended RAM banks per task */
	int bank_reg[task_COUNT];

	/** @brief set by Ether task when it want's a wakeup at switch to task_mrt */
	int ewfct;

	/** @brief display_state_machine() time time accu */
	int dsp_time;

	/** @brief display_state_machine() previous state */
	int dsp_state;

	/** @brief unload word time accu */
	int unload_time;

	/** @brief unload word number */
	int unload_word;
}	cpu_t;

/** @brief human readable task names */
const char *task_name[task_COUNT];

/** @brief register names */
const char *r_name[rsel_COUNT];

/** @brief human readable ALU function names */
const char *aluf_name[aluf_COUNT];

/** @brief human readable bus source names */
const char *bs_name[bs_COUNT];

/** @brief human readable F1 function names */
const char *f1_name[f1_COUNT];

/** @brief human readable F2 function names */
const char *f2_name[f2_COUNT];

/** @brief raw microcode words, decoded */
extern uint32_t ucode_raw[UCODE_SIZE];

/** @brief constant PROM, decoded */
extern uint32_t const_prom[CONST_SIZE];

/** @brief 2k control PROM u3 - 256x4 */
extern uint8_t ctl2k_u3[256];

/** @brief 2k control PROM u38 - 32x8 */
extern uint8_t ctl2k_u38[32];

/** @brief 2k control PROM u76 - 256x4 */
extern uint8_t ctl2k_u76[256];

/** @brief 3k CRAM PROM a37 - 256x4 */
extern uint8_t cram3k_a37[256];

/** @brief memory address PROM a64 - 256x4 */
extern uint8_t madr_a64[256];

/** @brief memory address PROM a65 - 256x4 */
extern uint8_t madr_a65[256];

/** @brief per task bus source function pointers, phases 0 and 1 */
extern void (*fn_bs[p_COUNT][task_COUNT][bs_COUNT])(void);

/** @brief per task f1 function pointers, phases 0 and 1 */
extern void (*fn_f1[p_COUNT][task_COUNT][f1_COUNT])(void);

/** @brief per task f2 function pointers, phases 0 and 1 */
extern void (*fn_f2[p_COUNT][task_COUNT][f2_COUNT])(void);

/** @brief set when task is RAM related */
extern char ram_related[task_COUNT];

/** @brief the current CPU context */
extern cpu_t cpu;

/** @brief number of nanoseconds left to execute in the current slice */
extern ntime_t alto_ntime;

/** @brief flag set by timer.c if alto_execute() shall leave its loop */
extern int alto_leave;

/** @brief reset the various registers */
extern int alto_reset(void);

/** @brief soft reset */
extern int alto_soft_reset(void);

/** @brief execute microcode for a number of nanosecs */
extern ntime_t alto_execute(ntime_t nsecs);

#endif	/* #!defined(_CPU_H_INCLUDED_) */
