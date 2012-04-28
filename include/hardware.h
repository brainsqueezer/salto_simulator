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
 * Memory mapped I/O for keyboard
 *
 * $Id: hardware.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_HARDWARE_H_INCLUDED_)
#define	_HARDWARE_H_INCLUDED_

/**
 * @brief get printer paper ready bit
 * Paper ready bit. 0 when the printer is ready for a paper scrolling operation.
 */
#define	GET_PPRDY(utilin)		ALTO_GET(utilin,16,0,0)

/** @brief put printer paper ready bit */
#define	PUT_PPRDY(utilin,val)		ALTO_PUT(utilin,16,0,0,val)

/**
 * @brief get printer check bit
 * Printer check bit bit. Should the printer find itself in an abnormal state,
 * it sets this bit to 0
 */
#define	GET_PCHECK(utilin)		ALTO_GET(utilin,16,1,1)

/** @brief put printer check bit */
#define	PUT_PCHECK(utilin,val)		ALTO_PUT(utilin,16,1,1,val)

/** @brief get unused bit 2 */
#define	GET_UNUSED_2(utilin)		ALTO_GET(utilin,16,2,2)
/** @brief put unused bit 2 */
#define	PUT_UNUSED_2(utilin,val)	ALTO_PUT(utilin,16,2,2,val)

/**
 * @brief get printer daisy ready bit
 * Daisy ready bit. 0 when the printer is ready to print a character.
 */
#define	GET_PCHRDY(utilin)		ALTO_GET(utilin,16,3,3)

/** @brief put printer daisy ready bit */
#define	PUT_PCHRDY(utilin,val)		ALTO_PUT(utilin,16,3,3,val)

/**
 * @brief get printer carriage ready bit
 * Carriage ready bit. 0 when the printer is ready for horizontal positioning.
 */
#define	GET_PCARRDY(utilin)		ALTO_GET(utilin,16,4,4)

/** @brief put printer carriage ready bit */
#define	PUT_PCARRDY(utilin,val)		ALTO_PUT(utilin,16,4,4,val)

/**
 * @brief get printer ready bit
 * Ready bit. Both this bit and the appropriate other ready bit (carriage,
 * daisy, etc.) must be 0 before attempting any output operation.
 */
#define	GET_PREADY(utilin)		ALTO_GET(utilin,16,5,5)

/** @brief put printer ready bit */
#define	PUT_PREADY(utilin,val)		ALTO_PUT(utilin,16,5,5,val)

/**
 * @brief memory configuration switch
 */
#define	GET_MEMCONFIG(utilin)		ALTO_GET(utilin,16,6,6)

/** @brief put memory configuration switch */
#define	PUT_MEMCONFIG(utilin,val)	ALTO_PUT(utilin,16,6,6,val)

/** @brief get unused bit 7 */
#define	GET_UNUSED_7(utilin)		ALTO_GET(utilin,16,7,7)
/** @brief put unused bit 7 */
#define	PUT_UNUSED_7(utilin,val)	ALTO_PUT(utilin,16,7,7,val)

/** @brief get key set key 0 */
#define	GET_KEYSET_KEY0(utilin)		ALTO_GET(utilin,16,8,8)
/** @brief put key set key 0 */
#define	PUT_KEYSET_KEY0(utilin,val)	ALTO_PUT(utilin,16,8,8,val)

/** @brief get key set key 1 */
#define	GET_KEYSET_KEY1(utilin)		ALTO_GET(utilin,16,9,9)
/** @brief put key set key 1 */
#define	PUT_KEYSET_KEY1(utilin,val)	ALTO_PUT(utilin,16,9,9,val)

/** @brief get key set key 2 */
#define	GET_KEYSET_KEY2(utilin)		ALTO_GET(utilin,16,10,10)
/** @brief put key set key 2 */
#define	PUT_KEYSET_KEY2(utilin,val)	ALTO_PUT(utilin,16,10,10,val)

/** @brief get key set key 3 */
#define	GET_KEYSET_KEY3(utilin)		ALTO_GET(utilin,16,11,11)
/** @brief put key set key 3 */
#define	PUT_KEYSET_KEY3(utilin,val)	ALTO_PUT(utilin,16,11,11,val)

/** @brief get key set key 4 */
#define	GET_KEYSET_KEY4(utilin)		ALTO_GET(utilin,16,12,12)
/** @brief put key set key 4 */
#define	PUT_KEYSET_KEY4(utilin,val)	ALTO_PUT(utilin,16,12,12,val)

/** @brief get mouse red button bit */
#define	GET_MOUSE_RED(utilin)		ALTO_GET(utilin,16,13,13)
/** @brief put mouse red button bit */
#define	PUT_MOUSE_RED(utilin,val)	ALTO_PUT(utilin,16,13,13,val)

/** @brief get mouse blue button bit */
#define	GET_MOUSE_BLUE(utilin)		ALTO_GET(utilin,16,14,14)
/** @brief put mouse blue button bit */
#define	PUT_MOUSE_BLUE(utilin,val)	ALTO_PUT(utilin,16,14,14,val)

/** @brief get mouse yellow button bit */
#define	GET_MOUSE_YELLOW(utilin)	ALTO_GET(utilin,16,15,15)
/** @brief put mouse yellow button bit */
#define	PUT_MOUSE_YELLOW(utilin,val)	ALTO_PUT(utilin,16,15,15,val)

/** @brief put mouse bits */
#define	PUT_MOUSE(utilin,val)		ALTO_PUT(utilin,16,13,15,val)

/**
 * @brief printer paper strobe bit
 * Paper strobe bit. Toggling this bit causes a paper scrolling operation.
 */
#define	GET_PPPSTR(utilout)		ALTO_GET(utilout,16,0,0)

/**
 * @brief printer retstore bit
 * Restore bit. Toggling this bit resets the printer (including clearing
 * the "check" condition if present) and moves the carriage to the
 * left margin.
 */
#define	GET_PREST(utilout)		ALTO_GET(utilout,16,1,1)

/**
 * @brief printer ribbon bit
 * Ribbon bit. When this bit is 1 the ribbon is up (in printing
 * position); when 0, it is down.
 */
#define	GET_PRIB(utilout)		ALTO_GET(utilout,16,2,2)

/**
 * @brief printer daisy strobe bit
 * Daisy strobe bit. Toggling this bit causes a character to be printed.
 */
#define	GET_PCHSTR(utilout)		ALTO_GET(utilout,16,3,3)

/**
 * @brief printer carriage strobe bit
 * Carriage strobe bit. Toggling this bit causes a horizontal position operation.
 */
#define	GET_PCARSTR(utilout)		ALTO_GET(utilout,16,4,4)

/**
 * @brief printer data
 * Argument to various output operations:
 * 1. Printing characters. When the daisy bit is toggled bits 9-15 of this field
 * are interpreted as an ASCII character code to be printed (it should be noted
 * that all codes less than 040 print as lower case "w").
 * 2. For paper and carriage operations the field is interpreted as a displacement
 * (-1024 to +1023), in units of 1/48 inch for paper and 1/60 inch for carriage.
 * Positive is down or to the right, negative up or to the left. The value is
 * represented as sign-magnitude (i.e., bit 5 is 1 for negative numbers, 0 for
 * positive; bits 6-15 are the absolute value of the number).
 */
#define	GET_PDATA(utilout)		ALTO_GET(utilout,16,5,15)

/** @brief miscellaneous hardware registers in the memory mapped I/O range */
typedef struct {
	/** @brief the EIA port at 0177001 */
	int eia;

	/** @brief the UTILOUT port at 0177016 (active-low outputs) */
	int utilout;

	/** @brief the XBUS port at 0177020 to 0177023 */
	int xbus[4];

	/** @brief the UTILIN port at 0177030 to 0177033 (same value on all addresses) */
	int utilin;
}	hardware_t;

/** @brief the miscellaneous hardware context */
extern hardware_t hw;

/** @brief read an UTILIN address */
extern int utilin_r(int addr);

/** @brief read the UTILOUT address */
extern int utilout_r(int addr);

/** @brief write the UTILOUT address */
extern void utilout_w(int addr, int data);

/** @brief read an XBUS address */
extern int xbus_r(int addr);

/** @brief write an XBUS address (?) */
extern void xbus_w(int addr, int data);

/** @brief initialize miscellaneous hardware */
extern int init_hardware(void);

#endif	/* !defined(_HARDWARE_H_INCLUDED_) */
