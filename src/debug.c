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
 * Debugger
 *
 * $Id: debug.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <SDL.h>

#include "alto.h"
#include "cpu.h"
#include "emu.h"
#include "memory.h"
#include "timer.h"
#include "display.h"
#include "disk.h"
#include "drive.h"
#include "debug.h"

#if	DEBUG
loglevel_t ll[log_COUNT+1] = {
	{0, "emu",		"emulator task log"},
	{0, "task1",		"task 01 log"},
	{0, "task2",		"task 02 log"},
	{0, "task3",		"task 03 log"},
	{0, "ksec",		"disk sector task log"},
	{0, "task5",		"task 05 log"},
	{0, "task6",		"task 06 log"},
	{0, "ether",		"Ethernet task log"},
	{0, "mrt",		"memory refresh task log"},
	{0, "dwt",		"display word task log"},
	{0, "curt",		"cursor task log"},
	{0, "dht",		"display horizontal task log"},
	{0, "dvt",		"display vertical task log"},
	{0, "part",		"parity error task log"},
	{0, "kwd",		"disk word task log"},
	{0, "task17",		"task 017 log"},
	{0, "tsw",		"task switch log"},
	{0, "mem",		"memory I/O log"},
	{0, "tmr",		"timer functions log"},
	{0, "dsp",		"display functions log"},
	{0, "dsk",		"disk functions log"},
	{0, "drv",		"drive functions log"},
	{0, "misc",		"miscellaneous functions log"},
	{0, "all",		"all debug log"}
};

#endif

/** @brief debugger context */
debug_t dbg;

/** @brief update rate (draw every xxx CPU cycles) */
int update_rate = 100000;

#define	I(x) ((x)^0xff)

static const uint8_t chargen_6x10[] = {
#if	1
	0x00,  0x00,0xa8,0x00,0x88,0x00,0x88,0x00,0xa8,0x00,0x00,
	0x1e,  0x00,0x08,0x18,0x38,0x78,0x78,0x38,0x18,0x08,0x00,
	0x1f,  0x00,0x80,0xc0,0xe0,0xf0,0xf0,0xe0,0xc0,0x80,0x00,
	0x20,  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x21,  0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,0x00,
	0x22,  0x00,0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,
	0x23,  0x00,0x50,0x50,0xf8,0x50,0xf8,0x50,0x50,0x00,0x00,
	0x24,  0x00,0x20,0x70,0xa0,0x70,0x28,0x70,0x20,0x00,0x00,
	0x25,  0x00,0x48,0xa8,0x50,0x20,0x50,0xa8,0x90,0x00,0x00,
	0x26,  0x00,0x40,0xa0,0xa0,0x40,0xa8,0x90,0x68,0x00,0x00,
	0x27,  0x00,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
	0x28,  0x00,0x10,0x20,0x40,0x40,0x40,0x20,0x10,0x00,0x00,
	0x29,  0x00,0x40,0x20,0x10,0x10,0x10,0x20,0x40,0x00,0x00,
	0x2a,  0x00,0x00,0x88,0x50,0xf8,0x50,0x88,0x00,0x00,0x00,
	0x2b,  0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,0x00,
	0x2c,  0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x20,0x40,0x00,
	0x2d,  0x00,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,0x00,
	0x2e,  0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x70,0x20,0x00,
	0x2f,  0x00,0x08,0x08,0x10,0x20,0x40,0x80,0x80,0x00,0x00,
	0x30,  0x00,0x20,0x50,0x88,0x88,0x88,0x50,0x20,0x00,0x00,
	0x31,  0x00,0x20,0x60,0xa0,0x20,0x20,0x20,0xf8,0x00,0x00,
	0x32,  0x00,0x70,0x88,0x08,0x30,0x40,0x80,0xf8,0x00,0x00,
	0x33,  0x00,0xf8,0x08,0x10,0x30,0x08,0x88,0x70,0x00,0x00,
	0x34,  0x00,0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0x00,
	0x35,  0x00,0xf8,0x80,0xb0,0xc8,0x08,0x88,0x70,0x00,0x00,
	0x36,  0x00,0x30,0x40,0x80,0xb0,0xc8,0x88,0x70,0x00,0x00,
	0x37,  0x00,0xf8,0x08,0x10,0x10,0x20,0x40,0x40,0x00,0x00,
	0x38,  0x00,0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x00,
	0x39,  0x00,0x70,0x88,0x98,0x68,0x08,0x10,0x60,0x00,0x00,
	0x3a,  0x00,0x00,0x20,0x70,0x20,0x00,0x20,0x70,0x20,0x00,
	0x3b,  0x00,0x00,0x20,0x70,0x20,0x00,0x30,0x20,0x40,0x00,
	0x3c,  0x00,0x08,0x10,0x20,0x40,0x20,0x10,0x08,0x00,0x00,
	0x3d,  0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,0x00,
	0x3e,  0x00,0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x00,
	0x3f,  0x00,0x70,0x88,0x10,0x20,0x20,0x00,0x20,0x00,0x00,
	0x40,  0x00,0x70,0x88,0x98,0xa8,0xb0,0x80,0x70,0x00,0x00,
	0x41,  0x00,0x20,0x50,0x88,0x88,0xf8,0x88,0x88,0x00,0x00,
	0x42,  0x00,0xf0,0x48,0x48,0x70,0x48,0x48,0xf0,0x00,0x00,
	0x43,  0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,0x00,
	0x44,  0x00,0xf0,0x48,0x48,0x48,0x48,0x48,0xf0,0x00,0x00,
	0x45,  0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,0x00,
	0x46,  0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x00,
	0x47,  0x00,0x70,0x88,0x80,0x80,0x98,0x88,0x70,0x00,0x00,
	0x48,  0x00,0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x00,
	0x49,  0x00,0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00,
	0x4a,  0x00,0x38,0x10,0x10,0x10,0x10,0x90,0x60,0x00,0x00,
	0x4b,  0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,0x00,
	0x4c,  0x00,0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x00,
	0x4d,  0x00,0x88,0x88,0xd8,0xa8,0x88,0x88,0x88,0x00,0x00,
	0x4e,  0x00,0x88,0x88,0xc8,0xa8,0x98,0x88,0x88,0x00,0x00,
	0x4f,  0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0x50,  0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x00,
	0x51,  0x00,0x70,0x88,0x88,0x88,0x88,0xa8,0x70,0x08,0x00,
	0x52,  0x00,0xf0,0x88,0x88,0xf0,0xa0,0x90,0x88,0x00,0x00,
	0x53,  0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,0x00,
	0x54,  0x00,0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,
	0x55,  0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0x56,  0x00,0x88,0x88,0x88,0x50,0x50,0x50,0x20,0x00,0x00,
	0x57,  0x00,0x88,0x88,0x88,0xa8,0xa8,0xd8,0x88,0x00,0x00,
	0x58,  0x00,0x88,0x88,0x50,0x20,0x50,0x88,0x88,0x00,0x00,
	0x59,  0x00,0x88,0x88,0x50,0x20,0x20,0x20,0x20,0x00,0x00,
	0x5a,  0x00,0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x00,
	0x5b,  0x00,0x70,0x40,0x40,0x40,0x40,0x40,0x70,0x00,0x00,
	0x5c,  0x00,0x80,0x80,0x40,0x20,0x10,0x08,0x08,0x00,0x00,
	0x5d,  0x00,0x70,0x10,0x10,0x10,0x10,0x10,0x70,0x00,0x00,
	0x5e,  0x00,0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00,0x00,
	0x5f,  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x00,
	0x60,  0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x61,  0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x00,
	0x62,  0x00,0x80,0x80,0xb0,0xc8,0x88,0xc8,0xb0,0x00,0x00,
	0x63,  0x00,0x00,0x00,0x70,0x88,0x80,0x88,0x70,0x00,0x00,
	0x64,  0x00,0x08,0x08,0x68,0x98,0x88,0x98,0x68,0x00,0x00,
	0x65,  0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x00,
	0x66,  0x00,0x30,0x48,0x40,0xf0,0x40,0x40,0x40,0x00,0x00,
	0x67,  0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x88,0x70,
	0x68,  0x00,0x80,0x80,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,
	0x69,  0x00,0x20,0x00,0x60,0x20,0x20,0x20,0x70,0x00,0x00,
	0x6a,  0x00,0x08,0x00,0x18,0x08,0x08,0x08,0x48,0x48,0x30,
	0x6b,  0x00,0x80,0x80,0x88,0x90,0xe0,0x90,0x88,0x00,0x00,
	0x6c,  0x00,0x60,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00,
	0x6d,  0x00,0x00,0x00,0xd0,0xa8,0xa8,0xa8,0x88,0x00,0x00,
	0x6e,  0x00,0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,
	0x6f,  0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,
	0x70,  0x00,0x00,0x00,0xb0,0xc8,0x88,0xc8,0xb0,0x80,0x80,
	0x71,  0x00,0x00,0x00,0x68,0x98,0x88,0x98,0x68,0x08,0x08,
	0x72,  0x00,0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,
	0x73,  0x00,0x00,0x00,0x70,0x80,0x70,0x08,0xf0,0x00,0x00,
	0x74,  0x00,0x40,0x40,0xf0,0x40,0x40,0x48,0x30,0x00,0x00,
	0x75,  0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x00,
	0x76,  0x00,0x00,0x00,0x88,0x88,0x50,0x50,0x20,0x00,0x00,
	0x77,  0x00,0x00,0x00,0x88,0x88,0xa8,0xa8,0x50,0x00,0x00,
	0x78,  0x00,0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,
	0x79,  0x00,0x00,0x00,0x88,0x88,0x98,0x68,0x08,0x88,0x70,
	0x7a,  0x00,0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x00,
	0x7b,  0x00,0x18,0x20,0x10,0x60,0x10,0x20,0x18,0x00,0x00,
	0x7c,  0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,
	0x7d,  0x00,0x60,0x10,0x20,0x18,0x20,0x10,0x60,0x00,0x00,
	0x7e,  0x00,0x48,0xa8,0x90,0x00,0x00,0x00,0x00,0x00,0x00,
	0xa0,  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xa1,  0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x00,
	0xa2,  0x00,0x00,0x20,0x78,0xa0,0xa0,0xa0,0x78,0x20,0x00,
	0xa3,  0x00,0x30,0x48,0x40,0xe0,0x40,0x48,0xb0,0x00,0x00,
	0xa4,  0x00,0x00,0x00,0x88,0x70,0x50,0x70,0x88,0x00,0x00,
	0xa5,  0x00,0x88,0x88,0x50,0x20,0xf8,0x20,0x20,0x20,0x00,
	0xa6,  0x00,0x20,0x20,0x20,0x00,0x20,0x20,0x20,0x00,0x00,
	0xa7,  0x00,0x70,0x80,0xe0,0x90,0x48,0x38,0x08,0x70,0x00,
	0xa8,  0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xa9,  0x00,0x70,0x88,0xa8,0xc8,0xa8,0x88,0x70,0x00,0x00,
	0xaa,  0x00,0x38,0x48,0x58,0x28,0x00,0x78,0x00,0x00,0x00,
	0xab,  0x00,0x00,0x00,0x24,0x48,0x90,0x48,0x24,0x00,0x00,
	0xac,  0x00,0x00,0x00,0x00,0x78,0x08,0x00,0x00,0x00,0x00,
	0xad,  0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,
	0xae,  0x00,0x70,0x88,0xe8,0xc8,0xc8,0x88,0x70,0x00,0x00,
	0xaf,  0xf8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xb0,  0x00,0x20,0x50,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
	0xb1,  0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0xf8,0x00,0x00,
	0xb2,  0x30,0x48,0x10,0x20,0x78,0x00,0x00,0x00,0x00,0x00,
	0xb3,  0x70,0x08,0x30,0x08,0x70,0x00,0x00,0x00,0x00,0x00,
	0xb4,  0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xb5,  0x00,0x00,0x00,0x88,0x88,0x88,0xc8,0xb0,0x80,0x00,
	0xb6,  0x00,0x78,0xe8,0xe8,0x68,0x28,0x28,0x28,0x00,0x00,
	0xb7,  0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,
	0xb8,  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20,
	0xb9,  0x20,0x60,0x20,0x20,0x70,0x00,0x00,0x00,0x00,0x00,
	0xba,  0x00,0x30,0x48,0x48,0x30,0x00,0x78,0x00,0x00,0x00,
	0xbb,  0x00,0x00,0x00,0x90,0x48,0x24,0x48,0x90,0x00,0x00,
	0xbc,  0x40,0xc0,0x40,0x40,0xe4,0x0c,0x14,0x3c,0x04,0x00,
	0xbd,  0x40,0xc0,0x40,0x40,0xe8,0x14,0x04,0x08,0x1c,0x00,
	0xbe,  0xc0,0x20,0x40,0x20,0xc8,0x18,0x28,0x78,0x08,0x00,
	0xbf,  0x00,0x20,0x00,0x20,0x20,0x40,0x88,0x70,0x00,0x00,
	0xc0,  0x40,0x20,0x70,0x88,0x88,0xf8,0x88,0x88,0x00,0x00,
	0xc1,  0x10,0x20,0x70,0x88,0x88,0xf8,0x88,0x88,0x00,0x00,
	0xc2,  0x20,0x50,0x70,0x88,0x88,0xf8,0x88,0x88,0x00,0x00,
	0xc3,  0x48,0xb0,0x70,0x88,0x88,0xf8,0x88,0x88,0x00,0x00,
	0xc4,  0x50,0x00,0x70,0x88,0x88,0xf8,0x88,0x88,0x00,0x00,
	0xc5,  0x20,0x50,0x70,0x88,0x88,0xf8,0x88,0x88,0x00,0x00,
	0xc6,  0x00,0x3c,0x50,0x90,0x9c,0xf0,0x90,0x9c,0x00,0x00,
	0xc7,  0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x20,0x40,
	0xc8,  0x40,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,0x00,
	0xc9,  0x10,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,0x00,
	0xca,  0x20,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,0x00,
	0xcb,  0x50,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,0x00,
	0xcc,  0x40,0x20,0x70,0x20,0x20,0x20,0x20,0x70,0x00,0x00,
	0xcd,  0x10,0x20,0x70,0x20,0x20,0x20,0x20,0x70,0x00,0x00,
	0xce,  0x20,0x50,0x70,0x20,0x20,0x20,0x20,0x70,0x00,0x00,
	0xcf,  0x50,0x00,0x70,0x20,0x20,0x20,0x20,0x70,0x00,0x00,
	0xd0,  0x00,0xf0,0x48,0x48,0xe8,0x48,0x48,0xf0,0x00,0x00,
	0xd1,  0x28,0x50,0x88,0xc8,0xa8,0x98,0x88,0x88,0x00,0x00,
	0xd2,  0x40,0x20,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xd3,  0x10,0x20,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xd4,  0x20,0x50,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xd5,  0x28,0x50,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xd6,  0x50,0x00,0x70,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xd7,  0x00,0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,
	0xd8,  0x00,0x70,0x98,0x98,0xa8,0xc8,0xc8,0x70,0x00,0x00,
	0xd9,  0x40,0x20,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xda,  0x10,0x20,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xdb,  0x20,0x50,0x00,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xdc,  0x50,0x00,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00,
	0xdd,  0x10,0x20,0x88,0x88,0x50,0x20,0x20,0x20,0x00,0x00,
	0xde,  0x00,0x80,0xf0,0x88,0xf0,0x80,0x80,0x80,0x00,0x00,
	0xdf,  0x00,0x70,0x88,0x90,0xa0,0x90,0x88,0xb0,0x00,0x00,
	0xe0,  0x40,0x20,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x00,
	0xe1,  0x10,0x20,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x00,
	0xe2,  0x20,0x50,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x00,
	0xe3,  0x28,0x50,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x00,
	0xe4,  0x00,0x50,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x00,
	0xe5,  0x20,0x50,0x20,0x70,0x08,0x78,0x88,0x78,0x00,0x00,
	0xe6,  0x00,0x00,0x00,0x78,0x14,0x7c,0x90,0x7c,0x00,0x00,
	0xe7,  0x00,0x00,0x00,0x70,0x88,0x80,0x88,0x70,0x20,0x40,
	0xe8,  0x40,0x20,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x00,
	0xe9,  0x10,0x20,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x00,
	0xea,  0x20,0x50,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x00,
	0xeb,  0x00,0x50,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x00,
	0xec,  0x40,0x20,0x00,0x60,0x20,0x20,0x20,0x70,0x00,0x00,
	0xed,  0x20,0x40,0x00,0x60,0x20,0x20,0x20,0x70,0x00,0x00,
	0xee,  0x20,0x50,0x00,0x60,0x20,0x20,0x20,0x70,0x00,0x00,
	0xef,  0x00,0x50,0x00,0x60,0x20,0x20,0x20,0x70,0x00,0x00,
	0xf0,  0x00,0xc0,0x30,0x70,0x88,0x88,0x88,0x70,0x00,0x00,
	0xf1,  0x28,0x50,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,
	0xf2,  0x40,0x20,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,
	0xf3,  0x10,0x20,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,
	0xf4,  0x20,0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,
	0xf5,  0x28,0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,
	0xf6,  0x00,0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,
	0xf7,  0x00,0x00,0x20,0x00,0xf8,0x00,0x20,0x00,0x00,0x00,
	0xf8,  0x00,0x00,0x00,0x78,0x98,0xa8,0xc8,0xf0,0x00,0x00,
	0xf9,  0x40,0x20,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x00,
	0xfa,  0x10,0x20,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x00,
	0xfb,  0x20,0x50,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x00,
	0xfc,  0x00,0x50,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x00,
	0xfd,  0x00,0x10,0x20,0x88,0x88,0x98,0x68,0x08,0x88,0x70,
	0xfe,  0x00,0x00,0x80,0xf0,0x88,0x88,0x88,0xf0,0x80,0x80,
	0xff,  0x00,0x50,0x00,0x88,0x88,0x98,0x68,0x08,0x88,0x70
#else
	0x00,	 0x00, 0xa8, 0x00, 0x88, 0x00, 0x88, 0x00, 0xa8, 0x00, 0x00,
	0x1e,	 0x00, 0x08, 0x18, 0x38, 0x78, 0x78, 0x38, 0x18, 0x08, 0x00,
	0x1f,	 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf0, 0xe0, 0xc0, 0x80, 0x00,
	0x20,	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x21,	 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x20, 0x00, 0x00,
	0x22,	 0x50, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x23,	 0x00, 0x00, 0x50, 0xf8, 0x50, 0x50, 0xf8, 0x50, 0x00, 0x00,
	0x24,	 0x20, 0x70, 0xa8, 0xa0, 0x70, 0x28, 0xa8, 0x70, 0x20, 0x00,
	0x25,	 0x00, 0xc8, 0xc8, 0x10, 0x20, 0x40, 0x98, 0x98, 0x00, 0x00,
	0x26,	 0x00, 0x40, 0xa0, 0xa0, 0x40, 0xa8, 0x90, 0x68, 0x00, 0x00,
	0x27,	 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x28,	 0x10, 0x20, 0x20, 0x40, 0x40, 0x40, 0x20, 0x20, 0x10, 0x00,
	0x29,	 0x40, 0x20, 0x20, 0x10, 0x10, 0x10, 0x20, 0x20, 0x40, 0x00,
	0x2a,	 0x00, 0x20, 0xa8, 0x70, 0x20, 0x70, 0xa8, 0x20, 0x00, 0x00,
	0x2b,	 0x00, 0x00, 0x20, 0x20, 0xf8, 0x20, 0x20, 0x00, 0x00, 0x00,
	0x2c,	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60, 0xc0, 0x00,
	0x2d,	 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2e,	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x00,
	0x2f,	 0x00, 0x08, 0x10, 0x10, 0x20, 0x40, 0x40, 0x80, 0x00, 0x00,
	0x30,	 0x00, 0x30, 0x48, 0x48, 0x48, 0x48, 0x48, 0x30, 0x00, 0x00,
	0x31,	 0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00,
	0x32,	 0x00, 0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0xf8, 0x00, 0x00,
	0x33,	 0x00, 0xf8, 0x08, 0x10, 0x30, 0x08, 0x88, 0x70, 0x00, 0x00,
	0x34,	 0x00, 0x10, 0x30, 0x50, 0x90, 0xf8, 0x10, 0x10, 0x00, 0x00,
	0x35,	 0x00, 0xf8, 0x80, 0xf0, 0x08, 0x08, 0x88, 0x70, 0x00, 0x00,
	0x36,	 0x00, 0x30, 0x40, 0x80, 0xf0, 0x88, 0x88, 0x70, 0x00, 0x00,
	0x37,	 0x00, 0xf8, 0x08, 0x10, 0x10, 0x20, 0x20, 0x20, 0x00, 0x00,
	0x38,	 0x00, 0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70, 0x00, 0x00,
	0x39,	 0x00, 0x70, 0x88, 0x88, 0x78, 0x08, 0x10, 0x60, 0x00, 0x00,
	0x3a,	 0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x60, 0x60, 0x00, 0x00,
	0x3b,	 0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x60, 0x60, 0xc0, 0x00,
	0x3c,	 0x00, 0x00, 0x10, 0x20, 0x40, 0x20, 0x10, 0x00, 0x00, 0x00,
	0x3d,	 0x00, 0x00, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
	0x3e,	 0x00, 0x00, 0x40, 0x20, 0x10, 0x20, 0x40, 0x00, 0x00, 0x00,
	0x3f,	 0x00, 0x70, 0x88, 0x10, 0x20, 0x20, 0x00, 0x20, 0x00, 0x00,
	0x40,	 0x00, 0x70, 0x88, 0xb8, 0xa8, 0xb8, 0x80, 0x70, 0x00, 0x00,
	0x41,	 0x00, 0x70, 0x88, 0x88, 0xf8, 0x88, 0x88, 0x88, 0x00, 0x00,
	0x42,	 0x00, 0xf0, 0x48, 0x48, 0x70, 0x48, 0x48, 0xf0, 0x00, 0x00,
	0x43,	 0x00, 0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 0x00, 0x00,
	0x44,	 0x00, 0xf0, 0x48, 0x48, 0x48, 0x48, 0x48, 0xf0, 0x00, 0x00,
	0x45,	 0x00, 0xf8, 0x80, 0x80, 0xf0, 0x80, 0x80, 0xf8, 0x00, 0x00,
	0x46,	 0x00, 0xf8, 0x80, 0x80, 0xf0, 0x80, 0x80, 0x80, 0x00, 0x00,
	0x47,	 0x00, 0x70, 0x88, 0x80, 0x80, 0x98, 0x88, 0x70, 0x00, 0x00,
	0x48,	 0x00, 0x88, 0x88, 0x88, 0xf8, 0x88, 0x88, 0x88, 0x00, 0x00,
	0x49,	 0x00, 0x70, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00,
	0x4a,	 0x00, 0x38, 0x10, 0x10, 0x10, 0x10, 0x90, 0x60, 0x00, 0x00,
	0x4b,	 0x00, 0x88, 0x90, 0xa0, 0xc0, 0xa0, 0x90, 0x88, 0x00, 0x00,
	0x4c,	 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf8, 0x00, 0x00,
	0x4d,	 0x00, 0x88, 0xd8, 0xa8, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00,
	0x4e,	 0x00, 0x88, 0x88, 0xc8, 0xa8, 0x98, 0x88, 0x88, 0x00, 0x00,
	0x4f,	 0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, 0x00,
	0x50,	 0x00, 0xf0, 0x88, 0x88, 0xf0, 0x80, 0x80, 0x80, 0x00, 0x00,
	0x51,	 0x00, 0x70, 0x88, 0x88, 0x88, 0xa8, 0x90, 0x68, 0x00, 0x00,
	0x52,	 0x00, 0xf0, 0x88, 0x88, 0xf0, 0xa0, 0x90, 0x88, 0x00, 0x00,
	0x53,	 0x00, 0x70, 0x88, 0x80, 0x70, 0x08, 0x88, 0x70, 0x00, 0x00,
	0x54,	 0x00, 0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
	0x55,	 0x00, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, 0x00,
	0x56,	 0x00, 0x88, 0x88, 0x88, 0x88, 0x50, 0x50, 0x20, 0x00, 0x00,
	0x57,	 0x00, 0x88, 0x88, 0x88, 0x88, 0xa8, 0xa8, 0x50, 0x00, 0x00,
	0x58,	 0x00, 0x88, 0x88, 0x50, 0x20, 0x50, 0x88, 0x88, 0x00, 0x00,
	0x59,	 0x00, 0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
	0x5a,	 0x00, 0xf8, 0x08, 0x10, 0x20, 0x40, 0x80, 0xf8, 0x00, 0x00,
	0x5b,	 0x70, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x70, 0x00,
	0x5c,	 0x00, 0x80, 0x40, 0x40, 0x20, 0x10, 0x10, 0x08, 0x00, 0x00,
	0x5d,	 0x70, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x70, 0x00,
	0x5e,	 0x20, 0x50, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5f,	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8,
	0x60,	 0x40, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x61,	 0x00, 0x00, 0x00, 0x70, 0x08, 0x78, 0x88, 0x78, 0x00, 0x00,
	0x62,	 0x00, 0x80, 0x80, 0xf0, 0x88, 0x88, 0x88, 0xf0, 0x00, 0x00,
	0x63,	 0x00, 0x00, 0x00, 0x70, 0x88, 0x80, 0x88, 0x70, 0x00, 0x00,
	0x64,	 0x00, 0x08, 0x08, 0x78, 0x88, 0x88, 0x88, 0x78, 0x00, 0x00,
	0x65,	 0x00, 0x00, 0x00, 0x70, 0x88, 0xf0, 0x80, 0x70, 0x00, 0x00,
	0x66,	 0x00, 0x30, 0x48, 0x40, 0xe0, 0x40, 0x40, 0x40, 0x00, 0x00,
	0x67,	 0x00, 0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x78, 0x08, 0x70,
	0x68,	 0x00, 0x80, 0x80, 0xf0, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00,
	0x69,	 0x00, 0x20, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00,
	0x6a,	 0x00, 0x08, 0x00, 0x18, 0x08, 0x08, 0x08, 0x08, 0x48, 0x30,
	0x6b,	 0x00, 0x80, 0x80, 0x88, 0x90, 0xe0, 0x90, 0x88, 0x00, 0x00,
	0x6c,	 0x00, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00,
	0x6d,	 0x00, 0x00, 0x00, 0xd0, 0xa8, 0xa8, 0xa8, 0xa8, 0x00, 0x00,
	0x6e,	 0x00, 0x00, 0x00, 0xb0, 0xc8, 0x88, 0x88, 0x88, 0x00, 0x00,
	0x6f,	 0x00, 0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x70, 0x00, 0x00,
	0x70,	 0x00, 0x00, 0x00, 0xf0, 0x88, 0x88, 0x88, 0xf0, 0x80, 0x80,
	0x71,	 0x00, 0x00, 0x00, 0x78, 0x88, 0x88, 0x88, 0x78, 0x08, 0x08,
	0x72,	 0x00, 0x00, 0x00, 0xb0, 0xc8, 0x80, 0x80, 0x80, 0x00, 0x00,
	0x73,	 0x00, 0x00, 0x00, 0x78, 0x80, 0x70, 0x08, 0xf0, 0x00, 0x00,
	0x74,	 0x00, 0x20, 0x20, 0xf8, 0x20, 0x20, 0x20, 0x18, 0x00, 0x00,
	0x75,	 0x00, 0x00, 0x00, 0x88, 0x88, 0x88, 0x98, 0x68, 0x00, 0x00,
	0x76,	 0x00, 0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20, 0x00, 0x00,
	0x77,	 0x00, 0x00, 0x00, 0x88, 0x88, 0xa8, 0xa8, 0x50, 0x00, 0x00,
	0x78,	 0x00, 0x00, 0x00, 0x88, 0x50, 0x20, 0x50, 0x88, 0x00, 0x00,
	0x79,	 0x00, 0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20, 0x40, 0x80,
	0x7a,	 0x00, 0x00, 0x00, 0xf8, 0x10, 0x20, 0x40, 0xf8, 0x00, 0x00,
	0x7b,	 0x10, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20, 0x20, 0x10, 0x00,
	0x7c,	 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x7d,	 0x40, 0x20, 0x20, 0x20, 0x10, 0x20, 0x20, 0x20, 0x40, 0x00,
	0x7e,	 0x00, 0x00, 0x00, 0x48, 0xa8, 0x90, 0x00, 0x00, 0x00, 0x00
#endif
};

#undef	I

chargen_t chargen[] = {
	{6, 10, chargen_6x10, sizeof(chargen_6x10)}
};

/**
 * @brief flush pending updates in the textmap to the bitmap
 */
void dbg_flush_textmap(void)
{
	size_t offs;
	int x, y, x0, y0;

	/* adjust to useable portion of debug surface */
	x0 = (DISPLAY_WIDTH - DBG_TEXTMAP_W*DBG_FONT_W) / 2;
	y0 = (DISPLAY_HEIGHT - DBG_TEXTMAP_H*DBG_FONT_H) / 2;

	for (y = 0, offs = 0; y < DBG_TEXTMAP_H; y++, offs += DBG_TEXTMAP_W) {
		/* skip unchanged rows */
		if (0 == memcmp(&dbg.backmap[offs], &dbg.textmap[offs], DBG_TEXTMAP_W * sizeof(int)))
			continue;
		for (x = 0; x < DBG_TEXTMAP_W; x++, offs++) {
			int ch = dbg.textmap[offs];
			/* skip over unchanged character / color */
			if (ch == dbg.backmap[offs])
				continue;
			sdl_debug(x0 + x * DBG_FONT_W, y0 + y * DBG_FONT_H, ch & 255, ch / 256);
			/* set new code in the backup map */
			dbg.backmap[offs] = ch;
		}
		offs -= DBG_TEXTMAP_W;
	}
#if	DEBUG
	if (dbg.visible)
		sdl_update(1);
#endif
}

/**
 * @brief put a character into the debugger bitmap at x, y
 */
static void dbg_putch_xy(char ch, int x, int y)
{
	size_t offs = y * DBG_TEXTMAP_W + x;
	if (x < 0 || y < 0 || x >= DBG_TEXTMAP_W || y >= DBG_TEXTMAP_H)
		return;
	dbg.textmap[offs] = ch + 256 * dbg.color;
}

/**
 * @brief scroll the window textmap range up by one line
 */
static void dbg_scroll_up(int x0, int y0, int w, int h)
{
	int x, y;

	for (y = DBG_WIN_Y; y < DBG_WIN_Y + DBG_WIN_H - 1; y++)
		memmove(&dbg.textmap[y * DBG_TEXTMAP_W + DBG_WIN_X],
			&dbg.textmap[(y+1) * DBG_TEXTMAP_W + DBG_WIN_X], w * sizeof(int));
	for (x = DBG_WIN_X; x < DBG_WIN_X + DBG_WIN_W; x++)
		dbg_putch_xy(' ', x, DBG_WIN_Y + DBG_WIN_H - 1);
}

/**
 * @brief print a character to a scrollable range, very limited ctrl-codes
 */
static int dbg_putch(char ch)
{
	int width = 0;

	switch (ch) {
	case '\n':
		dbg.cx = 0;
		if (++dbg.cy == DBG_WIN_H) {
			dbg_scroll_up(DBG_WIN_X, DBG_WIN_Y, DBG_WIN_W, DBG_WIN_H);
			dbg.cy = DBG_WIN_H - 1;
		}
		break;
	case '\t':
		do {
			dbg_putch(' ');
			width++;
		} while (dbg.cx & 7);
		break;
	case '\020': case '\021': case '\022': case '\023':
	case '\024': case '\025': case '\026': case '\027':
		dbg.color = ch & 7;
		break;
	default:
		dbg_putch_xy(ch, DBG_WIN_X + dbg.cx, DBG_WIN_Y + dbg.cy);
		width++;
		if (++dbg.cx == DBG_WIN_W) {
			dbg.cx = 0;
			if (++dbg.cy == DBG_WIN_H) {
				dbg_scroll_up(DBG_WIN_X, DBG_WIN_Y, DBG_WIN_W, DBG_WIN_H);
				dbg.cy = DBG_WIN_H - 1;
			}
		}
		break;
	}
	return width;
}

/**
 * @brief print a formatted string and arguments to the scrollable range
 */
static int dbg_vprintf(const char *fmt, va_list ap)
{
	static char temp[1024];
	int i, size, width, color;

	color = dbg.color;
	size = vsnprintf(temp, sizeof(temp), fmt, ap);
	for (i = 0, width = 0; temp[i]; i++)
		width += dbg_putch(temp[i]);
	dbg.color = color;
	return width;
}

/**
 * @brief print a formatted string to the scrollable range
 */
int dbg_printf(const char *fmt, ...)
{
	va_list ap;
	int width;

	va_start(ap, fmt);
	width = dbg_vprintf(fmt, ap);
	va_end(ap);

	return width;
}

/**
 * @brief print a formatted string into the debugger bitmap at x, y
 */
static int dbg_printf_xy(int x, int y, const char *fmt, ...)
{
	static char temp[1024];
	va_list ap;
	int x0 = x;
	int i, size, width, color;

	color = dbg.color;
	va_start(ap, fmt);
	size = vsnprintf(temp, sizeof(temp), fmt, ap);
	va_end(ap);
	for (i = 0, width = 0; i < size && x < DBG_TEXTMAP_W; i++) {
		switch (temp[i]) {
		case '\t':
			do {
				dbg_putch_xy(' ', x++, y);
				width++;
			} while (x & 7);
			break;
		case '\r':
			x = x0;
			break;
		case '\n':
			x = x0;
			y++;
			break;
		case '\020': case '\021': case '\022': case '\023':
		case '\024': case '\025': case '\026': case '\027':
			dbg.color = temp[i] & 7;
			break;
		default:
			dbg_putch_xy(temp[i], x++, y);
			width++;
		}
	}
	dbg.color = color;
	return width;
}

/**
 * @brief clear the screen to all blanks
 */
static void dbg_clear_screen(void)
{
	int x, y;

	for (y = 0; y < DBG_TEXTMAP_H; y++)
		for (x = 0; x < DBG_TEXTMAP_W; x++)
			dbg_putch_xy(' ', x, y);
	dbg_flush_textmap();
}

/**
 * @brief debug key event, called by sdl_update
 */
void dbg_key(void *sym, int down)
{
	SDL_keysym *keysym = (SDL_keysym *)sym;
	static int ctrl;
	int data;

	if (keysym->sym == SDLK_LCTRL || keysym->sym == SDLK_RCTRL)
		ctrl = down;
	if (!down)
		return;

	switch (keysym->sym) {
	case SDLK_LEFT:
		if (dbg.memory) {
			if (--dbg.memoffs < 0) {
				dbg.memoffs += 8;
				dbg.memaddr = (dbg.memaddr - 8) & (RAM_SIZE - 1);
			}
		} else {
			if (--dbg.secoffs < 0) {
				dbg.secoffs += 8;
				dbg.secaddr -= 8;
				if (dbg.secaddr < 0)
					dbg.secaddr = 0;
			}
		}
		break;

	case SDLK_RIGHT:
		if (dbg.memory) {
			if (++dbg.memoffs >= DBG_DUMP_WORDS) {
				dbg.memoffs -= 8;
				dbg.memaddr = (dbg.memaddr + 8) & (RAM_SIZE - 1);
			}
		} else {
			if (++dbg.secoffs >= DBG_DUMP_WORDS) {
				dbg.secoffs -= 8;
				dbg.secaddr += 8;
				if (dbg.secaddr >= 400 - DBG_DUMP_WORDS)
					dbg.secaddr = 400 - DBG_DUMP_WORDS - 8;
			}
		}
		break;

	case SDLK_UP:
		if (dbg.memory) {
			if ((dbg.memoffs -= 8) < 0) {
				dbg.memoffs += 8;
				dbg.memaddr = (dbg.memaddr - 8) & (RAM_SIZE - 1);
			}
		} else {
			if ((dbg.secoffs -= 8) < 0) {
				dbg.secoffs += 8;
				dbg.secaddr -= 8;
				if (dbg.secaddr < 0)
					dbg.secaddr = 0;
			}
		}
		break;

	case SDLK_DOWN:
		if (dbg.memory) {
			if ((dbg.memoffs += 8) >= DBG_DUMP_WORDS) {
				dbg.memoffs -= 8;
				dbg.memaddr = (dbg.memaddr + 8) & (RAM_SIZE - 1);
			}
		} else {
			if ((dbg.secoffs += 8) >= DBG_DUMP_WORDS) {
				dbg.secoffs -= 8;
				dbg.secaddr += 8;
				if (dbg.secaddr >= 400 - DBG_DUMP_WORDS)
					dbg.secaddr = 400 - DBG_DUMP_WORDS - 8;
			}
		}
		break;

	case SDLK_PAGEUP:
		if (dbg.memory) {
			if (ctrl)
				dbg.memaddr = (dbg.memaddr - 16 * DBG_DUMP_WORDS) & (RAM_SIZE - 1);
			else
				dbg.memaddr = (dbg.memaddr - DBG_DUMP_WORDS) & (RAM_SIZE - 1);
		} else {
			if (ctrl) {
				dbg.page -= 1;
				if (dbg.page < 0)
					dbg.page = 0;
			} else {
				dbg.secaddr = dbg.secaddr - DBG_DUMP_WORDS;
				if (dbg.secaddr < 0) {
					dbg.page -= 1;
					if (dbg.page < 0) {
						dbg.secaddr = dbg.secaddr + DBG_DUMP_WORDS;
						dbg.page = 0;
					} else {
						dbg.secaddr = 400 - DBG_DUMP_WORDS;
					}
				}
			}
		}
		break;

	case SDLK_PAGEDOWN:
		if (dbg.memory) {
			if (ctrl)
				dbg.memaddr = (dbg.memaddr + 16 * DBG_DUMP_WORDS) & (RAM_SIZE - 1);
			else
				dbg.memaddr = (dbg.memaddr + DBG_DUMP_WORDS) & (RAM_SIZE - 1);
		} else {
			if (ctrl) {
				dbg.page += 1;
				if (dbg.page >= DRIVE_CYLINDERS*DRIVE_HEADS*DRIVE_SPT)
					dbg.page = DRIVE_CYLINDERS*DRIVE_HEADS*DRIVE_SPT - 1;
			} else {
				dbg.secaddr = dbg.secaddr + DBG_DUMP_WORDS;
				if (dbg.secaddr >= 400 - DBG_DUMP_WORDS) {
					dbg.page += 1;
					if (dbg.page >= DRIVE_CYLINDERS*DRIVE_HEADS*DRIVE_SPT) {
						dbg.secaddr = dbg.secaddr - DBG_DUMP_WORDS;
						dbg.page = DRIVE_CYLINDERS*DRIVE_HEADS*DRIVE_SPT - 1;
					} else {
						dbg.secaddr = 0;
					}
				}
			}
		}
		break;

	case SDLK_HOME:
		if (dbg.memory) {
			if (ctrl)
				/* home to page 0 */
				dbg.memaddr = 0;
			else
				/* cursor home to top of page */
				dbg.memoffs = 0;
		} else {
			if (ctrl) {
				/* first sector */
				dbg.page = 0;
			} else {
				/* cursor to start of sector */
				dbg.secaddr = 0;
				dbg.secoffs = 0;
			}
		}
		break;

	case SDLK_END:
		if (dbg.memory) {
			if (ctrl)
				/* last mmio page */
				dbg.memaddr = RAM_SIZE - DBG_DUMP_WORDS;
			else
				/* cursor to bottom of page */
				dbg.memoffs = DBG_DUMP_WORDS-1;
		} else {
			if (ctrl) {
				/* last sector */
				dbg.page = DRIVE_CYLINDERS*DRIVE_HEADS*DRIVE_SPT - 1;
			} else {
				/* cursor to bottom of sector */
				dbg.secaddr = 400 - DBG_DUMP_WORDS;
				dbg.secoffs = DBG_DUMP_WORDS - 1;
			}
		}
		break;

	case SDLK_0:
		if (ctrl)
			/* switch to bank 0 */
			dbg.memaddr = dbg.memaddr & 0177777;
		break;

	case SDLK_1:
		if (ctrl)
			/* switch to bank 1 */
			dbg.memaddr = ((dbg.memaddr & 0177777) | 0200000) % RAM_SIZE;
		break;

	case SDLK_2:
		if (ctrl)
			/* switch to bank 2 */
			dbg.memaddr = ((dbg.memaddr & 0177777) | 0400000) % RAM_SIZE;
		break;

	case SDLK_3:
		if (ctrl)
			/* switch to bank 3 */
			dbg.memaddr = ((dbg.memaddr & 0177777) | 0600000) % RAM_SIZE;
		break;

	case SDLK_a:
		/* toggle between ASCII and memory check display */
		dbg.ascii ^= 1;
		break;

	case SDLK_m:
		/* toggle between memory and drive sector display */
		dbg.memory ^= 1;
		break;

	case SDLK_o:
		/* octal mode */
		if (dbg.base != base_OCT) {
			dbg_clear_screen();
			dbg.base = base_OCT;
		}
		break;

	case SDLK_d:
		/* decimal mode */
		if (dbg.base != base_DEC) {
			dbg_clear_screen();
			dbg.base = base_DEC;
		}
		break;

	case SDLK_h:
		/* hexadecimal mode */
		if (dbg.base != base_HEX) {
			dbg_clear_screen();
			dbg.base = base_HEX;
		}
		break;

	case SDLK_SPACE:
		if (dbg.memory) {
			/* follow pointer */
			data = debug_read_mem((dbg.memaddr + dbg.memoffs) % RAM_SIZE);
			/* stack current address */
			if (dbg.stackptr < DBG_STACK_SIZE) {
				dbg.retaddr[dbg.stackptr] = dbg.memaddr + dbg.memoffs;
				dbg.stackptr++;
			}
			dbg.memoffs = data % DBG_DUMP_WORDS;
			dbg.memaddr = data ^ dbg.memoffs;
		} else {
			/* go to current page of the drive */
			dbg.page = drive_page(dbg.drive);
		}
		break;

	case SDLK_BACKSPACE:
		if (dbg.memory) {
			/* return to stacked address */
			if (dbg.stackptr == 0)
				break;
			data = dbg.retaddr[--dbg.stackptr];
			dbg.memoffs = data % DBG_DUMP_WORDS;
			dbg.memaddr = data ^ dbg.memoffs;
		}
		break;

#if	DEBUG
	case SDLK_KP_MINUS:
		/* keypad minus: decreas loglevel for current task */
		if (ll[cpu.task].level > 0)
			ll[cpu.task].level -= 1;
		break;

	case SDLK_KP_PLUS:
		/* keypad plus: increase loglevel for current task */
		if (ll[cpu.task].level < 9)
			ll[cpu.task].level += 1;
		break;
#endif
	default:
		/* shut GCC */
		break;
	}
}

/** @brief format string for the R and S register file */
static const char *rs_format[base_COUNT] = {
	"r%02o:\023%06o\020 s%02o:\023%06o",
	"r%02x:\023%04x\020 s%02x:\023%04x",
	"r%02d:\023%05d\020 s%02d:\023%05d"
};

/** @brief format strings for the CPU status */
static const char *cpu_format[9][base_COUNT] = {
	{"    bus:\023%06o",
	 "    bus:\023%04x",
	 "    bus:\023%05d"},
	{"      t:\023%06o",
	 "      t:%04x",
	 "      t:%05d"},
	{"    alu:\023%06o\020 cy:\023%o",
	 "    alu:%04x cy:%x",
	 "    alu:%05d cy:%d"},
	{"      l:\023%06o\020 cy:\023%o",
	 "      l:\023%04x\020 cy:\023%x",
	 "      l:\023%05d\020 cy:\023%d"},
	{"shifter:\023%06o",
	 "shifter:\023%04x",
	 "shifter:\023%05d"},
	{"      m:\023%06o",
	 "      m:\023%04x",
	 "      m:\023%05d"},
	{"    mar:\023%06o",
	 "    mar:\023%04x",
	 "    mar:\023%05d"},
	{"     md:\023%06o",
	 "     md:\023%04x",
	 "     md:\023%05d"},
	{"   cram:\023%06o",
	 "   cram:\023%04x",
	 "   cram:\023%05d"}
};

/** @brief format strings for the emulator status */
static const char *emu_format[7][base_COUNT] = {
	{"   skip:\023%o",
	 "   skip:\023%x",
	 "   skip:\023%d"},
	{"     cy:\023%o",
	 "     cy:\023%x",
	 "     cy:\023%d"},
	{"    ac0:\023%06o",
	 "    ac0:\023%04x",
	 "    ac0:\023%05d"},
	{"    ac1:\023%06o",
	 "    ac1:\023%04x",
	 "    ac1:\023%05d"},
	{"    ac2:\023%06o",
	 "    ac2:\023%04x",
	 "    ac2:\023%05d"},
	{"    ac3:\023%06o",
	 "    ac3:\023%04x",
	 "    ac3:\023%05d"},
	{"     ir:\023%06o \027%s",
	 "     ir:\023%04x \027%s",
	 "     ir:\023%05d \027%s"}
};

/** @brief format strings for the disk controller status */
static const char *dsk_format[11][base_COUNT] = {
	{"address:\023%06o",
	 "address:\023%04x",
	 "address:\023%05d"},
	{"   kadr:\023%06o",
	 "   kadr:\023%04x",
	 "   kadr:\023%05dd"},
	{"  kstat:\023%06o",
	 "  kstat:\023%04x",
	 "  kstat:\023%05d"},
	{"   kcom:\023%06o",
	 "   kcom:\023%04x",
	 "   kcom:\023%05d"},
	{"  recno:\023%o",
	 "  recno:\023%x",
	 "  recno:\023%d"},
	{"    rwc:\023%o",
	 "    rwc:\023%x",
	 "    rwc:\023%d"},
	{" bitcnt:\023%02o\020 cy:\023%o",
	 " bitcnt:\023%02x\020 cy:\023%x",
	 " bitcnt:\023%02d\020 cy:\023%x"},
	{"<-shift:\023%06o",
	 "<-shift:\023%04x",
	 "<-shift:\023%05d"},
	{" <-data:\023%06o",
	 " <-data:\023%04x",
	 " <-data:\023%05d"},
	{" data->:\023%06o",
	 " data->:\023%04x",
	 " data->:\023%05d"},
	{"shift->:\023%06o",
	 "shift->:\023%04x",
	 "shift->:\023%05d"}
};

/** @brief format string for memory address */
static const char *mema_format[base_COUNT] = {
	"%07o:","%05x:","%06d:"
};

/** @brief witdh of memory address including colon and blank */
static const int mema_width[base_COUNT] = {
	8,	6,	7
};

/** @brief format string for memory data */
static const char *memd_format[base_COUNT] = {
	"%06o",	"%04x",	"%05d"
};

/** @brief witdh of memory data including 2 blanks */
static const int memd_width[base_COUNT] = {
	8,	6,	7
};


/** @brief structure to describe a bit field */
typedef struct {
	/** @brief starting bit number (leftmost is 0) */
	uint8_t from;
	/** @brief ending bit number (leftmost is 0) */
	uint8_t to;
	/** @brief CR + LF after field */
	uint8_t crlf;
	/** @brief name of the bit field */
	const char *name;
}	bits_info_t;

/** @brief structure to describe memory address */
typedef struct {
	/** @brief address mask */
	uint32_t mask;
	/** @brief lowest address for this ranged */
	uint32_t from;
	/** @brief highest address for this ranged (= from if just one word) */
	uint32_t to;
	/** @brief optional pointer to a bit info */
	const bits_info_t *bits;
	/** @brief name of this memory address or region */
	const char *name;
	/** @brief comment string */
	const char *comment;
}	mema_info_t;

bits_info_t ww_bits[] = {
	{ 1, 1, 0, "int17"},
	{ 2, 2, 0, "int16"},
	{ 3, 3, 1, "int15"},
	{ 4, 4, 0, "int14"},
	{ 5, 5, 0, "int13"},
	{ 6, 6, 1, "int12"},
	{ 7, 7, 0, "int11"},
	{ 8, 8, 0, "int10"},
	{ 9, 9, 1, "int07"},
	{10,10, 0, "int06"},
	{11,11, 0, "int05"},
	{12,12, 1, "int04"},
	{13,13, 0, "int03"},
	{14,14, 0, "int02"},
	{15,15, 1, "int01"},
	{ 0, 0, 0, NULL}
};

/** @brief bits info for KSTAT */
bits_info_t kstat_bits[] = {
	{ 0, 3, 1, "sector"},
	{ 4, 7, 1, "done"},
	{ 8, 8, 1, "seek failed"},
	{ 9, 9, 1, "seek in progress"},
	{10,10, 1, "disk unit not ready"},
	{11,11, 1, "sector data late"},
	{12,12, 1, "idle"},
	{13,13, 1, "checksum error"},
	{14,15, 1, "completion"},
	{ 0, 0, 0, NULL}
};

/** @brief bits info for KADDR */
bits_info_t kaddr_bits[] = {
	{ 0, 3, 1, "sector"},
	{ 4,12, 1, "cylinder"},
	{13,13, 1, "head"},
	{14,14, 1, "drive"},
	{15,15, 1, "restore"},
	{ 0, 0, 0, NULL}
};

/** @brief EIA status bits or data */
bits_info_t eia_bits[] = {
	{ 0, 0, 1, "D: disconnect"},
	{ 1, 1, 1, "CD: carrier detect"},
	{ 2, 2, 1, "RI: ring indicator"},
	{ 3, 3, 1, "SI: send indicator"},
	{ 4, 6, 1, "line"},
	{ 7, 7, 1, "SRD: remote break"},
	{ 8, 8, 1, "RDA: receive data available"},
	{ 9, 9, 1, "TBMT: transmit buffer empty"},
	{10,10, 1, "SCR: sync char received"},
	{11,11, 1, "FCT: fill char transmitted"},
	{12,12, 1, "RPE: receive parity error"},
	{13,13, 1, "ROR: receive overflow error"},
	{14,14, 1, "RFE: receive framing error"},
	{15,15, 1, "INTP: interrupt"},
	{ 0, 0, 0, NULL}
};

/** @brief bits info for MESR */
bits_info_t mesr_bits[] = {
	{ 0, 5, 1, "hamming code"},
	{ 6, 6, 1, "parity error"},
	{ 7, 7, 1, "memory parity"},
	{ 8,13, 1, "syndrome bits"},
	{14,15, 1, "bank"},
	{ 0, 0, 0, NULL}
};

/** @brief bits info for MECR */
bits_info_t mecr_bits[] = {
	{ 4,10, 1, "test hamming code"},
	{11,11, 1, "test mode"},
	{12,12, 1, "cause int on single-bit errors"},
	{13,13, 1, "cause int on double-bit errors"},
	{14,14, 1, "do not use error correction"},
	{ 0, 0, 0, NULL}
};

/** @brief bits info the per task bank registers */
bits_info_t bank_bits[] = {
	{12,13, 1, "normal RAM bank"},
	{14,15, 1, "extended RAM bank"},
	{ 0, 0, 0, NULL}
};

/** @brief bits info the per task bank registers */
bits_info_t utilin_bits[] = {
	{ 0, 0, 0, "paper ready"},
	{ 1, 1, 1, "prt check"},
	{ 2, 2, 1, "unused2"},
	{ 3, 3, 0, "daisy ready"},
	{ 4, 4, 0, "carriage ready"},
	{ 5, 5, 1, "prt ready"},
	{ 6, 6, 1, "memory configuration"},
	{ 7, 7, 1, "unused7"},
	{ 8,12, 1, "keyset[0-4]"},
	{13,13, 0, "mouse red"},
	{14,14, 0, "mouse blue"},
	{15,15, 1, "mouse yellow"},
	{ 0, 0, 0, NULL}
};

/** @brief well known memory addresses info */
mema_info_t mema_info[] = {
{0777777,0000402,0000411,NULL,		"BOOTLABEL",	"Label block on disk boot"},
{0777777,0000420,0000420,NULL,		"DASTART",	"Display list header"},
{0777777,0000421,0000421,ww_bits,	"VBINT",	"Vertical field interrupt bitword"},
{0777777,0000422,0000422,NULL,		"ITQUAN",	"Interval timer stored quantity"},
{0777777,0000423,0000423,NULL,		"ITBITS",	"Interval timer bitword"},
{0777777,0000424,0000424,NULL,		"MOUSEX",	"Mouse x coordinate"},
{0777777,0000425,0000425,NULL,		"MOUSEY",	"Mouse y coordinate"},
{0777777,0000426,0000426,NULL,		"CURSORX",	"Curxor x coordinate"},
{0777777,0000427,0000427,NULL,		"CURSORY",	"Curxor y coordinate"},
{0777777,0000430,0000430,NULL,		"RTC",		"Real time clock"},
{0777777,0000431,0000450,NULL,		"CURMAP",	"Cursor bitmap (16 words)"},
{0777777,0000452,0000452,ww_bits,	"WW",		"Interrupt wakeups waiting"},
{0777777,0000453,0000453,ww_bits,	"ACTIVE",	"Active interrupt bitword"},
{0777777,0000460,0000477,NULL,		"MASKTAB",	"Mask table for CONVERT (16 words)"},
{0777777,0000500,0000500,NULL,		"PCLOC",	"Saved interrupt PC"},
{0777777,0000501,0000517,NULL,		"INTVEC",	"Interrupt transfer vector (15 words)"},
{0777777,0000521,0000521,NULL,		"KBLKADR",	"Disk command block address"},
{0777777,0000522,0000522,kstat_bits,	"KSTAT",	"Disk status at start of current sector"},
{0777777,0000523,0000523,kaddr_bits,	"KADDR",	"Disk address of latest disk command"},
{0777777,0000524,0000524,NULL,		"KSECINT",	"Sector interrupt bitmask"},
{0777777,0000525,0000525,NULL,		"ITTIME",	"Interval timer time"},
{0777777,0000526,0000526,NULL,		"TRAPDISP",	"Trap displacement"},
{0777777,0000527,0000527,NULL,		"TRAPPC",	"Trap saved PC"},
{0777777,0000530,0000530,NULL,		"TRAPVEC",	"Unused; CYCLE (60000-60377)"},
{0777777,0000531,0000531,NULL,		"TRAPVEC+1",	"RAM trap (60400-60777)"},
{0777777,0000532,0000532,NULL,		"TRAPVEC+2",	"Unused; opcodes 61000-61026, ROM trap 61000-61377"},
{0777777,0000533,0000533,NULL,		"TRAPVEC+3",	"RAM trap (61400- 61777)"},
{0777777,0000534,0000534,NULL,		"TRAPVEC+4",	"RAM trap (62000- 62377)"},
{0777777,0000535,0000535,NULL,		"TRAPVEC+5",	"RAM trap (62400- 62777)"},
{0777777,0000536,0000536,NULL,		"TRAPVEC+6",	"RAM trap (63000- 63377)"},
{0777777,0000537,0000537,NULL,		"TRAPVEC+7",	"RAM trap (63400- 63777)"},
{0777777,0000540,0000540,NULL,		"TRAPVEC+8",	"RAM trap (64000-64377)"},
{0777777,0000541,0000541,NULL,		"TRAPVEC+9",	"Unused; JSRII (64400-64777)"},
{0777777,0000542,0000542,NULL,		"TRAPVEC+10",	"Unused; JSRIS (65000-65377)"},
{0777777,0000543,0000543,NULL,		"TRAPVEC+11",	"RAM trap (65400-65777)"},
{0777777,0000544,0000544,NULL,		"TRAPVEC+12",	"RAM trap (66000-66377)"},
{0777777,0000545,0000545,NULL,		"TRAPVEC+13",	"RAM trap (66400-66777)"},
{0777777,0000546,0000546,NULL,		"TRAPVEC+14",	"Unused; CONVERT (67000-67377"},
{0777777,0000547,0000547,NULL,		"TRAPVEC+15",	"RAM trap (67400-67777)"},
{0777777,0000550,0000550,NULL,		"TRAPVEC+16",	"RAM trap (70000-70377)"},
{0777777,0000551,0000551,NULL,		"TRAPVEC+17",	"RAM trap (70400-70777)"},
{0777777,0000552,0000552,NULL,		"TRAPVEC+18",	"RAM trap (71000-71377)"},
{0777777,0000553,0000553,NULL,		"TRAPVEC+19",	"RAM trap (71400-71777)"},
{0777777,0000554,0000554,NULL,		"TRAPVEC+20",	"RAM trap (72000-72377)"},
{0777777,0000555,0000555,NULL,		"TRAPVEC+21",	"RAM trap (72400-72777)"},
{0777777,0000556,0000556,NULL,		"TRAPVEC+22",	"RAM trap (73000-73377)"},
{0777777,0000557,0000557,NULL,		"TRAPVEC+23",	"RAM trap (73400-73777)"},
{0777777,0000560,0000560,NULL,		"TRAPVEC+24",	"RAM trap (74000-74377)"},
{0777777,0000561,0000561,NULL,		"TRAPVEC+25",	"RAM trap (74400-74777)"},
{0777777,0000562,0000562,NULL,		"TRAPVEC+26",	"RAM trap (75000-75377)"},
{0777777,0000563,0000563,NULL,		"TRAPVEC+27",	"RAM trap (75400-75777)"},
{0777777,0000564,0000564,NULL,		"TRAPVEC+28",	"RAM trap (76000-76377)"},
{0777777,0000565,0000565,NULL,		"TRAPVEC+29",	"RAM trap (76400-76777)"},
{0777777,0000566,0000566,NULL,		"TRAPVEC+30",	"RAM trap (77000-77377)"},
{0777777,0000567,0000567,NULL,		"TRAPVEC+31",	"RAM trap (77400-77777)"},
{0777777,0000570,0000577,NULL,		"TIMER",	"Timer data (8 words)"},
{0777777,0000600,0000600,NULL,		"EPLOC",	"Ethernet post location"},
{0777777,0000601,0000601,NULL,		"EBLOC",	"Ethernet interrupt bitmask"},
{0777777,0000602,0000602,NULL,		"EELOC",	"Ethernet ending count"},
{0777777,0000603,0000603,NULL,		"ELLOC",	"Ethernet load location"},
{0777777,0000604,0000604,NULL,		"EICLOC",	"Ethernet input buffer count"},
{0777777,0000605,0000605,NULL,		"EIPLOC",	"Ethernet input buffer pointer"},
{0777777,0000606,0000606,NULL,		"EOCLOC",	"Ethernet output buffer count"},
{0777777,0000607,0000607,NULL,		"EOPLOC",	"Ethernet output buffer pointer"},
{0777777,0000610,0000610,NULL,		"EHLOC",	"Ethernet host address"},
{0777777,0000611,0000612,NULL,		"",		"Reserved for Ethernet expansion"},
{0777777,0000613,0000613,NULL,		"",		"Alto I/II indication (0 = Alto I, -1 = Alto II)"},
{0777777,0000614,0000614,NULL,		"DCBR",		"Posted by parity task: main mem parity error"},
{0777777,0000615,0000615,NULL,		"KNMAR",	"Posted by parity task: main mem parity error"},
{0777777,0000616,0000616,NULL,		"DWA",		"Posted by parity task: main mem parity error"},
{0777777,0000617,0000617,NULL,		"CBA",		"Posted by parity task: main mem parity error"},
{0777777,0000620,0000620,NULL,		"PC",		"Posted by parity task: main mem parity error"},
{0777777,0000621,0000621,NULL,		"SAD",		"Posted by parity task: main mem parity error"},
{0777777,0000700,0000707,NULL,		"SWATR",	"Saved registers (Swat)"},
{0177777,0177001,0177003,eia_bits,	"EIA",		"EIA serial communications interface"},
{0177777,0177016,0177016,NULL,		"UTILOUT",	"Utility output bus (Printer)"},
{0177777,0177020,0177023,NULL,		"XBUS",		"AltoII extra input bus"},
{0177777,0177024,0177024,NULL,		"MEAR",		"Memory error address register"},
{0177777,0177025,0177025,mesr_bits,	"MESR",		"Memory error status register"},
{0177777,0177026,0177026,mecr_bits,	"MECR",		"Memory error control register"},
{0177777,0177030,0177033,utilin_bits,	"UTILIN",	"Utility input bus (4 words same thing)"},
{0177777,0177034,0177037,NULL,		"KBDAD",	"Keyboard matrix"},
{0177777,0177740,0177740,bank_bits,	"BANKREGS",	"Bank register for emulator task"},
{0177777,0177741,0177741,bank_bits,	"BANKREGS+1",	"Bank register for task 01"},
{0177777,0177742,0177742,bank_bits,	"BANKREGS+2",	"Bank register for task 02"},
{0177777,0177743,0177743,bank_bits,	"BANKREGS+3",	"Bank register for task 03"},
{0177777,0177744,0177744,bank_bits,	"BANKREGS+4",	"Bank register for disk sector task"},
{0177777,0177745,0177745,bank_bits,	"BANKREGS+5",	"Bank register for task 05"},
{0177777,0177746,0177746,bank_bits,	"BANKREGS+6",	"Bank register for task 07"},
{0177777,0177747,0177747,bank_bits,	"BANKREGS+7",	"Bank register for Ethernet task"},
{0177777,0177750,0177750,bank_bits,	"BANKREGS+8",	"Bank register for memory refresh task"},
{0177777,0177751,0177751,bank_bits,	"BANKREGS+9",	"Bank register for display word task"},
{0177777,0177752,0177752,bank_bits,	"BANKREGS+10",	"Bank register for cursor task"},
{0177777,0177753,0177753,bank_bits,	"BANKREGS+11",	"Bank register for display horizontal task"},
{0177777,0177754,0177754,bank_bits,	"BANKREGS+12",	"Bank register for display vertical task"},
{0177777,0177755,0177755,bank_bits,	"BANKREGS+13",	"Bank register for parity error task"},
{0177777,0177756,0177756,bank_bits,	"BANKREGS+14",	"Bank register for disk word task"},
{0177777,0177757,0177757,bank_bits,	"BANKREGS+15",	"Bank register for task 17"},
{0177777,0177000,0177777,NULL,		"MMIO",		"memory mapped I/O range (unknown)"},
/* last entry */
{0,0,0,NULL,NULL,NULL},
};

/**
 * @brief clear any previous and print a new bits info, if non-NULL
 *
 * @param x leftmost text column where to print info
 * @param y topmost text row where to print info
 * @param data data word to dissect
 * @param b pointer to the bits_info_t structure to apply
 */
static void dbg_bits_info(int x, int y, int data, const bits_info_t *b)
{
	int i, w;

	for (i = 0; i < DBG_BITS_H; i++)
		dbg_printf_xy(x, y + i, "%-*s", DBG_BITS_W, "");
	if (NULL == b || NULL == b->name)
		return;

	w = 0;
	while (b->name != NULL) {
		switch (dbg.base) {
		case base_OCT:
			w += dbg_printf_xy(x + w, y, "\024%02d-%02d \020%s \023%#3o\t",
				b->from, b->to, b->name, ALTO_GET(data, 16, b->from, b->to));
			break;
		case base_HEX:
			w += dbg_printf_xy(x + w, y, "\024%02d-%02d \020%s \023%#2x\t",
				b->from, b->to, b->name, ALTO_GET(data, 16, b->from, b->to));
			break;
		case base_DEC:
		default:
			w += dbg_printf_xy(x + w, y, "\024%02d-%02d \020%s \023%d\t",
				b->from, b->to, b->name, ALTO_GET(data, 16, b->from, b->to));
			break;
		}
		if (b->crlf) {
			y++;
			/* start at x + 0 again (i.e. carriage return) */
			w = 0;
		} else {
			/* next modulo 16 position */
			w = (w | 15) + 1;
		}
		b++;
	}
}

#define	emit(str) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str)
#define	emit1(str,arg) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str, arg)
#define	emit2(str,arg1,arg2) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str, arg1, arg2)
#define	emit3(str,arg1,arg2,arg3) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str, arg1, arg2, arg3)
#define	emit4(str,arg1,arg2,arg3,arg4) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str, arg1, arg2, arg3, arg4)

/**
 * @brief print a page0 displacement
 *
 * @param com pointer to a buffer to receive a comment
 * @param disp displacement value
 * @result returns a pointer to a string for disp in current base
 */
static const char *page0(char *com, int disp)
{
	static char buff[16];
	switch (dbg.base) {
	case base_OCT:
		snprintf(buff, sizeof(buff), "%#o", disp & 0377);
		if (com)
			snprintf(com, 32, ";=%#o", disp);
		break;
	case base_HEX:
		snprintf(buff, sizeof(buff), "%#x", disp & 0377);
		if (com)
			snprintf(com, 32, ";=%#x", disp);
		break;
	case base_DEC:
	default:
		snprintf(buff, sizeof(buff), "%d", disp & 0377);
		if (com)
			snprintf(com, 32, ";=%d", disp);
		break;
	}
	return buff;
}

/**
 * @brief print a PC relative displacement
 *
 * @param com pointer to a buffer to receive a comment
 * @param pc current program counter
 * @param disp displacement value
 * @result returns a pointer to a string for disp in current base
 */
static const char *pcrel(char *com, int pc, int disp)
{
	static char buff[32];
	char sign = (disp & 0200) ? '-' : '+';
	int udisp = (disp & 0200) ? 0200 - (disp & 0177) : disp;
	int ea = (pc + disp) & 0177777;

	switch (dbg.base) {
	case base_OCT:
		snprintf(buff, sizeof(buff), ".%c%#o", sign, udisp);
		if (com)
			snprintf(com, 32, ";=%#o", ea);
		break;
	case base_HEX:
		snprintf(buff, sizeof(buff), ".%c%#x", sign, udisp);
		if (com)
			snprintf(com, 32, ";=%#x", ea);
		break;
	case base_DEC:
	default:
		snprintf(buff, sizeof(buff), ".%c%d", sign, udisp);
		if (com)
			snprintf(com, 32, ";=%d", ea);
		break;
	}
	return buff;
}

/**
 * @brief print a signed displacement
 *
 * @param com pointer to a buffer to receive a comment
 * @param base base register number (2 or 3)
 * @param disp displacement value
 * @result returns a pointer to a string for disp in current base
 */
static const char *sdisp(char *com, int base, int disp)
{
	static char buff[32];
	char sign = (disp & 0200) ? '-' : '+';
	int udisp = (disp & 0200) ? 0200 - (disp & 0177) : disp;
	int ea = (cpu.r[base^3] + disp) & 0177777;

	switch (dbg.base) {
	case base_OCT:
		snprintf(buff, sizeof(buff), "%c%#o,%d", sign, udisp, base);
		if (com)
			snprintf(com, 32, ";=%#o", ea);
		break;
	case base_HEX:
		snprintf(buff, sizeof(buff), "%c%#x,%d", sign, udisp, base);
		if (com)
			snprintf(com, 32, ";=%#x", ea);
		break;
	case base_DEC:
	default:
		snprintf(buff, sizeof(buff), "%c%d,%d", sign, udisp, base);
		if (com)
			snprintf(com, 32, ";=%d", ea);
		break;
	}
	return buff;
}

/**
 * @brief disassemble one emulator instruction
 *
 * @param buff pointer to a string buffer to receive the disassembly
 * @param size max. size of the buffer
 * @param indent indentation wanted (non-zero after skip in previous inst.)
 * @param pc current program counter (e.g. R06)
 * @param ir instruction register, i.e. opcode
 * @result returns 1 if instruction may skip, 0 otherwise
 */
int dbg_dasm(char *buff, size_t size, int indent, int pc, unsigned int ir)
{
	char arith[8];
	char e[128];
	char com[128];
	int disp = ir & 0377;
	char *dst = buff;
	int ea = -1;
	int rc = 0;

	com[0] = '\0';

	if (IR_ARITH(ir)) {
		/* arithmetic group */
		switch (IR_AFunc(ir)) {
			case 0: strcpy(arith, "com"); break;
			case 1: strcpy(arith, "neg"); break;
			case 2: strcpy(arith, "mov"); break;
			case 3: strcpy(arith, "inc"); break;
			case 4: strcpy(arith, "adc"); break;
			case 5: strcpy(arith, "sub"); break;
			case 6: strcpy(arith, "add"); break;
			case 7: strcpy(arith, "and"); break;
		}

		switch (IR_SH(ir)) {
			case 0: break;
			case 1:	strcat(arith, "l"); break;
			case 2: strcat(arith, "r"); break;
			case 3: strcat(arith, "s"); break;
		}

		switch (IR_CY(ir)) {
			case 0: break;
			case 1: strcat(arith, "z"); break;
			case 2: strcat(arith, "o"); break;
			case 3: strcat(arith, "c"); break;
		}
		switch (IR_NL(ir)) {
			case 0: break;
			case 1: strcat(arith, "#"); break;
		}

		emit3("%-8s%d %d", arith, IR_SrcAC(ir), IR_DstAC(ir));
		switch (ir & sk_MASK) {
			case sk_NVR: break;
			case sk_SKP: emit(" skp"); rc = 1; break;
			case sk_SZC: emit(" szc"); rc = 1; break;
			case sk_SNC: emit(" snc"); rc = 1; break;
			case sk_SZR: emit(" szr"); rc = 1; break;
			case sk_SNR: emit(" snr"); rc = 1; break;
			case sk_SEZ: emit(" sez"); rc = 1; break;
			case sk_SBN: emit(" sbn"); rc = 1; break;
		}
	} else {
		if (IR_I(ir)) {
			switch (IR_X(ir)) {
			case 0:	ea = disp & 0377;
				snprintf(e, sizeof(e), "@%s",
					page0(com, disp));
				break;
			case 1:	ea = (pc + (signed char)disp) & 0177777;
				snprintf(e, sizeof(e), "@%s",
					pcrel(com, pc, (signed char)disp));
				break;
			default:
				snprintf(e, sizeof(e), "@%s",
					sdisp(com, IR_X(ir), (signed char)disp));
				break;
			}
		} else {
			switch (IR_X(ir)) {
			case 0:	ea = disp & 0377;
				snprintf(e, sizeof(e), "%s",
					page0(com, disp));
				break;
			case 1:	ea = (pc + (signed char)disp) & 0177777;
				snprintf(e, sizeof(e), "%s",
					pcrel(com, pc, (signed char)disp));
				break;
			default:
				snprintf(e, sizeof(e), "%s",
					sdisp(com, IR_X(ir), (signed char)disp));
				break;
			}
		}
		switch (IR_MFunc(ir)) {
			case 0: /* Jump Group */
				switch (IR_JFunc(ir)) {
				case 0:	/* JMP */
					emit3("%-8s%-12s%s",
						indent ? "  jmp" : "jmp", e, com);
					break;
				case 1:	/* JSR */
					emit3("%-8s%-12s%s",
						indent ? "  jsr" : "jsr", e, com);
					break;
				case 2:	/* ISZ */
					emit3("%-8s%-12s%s",
						indent ? "  isz" : "isz", e, com);
					rc = 1; 
					break;
				case 3:	/* DSZ */
					emit3("%-8s%-12s%s",
						indent ? "  isz" : "dsz", e, com);
					rc = 1; 
					break;
				}
				break;
			case 1:	/* LDA */
				emit4("%-8s%d %-10s%s",
					"lda", IR_DstAC(ir), e, com);
				break;
			case 2:	/* STA */
				emit4("%-8s%d %-10s%s",
					"sta", IR_DstAC(ir), e, com);
				break;
			case 3:
				switch (ir & 0177400) {
				case op_CYCLE:
					emit2("%-8s%03o",
						"cycle", ir & 017);
					break;
				case op_NODISP:
					switch (ir & (op_NODISP | 037)) {
					case op_DIR:
						emit("dir");
						break;
					case op_EIR:
						emit("eir");
						break;
					case op_BRI:
						emit("bri");
						break;
					case op_RCLK:
						emit("rclk");
						break;
					case op_SIO:
						emit("sio");
						break;
					case op_BLT:
						emit("blt");
						break;
					case op_BLKS:
						emit("blks");
						break;
					case op_SIT:
						emit("sit");
						break;
					case op_JMPRAM:
						emit("jmpram");
						break;
					case op_RDRAM:
						emit("rdram");
						break;
					case op_WRTRAM:
						emit("wrtram");
						break;
					case op_DIRS:
						emit("dirs");
						rc = 1;	/* skips if interrupts were disabled */
						break;
					case op_VERS:
						emit("vers");
						break;
					case op_DREAD:
						emit("dread");
						break;
					case op_DWRITE:
						emit("dwrite");
						break;
					case op_DEXCH:
						emit("dexch");
						break;
					case op_MUL:
						emit("mul");
						break;
					case op_DIV:
						emit("div");
						break;
					case op_DIAGNOSE1:
						emit("diagnose1");
						break;
					case op_DIAGNOSE2:
						emit("diagnose2");
						break;
					case op_BITBLT:
						emit("bitblt");
						break;
					case op_XMLDA:
						emit("xmlda");
						break;
					case op_XMSTA:
						emit("xmsta");
						break;
					default:
						emit2("%-8s%#o", "trap?", ir);
					}
					break;
				case op_JSRII:
					emit3("%-8s%-12s%s",
						"jsrii", pcrel(com, pc, disp), com);
					break;
				case op_JSRIS:
					emit3("%-8s%-12s%s",
						"jsris", sdisp(com, 2, disp), com);
					break;
				case op_CONVERT:
					emit3("%-8s%-12s%s",
						"convert", sdisp(com, 2, disp), com);
					break;
				default:
					emit2("%-8s%#o",
						"trap?", ir);
				}
				break;
		}
	}
	return rc;
}

/**
 * @brief dump various registers and states
 */
void dbg_dump_regs(void)
{
	static mema_info_t *info;
	static char dasm[64];
	static struct timeval tv0;
	static ntime_t cc0;
	struct timeval tv1;
	ntime_t us, cc1;
	int i, d, w, w1, h;
	int srb;

	/* prevent log messages while reading memory or mmio */
	dbg.lock = 1;

	gettimeofday(&tv1, NULL);
	cc1 = ntime();
	us = (1000000ll * tv1.tv_sec + tv1.tv_usec) -
		(1000000ll * tv0.tv_sec + tv0.tv_usec);

	/* R and S registers (task's bank) */
	srb = cpu.s_reg_bank[cpu.task];
	for (i = 0, h = 0; i < rsel_COUNT; i++)
		w = dbg_printf_xy(0, h++, rs_format[dbg.base],
			i, cpu.r[i], i, cpu.s[srb][i]);

	/* task status */
	h = 0;
	w += 1;
	dbg.color = 8;
	dbg_printf_xy(w, h++, "   TASK: LWBS pct cycles    ");
	dbg.color = 0;
	for (i = 0; i < task_COUNT; i++) {
		dbg.color = (i == cpu.task) ? 8 : 0;
		dbg_printf_xy(w, h + i, "%7s", task_name[i]);
		dbg.color = 0;
		dbg_printf_xy(w + 7, h + i, ":%d %c%c%d%3d%% %lld",
#if	DEBUG
			ll[i].level,
#else
			0,
#endif
			CPU_GET_TASK_WAKEUP(i) ? 'W' : '-',
			cpu.active_callback[i] ? 'B' : '-',
			cpu.s_reg_bank[i],
			(int)(cpu.task_ntime[i] * 100 / ntime()),
			cpu.task_ntime[i] / CPU_MICROCYCLE_TIME
		);
	}

	/* emulation speed */
	if (us >= 200000ll) {
		ntime_t ns = cc1 - cc0;
		dbg_printf_xy(w, h + i + 1, "%lldns/us (%3lld%%)   ",
			ns / us, ns > 0 ? ns / us / 10 : 0);
		tv0 = tv1;
		cc0 = cc1;
	}

	/* CPU status */
	h = 0;
	w += 29;
	dbg.color = 8;
	dbg_printf_xy(w, h, "       ALU status       ");
	dbg.color = 0;
	dbg_printf_xy(w, h + 1, cpu_format[0][dbg.base], cpu.bus);
	dbg_printf_xy(w, h + 2, cpu_format[1][dbg.base], cpu.t);
	dbg_printf_xy(w, h + 3, cpu_format[2][dbg.base], cpu.alu, cpu.aluc0);
	dbg_printf_xy(w, h + 4, cpu_format[3][dbg.base], cpu.l, cpu.laluc0);
	dbg_printf_xy(w, h + 5, cpu_format[4][dbg.base], cpu.shifter);
	dbg_printf_xy(w, h + 6, cpu_format[5][dbg.base], cpu.m);
	dbg_printf_xy(w, h + 7, cpu_format[6][dbg.base], mem.mar);
	dbg_printf_xy(w, h + 8, cpu_format[7][dbg.base], mem.md);
	dbg_printf_xy(w, h + 9, cpu_format[8][dbg.base], cpu.cram_addr);

	/* Emulator status */
	dbg.color = 8;
	dbg_printf_xy(w, h +10, "    Emulator status     ");
	dbg.color = 0;
	dbg_printf_xy(w, h +11, emu_format[0][dbg.base], emu.skip);
	dbg_printf_xy(w, h +12, emu_format[1][dbg.base], emu.cy);
	dbg_printf_xy(w, h +13, emu_format[2][dbg.base], cpu.r[3]);
	dbg_printf_xy(w, h +14, emu_format[3][dbg.base], cpu.r[2]);
	dbg_printf_xy(w, h +15, emu_format[4][dbg.base], cpu.r[1]);
	dbg_printf_xy(w, h +16, emu_format[5][dbg.base], cpu.r[0]);
	dbg_dasm(dasm, sizeof(dasm), 0, cpu.r[rsel_pc], emu.ir);
	w1 = w + dbg_printf_xy(w, h +17, emu_format[6][dbg.base], emu.ir, dasm);
	dbg_printf_xy(w1, h+18, "%*s", DBG_TEXTMAP_W - w1, "");

	/* Disk controller status */

	w += 25;
	dbg.color = 8;
	dbg_printf_xy(w, h, " Disk controller status ");
	dbg.color = 0;
	dbg_printf_xy(w, h + 1, dsk_format[ 0][dbg.base], dsk.kaddr);
	dbg_printf_xy(w, h + 2, dsk_format[ 1][dbg.base], dsk.kadr);
	dbg_printf_xy(w, h + 3, dsk_format[ 2][dbg.base], dsk.kstat);
	dbg_printf_xy(w, h + 4, dsk_format[ 3][dbg.base], dsk.kcom);
	dbg_printf_xy(w, h + 5, dsk_format[ 4][dbg.base], dsk.krecno);
	dbg_printf_xy(w, h + 6, dsk_format[ 5][dbg.base], dsk.krwc);
	dbg_printf_xy(w, h + 7, dsk_format[ 6][dbg.base], dsk.bitcount, dsk.carry);
	dbg_printf_xy(w, h + 8, dsk_format[ 7][dbg.base], dsk.shiftin & 0177777);
	dbg_printf_xy(w, h + 9, dsk_format[ 8][dbg.base], dsk.datain);
	dbg_printf_xy(w, h +10, dsk_format[ 9][dbg.base], dsk.dataout);
	dbg_printf_xy(w, h +11, dsk_format[10][dbg.base], dsk.shiftout & 0177777);

	w -= 29 + 25;
	h = 20;

	/* disk drive info */
	d = GET_KADDR_DRIVE(dsk.kaddr);
	dbg_printf_xy(w, h++, "#%d: %s page:%04d (c:%03d h:%d s:%02d adrack:%d lai:%d skinc:%d srw:%d)",
		d, drive_description(d),
		DRIVE_PAGE(GET_KADDR_CYLINDER(dsk.kaddr),
			GET_KADDR_HEAD(dsk.kaddr),
			GET_KADDR_SECTOR(dsk.kaddr)),
		GET_KADDR_CYLINDER(dsk.kaddr),
		GET_KADDR_HEAD(dsk.kaddr),
		GET_KADDR_SECTOR(dsk.kaddr),
		drive_addx_acknowledge_0(d),
		drive_log_addx_interlock_0(d),
		drive_seek_incomplete_0(d),
		drive_seek_read_write_0(d));

	/* timer peek */
	dbg.color = 8;
	dbg_printf_xy(w, h, " TimerID function(arg) +time when it fires       ");
	dbg.color = 0;
	h++;
	for (i = 0; h + i < 32 ; i++) {
		int id, arg;
		ntime_t atime;
		const char *name = timer_peek(i, &id, &arg, &atime);
		dbg_printf_xy(w, h + i, "%*s", DBG_TEXTMAP_W - w, "");
		if (name)
			dbg_printf_xy(w, h + i, "%08x %s(%d) %+lldns",
				id, name, arg, atime - ntime());
	}

	h = 32;
	if (dbg.memory) {
		dbg.color = 8;
		dbg_printf_xy(0, h++, "   Memory dump - press D, O, or H to change base - "
			" press A to toggle ASCII / Hamming code display  ");
		dbg.color = 0;
		/* memory dump */
		for (i = 0; i < DBG_DUMP_WORDS; i++) {
			int y = i / 8;
			int x0 = (i % 8) * memd_width[dbg.base];
			int x1 = mema_width[dbg.base] + (i % 8) * 2 + 8 * memd_width[dbg.base];

			if (0 == x0) {
				dbg_printf_xy(x0, h + y, mema_format[dbg.base],
					(dbg.memaddr + i) % RAM_SIZE);
			}
			x0 += mema_width[dbg.base];
			d = debug_read_mem((dbg.memaddr + i) % RAM_SIZE);
			dbg_printf_xy(x0 + 1, h + y, memd_format[dbg.base], d);

			if (i == dbg.memoffs && ((tv1.tv_usec / 1000) % 500) < 250) {
				/* display the cursor memory address word quotes */
				dbg_putch_xy(31, x0, h + y);
				dbg_putch_xy(30, x0 + memd_width[dbg.base]-1, h + y);
			} else {
				dbg_putch_xy(' ', x0, h + y);
				dbg_putch_xy(' ', x0 + memd_width[dbg.base]-1, h + y);
			}
			if (dbg.ascii) {
				dbg_putch_xy(d / 256, x1 + 1, h + y);
				dbg_putch_xy(d % 256, x1 + 2, h + y);
			} else {
				uint32_t dwa = ((dbg.memaddr + i) % RAM_SIZE) / 2;

				if (!(i & 1)) switch (dbg.base) {
				case base_DEC:
					dbg_printf_xy(x1 + 1, h + y, " %03d ", mem.hpb[dwa]);
					break;
				case base_HEX:
					dbg_printf_xy(x1 + 1, h + y, " %02x ", mem.hpb[dwa]);
					break;
				case base_OCT:
				default:
					dbg_printf_xy(x1 + 1, h + y, " %03o ", mem.hpb[dwa]);
					break;
				}
			}
		}
	} else {
		int sync = debug_read_sync(dbg.drive, dbg.page, 8*32);
		int addr = dbg.secaddr * 32 + (sync & 31);

		dbg.color = 8;
		dbg_printf_xy(0, h++, "   Drive image - press D, O, or H to change base - "
			" current drive:%d page:%04d sync:%04d           ",
			dbg.drive, dbg.page, sync);
		dbg.color = 0;
		/* drive image dump */
		for (i = 0; i < DBG_DUMP_WORDS; i++) {
			int y = i / 8;
			int x0 = (i % 8) * memd_width[dbg.base];
			int x1 = mema_width[dbg.base] + (i % 8) * 2 + 8 * memd_width[dbg.base];

			if (0 == x0) {
				dbg_printf_xy(x0, h + y, mema_format[dbg.base],
					addr + i*32);
			}
			x0 += mema_width[dbg.base];
			d = debug_read_sec(dbg.drive, dbg.page, addr + i*32);
			dbg_printf_xy(x0 + 1, h + y, memd_format[dbg.base], d);

			if (i == dbg.secoffs && ((tv1.tv_usec / 1000) % 500) < 250) {
				/* display the cursor memory address word quotes */
				dbg_putch_xy(31, x0, h + y);
				dbg_putch_xy(30, x0 + memd_width[dbg.base]-1, h + y);
			} else {
				dbg_putch_xy(' ', x0, h + y);
				dbg_putch_xy(' ', x0 + memd_width[dbg.base]-1, h + y);
			}
			dbg_putch_xy(d / 256, x1 + 1, h + y);
			dbg_putch_xy(d % 256, x1 + 2, h + y);
		}
	}

	h += DBG_DUMP_WORDS / 8;

	/* memory address info */
	i = (dbg.memaddr + dbg.memoffs) % RAM_SIZE;
	for (info = mema_info; info->name; info++) {
		if (0 == info->mask)
			break;
		if ((i&info->mask) >= info->from && (i&info->mask) <= info->to)
			break;
	}
	/* no info on memory location? */
	if (info->mask == 0)
		info = NULL;

	h++;
	if (info && info->name) {
		/* print memory address info */
		w = dbg_printf_xy(0, h, "%s", info->name);
		if (info->from != info->to && (i & info->mask) > info->from)
			w += dbg_printf_xy(w, h, "+%#o", (i & info->mask) - info->from);
		w += dbg_printf_xy(w, h, "%*s%s", 16 - w, "", info->comment);
		dbg_printf_xy(w, h, "%*s", DBG_TEXTMAP_W - w, "");

		h++;
		dbg_bits_info(0, h, debug_read_mem(i), info->bits);
	} else if ((i & 0177777) < IO_PAGE_BASE) {
		/* disassemble RAM */
		int y, skip;
		for (y = 0, skip = 0; y < DBG_BITS_H; y++) {
			d = debug_read_mem((i + y) % RAM_SIZE);
			skip = dbg_dasm(dasm, sizeof(dasm), skip, i+y, d);
			w = dbg_printf_xy(0, h + y, mema_format[dbg.base], (i + y) % RAM_SIZE);
			dbg_printf_xy(w, h + y, "%*s", DBG_TEXTMAP_W - w, "");
			w++;
			w += dbg_printf_xy(w, h + y, memd_format[dbg.base], d);
			dbg_printf_xy(16, h + y, "%s", dasm);
		}
	}

	dbg_flush_textmap();

	/* enable log messages again */
	dbg.lock = 0;
}


#if	DEBUG

/**
 * @brief callback hooked into memory write function
 *
 * @param mar memory address that was written
 * @param md memory data that was written
 */
static void dbg_watch_write(int mar, int md)
{
	int i;
	for (i = 0; i < dbg.ww_count; i++)
		if (mar == dbg.ww_addr[i])
			break;
	if (i == dbg.ww_count)
		return;
	dbg_printf("task %s hit write mem watchpoint #%d\n",
		task_name[cpu.task], i+1);
	debug_view(1);
	paused = 1;
}

/**
 * @brief callback hooked into memory read function
 *
 * @param mar memory address that was read
 * @param md memory data that was read
 */
static void dbg_watch_read(int mar, int md)
{
	int i;
	for (i = 0; i < dbg.wr_count; i++)
		if (mar == dbg.wr_addr[i])
			break;
	if (i == dbg.wr_count)
		return;
	dbg_printf("task %s hit read mem watchpoint #%d\n",
		task_name[cpu.task], i+1);
	debug_view(1);
	paused = 1;
}

/**
 * @brief log print a format string
 *
 * @param task 0/log_CPU: use cpu.task as index for loglevel, or one of log_t
 * @param level level of importance (higher values are less important)
 * @param fmt format string and optionally arguments
 * @result returns size of string written
 */
int logprintf(int task, int level, const char *fmt, ...)
{
	va_list ap;
	int size;

	/* don't print debug messages while the debug_lock is set */
	if (dbg.lock)
		return 0;
	if (0 == task)
		task = cpu.task;
	if (task > log_COUNT)
		fatal(1, "invalid log task/type number %d\n", task);
	if (level > ll[task].level)
		return 0;
	va_start(ap,fmt);
	size = vfprintf(stdout, fmt, ap);
	dbg_vprintf(fmt, ap);
	va_end(ap);
	fflush(stdout);

	return size;
}

/**
 * @brief adjust a loglevel by a delta value
 *
 * Add the delta to the loglevel, and clip the result to 0...9
 *
 * @param n number of the loglevel entry
 * @param delta negative or positive value
 */
static void adjust_loglevel(int n, int delta)
{
	if (n < 0 || n >= sizeof(ll)/sizeof(ll[0]))
		fatal(1,"internal error: bad loglevel index %d\n", n);
	ll[n].level += delta;
	if (ll[n].level < 0)
		ll[n].level = 0;
	else if (ll[n].level > 9)
		ll[n].level = 9;
	printf("set loglevel=%d for %s\n", ll[n].level, ll[n].description);
}

/**
 * @brief pass command line switches down to the debugger
 *
 * @param arg a pointer to a command line switch, like "-all" or "+tsw"
 * @result returns 0 if arg was accepted, -1 otherwise
 */
int dbg_args(const char *arg)
{
	char *equ;
	int i, val;

	switch (arg[0]) {
	case '-':
		arg++;
		equ = strchr(arg, '=');
		if (equ)
			val = -strtoull(equ+1,NULL,0);
		else
			val = -9;
		break;
	case '+':
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

	for (i = 0; i < log_COUNT; i++)
		if (!strncmp(arg,ll[i].name,strlen(ll[i].name)-1))
			break;
	if (i < log_COUNT) {
		adjust_loglevel(i, val);
	} else if (!strncmp(arg, "all", 3)) {
		/* "all" entry */
		for (i = 0; i < log_COUNT; i++)
			adjust_loglevel(i, val);
	} else if (!strncmp(arg,"wwa",3)) {
		mem.watch_write = dbg_watch_write;
		dbg.ww_addr[dbg.ww_count++] = abs(val);
		printf("watchpoint #%d write to addr %#o (%#x)\n",
			dbg.ww_count, abs(val), abs(val));
	} else if (0 == strncmp(arg,"wra",3)) {
		mem.watch_read = dbg_watch_read;
		dbg.wr_addr[dbg.wr_count++] = abs(val);
		printf("watchpoint #%d read from addr %#o (%#x)\n",
			dbg.wr_count, abs(val), abs(val));
	} else return -1;

	return 0;
}

/**
 * @brief print usage info for the debugger switches
 *
 * @param argc argument count
 * @param argv argument list
 * @result returns 0 (why should it fail?)
 */
int dbg_usage(int argc, char **argv)
{
	int i, w;
	for (i = 0; i < log_COUNT; i++) {
		w = printf("+|-%s", ll[i].name);
		while (w < 16) {
			w = (w | 7) + 1;
			printf("\t");
		}
		printf("turn on|off %s\n", ll[i].description);
	}
	printf("+wra=addr	add watchpoint mem read mar = addr\n");
	printf("+wwa=addr	add watchpoint mem write mar = addr\n");
	return 0;
}

#else	/* DEBUG */

int dbg_usage(int argc, char **argv)
{
	return 0;
}

#endif	/* !DEBUG */

/**
 * @brief timer callback to dump debug regs
 *
 * @param id timer id
 * @param arg update rate (every arg CPU cycles)
 */
static void dbg_callback(int id, int arg)
{
	if (dbg.visible)
		dbg_dump_regs();

	timer_insert(arg * CPU_MICROCYCLE_TIME, dbg_callback, arg, "dbg_dump_regs");
}

/**
 * @brief initialize the debugger and start the timer
 *
 * @result returns 0 on success
 */
int debug_init(void)
{
	/* clear out the debugger context */
	memset(&dbg, 0, sizeof(dbg));

	sdl_chargen_alpha(&chargen[0], 0, sdl_rgb(  0,  0,  0));
	sdl_chargen_alpha(&chargen[0], 1, sdl_rgb(  0,  0, 63));
	sdl_chargen_alpha(&chargen[0], 2, sdl_rgb(  0, 63,  0));
	sdl_chargen_alpha(&chargen[0], 3, sdl_rgb(  0, 63, 63));
	sdl_chargen_alpha(&chargen[0], 4, sdl_rgb( 63,  0,  0));
	sdl_chargen_alpha(&chargen[0], 5, sdl_rgb( 63,  0, 63));
	sdl_chargen_alpha(&chargen[0], 6, sdl_rgb( 63, 63,  0));
	sdl_chargen_alpha(&chargen[0], 7, sdl_rgb( 63, 63, 63));

	/* display memory dump */
	dbg.memory = 1;

	/* display ASCII for memory locations */
	dbg.ascii = 1;

	dbg_clear_screen();

	dbg_printf("SALTO Debugger:\n");
	dbg_printf("[SCROLLOCK] key to toggle Alto and Debugger display.\n");
#if	DEBUG
	/* These only work in a debug build */
	dbg_printf("[PAUSE] key to pause/continue.\n");
	dbg_printf("[RETURN] key to single step when paused.\n");
#endif
	dbg_printf("[LEFT], [RIGHT], [UP], [DOWN] keys to move memory cursor.\n");
	dbg_printf("[PGUP], [PGDOWN] to page up/down (+[CTRL] to page * 16)\n");
	dbg_printf("[HOME], [END] first/last word (+[CTRL] start/end of memory)\n");
	dbg_printf("[INSERT] to load a binary specified on the command line.\n");
	dbg_printf("[A] toggle between ASCII and Hamming/Parity bits display.\n");
	dbg_printf("[D] to switch to decimal, [O] octal, [H] hexadecimal.\n");

	timer_insert(update_rate * CPU_MICROCYCLE_TIME,
		dbg_callback, update_rate, "dbg_dump_regs");
	return 0;
}
