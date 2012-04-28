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
 * EIA functions
 *
 * $Id: eia.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <SDL.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "debug.h"
#include "memory.h"
#include "eia.h"

/**
 * @brief get EIA status Disconnect flag
 * Disconnect flag: when set, this bit indicates loss of RS-232 line;
 * Data Set Ready if data set interface is selected.
 * Data Terminal Ready if data terminal interface is selected.
 */
#define	GET_EIA_STATUS_D(i)		ALTO_BIT(eia[i].status,16,0)
/** @brief put EIA status Disconnect flag */
#define	PUT_EIA_STATUS_D(i,val)		ALTO_PUT(eia[i].status,16,0,0,val)

/** @brief get EIA status Carrier Detect flag */
#define	GET_EIA_STATUS_CD(i)		ALTO_BIT(eia[i].status,16,1)
/** @brief put EIA status Carrier Detect flag */
#define	PUT_EIA_STATUS_CD(i,val)	ALTO_PUT(eia[i].status,16,1,1,val)

/** @brief get EIA status Ring Indicator flag */
#define	GET_EIA_STATUS_RI(i)		ALTO_BIT(eia[i].status,16,2)
/** @brief put EIA status Ring Indicator flag */
#define	PUT_EIA_STATUS_RI(i,val)	ALTO_PUT(eia[i].status,16,2,2,val)

/**
 * @brief get EIA status Send Indicator flag
 * When bit is set, indicates presence of Clear To Send if data
 * set interface is selected. Request To Send if data terminal
 * interface is selected.
 */
#define	GET_EIA_STATUS_SI(i)		ALTO_BIT(eia[i].status,16,3)
/** @brief put EIA status Send Indicator flag */
#define	PUT_EIA_STATUS_SI(i,val)	ALTO_PUT(eia[i].status,16,3,3,val)

/** @brief get EIA status line selector */
#define	GET_EIA_STATUS_LINE(i)		ALTO_GET(eia[i].status,16,4,6)
/** @brief put EIA status line selector */
#define	PUT_EIA_STATUS_LINE(i,val)	ALTO_PUT(eia[i].status,16,4,6,val)

/**
 * @brief get EIA status Remote half-duplex break
 * In half-duplex this bit going to a zero signals a break
 * by the remote receiving station.
 */
#define	GET_EIA_STATUS_SRD(i)		ALTO_BIT(eia[i].status,16,7)
/** @brief put EIA status Remote half-duplex break flag */
#define	PUT_EIA_STATUS_SRD(i,val)	ALTO_PUT(eia[i].status,16,7,7,val)

/** @brief get EIA status Receive Data Available flag */
#define	GET_EIA_STATUS_RDA(i)		ALTO_BIT(eia[i].status,16,8)
/** @brief put EIA status Receive Data Available flag */
#define	PUT_EIA_STATUS_RDA(i,val)	ALTO_PUT(eia[i].status,16,8,8,val)

/** @brief get EIA status Transmit Buffer Empty flag */
#define	GET_EIA_STATUS_TBMT(i)		ALTO_BIT(eia[i].status,16,9)
/** @brief put EIA status Transmit Buffer Empty flag */
#define	PUT_EIA_STATUS_TBMT(i,val)	ALTO_PUT(eia[i].status,16,9,9,val)

/** @brief get EIA status Sync Character Received (USRT) */
#define	GET_EIA_STATUS_SCR(i)		ALTO_BIT(eia[i].status,16,10)
/** @brief put EIA status Sync Character Received (USRT) */
#define	PUT_EIA_STATUS_SCR(i,val)	ALTO_PUT(eia[i].status,16,10,10,val)

/** @brief get EIA status Fill Character Transmitted (USRT) */
#define	GET_EIA_STATUS_FCT(i)		ALTO_BIT(eia[i].status,16,11)
/** @brief put EIA status Fill Character Transmitted (USRT) */
#define	PUT_EIA_STATUS_FCT(i,val)	ALTO_PUT(eia[i].status,16,11,11,val)

/** @brief get EIA status Receive Parity Error flag */
#define	GET_EIA_STATUS_RPE(i)		ALTO_BIT(eia[i].status,16,12)
/** @brief put EIA status Receive Parity Error flag */
#define	PUT_EIA_STATUS_RPE(i,val)	ALTO_PUT(eia[i].status,16,12,12,val)

/** @brief get EIA status Receiver Overflow Error flag */
#define	GET_EIA_STATUS_ROR(i)		ALTO_BIT(eia[i].status,16,13)
/** @brief put EIA status Receiver Overflow Error flag */
#define	PUT_EIA_STATUS_ROR(i,val)	ALTO_PUT(eia[i].status,16,13,13,val)

/** @brief get EIA status Receive Framing Error (UART) */
#define	GET_EIA_STATUS_RFE(i)		ALTO_BIT(eia[i].status,16,14)
/** @brief put EIA status Receive Framing Error (UART) */
#define	PUT_EIA_STATUS_RFE(i,val)	ALTO_PUT(eia[i].status,16,14,14,val)

/**
 * @brief get EIA status Interrupt flag:
 * INTP' = D' + CD' + RI + SI' + RDA + TBMT + SCR + (SRD' SEND HDUP)
 */
#define	GET_EIA_STATUS_INTP(i)		ALTO_BIT(eia[i].status,16,15)
/** @brief put EIA status Interrupt flag */
#define	PUT_EIA_STATUS_INTP(i,val)	ALTO_PUT(eia[i].status,16,15,15,val)

/**
 * @brief structure of the EIA serial communication control interface context
 */
typedef struct {
	/** @brief 1: select line number for this interface */
	int line;

	/** @brief 1: communicate with Data Set; 0: communicate with Data Terminal */
	int data_set;

	/** @brief 1: operate half-duplex; 0: full duplex */
	int half_duplex;

	/** @brief 1: currently sending in half-duplex; 0: not sending */
	int sending;

	/** @brief 1: secondary RTS (to data set) or CD (to data terminal); 0: off  */
	int secondary;

	/** @brief selected baud rate (see table); bit time in nanoseconds */
	ntime_t baud_time;

	/** @brief number of data bits */
	int data_bits;

	/** @brief 1: asynchronous mode (UART); 0: synchronous mode (USRT) */
	int uart;

	/** @brief 1: enable parity bit; 0: disable parity bit */
	int parity_bit;

	/** @brief 1: even parity; 0: odd parity */
	int parity_even;

	/** @brief number of stop bits (UART) */
	int stop_bits;

	/** @brief transmitter fill character (USRT) */
	int fill_char;

	/** @brief receiver sync character (USRT) */
	int sync_char;

	/** @brief status word */
	int status;

	/** @brief data word */
	int data;

}	eia_t;

/** @brief up to three interfaces can be present in one AltoII */
static eia_t eia[EIA_MAX];

/**
 * @brief update the interrupt bit of the status word
 *
 * INTP' = D' + CD' + RI + SI' + RDA + TBMT + SCR + (SRD' SEND HDUP)
 *
 * @param i interface number (0 ... 2)
 */
static void eia_update_intp(int i)
{
	int intp = 1;

	if (GET_EIA_STATUS_D(i) == 0)
		intp = 0;
	if (GET_EIA_STATUS_CD(i) == 0)
		intp = 0;
	if (GET_EIA_STATUS_RI(i) == 1)
		intp = 0;
	if (GET_EIA_STATUS_SI(i) == 0)
		intp = 0;
	if (GET_EIA_STATUS_RDA(i) == 1)
		intp = 0;
	if (GET_EIA_STATUS_TBMT(i) == 1)
		intp = 0;
	if (GET_EIA_STATUS_SCR(i) == 1)
		intp = 0;
	if (GET_EIA_STATUS_SRD(i) == 0 && eia[i].sending && eia[i].half_duplex)
		intp = 0;

	PUT_EIA_STATUS_INTP(i, intp);
}

/**
 * @brief write a EIA control word
 *
 */
void eia_w(int addr, int data)
{
	int i, line = ALTO_GET(data,16,4,6);

	for (i = 0; i < EIA_MAX; i++)
		if (line == eia[i].line)
			break;

	if (i >= EIA_MAX) {
		/* non-existing interface line */
		return;
	}

	switch (data & EIA_CONTROL_MASK) {
	/**
	 * @brief Initialize interface circuits (INITIF)
	 *
	 * +---+-----------+-----------+---+---+---+---+---+---------------+
	 * |B0 |B1  B2  B3 |B4  B5  B6 |B7 |B8 |B9 |B10|B11|B12 B13 B14 B15|
	 * +---+-----------+-----------+---+---+---+---+---+---------------+
	 * | 1 | 0   0   0 | LINE      |DS |HD |S  |STD|X  | BAUD RATE     |
	 * +---+-----------+-----------+---+---+---+---+---+---------------+
	 *
	 * LINE	Addresses control word to select line control: up to (3)
	 * 	fullly programmable, full duplex line controls may be
	 *	accomodated in one AltoII processor.
	 *
	 * DS	When set, selected control will communicate with a data set;
	 *	default to data terminal
	 *
	 * HD	When set, selected control will operate half-duplex;
	 *	default full duplex
	 *
	 * S	Indicates also sending when set(1) in half-duplex
	 *
	 * STD	Sets secondary request to send (2SCA) to on when set and
	 *	interface is to data set. sets secondary carrier detect
	 *	to on when set and interface to data terminal
	 *
	 * BAUD	Selects baud rate at which line will operate
	 *	12 13 14 15  rate
	 *	------------------------
	 *	 0  0  1  0    50
	 *	 0  0  1  1    75
	 *	 0  1  0  0   134.5
	 *	 0  1  0  1   200
	 *	 0  1  1  0   600
	 *	 0  1  1  1  2400
	 *	 1  0  0  0  9600
	 *	 1  0  0  1  4800
	 *	 1  0  1  0  1800
	 *	 1  0  1  1  1200
	 *	 1  1  0  0  2400
	 *	 1  1  0  1   300
	 *	 1  1  1  0   150
	 *	 1  1  1  1   110
	 */
	case EIA_INITIF:
		eia[i].data_set = ALTO_BIT(data,16,7);
		eia[i].half_duplex = ALTO_BIT(data,16,8);
		eia[i].sending = ALTO_BIT(data,16,9);
		eia[i].secondary = ALTO_BIT(data,16,10);
		switch (ALTO_GET(data,16,12,15)) {
		case  2:
			eia[i].baud_time = TIME_S(1.0/50);
			break;
		case  3:
			eia[i].baud_time = TIME_S(1.0/75);
			break;
		case  4:
			eia[i].baud_time = TIME_S(1.0/134.5);
			break;
		case  5:
			eia[i].baud_time = TIME_S(1.0/200);
			break;
		case  6:
			eia[i].baud_time = TIME_S(1.0/600);
			break;
		case  7:
			eia[i].baud_time = TIME_S(1.0/2400);
			break;
		case  8:
			eia[i].baud_time = TIME_S(1.0/9600);
			break;
		case  9:
			eia[i].baud_time = TIME_S(1.0/4800);
			break;
		case 10:
			eia[i].baud_time = TIME_S(1.0/1800);
			break;
		case 11:
			eia[i].baud_time = TIME_S(1.0/1200);
			break;
		case 12:
			eia[i].baud_time = TIME_S(1.0/2400);
			break;
		case 13:
			eia[i].baud_time = TIME_S(1.0/300);
			break;
		case 14:
			eia[i].baud_time = TIME_S(1.0/150);
			break;
		case 15:
			eia[i].baud_time = TIME_S(1.0/110);
			break;
		default:
			 eia[i].baud_time = -1;
		}
#if	DEBUG
		LOG((log_MISC,0,"EIA INITIF DS:%d HD:%d S:%d STD:%d BAUD:%lldns\n",
			eia[i].data_set, eia[i].half_duplex, eia[i].sending,
			eia[i].secondary, eia[i].baud_time));
#else
		printf("EIA INITIF DS:%d HD:%d S:%d STD:%d BAUD:%lldns\n",
			eia[i].data_set, eia[i].half_duplex, eia[i].sending,
			eia[i].secondary, eia[i].baud_time);
#endif
		break;

	/**
	 * @brief Initialize receiver/transmitter (INITRT)
	 *
	 * +---+-----------+-----------+---+---+---+---+---+---------------+
	 * |B0 |B1  B2  B3 |B4  B5  B6 |B7  B8 |B9 |B10|B11|B12|B13 B14 B15|
	 * +---+-----------+-----------+-------+---+---+---+---+-----------+
	 * | 1 | 0   0   1 | LINE      |NDB    |NPB|A/S|POE|NSB| X   X   X |
	 * ----+-----------+-----------+---+---+---+---+---+---------------+
	 *
	 * NDB	Selects number of data bits per character
	 *	 7  8   bits
	 *	------------
	 *	 0  0   5
	 *	 0  1   6
	 *	 1  0   7
	 *	 1  1   8
	 *
	 * NPB	No parity bit when set
	 *
	 * A/S	Selects asynchronous (UART) operation when set;
	 *	default to synchronous operation (USRT)
	 *
	 * POE	Selects even parity when set, odd parity when not set
	 *
	 * NSB	Selects number of stop bits per character when asynchronous
	 *	operation is selected. selects 2 stop bits when set; 1 stop
	 *	bit when not set (exception: selects 1.5 stop bits when not
	 *	set and 5 data bits are selected)
	 */
	case EIA_INITRT:
		eia[i].data_bits = ALTO_GET(data,16,7,8) + 5;
		eia[i].parity_bit = ALTO_BIT(data,16,9);
		eia[i].uart = ALTO_BIT(data,16,10);
		eia[i].parity_even = ALTO_BIT(data,16,11);
		eia[i].stop_bits = ALTO_BIT(data,16,12) + 1;
#if	DEBUG
		LOG((log_MISC,0,"EIA INITRT NDB:%d BPB:%d A/S:%d POE:%d NSB:%d\n",
			eia[i].data_bits, eia[i].parity_bit, eia[i].uart,
			eia[i].parity_even, eia[i].stop_bits));
#else
		printf("EIA INITRT NDB:%d BPB:%d A/S:%d POE:%d NSB:%d\n",
			eia[i].data_bits, eia[i].parity_bit, eia[i].uart,
			eia[i].parity_even, eia[i].stop_bits);
#endif
		break;

	/**
	 * @brief Initialize registers (INITRG)
	 *
	 * +---+-----------+-----------+---+-------------------------------+
	 * |B0 |B1  B2  B3 |B4  B5  B6 |B7 |B8  B9  B10 B11 B12 B13 B14 B15|
	 * +---+-----------+-----------+---+-------------------------------+
	 * | 1 | 0   1   0 | LINE      |F/S| DATA FIELD                    |
	 * ----+-----------+-----------+---+-------------------------------+
	 *
	 * F/S	When set causes data field to be loaded into USRT transmitter
	 *	fill character register. When not set data field will be
	 *	loaded into USRT receiver sync character register.
	 */
	case EIA_INITRG:
		if (ALTO_BIT(data,16,7)) {
			eia[i].fill_char = ALTO_GET(data,16,8,15);
#if	DEBUG
			LOG((log_MISC,0,"EIA INITRG FILL:%03o\n",
				eia[i].fill_char));
#else
			printf("EIA INITRG FILL:%03o\n",
				eia[i].fill_char);
#endif
		} else {
			eia[i].sync_char = ALTO_GET(data,16,8,15);
#if	DEBUG
			LOG((log_MISC,0,"EIA INITRG SYNC:%03o\n",
				eia[i].sync_char));
#else
			printf("EIA INITRG SYNC:%03o\n",
				eia[i].sync_char);
#endif
		}
		break;

	/**
	 * @brief Generate controller resets (RESET)
	 *
	 * +---+-----------+-----------+---+---+---+-----------------------+
	 * |B0 |B1  B2  B3 |B4  B5  B6 |B7 |B8 |B9 |B10 B11 B12 B13 B14 B15|
	 * +---+-----------+-----------+---+---+---+-----------------------+
	 * | 1 | 0   1   1 | LINE      |D  |RRT|RR | X   X   X   X   X   X |
	 * ----+-----------+-----------+---+---+---+-----------------------+
	 *
	 * D	Generates a line disconnect/master clear of the selected line
	 *	when set.
	 *
	 * RRT	Clears the receiver/transmitter (UART/USRT) when set.
	 *
	 * RR	Resets the synchronous receiver, clears status register,
	 *	restarts synchronous receiver in the bit transparent mode
	 *	for sync search.
	 */
	case EIA_RESET:
		if (ALTO_BIT(data,16,7)) {
#if	DEBUG
			LOG((log_MISC,0,"EIA RESET disconnect\n"));
			/* TODO: what? */
#else
			printf("EIA RESET disconnect\n");
#endif
		}
		if (ALTO_BIT(data,16,8)) {
#if	DEBUG
			LOG((log_MISC,0,"EIA RESET UART/USRT clear\n"));
			/* TODO: what? */
#else
			printf("EIA RESET UART/USRT clear\n");
#endif
		}
		if (ALTO_BIT(data,16,9)) {
#if	DEBUG
			LOG((log_MISC,0,"EIA RESET USRT sync search\n"));
			/* TODO: what? */
#else
			printf("EIA RESET USRT sync search\n");
#endif
		}
		break;

	/**
	 * @brief Force the deselected control to request an interrupt (SWI)
	 *
	 * +---+-----------+-----------+-----------------------------------+
	 * |B0 |B1  B2  B3 |B4  B5  B6 |B7  B8  B9  B10 B11 B12 B13 B14 B15|
	 * +---+-----------+-----------+-----------------------------------+
	 * | 1 | 1   0   0 | LINE      | X   X   X   X   X   X   X   X   X |
	 * ----+-----------+-----------+-----------------------------------+
	 */
	case EIA_SWI:
		/* set the INTP bit of the status (?) */
		eia[i].status |= 1;
#if	DEBUG
		LOG((log_MISC,0,"EIA SWI\n"));
#else
		printf("EIA SWI\n");
#endif
		break;

	/**
	 * @brief Interrupt acknowledge to the selected line (INTA)
	 *
	 * +---+-----------+-----------+-----------------------------------+
	 * |B0 |B1  B2  B3 |B4  B5  B6 |B7  B8  B9  B10 B11 B12 B13 B14 B15|
	 * +---+-----------+-----------+-----------------------------------+
	 * | 1 | 1   0   1 | LINE      | X   X   X   X   X   X   X   X   X |
	 * ----+-----------+-----------+-----------------------------------+
	 */
	case EIA_INTA:
		/* reset the INTP bit of the status (?) */
		PUT_EIA_STATUS_INTP(i, 1);
#if	DEBUG
		LOG((log_MISC,0,"EIA INTA\n"));
#else
		printf("EIA INTA\n");
#endif
		break;
	}
}

/**
 * @brief read EIA status word on first read, data on following reads
 * <PRE>
 * Status word format
 *
 * +---+---+---+---+-----------+---+---+---+---+---+---+---+---+---+
 * |B0 |B1 |B2 |B3 |B4  B5  B6 |B7 |B8 |B9 |B10|B11|B12|B13|B14|B15|
 * +---+---+---+---+-----------+---+---+---+---+---+---+---+---+---+
 * |D  |CD |RI |SI | LINE      |SRD|RDA|TBM|SCR|FCT|RPE|ROR|RFE|INT|
 * +---+---+---+---+-----------+---+---+---+---+---+---+---+---+---+
 *
 * Interface Status (high order byte)
 *
 * D	Disconnect flag: when set, this bit indicates loss of RS-232 line;
 *	Data Set Ready if data set interface is selected.
 *	Data Terminal Ready if data terminal interface is selected.
 *
 * CD	Carrier Detect flag
 *
 * RI	Ring Indicator flag
 *
 * SI	Send Indicator flag: when bit is set, indicates presence of
 *	Clear To Send if data set interface is selected.
 *	Request To Send if data terminal interface is selected.
 *
 * SRD	Remote half-duplex break: in half-duplex this bit going to
 *	a zero signals a break by the remote receiving station.
 *
 * Receiver/Transmitter Status (low order byte)
 *
 * RDA	Receive Data Available
 *
 * TBMT	Transmit Buffer Empty
 *
 * SCR	Sync Character Received (USRT)
 *
 * FCT	Fill Character Transmitted (USRT)
 *
 * RPE	Receive Parity Error
 *
 * ROR	Receive Overflow Error
 *
 * RFE	Receive Framing Error (UART)
 *
 * INTP	Interrupt flag:
 *	INTP' = D' + CD' + RI + SI' + RDA + TBMT + SCR + (SRD' SEND HDUP)
 * </PRE>
 */
int eia_r(int addr)
{
	int data = 0177777;
	int i;

	if (0 == (addr % 4)) {
		return data;
	}

	/* interface 0, 1 or 2 */
	i = (addr % 4) - 1;

	if (i >= EIA_MAX) {
		/* non-existing interface line (TODO: return what?) */
		return data;
	}

	if (GET_EIA_STATUS_INTP(i) == 0) {
		/* interrupt pending: return status register */
		data = eia[i].status;
		if (!dbg.lock) {
			LOG((log_MISC,0,"EIA STATUS read:%06o\n", data));
			/* not a debug read: reset the interrupt pending flag */
			PUT_EIA_STATUS_INTP(i, 1);
		}
	} else {
		/* no interrupt pending: return data register */
		data = eia[i].data;
		LOG((log_MISC,0,"EIA DATA read:%06o\n", data));
	}

	return data;
}

/**
 * @brief reset the EIA serial communication control interface context
 *
 * @result return 0 on success
 */
int init_eia(void)
{
	int i;

	memset(&eia, 0, sizeof(eia));

	/* for now set lines #0, #1 and #2 in the interfaces */
	for (i = 0; i < EIA_MAX; i++) {
		eia[i].line = i;

		/* put EIA status Disconnect flag */
		PUT_EIA_STATUS_D(i, 0);
		/* put EIA status Carrier Detect flag */
		PUT_EIA_STATUS_CD(i, 0);
		/* put EIA status Ring Indicator flag */
		PUT_EIA_STATUS_RI(i, 0);
		/* put EIA status Send Indicator flag */
		PUT_EIA_STATUS_SI(i, 0);
		/* put EIA status line selector */
		PUT_EIA_STATUS_LINE(i, i);
		/* put EIA status Remote half-duplex break flag */
		PUT_EIA_STATUS_SRD(i, 0);
		/* put EIA status Receive Data Available flag */
		PUT_EIA_STATUS_RDA(i, 0);
		/* put EIA status Transmit Buffer Empty flag */
		PUT_EIA_STATUS_TBMT(i, 0);
		/* put EIA status Sync Character Received (USRT) */
		PUT_EIA_STATUS_SCR(i, 0);
		/* put EIA status Fill Character Transmitted (USRT) */
		PUT_EIA_STATUS_FCT(i, 0);
		/* put EIA status Receive Parity Error flag */
		PUT_EIA_STATUS_RPE(i, 0);
		/* put EIA status Receiver Overflow Error flag */
		PUT_EIA_STATUS_ROR(i, 0);
		/* put EIA status Receive Framing Error (UART) */
		PUT_EIA_STATUS_RFE(i, 0);

		/* update the interrupt status bit */
		eia_update_intp(i);
	}

	/*
	 * TODO: is there just one address and interface selection
	 * is done by means of the LINE bit field of BUS?
	 * For now assume there are (up to) three addresses.
	 */
	install_mmio_fn(0177001, 0177003, eia_r, eia_w);

	return 0;
}
