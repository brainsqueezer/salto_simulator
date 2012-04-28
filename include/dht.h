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
 * F2 handling for display horizontal task
 *
 * $Id: dht.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_DHT_H_INCLUDED_)
#define	_DHT_H_INCLUDED_

#include "alto.h"
#include "cpu.h"
#include "display.h"

/** @brief f2 task specific: dht */
#define	f2_dht_evenfield	f2_task_10
#define	f2_dht_setmode		f2_task_11

/** @brief initialize display horizontal task */
extern int init_dht(int task);

#endif	/* !defined(_DHT_H_INCLUDED_) */
