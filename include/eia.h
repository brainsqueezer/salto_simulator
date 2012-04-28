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
 * $Id: eia.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_EIA_H_INCLUDED_)
#define _EIA_H_INCLUDED_

/** @brief EIA line selection bits */
typedef enum {
	EIA_0, EIA_1, EIA_2,
	EIA_MAX
}	eia_line_t;

/** @brief EIA control words (less significant bits not defined here) */
typedef enum {
	EIA_DATA	= 0000000,
	EIA_INITIF	= 0100000,
	EIA_INITRT	= 0110000,
	EIA_INITRG	= 0120000,
	EIA_RESET	= 0130000,
	EIA_SWI		= 0140000,
	EIA_INTA	= 0150000,
	EIA_CONTROL_MASK= 0170000
}	eia_ctrl_t;

/**
 * @brief write a EIA control (or data) word
 *
 * The EIA_CONTROL_MASK bits define if a control command,
 * or if a data word is written.
 */
extern void eia_w(int addr, int data);

/**
 * @brief read EIA status word on first read, data on following reads
 *
 * Reading the status word causes the board to begin setup for reading
 * the data register. This makes it impossible to read the status more
 * than once per interrupt; if a second attempt is made, the input data
 * will appear instead. This condition is reset by the control command
 * to acknowledge the interrupt (INTA).
 */
extern int eia_r(int addr);

/**
 * @brief reset the EIA serial communication control interface context
 *
 * @result return 0 on success
 */
extern int init_eia(void);

#endif /* !defined(_EIA_H_INCLUDED_) */
