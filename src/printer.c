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
 * Printer functions
 *
 * $Id: printer.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <SDL.h>

#include "alto.h"
#include "cpu.h"
#include "debug.h"
#include "hardware.h"
#include "printer.h"

/**
 * @brief structure of the printer context
 */
typedef struct {
	/** @brief paper status (1 paper ready, 0 no paper) */
	int paper;

	/** @brief daisy status (1 can print char, 0 can't print char) */
	int daisy;

	/** @brief check status (1 error, 0 no error) */
	int check;

	/** @brief carriage status (1 can move carriage, 0 can't move carriage) */
	int carriage;

	/** @brief ribbon status (1 ribbon is up (i.e. printing position), 0 ribbon is down) */
	int ribbon;

	/** @brief reset status (1 reset, 0 don't reset) */
	int reset;

}	printer_t;

static printer_t prt;

/**
 * @brief set printer status bits (called before reading UTILIN)
 *
 * Change the appropriate bits of the UTILIN port to reflect the
 * printer status
 */
void printer_read(void)
{
	/* paper ready */
	PUT_PPRDY(hw.utilin, prt.paper ^ 1);

	/* printer check (never for now) */
	PUT_PCHECK(hw.utilin, prt.check ^ 1);

	/* printer daisy ready */
	PUT_PCHRDY(hw.utilin, prt.daisy ^ 1);

	/* printer carriage ready */
	PUT_PCARRDY(hw.utilin, prt.carriage ^ 1);

	/* printer ready (all of above) */
	PUT_PREADY(hw.utilin,
		((prt.paper | prt.daisy | prt.carriage) ^ 1) | prt.check);
}

/**
 * @brief set printer output bits (called after writing UTILOUT)
 *
 * Change the appropriate bits of the UTILIN port to reflect the
 * printer status
 */
void printer_write(void)
{
	int data = GET_PDATA(hw.utilout);

	/* signed 10 bit value, sign in bit 10 */
	if (data & (1 << 10))
		data = -(data & ((1<<10) - 1));

	if (GET_PREST(hw.utilout)) {
		/* assert reset */
		prt.reset = 1;
		prt.paper = 0;
		prt.daisy = 0;
		prt.check = 0;
		prt.carriage = 0;
		return;
	} else if (prt.reset) {
		/* deassert reset */
		prt.paper = 1;		/* we have paper */
		prt.paper = 1;		/* we can print characters */
		prt.paper = 1;		/* we can move the carriage */
		prt.reset = 0;		/* reset is gone */
	}

	if (GET_PRIB(hw.utilout) != prt.ribbon) {
		prt.ribbon = GET_PRIB(hw.utilout);
		printf("ribbon %s\n", prt.ribbon ? "up" : "down");
	}

	/* paper strobe? */
	if (GET_PPPSTR(hw.utilout)) {
		if (prt.paper) {
			printf("paper %+d/48 inch\n", data);
			prt.paper = 0;
		}
	} else if (0 == prt.paper) {
		prt.paper = 1;
	}

	/* carriage strobe? */
	if (GET_PCARSTR(hw.utilout)) {
		if (prt.carriage) {
			prt.carriage = 0;
			printf("carriage %+d/60 inch\n", data);
		}
	} else if (0 == prt.carriage) {
		prt.carriage = 1;
	}

	/* daisy strobe? */
	if (GET_PCHSTR(hw.utilout)) {
		if (prt.daisy) {
			prt.daisy = 0;
			printf("print char %03o\n", data & 0177);
		}
	} else if (0 == prt.daisy) {
		prt.daisy = 1;
	}
}

/**
 * @brief reset the printer context
 *
 * @result return 0 on success
 */
int init_printer(void)
{
	memset(&prt, 0, sizeof(prt));

	return 0;
}
