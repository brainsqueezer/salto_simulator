/*****************************************************************************
 * zcat - uncompress LZW compressed files
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
 * $Id: zcat.c,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "zcat.h"

/** @brief set non-zero to debug the string stack (tons of log output) */
#define	DEBUG_STACK	1

/** @brief set non-zero to debug filling the bit buffer */
#define	DEBUG_BBUF	1

#define	LOG(x)	logprintf x

typedef long	code_t;
typedef unsigned char char_t;

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
#define LZW_MAXCODE(n_bits) ((1<<(n_bits))-1)

/** @brief method for compress()d files */
#define	LZW_METHOD	LZW_BITS

/** @brief prefix of code i */
#define	LZW_PREFIXOF(i)	(z.codetab[i])

/** @brief suffix of code i */
#define	LZW_SUFFIXOF(i)	((char_t *)z.htab)[i]

/** @brief stack bottom */
#define	LZW_DE_STACK	((char_t *)&LZW_SUFFIXOF(1 << LZW_BITS))

/** @brief magic header of compressed files (compress) */
static const char_t z_magic[2] = {0037, 0235};

static const char_t rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

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
	int n_bits;

	/** @brief user settable max number of bits per code */
	int maxbits;

	/** @brief maximum code, given n_bits */
	code_t max;

	/** @brief should NEVER generate this code */
	code_t maxmax;

	/** @brief string of suffixes */
	int htab[LZW_HSIZE];

	/** @brief code prefix table and stack */
	int codetab[LZW_HSIZE];

	/** @brief block compression parameters */
	int block;

	/** @brief stack pointer */
	char_t *sp;

	/** @brief final character */
	code_t final;

	/** @brief current code */
	code_t code;

	/** @brief previous code */
	code_t old;

	/** @brief input code */
	code_t incode;

	/** @brief first free code */
	code_t free;

	/** @brief get code bit buffer */
	char_t bbuf[LZW_BITS];

	/** @brief get code buffer read offset (bits) */
	int boffs;

	/** @brief get code buffer filled (bits) */
	int bsize;
}	zstate_t;

static zstate_t z;

static void logprintf(int task, int level, const char *fmt, ...)
{
	va_list ap;

	if (level > 9)
		return;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

static void fatal(int exitcode, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(exitcode);
}

static code_t getcode(void)
{
	int boffs, bits, code;
	char_t *bp;

	if (z.eof)
		return -1;

	bp = z.bbuf;
	if (z.clear || z.boffs >= z.bsize || z.free > z.max) {
		if (z.free > z.max) {
			LOG((0,9,"FREE incode:%04x n_bits:%d drop:%d i:%x o:%x\n",
				z.incode, z.n_bits, z.bsize - z.boffs,
				(long)z.ioffs, (long)z.ooffs));
			z.n_bits++;
			if (z.n_bits >= z.maxbits)
				z.max = z.maxmax;
			else
				z.max = LZW_MAXCODE(z.n_bits);
		}
		if (z.clear) {
			LOG((0,9,"CLEAR incode:%04x n_bits:%d drop:%d i:%x o:%x\n",
				z.incode, z.n_bits, z.bsize - z.boffs,
				(long)z.ioffs, (long)z.ooffs));
			z.max = LZW_MAXCODE(z.n_bits = LZW_INIT_BITS);
			z.clear = 0;
		}

		/* refill the bit buffer */
		z.boffs = 0;
		z.bsize = 0;
		while (z.ioffs < z.isize && z.bsize < z.n_bits) {
			z.bbuf[z.bsize] = z.ip[z.ioffs];
			z.bsize++;
			z.ioffs++;
		}
		if (z.bsize <= 0) {
			z.eof = 1;
			return -1;
		}
#if	DEBUG_BBUF
		LOG((0,9,"BBUF n_bits:%d i:%x o:%x (",
			z.n_bits, (long)z.ioffs, (long)z.ooffs));
		for (boffs = 0; boffs < z.bsize; boffs++)
			LOG((0,9,"%02x", z.bbuf[boffs]));
		LOG((0,9,")\n"));
#endif
		/* round bit size down to integral number of code bits */
		z.bsize = (z.bsize << 3) - (z.n_bits - 1);
	}

	boffs = z.boffs;
	bits = z.n_bits;

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
	z.boffs += z.n_bits;

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
	LOG((0,6,"flag: %#o (%#x)\n", flag, flag));

	/* max bits in this image */
	z.maxbits = flag & LZW_BIT_MASK;
	z.block = flag & LZW_BLOCK_MASK;
	z.max = LZW_MAXCODE(z.n_bits = LZW_INIT_BITS);
	z.maxmax = 1ll << z.maxbits;
	z.free = z.block ? LZW_FIRST_CODE : 256;

	if (z.maxbits > LZW_BITS) {
		errno = EFTYPE;
		LOG((0,0, "z_copy() invalid maxbits:%d\n",
			z.maxbits));
		return (-1);
	}

	LOG((0,7,"maxbits: %#o (%#x)\n", z.maxbits, z.maxbits));
	LOG((0,7,"max: %#o (%#x)\n", z.max, z.max));
	LOG((0,7,"maxmax: %#o (%#x)\n", z.maxmax, z.maxmax));

	/* initialize the first 256 entries in the table */
	for (z.code = 255; z.code >= 0; z.code--) {
		LZW_PREFIXOF(z.code) = 0;
		LZW_SUFFIXOF(z.code) = (char_t)z.code;
	}

	z.final = z.old = getcode();
	if (-1 == z.old)
		goto eof;
	if (z.ooffs >= z.osize)
		goto full;
	/* first code is 8 bits character */
	z.op[z.ooffs++] = (char_t)z.final;
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
			*z.sp++ = z.final;
			z.code = z.old;
		}

		/* push suffixes */
		while (z.code >= 256) {
			if (z.code == LZW_PREFIXOF(z.code))
				fatal(1, "code == prefix[code] (%x)", z.code);
			*z.sp++ = LZW_SUFFIXOF(z.code);
			z.code = LZW_PREFIXOF(z.code);
		}
		z.final = LZW_SUFFIXOF(z.code);
		*z.sp++ = z.final;

#if	DEBUG_STACK
		{
			int i;
			LOG((0,9,"code:%04x final:%02x old:%04x free:%04x " \
				"max:%04x sp:%d i:%x o:%x (",
				z.code, z.final, z.old, z.free, z.max,
				(int)(z.sp - LZW_DE_STACK),
				(long)z.ioffs, (long)z.ooffs));
			for (i = 0; i < (int)(z.sp - LZW_DE_STACK); i++)
				LOG((0,9,"%02x", z.sp[-1-i]));
			LOG((0,9,")\n"));
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
	LOG((0,6, "z_copy() end of input at i:%x o:%x\n",
		(long)z.ioffs, (long)z.ooffs));
	/* return offs? */
	return z.ooffs;

full:
	LOG((0,6, "z_copy() uncompressed i:%x o:%x\n",
		(long)z.ioffs, (long)z.ooffs));
	return z.ooffs;
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
	uint8_t flag;
	uint32_t ztime;

	flag = ip[3];

	LOG((0,5, "header : %02x %02x\n", ip[0], ip[1]));

	/* skip compress()d magic header, if any */
	if (!memcmp(z_magic, ip, sizeof(z_magic))) {
		LOG((0,6, "method : %02x\n", ip[2]));
		/*
		 * Old compress (LZW) header is just the magic bytes
		 * The algorithm want's to see the method byte.
		 */
		if (pskip)
			*pskip = 2;
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	uint8_t *ibuf, *obuf;
	size_t isize, osize, skip;
	off_t done;
	FILE *fp;

	if (argc < 2) {
		fprintf(stderr, "usage: %s file\n", argv[0]);
		return 1;
	}

	fp = fopen(argv[1], "rb");
	if (!fp)
		fatal(1, "fopen(%s,'rb') failed\n", argv[1]);

	fseek(fp, 0, SEEK_END);
	isize = ftell(fp);
	rewind(fp);
	ibuf = malloc(isize);
	if (!ibuf)
		fatal(1, "malloc(%d) failed\n", isize);
	if (isize != fread(ibuf, 1, isize, fp))
		fatal(1, "fread(%s,%d) failed\n", argv[1], isize);
	fclose(fp);

	if (1 != z_header(ibuf, isize, &skip))
		fatal(1, "z_header() failed\n");

	osize = isize;
	obuf = NULL;
	do {
		osize *= 2;
		obuf = realloc(obuf, osize);
		if (!obuf)
			fatal(1, "malloc(%d) failed\n", osize);
		done = z_copy(obuf, osize, ibuf + skip, isize - skip);
	} while (done == osize);

	printf("%lld bytes out\n", done);
	if (argc > 2) {
		fp = fopen(argv[2], "wb");
		if (!fp)
			fatal(1, "fopen(%s,'wb') failed\n", argv[2]);
		if (done != fwrite(obuf, 1, done, fp))
			fatal(1, "fwrite(%s,%lld) failed\n", argv[2], done);
		fclose(fp);
	}
	return 0;
}
