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
 * Keyboard memory mapped I/O functions
 *
 * $Id: hardware.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <SDL.h>

#include "alto.h"
#include "cpu.h"
#include "memory.h"
#include "hardware.h"
#include "printer.h"

/** @brief the miscellaneous hardware context */
hardware_t hw;

/**
 * @brief read the UTILIN port
 *
 * @param addr memory mapped I/O address to be read
 * @result current value on the UTILIN port
 */
int utilin_r(int addr)
{
	int data;
	/* update the printer status */
	printer_read();

	data = hw.utilin;

	LOG((0,2,"	read UTILIN %#o (%#o)\n", addr, data));
	return data;
}

/**
 * @brief read the XBUS port
 *
 * @param addr memory mapped I/O address to be read
 * @result current value on the XBUS port latch
 */
int xbus_r(int addr)
{
	int data = hw.xbus[addr & 3];

	LOG((0,2,"	read XBUS[%d] %#o (%#o)\n", addr&3, addr, data));
	return data;
}

/**
 * @brief write the XBUS port
 *
 * The actual outputs are active-low.
 *
 * @param addr memory mapped I/O address to be read
 * @param data value to write to the XBUS port latch
 */
void xbus_w(int addr, int data)
{
	LOG((0,2,"	write XBUS[%d] %#o (%#o)\n", addr&3, addr, data));
	hw.xbus[addr&3] = data;
}

/**
 * @brief read the UTILOUT port
 *
 * @param addr memory mapped I/O address to be read
 * @result current value on the UTILOUT port latch
 */
int utilout_r(int addr)
{
	int data = hw.utilout ^ 0177777;
	LOG((0,2,"	read UTILOUT %#o (%#o)\n", addr, data));
	return data;
}

/**
 * @brief write the UTILOUT port
 *
 * The actual outputs are active-low.
 *
 * @param addr memory mapped I/O address to be read
 * @param data value to write to the UTILOUT port latch
 */
void utilout_w(int addr, int data)
{
	LOG((0,2,"	write UTILOUT %#o (%#o)\n", addr, data));
	hw.utilout = data ^ 0177777;

	/* write printer data */
	printer_write();
}

/**
 * @brief clear all keys and install the mmio handler for KBDAD to KBDAD+3
 *
 * @result return 0 on success
 */
int init_hardware(void)
{
	memset(&hw, 0, sizeof(hw));

	/* open inputs on UTILIN */
	hw.utilin = 0177777;

	/* open inputs on the XBUS (?) */
	hw.xbus[0] = 0177777;
	hw.xbus[1] = 0177777;
	hw.xbus[2] = 0177777;
	hw.xbus[3] = 0177777;

	install_mmio_fn(0177016, 0177016, utilout_r, utilout_w);
	install_mmio_fn(0177020, 0177023, xbus_r, xbus_w);
	install_mmio_fn(0177030, 0177033, utilin_r, NULL);

	return 0;
}
