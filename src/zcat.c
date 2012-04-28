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
 * Implementation of functions to handle (old) compress
 *
 * $Id: zcat.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <zlib.h>

#include "alto.h"
#include "cpu.h"
#include "debug.h"
#include "zcat.h"

/** @brief if EFTYPE isn't defined, use some arbitrary (found in NetBSD) error number for it */
#ifndef	EFTYPE
#define	EFTYPE	79
#endif

/** @brief type to hold LZW codes (> 16 bits) */
typedef long lzwcode_t;

/** @brief type to hold LZW characters (8 bits) */
typedef unsigned char lzwchar_t;

/** @brief set non-zero to debug the string stack (tons of log output) */
#define	DEBUG_STACK	0

/** @brief set non-zero to debug filling of the bit buffer */
#define	DEBUG_BBUF	0

/** @brief flags: compress (may be) ASCII */
#define LZW_ASCII	(1 << 0)

/** @brief flags: header contains CRC word */
#define LZW_HCRC	(1 << 1)

/** @brief flags: header contains extra record */
#define LZW_EXTRA	(1 << 2)

/** @brief flags: header contains the original filename */
#define LZW_FILENAME	(1 << 3)

/** @brief flags: header contains a comment string */
#define LZW_COMMENT	(1 << 4)

/** @brief flags: reserved bits */
#define	LZW_RESERVED	(7 << 5)

/** @brief default (and max.) bits for compression */
#define LZW_BITS	16

/** @brief table size for string and stack */
#define LZW_HSIZE	((1<<LZW_BITS)+3456)

/** @brief number of bits mask for for third byte of header */
#define LZW_BIT_MASK	0x1f

/** @brief next bit 5 */
#define LZW_BIT_5	0x20

/** @brief next bit 6 */
#define LZW_BIT_6	0x40

/** @brief block flag for third byte of header */
#define LZW_BLOCK_MASK	0x80

/** @brief initial number of bits per code */
#define LZW_INIT_BITS	9

/** @brief first free entry in codes */
#define	LZW_FIRST_CODE	257

/** @brief table clear output code */
#define	LZW_CLEAR_CODE	256

/** @brief return the maximum code for n bits */
#define LZW_MAXCODE(nbits) ((1<<(nbits))-1)

/** @brief method for compress()d files */
#define	LZW_METHOD	LZW_BITS

/** @brief prefix of code i */
#define	LZW_PREFIXOF(i)	(z.codetab[i])

/** @brief suffix of code i */
#define	LZW_SUFFIXOF(i)	((lzwchar_t *)z.htab)[i]

/** @brief stack bottom */
#define	LZW_DE_STACK	((lzwchar_t *)&LZW_SUFFIXOF(1 << LZW_BITS))

/** @brief stack top */
#define	LZW_STACKTOP	((lzwchar_t *)&LZW_SUFFIXOF(LZW_HSIZE))

/** @brief magic header of compressed files (compress) */
static const lzwchar_t z_magic[2] = {0037, 0235};

static const lzwchar_t rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

/** @brief structure of a compressed stream */
typedef struct {
	/** @brief input pointer */
	uint8_t *ip;

	/** @brief input offset */
	off_t ioffs;

	/** @brief input size in bytes */
	size_t isize;

	/** @brief output pointer */
	uint8_t	*op;

	/** @brief output offset */
	off_t ooffs;

	/** @brief output size in bytes */
	size_t osize;

	/** @brief set by getcode() if end of input is reached */
	int eof;

	/** @brief flag set non-zero when a clear code is found */
	int clear;

	/** @brief number of bits per code */
	int nbits;

	/** @brief user settable max number of bits per code */
	int bits;

	/** @brief maximum code, given nbits */
	lzwcode_t max;

	/** @brief should NEVER generate this code */
	lzwcode_t maxmax;

	/** @brief string of suffixes */
	lzwchar_t htab[LZW_HSIZE];

	/** @brief code prefix table and stack */
	lzwcode_t codetab[LZW_HSIZE];

	/** @brief block compression parameters */
	int block;

	/** @brief stack pointer */
	lzwchar_t *sp;

	/** @brief final character */
	lzwcode_t final;

	/** @brief current code */
	lzwcode_t code;

	/** @brief previous code */
	lzwcode_t old;

	/** @brief input code */
	lzwcode_t incode;

	/** @brief first free code */
	lzwcode_t free;

	/** @brief get code bit buffer */
	lzwchar_t bbuf[LZW_BITS];

	/** @brief get code buffer read offset (bits) */
	int boffs;

	/** @brief get code buffer filled (bits) */
	int bsize;
}	zstate_t;

static zstate_t z;


static lzwcode_t getcode(void)
{
	int boffs, bits, code;
	lzwchar_t *bp;

	if (z.eof)
		return -1;

	bp = z.bbuf;
	if (z.clear || z.boffs >= z.bsize || z.free > z.max) {
		if (z.free > z.max) {
			LOG((log_MISC,5,"FREE incode:%04x nbits:%d drop:%d i:%x o:%x\n",
				z.incode, z.nbits, z.bsize - z.boffs,
				(long)z.ioffs, (long)z.ooffs));
			z.nbits++;
			if (z.nbits >= z.bits)
				z.max = z.maxmax;
			else
				z.max = LZW_MAXCODE(z.nbits);
		}
		if (z.clear) {
			LOG((log_MISC,5,"CLEAR incode:%04x nbits:%d drop:%d i:%x o:%x\n",
				z.incode, z.nbits, z.bsize - z.boffs,
				(long)z.ioffs, (long)z.ooffs));
			z.max = LZW_MAXCODE(z.nbits = LZW_INIT_BITS);
			z.clear = 0;
		}

		/* refill the bit buffer */
		z.boffs = 0;
		z.bsize = 0;
		while (z.ioffs < z.isize && z.bsize < z.nbits) {
			z.bbuf[z.bsize] = z.ip[z.ioffs];
			z.bsize++;
			z.ioffs++;
		}
		if (z.bsize <= 0) {
			z.eof = 1;
			return -1;
		}
#if	DEBUG_BBUF
		LOG((log_MISC,9,"BBUF nbits:%d i:%x o:%x (",
			z.nbits, (long)z.ioffs, (long)z.ooffs));
		for (boffs = 0; boffs < z.bsize; boffs++)
			LOG((log_MISC,9,"%02x", z.bbuf[boffs]));
		LOG((log_MISC,9,")\n"));
#endif
		/* round bit size down to integral number of code bits */
		z.bsize = (z.bsize << 3) - (z.nbits - 1);
	}

	boffs = z.boffs;
	bits = z.nbits;

	/* point bp to the first byte */
	bp += boffs >> 3;
	boffs &= 7;
	/* get low order bits */
	code = *bp++ >> boffs;
	bits -= (8 - boffs);
	boffs = 8 - boffs;
	/* get any 8 bit parts in the middle */
	if (bits >= 8) {
		code |= *bp++ << boffs;
		boffs += 8;
		bits -= 8;
	}
	/* get any high order bits */
	code |= (*bp & rmask[bits]) << boffs;
	z.boffs += z.nbits;

	return code;
}

/**
 * @brief uncompress a compress()d block of memory
 *
 * @param op pointer to output buffer
 * @param osize number of bytes in output buffer
 * @param ip pointer to input buffer
 * @param isize number of bytes in input buffer
 * @result returns the size of the uncompressed data
 */
off_t z_copy(uint8_t *op, size_t osize, uint8_t *ip, size_t isize)
{
	int flag;

	memset(&z, 0, sizeof(z));
	/* remember input pointer and size */
	z.ip = ip;
	z.isize = isize;

	/* remember output pointer and size */
	z.op = op;
	z.osize = osize;

	if (z.ioffs >= isize)
		goto eof;

	/* skip magic header, if it is there */
	if (!memcmp(ip, z_magic, sizeof(z_magic)))
		z.ioffs += 2;

	if (z.ioffs >= isize)
		goto eof;

	flag = z.ip[z.ioffs++];
	LOG((log_MISC,5,"flag: %#o (%#x)\n", flag, flag));

	/* max bits in this image */
	z.bits = flag & LZW_BIT_MASK;
	z.block = flag & LZW_BLOCK_MASK;
	z.max = LZW_MAXCODE(z.nbits = LZW_INIT_BITS);
	z.maxmax = 1ll << z.bits;
	z.free = z.block ? LZW_FIRST_CODE : 256;

	if (z.bits > LZW_BITS) {
		errno = EFTYPE;
		LOG((log_MISC,0, "z_copy() invalid bits:%d\n", z.bits));
		return (-1);
	}

	LOG((log_MISC,5,"bits   : %#o (%#x)\n", z.bits, z.bits));
	LOG((log_MISC,5,"max    : %#o (%#x)\n", z.max, z.max));
	LOG((log_MISC,5,"maxmax : %#o (%#x)\n", z.maxmax, z.maxmax));

	/* initialize the first 256 entries in the table */
	for (z.code = 255; z.code >= 0; z.code--) {
		LZW_PREFIXOF(z.code) = 0;
		LZW_SUFFIXOF(z.code) = (lzwchar_t)z.code;
	}

	z.final = z.old = getcode();
	if (-1 == z.old)
		goto eof;
	if (z.ooffs >= z.osize)
		goto full;
	/* first code is 8 bits character */
	z.op[z.ooffs++] = (lzwchar_t)z.final;
	z.sp = LZW_DE_STACK;

	while ((z.code = getcode()) > -1) {

		if (z.block && LZW_CLEAR_CODE == z.code) {
			/* reset the prefixes for characters */
			for (z.code = 255; z.code >= 0; z.code--)
				LZW_PREFIXOF(z.code) = 0;
			z.clear = 1;
			z.free = LZW_FIRST_CODE - 1;
			z.code = getcode();
			if (-1 == z.code)
				break;
		}
		z.incode = z.code;

		/* special case: repetitions */
		if (z.code >= z.free) {
			if (z.sp >= LZW_STACKTOP)
				fatal(1, "bogus LZW compress() stack overrun\n");
			*z.sp++ = z.final;
			z.code = z.old;
		}

		/* push suffixes */
		while (z.code >= 256) {
			if (z.code == LZW_PREFIXOF(z.code))
				fatal(1, "code == prefix[code] (%x)", z.code);
			if (z.sp >= LZW_STACKTOP)
				fatal(1, "bogus LZW compress() stack overrun\n");
			*z.sp++ = LZW_SUFFIXOF(z.code);
			z.code = LZW_PREFIXOF(z.code);
		}
		z.final = LZW_SUFFIXOF(z.code);
		*z.sp++ = z.final;

#if	DEBUG_STACK
		{
			int i;
			LOG((log_MISC,9,"code:%04x final:%02x old:%04x free:%04x " \
				"max:%04x sp:%d i:%x o:%x (",
				z.code, z.final, z.old, z.free, z.max,
				(int)(z.sp - LZW_DE_STACK),
				(long)z.ioffs, (long)z.ooffs));
			for (i = 0; i < (int)(z.sp - LZW_DE_STACK); i++)
				LOG((log_MISC,9,"%02x", z.sp[-1-i]));
			LOG((log_MISC,9,")\n"));
		}
#endif

		/* ... and pop the stack to the output */
		do {
			if (z.ooffs >= z.osize)
				goto full;
			z.op[z.ooffs++] = *--z.sp;
		} while (z.sp > LZW_DE_STACK);

		/* generate the new entry */
		z.code = z.free;
		if (z.code < z.maxmax) {
			LZW_PREFIXOF(z.code) = (uint16_t)z.old;
			LZW_SUFFIXOF(z.code) = z.final;
			z.free = z.code + 1;
		}

		/* remember previous code */
		z.old = z.incode;
	}

eof:
	LOG((log_MISC,5, "z_copy() end of input at i:%x o:%x\n",
		(long)z.ioffs, (long)z.ooffs));
	/* return offs? */
	return z.ooffs;

full:
	LOG((log_MISC,5, "z_copy() uncompressed i:%x o:%x\n",
		(long)z.ioffs, (long)z.ooffs));
	return z.ooffs;
}

/** @brief magic header of compressed files (gzip) */
static const uint8_t gz_magic[2] = {0037, 0213};

/** 
 * @brief uncompress from an input buffer to an output buffer using gzip's inflate()
 *
 * @param op pointer to output buffer
 * @param osize number of bytes in output buffer
 * @param ip pointer to input buffer
 * @param isize number of bytes in input buffer
 * @result returns the size of the inflate()
 */
off_t gz_copy(uint8_t *op, size_t osize, uint8_t *ip, size_t isize)
{
	z_stream d;
	int err;

	d.zalloc = (alloc_func)0;
	d.zfree = (free_func)0;
	d.opaque = (voidpf)0;

#if	1
	d.next_in  = ip;
	d.avail_in = 2;			/* just read the zlib header */

	err = inflateInit(&d);
	if (err != Z_OK) {
		LOG((log_MISC,0,"inflateInit() returned %d (%s)\n",
			err, d.msg ? d.msg : "-/-"));
		return -1;
	}

	d.next_out = op;
	d.avail_out = (uInt)osize;
	d.avail_in = (uInt)isize-2;	/* read all compressed data */

	inflate(&d, Z_FULL_FLUSH);
	if (err != Z_OK) {
		LOG((log_MISC,0,"inflate(Z_NO_FLUSH) returned %d (%s)\n",
			err, d.msg ? d.msg : "-/-"));
		return -1;
	}

	err = inflate(&d, Z_FINISH);
	if (err != Z_DATA_ERROR) {
		LOG((log_MISC,0,"inflate(Z_FINISH) should return ZDATA_ERROR, returned %d (%s)\n",
			err, d.msg ? d.msg : "-/-"));
		return -1;
	}
#else
	d.next_in  = ip;
	d.avail_in = isize;		/* all the data */

	err = inflateInit(&d);
	if (err != Z_OK) {
		LOG((log_MISC,0,"inflateInit() returned %d (%s)\n",
			err, d.msg ? d.msg : "-/-"));
		return -1;
	}

	d.next_out = op;
	d.avail_out = (uInt)osize;

	err = inflate(&d, Z_FINISH);
	if (err != Z_DATA_ERROR) {
		LOG((log_MISC,0,"inflate(Z_FINISH) should return ZDATA_ERROR, returned %d (%s)\n",
			err, d.msg ? d.msg : "-/-"));
		return -1;
	}
#endif

	LOG((log_MISC,0,"avail_out:%#x avail_in:%#x\n", d.avail_out, d.avail_in));

	err = inflateEnd(&d);
	if (err != Z_OK) {
		LOG((log_MISC,0,"inflateEnd() returned %d (%s)\n",
			err, d.msg ? d.msg : "-/-"));
		return -1;
	}
	osize -= d.avail_out;

	LOG((log_MISC,0,"done\n"));
	return osize;
}

/**
 * @brief analyze a compress/gzip header and return the (probable) type
 *
 * @param ip input buffer pointer
 * @param size size of input buffer in bytes
 * @param pskip point to a size_t to receive the number of bytes to skip
 * @result returns 0 for unknown header, 1 for compress, 2 for gzip
 */
int z_header(uint8_t *ip, size_t size, size_t *pskip)
{
	size_t skip = 10;
	uint8_t flag;
	uint32_t ztime;

	flag = ip[3];

	LOG((log_MISC,5, "header : %02x %02x\n", ip[0], ip[1]));

	/* skip compress()d magic header, if any */
	if (!memcmp(z_magic, ip, sizeof(z_magic))) {
		LOG((log_MISC,6, "method : %02x\n", ip[2]));
		/*
		 * Old compress (LZW) header is just the magic bytes
		 * The algorithm want's to see the method byte.
		 */
		if (pskip)
			*pskip = 2;
		return 1;
	}

	/* return 0 if unknown header */
	if (memcmp(gz_magic, ip, sizeof(gz_magic))) {
		if (pskip)
			*pskip = 0;
		return 0;
	}

	/* assume it's a gzip */
	LOG((log_MISC,5, "method : %02x\n", ip[2]));

	LOG((log_MISC,5, "flags  : %02x%s%s%s%s%s%s\n",
		flag,
		(flag & LZW_ASCII) ? " ASCII" : "",
		(flag & LZW_HCRC) ? " CRC" : "",
		(flag & LZW_EXTRA) ? " EXTRA" : "",
		(flag & LZW_FILENAME) ? " FILENAME" : "",
		(flag & LZW_COMMENT) ? " COMMENT" : "",
		(flag & LZW_RESERVED) ? " RESERVED" : ""));
	ztime = ip[4] + (ip[5] << 8) + (ip[6] << 16) + (ip[7] << 24);
	LOG((log_MISC,5, "time   : %08x\n", ztime));
	LOG((log_MISC,5, "xflags : %02x\n", ip[8]));
	switch (ip[9]) {
	case 0x00:
		LOG((log_MISC,5, "OS code: %02x (MS-DOS)\n", ip[9]));
		break;
	case 0x01:
		LOG((log_MISC,5, "OS code: %02x (Amiga)\n", ip[9]));
		break;
	case 0x02:
		LOG((log_MISC,5, "OS code: %02x (VAX/VMS)\n", ip[9]));
		break;
	case 0x03:
		LOG((log_MISC,5, "OS code: %02x (Unix)\n", ip[9]));
		break;
	case 0x05:
		LOG((log_MISC,5, "OS code: %02x (Atari)\n", ip[9]));
		break;
	case 0x06:
		LOG((log_MISC,5, "OS code: %02x (OS/2)\n", ip[9]));
		break;
	case 0x07:
		LOG((log_MISC,5, "OS code: %02x (MacOS)\n", ip[9]));
		break;
	case 0x0a:
		LOG((log_MISC,5, "OS code: %02x (TOPS20)\n", ip[9]));
		break;
	case 0x0f:
		LOG((log_MISC,5, "OS code: %02x (Prime/PRIMOS)\n", ip[9]));
		break;
	default:
		LOG((log_MISC,5, "OS code: %02x (unknown)\n", ip[9]));
	}

	if (flag & LZW_EXTRA) {
		size_t extra;
		if (skip + 1 >= size) {
			LOG((log_MISC,0, "invalid magic header EXTRA (%#x)\n", flag));
			return 0;
		}
		extra = ip[skip] + 256 * ip[skip+1];
		LOG((log_MISC,5,"header extra bytes: %d\n", extra));
		/* skip extra header bytes */
		skip += 2 + extra;
	}

	if (skip >= size) {
		LOG((log_MISC,0, "invalid magic header EXTRA (%#x) skip:%d\n", flag, skip));
		return 0;
	}

	if (flag & LZW_FILENAME) {
		LOG((log_MISC,5,"header filename: '%s'\n", ip + skip));
		/* skip NUL terminated filename */
		while (skip < size && ip[skip])
			skip++;
		skip++;
	}

	if (skip >= size) {
		LOG((log_MISC,0, "invalid magic header FILENAME (%#x) skip:%d\n", flag, skip));
		return 0;
	}

	if (flag & LZW_COMMENT) {
		LOG((log_MISC,5,"header comment: '%s'\n", ip + skip));
		/* skip NUL terminated comment string */
		while (skip < size && ip[skip])
			skip++;
		skip++;
	}

	if (skip >= size) {
		LOG((log_MISC,0, "invalid magic header COMMENT (%#x) skip:%d\n", flag, skip));
		return 0;
	}

	if (flag & LZW_HCRC) {
#if	DEBUG
		unsigned crc = ip[skip] + 256 * ip[skip + 1];
		LOG((log_MISC,5,"header CRC: %04x\n", crc));
#endif
		/* skip header CRC */
		skip += 2;
	}

	if (skip >= size) {
		LOG((log_MISC,0, "invalid magic header HCRC (%#x) skip:%d\n", flag, skip));
		return 0;
	}

	/* skip now contains the number of header bytes */
	if (pskip)
		*pskip = skip;
	return 2;
}
