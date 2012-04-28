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
 * Ethernet bus source and F1 functions
 *
 * $Id: ether.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "debug.h"
#include "ether.h"

/** @brief the ethernet context */
ethernet_t eth;

/** @brief the ethernet ID of this machine */
uint8_t ether_id = 254;

/**
 * @brief BPROMs P3601-1; 256x4; enet.a41 "PE1" and enet.a42 "PE2"
 *
 * Phase encoder
 *
 * a41: P3601-1; 256x4; "PE1"
 * a42: P3601-1; 256x4; "PE2"
 *
 * PE1/PE2 inputs
 * ----------------
 * A0  (5) OUTGO
 * A1  (6) XDATA
 * A2  (7) OSDATAG
 * A3  (4) XCLOCK
 * A4  (3) OCNTR0
 * A5  (2) OCNTR1
 * A6  (1) OCNTR2
 * A7 (15) OCNTR3
 *
 * PE1 outputs
 * ----------------
 * D0 (12) OCNTR0
 * D1 (11) OCNTR1
 * D2 (10) OCNTR2
 * D3  (9) OCNTR3
 *
 * PE2 outputs
 * ----------------
 * D0 (12) n.c.
 * D1 (11) to OSLOAD flip flop J and K'
 * D2 (10) XDATA
 * D3  (9) XCLOCK
 */
uint8_t ether_a41[256];
uint8_t ether_a42[256];

/**
 * @brief BPROM; P3601-1; 265x4 enet.a49 "AFIFO"
 *
 * Perhaps try with the contents of the display FIFO, as it is
 * the same type and the display FIFO has the same size.
 *
 * FIFO control
 *
 * a49: P3601-1; 256x4; "AFIFO"
 *
 * inputs
 * ----------------
 * A0  (5) fifo_wr[0]
 * A1  (6) fifo_wr[1]
 * A2  (7) fifo_wr[2]
 * A3  (4) fifo_wr[3]
 * A4  (3) fifo_rd[0]
 * A5  (2) fifo_rd[1]
 * A6  (1) fifo_rd[2]
 * A7 (15) fifo_rd[3]
 *
 * outputs active low
 * ----------------------------
 * D0 (12) BE'    (buffer empty)
 * D1 (11) BNE'   (buffer next empty ?)
 * D2 (10) BNNE'  (buffer next next empty ?)
 * D3  (9) BF'    (buffer full)
 *
 * Data from enet.a49 after address line reversal:
 * 000: 010 007 017 017 017 017 017 017 017 017 017 017 017 017 013 011
 * 020: 011 010 007 017 017 017 017 017 017 017 017 017 017 017 017 013
 * 040: 013 011 010 007 017 017 017 017 017 017 017 017 017 017 017 017
 * 060: 017 013 011 010 007 017 017 017 017 017 017 017 017 017 017 017
 * 100: 017 017 013 011 010 007 017 017 017 017 017 017 017 017 017 017
 * 120: 017 017 017 013 011 010 007 017 017 017 017 017 017 017 017 017
 * 140: 017 017 017 017 013 011 010 007 017 017 017 017 017 017 017 017
 * 160: 017 017 017 017 017 013 011 010 007 017 017 017 017 017 017 017
 * 200: 017 017 017 017 017 017 013 011 010 007 017 017 017 017 017 017
 * 220: 017 017 017 017 017 017 017 013 011 010 007 017 017 017 017 017
 * 240: 017 017 017 017 017 017 017 017 013 011 010 007 017 017 017 017
 * 260: 017 017 017 017 017 017 017 017 017 013 011 010 007 017 017 017
 * 300: 017 017 017 017 017 017 017 017 017 017 013 011 010 007 017 017
 * 320: 017 017 017 017 017 017 017 017 017 017 017 013 011 010 007 017
 * 340: 017 017 017 017 017 017 017 017 017 017 017 017 013 011 010 007
 * 360: 007 017 017 017 017 017 017 017 017 017 017 017 017 013 011 010
 */
uint8_t ether_a49[256];


#define	BREATHLEN	0400	/* ethernet packet length */
#define	BREATHADDR	0177400	/* dest,,source */
#define	BREATHTYPE	0000602	/* ethernet packet type */
static const int duckbreath[BREATHLEN] =
{
	BREATHADDR,		/* 3MB dest,,source */ 
	BREATHTYPE,		/* ether packet type  */
	/* the rest is the contents of a breath of life packet.
	 * see <altosource>etherboot.dm (etherboot.asm) for alto
	 * assembly code.
	 */
			  0022574, 0100000, 0040437, 0102000, 0034431, 0164000,
	0061005, 0102460, 0024567, 0034572, 0061006, 0024565, 0034570, 0061006,
	0024564, 0034566, 0061006, 0020565, 0034565, 0061005, 0125220, 0046573,
	0020576, 0061004, 0123400, 0030551, 0041211, 0004416, 0000000, 0001000,
	0000026, 0000244, 0000000, 0000000, 0000000, 0000000, 0000004, 0000000,
	0000000, 0000020, 0177777, 0055210, 0025400, 0107000, 0045400, 0041411,
	0020547, 0041207, 0020544, 0061004, 0006531, 0034517, 0030544, 0051606,
	0020510, 0041605, 0042526, 0102460, 0041601, 0020530, 0061004, 0021601,
	0101014, 0000414, 0061020, 0014737, 0000773, 0014517, 0000754, 0020517,
	0061004, 0030402, 0002402, 0000000, 0000732, 0034514, 0162414, 0000746,
	0021001, 0024511, 0106414, 0000742, 0021003, 0163400, 0035005, 0024501,
	0106415, 0175014, 0000733, 0021000, 0042465, 0034457, 0056445, 0055775,
	0055776, 0101300, 0041400, 0020467, 0041401, 0020432, 0041402, 0121400,
	0041403, 0021006, 0041411, 0021007, 0041412, 0021010, 0041413, 0021011,
	0041406, 0021012, 0041407, 0021013, 0041410, 0015414, 0006427, 0012434,
	0006426, 0020421, 0024437, 0134000, 0030417, 0002422, 0177035, 0000026,
	0000415, 0000427, 0000567, 0000607, 0000777, 0177751, 0177641, 0177600,
	0000225, 0177624, 0001013, 0000764, 0000431, 0000712, 0000634, 0000735,
	0000611, 0000567, 0000564, 0000566, 0000036, 0000002, 0000003, 0000015,
	0000030, 0000377, 0001000, 0177764, 0000436, 0054731, 0050750, 0020753,
	0040745, 0102460, 0040737, 0020762, 0061004, 0020734, 0105304, 0000406,
	0020743, 0101014, 0014741, 0000772, 0002712, 0034754, 0167700, 0116415,
	0024752, 0021001, 0106414, 0000754, 0021000, 0024703, 0106414, 0000750,
	0021003, 0163400, 0024736, 0106405, 0000404, 0121400, 0101404, 0000740,
	0044714, 0021005, 0042732, 0024664, 0122405, 0000404, 0101405, 0004404,
	0000727, 0010656, 0034654, 0024403, 0120500, 0101404, 0000777, 0040662,
	0040664, 0040664, 0102520, 0061004, 0020655, 0101015, 0000776, 0106415,
	0001400, 0014634, 0000761, 0020673, 0061004, 0000400, 0061005, 0102000,
	0143000, 0034672, 0024667, 0166400, 0061005, 0004670, 0020663, 0034664,
	0164000, 0147000, 0061005, 0024762, 0132414, 0133000, 0020636, 0034416,
	0101015, 0156415, 0131001, 0000754, 0024643, 0044625, 0101015, 0000750,
	0014623, 0004644, 0020634, 0061004, 0002000, 0176764, 0001401, 0041002
};

/** @brief ethernet enable flag */
static int ether_enable;

/** @brief interval between broadcasting duckbreath packets locally */
static int duckbreath_sec;


#define	LED_W		16
#define	LED_H		16
#define	LED_COUNT	10

/**
 * @brief update the frontend's Ethernet activity indicator
 *
 * Note: There is no such indicator in the real Alto. This is just
 * to keep the user informed about drive activity.
 */
void ether_show_indicator(int n, int val)
{
	static int prev[LED_COUNT];

	if (n < 0 || n >= LED_COUNT)
		return;

	if (val == prev[n])
		return;

	sdl_draw_icon(64 + n * LED_W, DBG_FONT_H + 1, val);
	prev[n] = val;
}

/**
 * @brief update the frontend's Ethernet activity indicators
 *
 * Note: There is no such indicator in the real Alto. This is just
 * to keep the user informed about drive activity.
 */
void ether_show_indicators(int counts)
{
	ether_show_indicator(0,
		GET_ETH_ICMD(eth.status) ? led_yellow_on : led_yellow_off);
	ether_show_indicator(1,
		GET_ETH_IBUSY(eth.status) ? led_green_on : led_green_off);
	ether_show_indicator(2,
		GET_ETH_IGONE(eth.status) ? led_yellow_on : led_yellow_off);
	ether_show_indicator(3,
		GET_ETH_IDL(eth.status) ? led_red_on : led_red_off);

	ether_show_indicator(5,
		GET_ETH_OCMD(eth.status) ? led_yellow_on : led_yellow_off);
	ether_show_indicator(6,
		GET_ETH_OBUSY(eth.status) ? led_red_on : led_red_off);
	ether_show_indicator(7,
		GET_ETH_OGONE(eth.status) ? led_yellow_on : led_yellow_off);
	ether_show_indicator(8,
		GET_ETH_COLL(eth.status) ? led_red_on : led_red_off);
	if (counts) {
		border_printf( 64, 1, "rx%8d", eth.rx_count);
		border_printf(144, 1, "tx%8d", eth.tx_count);
	}
}

/**
 * @brief check for the various reasons to wakeup the Ethernet task
 */
static void eth_wakeup(void)
{
	register int busy, idr, odr, etac;

	etac = cpu.task == task_ether;

	dbg_printf("eth_wakeup: ibusy=%d obusy=%d ",
		GET_ETH_IBUSY(eth.status), GET_ETH_OBUSY(eth.status));
	busy = GET_ETH_IBUSY(eth.status) | GET_ETH_OBUSY(eth.status);
	/* if not busy, reset the FIFO read and write counters */
	if (busy == 0) {
		eth.fifo_rd = 0;
		eth.fifo_wr = 0;
	}

	ether_show_indicators(0);

	/*
	 * POST conditions to wakeup the Ether task:
	 *	input data late
	 *	output command
	 *	input command
	 *	output gone
	 *	input gone
	 */
	if (GET_ETH_IDL(eth.status)) {
		dbg_printf("post (input data late)\n");
		CPU_SET_TASK_WAKEUP(task_ether);
		return;
	}
	if (GET_ETH_OCMD(eth.status)) {
		dbg_printf("post (output command)\n");
		CPU_SET_TASK_WAKEUP(task_ether);
		return;
	}
	if (GET_ETH_ICMD(eth.status)) {
		dbg_printf("post (input command)\n");
		CPU_SET_TASK_WAKEUP(task_ether);
		return;
	}
	if (GET_ETH_OGONE(eth.status)) {
		dbg_printf("post (output gone)\n");
		CPU_SET_TASK_WAKEUP(task_ether);
		return;
	}
	if (GET_ETH_IGONE(eth.status)) {
		dbg_printf("post (input gone)\n");
		CPU_SET_TASK_WAKEUP(task_ether);
		return;
	}

	/*
	 * IDR (input data ready) conditions to wakeup the Ether task (AND):
	 *	IBUSY	input busy
	 *	BNNE	buffer next next empty
	 *	BNE	buffer next empty
	 *	ETAC	ether task active
	 *
	 * IDR' = (IBUSY & (BNNE & (BNE' & ETAC')')')'
	 */
	idr = GET_ETH_IBUSY(eth.status) &&
		(ETHER_A49_BNNE || (ETHER_A49_BNE == 0 && etac));
	if (idr) {
		CPU_SET_TASK_WAKEUP(task_ether);
		dbg_printf("input data ready\n");
		return;
	}

	/*
	 * ODR (output data ready) conditions to wakeup the Ether task:
	 *	WLF	write latch filled(?)
	 *	BF	buffer (FIFO) full
	 *	OEOT	output end of transmission
	 *	OBUSY	output busy
	 *
	 * ODR'	= (OBUSY & OEOT' & (BF' & WLF')')'
	 */
	odr = GET_ETH_OBUSY(eth.status) &&
	 	(GET_ETH_OEOT(eth.status) ||
		(GET_ETH_WLF(eth.status) && ETHER_A49_BF == 0));
	if (odr) {
		CPU_SET_TASK_WAKEUP(task_ether);
		dbg_printf("output data ready\n");
		return;
	}

	/*
	 * EWFCT (ether wake function) conditions to wakeup the Ether task:
	 *	EWFCT	flip flop set by the F1 EWFCT
	 * The task is activated by the display.c code together with the
	 * next wakeup of the MRT (memory refresh task).
	 */
	if (cpu.ewfct) {
		CPU_SET_TASK_WAKEUP(task_ether);
		dbg_printf("ether wake function\n");
		return;
	}

	/* otherwise no more wakeup for the Ether task */
	dbg_printf("-/-\n");
	CPU_CLR_TASK_WAKEUP(task_ether);
}

/**
 * @brief F9401 CRC checker
 * <PRE>
 *
 * The F9401 looks similiar to the SN74F401. However, in the schematics
 * there is a connection from pin 9 (labeled D9) to pin 2 (labeled Q8).
 * See below for the difference:
 *
 *           SN74F401                       F9401  
 *         +---+-+---+                   +---+-+---+
 *         |   +-+   |                   |   +-+   |
 *    CP' -|1      14|-  Vcc       CLK' -|1      14|-  Vcc
 *         |         |                   |         |
 *     P' -|2      13|-  ER          P' -|2      13|-  CRCZ'
 *         |         |                   |         |
 *    S0  -|3      12|-  Q           Z  -|3      12|-  CRCDATA
 *         |         |                   |         |
 *    MR  -|4      11|-  D          MR  -|4      11|-  SDI
 *         |         |                   |         |
 *    S1  -|5      10|-  CWE         Y  -|5      10|-  SR
 *         |         |                   |         |
 *    NC  -|6       9|-  NC         D1  -|6       9|-  D9
 *         |         |                   |         |
 *   GND  -|7       8|-  S2        GND  -|7       8|-  X
 *         |         |                   |         |
 *         +---------+	        	 +---------+
 *
 * Functional description (SN74F401)
 *
 * The 'F401 is a 16-bit programmable device which operates on serial data
 * streams and provides a means of detecting transmission errors. Cyclic
 * encoding and decoding schemes for error detection are based on polynomial
 * manipulation in modulo arithmetic. For encoding, the data stream (message
 * polynomial) is divided by a selected polynomial. This division results
 * in a remainder which is appended to the message as check bits. For error
 * checking, the bit stream containing both data and check bits is divided
 * by the same selected polynomial. If there are no detectable errors, this
 * division results in a zero remainder. Although it is possible to choose
 * many generating polynomials of a given degree, standards exist that
 * specify a small number of useful polynomials. The 'F401 implements the
 * polynomials listed in Tabel I by applying the appropriate logic levels
 * to the select pins S0, S1 and S2.
 *
 * Teh 'F401 consists of a 16-bit register, a Read Only Memory (ROM) and
 * associated control circuitry as shown in the block diagram. The
 * polynomial control code presented at inputs S0, S1 and S2 is decoded
 * by the ROM, selecting the desired polynomial by establishing shift
 * mode operation on the register with Exclusive OR gates at appropriate
 * inputs. To generate check bits, the data stream is entered via the
 * Data inputs (D), using the HIGH-to-LOW transition of the Clock input
 * (CP'). This data is gated with the most significant output (Q) of
 * the register, and controls the Exclusive OR gates (Figure 1). The
 * Check Word Enable (CWE) must be held HIGH while the data is being
 * entered. After the last data bit is entered, the CWE is brought LOW
 * and the check bits are shifted out of the register and appended to
 * the data bits using external gating (Figure 2).
 *
 * To check an incoming message for errors, both the data and check bits
 * are entered through the D input with the CWE input held HIGH. The
 * 'F401 is not in the data path, but only monitors the message. The
 * Error output becomes valid after the last check bit has been entered
 * into the 'F401 by a HIGH-to-LOW transition of CP'. If no detectable
 * errors have occured during the transmission, the resultant internal
 * register bits are all LOW and the Error Output (ER) is LOW.
 * If a detectable error has occured, ER is HIGH.
 *
 * A HIGH on the Master Reset input (MR) asynchronously clears the
 * register. A LOW on the Preset input (P') asynchronously sets the
 * entire register if the control code inputs specify a 16-bit
 * polynomial; in the case of 12- or 8-bit check polynomials only the
 * most significant 12 or 8 register bits are set and the remaining
 * bits are cleared.
 *
 * [Table I]
 *
 * S2 S1 S0	polynomial			remarks
 * ----------------------------------------------------------------
 * L  L  L	x^16+x^15+x^2+1			CRC16
 * L  L  H	x^16+x^14+x+1			CRC16 reverse
 * L  H  L	x^16+x^15+x^13+x^7+x^4+x^2+x+1	-/-
 * L  H  H	x^12+x^11+x^3+x^2+x+1		CRC-12
 * H  L  L	x^8+x^7+x^5+x^4+x+1		-/-
 * H  L  H	x^8+1				LRC-8
 * H  H  L	X^16+x^12+x^5+1			CRC-CCITT
 * H  H  H	X^16+x^11+x^4+1			CRC-CCITT reverse
 *
 * </PRE>
 * The Alto Ethernet interface seems to be using the last one of polynomials,
 * or perhaps something entirely different.
 *
 * TODO: verify polynomial generator; build a lookup table to make it faster.
 *
 * @param crc previous CRC value
 * @param data 16 bit data
 * @result new CRC value after 16 bits
 */
uint32_t f9401_7(uint32_t crc, uint32_t data)
{
	int i;
	for (i = 0; i < 16; i++) {
		crc <<= 1;
		if (data & 0100000)
			crc ^= (1<<15) | (1<<10) | (1<<3) | (1<<0);
		data <<= 1;
	}
	return crc & 0177777;
}

/**
 * @brief HACK: pull the next word from the duckbreath in the fifo
 *
 * This is probably lacking the updates to one or more of
 * the status flip flops.
 */
static void rx_duckbreath(int id, int arg)
{
	uint32_t data;

	if (arg == 0) {
		/* first word: set the IBUSY flip flop */
		PUT_ETH_IBUSY(eth.status, 1);
	}

	data = duckbreath[arg++];
	eth.rx_crc = f9401_7(eth.rx_crc, data);

	eth.fifo[eth.fifo_wr] = data;
	if (++eth.fifo_wr == ETHER_FIFO_SIZE)
		eth.fifo_wr = 0;

	PUT_ETH_WLF(eth.status, 1);
	if (ETHER_A49_BF == 0) {
		/* fifo is overrun: set input data late flip flop */
		PUT_ETH_IDL(eth.status, 1);
	}

	if (arg == BREATHLEN) {
		/*
		 * last word: reset the receiver CRC
		 *
		 * TODO: if data comes from some other source,
		 * compare our CRC with the next word received
		 * and set the CRC error flag if they differ.
		 */
		eth.rx_crc = 0;
		/* set the IGONE flip flop */
		PUT_ETH_IGONE(eth.status, 1);
		ether_show_indicators(1);

		timer_insert(TIME_S(duckbreath_sec), rx_duckbreath, 0, "duckbreath");
	} else {
		/* 5.44us per word (?) */
		timer_insert(TIME_US(5.44), rx_duckbreath, arg, "duckbreath");
	}

	eth_wakeup();
}

/**
 * @brief transmit data from the FIFO to <nirvana for now>
 *
 * @param id timer id
 * @param arg word count if >= 0, -1 if CRC is to be transmitted (last word)
 */
static void tx_packet(int id, int arg)
{
	uint32_t data;

	/* last word is the CRC */
	if (arg == -1) {
		eth.tx_id = 0;
		/* TODO: send the CRC as final word of the packet */
		dbg_printf(" CRC:%06o\n", eth.tx_crc);
		eth.tx_crc = 0;
		/* set the OGONE flip flop */
		PUT_ETH_OGONE(eth.status, 1);
		eth_wakeup();
		return;
	}

	data = eth.fifo[eth.fifo_rd];
	eth.tx_crc = f9401_7(eth.tx_crc, data);
	if (eth.fifo_rd % 8)
		dbg_printf(" %06o", data);
	else
		dbg_printf("\n%06o: %06o", eth.tx_count, data);
	if (++eth.fifo_rd == ETHER_FIFO_SIZE)
		eth.fifo_rd = 0;
	eth.tx_count++;

	/* is the FIFO empty now? */
	if (ETHER_A49_BE) {
		/* clear the OBUSY and WLF flip flops */
		PUT_ETH_OBUSY(eth.status, 0);
		PUT_ETH_WLF(eth.status, 0);
		eth.tx_id = timer_insert(TIME_US(5.44), tx_packet, -1, "tx packet CRC");
		eth_wakeup();
		return;
	}

	/* next word */
	eth.tx_id = timer_insert(TIME_US(5.44), tx_packet, arg + 1, "tx packet");
	eth_wakeup();
}

/**
 * @brief Ethernet input data function
 *
 * Gates the contents of the FIFO to BUS[0-15], and increments
 * the read pointer at the end of the cycle.
 */
static void bs_eidfct_0(void)
{
	int and = eth.fifo[eth.fifo_rd];

	LOG((0,3, "	<-EIDFCT; pull %06o from FIFO[%02o]\n",
		and, eth.fifo_rd));
	if (++eth.fifo_rd == ETHER_FIFO_SIZE)
		eth.fifo_rd = 0;
	cpu.bus &= and;
	eth.rx_count++;

	eth_wakeup();
}

/**
 * @brief block the Ether task
 */
static void f1_block_0(void)
{
	LOG((0,2,"	BLOCK %s\n", task_name[cpu.task]));
	CPU_CLR_TASK_WAKEUP(task_ether);
}

/**
 * @brief Ethernet input look function
 *
 * Gates the contents of the FIFO to BUS[0-15], but does not
 * increment the read pointer;
 */
static void f1_eilfct_0(void)
{
	int and = eth.fifo[eth.fifo_rd];
	LOG((0,3, "	<-EILFCT; %06o at FIFO[%02o]\n",
		and, eth.fifo_rd));
	cpu.bus &= and;
}

/**
 * @brief Ethernet post function
 *
 * Gates the interface status to BUS[8-15]. Resets the interface
 * at the end of the function.
 *
 * The schematics suggest that just BUS[10-15] is modified.
 *
 * Also a comment from the microcode:
 * ;Ether Post Function - EPFCT.  Gate the hardware status
 * ;(LOW TRUE) to Bus [10:15], reset interface.
 *
 */
static void f1_epfct_0(void)
{
	int and = ~ALTO_GET(eth.status,16,10,15) & 0177777;

	LOG((0,3, "	<-EPFCT; BUS[8-15] = STATUS (%#o)\n", and));
	cpu.bus &= and;

	eth.status = 0;
	eth_wakeup();
}

/**
 * @brief Ethernet countdown wakeup function
 *
 * Sets a flip flop in the interface that will cause a wakeup to the
 * Ether task on the next tick of SWAKMRT (memory refresh task).
 * This function must be issued in the instruction after a TASK.
 * The resulting wakeup is cleared when the Ether task next runs.
 */
static void f1_ewfct_1(void)
{
	/*
	 * Set a flag in the CPU to handle the next task switch
	 * to the task_mrt by also waking up the task_ether.
	 */
	cpu.ewfct = ether_enable;
}

/**
 * @brief Ethernet output data function
 *
 * Loads the FIFO from BUS[0-15], then increments the write
 * pointer at the end of the cycle.
 */
static void f2_eodfct_1(void)
{
	LOG((0,3, "	EODFCT<-; push %06o into FIFO[%02o]\n",
		cpu.bus, eth.fifo_wr));

	eth.fifo[eth.fifo_wr] = cpu.bus;
	if (++eth.fifo_wr == ETHER_FIFO_SIZE)
		eth.fifo_wr = 0;

	PUT_ETH_WLF(eth.status, 1);
	PUT_ETH_OBUSY(eth.status, 1);
	/* if the FIFO is full */
	if (ETHER_A49_BF == 0) {
		if (0 == eth.tx_id)
			eth.tx_id = timer_insert(TIME_US(5.44), tx_packet, 0, "tx packet");
	}
	eth_wakeup();
}

/**
 * @brief Ethernet output start function
 *
 * Sets the OBUSY flip flop in the interface, starting data
 * wakeups to fill the FIFO for output. When the FIFO is full,
 * or EEFCT has been issued, the interface will wait for silence
 * on the Ether and begin transmitting.
 */
static void f2_eosfct_1(void)
{
	LOG((0,3, "	EOSFCT\n"));
	PUT_ETH_WLF(eth.status, 0);
	PUT_ETH_OBUSY(eth.status, 0);
	eth_wakeup();
}

/**
 * @brief Ethernet reset branch function
 *
 * This command dispatch function merges the ICMD and OCMD flip flops
 * into NEXT[6-7]. These flip flops are the means of communication
 * between the emulator task and the Ethernet task. The emulator
 * task sets them up from BUS[14-15] with the STARTF function,
 * causing the Ethernet task to wakeup, dispatch on them and then
 * reset them with EPFCT.
 */
static void f2_erbfct_1(void)
{
	int or = 0;
	ALTO_PUT(or,10,6,6,GET_ETH_ICMD(eth.status));
	ALTO_PUT(or,10,7,7,GET_ETH_OCMD(eth.status));
	LOG((0,3, "	ERBFCT; NEXT[6-7] = ICMD,OCMD (%#o | %#o)\n",
		cpu.next2, or));
	CPU_BRANCH(or);
	eth_wakeup();
}

/**
 * @brief Ethernet end of transmission function
 *
 * This function is issued when all of the main memory output buffer
 * has been transferred to the FIFO. EEFCT disables further data
 * wakeups.
 */
static void f2_eefct_1(void)
{
	/* start transmitting the packet */
	PUT_ETH_OBUSY(eth.status, 1);
	PUT_ETH_OEOT(eth.status, 1);
	if (0 == eth.tx_id)
		eth.tx_id = timer_insert(TIME_US(5.44), tx_packet, 0, "tx packet");
	eth_wakeup();
}

/**
 * @brief Ethernet branch function
 *
 * ORs a one into NEXT[7] if an input data late is detected, or an SIO
 * with AC0[14-15] non-zero is issued, or if the transmitter or
 * receiver goes done. ORs a one into NEXT[6] if a collision is detected.
 */
static void f2_ebfct_1(void)
{
	int or = 0;

	ALTO_PUT(or,10,6,6,
		GET_ETH_COLL(eth.status));
	ALTO_PUT(or,10,7,7,
		GET_ETH_IDL(eth.status) |
		GET_ETH_ICMD(eth.status) |
		GET_ETH_OCMD(eth.status) |
		GET_ETH_IGONE(eth.status) |
		GET_ETH_OGONE(eth.status));
	LOG((0,3, "	EBFCT; NEXT ... (%#o | %#o)\n",
		cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief Ethernet countdown branch function
 *
 * ORs a one into NEXT[7] if the FIFO is not empty.
 */
static void f2_ecbfct_1(void)
{
	int or = 0;

	/* TODO: the BE' (buffer empty) signal is output D0 of PROM a49 */
	ALTO_PUT(or,10,7,7,ETHER_A49_BE);
	LOG((0,3, "	ECBFCT; NEXT[7] = FIFO %sempty (%#o | %#o)\n",
		or ? "not " : "is ", cpu.next2, or));
	CPU_BRANCH(or);
}

/**
 * @brief Ethernet input start function
 *
 * Sets the IBUSY flip flop in the interface, causing it to hunt
 * for the beginning of a packet: silence on the Ether followed
 * by a transition. When the interface has collected two words,
 * it will begin generating data wakeups to the microcode.
 */
static void f2_eisfct_1(void)
{
	LOG((0,3, "	EISFCT\n"));
	PUT_ETH_IBUSY(eth.status, 0);
	eth_wakeup();
}

/** @brief called by the CPU when the Ethernet task becomes active
 *
 * Reset the Ether wake flip flop
 */
static void activate(void)
{
	cpu.ewfct = 0;
}

/**
 * @brief pass command line switches down to the Ethernet code
 *
 * @param arg a pointer to a command line switch, like "-db"
 * @result returns 0 if arg was accepted, -1 otherwise
 */
int ether_args(const char *arg)
{
	char *equ;
	int val;

	switch (arg[0]) {
	case '-':
		arg++;
		equ = strchr(arg, '=');
		if (equ)
			val = strtoull(equ+1,NULL,0);
		else
			val = 5;
		break;
	default:
		return -1;
	}

	if (!strncmp(arg, "ee", 2)) {
		ether_enable = 1;
		return 0;
	}
	if (!strncmp(arg, "db", 2)) {
		duckbreath_sec = val;
		if (duckbreath_sec > 0)
			timer_insert(TIME_S(duckbreath_sec), rx_duckbreath, 0, "duckbreath");
		return 0;
	}
	if (!strncmp(arg, "eh", 2)) {
		if (val < 1 || val > 254)
			fatal(1, "Invalid Ether Host: %d\n", val);
		ether_id = val;
		return 0;
	}
	return -1;
}

/**
 * @brief print usage info for the Ethernet switches
 *
 * @param argc argument count
 * @param argv argument list
 * @result returns 0 (why should it fail?)
 */
int ether_usage(int argc, char **argv)
{
	printf("-ee		Enable Ether net task\n");
	printf("-eh=n		set Ether Host ID for this machine (1 to 254)\n");
	printf("-db[=n]		broadcast duckbreath (every n seconds; default 5)\n");
	return 0;
}

/**
 * @brief Ethernet task slot initialization
 */
int init_ether(int task)
{
	memset(&eth, 0, sizeof(eth));
	SET_FN(bs, ether_eidfct,	bs_eidfct_0,	NULL);

	SET_FN(f1, block,		f1_block_0,	NULL);
	SET_FN(f1, ether_eilfct,	f1_eilfct_0,	NULL);
	SET_FN(f1, ether_epfct,		f1_epfct_0,	NULL);
	SET_FN(f1, ether_ewfct,		NULL,		f1_ewfct_1);

	SET_FN(f2, ether_eodfct,	NULL,		f2_eodfct_1);
	SET_FN(f2, ether_eosfct,	NULL,		f2_eosfct_1);
	SET_FN(f2, ether_erbfct,	NULL,		f2_erbfct_1);
	SET_FN(f2, ether_eefct,		NULL,		f2_eefct_1);
	SET_FN(f2, ether_ebfct,		NULL,		f2_ebfct_1);
	SET_FN(f2, ether_ecbfct,	NULL,		f2_ecbfct_1);
	SET_FN(f2, ether_eisfct,	NULL,		f2_eisfct_1);

	CPU_SET_ACTIVATE_CB(task, activate);

	ether_show_indicators(1);
	return 0;
}
