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
 * Disk I/O functions
 *
 * $Id: disk.new.c,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "disk.h"
#include "drive.h"

/** @brief 1 to debug the JK flip-flops, 0 to use a lookup table */
#define	RECNO_HEADER	0
#define	RECNO_PAGENO	1
#define	RECNO_LABEL	2
#define	RECNO_DATA	3

/**
 *
 * Just for completeness' sake:
 * The mapping of disk controller connector P2 pins to the
 * Winchester disk drive signals (see drive.h)
 *
 * Alto Controller     Winchester
 * P2 signal           disk bus
 * -----------------------------------------------
 *  1 GND              D_GROUND
 *  2 RDCLK'           A_READ_CLOCK
 *  3 WRDATA'          B_WRITE_DATA_AND_CLOCK
 *  4 SRWRDY'          F_S_R_W
 *  5 DISK             L_SELECT_LINE_UNIT_1
 *  6 CYL(7)'          N_CYL_7
 *  7 DISK'            R_SELECT_LINE_UNIT_2
 *  8 CYL(2)'          T_CYL_2
 *  9 ???              V_SELECT_LINE_UNIT_3
 * 10 CYL(4)'          X_CYL_4
 * 11 CYL(0)'          Z_CYL_0
 * 12 CYL(1)'          BB_CYL_1
 * 13 CYL(3)'          FF_CYL_3
 * 14 ???              KK_BIT_2
 * 15 CYL(8)'          LL_CYL_8
 * 16 ADRACK'          NN_ADDX_ACKNOWLEDGE
 * 17 SKINC'           TT_SEEK_INCOMPLETE
 * 18 LAI'             XX_LOG_ADDX_INTERLOCK
 * 19 CYL(6)'          RR_CYL_6
 * 20 RESTOR'          VV_RESTORE
 * 21 ???              UU_BIT_16
 * 22 STROBE'          SS_STROBE
 * 23 ???              MM_BIT_8
 * 24 ???              KK_BIT_4
 * 25 ???              HH_WRITE_CHK
 * 26 WRTGATE'         EE_WRITE_GATE
 * 27 ???              CC_BIT_SECTOR_ADDX
 * 28 HEAD'            AA_HEAD_SELECT
 * 29 ???              Y_INDEX_MARK
 * 30 SECT(4)'         W_SECTOR_MARK
 * 31 READY'           U_FILE_READY
 * 32 ???              S_PSEUDO_SECTOR_MARK
 * 33 ???              P_WRITE_PROTECT_IND
 * 34 ???              H_WRITE_PROTECT_INPUT_ATTENTION
 * 35 ERGATE'          K_ERASE_GATE
 * 36 ???              M_HIGH_DENSITY
 * 37 CYL(5)'          J_CYL_5
 * 38 RDDATA'          C_READ_DATA
 * 39 RDGATE'          E_READ_GATE
 * 40 GND              ??
 */

/**
 * @brief structure of the disk controller context
 */
typedef struct {
	/** @brief selected drive from KADDR[14] (written to data out with SENDADR) */
	int	drive;

	/** @brief A[0-15] disk hardware address (sector, cylinder, head, drive, restore) */
	int	kaddr;

	/** @brief C[0-15] with read/write/check modes for header, label and data */
	int	kadr;

	/** @brief S[0-15] disk status */
	int	kstat;

	/** @brief input shift register */
	int	shiftreg_in;

	/** @brief output shift register */
	int	shiftreg_out;

	/** @brief disk data in latch */
	int	data_in;

	/** @brief disk data out */
	int	data_out;

	/** @brief disk command (5 bits kcom[1-5]) */
	int	kcom;

	/** @brief record number (2 bits indexing noxfer/disk, header, label, data) */
	int	krecno;

	/** @brief read/write/check for current record */
	int	krwc;

	/** @brief disk fatal error signal state */
	int	kfer;

	/** @brief drive ready latched at CLRSTAT */
	int	rdylat;

	/** @brief sequence error latched at CLRSTAT */
	int	seqerr;

	/** @brief checksum error latch; cleared at CLRSTAT, set in KCOM<- */
	int	chsmerror;

	/** @brief disk word task enable */
	int	wdtskena;

	/** @brief disk word task init */
	int	wdinit;

	/** @brief disk sector task enable */
	int	stskena;

	/** @brief recent sector mark state */
	int	sect4;

	/** @brief strobe (still) active */
	int	strobe;

	/** @brief bit counter */
	int	bitcount;

	/** @brief current bitclk state */
	int	bitclk;

	/** @brief current input datin value */
	int	datin;

	/** @brief carry output of the bitcounter */
	int	carry;

	/** @brief sector late */
	int	seclate;

	/** @brief sector late timer id */
	int	seclate_id;

	/** @brief seekok state (SKINC' & LAI' & ff_44a.Q') */
	int	seekok;

	/** @brief ok to run signal (set to 1 some time after reset ?) */
	int	ok_to_run;

	/** @brief ready monoflop 31a */
	int	ready_mf31a;

}	disk_t;

/** @brief disk context */
static disk_t disk;

static void disk_seclate(int id, int arg);
static void disk_ok_to_run(int id, int arg);
static void disk_strobon(int id, int arg);

#if	DEBUG
/** @brief human readable names for the KADR<- modes */
static const char *rwc_name[4] = {"read", "check", "write", "write3"};
#endif

/**
 * SECTOR, ERROR WAKEUPS
 *
 *
 * Monoflop pulse duration:
 * tW = K * Rt * Cext * (1 + 0.7/Rt)
 * K = 0.28 for 74123
 * Rt = kOhms
 * Cext = pF
 *
 *                     +------+
 *  CLRSTAT' >---------oS'    | 15k, .47µF (=470000pF)
 *                     | MONO | 2066120ns ~= 2ms
 *                     | FLOP |
 *                     |      | Q'     +----+
 *  READY'   >---------oC'    o--------|NAND|    ERRWAKE'
 *                     +------+        |    o----.
 *  RDYLAT'  >-------------------------|    |    |
 *                                     +----+    |
 *                                               |
 *                                               |
 *                         .---------------------´
 *                         |
 *                     +---o--+ Q            +------+
 *               .-----|J  S' |----+---------|S     | 30k, .01µF (=10000pF)
 *               |     |      |    |         | MONO | 8596ns ~= 8.5µs
 *  SECT[4]' >---|-----|CLK   |    |         | FLOP |
 *               |     |   21a|    |         |      | Q'
 *               | 1 >-|K' C' |    |     1 >-|C'    |--------------------> SECLATE
 *               |     +---o--+    |         +------+
 *               |         |       |
 *               `---------+-------|-----------------------------------.
 *                                 |                                   |
 *                 .---------------´                                   |
 *                 |                                                   |
 *                 |       1                 1   RESET' >------.       |
 *                 |       |                 |                 |       |
 *                 |   +---o--+ Q        +---o--+ Q        +---o--+ Q  |
 *                 `---|J  S' |----------|J  S' |----------|J  S' |------> STSKENA
 *                     |      |          |      |          |      |    |
 *  SYSCLKB' >--+------|CLK   |  .-------|CLK   |  .-------|CLK   |    |
 *              |      |   21b|  |       |   22a|  |       |   22b| Q' |
 *              |  1 >-|K' C' |  |   1 >-|K' C' |  |   .---|K' C' |----+-> WAKEST'
 *              |      +---o--+  |       +---o--+  |   |   +---o--+    |
 *              |          |     |           |     |   |       1       |
 *              |          `-----|-----------+-----|---|---------------´
 *              |                |                 |   |
 *              `----------------+-----------------´   |
 *                                                     |
 *                                     +----+          |
 *   BLOCK   >-------------------------|NAND|          |
 *                                     |    o----------´
 *  STSKACT  >-------------------------|    |
 *                                     +----+
 *
 * A CLRSTAT starts the monoflop, and READY', i.e. the ready signal from the disk
 * drive, clears it. The Q' output is thus 0 for some time after CLRSTAT, and as
 * long as the disk signals being ready.
 *
 * If the disk is not ready, i.e. the Q' being 1, and if RDYLAT' - the READY' state
 * latched at the most recent CLRSTAT - is also 1, the ERRWAKE' signal will go 0.
 *
 * Each new sector (SECT[4]' going 1) will clock the FF 21a, which changes
 * its Q output depending on WAKEST' (K' is always 1):
 *   if J and K' are both 1, sets its Q to 1.
 *   if J is 0, and K' is 1, keeps Q as is.
 * So Q becomes 0 by WAKEST' going 0, and it becomes 1 with the next sector, if
 * WAKEST' is 1.
 *
 * The mono-flop to the right will generate a SECLATE signal, if WAKEST' was
 * not 0 when the disk signalled a new sector.
 *
 * The three J-K FFs at the bottom are all clocked with the rising edge of
 * SYSCLKB' (i.e falling edged of SYSCLKB).
 *
 * The left JK-FF propagates the current state of the upper JK-FF's Q output
 * to its own Q. The middle propagates the previous state of the left one,
 * and the JK-FF to the right delays the wandering Q for a third SYSCLKB'
 * rising edge, but only in one case:
 * 1)  if J and K' are both 1, set its Q to 1.
 * 2)  if J is 1, and K' is 0, toggle Q.
 * 3)  if J is 0, and K' is 1, keep Q as is.
 * 4)  if J and K' are both 0, set its Q to 0.
 *
 * The right FF's K' is 0 whenever the BLOCK signal (see DISK WORD TIMING)
 * and the sector task active signal (STSKACT) are 1 at the same time.
 *
 * Case 1) is the normal case, and it wakes the KSECT on the third SYSCLKB'
 * positive edge. It resets at that same time the left, middle, and upper
 * J-K FFs .
 *
 * Case 2) is due, when the sector task is already active the moment
 * the BLOCK signal arrives. This toggles the output, i.e. removes the
 * wake.
 *
 * Case 3) is for an active sector task without a new sector.
 *
 * And finally case 4) happens when an active sector task sees no new
 * sector, and BLOCK rises.
 *
 * (This is like the video timing's dwt_blocks and dht_blocks signals)
 */
#define	TW_READY	((ntime_t)2066120)
#define	TW_SECLATE	((ntime_t)8596)
#define	TW_STROBON	((ntime_t)8596)

/**
 * DISK WORD TIMING
 *
 *
 *                       SECLATE ----+  +-+-+-+---< 1
 *                                   |  | | | |
 *                                +--o-----------+ CARRY +---+
 *                                | CLR 1 2 4 8  |-------|INVo-----> WDDONE'
 *                +---+  BITCLK'  |              |       +---+
 *    BITCLK >----|INVo----+------|CLK/   74161  |
 *                +---+    |      +----o---------+
 *                         |           |LOAD'
 *              .----------´           |
 *              |  +----+              |
 *              `--|NAND|              |
 *                 |    o----.         |
 *  HIORDBIT >-----|    |    |         |
 *                 +----+    |         |
 *                           |         |
 *                       +---o--+ Q    |
 *    BUS[4] >--------+--|J  S' |------´
 *                    |  |      |                               +----+
 *    LDCOM' >--------|--|CLK   |                        .------|NAND|
 *                    |  |   67b|                        |      |    o----> WAKEWDT'
 *                    +--|K' C' |                        |   .--|    |
 *                       +---o--+                        |   |  +----+
 *                           |                           |   |
 *                           1                           |    `---------.
 *                                                       |              |
 * OK TO RUN >---------------.                 1         |       1      |
 * (1 in AltoI)              |                 |         |       |      |
 *                       +---o--+ Q        +---o--+ Q    |   +---o--+ Q |
 *                   1 >-|J  S' |----------|J  S' |------+---|J  S' |---´
 *                       |      |          |      |      |   |      |
 *   WDDONE' >-----------|CLK   |  .-------|CLK   |   .--|---|CLK   |
 *                       |   43b|  |       |   53a|   |  |   |   43a|
 *                   1 >-|K' C' |  | .-----|K' C' |   |  `---|K' C' o---.
 *                       +---o--+  | |     +---o--+   |      +---o--+ Q'|
 *                           |     | |         |      |          |      |
 *                           `-----|-|---------|------|----------|------´
 *                                 | |         |      |          |
 *  SYSCLKA' >---------------------|-|---------|------´          |
 *                                 | |         |                 |
 *  WDALLOW  >---------------------|-|---------+-----------------´
 *                                 | |         |
 *  SYSCLKB' >---------------------+ |     +---o--+ Q
 *                                 | | 0 >-|J  S' |-------------> WDINIT
 *                                 | |     |      |
 *              +----+             `-|-----|CLK   |
 *     BLOCK >--|NAND|               |     |   53b|
 *              |    o---------------+-----|K  C' |
 *  WDTSKACT >--|    |                     +---o--+
 *              +----+                         |
 *                                             1
 *
 *
 * If SECLATE is 0, WDDONE' never goes low (counter's clear has precedence).
 *
 * If SECLATE is 1, WDDONE', the counter will count:
 *
 * If HIORDBIT is 1 at the falling edge of BITCLK, it sets the J-K flip-flop 67b, and
 * thus takes away the LOAD' assertion from the counter. It has been loaded with
 * 15, so it counts to 16 on the next rising edge and makes WDDONE' go to 0.
 *
 * If HIORDBIT is 0 at the falling edge of BITCLK, counting continues as it was
 * preset with BUS[4] at the last KCOM<- load:
 *
 * If BUS[4] was 1, both J and K' of the FF (74109) will be 1 at the rising edge
 * of LDCOM' (at the end of KCOM<-) and Q will be 1 => LOAD' is deassterted.
 *
 * If BUS[4] was 0, both J and K' will be 0, and Q will be 0 => LOAD' asserted.
 *
 * WDDONE' going from 0 to 1 will make the Q output of FF 43b go to 1.
 * The FF is also set, as long as OK TO RUN is 0.
 *
 * The FF 53a is clocked with falling edge of SYSCLKB (rising of SYSCLKB'),
 * and will:
 *   if J and K' are both 1, set its Q to 1.
 *   if J is 1, and K' is 0, toggle Q.
 *   if J is 0, and K' is 1, keep Q as is.
 *   if J and K' are both 0, set its Q to 0.
 * J is = Q of the FF 43b.
 * K is = 0, if both BLOCK and WDTASKACT are 1, and 1 otherwise.
 *
 * The FF 43a is clocked with falling edge of SYSCLKA (rising of SYSCLKA').
 * Its J and K' inputs are the Q output of the previous (middle) FF, thus
 * it will propagate the middle FF's Q to its own Q when SYSCLKA goes 0.
 *
 * If Q (53a) and Q (43a) are both 1, the WAKEKWDT' is 0 and the
 * word task wakeup signal is sent.
 *
 * WDALLOW going 0 asynchronously resets the 53a and 43a FFs, and thus
 * deasserts the WAKEKWD'. It also asynchronously sets the FF 53b, and
 * its output Q is the WDINIT signal.
 *
 * WDINIT is also deasserted with SYSCLKB going high, whenever both BLOCK
 * and WDTSKACT are 1.
 *
 * Whoa there! :-)
 */
/**
 * @brief disk word timing
 *
 * Implements the flip flops and gates in the description above.
 *
 * @param bitclk the current bit clock level
 * @param datin the level of the bit read from the disk
 * @param block task number of a blocking task, or zero
 */
static void kwd_timing(int bitclk, int datin, int block)
{
	int sect4 = drive_sector_mark_0(disk.drive);
	int carry;

	LOG((log_DSK,5,"	>>> KWD timing bitclk:%d datin:%d sect4:%d\n", bitclk, datin, sect4));

	if (0 == disk.seclate) {
		/* If SECLATE is 0, WDDONE' never goes low (counter's clear has precedence). */
		LOG((log_DSK,3,"	SECLATE:0 clears bitcount:0\n"));
		disk.bitcount = 0;
		carry = 0;
	} else {
		/*
		 * If SECLATE is 1, the counter will count or load:
		 */
		if ((disk.shiftreg_in & 0x10000) && !GET_KCOM_WFFO(disk.kcom)) {
			/*
			 * If HIORDBIT is 1 at the falling edge of BITCLK, it sets the
			 * JK-FF 67b, and thus takes away the LOAD' assertion from the
			 * counter. It has been loaded with 15, so it counts to 16 on
			 * the next rising edge and makes WDDONE' go to 0.
			 */
			LOG((log_DSK,3,"	HIORDBIT:1 sets WFFO:1\n"));
			PUT_KCOM_WFFO(disk.kcom, 1);
		}
		if (disk.bitclk && !bitclk) {
			/*
			 * Falling edge of BITCLK, counting continues as it was preset
			 * with BUS[4] (WFFO) at the last KCOM<- load, or as set by a
			 * 1 bit being read in HIORDBIT.
			 */
			if (GET_KCOM_WFFO(disk.kcom)) {
				/*
				 * If BUS[4] (WFFO) was 1, both J and K' of the FF (74109) will
				 * be 1 at the rising edge of LDCOM' (at the end of KCOM<-)
				 * and Q will be 1. LOAD' is deassterted: count on clock.
				 */
				if (++disk.bitcount > 15)
					disk.bitcount = 0;
				carry = disk.bitcount == 15;
				LOG((log_DSK,3,"	WFFO:1 count bitcount:%2d\n",
					disk.bitcount));
			} else {
				/*
				 * If BUS[4] (WFFO) was 0, both J and K' will be 0, and Q
				 * will be 0. LOAD' is asserted: load on clock.
				 */
				disk.bitcount = 15;
				carry = 1;
				LOG((log_DSK,3,"	WFFO:0 load bitcount:%2d\n",
					disk.bitcount));
			}
		}
	}

	if (!disk.bitclk && bitclk) {
		/* clock the shift register on the rising edge of bitclk */
		disk.shiftreg_in = (disk.shiftreg_in << 1) | datin;
		/* and the output shift register, too */
		disk.shiftreg_out = disk.shiftreg_out << 1;
	}

	if (block == task_kwd) {
		disk.wdinit = 1;
		disk.wdtskena = 0;
		CPU_CLR_TASK_WAKEUP(task_kwd);
	} else if (block == task_ksec) {
		disk.stskena = 0;
		CPU_CLR_TASK_WAKEUP(task_ksec);
	}

	if (disk.carry != carry) {
		/* CARRY change */
		LOG((log_DSK,2,"	CARRY:%d->%d\n", disk.carry, carry));
		if (carry == 0) {
			if (disk.wdtskena) {
				LOG((log_DSK,2,"	wake KWD\n"));
				CPU_SET_TASK_WAKEUP(task_kwd);
			} else {
				disk.wdtskena = !GET_KCOM_WDINHIB(disk.kcom);
			}
		}
	} else if (carry == 1) {
		/*
		 * Latch a new data word while CARRY is high (WDDONE is 0)
		 * Note: The shifter outputs for bits 0 to 14 are connected
		 * to the latches inputs 1 to 15, while input bit 0 comes
		 * from the current datin.
		 * Shifter output 15 is the HIORDBIT signal.
		 */
		disk.data_in = disk.shiftreg_in & 0177777;
		LOG((log_DSK,5," 	LATCH:%06o (0x%04x)\n",
			disk.data_in, disk.data_in));
	}

	if (disk.sect4 != sect4) {
		LOG((log_DSK,2,"	SECT4:%d->%d\n", disk.sect4, sect4));
		if (sect4 == 0) {
			disk.stskena = 1;
			CPU_SET_TASK_WAKEUP(task_ksec);
		} else 	if (sect4 && cpu.task != task_ksec) {
			disk.seclate = 1;
			if (disk.seclate_id)
				timer_remove(disk.seclate_id);
			disk.seclate_id = timer_insert(TW_SECLATE,
				disk_seclate, 0, "seclate");
		}
	}

	/* remember previous state of signals */
	disk.bitclk = bitclk;
	disk.datin = datin;
	disk.sect4 = sect4;
	disk.carry = carry;
	LOG((log_DSK,5,"	<<< KWD timing\n"));
}


/** @brief timer callback to take away the SECLATE pulse (monoflop) */
static void disk_seclate(int id, int arg)
{
	LOG((log_DSK,2,"	SECLATE -> %d\n", arg));
	disk.seclate = arg;
}

/** @brief timer callback to take away the OK TO RUN pulse (reset) */
static void disk_ok_to_run(int id, int arg)
{
	LOG((log_DSK,2,"	OK TO RUN -> %d\n", arg));
	disk.ok_to_run = arg;
}

/**
 * @brief timer callback to pulse the STROBE' signal to the drive
 *
 * @param id timer id
 * @param arg contains the drive, cylinder, and restore flag
 */
static void disk_strobon(int id, int arg)
{
	int drive = arg % 2;
	int restore = (arg / 2) % 2;
	int cylinder = arg / 4;
	int seekok;
	int strobe;

	LOG((log_DSK,2,"	STROBE -> #%d restore:%d cylinder:%d\n",
		drive, restore, cylinder));

	/* This is really monoflop 52a generating a very short 0 pulse */
	for (strobe = 0; strobe < 2; strobe++) {
		/* pulse the strobe signal to the drive */
		drive_strobe(drive, cylinder, restore, strobe);

		/* if address is acknowledged */
		if (0 == drive_addx_acknowledge_0(drive)) {
			/* clear the monoflop 52b, i.e. no timer restart */
			LOG((log_DSK,2,"	STROBON:0\n"));
			disk.strobe = 0;
		}
	}

	/* update the seekok status: SKINC' && LAI' && ready */
	seekok = drive_seek_incomplete_0(drive) &
		drive_log_addx_interlock_0(drive) &
		~drive_seek_read_write_0(drive);

	if (seekok != disk.seekok) {
		disk.seekok = seekok;
		LOG((log_DSK,2,"	SEEKOK:%d\n", disk.seekok));
	}

	/* if the strobe is still set, restart the timer */
	if (disk.strobe)
		timer_insert(TW_STROBON, disk_strobon, arg, "strobe");
}

/** @brief timer callback to change the READY monoflop 31a */
static void disk_ready_mf31a(int id, int arg)
{
	disk.ready_mf31a = arg & drive_ready_0(disk.drive);
	LOG((log_DSK,2,"	*** READY mf31a:%d\n", disk.ready_mf31a));
}

/**
 * @brief called if one of the disk tasks blocks
 *
 * @param task task that blocks (either task_ksec or task_kwd)
 */
void disk_block(int task)
{
	kwd_timing(disk.bitclk, disk.datin, task);
}

/**
 * @brief bus driven by disk status register KSTAT
 *
 * Part of the KSTAT register is made of two 4 bit latches S8T10 (Signetics).
 *
 * The signals BUS[8-11] are the current state of:
 *     BUS[0-3]   SECT[0-3]; from the Winchester drive (inverted)
 *     BUS[8]     SEEKOK'
 *     BUS[9]     SRWRDY' from the Winchester drive
 *     BUS[10]    RDYLAT' (latched READY' at last CLRSTAT, FF 45a output Q)
 *     BUS[11]    SEQERR (latched SEQERR at last CLRSTAT, FF 45b output Q')
 * The signals BUS[12,14-15] are just as they were loaded at KSTAT<- time.
 *     BUS[13]    CHSEMERROR (FF 44b output Q' inverted)
 */
void bs_read_kstat_0(void)
{
	int unit = disk.drive;
	int and;

	/* KSTAT[0-3] update the current sector in the kstat field */
	PUT_KSTAT_SECTOR(disk.kstat, drive_sector(unit) ^ DRIVE_SECTOR_MASK);

	/* KSTAT[4-7] bus is open */
	PUT_KSTAT_DONE(disk.kstat, 017);

	/* KSTAT[8] latch the inverted seekok status */
	PUT_KSTAT_SEEKFAIL(disk.kstat, disk.seekok ? 0 : 1);

	/* KSTAT[9] latch the drive seek/read/write status */
	PUT_KSTAT_SEEK(disk.kstat, drive_seek_read_write_0(unit));

	/* KSTAT[10] latch the latched (FF 45a at CLRSTAT) ready status */
	PUT_KSTAT_NOTRDY(disk.kstat, disk.rdylat);

	/* KSTAT[11] latch the latched (FF 45b at CLRSTAT) seqerr status (Q') */
	PUT_KSTAT_DATALATE(disk.kstat, disk.seqerr);

	/* KSTAT[13] latch the latched (FF 44b at CLRSTAT/KSTAT<-) chsmerror status (Q') */
	PUT_KSTAT_CHSMERROR(disk.kstat, disk.chsmerror);

	/* toggle the CHSMERROR bit in the and mask */
	and = disk.kstat;

	LOG((0,1,"	<-KSTAT; BUS &= %#o\n",
		and));
	LOG((0,2,"		sector:    %d\n", GET_KSTAT_SECTOR(disk.kstat)));
	LOG((0,2,"		done:      %d\n", GET_KSTAT_DONE(disk.kstat)));
	LOG((0,2,"		seekfail:  %d\n", GET_KSTAT_SEEKFAIL(disk.kstat)));
	LOG((0,2,"		seek:      %d\n", GET_KSTAT_SEEK(disk.kstat)));
	LOG((0,2,"		notrdy:    %d\n", GET_KSTAT_NOTRDY(disk.kstat)));
	LOG((0,2,"		datalate:  %d\n", GET_KSTAT_DATALATE(disk.kstat)));
	LOG((0,2,"		idle:      %d\n", GET_KSTAT_IDLE(disk.kstat)));
	LOG((0,2,"		chsmerror: %d\n", GET_KSTAT_CHSMERROR(disk.kstat)));
	LOG((0,2,"		completion:%d\n", GET_KSTAT_COMPLETION(disk.kstat)));

	cpu.bus &= and;
}

/**
 * @brief bus driven by disk data register KDATA input
 *
 * The input data register is a latch that latches the contents of
 * the lower 15 bits of a 16 bit shift register in its more significant
 * 15 bits, and the current read data bit is the least significant
 * bit. This is handled in kwd_timing.
 */
void bs_read_kdata_0(void)
{
	int and;
	/* get the current word from the drive */
	and = disk.data_in;
	LOG((0,1,"	<-KDATA (%#o)\n", and));
	cpu.bus &= and;
}

/**
 * @brief initiates a disk seek
 *
 * Initiates a disk seek operation. The KDATA register must have
 * been loaded previously, and the SENDADR bit of the KCOM
 * register previously set to 1.
 */
void f1_strobe_1(void)
{
	if (GET_KCOM_SENDADR(disk.kcom)) {
		LOG((0,1,"	STROBE (w/ SENDADR)\n"));
		/* Set the STROBON flag and start the STROBON monoflop */
		disk.strobe = 1;
		timer_insert(TW_STROBON, disk_strobon,
			4 * GET_KADDR_CYLINDER(disk.kaddr) +
			2 * GET_KADDR_RESTORE(disk.kaddr) +
			disk.drive,
			"STROBE");
	} else {
		LOG((0,1,"	STROBE (w/o SENDADR)\n"));
		/* FIXME: what to do if SENDADR isn't set? */
	}
}

/**
 * @brief load disk status register
 *
 * KSTAT[12-15] are loaded from BUS[12-15], except that BUS[13] is
 * ORed into KSTAT[13].
 *
 * NB: The 4 bits are just software, not changed by hardware
 */
void f1_load_kstat_1(void)
{
	LOG((0,1,"	KSTAT<-; BUS[12-15] %#o\n", cpu.bus));

	/* KSTAT[12] is just taken from BUS[12] */
	PUT_KSTAT_IDLE(disk.kstat, GET_KSTAT_IDLE(cpu.bus));

	/* KSTAT[14-15] are just taken from BUS[14-15] */
	PUT_KSTAT_COMPLETION(disk.kstat, GET_KSTAT_COMPLETION(cpu.bus));
	PUT_KSTAT_CHSMERROR(disk.kstat,
		disk.chsmerror |= GET_KSTAT_CHSMERROR(cpu.bus));

	LOG((0,2,"		idle:      %d\n", GET_KSTAT_IDLE(disk.kstat)));
	LOG((0,2,"		chsmerror: %d\n", GET_KSTAT_CHSMERROR(disk.kstat)));
	LOG((0,2,"		completion:%d\n", GET_KSTAT_COMPLETION(disk.kstat)));
}

/**
 * @brief load data out register, or the disk address register
 *
 * KDATA is loaded from BUS.
 */
void f1_load_kdata_1(void)
{
	disk.data_out = cpu.bus;
	/* at the same time load the output shift register */
	disk.shiftreg_out = cpu.bus;
	if (GET_KCOM_SENDADR(disk.kcom)) {
		PUT_KADDR_SECTOR(disk.kaddr,
			GET_KADDR_SECTOR(cpu.bus));
		PUT_KADDR_CYLINDER(disk.kaddr,
			GET_KADDR_CYLINDER(cpu.bus));
		PUT_KADDR_HEAD(disk.kaddr,
			GET_KADDR_HEAD(cpu.bus));
		PUT_KADDR_DRIVE(disk.kaddr,
			GET_KADDR_DRIVE(cpu.bus));
		PUT_KADDR_RESTORE(disk.kaddr,
			GET_KADDR_RESTORE(cpu.bus));

		LOG((0,1,"	KDATA<-; BUS (%#o) (sec:%d cyl:%d head:%d drive:%d restore:%d)\n",
			cpu.bus,
			GET_KADDR_SECTOR(disk.kaddr),
			GET_KADDR_CYLINDER(disk.kaddr),
			GET_KADDR_HEAD(disk.kaddr),
			GET_KADDR_DRIVE(disk.kaddr),
			GET_KADDR_RESTORE(disk.kaddr)));
#if	1
		{
			static int last_kaddr;
			if (disk.kaddr != last_kaddr) {
				printf("#%d C:%d H:%d S:%d restore:%d\n",
					GET_KADDR_DRIVE(disk.kaddr),
					GET_KADDR_CYLINDER(disk.kaddr),
					GET_KADDR_HEAD(disk.kaddr),
					GET_KADDR_SECTOR(disk.kaddr),
					GET_KADDR_RESTORE(disk.kaddr));
				last_kaddr = disk.kaddr;
			}
		}
#endif
	} else {
		LOG((0,1,"	KDATA<-; BUS (%#o)\n", cpu.bus));
	}
}

/**
 * @brief advances shift registers holding KADR
 *
 * Advances the shift registers holding the KADR register so that they
 * present the number and read/write/check status of the next record
 * to the hardware.
 *
 * Sheet 10, shifter (74195) parts #36 and #37
 *
 * Vcc, BUS[08], BUS[10], BUS[12] go to #36 A,B,C,D
 * Vcc, BUS[09], BUS[11], BUS[13] go to #37 A,B,C,D
 * A is connected to ground on both chips;
 * both shifters are loaded with KADR<-
 *
 * The QA outputs are #36 -> RECNO(0) and #37 -> RECNO(1)
 *
 * RECNO(0) (QA of #37) goes to J and K' of #36
 * RECNO(1) (QA of #36) is inverted and goes to J and K' of #37
 *
 *  shift/   RECNO(0)    RECNO(1)     R/W/C presented
 *   load      #37         #36        to the drive
 * ---------------------------------------------------
 *   load       0           0         HEADER
 * 1st shift    1           0         LABEL
 * 2nd shift    1           1         DATA
 * 3rd shift    0           1         (none) 0 = read
 * [ 4th        0           0         (none) 2 = write ]
 * [ 5th        1           0         (none) 3 = write ]
 * [ 6th        1           1         (none) 1 = check ]
 *
 */
void f1_increcno_1(void)
{
	switch (disk.krecno) {
	case RECNO_HEADER:
		disk.krecno = RECNO_LABEL;
		disk.krwc = GET_KADR_LABEL(disk.kadr);
		LOG((0,1,"	INCRECNO; HEADER -> LABEL (%#o)\n", disk.krecno));
		break;
	case RECNO_PAGENO:
		disk.krecno = RECNO_HEADER;
		disk.krwc = GET_KADR_HEADER(disk.kadr);
		LOG((0,1,"	INCRECNO; PAGENO -> HEADER (%#o)\n", disk.krecno));
		break;
	case RECNO_LABEL:
		disk.krecno = RECNO_DATA;
		disk.krwc = GET_KADR_DATA(disk.kadr);
		LOG((0,1,"	INCRECNO; LABEL -> DATA (%#o)\n", disk.krecno));
		break;
	case RECNO_DATA:
		disk.krecno = RECNO_PAGENO;
		disk.krwc = 0;	/* read */
		LOG((0,1,"	INCRECNO; DATA -> PAGENO (%#o)\n", disk.krecno));
		break;
	}
}

/**
 * @brief reset all error latches
 *
 * Causes all error latches in the disk controller hardware to reset,
 * clears KSTAT[13].
 *
 * NB: IDLE (KSTAT[12]) and COMPLETION (KSTAT[14-15]) are not cleared
 */
void f1_clrstat_1(void)
{
	LOG((0,1,"	CLRSTAT\n"));

	disk.seekok = 1;
	disk.chsmerror = 0;
	disk.rdylat = drive_ready_0(disk.drive);
	disk.seqerr = 0;

	/* set or reset monoflop 31a, depending on drive READY' */
	disk.ready_mf31a = drive_ready_0(disk.drive);

	kwd_timing(disk.bitclk, disk.datin, 0);

	/* start monoflop 31a, which resets ready_mf31a */
	timer_insert(TW_READY, disk_ready_mf31a, 1, "drive READY'");
}

/**
 * @brief load the KCOM register from bus
 *
 * This causes the KCOM register to be loaded from BUS[1-5]. The
 * KCOM register has the following interpretation:
 *	(1) XFEROFF = 1 inhibits data transmission to/from the disk.
 *	(2) WDINHIB = 1 prevents the disk word task from awakening.
 * 	(3) BCLKSRC = 0 takes bit clock from disk input or crystal clock,
 * 	    as appropriate. BCLKSRC = 1 force use of crystal clock.
 *	(4) WFFO = 0 holds the disk bit counter at -1 until a 1 bit is read.
 *	    WFFO = 1 allows the bit counter to proceed normally.
 *	(5) SENDADR = 1 causes KDATA[4-12] and KDATA[15] to be transmitted
 *	    to disk unit as track address. SENDADR = 0 inhibits such
 *	    transmission.
 */
void f1_load_kcom_1(void)
{
	disk.kcom = cpu.bus;
	LOG((0,1,"	KCOM<-; BUS %#o\n", disk.kcom));
	LOG((0,2,"		xferoff: %d\n", GET_KCOM_XFEROFF(disk.kcom)));
	LOG((0,2,"		wdinhib: %d\n", GET_KCOM_WDINHIB(disk.kcom)));
	LOG((0,2,"		bclksrc: %d\n", GET_KCOM_BCLKSRC(disk.kcom)));
	LOG((0,2,"		wffo:    %d\n", GET_KCOM_WFFO(disk.kcom)));
	LOG((0,2,"		sendadr: %d\n", GET_KCOM_SENDADR(disk.kcom)));
	kwd_timing(disk.bitclk, disk.datin, 0);
}

/**
 * @brief load the KADR register from bus
 *
 * The KADR register is loaded from BUS[8-14]. This register has the format
 * of word C in section 6.0 above. In addition, it causes the head address
 * bit to be loaded from KDATA[13].
 *
 * NB: the record numer RECNO(0) and RECNO(1) is reset to 0
 */
void f1_load_kadr_1(void)
{
	int unit, head;

	/* store into the separate fields of KADR */
	PUT_KADR_SEAL(disk.kadr, GET_KADR_SEAL(cpu.bus));
	PUT_KADR_HEADER(disk.kadr, GET_KADR_HEADER(cpu.bus));
	PUT_KADR_LABEL(disk.kadr, GET_KADR_LABEL(cpu.bus));
	PUT_KADR_DATA(disk.kadr, GET_KADR_DATA(cpu.bus));
	PUT_KADR_NOXFER(disk.kadr, GET_KADR_NOXFER(cpu.bus));
	PUT_KADR_DRIVE(disk.kadr, GET_KADR_DRIVE(cpu.bus));

	/* latch selected drive from DATA[14] output (FF 67a really) */
	unit = GET_KADDR_DRIVE(disk.data_out);
	PUT_KADDR_DRIVE(disk.kaddr, unit);
	/* select the head on the current drive (select line is HEAD')*/
	head = GET_KADDR_HEAD(disk.data_out);
	PUT_KADDR_HEAD(disk.kaddr, head);
	/* select the drive unit nad head */
	disk.drive = drive_unit(unit);
	drive_unit_select(unit);
	drive_head_select(unit, head);
	/* On KDAR<- bit 0 of parts #36 and #37 is reset to 0, i.e. recno = 0 */
	disk.krecno = 0;
	/* current read/write/check is that for the header */
	disk.krwc = GET_KADR_HEADER(disk.kadr);

	LOG((0,1,"	KADR<-; BUS[8-14] #%o\n", disk.kadr));
	LOG((0,2,"		seal:   %#o\n", GET_KADR_SEAL(disk.kadr)));
	LOG((0,2,"		header: %s\n",  rwc_name[GET_KADR_HEADER(disk.kadr)]));
	LOG((0,2,"		label:  %s\n",  rwc_name[GET_KADR_LABEL(disk.kadr)]));
	LOG((0,2,"		data:   %s\n",  rwc_name[GET_KADR_DATA(disk.kadr)]));
	LOG((0,2,"		noxfer: %d\n",  GET_KADR_NOXFER(disk.kadr)));
	LOG((0,2,"		drive:  %d\n",  GET_KADR_DRIVE(disk.kadr)));
}

/**
 * @brief branch on disk word task active and init
 *
 * NEXT <- NEXT OR (WDTASKACT && WDINIT ? 037 : 0)
 */
void f2_init_1(void)
{
	int or = cpu.task == task_kwd && disk.wdinit ? 037 : 0;
	LOG((0,1,"	INIT; %sbranch (%#o | %#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief branch on read/write/check state of the current record
 *
 * NEXT <- NEXT OR (current record to be written ? 3 :
 *	current record to be checked ? 2 : 0);
 *
 * NB: note how krecno counts 0,2,3,1 ... etc.
 * on 0: it presents the RWC for HEADER
 * on 2: it presents the RWC for LABEL
 * on 3: it presents the RWC for DATA
 * on 1: it presents the RWC 0, i.e. READ
 *
 * -NEXT[08] = -CHECK = RWC[0] | RWC[1]
 * -NEXT[09] = W/R = RWC[0]
 *
 *  rwc   | -next
 * -------+------
 *  0  0  |  0
 *  0  1  |  2
 *  1  0  |  3
 *  1  1  |  3
 *
 */
void f2_rwc_1(void)
{
	static int branch_map[4] = {0,2,3,3};
	int rwc = disk.krwc;
	int or = 0;
	int init = cpu.task == task_kwd && disk.wdinit ? 037 : 0;

	switch (disk.krecno) {
	case RECNO_HEADER:
		or = branch_map[rwc];
		LOG((0,1,"	RWC; %sbranch %o header:%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", disk.krecno,
			rwc_name[rwc], cpu.next2, or, init));
		break;
	case RECNO_PAGENO:
		or = branch_map[rwc];
		LOG((0,1,"	RWC; %sbranch %o pageno:%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", disk.krecno,
			rwc_name[rwc], cpu.next2, or, init));
		break;
	case RECNO_LABEL:
		or = branch_map[rwc];
		LOG((0,1,"	RWC; %sbranch %o label:%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", disk.krecno,
			rwc_name[rwc], cpu.next2, or, init));
		break;
	case RECNO_DATA:
		or = branch_map[rwc];
		LOG((0,1,"	RWC; %sbranch %o data:%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", disk.krecno,
			rwc_name[rwc], cpu.next2, or, init));
		break;
	}
	CPU_BRANCH(or | init);
}

/**
 * @brief branch on the current record number by a lookup table
 *
 * NEXT <- NEXT OR MAP (current record number) where
 *   MAP(0) = 0		(header)
 *   MAP(1) = 2		(label)
 *   MAP(2) = 3		(pageno)
 *   MAP(3) = 1		(data)
 *
 * NB: The map isn't needed, because disk.krecno counts exactly this way.
 */
void f2_recno_1(void)
{
	int or = disk.krecno;
	int init = cpu.task == task_kwd && disk.wdinit ? 037 : 0;

	LOG((0,1,"	RECNO; %sbranch recno:%d (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", disk.krecno, cpu.next2, or, init));
	CPU_BRANCH(or | init);
}

/**
 * @brief branch on the data transfer state
 *
 * NEXT <- NEXT OR (if current command wants data transfer ? 1 : 0)
 */
void f2_xfrdat_1(void)
{
	int or = GET_KADR_NOXFER(disk.kadr) ? 0 : 1;
	int init = cpu.task == task_kwd && disk.wdinit ? 037 : 0;

	LOG((0,1,"	XFRDAT; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
}

/**
 * @brief branch on the disk ready signal
 *
 * NEXT <- NEXT OR (if disk not ready to accept command ? 1 : 0)
 */
void f2_swrnrdy_1(void)
{
	int or = drive_seek_read_write_0(disk.drive);
	int init = cpu.task == task_kwd && disk.wdinit ? 037 : 0;

	LOG((0,1,"	SWRNRDY; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
}

/**
 * @brief branch on the disk fatal error condition
 *
 * NEXT <- NEXT OR (if fatal error in latches ? 0 : 1)
 */
void f2_nfer_1(void)
{
	int or = disk.kfer ? 0 : 1;
	int init = cpu.task == task_kwd && disk.wdinit ? 037 : 0;

	LOG((0,1,"	NFER; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
}

/**
 * @brief branch on the seek busy status
 *
 * NEXT <- NEXT OR (if seek strobe still on ? 1 : 0)
 *
 * The STROBE signal is elongated with the help of two monoflops.
 * The first one has a rather short pulse duration:
 * 	tW = K * Rt * Cext * (1 + 0.7/Rt)
 *	K = 0.28 for 74123
 *	Rt = kOhms
 * 	Cext = pF
 * Rt = 20k, Cext = 150pf => 870ns
 *
 * The first one triggers the second, which will be cleared
 * by ADDRACK' from the drive going 0.
 * Its duration is:
 * Rt = 20k, Cext = 0.01µF (=10000pF) => 8596ns
 */
void f2_strobon_1(void)
{
	int or = disk.strobe;
	int init = cpu.task == task_kwd && disk.wdinit ? 037 : 0;
	LOG((0,2,"	STROBON; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
}

/**
 * @brief update the disk controller with a new bitclk
 *
 * @param id timer id
 * @param arg is the current data word<<4, and bit count in lower 4 bits
 */
static void disk_bitclk(int id, int arg)
{
	int clk = arg & 1;
	int bits = drive_bits_per_sector();
	int bit = 0;

	/*
	 * The source for BITCLK and DATAIN depends on disk controller part #65
	 *
	 *  BCLKSRC  W/R | BITCLK | DATAIN
	 * --------------+--------+---------
	 *    0       0  |  RDCLK | RDDATA
	 *    0       1  |  CLK%2 | DATOUT
	 *    1       0  |  CLK%2 | RDDATA
	 *    1       1  |  CLK%2 | DATOUT
	 *
	 */
	switch (GET_KCOM_BCLKSRC(disk.kcom)) {
	case 0:	/* select clock as appropiate */
		if (disk.krwc & 2) {
			bit = (disk.shiftreg_out >> 15) & 1;
			kwd_timing(clk, bit, 0);
		} else {
			/* if XFEROFF is set, keep the bit at 1 (RDGATE' is high) */
			if (GET_KCOM_XFEROFF(disk.kcom))
				bit = 1;
			else
				bit = drive_rddata(disk.drive, arg);
			LOG((log_DSK,7,"	BITCLK#%d clk:%d (DRIVE) bit:%d @%lldns\n",
				arg, clk, bit, ntime()));
			kwd_timing(clk, bit, 0);
		}
		break;
	case 1:	/* always select the crystal clock */
		if (disk.krwc & 2) {
			bit = (disk.shiftreg_out >> 15) & 1;
			kwd_timing(clk, bit, 0);
		} else {
			bit = drive_rddata(disk.drive, arg);
			LOG((log_DSK,7,"	BITCLK#%d clk:%d (CRYST) bit:%d @%lldns\n",
				arg, clk, bit, ntime()));
			kwd_timing(clk, bit, 0);
		}
		break;
	}

	/* more bits to clock? */
	if (++arg < bits)
		timer_insert(drive_bit_time(disk.drive), disk_bitclk,
			arg, "disk bitclk");
}

/**
 * @brief this callback is called by the drive timer whenever a new sector starts
 *
 * @param unit the unit number
 */
void disk_sector_start(int unit)
{
	/* just return, if this isn't the selected drive */
	if (unit != disk.drive)
		return;

	LOG((log_DSK,1,"	unit %d sector start (%d)\n",
		unit, drive_sector(unit) ^ DRIVE_SECTOR_MASK));
	/* start a timer chain for the bit clock */
	disk_bitclk(unit, 0);
}

/**
 * @brief pass down command line arguments to the disk emulation
 *
 * @param arg command line argument
 * @result returns 0 on success, -1 on error
 */
int disk_args(const char *arg)
{
	return -1;
}

/**
 * @brief initialize the disk context and insert a disk wort timer
 *
 * @result returns 0 on success, fatal() on error
 */
int disk_init(void)
{
	memset(&disk, 0, sizeof(disk));

	disk.wdtskena = 1;

	disk.seclate = 1;
	timer_insert(TW_SECLATE, disk_seclate, 0, "seclate");
	disk.ok_to_run = 0;
	timer_insert(15 * CPU_MICROCYCLE_TIME, disk_ok_to_run, 1, "ok to run");

	/* install a callback to be called whenever a drive sector starts */
	drive_sector_callback(0, disk_sector_start);
	drive_sector_callback(1, disk_sector_start);

	drive_unit_select(disk.drive);
	drive_head_select(disk.drive, GET_KADDR_HEAD(disk.data_out));

	return 0;
}
