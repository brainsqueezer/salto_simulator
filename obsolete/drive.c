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
 * $Id: drive.c,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "alto.h"
#include "cpu.h"
#include "timer.h"
#include "drive.h"
#include "zcat.h"

#define	DRIVE_ENABLED	1

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

/** @brief DIABLO 31 rotation time is approx. 40ms */
#define	DIABLO31_ROTATION_TIME	((ntime_t)39999900ll)

/** @brief DIABLO 31 sector time */
#define	DIABLO31_SECTOR_TIME	(DIABLO31_ROTATION_TIME/DRIVE_SPT)

/** @brief DIABLO 31 bit clock is 3330kHz ~= 300ns per bit, ~133333 bits/track (?) */
#define	DIABLO31_BIT_TIME	((ntime_t)300ll)

/** @brief DIABLO 44 rotation time is approx. 25ms */
#define	DIABLO44_ROTATION_TIME	((ntime_t)25000000ll)

/** @brief DIABLO 44 sector time */
#define	DIABLO44_SECTOR_TIME	(DIABLO44_ROTATION_TIME/DRIVE_SPT)

/** @brief DIABLO 44 bit clock is 5000kHz ~= 200ns per bit, ~125000 bits/track (?) */
#define	DIABLO44_BIT_TIME	((ntime_t)200ll)

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

/** @brief MFROBL: disk header preamble is 34 words */
#define	MFROBL		34

/** @brief MFRRDL: disk header read delay is 21 words */
#define	MFRRDL		21

/** @brief MIRRDL: interrecord read delay is 4 words */
#define	MIRRDL		4

/** @brief MIROBL: disk interrecord preamble is 3 words */
#define	MIROBL		3

/** @brief MRPAL: disk read postamble length is 3 words */
#define	MRPAL		3

/** @brief MWPAL: disk write postamble length is 5 words */
#define	MWPAL		5

/**
 * @brief format of the raw disk image including the non-data words
 *
 * This includes preambles, postambles and gaps between the records
 *
 * Note: this is all guesswork!
 *
 * ---
 * 291 words per sector (at least)

 * To get to the DIABLO44 125000 bits + clocks per track, we'd need:
 *  125000 / 32 = 3906.25 words per track
 *    3906.25 / 12 = 325.5... words per sector
 *
 * To get to the DIABLO31 133333 bits + clocks per track, we'd need:
 *  133333 / 32 = 4166.6... words per track
 *    4166.6... / 12 = 347.2... words per sector
 *
 * 291 words per raw sector is within both limits.
 *
 */
typedef struct {
	uint8_t mfrdl1[2*MFRRDL];
	uint8_t header[2*HEADER_WORDS];
	uint8_t cksum1[2*CKSUM_WORDS];
	uint8_t mirrdl2[2*MIRRDL];
	uint8_t mwpal2[2*MWPAL];
	uint8_t label[2*LABEL_WORDS];
	uint8_t cksum2[2*CKSUM_WORDS];
	uint8_t mirrdl3[2*MIRRDL];
	uint8_t mwpal3[2*MWPAL];
	uint8_t data[2*DATA_WORDS];
	uint8_t cksum3[2*CKSUM_WORDS];
	uint8_t mwpal4[2*MWPAL];
}	raw_sector_t;

typedef enum {
	RECNO_MFRDL1,
	RECNO_HEADER,
	RECNO_CKSUM1,
	RECNO_MIRRDL2,
	RECNO_MWPAL2,
	RECNO_LABEL,
	RECNO_CKSUM2,
	RECNO_MIRRDL3,
	RECNO_MWPAL3,
	RECNO_DATA,
	RECNO_CKSUM3,
	RECNO_MWPAL4,
	RECNO_SLACK
}	drive_recno_t;

typedef struct {
	drive_recno_t recno;
	off_t offs;
	size_t size;
	const char *name;
}	drive_record_t;

static uint8_t slack[512];

/** @brief get a word from 'field' at word offset 'offs'  */
#define	GET_WORD(field,offs) ((field)[2*(offs)] | 256*(field)[2*(offs)+1])

/** @brief get a word from 'field' at word offset 'offs'  */
#define	PUT_WORD(field,offs,data) do { \
	(field)[2*(offs)] = (data) % 256; \
	(field)[2*(offs)+1] = (data) / 256; \
} while (0)

/** @brief offset of a 'field' in the structure 'type' */
#define OFFSETOF(type, field) ((int)&(((type *)NULL)->field) )

/** @brief word size of a 'field' in the structure 'type' */
#define WSIZEOF(type, field) (sizeof(((type *)NULL)->field)/2)

static const drive_record_t records[] = {
	{
		RECNO_MFRDL1,
		OFFSETOF(raw_sector_t,mfrdl1),
		WSIZEOF(raw_sector_t,mfrdl1),
		"MFRDL1"
	},
	{
		RECNO_HEADER,
		OFFSETOF(raw_sector_t,header),
		WSIZEOF(raw_sector_t,header),
		"HEADER"
	},
	{
		RECNO_CKSUM1,
		OFFSETOF(raw_sector_t,cksum1),
		WSIZEOF(raw_sector_t,cksum1),
		"CKSUM1"
	},
	{
		RECNO_MIRRDL2,
		OFFSETOF(raw_sector_t,mirrdl2),
		WSIZEOF(raw_sector_t,mirrdl2),
		"MIRRDL2"
	},
	{
		RECNO_MWPAL2,
		OFFSETOF(raw_sector_t,mwpal2),
		WSIZEOF(raw_sector_t,mwpal2),
		"MWPAL2"
	},
	{
		RECNO_LABEL,
		OFFSETOF(raw_sector_t,label),
		WSIZEOF(raw_sector_t,label),
		"LABEL"	
	},
	{
		RECNO_CKSUM2,
		OFFSETOF(raw_sector_t,cksum2),
		WSIZEOF(raw_sector_t,cksum2),
		"CKSUM2"
	},
	{
		RECNO_MIRRDL3,
		OFFSETOF(raw_sector_t,mirrdl3),
		WSIZEOF(raw_sector_t,mirrdl3),
		"MRPAL3"
	},
	{
		RECNO_MWPAL3,
		OFFSETOF(raw_sector_t,mwpal3),
		WSIZEOF(raw_sector_t,mwpal3),
		"MWPAL3"
	},
	{
		RECNO_DATA,
		OFFSETOF(raw_sector_t,data),
		WSIZEOF(raw_sector_t,data),
		"DATA"	
	},
	{
		RECNO_CKSUM3,
		OFFSETOF(raw_sector_t,cksum3),
		WSIZEOF(raw_sector_t,cksum3),
		"CKSUM3"
	},
	{
		RECNO_MWPAL4,
		OFFSETOF(raw_sector_t,mwpal4),
		WSIZEOF(raw_sector_t,mwpal4),
		"MWPAL4"
	},
	{
		RECNO_SLACK,	
		-1,
		sizeof(slack)/2,
		"SLACK"
	}
};

/**
 * @brief structur
		 of the disk drive context (2 drives or packs per system)
 */
typedef struct {
	/** @brief raw disk image, made up of 203 x 2 x 12 sectors */
	raw_sector_t *image;

	/** @brief drive unit number (0 or 1) */
	int	unit;

	/** @brief description of the drive(s) */
	char	description[32];

	/** @brief number of packs in drive (1 or 2) */
	int	packs;

	/** @brief rotation time */
	ntime_t	rotation_time;

	/** @brief per sector time (derived: rotation_time / DRIVE_SPT) */
	ntime_t	sector_time;

	/** @brief bit time in nano seconds */
	ntime_t	bit_time;

	/** @brief start time of current sector in the disk rotation */
	ntime_t t0;

	/** @brief pointer to current raw sector */
	raw_sector_t *rs;

	/** @brief callback to call at the start of each sector */
	void	(*sector_callback)(int unit);

	/** @brief drive seek/read/write signal (active 0) */
	int	s_r_w_0;

	/** @brief drive ready signal (active 0) */
	int	ready_0;

	/** @brief sector mark (0 if new sector) */
	int	sector_mark_0;

	/** @brief address acknowledge, i.e. seek successful (active 0) */
	int	addx_acknowledge_0;

	/** @brief log address interlock, i.e. seek in progress (active 0) */
	int	log_addx_interlock_0;

	/** @brief seek incomplete, i.e. seek in progress (active 0) */
	int	seek_incomplete_0;

	/** @brief current cylinder number */
	int	cylinder;

	/** @brief current head (track) number on cylinder */
	int	head;

	/** @brief current sector number in track */
	int	sector;

	/** @brief current record number in sector */
	int	record;

	/** @brief current offset into record */
	int	offset;

	/** @brief current bit index in record word */
	int	index;

	/** @brief current data word */
	int	data;
}	drive_t;


static drive_t drive[DRIVE_MAX] = {
	{
		NULL,
		0,
		"DIABLO44",
		2,
		DIABLO44_ROTATION_TIME,
		DIABLO44_SECTOR_TIME,
		DIABLO44_BIT_TIME,
		0,
	},{
		NULL,
		1,
		"DIABLO44",
		2,
		DIABLO44_ROTATION_TIME,
		DIABLO44_SECTOR_TIME,
		DIABLO44_BIT_TIME,
		0,
	}
};

/** 
 * @brief calculate the offset into the current sector
 *
 * Modifies drive's record, offset, and index fields to the
 * values that contain the clock and bit 'index'
 *
 * @param unit unit number
 * @param index bit index into the current sector
 */
static void drive_sector_offset(int unit, int index)
{
	static int last_index;
	static int last_offs;
	drive_t *d = &drive[unit];
	int offs;
	int i;

	if (!d->image)
		return;

	/* quick check for same index */
	if (last_index == index)
		return;
	last_index = index;

	/* offset in words */
	offs = index / 32;

	/* quick check for same word offset */
	if (last_offs == offs) {
		d->index = index % 32;
		return;
	}
	last_offs = offs;

	/* scan records for the record containing the offs */
	for (i = 0; i < sizeof(records)/sizeof(records[0]); i++) {
		const drive_record_t *r = &records[i];
		if (offs < r->size) {
			d->record = r->recno;
			d->offset = offs;
			d->index = index % 32;
			return;
		}
		offs -= r->size;
	}
	d->offset = offs;
	d->record = RECNO_SLACK;
}

/** 
 * @brief calculate the raw sector from the logical block address
 *
 * Modifies drive's 'rs' pointer by calculating the logical
 * block address from cylinder, head, and sector.
 *
 * @param unit unit number
 */
static void drive_raw_sector(int unit)
{
	drive_t *d = &drive[unit];
	size_t lba;
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_raw_sector()\n", unit);

	/* If there's no image, just reset the raw sector pointer */
	if (!d->image) {
		d->rs = NULL;
		return;
	}
	/* calculate the new disk relative sector offset */
	lba = (d->cylinder * DRIVE_HEADS + d->head) * DRIVE_SPT + d->sector;
	LOG((log_DRV,9,"	DRIVE C/H/S:%d/%d/%d => lba:%d\n",
		d->cylinder, d->head, d->sector, lba));

	d->rs = &d->image[lba];
}

/** 
 * @brief return number of bitclk edges for a raw sector
 *
 * @result number of bitclk edges for a raw sector
 */
int drive_bits_per_sector(void)
{
	return sizeof(raw_sector_t) * 16;
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
	int mark;
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_sector_mark()\n", unit);

	/* get and reset (to 1) the sector mark */
	mark = d->sector_mark_0;
	d->sector_mark_0 = 1;

	/* set t0 to now, if the sector mark was just read */
	if (0 == mark) {
		d->t0 = ntime();
	}

	/* return the sector mark */
	return mark;
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
 * @brief select the head of a drive unit
 *
 * @param unit unit number
 * @param head head number
 */
void drive_select(int unit, int head)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_select()\n", unit);

	/* if this drive has no image, just return */
	if (!d->image) {
		LOG((log_DRV,1,"	drive_select unit #%d has no image\n", unit));
		d->ready_0 = 1;
		return;
	}
	/* other unit is not ready now */
	drive[unit ^ 1].ready_0 = 1;
	d->ready_0 = 0;		/* drive is ready now */
	LOG((log_DRV,1,"	drive_select unit #%d ready\n", unit));

	/* Note: head select input is active low (0: selects head 1, 1: selects head 0) */
	head = head & DRIVE_HEAD_MASK;
	if (head != d->head) {
		d->head = head;
		LOG((log_DRV,1,"	drive_select head %d on unit #%d\n",
			head, unit));
	}
	/* lookup the raw sector */
	drive_raw_sector(unit);
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
 * @brief return the current record of a drive unit
 *
 * Note: the drive itself has not really notion of a record,
 * but just a sector. This function is a helper for the debug output.
 *
 * @param unit unit number
 * @result the current record number inside the current sector
 */
int drive_record(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_record()\n", unit);
	return d->record;
}

/** 
 * @brief return the name for the current record of a drive unit
 *
 * Note: the drive itself has not really notion of a record,
 * but just a sector. This function is a helper for the debug output.
 *
 * @param unit unit number
 * @result pointer to a string descibing the record
 */
const char *drive_record_name(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_record()\n", unit);
	return records[d->record].name;
}

/** 
 * @brief return the current offset into the record of a drive unit
 *
 * @param unit unit number
 * @result the current offset into the current record
 */
extern int drive_offset(int unit)
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_offset()\n", unit);
	return d->offset;
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

	if (strobe == 1) {
		LOG((log_DRV,1,"	STROBE end of interlock\n", seekto));
		d->log_addx_interlock_0 = 1;	/* reset log address interlock */
		return;
	}


	d->s_r_w_0 = 1;			/* reset the seek/read/write status */
	d->log_addx_interlock_0 = 0;	/* set the log address interlock */
	if (seekto == d->cylinder) {
		LOG((log_DRV,1,"	STROBE to cylinder %d acknowledge\n", seekto));
		d->s_r_w_0 = 0;			/* set the seek/read/write status */
		d->addx_acknowledge_0 = 0;	/* address acknowledge, if cylinder is reached */
		d->seek_incomplete_0 = 1;	/* reset seek incomplete */
		return;
	}

	d->addx_acknowledge_0 = 1;	/* no address acknowledge yet */
	d->seek_incomplete_0 = 0;	/* set seek incomplete */

	if (seekto < d->cylinder) {
		/* decrement cylinder */
		d->cylinder -= 1;
		if (d->cylinder < 0) {
			LOG((log_DRV,1,"	STROBE to cylinder %d incomplete\n", seekto));
			d->cylinder = 0;
			d->s_r_w_0 = 0;			/* set the seek/read/write status */
			return;
		}
	} else {
		/* increment cylinder */
		d->cylinder += 1;
		if (d->cylinder >= DRIVE_CYLINDERS) {
			LOG((log_DRV,1,"	STROBE to cylinder %d incomplete\n", seekto));
			d->cylinder = DRIVE_CYLINDERS - 1;
			d->s_r_w_0 = 0;			/* set the seek/read/write status */
			return;
		}
	}
	LOG((log_DRV,1,"	STROBE to cylinder %d (now %d) - interlock\n",
		seekto, d->cylinder));
}

/** 
 * @brief install a callback to be called whenever a drive sector starts
 *
 * @param unit is the drive index
 * @param callback function to call when a new sector begins
 * @result returns 0 on success
 */
int drive_sector_callback(int unit, void (*callback)(int))
{
	drive_t *d = &drive[unit];
	if (unit < 0 || unit >= DRIVE_MAX)
		fatal(1, "invalid unit %d in call to drive_sector_callback()\n", unit);

	d->sector_callback = callback;
	return 0;
}

/** 
 * @brief get the sector relative bit at index
 *
 * Note: this is a gross hack to allow the controller pulling
 * bits at its will, rather than clocking them with the drive's
 * RDCLK-
 *
 * @param unit drive unit number
 * @param index relative index of bit/clock into sector
 * @result return the sectore relative bit at index
 */
int drive_rddata(int unit, int index)
{
	drive_t *d = &drive[unit];
	raw_sector_t *rs = d->rs;

	if (!d->image || !rs)
		return 0177777;

	drive_sector_offset(unit, index);

	switch (d->record) {
	case RECNO_MFRDL1:
		d->data = GET_WORD(rs->mfrdl1,d->offset);
		break;
	case RECNO_HEADER:
		d->data = GET_WORD(rs->header,d->offset);
		break;
	case RECNO_CKSUM1:
		d->data = GET_WORD(rs->cksum1,d->offset);
		break;
	case RECNO_MIRRDL2:
		d->data = GET_WORD(rs->mirrdl2,d->offset);
		break;
	case RECNO_MWPAL2:
		d->data = GET_WORD(rs->mwpal2,d->offset);
		break;
	case RECNO_LABEL:
		d->data = GET_WORD(rs->label,d->offset);
		break;
	case RECNO_CKSUM2:
		d->data = GET_WORD(rs->cksum2,d->offset);
		break;
	case RECNO_MIRRDL3:
		d->data = GET_WORD(rs->mirrdl3,d->offset);
		break;
	case RECNO_MWPAL3:
		d->data = GET_WORD(rs->mwpal3,d->offset);
		break;
	case RECNO_DATA:
		d->data = GET_WORD(rs->data,d->offset);
		break;
	case RECNO_CKSUM3:
		d->data = GET_WORD(rs->cksum3,d->offset);
		break;
	case RECNO_MWPAL4:
		d->data = GET_WORD(rs->mwpal4,d->offset);
		break;
	case RECNO_SLACK:
		d->data = GET_WORD(slack,d->offset);
		break;
	}

	if (index & 1) {
		LOG((log_DRV,4,
			"	DRIVE read sector #%d index:%d bit:%2d" \
			" (%d)%s[0x%02x] (%07o = 0x%04x)\n",
			d->sector, index, d->index / 2,
			d->record, records[d->record].name,
			d->offset, d->data, d->data));
	}

	return (d->data >> (15 - (d->index / 2))) & 1;
}

/** 
 * @brief write the sector relative bit at index
 *
 * @param unit drive unit number
 * @param index relative index of bit/clock into sector
 * @param wrdata write data clock or bit
 */
void drive_wrdata(int unit, int index, int wrdata)
{
	drive_t *d = &drive[unit];
	raw_sector_t *rs = d->rs;

	if (!d->image || !rs)
		return;

	switch (index % 32) {
	case  1: d->data = (d->data & ~(1 <<15)) | (wrdata <<15); break;
	case  3: d->data = (d->data & ~(1 <<14)) | (wrdata <<14); break;
	case  5: d->data = (d->data & ~(1 <<13)) | (wrdata <<13); break;
	case  7: d->data = (d->data & ~(1 <<12)) | (wrdata <<12); break;
	case  9: d->data = (d->data & ~(1 <<11)) | (wrdata <<11); break;
	case 11: d->data = (d->data & ~(1 <<10)) | (wrdata <<10); break;
	case 13: d->data = (d->data & ~(1 << 9)) | (wrdata << 9); break;
	case 15: d->data = (d->data & ~(1 << 8)) | (wrdata << 8); break;
	case 17: d->data = (d->data & ~(1 << 7)) | (wrdata << 7); break;
	case 19: d->data = (d->data & ~(1 << 6)) | (wrdata << 6); break;
	case 21: d->data = (d->data & ~(1 << 5)) | (wrdata << 5); break;
	case 23: d->data = (d->data & ~(1 << 4)) | (wrdata << 4); break;
	case 25: d->data = (d->data & ~(1 << 3)) | (wrdata << 3); break;
	case 27: d->data = (d->data & ~(1 << 2)) | (wrdata << 2); break;
	case 29: d->data = (d->data & ~(1 << 1)) | (wrdata << 1); break;
	case 31:
		d->data = (d->data & ~(1 << 0)) | (wrdata << 0);
		/* now, after the last bit is written, store the data word */
		drive_sector_offset(unit, index);
		switch (d->record) {
		case RECNO_MFRDL1:
			PUT_WORD(rs->mfrdl1,d->offset,d->data);
			break;
		case RECNO_HEADER:
			PUT_WORD(rs->header,d->offset,d->data);
			break;
		case RECNO_CKSUM1:
			PUT_WORD(rs->cksum1,d->offset,d->data);
			break;
		case RECNO_MIRRDL2:
			PUT_WORD(rs->mirrdl2,d->offset,d->data);
			break;
		case RECNO_MWPAL2:
			PUT_WORD(rs->mwpal2,d->offset,d->data);
			break;
		case RECNO_LABEL:
			PUT_WORD(rs->label,d->offset,d->data);
			break;
		case RECNO_CKSUM2:
			PUT_WORD(rs->cksum2,d->offset,d->data);
			break;
		case RECNO_MIRRDL3:
			PUT_WORD(rs->mirrdl3,d->offset,d->data);
			break;
		case RECNO_MWPAL3:
			PUT_WORD(rs->mwpal3,d->offset,d->data);
			break;
		case RECNO_DATA:
			PUT_WORD(rs->data,d->offset,d->data);
			break;
		case RECNO_CKSUM3:
			PUT_WORD(rs->cksum3,d->offset,d->data);
			break;
		case RECNO_MWPAL4:
			PUT_WORD(rs->mwpal4,d->offset,d->data);
			break;
		case RECNO_SLACK:
			PUT_WORD(slack,d->offset,d->data);
			break;
		}
		break;
	}
}

/** 
 * @brief timer callback that is called once per sector in the rotation
 *
 * @param id timer id
 * @param arg drive unit number
 */
static void drive_next_sector(int id, int arg)
{
	int unit = arg;
	drive_t *d;

	if (drive[unit].ready_0)
		unit ^= 1;
	d = &drive[unit];

	/*
	 * just remember the nanotime of the start of this sector
	 * Note: reading the drive's RDCLK is not yet implemented
	 */
	d->t0 = ntime();

	/* count sectors */
	d->sector = (d->sector + 1) % DRIVE_SPT;

	drive_raw_sector(unit);

	d->sector_mark_0 = 0;		/* set sector mark */

	/* call the sector_callback, if any */
	if (d->sector_callback)
		(*d->sector_callback)(unit);

	/* restart the timer */
	timer_insert(d->sector_time, drive_next_sector, unit, "next sector");
}

/** 
 * @brief initialize a preamble field of a raw sector
 *
 * @param dst pointer to the data to be filled ('size' bytes)
 * @param size size of data in bytes
 * @result returns 0
 */
int drive_preamble(uint8_t *dst, size_t size)
{
	if (size <= 0)
		return -1;
	memset(dst, 0, size);
	/* set the sync bit in the last word */
	dst[size - 2] = 1;
	return 0;
}

/** 
 * @brief initialize a postamble field of a raw sector
 *
 * @param dst pointer to the data to be filled ('size' bytes)
 * @param size size of data in bytes
 * @param ones number of ones to set in the first words of the postamble
 * @result returns 0
 */
int drive_postamble(uint8_t *dst, size_t size, size_t ones)
{
	size_t offs;
	if (size <= 0)
		return -1;
	memset(dst, 0, size);
	/* set the first 'ones' word's sync bit */
	for (offs = 0; offs < 2*ones; offs += 2)
		dst[offs] = 1;
	return 0;
}

/** 
 * @brief copy a sector field and calculate and store a checksum
 *
 * Copy a field from a 'cooked' sector data to the 'raw' sector.
 * The data is put there in reverse order, because this is how the
 * KWD microcode expects it to be.
 *
 * @param cksum pointer to the checksum (2 bytes)
 * @param dst pointer to the data to be filled ('size' bytes)
 * @param src pointer to the original data ('size' bytes)
 * @param size size of data in words
 * @param start start value for the checksum
 * @result returns the checksum
 */
int drive_cksum(uint8_t *cksum, uint8_t *dst, uint8_t *src, size_t size, int start)
{
	int sum = start;
	int word;
	size_t s, d;

	/* compute XOR of all words */
	for (d = 0, s = size - 1; d < size; d++, s--) {
		word = src[2*s+0] + 256 * src[2*s+1];
		dst[2*d+0] = word % 256;
		dst[2*d+1] = word / 256;
		sum ^= word;
	}
	cksum[0] = sum % 256;
	cksum[1] = sum / 256;
	return sum;
}

/** 
 * @brief pass down command line arguments to the drive emulation
 *
 * @param arg command line argument
 * @result returns 0 if this was a disk image, -1 on error
 */
int drive_args(const char *arg)
{
	/* magic constant 0521 (contained in constant prom address 035) */
	int magic = const_prom[035];
	int unit;
	int hdr, c, h, s, chs;
	drive_t *d;
	int zcat;
	FILE *fp;
	size_t size;		/* size in sectors */
	size_t done;		/* read loops */
	size_t csize;		/* size of cooked image */
	size_t rsize;		/* size of raw image */
	uint8_t *image;		/* file image (either cooked, or compressed) */
	sector_t *cooked;	/* cooked image */
	raw_sector_t *raw;	/* raw image */
	uint8_t *ip, *cp, *rp;
	char *p;

	/* don't care about switches (yet) */
	if (arg[0] == '-' || arg[0] == '+')
		return -1;

	/* find trailing dot */
	p = strrchr(arg, '.');
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

	LOG((log_DRV,0, "got %d (%#x) bytes from file %s\n", done, done, arg));

	cooked = (sector_t *)malloc(csize);
	if (!cooked)
		fatal(1, "failed to malloc(%d) bytes\n", csize);
	cp = (uint8_t *)cooked;

	if (zcat) {
		size_t skip;
		int type = z_header(ip, done, &skip);

		switch (type) {
		case 0:
			LOG((log_DRV,0, "compression type unknown, skip:%d\n", skip));
			csize = gz_copy(cp, csize, ip, done);
			break;
		case 1:
			LOG((log_DRV,0, "compression type compress, skip:%d\n", skip));
			csize = z_copy(cp, csize, ip + skip, done - skip);
			break;
		case 2:
			LOG((log_DRV,0, "compression type gzip, skip:%d\n", skip));
			csize = gz_copy(cp, csize, ip, done);
			break;
		}
	} else {
		memcpy(cp, ip, done);
		csize = done;
	}
	free(image);
	image = NULL;

	cp = (uint8_t *)cooked;

#if	1
	fp = fopen("disk.raw", "wb");
	if (!fp)
		fatal(1,"failed to fopen() disk.raw\n");
	if (size != fwrite(cp, sizeof(sector_t), size, fp))
		fatal(1,"failed to fwrite() disk.raw (%d bytes)\n", done);
	if (fclose(fp))
		fatal(1,"failed to fclose() disk.raw\n");
#endif

	if (csize != size * sizeof(sector_t)) {
		LOG((log_DRV,0,"disk image %s size mismatch (%d bytes)\n", arg, csize));
		free(cooked);
		return -1;
	}

	rsize = size * sizeof(raw_sector_t);
	raw = (raw_sector_t *)malloc(rsize);
	if (!raw)
		fatal(1, "failed to malloc(%d) bytes\n", rsize);
	rp = (uint8_t *)raw;

	/* reset cylinder, head, sector counters */
	c = h = s = 0;

	/* copy image sectors to raw sectors */
	for (done = 0; done < size; done++) {
		/* copy the available records in their place */

		/* copy and build checksum for header */
		drive_preamble(raw->mfrdl1, sizeof(raw->mfrdl1));
		drive_cksum(raw->cksum1, raw->header,
			cooked->header, HEADER_WORDS, magic);
		drive_postamble(raw->mirrdl2, sizeof(raw->mirrdl2), MIRRDL);

		/* copy and build checksum for label */
		drive_preamble(raw->mwpal2, sizeof(raw->mwpal2));
		drive_cksum(raw->cksum2, raw->label,
			cooked->label, LABEL_WORDS, magic);
		drive_postamble(raw->mirrdl3, sizeof(raw->mirrdl3), MIRRDL);

		/* copy and build checksum for data */
		drive_preamble(raw->mwpal3, sizeof(raw->mwpal3));
		drive_cksum(raw->cksum3, raw->data,
			cooked->data, DATA_WORDS, magic);
		drive_postamble(raw->mwpal4, sizeof(raw->mwpal4), MRPAL);

		/* don't know if and where to put the pageno */

		/* verify the chs with the header and log any mismatches */
		hdr = GET_WORD(raw->header, 0);
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
		raw++;
	}
	fclose(fp);

	free(cp);
	cp = NULL;
	cooked = NULL;


	/* set drive image */
	d->image = (raw_sector_t *)rp;

	LOG((log_DRV,0,"drive #%d successfully created raw image for %s\n", unit, arg));

	drive_select(unit, 0);

	d->addx_acknowledge_0 = 0;	/* drive address acknowledge is active now */
	d->log_addx_interlock_0 = 1;
	d->seek_incomplete_0 = 1;

	return 0;
}


/** 
 * @brief initialize the disk context and insert a disk wort timer
 *
 * @result returns 0 on success, fatal() on error
 */
int drive_init(void)
{
	int i;

	if (sizeof(sector_t) != 267 * 2)
		fatal(1, "sizeof(sector_t) is not %d (%d)\n",
			267 * 2, sizeof(sector_t));

	for (i = 0; i < DRIVE_MAX; i++) {
		drive_t *d = &drive[i];

		d->unit = i;			/* set the unit number */
		d->s_r_w_0 = 0;			/* seek/read/write is active */
		d->ready_0 = 1;			/* drive is not ready */
		d->sector_mark_0 = 1;		/* sector mark clear */
		d->addx_acknowledge_0 = 1;	/* drive address acknowledge is not active */
		d->log_addx_interlock_0 = 1;	/* drive log address interlock is not active */
		d->seek_incomplete_0 = 1;	/* drive seek incomplete is not active */

		/* reset the disk drive's address */
		d->cylinder = 0;
		d->head = 0;
		d->sector = 0;
		d->record = 0;
		d->offset = 0;
	}

	/* restart next sector timer */
	timer_insert(drive[0].sector_time, drive_next_sector, 0, "next sector");

	return 0;
}
