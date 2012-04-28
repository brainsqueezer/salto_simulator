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
 * Bus source and F1 handling for Ethernet task
 *
 * $Id: ether.h,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#if !defined(_ETHER_H_INCLUDED_)
#define	_ETHER_H_INCLUDED_

#include "alto.h"
#include "cpu.h"

/** @brief bs task specific: Ethernet input data */
#define	bs_ether_eidfct		bs_task_4

/** @brief f1 task specific: Ethernet input look function */
#define	f1_ether_eilfct		f1_task_13

/** @brief f1 task specific: Ethernet post function */
#define	f1_ether_epfct		f1_task_14

/** @brief f1 task specific: Ethernet countdown wakeup function */
#define	f1_ether_ewfct		f1_task_15

/** @brief f2 task specific: Ethernet output data function */
#define	f2_ether_eodfct		f2_task_10

/** @brief f2 task specific: Ethernet output start function */
#define	f2_ether_eosfct		f2_task_11

/** @brief f2 task specific: Ethernet reset branch function */
#define	f2_ether_erbfct		f2_task_12

/** @brief f2 task specific: Ethernet end of transmission function */
#define	f2_ether_eefct		f2_task_13

/** @brief f2 task specific: Ethernet branch function */
#define	f2_ether_ebfct		f2_task_14

/** @brief f2 task specific: Ethernet countdown branch function */
#define	f2_ether_ecbfct		f2_task_15

/** @brief f2 task specific: Ethernet input start function */
#define	f2_ether_eisfct		f2_task_16

/** @brief hardware status: write latch full (WLF) */
#define	GET_ETH_WLF(st)		ALTO_BIT(st,16,4)
#define	PUT_ETH_WLF(st,val)	ALTO_PUT(st,16,4,4,val)

/** @brief hardware status: ouput end of transmission (OEOT) */
#define	GET_ETH_OEOT(st)	ALTO_BIT(st,16,5)
#define	PUT_ETH_OEOT(st,val)	ALTO_PUT(st,16,5,5,val)

/** @brief hardware status: input gone */
#define	GET_ETH_IGONE(st)	ALTO_BIT(st,16,6)
#define	PUT_ETH_IGONE(st,val)	ALTO_PUT(st,16,6,6,val)

/** @brief hardware status: output gone */
#define	GET_ETH_OGONE(st)	ALTO_BIT(st,16,7)
#define	PUT_ETH_OGONE(st,val)	ALTO_PUT(st,16,7,7,val)

/** @brief hardware status: input busy (set by EISFCT, bit isn't visible to microcode) */
#define	GET_ETH_IBUSY(st)	ALTO_BIT(st,16,8)
#define	PUT_ETH_IBUSY(st,val)	ALTO_PUT(st,16,8,8,val)

/** @brief hardware status: output busy (set by EOSFCT, bit isn't visible to microcode) */
#define	GET_ETH_OBUSY(st)	ALTO_BIT(st,16,9)
#define	PUT_ETH_OBUSY(st,val)	ALTO_PUT(st,16,9,9,val)

/** @brief hardware status: input data late */
#define	GET_ETH_IDL(st)		ALTO_BIT(st,16,10)
#define	PUT_ETH_IDL(st,val)	ALTO_PUT(st,16,10,10,val)

/** @brief hardware status: collision */
#define	GET_ETH_COLL(st)	ALTO_BIT(st,16,11)
#define	PUT_ETH_COLL(st,val)	ALTO_PUT(st,16,11,11,val)

/** @brief hardware status: CRC error */
#define	GET_ETH_CRC(st)		ALTO_BIT(st,16,12)
#define	PUT_ETH_CRC(st,val)	ALTO_PUT(st,16,12,12,val)

/** @brief hardware status: input command (set from BUS[14] on SIO, reset by EPFCT) */
#define	GET_ETH_ICMD(st)	ALTO_BIT(st,16,13)
#define	PUT_ETH_ICMD(st,val)	ALTO_PUT(st,16,13,13,val)

/** @brief hardware status: output command (set from BUS[15] on SIO, reset by EPFCT) */
#define	GET_ETH_OCMD(st)	ALTO_BIT(st,16,14)
#define	PUT_ETH_OCMD(st,val)	ALTO_PUT(st,16,14,14,val)

/** @brief hardware status: IT flip flop & ISRFULL' */
#define	GET_ETH_IT(st)		ALTO_GET(st,16,15,15)
#define	PUT_ETH_IT(st,val)	ALTO_PUT(st,16,15,15,val)

/** @brief size of the FIFO */
#define	ETHER_FIFO_SIZE	16

typedef struct {
	/** @brief FIFO buffer */
	int fifo[ETHER_FIFO_SIZE];

	/** @brief FIFO input pointer */
	int fifo_rd;

	/** @brief FIFO output pointer */
	int fifo_wr;

	/** @brief status word */
	int status;

	/** @brief receiver CRC */
	uint32_t rx_crc;

	/** @brief transmitter CRC */
	uint32_t tx_crc;

	/** @brief received words count */
	size_t rx_count;

	/** @brief transmitted words count */
	size_t tx_count;

	/** @brief if non-zero, interval in seconds at which to broadcast the duckbreath */
	int duckbreath;
}	ethernet_t;

/** @brief the ethernet context */
extern ethernet_t eth;

/** @brief missing PROM ether.u41; "PE1" phase encoder 1 */
extern uint8_t ether_a41[256];

/** @brief missing PROM ether.u42; "PE2" phase encoder 2 */
extern uint8_t ether_a42[256];

/** @brief missing PROM ether.u49; "AFIFO" FIFO control */
extern uint8_t ether_a49[256];

/** @brief ethernet node id */
extern uint8_t ether_id;

/** @brief missing PROM ether.u49; buffer empty (active low) */
#define	ETHER_A49_BE	ALTO_BIT(ether_a49[(eth.fifo_wr<<4)|eth.fifo_rd],4,3)

/** @brief missing PROM ether.u49; buffer next(?) empty (active low) */
#define	ETHER_A49_BNE	ALTO_BIT(ether_a49[(eth.fifo_wr<<4)|eth.fifo_rd],4,2)

/** @brief missing PROM ether.u49; buffer next next(?) empty (active low) */
#define	ETHER_A49_BNNE	ALTO_BIT(ether_a49[(eth.fifo_wr<<4)|eth.fifo_rd],4,1)

/** @brief missing PROM ether.u49; buffer full (active low) */
#define	ETHER_A49_BF	ALTO_BIT(ether_a49[(eth.fifo_wr<<4)|eth.fifo_rd],4,0)

/** @brief pass command line switches down to the Ethernet code */
extern int ether_args(const char *arg);

/** @brief print usage info for the Ethernet switches */
extern int ether_usage(int argc, char **argv);

/** @brief initialize ethernet task */
extern int init_ether(int task);

#endif	/* !defined(_ETHER_H_INCLUDED_) */
