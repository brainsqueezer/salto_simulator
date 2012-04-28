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
 * $Id: mouse.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_MOUSE_H_INCLUDED_)
#define	_MOUSE_H_INCLUDED_

/** @brief MX1 signal is bit 0 (latch bit 1) */
#define	MX1	(1<<0)
#define	LMX1	(1<<1)

/** @brief MX2 signal is bit 2 (latch bit 3) */
#define	MX2	(1<<2)
#define	LMX2	(1<<3)

/** @brief MY1 signal is bit 4 (latch bit 5) */
#define	MY1	(1<<4)
#define	LMY1	(1<<5)

/** @brief MY2 signal is bit 6 (latch bit 7) */
#define	MY2	(1<<6)
#define	LMY2	(1<<7)

/** @brief mask for the active bits */
#define	MACTIVE	(MX1|MX2|MY1|MY2)

/** @brief mask for the latched bits */
#define	MLATCH	(LMX1|LMX2|LMY1|LMY2)

/**
 * @brief structure of the mouse context
 */
typedef struct {
	/** @brief current mouse x */
	int x;

	/** @brief current mouse y */
	int y;

	/** @brief mouse destination x */
	int dx;

	/** @brief mouse destination y */
	int dy;

	/** @brief mouse PROM address latch */
	int latch;

}	mouse_t;

extern mouse_t mouse;

/** @brief mouse branch lookup table PROM */
extern uint8_t madr_a32[256];

/** @brief return the mouse motion flags */
extern int mouse_read(void);

/** @brief register a mouse motion */
extern void mouse_motion(int x, int y);

/** @brief register a mouse button change */
extern void mouse_button(int b);

/** @brief initialize display */
extern int mouse_init(void);

#endif	/* !defined(_MOUSE_H_INCLUDED_) */
