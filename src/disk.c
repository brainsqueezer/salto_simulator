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
 * $Id: disk.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "debug.h"
#include "memory.h"
#include "disk.h"
#include "drive.h"

/** @brief 1 to debug the JK flip-flops, 0 to use a lookup table */
#define	JKFF_FUNCTION	0

/**
 *
 * Just for completeness' sake:
 * The mapping of disk controller connector P2 pins to the
 * Winchester disk drive signals (see drive.h)
 * <PRE>
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
 * </PRE>
 */

#define	UNIT_W		256
#define	LED_W		16
#define	LED_H		16
#define	LED_COUNT	5

/**
 * @brief update the frontend's drive image name status
 *
 * Note: There is no such indicator in the real Alto. This is just
 * to keep the user informed about drive activity.
 */
void disk_show_image_name(int unit)
{
	const char *basename = drive_basename(unit);
	int x = DISPLAY_WIDTH - unit * UNIT_W - (strlen(basename) + 2) * DBG_FONT_W;
	border_printf(x, 1, " %s ", basename);
}

/**
 * @brief update the frontend's drive activity indicators
 *
 * Note: There is no such indicator in the real Alto. This is just
 * to keep the user informed about drive activity.
 */
void disk_show_indicator(int unit, int n, int val)
{
	static int prev[2][LED_COUNT];

	if (n < 0 || n >= LED_COUNT)
		return;

	if (val == prev[unit][n])
		return;

	sdl_draw_icon(DISPLAY_WIDTH - (LED_COUNT - n) * LED_W - unit * UNIT_W,
		DBG_FONT_H + 1, val);
	prev[unit][n] = val;
}

/**
 * @brief update the frontend's drive activity indicators
 *
 * Note: There is no such indicator in the real Alto. This is just
 * to keep the user informed about drive activity.
 */
void disk_show_indicators(int unit)
{
	static int rwc[2][4] = {
		{led_green_on,led_yellow_on,led_red_on,led_red_on},
		{led_green_off,led_yellow_off,led_red_off,led_red_off}
	};
	int page;

#if	(LED_COUNT==9)
	disk_show_indicator(unit, 0,
		GET_KCOM_XFEROFF(dsk.kcom) ? led_green_off : led_green_on);
	disk_show_indicator(unit, 1,
		GET_KCOM_WDINHIB(dsk.kcom) ? led_green_off : led_green_on);
	disk_show_indicator(unit, 2,
		GET_KCOM_BCLKSRC(dsk.kcom) ? led_yellow_on : led_yellow_off);
	disk_show_indicator(unit, 3,
		GET_KCOM_WFFO(dsk.kcom) ? led_green_off : led_green_on);
	disk_show_indicator(unit, 4,
		rwc[GET_KCOM_WDINHIB(dsk.kcom)][dsk.krwc]);
	page =  DRIVE_PAGE(GET_KADDR_CYLINDER(dsk.kaddr),
		GET_KADDR_HEAD(dsk.kaddr),
		GET_KADDR_SECTOR(dsk.kaddr));
	disk_show_indicator(unit, 5, led_0 + page / 1000);
	disk_show_indicator(unit, 6, led_0 + (page / 100) % 10);
	disk_show_indicator(unit, 7, led_0 + (page / 10) % 10);
	disk_show_indicator(unit, 8, led_0 + page % 10);
#elif	(LED_COUNT==5)
	disk_show_indicator(unit, 0,
		rwc[GET_KCOM_WDINHIB(dsk.kcom)][dsk.krwc]);
	page =  DRIVE_PAGE(GET_KADDR_CYLINDER(dsk.kaddr),
		GET_KADDR_HEAD(dsk.kaddr),
		GET_KADDR_SECTOR(dsk.kaddr));
	disk_show_indicator(unit, 1, led_0 + page / 1000);
	disk_show_indicator(unit, 2, led_0 + (page / 100) % 10);
	disk_show_indicator(unit, 3, led_0 + (page / 10) % 10);
	disk_show_indicator(unit, 4, led_0 + page % 10);
#endif
}

/** @brief simulate previous sysclka */
static const char sysclka0[4] = {
	JKFF_CLK,
	0,
	0,
	JKFF_CLK
};

/** @brief simulate current sysclka */
static const char sysclka1[4] = {
	0,
	0,
	JKFF_CLK,
	JKFF_CLK
};

/** @brief simulate previous sysclkb */
static const char sysclkb0[4] = {
	JKFF_CLK,
	JKFF_CLK,
	0,
	0
};

/** @brief simulate current sysclkb */
static const char sysclkb1[4] = {
	JKFF_CLK,
	0,
	0,
	JKFF_CLK
};

/** @brief disk context */
disk_t dsk;

static void disk_seclate(int id, int arg);
static void disk_ok_to_run(int id, int arg);
static void disk_strobon(int id, int arg);

#if	DEBUG
/** @brief human readable names for the KADR<- modes */
static const char *rwc_name[4] = {"read", "check", "write", "write3"};
#endif

#if	JKFF_FUNCTION

#if	DEBUG
static const char *jkff_name;
/** @brief macro to set the name of a FF in DEBUG=1 builds only */
#define	DEBUG_NAME(x)	jkff_name = x
#else
#define	DEBUG_NAME(x)
#endif

/**
 * @brief simulate a 74109 J-K flip-flop with set and reset inputs
 *
 * @param s0 is the previous state of the FF's in- and outputs
 * @param s1 is the next state
 * @result returns the next state and probably modified Q output
 */
static __inline jkff_t update_jkff(jkff_t s0, jkff_t s1)
{
	switch (s1 & (JKFF_C | JKFF_S)) {
	case JKFF_C | JKFF_S:	/* C' is 1, and S' is 1 */
		if (((s0 ^ s1) & s1) & JKFF_CLK) {
			/* rising edge of the clock */
			switch (s1 & (JKFF_J | JKFF_K)) {
			case 0:
				/* both J and K' are 0: set Q to 0, Q' to 1 */
				s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				if (s0 & JKFF_Q) {
					LOG((log_DSK,5,"%s J:0 K':0 -> Q:0\n", jkff_name));
				}
				break;
			case JKFF_J:
				/* J is 1, and K' is 0: toggle Q */
				if (s0 & JKFF_Q)
					s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				else
					s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				LOG((log_DSK,5,"%s J:0 K':1 flip-flop Q:%d\n",
					jkff_name, (s1 & JKFF_Q) ? 1 : 0));
				break;
			case JKFF_K:
				if ((s0 ^ s1) & JKFF_Q) {
					LOG((log_DSK,5,"%s J:0 K':1 keep Q:%d\n",
						jkff_name, (s1 & JKFF_Q) ? 1 : 0));
				}
				/* J is 0, and K' is 1: keep Q as is */
				if (s0 & JKFF_Q)
					s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				else
					s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				break;
			case JKFF_J | JKFF_K:
				/* both J and K' are 1: set Q to 1 */
				s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				if (!(s0 & JKFF_Q)) {
					LOG((log_DSK,5,"%s J:1 K':1 -> Q:1\n",
					jkff_name));
				}
				break;
			}
		} else {
			/* keep Q */
			s1 = (s1 & ~JKFF_Q) | (s0 & JKFF_Q);
		}
		break;
	case JKFF_S:
		/* S' is 1, C' is 0: set Q to 0, Q' to 1 */
		s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
		if (s0 & JKFF_Q) {
			LOG((log_DSK,5,"%s C':0 -> Q:0\n", jkff_name));
		}
		break;
	case JKFF_C:
		/* S' is 0, C' is 1: set Q to 1, Q' to 0 */
		s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
		if (!(s0 & JKFF_Q)) {
			LOG((log_DSK,5,"%s S':0 -> Q:1\n", jkff_name));
		}
		break;
	case 0:
	default:
		/* unstable state (what to do?) */
		s1 = s1 | JKFF_Q | JKFF_Q0;
		LOG((log_DSK,5,"%s C':0 S':0 -> Q:1 and Q':1 <unstable>\n", jkff_name));
		break;
	}
	return s1;
}

#else

#include "jkfflut.c"

/** @brief just ignore debug arguments with the lookup table */
#define	DEBUG_NAME(name)

/** @brief lookup JK flip-flop state change from s0 to s1 */
#define	update_jkff(s0,s1) jkff_lookup[(s1)&63][(s0)&63]

#endif

/**
 * <PRE>
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
 *               |     |      |    |         | MONO | 85960ns ~= 86µs
 *   SECT[4] >---|-----|CLK   |    |         | FLOP |
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
 * </PRE>
 */

/**
 * @brief monoflop 31a pulse duration
 * Rt = 15k, Cext = .47µF (=470000pF) => 2066120ns (~2ms)
 */
#define	TW_READY	((ntime_t)2066120)

/**
 * @brief monoflop 31b pulse duration
 * Rt = 30k, Cext = .01µF (=10000pF) => 86960ns (~85us)
 *
 * There's something wrong with this, or the KSEC would never ever
 * be able to commence the KWD. The SECLATE monoflop ouput has to go
 * high some time into the KSEC task microcode, before the sequence
 * error state is checked.
 *
 *  TW_SECLATE	((ntime_t)85960)
 *  TW_SECLATE	(46*CPU_MICROCYCLE_TIME)
*/
#define	TW_SECLATE	((ntime_t)8596)

/** @brief monoflop 52b pulse duration
 * Rt = 20k, Cext = 0.01µF (=10000pF) => 57960ns (~58us)
 */
#define	TW_STROBON	((ntime_t)57960)

/**
 * <PRE>
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
 * </PRE>
 */
#define	INIT	(cpu.task == task_kwd && dsk.wdinit0)

#define	WFFO	(GET_KCOM_WFFO(dsk.kcom))
#define	WDALLOW	(!GET_KCOM_WDINHIB(dsk.kcom))
#define	WDINIT	((dsk.ff_53b & JKFF_Q) ? 1 : 0)
#define	RDYLAT	((dsk.ff_45a & JKFF_Q) ? 1 : 0)
#define	SEQERR	((cpu.task == task_ksec && dsk.seclate == 0) || \
		(cpu.task == task_kwd && dsk.carry == 1))
#define	ERRWAKE	(RDYLAT | dsk.ready_mf31a)
#define	SEEKOK	(dsk.seekok)

/**
 * @brief disk word timing
 *
 * Implement the FIFOs and gates in the description above.
 *
 * @param bitclk the current bitclk level
 * @param datin the level of the bit read from the disk
 * @param block contains the task number of a blocking task, or 0 otherwise
 */
static void kwd_timing(int bitclk, int datin, int block)
{
	static int wddone0;
	int wddone1 = wddone0;
	int i;

	LOG((log_DSK,5,"	>>> KWD timing bitclk:%d datin:%d sect4:%d\n",
		bitclk, datin, drive_sector_mark_0(dsk.drive)));

	if (0 == dsk.seclate) {
		/* If SECLATE is 0, WDDONE' never goes low (counter's clear has precedence). */
		LOG((log_DSK,3,"	SECLATE:0 clears bitcount:0\n"));
		dsk.bitcount = 0;
		dsk.carry = 0;
	} else if (dsk.bitclk && !bitclk) {
		/*
		 * If SECLATE is 1, the counter will count or load:
		 */
		if ((dsk.shiftin & 0x10000) && !WFFO) {
			/*
			 * If HIORDBIT is 1 at the falling edge of BITCLK, it sets the
			 * JK-FF 67b, and thus takes away the LOAD' assertion from the
			 * counter. It has been loaded with 15, so it counts to 16 on
			 * the next rising edge and makes WDDONE' go to 0.
			 */
			LOG((log_DSK,3,"	HIORDBIT:1 sets WFFO:1\n"));
			PUT_KCOM_WFFO(dsk.kcom, 1);
			disk_show_indicators(dsk.drive);
		}
		/*
		 * Falling edge of BITCLK, counting continues as it was preset
		 * with BUS[4] (WFFO) at the last KCOM<- load, or as set by a
		 * 1 bit being read in HIORDBIT.
		 */
		if (WFFO) {
			/*
			 * If BUS[4] (WFFO) was 1, both J and K' of the FF (74109) will
			 * be 1 at the rising edge of LDCOM' (at the end of KCOM<-)
			 * and Q will be 1. LOAD' is deassterted: count on clock.
			 */
			if (++dsk.bitcount > 15)
				dsk.bitcount = 0;
			dsk.carry = dsk.bitcount == 15;
			LOG((log_DSK,3,"	WFFO:1 count bitcount:%2d\n", dsk.bitcount));
		} else {
			/*
			 * If BUS[4] (WFFO) was 0, both J and K' will be 0, and Q
			 * will be 0. LOAD' is asserted: load on clock.
			 */
			dsk.bitcount = 15;
			dsk.carry = 1;
			LOG((log_DSK,3,"	WFFO:0 load bitcount:%2d\n", dsk.bitcount));
		}
	} else if (!dsk.bitclk && bitclk) {
		/* clock the shift register on the rising edge of bitclk */
		dsk.shiftin = (dsk.shiftin << 1) | datin;
		/* and the output shift register, too */
		dsk.shiftout = dsk.shiftout << 1;
	}

	if (wddone0 != wddone1) {
		LOG((log_DSK,2,"	WDDONE':%d->%d\n", wddone0, wddone1));
	}

	if (dsk.carry) {
		/* CARRY = 1 -> WDDONE' = 0 */
		wddone1 = 0;
		if (wddone0 == 0) {
			/*
			 * Latch a new data word while WDDONE is 0
			 * Note: The shifter outputs for bits 0 to 14 are connected
			 * to the latches inputs 1 to 15, while input bit 0 comes
			 * from the current datin.
			 * Shifter output 15 is the HIORDBIT signal.
			 */
			dsk.datain = dsk.shiftin & 0177777;
			/* load the output shift register */
			dsk.shiftout = dsk.dataout;
			LOG((log_DSK,6," 	LATCH in:%06o (0x%04x) out:%06o (0x%04x)\n",
			dsk.datain, dsk.datain, dsk.dataout, dsk.dataout));
		}
	} else {
		/* CARRY = 0 -> WDDONE' = 1 */
		wddone1 = 1;
	}

	/* remember previous state of wddone */
	wddone0 = wddone1;

	/**
	 * JK flip-flop 43b (word task)
	 * <PRE>
	 * CLK	WDDONE'
	 * J	1
	 * K'	1
	 * S'	1
	 * C'	WDTSKENA
	 * Q	to 53a J
	 * </PRE>
	 */
	DEBUG_NAME("		KWD 43b");
	dsk.ff_43b = update_jkff(dsk.ff_43b,
		(wddone1 ? JKFF_CLK : 0) |
		JKFF_J |
		JKFF_K |
		(dsk.ok_to_run ? JKFF_S : 0) |
		((dsk.ff_43a & JKFF_Q) ? 0 : JKFF_C));

	/* count for the 4 stages of sysclka and sysclkb transitions */
	for (i = 0; i < 4; i++) {

		if (sysclka0[i] != sysclka1[i]) {
			LOG((log_DSK,7,"	SYSCLKA':%d->%d\n",
			sysclka0[i], sysclka1[i]));
		}
		if (sysclkb0[i] != sysclkb1[i]) {
			LOG((log_DSK,7,"	SYSCLKB':%d->%d\n",
			sysclkb0[i], sysclkb1[i]));
		}

		/**
		 * JK flip-flop 53b (word task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	0
		 * K'	(BLOCK & WDTSKACT)'
		 * S'	WDALLOW
		 * C'	1
		 * Q	WDINIT
		 * </PRE>
		 */
		DEBUG_NAME("		KWD 53b");
		dsk.ff_53b = update_jkff(dsk.ff_53b,
			sysclkb1[i] |
			0 |
			((block == task_kwd) ? 0 : JKFF_K) |
			(WDALLOW ? JKFF_S : 0) |
			JKFF_C);
		/**
		 * JK flip-flop 53a (word task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 43b Q
		 * K'	(BLOCK & WDTSKACT)'
		 * S'	1
		 * C'	WDALLOW
		 * Q	to 43a J and K'
		 * </PRE>
		 */
		DEBUG_NAME("		KWD 53a");
		dsk.ff_53a = update_jkff(dsk.ff_53a,
			sysclkb1[i] |
			((dsk.ff_43b & JKFF_Q) ? JKFF_J : 0) |
			((block == task_kwd) ? 0 : JKFF_K) |
			JKFF_S |
			(WDALLOW ? JKFF_C : 0));

		/**
		 * JK flip-flop 43a (word task)
		 * <PRE>
		 * CLK	SYSCLKA'
		 * J	from 53a Q
		 * K'	from 53a Q
		 * S'	1
		 * C'	WDALLOW
		 * Q	WDTSKENA', Q' WDTSKENA
		 * </PRE>
		 */
		DEBUG_NAME("		KWD 43a");
		dsk.ff_43a = update_jkff(dsk.ff_43a,
			sysclka1[i] |
			((dsk.ff_53a & JKFF_Q) ? JKFF_J : 0) |
			((dsk.ff_53a & JKFF_Q) ? JKFF_K : 0) |
			JKFF_S |
			(WDALLOW ? JKFF_C : 0));

		/**
		 * JK flip-flop 45a (ready latch)
		 * <PRE>
		 * CLK	SYSCLKA'
		 * J	READY' from drive
		 * K'	1
		 * S'	1
		 * C'	CLRSTAT'
		 * Q	RDYLAT'
		 * </PRE>
		 */
		DEBUG_NAME("		RDYLAT 45a");
		dsk.ff_45a = update_jkff(dsk.ff_45a,
			sysclka1[i] |
			(drive_ready_0(dsk.drive) ? JKFF_J : 0) |
			JKFF_K |
			JKFF_S |
			JKFF_C);

		/**
		 * sets the seqerr flip-flop 45b (Q' is SEQERR)
		 * JK flip-flop 45b (seqerr latch)
		 * <PRE>
		 * CLK	SYSCLKA'
		 * J	1
		 * K'	SEQERR'
		 * S'	CLRSTAT'
		 * C'	1
		 * Q	to KSTAT[11] DATALATE
		 * </PRE>
		 */
		DEBUG_NAME("		SEQERR 45b");
		dsk.ff_45b = update_jkff(dsk.ff_45b,
			sysclka1[i] |
			JKFF_J |
			(SEQERR ? JKFF_K : 1) |
			JKFF_S |
			JKFF_C);

		/**
		 * JK flip-flop 22b (sector task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 22a Q
		 * K'	(BLOCK & STSKACT)'
		 * S'	1 (really it's RESET')
		 * C'	1
		 * Q	STSKENA; Q' WAKEKST'
		 * </PRE>
		 */
		DEBUG_NAME("		KSEC 22b");
		dsk.ff_22b = update_jkff(dsk.ff_22b,
			sysclkb1[i] |
			((dsk.ff_22a & JKFF_Q) ? JKFF_J : 0) |
			((block == task_ksec) ? 0 : JKFF_K) |
			JKFF_S |
			JKFF_C);

		/**
		 * JK flip-flop 22a (sector task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 21b Q
		 * K'	1
		 * S'	1
		 * C'	WAKEST'
		 * Q	to 22b J
		 * </PRE>
		 */
		DEBUG_NAME("		KSEC 22a");
		dsk.ff_22a = update_jkff(dsk.ff_22a,
			sysclkb1[i] |
			((dsk.ff_21b & JKFF_Q) ? JKFF_J : 0) |
			JKFF_K |
			JKFF_S |
			((dsk.ff_22b & JKFF_Q) ? 0 : JKFF_C));

		/**
		 * JK flip-flop 21b (sector task)
		 * <PRE>
		 * CLK	SYSCLKB'
		 * J	from 21a Q
		 * K'	1
		 * S'	1
		 * C'	WAKEST'
		 * Q	to 22a J
		 * </PRE>
		 */
		DEBUG_NAME("		KSEC 21b");
		dsk.ff_21b = update_jkff(dsk.ff_21b,
			sysclkb1[i] |
			((dsk.ff_21a & JKFF_Q) ? JKFF_J : 0) |
			JKFF_K |
			JKFF_S |
			((dsk.ff_22b & JKFF_Q) ? 0 : JKFF_C));
	}

	/* The 53b FF Q output is the WDINIT signal. */
	if (WDINIT != dsk.wdinit) {
		dsk.wdinit0 = dsk.wdinit;
		/* rising edge immediately */
		if ((dsk.wdinit = WDINIT) == 1)
			dsk.wdinit0 = 1;
		LOG((log_DSK,2,"	WDINIT:%d\n", dsk.wdinit));
	}

	/*
	 * If Q (53a) and Q (43a) are both 1, the WAKEKWDT'
	 * output is 0 and the disk word task wakeup signal is asserted.
	 */
	if (dsk.ff_53a & dsk.ff_43a & JKFF_Q) {
		if (dsk.wdtskena == 1) {
			LOG((log_DSK,2,"	WDTSKENA':0 and WAKEKWDT':0 wake KWD\n"));
			dsk.wdtskena = 0;
			CPU_SET_TASK_WAKEUP(task_kwd);
		}
	} else if (dsk.ff_43a & JKFF_Q) {
		/*
		 * If Q (43a) is 1, the WDTSKENA' signal is deasserted.
		 */
		if (dsk.wdtskena == 0) {
			LOG((log_DSK,2,"	WDTSKENA':1\n"));
			dsk.wdtskena = 1;
			CPU_CLR_TASK_WAKEUP(task_kwd);
		}
	}

	if (dsk.kfer) {
		/* no fatal error: ready and not seqerr and seekok */
		if (!RDYLAT && !SEQERR && SEEKOK) {
			LOG((log_DSK,2,"	reset KFER\n"));
			dsk.kfer = 0;
		}
	} else {
		/* fatal error: not ready or seqerr or not seekok */
		if (RDYLAT) {
			LOG((log_DSK,2,"	RDYLAT sets KFER\n"));
			dsk.kfer = 1;
		} else if (SEQERR) {
			LOG((log_DSK,2,"	SEQERR sets KFER\n"));
			dsk.kfer = 1;
		} else if (!SEEKOK) {
			LOG((log_DSK,2,"	not SEEKOK sets KFER\n"));
			dsk.kfer = 1;
		}
	}

	/*
	 * The FF 22b Q output is the STSKENA signal,
	 * the Q' is the WAKEKST' signal.
	 */
	if (dsk.ff_22b & JKFF_Q) {
		if (!CPU_GET_TASK_WAKEUP(task_ksec)) {
			LOG((log_DSK,2,"	STSKENA:1; WAKEST':0 wake KSEC\n"));
			CPU_SET_TASK_WAKEUP(task_ksec);
		}
	} else {
		if (CPU_GET_TASK_WAKEUP(task_ksec)) {
			LOG((log_DSK,2,"	STSKENA:0; WAKEST':1\n"));
			CPU_CLR_TASK_WAKEUP(task_ksec);
		}
	}

	/**
	 * JK flip-flop 21a (sector task)
	 * <PRE>
	 * CLK	SECT4 (inverted sector mark from drive)
	 * J	WAKEST'
	 * K'	1
	 * S'	ERRWAKE'
	 * C'	WAKEST'
	 * Q	to seclate monoflop
	 * </PRE>
	 */
	DEBUG_NAME("		KSEC 21a");
	dsk.ff_21a = update_jkff(dsk.ff_21a,
		(drive_sector_mark_0(dsk.drive) ? 0 : JKFF_CLK) |
		((dsk.ff_22b & JKFF_Q) ? 0 : JKFF_J) |
		JKFF_K |
		(ERRWAKE ? 0 : JKFF_S) |
		((dsk.ff_22b & JKFF_Q) ? 0 : JKFF_C));

	/*
	 * If the KSEC FF 21a Q goes 1, pulse the SECLATE signal for
	 * some time.
	 */
	if (!(dsk.ff_21a_old & JKFF_Q) && (dsk.ff_21a & JKFF_Q)) {
		if (dsk.seclate_id)
			timer_remove(dsk.seclate_id);
		dsk.seclate_id = timer_insert(TW_SECLATE, disk_seclate, 1, "seclate");
		if (dsk.seclate) {
			dsk.seclate = 0;
			LOG((log_DSK,4,"	SECLATE -> 0 pulse until %lldns\n",
				ntime() + TW_SECLATE));
		}
	}

	/*
	 * check if write and erase gate, or read gate are changed
	 */
	if (CPU_GET_TASK_WAKEUP(task_ksec) || GET_KCOM_XFEROFF(dsk.kcom) || dsk.kfer) {
		drive_egate(dsk.drive, 1);
		drive_wrgate(dsk.drive, 1);
		drive_rdgate(dsk.drive, 1);
	} else {
		/* enable either read or write gates depending on current record R/W */
		if (dsk.krwc & RWC_WRITE) {
			/* enable erase and write gates only if OKTORUN is high */
			if (dsk.ok_to_run) {
				drive_egate(dsk.drive, 0);
				drive_wrgate(dsk.drive, 0);
			}
		} else {
			/* enable read gate */
			drive_rdgate(dsk.drive, 0);
		}
	}

	dsk.ff_21a_old = dsk.ff_21a;
	dsk.bitclk = bitclk;
	dsk.datin = datin;
	LOG((log_DSK,5,"	<<< KWD timing\n"));
}


/** @brief timer callback to take away the SECLATE pulse (monoflop) */
static void disk_seclate(int id, int arg)
{
	LOG((log_DSK,2,"	SECLATE -> %d\n", arg));
	dsk.seclate = arg;
	dsk.seclate_id = 0;
}

/** @brief timer callback to take away the OK TO RUN pulse (reset) */
static void disk_ok_to_run(int id, int arg)
{
	LOG((log_DSK,2,"	OK TO RUN -> %d\n", arg));
	dsk.ok_to_run = arg;
}

/**
 * @brief timer callback to pulse the STROBE' signal to the drive
 *
 * STROBE' pulses are sent to the drive at a rate that depends on
 * the monoflop 52b external resistor and capacitor.
 *
 * The drive compares the cylinder number that is presented on
 * it's inputs against the current cylinder, and if they don't
 * match steps into the corresponding direction.
 *
 * On the falling edge of a strobe, the drive sets the log_addx_interlock
 * flag 0 (LAI, active low). On the rising edge of the strobe the drive then
 * indicates seek completion by setting addx_acknowledge to 0 (ADDRACK, active low).
 * If the seek is not yet complete, it instead keeps the seek_incomplete
 * flag 0 (SKINC, active low). If the seek would go beyond the last cylinder,
 * the drive deasserts seek_incomplete, but does not assert the addx_acknowledge.
 *
 * @param id timer id
 * @param arg contains the drive, cylinder, and restore flag
 */
static void disk_strobon(int id, int arg)
{
	int unit = arg % 2;
	int restore = (arg / 2) % 2;
	int cylinder = arg / 4;
	int seekok;
	int lai;
	int strobe;

	LOG((log_DSK,2,"	STROBE #%d restore:%d cylinder:%d\n",
		unit, restore, cylinder));

	/* This is really monoflop 52a generating a very short 0 pulse */
	for (strobe = 0; strobe < 2; strobe++) {
		/* pulse the strobe signal to the unit */
		drive_strobe(unit, cylinder, restore, strobe);

		lai = drive_log_addx_interlock_0(unit);
		LOG((log_DSK,6,"		LAI':%d\n", lai));
		/**
		 * JK flip-flop 44a (LAI' clocked)
		 * <PRE>
		 * CLK	LAI
		 * J	1
		 * K'	1
		 * S'	1
		 * C'	CLRSTAT' (not now)
		 * Q	to seekok
		 * </PRE>
		 */
		DEBUG_NAME("		LAI 44a");
		dsk.ff_44a = update_jkff(dsk.ff_44a,
			(lai ? JKFF_CLK : 0) |
			JKFF_J |
			JKFF_K |
			JKFF_S |
			JKFF_C);
		if (drive_addx_acknowledge_0(unit) == 0 &&
			(dsk.ff_44a & JKFF_Q)) {
			/* if address is acknowledged, and Q' of FF 44a, clear the strobe */
			dsk.strobe = 0;
		}
	}

	if (drive_addx_acknowledge_0(unit)) {

		/* no acknowledge yet */

	} else {
		/* clear the monoflop 52b, i.e. no timer restart */
		LOG((log_DSK,2,"		STROBON:%d\n", dsk.strobe));

		/* update the seekok status: SKINC' && LAI' && Q' of FF 44a */
		seekok = drive_seek_incomplete_0(unit);
		if (seekok != dsk.seekok) {
			dsk.seekok = seekok;
			LOG((log_DSK,2,"		SEEKOK:%d\n", dsk.seekok));
		}
	}


	LOG((log_DSK,2,"	current cylinder:%d\n",
		drive_cylinder(unit) ^ DRIVE_CYLINDER_MASK));

	/* if the strobe is still set, restart the timer */
	if (dsk.strobe)
		timer_insert(TW_STROBON, disk_strobon, arg, "strobe");
}

/** @brief timer callback to change the READY monoflop 31a */
static void disk_ready_mf31a(int id, int arg)
{
	dsk.ready_mf31a = arg & drive_ready_0(dsk.drive);
	/* log the not ready result with level 0, else 2 */
	LOG((log_DSK,dsk.ready_mf31a ? 0 : 2,
		"	ready mf31a:%d\n", dsk.ready_mf31a));
}

/**
 * @brief called if one of the disk tasks (task_kwd or task_ksec) blocks
 *
 * @param task task that blocks (either task_ksec or task_kwd)
 */
void disk_block(int task)
{
	kwd_timing(dsk.bitclk, dsk.datin, task);
}

/**
 * @brief bus driven by disk status register KSTAT
 * <PRE>
 * Part of the KSTAT register is made of two 4 bit latches S8T10 (Signetics).
 * The signals BUS[8-11] are the current state of:
 *     BUS[0-3]   SECT[0-3]; from the Winchester drive (inverted)
 *     BUS[8]     SEEKOK'
 *     BUS[9]     SRWRDY' from the Winchester drive
 *     BUS[10]    RDYLAT' (latched READY' at last CLRSTAT, FF 45a output Q)
 *     BUS[11]    SEQERR (latched SEQERR at last CLRSTAT, FF 45b output Q')
 * The signals BUS[12,14-15] are just as they were loaded at KSTAT<- time.
 *     BUS[13]    CHSEMERROR (FF 44b output Q' inverted)
 * </PRE>
 */
void bs_read_kstat_0(void)
{
	int unit = dsk.drive;
	int and;

	/* KSTAT[4-7] bus is open */
	PUT_KSTAT_DONE(dsk.kstat, 017);

	/* KSTAT[8] latch the inverted seekok status */
	PUT_KSTAT_SEEKFAIL(dsk.kstat, dsk.seekok ? 0 : 1);

	/* KSTAT[9] latch the drive seek/read/write status */
	PUT_KSTAT_SEEK(dsk.kstat, drive_seek_read_write_0(unit));

	/* KSTAT[10] latch the latched (FF 45a at CLRSTAT) ready status */
	PUT_KSTAT_NOTRDY(dsk.kstat, dsk.ff_45a & JKFF_Q ? 1 : 0);

	/* KSTAT[11] latch the latched (FF 45b at CLRSTAT) seqerr status (Q') */
	PUT_KSTAT_DATALATE(dsk.kstat, dsk.ff_45b & JKFF_Q ? 0 : 1);

	/* KSTAT[13] latch the latched (FF 44b at CLRSTAT/KSTAT<-) checksum status */
	PUT_KSTAT_CKSUM(dsk.kstat, dsk.ff_44b & JKFF_Q ? 1 : 0);

	and = dsk.kstat;

	LOG((0,1,"	<-KSTAT; BUS &= %#o\n",
		and));
	LOG((0,2,"		sector:    %d\n", GET_KSTAT_SECTOR(dsk.kstat)));
	LOG((0,2,"		done:      %d\n", GET_KSTAT_DONE(dsk.kstat)));
	LOG((0,2,"		seekfail:  %d\n", GET_KSTAT_SEEKFAIL(dsk.kstat)));
	LOG((0,2,"		seek:      %d\n", GET_KSTAT_SEEK(dsk.kstat)));
	LOG((0,2,"		notrdy:    %d\n", GET_KSTAT_NOTRDY(dsk.kstat)));
	LOG((0,2,"		datalate:  %d\n", GET_KSTAT_DATALATE(dsk.kstat)));
	LOG((0,2,"		idle:      %d\n", GET_KSTAT_IDLE(dsk.kstat)));
	LOG((0,2,"		cksum:     %d\n", GET_KSTAT_CKSUM(dsk.kstat)));
	LOG((0,2,"		completion:%d\n", GET_KSTAT_COMPLETION(dsk.kstat)));

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
	and = dsk.datain;
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
	if (GET_KCOM_SENDADR(dsk.kcom)) {
		LOG((0,1,"	STROBE (SENDADR:1)\n"));
		/* Set the STROBON flag and start the STROBON monoflop */
		dsk.strobe = 1;
		disk_strobon(0,
			4 * GET_KADDR_CYLINDER(dsk.kaddr) +
			2 * GET_KADDR_RESTORE(dsk.kaddr) +
			dsk.drive);
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
	int i;
	LOG((0,1,"	KSTAT<-; BUS[12-15] %#o\n", cpu.bus));
	LOG((0,2,"		idle:      %d\n", GET_KSTAT_IDLE(cpu.bus)));
	LOG((0,2,"		cksum:     %d\n", GET_KSTAT_CKSUM(cpu.bus)));
	LOG((0,2,"		completion:%d\n", GET_KSTAT_COMPLETION(cpu.bus)));

	/* KSTAT[12] is just taken from BUS[12] */
	PUT_KSTAT_IDLE(dsk.kstat, GET_KSTAT_IDLE(cpu.bus));

	/* KSTAT[14-15] are just taken from BUS[14-15] */
	PUT_KSTAT_COMPLETION(dsk.kstat, GET_KSTAT_COMPLETION(cpu.bus));

	/* May set the the CKSUM flip-flop 44b
	 * JK flip-flop 44b (KSTAT<- clocked)
	 * CLK	SYSCLKA'
	 * J	!BUS[13]
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT' (not now)
	 * Q	Q' inverted to BUS[13] on <-KSTAT
	 */
	for (i = 0; i < 2; i++) {
		DEBUG_NAME("		CKSUM 44b");
		dsk.ff_44b = update_jkff(dsk.ff_44b,
			(i ? JKFF_CLK : 0) |
			(ALTO_GET(cpu.bus,16,13,13) ? 0 : JKFF_J) |
			JKFF_K |
			JKFF_S |
			JKFF_C);
	}
}

/**
 * @brief load data out register, or the disk address register
 *
 * KDATA is loaded from BUS.
 */
void f1_load_kdata_1(void)
{
	dsk.dataout = cpu.bus;
	if (GET_KCOM_SENDADR(dsk.kcom)) {
		PUT_KADDR_SECTOR(dsk.kaddr,
			GET_KADDR_SECTOR(cpu.bus));
		PUT_KADDR_CYLINDER(dsk.kaddr,
			GET_KADDR_CYLINDER(cpu.bus));
		PUT_KADDR_HEAD(dsk.kaddr,
			GET_KADDR_HEAD(cpu.bus));
		PUT_KADDR_DRIVE(dsk.kaddr,
			GET_KADDR_DRIVE(cpu.bus));
		PUT_KADDR_RESTORE(dsk.kaddr,
			GET_KADDR_RESTORE(cpu.bus));
		PUT_KADDR_DRIVE(dsk.kaddr,
			GET_KADDR_DRIVE(cpu.bus));
		dsk.drive = GET_KADDR_DRIVE(dsk.kaddr);

		/* XXX: update drive activity indicators */
		disk_show_indicators(dsk.drive);

		LOG((0,1,"	KDATA<-; BUS (%#o) (drive:%d restore:%d %d/%d/%02d page:%d)\n",
			cpu.bus,
			GET_KADDR_DRIVE(dsk.kaddr),
			GET_KADDR_RESTORE(dsk.kaddr),
			GET_KADDR_CYLINDER(dsk.kaddr),
			GET_KADDR_HEAD(dsk.kaddr),
			GET_KADDR_SECTOR(dsk.kaddr),
			DRIVE_PAGE(GET_KADDR_CYLINDER(dsk.kaddr),
				GET_KADDR_HEAD(dsk.kaddr),
				GET_KADDR_SECTOR(dsk.kaddr))));
#if	0
		/* printing changes in the disk address */
		{
			static int last_kaddr;
			if (dsk.kaddr != last_kaddr) {
				int c = GET_KADDR_CYLINDER(dsk.kaddr);
				int h = GET_KADDR_HEAD(dsk.kaddr);
				int s = GET_KADDR_SECTOR(dsk.kaddr);
				int page = DRIVE_PAGE(c,h,s);
				last_kaddr = dsk.kaddr;
				printf("	unit:%d restore:%d %d/%d/%02d page:%d\n",
					GET_KADDR_DRIVE(dsk.kaddr),
					GET_KADDR_RESTORE(dsk.kaddr),
					c, h, s, page);
			}
		}
#endif
	} else {
		LOG((0,1,"	KDATA<-; BUS %#o (%#x)\n", cpu.bus, cpu.bus));
	}
}

/**
 * @brief advances shift registers holding KADR
 *
 * Advances the shift registers holding the KADR register so that they
 * present the number and read/write/check status of the next record
 * to the hardware.
 *
 * <PRE>
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
 * </PRE>
 */
void f1_increcno_1(void)
{
	switch (dsk.krecno) {
	case RECNO_HEADER:
		dsk.krecno = RECNO_LABEL;
		dsk.krwc = GET_KADR_LABEL(dsk.kadr);
		LOG((0,2,"	INCRECNO; HEADER -> LABEL (%o, rwc:%o)\n",
			dsk.krecno, dsk.krwc));
		break;
	case RECNO_PAGENO:
		dsk.krecno = RECNO_HEADER;
		dsk.krwc = GET_KADR_HEADER(dsk.kadr);
		LOG((0,2,"	INCRECNO; PAGENO -> HEADER (%o, rwc:%o)\n",
			dsk.krecno, dsk.krwc));
		break;
	case RECNO_LABEL:
		dsk.krecno = RECNO_DATA;
		dsk.krwc = GET_KADR_DATA(dsk.kadr);
		LOG((0,2,"	INCRECNO; LABEL -> DATA (%o, rwc:%o)\n",
			dsk.krecno, dsk.krwc));
		break;
	case RECNO_DATA:
		dsk.krecno = RECNO_PAGENO;
		dsk.krwc = 0;	/* read (?) */
		LOG((0,2,"	INCRECNO; DATA -> PAGENO (%o, rwc:%o)\n",
			dsk.krecno, dsk.krwc));
		break;
	}
	disk_show_indicators(dsk.drive);
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

	/* clears the LAI clocked flip-flop 44a
	 * JK flip-flop 44a (LAI' clocked)
	 * CLK	(LAI')'
	 * J	1
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	to seekok
	 */
	DEBUG_NAME("		LAI 44a");
	dsk.ff_44a = update_jkff(dsk.ff_44a,
		(dsk.ff_44a & JKFF_CLK) |
		JKFF_J |
		JKFF_K |
		JKFF_S |
		0);

	/* clears the CKSUM flip-flop 44b
	 * JK flip-flop 44b (KSTAT<- clocked)
	 * CLK	SYSCLKA' (not used here, just clearing)
	 * J	1 (BUS[13] during KSTAT<-)
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	to seekok
	 */
	DEBUG_NAME("		CKSUM 44b");
	dsk.ff_44b = update_jkff(dsk.ff_44b,
		(dsk.ff_44b & JKFF_CLK) |
		(dsk.ff_44b & JKFF_J) |
		JKFF_K |
		JKFF_S |
		0);

	/* clears the rdylat flip-flop 45a
	 * JK flip-flop 45a (ready latch)
	 * CLK	SYSCLKA'
	 * J	READY' from drive
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	RDYLAT'
	 */
	DEBUG_NAME("		RDYLAT 45a");
	dsk.ff_45a = update_jkff(dsk.ff_45a,
		(dsk.ff_45a & JKFF_CLK) |
		drive_ready_0(dsk.drive) |
		JKFF_K |
		JKFF_S |
		0);

	/* sets the seqerr flip-flop 45b (Q' is SEQERR)
	 * JK flip-flop 45b (seqerr latch)
	 * CLK	SYSCLKA'
	 * J	1
	 * K'	SEQERR'
	 * S'	CLRSTAT'
	 * C'	1
	 * Q	to KSTAT[11] DATALATE
	 */
	DEBUG_NAME("		SEQERR 45b");
	dsk.ff_45b = update_jkff(dsk.ff_45b,
		(dsk.ff_45b & JKFF_CLK) |
		JKFF_J |
		(dsk.ff_45b & JKFF_K) |
		0 |
		JKFF_C);

	/* set or reset monoflop 31a, depending on drive READY' */
	dsk.ready_mf31a = drive_ready_0(dsk.drive);

	/* start monoflop 31a, which resets ready_mf31a */
	timer_insert(TW_READY, disk_ready_mf31a, 1, "drive READY'");
}

/**
 * @brief load the KCOM register from bus
 * <PRE>
 * This causes the KCOM register to be loaded from BUS[1-5]. The
 * KCOM register has the following interpretation:
 *	(1) XFEROFF = 1 inhibits data transmission to/from the dsk.
 *	(2) WDINHIB = 1 prevents the disk word task from awakening.
 * 	(3) BCLKSRC = 0 takes bit clock from disk input or crystal clock,
 * 	    as appropriate. BCLKSRC = 1 force use of crystal clock.
 *	(4) WFFO = 0 holds the disk bit counter at -1 until a 1 bit is read.
 *	    WFFO = 1 allows the bit counter to proceed normally.
 *	(5) SENDADR = 1 causes KDATA[4-12] and KDATA[15] to be transmitted
 *	    to disk unit as track address. SENDADR = 0 inhibits such
 *	    transmission.
 * </PRE>
 */
void f1_load_kcom_1(void)
{
	if (dsk.kcom != cpu.bus) {
		dsk.kcom = cpu.bus;
		LOG((0,2,"	KCOM<-; BUS %06o\n", dsk.kcom));
		LOG((0,2,"		xferoff: %d\n", GET_KCOM_XFEROFF(dsk.kcom)));
		LOG((0,2,"		wdinhib: %d\n", GET_KCOM_WDINHIB(dsk.kcom)));
		LOG((0,2,"		bclksrc: %d\n", GET_KCOM_BCLKSRC(dsk.kcom)));
		LOG((0,2,"		wffo:    %d\n", GET_KCOM_WFFO(dsk.kcom)));
		LOG((0,2,"		sendadr: %d\n", GET_KCOM_SENDADR(dsk.kcom)));

		/* XXX: update drive activity indicators */
		disk_show_indicators(dsk.drive);
	}
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
	PUT_KADR_SEAL(dsk.kadr, GET_KADR_SEAL(cpu.bus));
	PUT_KADR_HEADER(dsk.kadr, GET_KADR_HEADER(cpu.bus));
	PUT_KADR_LABEL(dsk.kadr, GET_KADR_LABEL(cpu.bus));
	PUT_KADR_DATA(dsk.kadr, GET_KADR_DATA(cpu.bus));
	PUT_KADR_NOXFER(dsk.kadr, GET_KADR_NOXFER(cpu.bus));
	PUT_KADR_UNUSED(dsk.kadr, GET_KADR_UNUSED(cpu.bus));

	/* get selected drive from DATA[14] output (FF 67a really) */
	unit = GET_KADDR_DRIVE(dsk.kaddr);

	/* get the selected head, and select drive unit and head */
	/* latch head from DATA[13] */
	head = GET_KADDR_HEAD(dsk.dataout);
	PUT_KADDR_HEAD(dsk.kaddr, head);
	drive_select(unit, head);

	/* On KDAR<- bit 0 of parts #36 and #37 is reset to 0, i.e. recno = 0 */
	dsk.krecno = 0;
	/* current read/write/check is that for the header */
	dsk.krwc = GET_KADR_HEADER(dsk.kadr);

	LOG((0,1,"	KADR<-; BUS[8-14] #%o\n", dsk.kadr));
	LOG((0,2,"		seal:   %#o\n", GET_KADR_SEAL(dsk.kadr)));
	LOG((0,2,"		header: %s\n",  rwc_name[GET_KADR_HEADER(dsk.kadr)]));
	LOG((0,2,"		label:  %s\n",  rwc_name[GET_KADR_LABEL(dsk.kadr)]));
	LOG((0,2,"		data:   %s\n",  rwc_name[GET_KADR_DATA(dsk.kadr)]));
	LOG((0,2,"		noxfer: %d\n",  GET_KADR_NOXFER(dsk.kadr)));
	LOG((0,2,"		unused: %d (drive?)\n",  GET_KADR_UNUSED(dsk.kadr)));

	disk_show_indicators(dsk.drive);
}

/**
 * @brief branch on disk word task active and init
 *
 * NEXT <- NEXT OR (WDTASKACT && WDINIT ? 037 : 0)
 */
void f2_init_1(void)
{
	int or = INIT ? 037 : 0;

	LOG((0,1,"	INIT; %sbranch (%#o | %#o)\n",
		or ? "" : "no ", cpu.next2, or));
	CPU_BRANCH(or);
	dsk.wdinit0 = 0;
}

/**
 * @brief branch on read/write/check state of the current record
 * <PRE>
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
 * </PRE>
 */
void f2_rwc_1(void)
{
	static int branch_map[4] = {0,2,3,3};
	int or = branch_map[dsk.krwc];;
	int init = INIT ? 037 : 0;

	switch (dsk.krecno) {
	case RECNO_HEADER:
		LOG((0,1,"	RWC; %sbranch header(%d):%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", dsk.krecno,
			rwc_name[dsk.krwc], cpu.next2, or, init));
		break;
	case RECNO_PAGENO:
		LOG((0,1,"	RWC; %sbranch pageno(%d):%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", dsk.krecno,
			rwc_name[dsk.krwc], cpu.next2, or, init));
		break;
	case RECNO_LABEL:
		LOG((0,1,"	RWC; %sbranch label(%d):%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", dsk.krecno,
			rwc_name[dsk.krwc], cpu.next2, or, init));
		break;
	case RECNO_DATA:
		LOG((0,1,"	RWC; %sbranch data(%d):%s (%#o|%#o|%#o)\n",
			(or | init) ? "" : "no ", dsk.krecno,
			rwc_name[dsk.krwc], cpu.next2, or, init));
		break;
	}
	CPU_BRANCH(or | init);
	dsk.wdinit0 = 0;
}

/**
 * @brief branch on the current record number by a lookup table
 * <PRE>
 * NEXT <- NEXT OR MAP (current record number) where
 *   MAP(0) = 0		(header)
 *   MAP(1) = 2		(label)
 *   MAP(2) = 3		(pageno)
 *   MAP(3) = 1		(data)
 * </PRE>
 * NB: The map isn't needed, because dsk.krecno counts exactly this way.
 */
void f2_recno_1(void)
{
	int or = dsk.krecno;
	int init = INIT ? 037 : 0;

	LOG((0,1,"	RECNO; %sbranch recno:%d (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", dsk.krecno, cpu.next2, or, init));
	CPU_BRANCH(or | init);
	dsk.wdinit0 = 0;
}

/**
 * @brief branch on the data transfer state
 *
 * NEXT <- NEXT OR (if current command wants data transfer ? 1 : 0)
 */
void f2_xfrdat_1(void)
{
	int or = GET_KADR_NOXFER(dsk.kadr) ? 0 : 1;
	int init = INIT ? 037 : 0;

	LOG((0,1,"	XFRDAT; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
	dsk.wdinit0 = 0;
}

/**
 * @brief branch on the disk ready signal
 *
 * NEXT <- NEXT OR (if disk not ready to accept command ? 1 : 0)
 */
void f2_swrnrdy_1(void)
{
	int or = drive_seek_read_write_0(dsk.drive);
	int init = INIT ? 037 : 0;

	LOG((0,1,"	SWRNRDY; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
	dsk.wdinit0 = 0;
}

/**
 * @brief branch on the disk fatal error condition
 *
 * NEXT <- NEXT OR (if fatal error in latches ? 0 : 1)
 */
void f2_nfer_1(void)
{
	int or = dsk.kfer ? 0 : 1;
	int init = INIT ? 037 : 0;

	LOG((0,1,"	NFER; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
	dsk.wdinit0 = 0;
}

/**
 * @brief branch on the seek busy status
 *
 * NEXT <- NEXT OR (if seek strobe still on ? 1 : 0)
 * <PRE>
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
 * Rt = 20k, Cext = 0.01µF (=10000pF) => 57960ns (~= 58µs)
 * </PRE>
 */
void f2_strobon_1(void)
{
	int or = dsk.strobe;
	int init = INIT ? 037 : 0;

	LOG((0,2,"	STROBON; %sbranch (%#o|%#o|%#o)\n",
		(or | init) ? "" : "no ", cpu.next2, or, init));
	CPU_BRANCH(or | init);
	dsk.wdinit0 = 0;
}

/**
 * @brief update the disk controller with a new bitclk
 *
 * @param id timer id
 * @param arg bit number
 */
static void disk_bitclk(int id, int arg)
{
	int bits = drive_bits_per_sector();
	int clk = arg & 1;
	int bit = 0;

	/**
	 * The source for BITCLK and DATAIN depends on disk controller part #65
	 * <PRE>
	 *  BCLKSRC  W/R | BITCLK | DATAIN
	 * --------------+--------+---------
	 *    0       0  |  RDCLK | RDDATA
	 *    0       1  |  CLK/2 | DATOUT
	 *    1       0  |  CLK/2 | RDDATA
	 *    1       1  |  CLK/2 | DATOUT
	 * </PRE>
	 */
	if (dsk.krwc & RWC_WRITE) {
		bit = (dsk.shiftout >> 15) & 1;
		kwd_timing(clk, bit, 0);
		if (GET_KCOM_XFEROFF(dsk.kcom)) {
			/* do anything, if the transfer is off? */
		} else {
			LOG((log_DSK,7,"	BITCLK#%d bit:%d (write) @%lldns\n",
				arg, bit, ntime()));
			if (clk)
				drive_wrdata(dsk.drive, arg, bit);
			else
				drive_wrdata(dsk.drive, arg, 1);
		}
	} else if (GET_KCOM_BCLKSRC(dsk.kcom)) {
		/* always select the crystal clock */
		bit = drive_rddata(dsk.drive, arg);
		LOG((log_DSK,7,"	BITCLK#%d bit:%d (read, crystal) @%lldns\n",
			arg, bit, ntime()));
		kwd_timing(clk, bit, 0);
	} else {
		/* if XFEROFF is set, keep the bit at 1 (RDGATE' is high) */
		if (GET_KCOM_XFEROFF(dsk.kcom)) {
			bit = 1;
		} else {
			clk = drive_rdclk(dsk.drive, arg);
			bit = drive_rddata(dsk.drive, arg);
			LOG((log_DSK,7,"	BITCLK#%d bit:%d (read, driveclk) @%lldns\n",
				arg, bit, ntime()));
		}
		kwd_timing(clk, bit, 0);
	}

	/* more bits to clock? */
	arg++;
	if (arg < bits)
		timer_insert(drive_bit_time(dsk.drive), disk_bitclk,
			arg, "disk bitclk");
}

/**
 * @brief this callback is called by the drive timer whenever a new sector starts
 *
 * @param unit the unit number
 */
void disk_sector_start(int unit)
{
	if (dsk.bitclk_id) {
		LOG((log_DSK,0,"	unit #%d stop bitclk\n", dsk.bitclk));
		timer_remove(dsk.bitclk_id);
		dsk.bitclk_id = 0;
	}

	/* KSTAT[0-3] update the current sector in the kstat field */
	PUT_KSTAT_SECTOR(dsk.kstat, drive_sector(unit) ^ DRIVE_SECTOR_MASK);

	/* clear input and output shift registers (?) */
	dsk.shiftin = 0;
	dsk.shiftout = 0;

	LOG((log_DSK,1,"	unit #%d sector %d start\n",
		unit, GET_KSTAT_SECTOR(dsk.kstat)));

	/* HACK: no command, no bit clock */
	if (debug_read_mem(0521))
		/* start a timer chain for the bit clock */
		disk_bitclk(0, 0);
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
	memset(&dsk, 0, sizeof(dsk));

	dsk.wdtskena = 1;

	dsk.seclate = 0;
	timer_insert(TW_SECLATE, disk_seclate, 1, "seclate");
	dsk.ok_to_run = 0;
	timer_insert(15 * CPU_MICROCYCLE_TIME, disk_ok_to_run, 1, "ok to run");

	/* install a callback to be called whenever a drive sector starts */
	drive_sector_callback(disk_sector_start);

	/* update the KCOM indicators for drive 0 and 1 */
	dsk.kcom = 066000;
	disk_show_indicators(0);
	disk_show_indicators(1);
	dsk.kcom = 0;

	return 0;
}
