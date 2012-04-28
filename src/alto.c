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
 * Alto ROMs, PROMs and constants initializing
 *
 * $Id: alto.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "SDL.h"

#include "alto.h"
#include "cpu.h"
#include "emu.h"
#include "memory.h"
#include "ram.h"
#include "display.h"
#include "mouse.h"
#include "ether.h"
#include "md5.h"

#define	DUMP_UCODE_PROM	0
#define	DUMP_CONST_PROM	0
#define	DUMP_MISC_PROM	0
#define	DUMP_MISC_BIN	0

/**
 * @brief structure to define a ROM's or PROM's loading options
 */
typedef struct {
	/** @brief (usual, alternate) filename of the ROM image */
	const char *name, *name2;
	/** @brief MD5 hash of the file */
	const char *md5;
	/** @brief size of the file, and elements in destination memory */
	int size;
	/** @brief base address where to load ROM */
	void *base;
	/** @brief pointer to an address bit mapping, or NULL for default */
	const uint8_t amap[16];
	/** @brief address XOR mask (applied to source address) */
	int axor;
	/** @brief data XOR mask (applied before shifting and mapping) */
	int dxor;
	/** @brief width in bits */
	int width;
	/** @brief left shift in bits */
	int shift;
	/** @brief data bit mapping */
	const uint8_t dmap[16];
	/** @brief ANDing destination with this value, before ORing the data */
	int dand;
	/** @brief type of the destination, i.e. sizeof(type) */
	int type;
}	rom_load_t;

#define	ZERO			0
#define	KEEP			0xffffffff

#define	AMAP_DEFAULT		{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
#define	AMAP_CONST_PROM		{3,2,1,4,5,6,7,0,}
#define	AMAP_REVERSE_0_7	{7,6,5,4,3,2,1,0,}

#define	DMAP_DEFAULT		{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
#define	DMAP_REVERSE_0_3	{3,2,1,0,}

/**
 * @brief list of microcoode PROM filenames
 */
static const rom_load_t ucode_prom_list[] = {
	{	/* 0000-01777 RSEL(0)',RSEL(1)',RSEL(2)',RSEL(3)' */
		"55x.3",	NULL,
		"32914b59426607d613b956bd0b8b9c4f",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	28,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint32_t)
	},
	{	/* 0000-01777 RSEL(4)',ALUF(0)',ALUF(1)',ALUF(2)' */
		"64x.3",	NULL,
		"64291e447555d57af64eeb2b34f28ab7",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	24,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 0000-01777 ALUF(3)',BS(0)',BS(1)',BS(2)' */
		"65x.3",	NULL,
		"2a79beaaebb770ab98f41369a9ff225a",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	20,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 0000-01777 F1(0),F1(1)',F1(2)',F1(3)' */
		"63x.3",	NULL,
		"081127d21081d35475c010597ef48c0b",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK, 
/* dxor */	007,						/* invert D1-D3 */
/* width */	4,
/* shift */	16,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 0000-01777 F2(0),F2(1)',F2(2)',F2(3)' */
		"53x.3",	NULL,
		"2be73393fd81d108f2da178468d859c4",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK, 
/* dxor */	007,						/* invert D1-D3 */
/* width */	4,
/* shift */	12,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 0000-01777 LOADT',LOADL,NEXT(0)',NEXT(1)' */
		"60x.3",	NULL,
		"cfb4cd3778b1644e16b769cd1407b3ec",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	013,						/* invert D0,D2-D3 */
/* width */	4,
/* shift */	8,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 0000-01777 NEXT(2)',NEXT(3)',NEXT(4)',NEXT(5)' */
		"61x.3",	NULL,
		"bf938c13565228e8a26021b044512998",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	4,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 0000-01777 NEXT(6)',NEXT(7)',NEXT(8)',NEXT(9)' */
		"62x.3",	NULL,
		"dbaf81087cec042f9c5204a5111e0657",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	}

#if	(UCODE_ROM_PAGES > 1)
	,
	{	/* 02000-03777 RSEL(0)',RSEL(1)',RSEL(2)',RSEL(3)' */
		"xm51.u54",	NULL,
		"23cbe063e896a4a09aea6ee9872bd151",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	28,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint32_t)
	},
	{	/* 02000-03777 RSEL(4)',ALUF(0)',ALUF(1)',ALUF(2)' */
		"xm51.u74",	NULL,
		"709b01654e758ff77f41824062e6f3e3",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	24,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 02000-03777 ALUF(3)',BS(0)',BS(1)',BS(2)' */
		"xm51.u75",	NULL,
		"e80368aca0ddd53e942475ed0d5270d6",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	20,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 02000-03777 F1(0),F1(1)',F1(2)',F1(3)' */
		"xm51.u73",	NULL,
		"4b7abe072c6934adc0dbac1189e82a7c",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK, 
/* dxor */	007,						/* invert D1-D3 */
/* width */	4,
/* shift */	16,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 02000-03777 F2(0),F2(1)',F2(2)',F2(3)' */
		"xm51.u52",	NULL,
		"901c19ca502bb10de280fda12f5960d8",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK, 
/* dxor */	007,						/* invert D1-D3 */
/* width */	4,
/* shift */	12,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 02000-03777 LOADT',LOADL,NEXT(0)',NEXT(1)' */
		"xm51.u70",	NULL,
		"3e122f85495085e05a85395e992615eb",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	013,						/* invert D0,D2-D3 */
/* width */	4,
/* shift */	8,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 02000-03777 NEXT(2)',NEXT(3)',NEXT(4)',NEXT(5)' */
		"xm51.u71",	NULL,
		"a9b3f2e38a72c58e1dce999e6874ad61",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	4,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{	/* 02000-03777 NEXT(6)',NEXT(7)',NEXT(8)',NEXT(9)' */
		"xm51.u72",	NULL,
		"2b57ebd7069834c1a574055a9590ba4e",
/* size */	UCODE_PAGE_SIZE,
/* base */	ucode_raw + UCODE_PAGE_SIZE,
/* amap */	AMAP_DEFAULT,
/* axor */	UCODE_PAGE_MASK,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	}
#endif	/* (UCODE_ROM_PAGES > 1) */
};

static const rom_load_t const_prom_list[] = {
	{
		"madr.a6",	"c3.3",
		"1cc09387e9024f4490ce51edd67e4742",
/* size */	CONST_SIZE,
/* base */	const_prom,
/* amap */	AMAP_CONST_PROM,				/* descramble constant address */
/* axor */	0,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_REVERSE_0_3,				/* reverse D0-D3 to D3-D0 */
/* dand */	ZERO,
/* type */	sizeof(uint32_t)
	},
	{
		"madr.a5",	"c2.3",
		"89605e96a6ac0f66fce16844e6fa0113",
/* size */	CONST_SIZE,
/* base */	const_prom,
/* amap */	AMAP_CONST_PROM,				/* descramble constant address */
/* axor */	0,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	4,
/* dmap */	DMAP_REVERSE_0_3,				/* reverse D0-D3 to D3-D0 */
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{
		"madr.a4",	"c1.3",
		"5c16cee2a79926bfae348ff46d2efb74",
/* size */	CONST_SIZE,
/* base */	const_prom,
/* amap */	AMAP_CONST_PROM,				/* descramble constant address */
/* axor */	0,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	8,
/* dmap */	DMAP_REVERSE_0_3,				/* reverse D0-D3 to D3-D0 */
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	},
	{
		"madr.a3",	"c0.3",
		"394c625b29261fd54b91efec11429ec8",
/* size */	CONST_SIZE,
/* base */	const_prom,
/* amap */	AMAP_CONST_PROM,				/* descramble constant address */
/* axor */	0,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	12,
/* dmap */	DMAP_REVERSE_0_3,				/* reverse D0-D3 to D3-D0 */
/* dand */	KEEP,
/* type */	sizeof(uint32_t)
	}
};

static const rom_load_t prom_list[] = {
	{	/* P3601 256x4 BPROM; display FIFO control: STOPWAKE, MBEMPTY */
		"displ.a38",	NULL,
		"af1dcb5aacea67756c0c7f0eade018bb",
/* size */	sizeof(displ_a38),
/* base */	displ_a38,
/* amap */	AMAP_REVERSE_0_7,				/* reverse address lines A0-A7 */
/* axor */	0,
/* dxor */	0,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* 82S23 32x8 BPROM; display HBLANK, HSYNC, SCANEND, HLCGATE ... */
		"displ.a63",	NULL,
		"b7e9b644a154c8884288daa341922df7",
/* size */	sizeof(displ_a63),
/* base */	displ_a63,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	0,
/* width */	8,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* P3601 256x4 BPROM; display VSYNC and VBLANK */
		"displ.a66",	NULL,
		"b33e2539c3b781bf838e97f79cdca036",
/* size */	sizeof(displ_a66),
/* base */	displ_a66,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	0,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* 3601-1 256x4 BPROM; Emulator address modifier */
		"2kctl.u3",	NULL,
		"831cdebba31897b49bed25626053f03e",
/* size */	sizeof(ctl2k_u3),
/* base */	ctl2k_u3,
/* amap */	AMAP_REVERSE_0_7,				/* reverse address lines A0-A7 */
/* axor */	0377,						/* invert address lines A1-A7 */
/* dxor */	017,						/* invert data lines D0-D3 */
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* 82S23 32x8 BPROM; task priority and initial address */
		"2kctl.u38",	NULL,
		"c5f958041964d4b8f8f4c610f4cb194d",
/* size */	sizeof(ctl2k_u38),
/* base */	ctl2k_u38,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	0,
/* width */	8,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* 3601-1 256x4 BPROM; 2KCTL replacement for u51 (1KCTL) */
		"2kctl.u76",	NULL,
		"b210c2670f8f79026151abb75ac25a5b",
/* size */	sizeof(ctl2k_u76),
/* base */	ctl2k_u76,
/* amap */	AMAP_DEFAULT,
/* axor */	0077,						/* invert address lines A0-A5 */
/* dxor */	0,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{
		"3kcram.a37",	NULL,
		"e43c6168075675bb6aca5bcff414357a",
/* size */	sizeof(cram3k_a37),
/* base */	cram3k_a37,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	017,						/* invert D0-D3 */
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{
		"madr.a32",	NULL,
		"a7f0d1e5bb063c911f856631a6912033",
/* size */	sizeof(madr_a32),
/* base */	madr_a32,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	017,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_REVERSE_0_3,				/* reverse D0-D3 to D3-D0 */
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{
		"madr.a64",	NULL,
		"6bd1f4ed11372da23f6ffa7a11637f5e",
/* size */	sizeof(madr_a64),
/* base */	madr_a64,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	017,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{
		"madr.a65",	NULL,
		"dd90886e1ccdb7274444f1a44aa7fb9c",
/* size */	sizeof(madr_a65),
/* base */	madr_a65,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	017,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* P3601 256x4 BPROM; Ethernet phase encoder 1 "PE1" */
		"enet.a41",	NULL,
		"0f18a6a1432d7e4eb0dee545775c553a",
/* size */	sizeof(ether_a41),
/* base */	ether_a41,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	0,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* P3601 256x4 BPROM; Ethernet phase encoder 2 "PE2" */
		"enet.a42",	NULL,
		"32cefe07ec1540d221d3384cb79a0d47",
/* size */	sizeof(ether_a42),
/* base */	ether_a42,
/* amap */	AMAP_DEFAULT,
/* axor */	0,
/* dxor */	0,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	},
	{	/* P3601 256x4 BPROM; Ethernet FIFO control "AFIFO" */
		"enet.a49",	NULL,
		"bbba396731d62cbc650b7b099da30ff5",
/* size */	sizeof(ether_a49),
/* base */	ether_a49,
/* amap */	AMAP_REVERSE_0_7,				/* reverse address lines A0-A7 */
/* axor */	0,
/* dxor */	0,
/* width */	4,
/* shift */	0,
/* dmap */	DMAP_DEFAULT,
/* dand */	ZERO,
/* type */	sizeof(uint8_t)
	}
};


/**
 * @brief return number of 1 bits in a 32 bit value
 *
 * 32-bit recursive reduction using SWAR,
 * but first step is mapping 2-bit values
 * into sum of 2 1-bit values in sneaky way.
 */
static uint32_t ones_u32(uint32_t val)
{
        val -= ((val >> 1) & 0x55555555);
        val = (((val >> 2) & 0x33333333) + (val & 0x33333333));
        val = (((val >> 4) + val) & 0x0f0f0f0f);
        val += (val >> 8);
        val += (val >> 16);
        return (val & 0x0000003f);
}

/**
 * @brief return the log2 of an integer value
 */
static uint32_t log2_u32(uint32_t val)
{
	val |= (val >> 1);
	val |= (val >> 2);
	val |= (val >> 4);
	val |= (val >> 8);
	val |= (val >> 16);
	return ones_u32(val >> 1);
}

#if	DUMP_PROMS_BIN
/**
 * @brief convert a value to a binary string
 *
 * @param val value
 * @param width number of (rightmost) bits
 * @result returns a string of 'width' 1s and 0s
 */
static const char *bin2str(uint32_t val, int width)
{
	static char str[32+1];
	int i;

	if (width > 32)
		fatal(1, "bin2str() width > 32\n");
	i = 32;
	while (width-- > 0) {
		str[--i] = '0' + (val & 1);
		val>>= 1;
	}
	return &str[i];
}
#endif


/**
 * @brief map a number of data or address lines using a lookup table
 *
 * @param map pointer to an array of values, or NULL for default
 * @param lines number of data or address lines
 * @param val value to map
 * @result returns the remapped value, or just val, if map was NULL
 */
static uint32_t map_lines(const uint8_t *map, int lines, uint32_t val)
{
	int i;
	uint32_t res = 0;

	if (NULL == map)
		return val;

	for (i = 0; i < lines; i++)
		if (val & (1 << i))
			res |= 1 << map[i];
	return res;
}

/**
 * @brief write to a ROM base + address of type 'type', ANDing with and, ORing with or
 *
 * @param base ROM base address in memory
 * @param type one of 1 for uint8_t, 2 for uint16_t, 4 for uint32_t
 * @param addr address offset into base
 * @param and value to AND to contents before ORing
 * @param or value to OR before writing back
 */
static void write_type_and_or(void *base, int type, uint32_t addr, uint32_t and, uint32_t or)
{
	uint32_t *base32 = (uint32_t *)base;
	uint16_t *base16 = (uint16_t *)base;
	uint8_t *base8 = (uint8_t *)base;

	switch (type) {
	case sizeof(uint8_t):
		base8[addr] = (base8[addr] & and) | or;
		break;
	case sizeof(uint16_t):
		base16[addr] = (base16[addr] & and) | or;
		break;
	case sizeof(uint32_t):
		base32[addr] = (base32[addr] & and) | or;
		break;
	default:
		fatal(1, "write_type_and_or() invalid type size (%d) in ROM definitions\n", type);
	}
}

#if	(DUMP_UCODE_PROM || DUMP_CONST_PROM || DUMP_MISC_PROM)
/**
 * @brief read from a ROM base + address of type 'type'
 *
 * @param base ROM base address in memory
 * @param type one of 1 for uint8_t, 2 for uint16_t, 4 for uint32_t
 * @param addr address offset into base
 * @result the contents of that ROM address extended to 32 bits
 */
static uint32_t read_type(void *base, int type, uint32_t addr)
{
	uint32_t *base32 = (uint32_t *)base;
	uint16_t *base16 = (uint16_t *)base;
	uint8_t *base8 = (uint8_t *)base;

	switch (type) {
	case sizeof(uint8_t):
		return base8[addr];
		break;
	case sizeof(uint16_t):
		return base16[addr];
		break;
	case sizeof(uint32_t):
		return base32[addr];
		break;
	default:
		fatal(1, "read_type() invalid type size (%d) in ROM definitions\n", type);
	}
	/* pacify the compiler */
	return 0;
}
#endif

/**
 * @brief load a file from a specified path and check its MD5 hash
 *
 * @param path pathname component
 * @param name filename component
 * @param size expected size of the (P)ROM image
 * @param md5 expected MD5 digest for the (P)ROM image
 * @result allocated array of bytes with file contents, or NULL on error
 */
static uint8_t *load_file_md5(const char *path, const char *name, size_t size, const char *md5)
{
	char filename[FILENAME_MAX];
	const char *md5_check;
	FILE *fp;
	uint8_t *buf;

	buf = malloc(size);
	if (!buf)
		return NULL;

	snprintf(filename, sizeof(filename), "%s/%s", path, name);
	fp = fopen(filename, "rb");
	if (!fp)
		fatal(3, "fopen(%s,\"rb\") failed (%s)\n",
			filename, strerror(errno));
	if (size != fread(buf, 1, size, fp))
		fatal(3, "fread(%d) for file %s failed (%s)\n",
			size, filename, strerror(errno));
	if (fclose(fp))
		fatal(3, "fclose(%s) failed (%s)\n",
			filename, strerror(errno));
	md5_check = md5_digest(buf, size);

	if (md5 && strcmp(md5, md5_check)) {
		LOG((0,0,"md5 check failed for %s (%s)\n", filename, md5_check));
	}
	return buf;
}

/**
 * @brief load the microcode PROMs
 *
 * TODO: unify loaders
 *
 * @param path pathname where to find the PROM files
 */
static int load_ucode(const char *path)
{
	uint8_t *buf;
	uint32_t data, addr;
	int page, segment;

	/* initialize to default (inverted bits = 1) */
	for (addr = 0; addr < sizeof(ucode_raw)/sizeof(ucode_raw[0]); addr++)
		ucode_raw[addr] = UCODE_INVERTED;

	for (page = 0; page < UCODE_ROM_PAGES; page++) {
		for (segment = 0; segment < 8; segment++) {
			const rom_load_t *rom = &ucode_prom_list[8 * page + segment];

			buf = load_file_md5(path, rom->name, rom->size, rom->md5);
			if (!buf && rom->name2)
				buf = load_file_md5(path, rom->name2, rom->size, rom->md5);
			if (!buf)
				fatal(1, "load_file_md5(%s,%s,%#x,%s) failed\n",
					path, rom->name, rom->size, rom->md5);
			for (addr = 0; addr < rom->size; addr++) {
				int a = map_lines(rom->amap, log2_u32(rom->size)+1, addr);
				data = buf[addr ^ rom->axor] ^ rom->dxor;
				data = data & BITMASK(0,rom->width-1);
				data = map_lines(rom->dmap, rom->width, data);
				data = data << rom->shift;
				write_type_and_or(rom->base, rom->type, a, rom->dand, data);
			}
			free(buf);
		}
#if	DUMP_UCODE_PROM
		for (addr = 0; addr < UCODE_PAGE_SIZE; addr++) {
			int data = read_type(rom->base, rom->type, addr);
			printf("%05o: %03o %02o %o %02o %02o %o %o %04o\n",
				a, MIR_RSEL(data), MIR_ALUF(data), MIR_BS(data),
				MIR_F1(data), MIR_F2(data), MIR_T(data), MIR_L(data),
				MIR_NEXT(data));
		}
#endif
	}
#if	DUMP_UCODE_PROM
	printf("\n");
#endif
	return 0;
}

/**
 * @brief load the constant PROMs
 *
 * TODO: unify loaders
 *
 * @param path pathname where to find the PROM files
 */
static int load_const(const char *path)
{
	uint8_t *buf;
	uint32_t data, addr;
	int segment;

	for (segment = 0; segment < 4; segment++) {
		const rom_load_t *rom = &const_prom_list[segment];

		buf = load_file_md5(path, rom->name, rom->size, rom->md5);
		if (!buf && rom->name2)
			buf = load_file_md5(path, rom->name2, rom->size, rom->md5);
		if (!buf)
			fatal(1, "load_file_md5(%s,%s,%#x,%s) failed\n",
				path, rom->name, rom->size, rom->md5);
		for (addr = 0; addr < rom->size; addr++) {
			int a = map_lines(rom->amap, log2_u32(rom->size)+1, addr);
			data = buf[addr ^ rom->axor] ^ rom->dxor;
			data = data & BITMASK(0,rom->width-1);
			data = map_lines(rom->dmap, rom->width, data);
			data = data << rom->shift;
			write_type_and_or(rom->base, rom->type, a, rom->dand, data);
		}
		free(buf);
	}
#if	DUMP_CONST_PROM
	printf("constants PROMs:");
	/* dump PROM contents */
	for (addr = 0; addr < CONST_SIZE; addr++) {
		int data = read_type(rom->base, rom->type, addr);
		if (addr % 8)
			printf(" %06o", data);
		else
			printf("\n%03o: %06o", addr, data);
	}
	printf("\n");
#endif
	return 0;
}

static int load_proms(const char *path)
{
	uint8_t *buf;
	uint32_t data, addr;
	int i;

	for (i = 0; i < sizeof(prom_list)/sizeof(prom_list[0]); i++) {
		const rom_load_t *rom = &prom_list[i];

		buf = load_file_md5(path, rom->name, rom->size, rom->md5);
		if (!buf && rom->name2)
			buf = load_file_md5(path, rom->name2, rom->size, rom->md5);
		if (!buf)
			fatal(1, "load_file_md5(%s,%s,%#x,%s) failed\n",
				path, rom->name, rom->size, rom->md5);
		for (addr = 0; addr < rom->size; addr++) {
			int a = map_lines(rom->amap, log2_u32(rom->size)+1, addr);
			if (a >= rom->size)
				fatal(1, "wrong address map for PROM %s", rom->name);
			data = buf[addr ^ rom->axor] ^ rom->dxor;
			data = data & BITMASK(0,rom->width-1);
			data = map_lines(rom->dmap, rom->width, data);
			data = data << rom->shift;
			write_type_and_or(rom->base, rom->type, a, rom->dand, data);
		}
		free(buf);
#if	DUMP_MISC_PROM
		printf("PROM: %s (%dx%d)", rom->name, rom->size, rom->width);
		for (addr = 0; addr < rom->size; addr++) {
			int data = read_type(rom->base, rom->type, addr);
#if	DUMP_MISC_BIN
			if (addr % (64 / rom->width))
				printf(" %s", bin2str(data, rom->width));
			else
				printf("\n%04o: %s", addr, bin2str(data, rom->width));
#else
			if (addr % 16)
				printf(" %04o", data);
			else
				printf("\n%04o: %04o", addr, data);
#endif
		}
		printf("\n\n");
#endif
	}
	return 0;
}

int alto_init(const char *rompath)
{
	struct stat st;
	int rc;

	if (0 != (rc = md5_test())) {
		fatal(1,"MD5 test failed (%d)\n", rc);
	}

	if (stat(rompath, &st)) {
		fatal(2,"rompath %s is invalid\n", rompath);
	}
	if (!S_ISDIR(st.st_mode)) {
		fatal(2,"rompath %s is not a directory\n", rompath);
	}
	if (load_ucode(rompath))
		return -1;
	if (load_const(rompath))
		return -1;
	if (load_proms(rompath))
		return -1;
	return 0;
}

int alto_usage(int argc, char **argv)
{
	printf("+|-h		display this help\n");
	return 0;
}
