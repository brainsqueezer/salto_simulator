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
 * Mouse emulation
 *
 * $Id: mouse.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "memory.h"
#include "display.h"
#include "hardware.h"
#include "mouse.h"

/**
 * @brief structure of the display context
 */
mouse_t mouse;

/**
 * @brief PROM madr.a32 contains a lookup table to translate mouse motions
 *
 * <PRE>
 * The 4 mouse motion signals MX1, MX2, MY1, and MY2 are connected
 * to a 256x4 PROM's (3601, SN74387) address lines A0, A2, A4, and A6.
 * The previous (latched) state of the 4 signals is connected to the
 * address lines A1, A3, A5, and A7.
 *
 *                  SN74387
 *               +---+--+---+
 *               |   +--+   |
 *  MY2     A6  -|1       16|-  Vcc
 *               |          |
 *  LMY1    A5  -|2       15|-  A7     LMY2
 *               |          |
 *  MY1     A4  -|3       14|-  FE1'   0
 *               |          |
 *  LMX2    A3  -|4       13|-  FE2'   0
 *               |          |
 *  MX1     A0  -|5       12|-  D0     BUS[12]
 *               |          |
 *  LMX1    A1  -|6       11|-  D1     BUS[13]
 *               |          |
 *  MX2     A2  -|7       10|-  D2     BUS[14]
 *               |          |
 *         GND  -|8        9|-  D3     BUS[15]
 *               |          |
 *               +----------+
 *
 * A motion to the west will first toggle MX2, then MX1.
 * sequence: 04 -> 0d -> 0b -> 02
 * A motion to the east will first toggle MX1, then MX2.
 * sequence: 01 -> 07 -> 0e -> 08
 *
 * A motion to the north will first toggle MY2, then MY1.
 * sequence: 40 -> d0 -> b0 -> 20
 * A motion to the south will first toggle MY1, then MY2.
 * sequence: 10 -> 70 -> e0 -> 80
 *
 * This dump is from PROM madr.a32:
 * 0000: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017,
 * 0020: 003,015,005,003,005,003,003,015,015,003,003,005,003,005,015,003,
 * 0040: 011,001,016,011,016,011,011,001,001,011,011,016,011,016,001,011,
 * 0060: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017,
 * 0100: 011,001,016,011,016,011,011,001,001,011,011,016,011,016,001,011,
 * 0120: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017,
 * 0140: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017,
 * 0160: 003,015,005,003,005,003,003,015,015,003,003,005,003,005,015,003,
 * 0200: 003,015,005,003,005,003,003,015,015,003,003,005,003,005,015,003,
 * 0220: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017,
 * 0240: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017,
 * 0260: 011,001,016,011,016,011,011,001,001,011,011,016,011,016,001,011,
 * 0300: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017,
 * 0320: 011,001,016,011,016,011,011,001,001,011,011,016,011,016,001,011,
 * 0340: 003,015,005,003,005,003,003,015,015,003,003,005,003,005,015,003,
 * 0360: 017,007,013,017,013,017,017,007,007,017,017,013,017,013,007,017
 * </PRE>
 */
uint8_t madr_a32[256];

/**
 * <PRE>
 * The mouse inputs from the shutters are connected to a quad
 * 2/3 input RS flip flop (SN74279).
 *
 *          74279
 *       +---+--+---+
 *       |   +--+   |
 *   R1 -|1       16|- Vcc
 *       |          |
 *  S1a -|2       15|- S4
 *       |          |
 *  S1b -|3       14|- R4
 *       |          |
 *   Q1 -|4       13|- Q4
 *       |          |
 *   R2 -|5       12|- S3a
 *       |          |
 *   S2 -|6       11|- S3b
 *       |          |
 *   Q2 -|7       10|- R3
 *       |          |
 *  GND -|8        9|- Q3
 *       |          |
 *       +----------+
 *
 * The 'Y' Encoder signals are connected to IC1:
 *	shutter	pin(s)	R/S	output
 *	------------------------------------
 *	0	2,3 	S1a,b	Q1 MX2 -> 1
 *	1	1	R1	Q1 MX2 -> 0
 *	2	5	R2	Q2 MX1 -> 0
 *	3	6	S2	Q2 MX1 -> 1
 *
 * The 'X' Encoder signals are connected to IC2:
 *	shutter	pin(s)	R/S	output
 *	------------------------------------
 *	0	2,3 	S1a,b	Q1 MY2 -> 1
 *	1	1	R1	Q1 MY2 -> 0
 *	2	5	R2	Q2 MY1 -> 0
 *	3	6	S2	Q2 MY1 -> 1
 *
 *
 * The pulse train generated by a left or up rotation is:
 *
 *             +---+   +---+   +---+
 * MX1/MY1     |   |   |   |   |   |
 *          ---+   +---+   +---+   +---
 *
 *           +---+   +---+   +---+   +-
 * MX2/MY2   |   |   |   |   |   |   |
 *          -+   +---+   +---+   +---+
 *
 *
 * The pulse train generated by a right or down rotation is:
 *
 *           +---+   +---+   +---+   +-
 * MX1/MY1   |   |   |   |   |   |   |
 *          -+   +---+   +---+   +---+
 *
 *             +---+   +---+   +---+
 * MX2/MY2     |   |   |   |   |   |
 *          ---+   +---+   +---+   +---
 *
 * In order to simulate the shutter sequence for the mouse motions
 * we have to generate a sequence of pulses on MX1/MX2 and MY1/MY2
 * that have their phases shifted by 90°.
 * </PRE>
 */


#define	MOVEX(x) ((((x) < 0) ? MY2 : ((x) > 0) ? MY1 : 0))
#define	MOVEY(y) ((((y) < 0) ? MX2 : ((y) > 0) ? MX1 : 0))

/**
 * @brief return the mouse motion flags
 *
 * Advance the mouse x and y coordinates to the dx and dy
 * coordinates by either toggling MX2 or MX1 first for a
 * y movement, MY2 or MY1 for x movement.
 *
 * @result lookup value from madr_a32
 */
int mouse_read(void)
{
	static int arg;
	int data;

	mouse.latch = (mouse.latch << 1) & MLATCH;
	data = madr_a32[mouse.latch];

	switch (arg & 3) {
	case 0:
		mouse.latch |= MOVEX(mouse.dx - mouse.x);
		mouse.latch |= MOVEY(mouse.dy - mouse.y);
		break;
	case 1:
		mouse.latch |= MACTIVE;
		if (mouse.x < mouse.dx)
			mouse.x++;
		else if (mouse.x > mouse.dx)
			mouse.x--;
		if (mouse.y < mouse.dy)
			mouse.y++;
		else if (mouse.y > mouse.dy)
			mouse.y--;
		break;
	case 2:
		mouse.latch ^= MOVEX(mouse.dx - mouse.x);
		mouse.latch ^= MOVEY(mouse.dy - mouse.y);
		break;
	default:
		mouse.latch &= ~MACTIVE;
		if (mouse.x < mouse.dx)
			mouse.x++;
		else if (mouse.x > mouse.dx)
			mouse.x--;
		if (mouse.y < mouse.dy)
			mouse.y++;
		else if (mouse.y > mouse.dy)
			mouse.y--;
	}
	arg++;

	return data;
}

/**
 * @brief register a mouse motion
 *
 * @param x new mouse x coordinate
 * @param y new mouse y coordinate
 */
void mouse_motion(int x, int y)
{
	/* set new destination (absolute) mouse x and y coordinates */
	mouse.dx = x;
	mouse.dy = y;
#if	1
	/* XXX: dirty, dirty, hack */
#if	HAMMING_CHECK
	mem.ram[0424/2] = hamming_code(1, 0424 / 2, (x << 16) | y);
#else
	mem.ram[0424/2] = (x << 16) | y;
#endif
#endif
}

/**
 * @brief register mouse button change
 *
 * convert button bits to UTILIN[13-15]
 *
 * @param b mouse buttons (bit 0:left 1:right 2:middle)
 */
void mouse_button(int b)
{
	/* UTILIN[13] TOP or LEFT button (RED) */
	PUT_MOUSE_RED   (hw.utilin, (b & SDL_BUTTON_LMASK) ? 0 : 1);
	/* UTILIN[14] BOTTOM or RIGHT button (BLUE) */
	PUT_MOUSE_BLUE  (hw.utilin, (b & SDL_BUTTON_RMASK) ? 0 : 1);
	/* UTILIN[15] MIDDLE button (YELLOW) */
	PUT_MOUSE_YELLOW(hw.utilin, (b & SDL_BUTTON_MMASK) ? 0 : 1);
}


/**
 * @brief initialize the mouse context to useful values
 *
 * From the Alto Hardware Manual:
 * <PRE>
 * The mouse is a hand-held pointing device which contains two encoders
 * which digitize its position as it is rolled over a table-top. It also
 * has three buttons which may be read as the three low order bits of
 * memory location UTILIN (0177030), iin the manner of the keyboard.
 * The bit/button correspondence in UTILIN are (depressed keys
 * correspond to 0's in memory):
 *
 *      UTILIN[13]      TOP or LEFT button (RED)
 *      UTILIN[14]      BOTTOM or RIGHT button (BLUE)
 *      UTILIN[15]      MIDDLE button (YELLOW)
 *
 * The mouse coordinates are maintained by the MRT microcode in locations
 * MOUSELOC(0424)=X and MOUSELOC+1(0425)=Y in page one of the Alto memory.
 * These coordinates are relative, i.e., the hardware only increments and
 * decrements them. The resolution of the mouse is approximately 100 points
 * per inch.
 * </PRE>
 */
int mouse_init(void)
{
	memset(&mouse, 0, sizeof(mouse));
	return 0;
}
