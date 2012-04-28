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
 * Winchester drive implementation
 *
 * $Id: drive.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "debug.h"
#include "drive.h"
#include "zcat.h"

#define	DIABLO31		1

/** @brief number of words in a page number (this doesn't really belong here) */
#define	PAGENO_WORDS		1

/** @brief number of words in a header (this doesn't really belong here) */
#define	HEADER_WORDS		2

/** @brief number of words in a label (this doesn't really belong here) */
#define	LABEL_WORDS		8

/** @brief number of data words (this doesn't really belong here) */
#define	DATA_WORDS		256

/** @brief number of words for a checksum (this doesn't really belong here) */
#define	CKSUM_WORDS		1

/** @brief from the microcode: disk header preamble is 34 words */
#define	MFROBL		34

/** @brief from the microcode: disk header read delay is 21 words */
#define	MFRRDL		21

/** @brief from the microcode: interrecord read delay is 4 words */
#define	MIRRDL		4

/** @brief from the microcode: disk interrecord preamble is 3 words */
#define	MIROBL		3

/** @brief from the microcode: disk read postamble length is 3 words */
#define	MRPAL		3

/** @brief from the microcode: disk write postamble length is 5 words */
#define	MWPAL		5

/**
 * @brief description of the sector layout
 * <PRE>
 *
 *                                   xx.x msec sector mark pulses
 * -+   +-------------------------------------------------------------------------------+   +--
 *  |   |                                                                               |   |
 *  +---+                                                                               +---+
 *                                                                                         
 *    |                                                                                   |
 *                                                                                         
 *    +------+----+------+-----+------+----+-------+-----+------+----+-------+-----+------+
 *    | PRE- |SYNC|HEADER|CKSUM| PRE- |SYNC| LABEL |CKSUM| PRE- |SYNC| DATA  |CKSUM| POST |
 *    |AMBLE1|  1 |      |  1  |AMBLE2|  2 |       |  2  |AMBLE3|  3 |       |  3  |AMBLE |
 *    +------+----+------+-----+------+----+-------+-----+------+----+-------+-----+------+
 *                                                                                         
 *    |                                                                                   |
 *                                                                                         
 *    +-----------------------------------------------------------------------------------+    
 *    |                                                                                   |   
 * ---+                                                                                   +----
 *      FORMAT WRITE GATE FOR INITIALIZING
 *    |                                                                                   |
 *                                                                                         
 *    |                                                    +------------------------------+    
 *                                                         |                              |   
 * ---|----------------------------------------------------+                              +----
 *      WRITE GATE FOR DATA XFER (*)
 *    |                                                                                   |
 *                                                                                         
 *    |                          +-----------------------+-+------------------------------+    
 *                               |                       | | may be continuous (?)        |   
 * ------------------------------+                       +-+                              +----
 * ???  WRITE GATE FOR LABEL AND DATA XFER (*)
 *    |                                                                                   |
 *                                                                                         
 *    |   +--------------------+   +---------------------+   +----------------------------+    
 *        |                    |   |                     |   |                            |   
 * -------+                    +---+                     +---+                            +----
 *      READ GATE FOR INITIALIZING OR DATA XFER (**)
 *
 *
 *  (*) Enable should be delayed 1 byte/word time from last bit of checks sum.
 *  (**) Read Gate should be enabled 1/2 way through the preamble area. This
 *       insures reading a zero field for data separator synchronization.
 *
 * </PRE>
 */
#if	DIABLO31

/** @brief DIABLO 31 rotation time is approx. 40ms */
#define	ROTATION_TIME	TIME_MS(39.9999)

/** @brief DIABLO 31 sector time */
#define	SECTOR_TIME	(ROTATION_TIME/DRIVE_SPT)

/** @brief DIABLO 31 bit clock is 3330kHz ~= 300ns per bit
 * ~= 133333 bits/track (?)
 * ~= 11111 bits/sector
 * ~= 347 words/sector
 */
#define	BIT_TIME	TIME_NS(300)

/** @brief DIABLO 31 possible sector words */
#define	SECTOR_WORDS	(int)(SECTOR_TIME / BIT_TIME / 32)


/** @brief pulse width of sector mark before the next sector begins */
#define	SECTOR_MARK_PULSE_PRE	(16 * BIT_TIME)

/** @brief pulse width of sector mark after the next sector began */
#define	SECTOR_MARK_PULSE_POST	(16 * BIT_TIME)

#else	/* DIABLO31 */


/** @brief DIABLO 44 rotation time is approx. 25ms */
#define	ROTATION_TIME	TIME_MS(25)

/** @brief DIABLO 44 sector time */
#define	SECTOR_TIME	(ROTATION_TIME/DRIVE_SPT)

/** @brief DIABLO 44 bit clock is 5000kHz ~= 200ns per bit
 * ~= 125184 bits/track (?)
 * ~= 10432 bits/sector
 * ~= 325 words/sector
 */
#define	BIT_TIME	TIME_NS(200)

/** @brief DIABLO 44 possible sector words */
#define	SECTOR_WORDS	(int)(SECTOR_TIME / BIT_TIME / 32)

/** @brief pulse width of sector mark before the next sector begins */
#define	SECTOR_MARK_PULSE_PRE	(16 * BIT_TIME)

/** @brief pulse width of sector mark after the next sector began */
#define	SECTOR_MARK_PULSE_POST	(16 * BIT_TIME)
#endif


/** @brief end of the guard zone at the beginning of a sector (wild guess!) */
#define	GUARD_ZONE_BITS	(16*32)

static const uint32_t setbit[32] = {
	 (1ul<< 0), (1ul<< 1), (1ul<< 2), (1ul<< 3), (1ul<< 4), (1ul<< 5), (1ul<< 6), (1ul<< 7),
	 (1ul<< 8), (1ul<< 9), (1ul<<10), (1ul<<11), (1ul<<12), (1ul<<13), (1ul<<14), (1ul<<15),
	 (1ul<<16), (1ul<<17), (1ul<<18), (1ul<<19), (1ul<<20), (1ul<<21), (1ul<<22), (1ul<<23),
	 (1ul<<24), (1ul<<25), (1ul<<26), (1ul<<27), (1ul<<28), (1ul<<29), (1ul<<30), (1ul<<31)
};

static const uint32_t clrbit[32] = {
	~(1ul<< 0),~(1ul<< 1),~(1ul<< 2),~(1ul<< 3),~(1ul<< 4),~(1ul<< 5),~(1ul<< 6),~(1ul<< 7),
	~(1ul<< 8),~(1ul<< 9),~(1ul<<10),~(1ul<<11),~(1ul<<12),~(1ul<<13),~(1ul<<14),~(1ul<<15),
	~(1ul<<16),~(1ul<<17),~(1ul<<18),~(1ul<<19),~(1ul<<20),~(1ul<<21),~(1ul<<22),~(1ul<<23),
	~(1ul<<24),~(1ul<<25),~(1ul<<26),~(1ul<<27),~(1ul<<28),~(1ul<<29),~(1ul<<30),~(1ul<<31)
};

/** @brief write a bit into an array of unit32_t */
#define	WRBIT(bits,dst,bit) do { \
	if (bit) { \
		bits[(dst)>>5] |= setbit[(dst)&31]; \
	} else { \
		bits[(dst)>>5] &= clrbit[(dst)&31]; \
	} \
} while (0)

/** @brief read a bit from an array of unit32_t */
#define	RDBIT(bits,src) ((bits[(src)>>5] &setbit[(src)&31]) ? 1 : 0)

/**
 * @brief format of the cooked disk image sectors, i.e. pure data
 *
 * The available images are a multiple of 267 words per sector,
 * i.e. 1 page number, 2 header, 8 label, and 256 data words.
 */
typedef struct {
	/** @brief sector page number */
	uint8_t pageno[2*PAGENO_WORDS];

	/** @brief sector header words */
	uint8_t header[2*HEADER_WORDS];

	/** @brief sector label words */
	uint8_t label[2*LABEL_WORDS];

	/** @brief sector data words */
	uint8_t data[2*DATA_WORDS];
}	sector_t;


/** @brief callback to call at the start of each sector */
static void (*sector_callback)(int);

/**
 * @brief Structure of the disk drive context (2 drives or packs per system)
 */
typedef struct {
	/** @brief disk image, made up of 203 x 2 x 12 sectors */
	sector_t *image;

	/** @brief drive unit number (0 or 1) */
	int unit;

	/** @brief description of the drive(s) */
	char description[32];

	/** @brief basename of the drive image */
	char basename[80];

	/** @brief number of packs in drive (1 or 2) */
	int packs;

	/** @brief rotation time */
	ntime_t	rotation_time;

	/** @brief per sector time (derived: rotation_time / DRIVE_SPT) */
	ntime_t	sector_time;

	/** @brief bit time in nano seconds */
	ntime_t	bit_time;

	/** @brief drive seek/read/write signal (active 0) */
	int s_r_w_0;

	/** @brief drive ready signal (active 0) */
	int ready_0;

	/** @brief sector mark (0 if new sector) */
	int sector_mark_0;

	/** @brief address acknowledge, i.e. seek successful (active 0) */
	int addx_acknowledge_0;

	/** @brief log address interlock, i.e. seek in progress (active 0) */
	int log_addx_interlock_0;

	/** @brief seek incomplete, i.e. seek in progress (active 0) */
	int seek_incomplete_0;

	/** @brief erase gate */
	int egate_0;

	/** @brief write gate */
	int wrgate_0;

	/** @brief read gate */
	int rdgate_0;

	/** @brief current cylinder number */
	int cylinder;

	/** @brief current head (track) number on cylinder */
	int head;

	/** @brief current sector number in track */
	int sector;

	/** @brief sectors expanded to bits */
	uint32_t *bits[DRIVE_CYLINDERS * DRIVE_HEADS * DRIVE_SPT];

	/** @brief current page = (cylinder * HEADS + head) * SPT + sector */
	int page;

	/** @brief set to first bit of a sector that is read from */
	int rdfirst;

	/** @brief set to last bit of a sector that was read from */
	int rdlast;

	/** @brief set to non-zero if a sector is written to */
	int wrfirst;

	/** @brief set to last bit of a sector that was written to */
	int wrlast;
}	drive_t;

/** @brief selected unit numer */
static int selected;

/** @brief timer id for drive_sector_mark() and drive_next_sector() */
static int timer_id;

/** @brief dump raw image at exit */
static int dump_raw;

static void sector_mark_0(int id, int arg);
static void sector_mark_1(int id, int arg);

#if	DIABLO31
static drive_t drive[DRIVE_MAX] = {
	{
		NULL,
		0,
		"DIABLO31", "",
		1,
		ROTATION_TIME,
		SECTOR_TIME,
		BIT_TIME,
		0,
	},{
		NULL,
		1,
		"DIABLO31", "",
		1,
		ROTATION_TIME,
		SECTOR_TIME,
		BIT_TIME,
		0,
	}
};
#else
static drive_t drive[DRIVE_MAX] = {
	{
		NULL,
		0,
		"DIABLO44", "",
		1,
		ROTATION_TIME,
		SECTOR_TIME,
		BIT_TIME,
		0,
	},{
		NULL,
		1,
		"DIABLO44", "",
		1,
		ROTATION_TIME,
		SECTOR_TIME,
		BIT_TIME,
		0,
	}
};
#endif

/** 
 * @brief calculate the sector from the logical block address
 *
 * Modifies drive's page by calculating the logical
 * block address from cylinder, head, and sector.
 *
 * @param unit unit number
 */
static void drive_get_sector(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_get_sector()\n", unit);

	/* If there's no image, just reset the page number */
	if (!d->image) {
		d->page = -1;
		return;
	}
	if (d->cylinder < 0 || d->cylinder >= DRIVE_CYLINDERS) {
		LOG((log_DRV,9,"	DRIVE C/H/S:%d/%d/%d => invalid cylinder\n",
			d->cylinder, d->head, d->sector));
		d->page = -1;
		return;
	}
	/* calculate the new disk relative sector offset */
	d->page = DRIVE_PAGE(d->cylinder, d->head, d->sector);
	LOG((log_DRV,9,"	DRIVE C/H/S:%d/%d/%d => page:%d\n",
		d->cylinder, d->head, d->sector, d->page));
}

/** 
 * @brief compute the checksum of a record
 *
 * @param src pointer to a record (header, label, data)
 * @param size size of the record in bytes
 * @param start start value for the checksum
 * @result returns the checksum of the record
 */
static int cksum(uint8_t *src, size_t size, int start)
{
	size_t offs;
	int sum = start;
	int word;

	/* compute XOR of all words */
	for (offs = 0; offs < size; offs += 2) {
		word = src[size - 2 - offs] + 256 * src[size - 2 - offs + 1];
		sum ^= word;
	}
	return sum;
}

/** 
 * @brief expand a series of clock bits and 0 data bits
 *
 * @param bits pointer to the sector bits
 * @param dst destination offset into bits (bit number)
 * @param size number of words to write
 * @result pointer to next destination bit
 */
static size_t expand_zeroes(uint32_t *bits, size_t dst, size_t size)
{
	size_t offs;

	for (offs = 0; offs < 32 * size; offs += 2) {
		/* write the clock bit */
		WRBIT(bits, dst, 1);
		dst++;
		/* write the 0 data bit */
		WRBIT(bits, dst, 0);
		dst++;
	}

	return dst;
}

/** 
 * @brief expand a series of 0 words and write a final sync bit
 *
 * @param bits pointer to the sector bits
 * @param dst destination offset into bits (bit number)
 * @param size number of words to write
 * @result pointer to next destination bit
 */
static size_t expand_sync(uint32_t *bits, size_t dst, size_t size)
{
	size_t offs;

	for (offs = 0; offs < 32 * size - 2; offs += 2) {
		/* write the clock bit */
		WRBIT(bits, dst, 1);
		dst++;
		/* write the 0 data bit */
		WRBIT(bits, dst, 0);
		dst++;
	}
	/* write the final clock bit */
	WRBIT(bits, dst, 1);
	dst++;
	/* write the 1 data bit */
	WRBIT(bits, dst, 1);
	dst++;

	return dst;
}

/** 
 * @brief expand a record of words into a array of bits at dst
 *
 * @param bits pointer to the sector bits
 * @param dst destination offset into bits (bit number)
 * @param field pointer to the record data (bytes)
 * @param size size of the record in bytes
 * @result pointer to next destination bit
 */
static size_t expand_record(uint32_t *bits, size_t dst, uint8_t *field, size_t size)
{
	size_t offs, bit;

	for (offs = 0; offs < size; offs += 2) {
		int word = field[size - 2 - offs] + 256 * field[size - 2 - offs + 1];
		for (bit = 0; bit < 16; bit++) {
			/* write the clock bit */
			WRBIT(bits, dst, 1);
			dst++;
			/* write the data bit */
			WRBIT(bits, dst, (word >> 15) & 1);
			dst++;
			word <<= 1;
		}
	}
	return dst;
}

/** 
 * @brief Expand a record's checksum word to 32 bits
 *
 * @param bits pointer to the sector bits
 * @param dst destination offset into bits (bit number)
 * @param field pointer to the record data (bytes)
 * @param size size of the record in bytes
 * @result pointer to next destination bit
 */
static size_t expand_cksum(uint32_t *bits, size_t dst, uint8_t *field, size_t size)
{
	size_t bit;
	int word = cksum(field, size, 0521);

	for (bit = 0; bit < 32; bit += 2) {
		/* write the clock bit */
		WRBIT(bits, dst, 1);
		dst++;
		/* write the data bit */
		WRBIT(bits, dst, (word >> 15) & 1);
		dst++;
		word <<= 1;
	}
	return dst;
}

/** 
 * @brief Expand a sector into an array of clock and data bits
 *
 * @param unit drive unit number (0 or 1)
 * @param page page number (0 to DRIVE_PAGES-1)
 */
static void expand_sector(int unit, int page)
{
	drive_t *d = &drive[unit];
	sector_t *s;
	uint32_t *bits;
	size_t dst;

	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to expand_sector()\n", unit);

	if (page < 0 || page >= DRIVE_PAGES)
		return;

	/* already expanded this sector? */
	if (d->bits[page])
		return;

	if (-1 == page || !d->image) {
		LOG((log_DRV,0,"	no sector for #%d: %d/%d/%d\n",
			d->unit, d->cylinder, d->head, d->sector));
		return;
	}

	/* get the sector pointer */
	s = &d->image[page];

	/* allocate a bits image */
	bits = (uint32_t *)calloc(400, sizeof(uint32_t));
	if (!bits) {
		fatal(1, "failed to malloc(%d) bytes bits for drive #%d page #%d\n",
			sizeof(bits), unit, page);
	}

#if	DIABLO31
	/* write sync bit after 31 words - 1 bit */
	dst = expand_sync(bits, 0, 31);
	dst = expand_record(bits, dst, s->header, sizeof(s->header));
	dst = expand_cksum(bits, dst, s->header, sizeof(s->header));

	/* write sync bit after 2 * 5 + 1 words - 1 bit */
	dst = expand_sync(bits, dst, 2 * MWPAL);
	dst = expand_record(bits, dst, s->label, sizeof(s->label));
	dst = expand_cksum(bits, dst, s->label, sizeof(s->label));

	/* write sync bit after 2 * 5 + 1 words - 1 bit */
	dst = expand_sync(bits, dst, 2 * MWPAL);
	dst = expand_record(bits, dst, s->data, sizeof(s->data));
	dst = expand_cksum(bits, dst, s->data, sizeof(s->data));

	/* fill MWPAL words of clock and 0 data bits */
	dst = expand_zeroes(bits, dst, MWPAL);
#else
	/* write sync bit after 31 words - 1 bit */
	dst = expand_sync(bits, 0, 31);
	dst = expand_record(bits, dst, s->header, sizeof(s->header));
	dst = expand_cksum(bits, dst, s->header, sizeof(s->header));

	/* write sync bit after 2 * 5 words - 1 bit */
	dst = expand_sync(bits, dst, 2 * MWPAL);
	dst = expand_record(bits, dst, s->label, sizeof(s->label));
	dst = expand_cksum(bits, dst, s->label, sizeof(s->label));

	/* write sync bit after 2 * 5 words - 1 bit */
	dst = expand_sync(bits, dst, 2 * MWPAL);
	dst = expand_record(bits, dst, s->data, sizeof(s->data));
	dst = expand_cksum(bits, dst, s->data, sizeof(s->data));

	/* fill MWPAL words of clock and 0 data bits */
	dst = expand_zeroes(bits, dst, MWPAL);
#endif
	d->bits[page] = bits;

	LOG((log_DRV,0,"	BITS #%d: %03d/%d/%02d #%-5d bits (@%03d.%02d)\n",
		d->unit, d->cylinder, d->head, d->sector,
		dst, dst / 32, dst % 32));

}

#if	DEBUG
static void drive_dump_ascii(uint8_t *src, size_t size)
{
	size_t offs;
	LOG((log_DRV,0," ["));
	for (offs = 0; offs < size; offs++) {
		char ch = (char)src[offs ^ 1];
		LOG((log_DRV,0, "%c", ch < 32 || ch > 126 ? '.' : ch));
	}
	LOG((log_DRV,0,"]\n"));
}


/** 
 * @brief Dump a record's contents
 *
 * @param src pointer to a record (header, label, data)
 * @param size size of the record in bytes
 * @param name name to print before the dump
 */
static size_t dump_record(uint8_t *src, size_t addr, size_t size, const char *name, int cr)
{
	size_t offs;
	LOG((log_DRV,0,"%s:", name));
	for (offs = 0; offs < size; offs += 2) {
		int word = src[offs] + 256 * src[offs + 1];
		if (offs % 16) {
			LOG((log_DRV,0," %06o", word));
		} else {
			if (offs > 0)
				drive_dump_ascii(&src[offs-16], 16);
			LOG((log_DRV,0,"\t%05o: %06o", (addr + offs) / 2, word));
		}
	}
	if (offs % 16) {
		drive_dump_ascii(&src[offs - (offs % 16)], offs % 16);
	} else {
		drive_dump_ascii(&src[offs-16], 16);
	}
	if (cr) {
		LOG((log_DRV,0,"\n"));
	}
	return size;
}
#endif

/** 
 * @brief find a sync bit in an array of clock and data bits
 *
 * @param bits pointer to the sector's bits
 * @param src source offset into bits (bit number)
 * @param size number of words to scan for a sync word
 * @result next src pointer
 */
static size_t squeeze_sync(uint32_t *bits, size_t src, size_t size)
{
	size_t offs, bitcount;
	uint32_t accu = 0;
	
	/* hunt for the first 0x0001 word */
	for (bitcount = 0, offs = 0; offs < size; /* */) {
		/*
		 * accumulate clock and data bits until we are
		 * on the clock bit boundary
		 */
		accu = (accu << 1) | RDBIT(bits,src);
		src++;
		/*
		 * look for 15 alternating clocks and 0-bits
		 * and the 16th clock with a 1-bit
		 */
		if (accu == 0xaaaaaaab)
			return src;
		if (++bitcount == 32) {
			bitcount = 0;
			offs++;
		}
	}
	/* return if no sync found within size*32 clock and data bits */
	LOG((log_DRV,0,"	no sync within %d words\n", size));
	return src;
}

/** 
 * @brief find a 0 bit sequence in an array of clock and data bits
 *
 * @param bits pointer to the sector's bits
 * @param src source offset into bits (bit number)
 * @param size number of words to scan for a sync word
 * @result next src pointer
 */
static size_t squeeze_unsync(uint32_t *bits, size_t src, size_t size)
{
	size_t offs, bitcount;
	uint32_t accu = 0;
	
	/* hunt for the first 0x0000 word */
	for (bitcount = 0, offs = 0; offs < size; /* */) {
		/*
		 * accumulate clock and data bits until we are
		 * on the clock bit boundary
		 */
		accu = (accu << 1) | RDBIT(bits,src);
		src++;
		/*
		 * look for 15 alternating clocks and 0-bits
		 * and the 16th clock with a 1-bit
		 */
		if (accu == 0xaaaaaaaa)
			return src;
		if (++bitcount == 32) {
			bitcount = 0;
			offs++;
		}
	}
	/* return if no sync found within size*32 clock and data bits */
	LOG((log_DRV,0,"	no unsync within %d words\n", size));
	return src;
}

/** 
 * @brief squeeze an array of clock and data bits into a sector's record
 *
 * @param bits pointer to the sector's bits
 * @param src source offset into bits (bit number)
 * @param field pointer to the record data (bytes)
 * @param size size of the record in bytes
 * @result next src pointer
 */
static size_t squeeze_record(uint32_t *bits, size_t src, uint8_t *field, size_t size)
{
	size_t offs, bitcount;
	uint32_t accu = 0;

	
	for (bitcount = 0, offs = 0; offs < size; /* */) {
		/* skip clock */
		src++;
		/* get data bit */
		accu = (accu << 1) | RDBIT(bits,src);
		src++;
		bitcount += 2;
		if (bitcount == 32) {
			/* collected a word */
			field[size - 2 - offs + 0] = accu % 256;
			field[size - 2 - offs + 1] = accu / 256;
			offs += 2;
			bitcount = 0;
		}
	}
	return src;
}

/** 
 * @brief squeeze an array of 32 clock and data bits into a checksum word
 *
 * @param bits pointer to the sector's bits
 * @param src source offset into bits (bit number)
 * @param cksum pointer to an int to receive the checksum word
 * @result next src pointer
 */
static size_t squeeze_cksum(uint32_t *bits, size_t src, int *cksum)
{
	size_t bitcount;
	uint32_t accu = 0;

	
	for (bitcount = 0; bitcount < 32; bitcount += 2) {
		/* skip clock */
		src++;
		/* get data bit */
		accu = (accu << 1) | RDBIT(bits,src);
		src++;
	}

	/* set the cksum to the extracted word */
	*cksum = accu;
	return src;
}

/** 
 * @brief squeeze a array of clock and data bits into a sector's data
 */
static void squeeze_sector(int unit)
{
	drive_t *d = &drive[unit];
	sector_t *s;
	uint32_t *bits;
	size_t src;
	int cksum_header, cksum_label, cksum_data;

	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to squeeze_sector()\n", unit);

	if (d->rdfirst >= 0) {
		LOG((log_DRV,0,
			"	RD #%d %03d/%d/%02d bit#%-5d (@%03d.%02d) ... bit#%-5d (@%03d.%02d)\n",
			d->unit, d->cylinder, d->head, d->sector,
			d->rdfirst, d->rdfirst / 32, d->rdfirst % 32,
			d->rdlast, d->rdlast / 32, d->rdlast % 32));
	}
	d->rdfirst = -1;
	d->rdlast = -1;

	/* not written to, just drop it now */
	if (d->wrfirst < 0) {
		d->wrfirst = -1;
		d->wrlast = -1;
		return;
	}

	/* did write into the next sector (?) */
	if (d->wrlast > d->wrfirst && d->wrlast < 256) {
		d->wrfirst = -1;
		d->wrlast = -1;
		return;
	}

	if (d->wrfirst >= 0) {
		LOG((log_DRV,0,
			"	WR #%d %03d/%d/%02d bit#%-5d (@%03d.%02d) ... bit#%-5d (@%03d.%02d)\n",
			d->unit, d->cylinder, d->head, d->sector,
			d->wrfirst, d->wrfirst / 32, d->wrfirst % 32,
			d->wrlast, d->wrlast / 32, d->wrlast % 32));
	}
	d->wrfirst = -1;
	d->wrlast = -1;

	if (d->page < 0 || d->page >= DRIVE_PAGES) {
		LOG((log_DRV,0,"	no sector for #%d: %d/%d/%d\n",
			d->unit, d->cylinder, d->head, d->sector));
		return;
	}

	/* get the sector pointer */
	s = &d->image[d->page];

	/* get the bits pointer */
	bits = d->bits[d->page];

	/* no bits to write? */
	if (!bits) {
		LOG((log_DRV,0,"	no sector for #%d: %d/%d/%d\n",
			d->unit, d->cylinder, d->head, d->sector));
		return;
	}

	/* zap the sector first */
	memset(s->header, 0, sizeof(s->header));
	memset(s->label, 0, sizeof(s->label));
	memset(s->data, 0, sizeof(s->data));

	src = MFRRDL * 32;
	/* skip first words and garbage until 0 bits are coming in */
	src = squeeze_unsync(bits, src, 40);
	/* sync on header preamble */
	src = squeeze_sync(bits, src, 40);
	LOG((log_DRV,0,"	header sync bit #%d (@%03d.%02d)\n",
		src, src / 32, src % 32));
	src = squeeze_record(bits, src, s->header, sizeof(s->header));
	src = squeeze_cksum(bits, src, &cksum_header);
#if	DEBUG
	dump_record(s->header, 0, sizeof(s->header), "header", 0);
#endif

	/* skip garbage until 0 bits are coming in */
	src = squeeze_unsync(bits, src, 40);
	/* sync on label preamble */
	src = squeeze_sync(bits, src, 40);
	LOG((log_DRV,0,"	label  sync bit #%d (@%03d.%02d)\n",
		src, src / 32, src % 32));
	src = squeeze_record(bits, src, s->label, sizeof(s->label));
	src = squeeze_cksum(bits, src, &cksum_label);
#if	DEBUG
	dump_record(s->label, 0, sizeof(s->label), "label", 0);
#endif

	/* skip garbage until 0 bits are coming in */
	src = squeeze_unsync(bits, src, 40);
	/* sync on data preamble */
	src = squeeze_sync(bits, src, 40);
	LOG((log_DRV,0,"	data   sync bit #%d (@%03d.%02d)\n",
		src, src / 32, src % 32));
	src = squeeze_record(bits, src, s->data, sizeof(s->data));
	src = squeeze_cksum(bits, src, &cksum_data);
#if	DEBUG
	dump_record(s->data, 0, sizeof(s->data), "data", 1);
#endif

	/* TODO: what is the cksum start value for the data record? */
	cksum_header ^= cksum(s->header, sizeof(s->header), 0521);
	cksum_label ^= cksum(s->label, sizeof(s->label), 0521);
	cksum_data ^= cksum(s->data, sizeof(s->data), 0521);

	if (cksum_header || cksum_label || cksum_data) {
#if	DEBUG
		LOG((log_DRV,0,"	cksum check - header:%06o label:%06o data:%06o\n",
			cksum_header, cksum_label, cksum_data));
#else
		printf("	cksum check - header:%06o label:%06o data:%06o\n",
			cksum_header, cksum_label, cksum_data);
#endif
	}
}

/** 
 * @brief return number of bitclk edges for a sector
 *
 * @result number of bitclk edges for a sector
 */
int drive_bits_per_sector(void)
{
	return SECTOR_WORDS * 32;
}

/** 
 * @brief return a pointer to a drive's description
 *
 * @param unit unit number
 * @result a pointer to the string description
 */
const char *drive_description(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_description()\n", unit);
	return d->description;
}

/** 
 * @brief return a pointer to a drive's image basename
 *
 * @param unit unit number
 * @result a pointer to the string basename
 */
const char *drive_basename(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_description()\n", unit);
	return d->basename;
}

/** 
 * @brief return the number of a drive unit
 *
 * This is usually a no-op, but drives may be swapped?
 *
 * @param unit unit number
 * @result the unit number
 */
int drive_unit(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_unit()\n", unit);
	return d->unit;
}

/** 
 * @brief return the time for a full rotation
 *
 * @param unit unit number
 * @result the time for a full track rotation
 */
ntime_t drive_rotation_time(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_rotation_time()\n", unit);
	return d->rotation_time;
}

/** 
 * @brief return the time for a data bit
 *
 * @param unit unit number
 * @result the time in nano seconds per bit clock
 */
ntime_t drive_bit_time(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_bit_time()\n", unit);
	return d->bit_time;
}

/** 
 * @brief return the seek/read/write status of a drive
 *
 * @param unit unit number
 * @result the seek/read/write status for the drive unit (0: active, 1: inactive)
 */
int drive_seek_read_write_0(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_seek_read_write_0()\n", unit);
	return d->s_r_w_0;
}

/** 
 * @brief return the ready status of a drive
 *
 * @param unit unit number
 * @result the ready status for the drive unit
 */
int drive_ready_0(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_ready_0()\n", unit);
	return d->ready_0;
}

/** 
 * @brief return the current sector mark status of a drive unit
 *
 * The sector mark is derived from the offset into the current sector.
 *
 * @param unit unit number
 * @result the current sector for the drive unit
 */
int drive_sector_mark_0(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_sector_mark_0()\n", unit);
	/* no sector marks while seeking (?) */
	if (d->s_r_w_0)
		return 1;
	/* return the sector mark */
	return d->sector_mark_0;
}

/** 
 * @brief return the address acknowledge state
 *
 * @param unit unit number
 * @result the address acknowledge state
 */
int drive_addx_acknowledge_0(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_addx_acknowledge_0()\n", unit);
	return d->addx_acknowledge_0;
}

/** 
 * @brief return the log address interlock state
 *
 * @param unit unit number
 * @result the log address interlock state
 */
int drive_log_addx_interlock_0(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_log_addx_interlock_0()\n", unit);
	return d->log_addx_interlock_0;
}

/** 
 * @brief return the seek incomplete state
 *
 * @param unit unit number
 * @result the address acknowledge state
 */
int drive_seek_incomplete_0(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_addx_acknowledge_0()\n", unit);
	return d->seek_incomplete_0;
}

/** 
 * @brief return the current cylinder of a drive unit
 *
 * Note: the bus lines are active low, thus the XOR with DRIVE_CYLINDER_MASK.
 *
 * @param unit unit number
 * @result the current cylinder number for the drive unit
 */
int drive_cylinder(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_cylinder()\n", unit);
	return d->cylinder ^ DRIVE_CYLINDER_MASK;
}

/** 
 * @brief return the current head of a drive unit
 *
 * Note: the bus lines are active low, thus the XOR with DRIVE_HEAD_MASK.
 *
 * @param unit unit number
 * @result the currently selected head for the drive unit
 */
int drive_head(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_head()\n", unit);
	return d->head ^ DRIVE_HEAD_MASK;
}

/** 
 * @brief return the current sector of a drive unit
 *
 * The current sector number is derived from the time since the
 * most recent track rotation started.
 *
 * Note: the bus lines are active low, thus the XOR with DRIVE_SECTOR_MASK.
 *
 * @param unit unit number
 * @result the current sector for the drive unit
 */
int drive_sector(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_sector()\n", unit);
	return d->sector ^ DRIVE_SECTOR_MASK;
}

/** 
 * @brief return the current page of a drive unit
 *
 * The current page number is derived from the cylinder, head,
 * and sector numbers.
 *
 * @param unit unit number
 * @result the current page for the drive unit
 */
int drive_page(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_page()\n", unit);
	return d->page;
}

/** 
 * @brief select a drive unit and head
 *
 * @param unit unit number
 * @param head head number
 */
void drive_select(int unit, int head)
{
	drive_t *d = &drive[unit];

	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_select()\n", unit);

	if (unit != selected) {
		/* this drive is selected */
		selected = unit;
		printf("select unit:%d head:%d\n", unit, head);
	}

	if (d->image) {
		/* it is ready */
		d->ready_0 = 0;
		/* and can take seek/read/write commands */
		d->s_r_w_0 = 0;
		/* address acknowledge (?) */
		d->addx_acknowledge_0 = 0;
		/* clear log address interlock (?) */
		d->log_addx_interlock_0 = 1;
		LOG((log_DRV,1,"	UNIT select %d ready\n", unit));
	} else {
		/* it is not ready (?) */
		d->ready_0 = 1;
		/* can't take seek/read/write commands (?) */
		d->s_r_w_0 = 1;
		/* address acknowledge (?) */
		d->addx_acknowledge_0 = 0;
		LOG((log_DRV,1,"	UNIT select %d not ready (no image)\n", unit));
	}

	/* Note: head select input is active low (0: selects head 1, 1: selects head 0) */
	head = head & DRIVE_HEAD_MASK;
	if (head != d->head) {
		d->head = head;
		LOG((log_DRV,1,"	HEAD %d select on unit %d\n",
			head, unit));
	}
	drive_get_sector(unit);
}

/**
 * @brief strobe a seek operation
 *
 * Seek to the cylinder cylinder, or restore.
 *
 * @param unit unit number
 * @param cylinder cylinder number to seek to
 * @param restore flag if the drive should restore to cylinder 0
 * @param strobe current level of the strobe signal (for edge detection)
 */
void drive_strobe(int unit, int cylinder, int restore, int strobe)
{
	drive_t *d = &drive[unit];
	int seekto = restore ? 0 : cylinder;

	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_strobe()\n", unit);

	if (strobe == 1) {
		LOG((log_DRV,1,"	STROBE end of interlock\n", seekto));
		/* deassert the log address interlock */
		d->log_addx_interlock_0 = 1;
		return;
	}

	/* assert the log address interlock */
	d->log_addx_interlock_0 = 0;

	if (seekto == d->cylinder) {
		LOG((log_DRV,1,"	STROBE to cylinder %d acknowledge\n", seekto));
		d->addx_acknowledge_0 = 0;	/* address acknowledge, if cylinder is reached */
		d->seek_incomplete_0 = 1;	/* reset seek incomplete */
		return;
	}

	d->s_r_w_0 = 0;

	if (seekto < d->cylinder) {
		/* decrement cylinder */
		d->cylinder -= 1;
		if (d->cylinder < 0) {
			d->cylinder = 0;
			d->log_addx_interlock_0 = 1;	/* deassert the log address interlock */
			d->seek_incomplete_0 = 1;	/* deassert seek incomplete */
			d->addx_acknowledge_0 = 0;	/* assert address acknowledge  */
			LOG((log_DRV,1,"	STROBE to cylinder %d incomplete\n", seekto));
			return;
		}
	} else {
		/* increment cylinder */
		d->cylinder += 1;
		if (d->cylinder >= DRIVE_CYLINDERS) {
			d->cylinder = DRIVE_CYLINDERS - 1;
			d->log_addx_interlock_0 = 1;	/* deassert the log address interlock */
			d->seek_incomplete_0 = 1;	/* deassert seek incomplete */
			d->addx_acknowledge_0 = 0;	/* assert address acknowledge  */
			LOG((log_DRV,1,"	STROBE to cylinder %d incomplete\n", seekto));
			return;
		}
	}
	LOG((log_DRV,1,"	STROBE to cylinder %d (now %d) - interlock\n",
		seekto, d->cylinder));

	d->addx_acknowledge_0 = 1;	/* deassert address acknowledge  */
	d->seek_incomplete_0 = 1;	/* deassert seek incomplete */
	drive_get_sector(unit);
}

/** 
 * @brief install a callback to be called whenever a drive sector starts
 *
 * @param callback function to call when a new sector begins
 * @result returns the previous callback
 */
void *drive_sector_callback(void (*callback)(int))
{
	void *previous_callback = sector_callback;
	sector_callback = callback;
	return previous_callback;
}


/** 
 * @brief set the drive erase gate
 *
 * @param unit drive unit number
 * @param gate value of erase gate
 */
void drive_egate(int unit, int gate)
{
	drive_t *d = &drive[unit];
	d->egate_0 = gate;
}

/** 
 * @brief set the drive write gate
 *
 * @param unit drive unit number
 * @param gate value of write gate
 */
void drive_wrgate(int unit, int gate)
{
	drive_t *d = &drive[unit];
	d->wrgate_0 = gate;
}

/** 
 * @brief set the drive read gate
 *
 * @param unit drive unit number
 * @param gate value of read gate
 */
void drive_rdgate(int unit, int gate)
{
	drive_t *d = &drive[unit];
	d->rdgate_0 = gate;
}

/** 
 * @brief write the sector relative bit at index
 *
 * The disk controller writes a combined clock and data pulse to one output
 * <PRE>
 * Encoding of binary 01011
 *
 *   clk   data  clk   data  clk   data  clk   data  clk   data
 *   0     1     2     3     4     5     6     7     8     9
 *   +--+        +--+  +--+  +--+        +--+  +--+  +--+  +--+  +--
 *   |  |        |  |  |  |  |  |        |  |  |  |  |  |  |  |  |
 * --+  +--------+  +--+  +--+  +--------+  +--+  +--+  +--+  +--+
 * </PRE>
 *
 * @param unit drive unit number
 * @param index relative index of bit/clock into sector
 * @param wrdata write data clock or bit
 */
void drive_wrdata(int unit, int index, int wrdata)
{
	drive_t *d = &drive[unit];
	uint32_t *bits;

	if (d->wrgate_0) {
		/* write gate is not asserted (active 0) */
		return;
	}

	/* don't write before or beyond the sector */
	if (index < 0 || index >= drive_bits_per_sector())
		return;

	if (-1 == d->page) {
		/* invalid page */
		return;
	}

	bits = d->bits[d->page];
	if (!bits) {
		/* expand the sector to bits */
		expand_sector(unit, d->page);
		bits = d->bits[d->page];
		/* just to be safe */
		if (!bits)
			fatal(1, "No bits for drive #%d page #%d\n", unit, d->page);
	}
	if (-1 == d->wrfirst)
		d->wrfirst = index;

	LOG((log_DRV,7,
		"	write #%d %d/%d/%d bit #%d bit:%d\n",
		unit, d->cylinder, d->head, d->sector,
		index, wrdata));

	if (index < GUARD_ZONE_BITS) {
		/* don't write in the guard zone (?) */
	} else {
		WRBIT(bits,index,wrdata);
	}
	d->wrlast = index;
}

/** 
 * @brief get the sector relative bit at index
 *
 * Note: this is a gross hack to allow the controller pulling bits
 * at its will, rather than clocking them with the drive's RDCLK-
 *
 * @param unit is the drive index
 * @param index is the sector relative bit index
 * @result returns the sector's bit by index
 */
int drive_rddata(int unit, int index)
{
	drive_t *d = &drive[unit];
	uint32_t *bits;
	int bit = 0;

	if (d->rdgate_0) {
		/* read gate is not asserted (active 0) */
		return 0;
	}

	/* don't read before or beyond the sector */
	if (index < 0 || index >= drive_bits_per_sector())
		return 1;

	/* no data while sector mark is low (?) */
	if (0 == d->sector_mark_0)
		return 1;

	if (-1 == d->page) {
		/* invalid page */
		return 1;
	}

	bits = d->bits[d->page];
	if (!bits) {
		/* expand the sector to bits */
		expand_sector(unit, d->page);
		bits = d->bits[d->page];
		/* just to be safe */
		if (!bits)
			fatal(1, "No bits for drive #%d page #%d\n", unit, d->page);
	}
	if (-1 == d->rdfirst)
		d->rdfirst = index;

	bit = RDBIT(bits,index);
	LOG((log_DRV,7,
		"	read #%d %d/%d/%d bit #%d:%d\n",
		unit, d->cylinder, d->head, d->sector,
		index, bit));
	d->rdlast = index;
	return bit;
}

/** 
 * @brief get the sector relative clock at index
 *
 * Note: this is a gross hack to allow the controller pulling bits
 * at its will, rather than clocking them with the drive's RDCLK-
 *
 * @param unit is the drive index
 * @param index is the sector relative bit index
 * @result returns the sector's bit by index
 */
int drive_rdclk(int unit, int index)
{
	drive_t *d = &drive[unit];
	uint32_t *bits;
	int clk;

	/* don't read before or beyond the sector */
	if (index < 0 || index >= drive_bits_per_sector())
		return 1;

	/* no clock while sector mark is low (?) */
	if (0 == d->sector_mark_0)
		return 1;

	if (-1 == d->page) {
		/* invalid page */
		return 1;
	}

	bits = d->bits[d->page];
	if (!bits) {
		/* expand the sector to bits */
		expand_sector(unit, d->page);
		bits = d->bits[d->page];
		/* just to be safe */
		if (!bits)
			fatal(1, "No bits for drive #%d page #%d\n", unit, d->page);
	}
	if (-1 == d->rdfirst)
		d->rdfirst = index;

	if (index & 1) {
		clk = 0;
	} else {
		clk = RDBIT(bits,index);
	}

	LOG((log_DRV,7,
		"	read #%d %d/%d/%d clk #%d:%d\n",
		unit, d->cylinder, d->head, d->sector,
		index, clk));

	d->rdlast = index;
	return clk ^ 1;
}

/** 
 * @brief debug function to read sync bit position from a sector
 *
 * @param unit is the drive index
 * @param page is the page number to read
 * @param offs is the first bit offset
 * @result returns bit offset after the next sync bit
 */
int debug_read_sync(int unit, int page, int offs)
{
	drive_t *d = &drive[unit];
	uint32_t *bits;
	uint32_t accu;

	if (unit < 0 || unit > 1)
		return 0;

	/* don't read before or beyond the sector */
	if (offs < 0 || offs >= 400*32)
		return 0;

	/* check for invalid page */
	if (page < 0 || page >= DRIVE_CYLINDERS * DRIVE_HEADS * DRIVE_SPT)
		return 0;

	bits = d->bits[page];
	if (!bits) {
		/* expand the sector to bits */
		expand_sector(unit, page);
		bits = d->bits[page];
		/* just to be safe */
		if (!bits)
			return 0;
	}

	accu = 0;
	while (offs < 400 * 32) {
		accu = (accu << 1) | RDBIT(bits,offs);
		offs++;
		if (accu == 0xaaaaaaab)
			break;
	}

	return offs;
}

/** 
 * @brief debug function to read bits from a drive sector
 *
 * @param unit is the drive index
 * @param page is the page number to read
 * @param offs is the first bit offset
 * @result returns a word starting at the bit offset
 */
int debug_read_sec(int unit, int page, int offs)
{
	drive_t *d = &drive[unit];
	uint32_t *bits;
	int i, clks, word;

	if (unit < 0 || unit > 1)
		return 0177777;

	/* don't read before or beyond the sector */
	if (offs < 0 || offs >= 400*32)
		return 0177777;

	/* check for invalid page */
	if (page < 0 || page >= DRIVE_CYLINDERS * DRIVE_HEADS * DRIVE_SPT)
		return 0177777;

	bits = d->bits[page];
	if (!bits) {
		/* expand the sector to bits */
		expand_sector(unit, page);
		bits = d->bits[page];
		/* just to be safe */
		if (!bits)
			return 0177777;
	}

	for (i = 0, clks = 0, word = 0; i < 16; i++, offs += 2) {
		clks = (clks << 1) | RDBIT(bits,offs);
		word = (word << 1) | RDBIT(bits,offs+1);
	}

	return word;
}

/** 
 * @brief timer callback that is called once per sector in the rotation
 *
 * @param id timer id
 * @param arg argument supplied to timer_insert (unused)
 */
static void drive_next_sector(int id, int arg)
{
	int unit = selected;
	drive_t *d = &drive[unit];

	LOG((log_DRV,5,
		"	next sector (unit #%d sector %d)\n",
		unit, d->sector));

	/* deassert sector mark after pulse width */
	timer_id = timer_insert(SECTOR_MARK_PULSE_POST,
		sector_mark_1, unit, "sector mark 1");

	/* assert sector mark again at end of the sector */
	timer_id = timer_insert(d->sector_time - SECTOR_MARK_PULSE_PRE,
		sector_mark_0, unit, "sector mark 0");

	/* call the sector_callback, if any */
	if (sector_callback)
		(*sector_callback)(unit);
}

/** 
 * @brief timer callback that deasserts the sector mark
 *
 * @param id timer id
 * @param arg drive unit number
 */
static void sector_mark_1(int id, int arg)
{
	int unit = arg;
	drive_t *d = &drive[unit];

	LOG((log_DRV,5,
		"	sector mark 1 (unit #%d sector %d)\n",
		unit, d->sector));

	/* set sector mark to 1 */
	d->sector_mark_0 = 1;
}

/** 
 * @brief timer callback that asserts the sector mark
 *
 * @param id timer id
 * @param arg drive unit number
 */
static void sector_mark_0(int id, int arg)
{
	int unit = arg;
	drive_t *d = &drive[unit];

	LOG((log_DRV,5,
		"	sector mark 0 (unit #%d sector %d)\n",
		unit, d->sector));

	/* squeeze previous sector, if it was written to */
	squeeze_sector(unit);

	d->sector_mark_0 = 0;

	/* reset read and write bit locations */
	d->rdfirst = -1;
	d->rdlast = -1;
	d->wrfirst = -1;
	d->wrlast = -1;

	/* count sectors */
	d->sector = (d->sector + 1) % DRIVE_SPT;
	drive_get_sector(unit);

	/* next sector starting soon now */
	timer_id = timer_insert(SECTOR_MARK_PULSE_PRE,
		drive_next_sector, unit, "next sector");
}

/**
 * @brief print usage info for the drive switches
 *
 * @param argc argument count
 * @param argv argument list
 * @result returns 0 (why should it fail?)
 */
int drive_usage(int argc, char **argv)
{
	printf("-dr		dump raw drive 0 image to 'dump.raw' at exit\n");
	return 0;
}

/** 
 * @brief pass down command line arguments to the drive emulation
 *
 * @param arg command line argument
 * @result returns 0 if this was a disk image, -1 on error
 */
int drive_args(const char *arg)
{
	const char *basename;
	int unit;
	int hdr, c, h, s, chs;
	drive_t *d;
	int zcat;
	FILE *fp;
	size_t size;		/* size in sectors */
	size_t done;		/* read loops */
	size_t csize;		/* size of cooked image */
	uint8_t *image;		/* file image (either cooked, or compressed) */
	sector_t *cooked;	/* cooked sector data */
	uint8_t *ip, *cp;
	char *p;

	/* dump raw image at exit? */
	if (!strcmp(arg, "-dr")) {
		dump_raw = 1;
		return 0;
	}

	/* don't care about other switches */
	if (arg[0] == '-' || arg[0] == '+')
		return -1;

	basename = strrchr(arg, '/');
	if (basename) {
		basename++;
	} else {
		basename = strrchr(arg, '\\');
		if (basename)
			basename++;
		else
			basename = arg;
	}

	/* find trailing dot */
	p = strrchr(basename, '.');
	if (!p)
		return -1;

	/* check for .Z extension */
	if (!strcmp(p, ".z") || !strcmp(p, ".Z") ||
		!strcmp(p, ".gz") || !strcmp(p, ".GZ")) {
		/* compress (LZW) or gzip compressed image */
		LOG((log_DRV,0,"loading compressed disk image %s\n", arg));
		zcat = 1;
	} else if (!strcmp(p, ".dsk") || !strcmp(p, ".DSK")) {
		/* uncompressed .dsk extension */
		LOG((log_DRV,0,"loading uncompressed disk image %s\n", arg));
		zcat = 0;
	} else {
		return -1;
	}

	for (unit = 0; unit < DRIVE_MAX; unit++)
		if (!drive[unit].image)
			break;

	/* all drive slots filled? */
	if (unit == DRIVE_MAX)
		return -1;

	d = &drive[unit];

	snprintf(d->basename, sizeof(d->basename), "%s", basename);

	fp = fopen(arg, "rb");
	if (!fp)
		fatal(1, "failed to fopen(%s,\"rb\") (%s)\n",
			arg, strerror(errno));

	/* size in sectors */
	size = DRIVE_CYLINDERS * DRIVE_HEADS * DRIVE_SPT;

	csize = size * sizeof(sector_t);
	image = (uint8_t *)malloc(csize + 1);
	if (!image)
		fatal(1, "failed to malloc(%d) bytes\n", csize);
	ip = (uint8_t *)image;
	for (done = 0; done < size * sizeof(sector_t); /* */) {
		int got = fread(ip + done, 1, size * sizeof(sector_t) - done, fp);
		if (got < 0) {
			LOG((0,0,"fread() returned error (%s)\n",
				strerror(errno)));
			break;
		}
		if (got == 0)
			break;
		done += got;
	}
	fclose(fp);

	LOG((log_DRV,0, "got %d (%#x) bytes\n", done, done));

	cooked = (sector_t *)malloc(csize);
	if (!cooked)
		fatal(1, "failed to malloc(%d) bytes\n", csize);
	cp = (uint8_t *)cooked;

	if (zcat) {
		size_t skip;
		int type = z_header(ip, done, &skip);

		switch (type) {
		case 0:
			LOG((log_MISC,0, "compression type unknown, skip:%d\n", skip));
			csize = gz_copy(cp, csize, ip, done);
			break;
		case 1:
			LOG((log_MISC,0, "compression type compress (LZW), skip:%d\n", skip));
			csize = z_copy(cp, csize, ip + skip, done - skip);
			break;
		case 2:
			LOG((log_MISC,0, "compression type gzip, skip:%d\n", skip));
			csize = gz_copy(cp, csize, ip, done);
			break;
		}
	} else {
		memcpy(cp, ip, done);
		csize = done;
	}
	free(image);
	image = NULL;
	ip = NULL;

	if (csize != size * sizeof(sector_t)) {
		LOG((log_DRV,0,"disk image %s size mismatch (%d bytes)\n", arg, csize));
		free(cooked);
		return -1;
	}

	/* reset cylinder, head, sector counters */
	c = h = s = 0;

	/* check image sectors for sanity */
	for (done = 0; done < size; done++) {
		/* copy the available records in their place */

		/* verify the chs with the header and log any mismatches */
		hdr = cooked->header[2] + 256 * cooked->header[3];
		chs = (s << 12) | (c << 3) | (h << 2) | (unit << 1);
		if (chs != hdr) {
			LOG((log_DRV,0,"WARNING: header mismatch C/H/S: %3d/%d/%2d chs:%06o hdr:%06o\n",
				c, h, s, chs, hdr));
		}
		if (++s == DRIVE_SPT) {
			s = 0;
			if (++h == DRIVE_HEADS) {
				h = 0;
				c++;
			}
		}
		cooked++;
	}

	/* set drive image */
	d->image = (sector_t *)cp;

	LOG((log_DRV,0,"drive #%d successfully created image for %s\n", unit, arg));

	drive_select(unit, 0);

	/* drive address acknowledge is active now */
	d->s_r_w_0 = 0;
	/* drive address acknowledge is active now */
	d->addx_acknowledge_0 = 0;

	LOG((log_DRV,0,"possible %s sector size is %#o (%d) words\n",
		d->description, SECTOR_WORDS, SECTOR_WORDS));

	LOG((log_DRV,0,"sector mark pulse length is %lldns\n",
		SECTOR_MARK_PULSE_PRE + SECTOR_MARK_PULSE_POST));

	return 0;
}

/** 
 * @brief dump the raw image to a file at exit
 */
static void drive_dump(void)
{
	static const char *filename = "disk.raw";
	FILE *fp;
	size_t size = DRIVE_CYLINDERS * DRIVE_HEADS * DRIVE_SPT;

	if (!dump_raw)
		return;

	if (!drive[0].image)
		return;

	fp = fopen(filename, "wb");
	if (!fp)
		fatal(1,"failed to fopen() %s\n", filename);
	if (size != fwrite(drive[0].image, sizeof(sector_t), size, fp))
		fatal(1,"failed to fwrite() %s (%d sectors)\n", filename, size);
	if (fclose(fp))
		fatal(1,"failed to fclose() %s\n", filename);
}

/** 
 * @brief initialize the disk context and insert a disk wort timer
 *
 * @result returns 0 on success, fatal() on error
 */
int drive_init(void)
{
	int i;

	atexit(drive_dump);

	if (sizeof(sector_t) != 267 * 2)
		fatal(1, "sizeof(sector_t) is not %d (%d)\n",
			267 * 2, sizeof(sector_t));

	for (i = 0; i < DRIVE_MAX; i++) {
		drive_t *d = &drive[i];

		d->unit = i;			/* set the unit number */
		d->s_r_w_0 = 1;			/* seek/read/write not ready */
		d->ready_0 = 1;			/* drive is not ready */
		d->sector_mark_0 = 1;		/* sector mark clear */
		d->addx_acknowledge_0 = 1;	/* drive address acknowledge is not active */
		d->log_addx_interlock_0 = 1;	/* drive log address interlock is not active */
		d->seek_incomplete_0 = 1;	/* drive seek incomplete is not active */

		/* reset the disk drive's address */
		d->cylinder = 0;
		d->head = 0;
		d->sector = 0;

		d->egate_0 = 1;
		d->wrgate_0 = 1;
		d->rdgate_0 = 1;

		/* clocks + bits for the expanded sector + some slack */
		d->wrfirst = -1;
		d->wrlast = -1;
		d->rdfirst = -1;
		d->rdlast = -1;
	}

	timer_id = timer_insert(drive[0].sector_time - SECTOR_MARK_PULSE_PRE,
		sector_mark_0, 0, "sector mark 0");
	return 0;
}
