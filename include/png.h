/*****************************************************************************
 * Portable Network Graphics functions
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
 * $Id: png.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_PNGIMG_H_INCLUDED_)
#define	_PNGIMG_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "zlib.h"

#include "altoint.h"

#ifndef	O_BINARY
#define	O_BINARY	0
#endif


#define	COLOR_GRAYSCALE	0
#define	COLOR_RGBTRIPLE	2
#define	COLOR_PALETTE	3
#define	COLOR_GRAYALPHA	4
#define	COLOR_RGBALPHA	6

/** @brief structure of the common part of PNG and MNG context (at least) */
typedef	struct xng_s {
	/** @brief cookie passed down to input or output function */
	void *cookie;

	/** @brief input function to read from e.g. a file */
	int (*input)(void *cookie, uint8_t *data, int size);

	/** @brief output function to write to e.g. a file */
	int (*output)(void *cookie, uint8_t *data, int size);

	/** @brief ISO CRC of current PNG or MNG element */
	uint32_t crc;
}	xng_t;


/** @brief structure of a PNG context */
typedef	struct png_s {
	/** @brief common part of structure shared with MNG */
	xng_t x;

	/** @brief bytes per scanline */
	uint32_t stride;

	/** @brief image width in pixels */
	uint32_t w;

	/** @brief image height in pixels */
	uint32_t h;

	/** @brief image bits per pixel */
	uint32_t bpp;

	/** @brief color mode */
	uint8_t color;

	/** @brief depth of the elements of a color */
	uint8_t depth;

	/** @brief compression type */
	uint8_t compression;

	/** @brief filter method */
	uint8_t filter;

	/** @brief interlaced flag */
	uint8_t interlace;

	/** @brief physical pixel dimension x */
	uint32_t px;

	/** @brief physical pixel dimension y */
	uint32_t py;

	/** @brief unit for the dimensions */
	uint8_t unit;

	/** @brief time stamp C,Y,M,D,H,M,S */
	uint8_t time[7];

	/** @brief size of the sRGB chunk */
	size_t srgb_size;

	/** @brief sRGB data */
	uint8_t srgb[1];

	/** @brief gAMA data (iamge gamma) */
	uint32_t gamma;

	/** @brief size of the background color */
	size_t bkgd_size;

	/** @brief background color */
	uint8_t bkgd[6];

	/** @brief size of the transparency map */
	size_t trns_size;

	/** @brief transparency map */
	uint8_t trns[256];

	/** @brief size of the (compressed) image data */
	size_t isize;

	/** @brief offset into the (compressed) image data while reading */
	size_t ioffs;

	/** @brief compressed image data */
	uint8_t *idat;

	/** @brief offset into the uncompressed image data while reading */
	size_t offs;

	/** @brief image bitmap size in bytes */
	size_t size;

	/** @brief image bitmap data */
	uint8_t *img;

	/** @brief palette entries (used only if color mode is COLOR_PALETTE) */
	uint8_t pal[3*256];

	/** @brief comment string */
	char *comment;

	/** @brief author string */
	char *author;

}	png_t;

/** @brief convert a red, green, blue triple to a PNG color word */
#define	PNG_RGB(r,g,b)	((((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff))

/**
 * @brief reset the CRC32
 *
 * @param pcrc pointer to a CRC word
 */
extern void isocrc_reset(uint32_t *pcrc);

/**
 * @brief append a byte to the CRC32
 *
 * @param pcrc pointer to a CRC word
 * @param b byte to append
 */
extern void isocrc_byte(uint32_t *pcrc, uint8_t b);

/**
 * @brief append a number of bytes to the CRC32
 *
 * @param pcrc pointer to a CRC word
 * @param buff array of bytes to append
 * @param size number of bytes to append
 */
extern void isocrc_bytes(uint32_t *pcrc, uint8_t *buff, size_t size);

/**
 * @brief get the cookie from a xng_t struct
 *
 * @param ptr pointer to a mng_t or png_t context
 * @result returns the cookie supplied to png_create() or mng_create() calls.
 */
extern void *xng_get_cookie(void *ptr);

/**
 * @brief write a PNG/MNG size header
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param size unsigned 32 bit size value to write
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_size(void *ptr, uint32_t size);

/**
 * @brief read a PNG/MNG size header
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param psize pointer to an unsigned 32 bit size value to read
 * @result returns 0 on success, -1 on error
 */
extern int xng_read_size(void *ptr, uint32_t *psize);

/**
 * @brief write a PNG/MNG string
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param src pointer to a NUL terminated string to write
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_string(void *ptr, const char *src);

/**
 * @brief read a PNG/MNG string
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param dst pointer to a string buffer to read to
 * @param size maximum size of the string to read
 * @result returns 0 on success, -1 on error
 */
extern int xng_read_string(void *ptr, char *dst, uint32_t size);

/**
 * @brief write a PNG/MNG array of bytes
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param buff pointer to an array of bytes
 * @param size number of bytes in buff
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_bytes(void *ptr, uint8_t *buff, uint32_t size);

/**
 * @brief read a PNG/MNG array of bytes
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param buff pointer to an array of bytes
 * @param size number of bytes in buff
 * @result returns 0 on success, -1 on error
 */
extern int xng_read_bytes(void *ptr, uint8_t *buff, uint32_t size);

/**
 * @brief write a PNG/MNG byte
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param b byte to write
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_byte(void *ptr, uint8_t b);

/**
 * @brief read a PNG/MNG byte
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param pbyte pointer to byte to read
 * @result returns 0 on success, -1 on error
 */
extern int xng_read_byte(void *ptr, uint8_t *pbyte);

/**
 * @brief write a PNG/MNG unsigned 16 bit integer
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param val 16-bit unsigned integer to write
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_uint16(void *ptr, uint16_t val);

/**
 * @brief read a PNG/MNG unsigned 16 bit integer
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param pval pointer to an 16-bit unsigned integer to read
 */
extern int xng_read_uint16(void *ptr, uint16_t *pval);

/**
 * @brief write a PNG/MNG unsigned integer
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param val (32-bit) unsigned integer to write
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_uint(void *ptr, uint32_t val);

/**
 * @brief read a PNG/MNG unsigned integer
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param pval pointer to an (32-bit) unsigned integer to read
 */
extern int xng_read_uint(void *ptr, uint32_t *pval);

/**
 * @brief write a PNG/MNG signed 32 bit integer
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param i (32-bit) signed integer to write
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_int(void *ptr, int32_t i);

/**
 * @brief read a PNG/MNG signed 32 bit integer
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param pval pointer to a (32-bit) signed integer to read
 */
extern int xng_read_int(void *ptr, int32_t *pval);

/**
 * @brief write the current PNG/MNG CRC32
 *
 * @param ptr pointer to a mng_t or png_t context
 * @result returns 0 on success, -1 on error
 */
extern int xng_write_crc(void *ptr);

/**
 * @brief read the PNG/MNG CRC32 and compare with the calculated crc
 *
 * @param ptr pointer to a mng_t or png_t context
 * @result returns 0 on success, -1 on error
 */
extern int xng_read_crc(void *ptr);

/**
 * @brief read a PNG file and setup a handler to write it on png_finish()
 *
 * @param filename filename of the PNG file to read
 * @param cookie argument passed to the function to write bytes at png_finish() time
 * @param output function to write an array of bytes at png_finish() time
 * @result returns byte read, or -1 on EOF
 */
extern png_t *png_read(const char *filename,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size));

/**
 * @brief create a fresh PNG context and setup a handler to write it on png_finish()
 *
 * @param w width of the image in pixels
 * @param h height of the image in pixels
 * @param color PNG color mode to use for the image
 * @param depth number of bits per color component
 * @param cookie argument passed to the function to write bytes at png_finish() time
 * @param output function to write an array of bytes at png_finish() time
 * @result returns a pointer to a struct png_t on success, NULL on error
 */
extern png_t *png_create(int w, int h, int color, int depth,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size));

/**
 * @brief set a palette entry
 *
 * @param png pointer to a png_t context
 * @param idx palette index (0 to 255, or 1, 3, 15 for smaller palette)
 * @param color PNG_RGB() color value for the palette entry
 * @result returns 0 on success, -1 on error
 */
extern int png_set_palette(png_t *png, int idx, int color);

/**
 * @brief finish a PNG image (read or created) and write to output
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
extern int png_finish(png_t *png);

/**
 * @brief finish a PNG image to be written to a MNG stream
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
extern int png_finish_mng(png_t *png);

/**
 * @brief discard a PNG image without writing it to an output
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
extern int png_discard(png_t *png);

/**
 * @brief put a pixel into a png_t context
 *
 * @param png pointer to a png_t context
 * @param x x coordinate of the pixel (0 .. png->w - 1)
 * @param y y coordinate of the pixel (0 .. png->h - 1)
 * @param color color value (format depends on PNG color format; -1 is background color)
 * @param alpha alpha value (only used in color formats with alpha channel)
 * @result returns 0 on success, -1 on error
 */
extern int png_put_pixel(png_t *png, int x, int y, int color, int alpha);

/**
 * @brief get a pixel from a png_t context
 *
 * @param png pointer to a png_t context
 * @param x x coordinate of the pixel (0 .. png->w - 1)
 * @param y y coordinate of the pixel (0 .. png->h - 1)
 * @param color pointer to color value to return
 * @param alpha pointer to alpha value to return
 * @result returns 0 on success, -1 on error
 */
extern int png_get_pixel(png_t *png, int x, int y, int *color, int *alpha);

/**
 * @brief bit block transfer from 1bpp source
 *
 * @param png pointer to a png_t context
 * @param dx destination x
 * @param dy destination y
 * @param sx source x
 * @param sy source y
 * @param w width
 * @param h height
 * @param src source memory
 * @param stride source memory stride per scanline
 * @param sxor source memory address xor (for byte or word source)
 * @param colors array of integers with colors for source pixels
 * @param alpha alpha value (used only if png->color is a alpha-mode)
 * @result returns 0 on success, -1 on error
 */
extern int png_blit_1bpp(png_t *png, int dx, int dy, int sx, int sy,
	int w, int h, uint8_t *src, int stride, int sxor,
	int colors[], int alpha);

#endif	/* !defined(_PNGIMG_H_INCLUDED_) */
