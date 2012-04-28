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
 * $Id: png.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/

#include "png.h"

/** @brief check if standalone logging */
#if !defined(LOG)
#if	DEBUG
#define	LOG(x)	logprintf x
#define	log_MISC 0

/** @brief print a log message */
static void logprintf(int task, int level, const char *fmt, ...)
{
	va_list ap;

	if (level > 4)
		return;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}
#else	/* DEBUG */

#define	LOG(x)

#endif	/* !DEBUG */
#endif	/* !defined(LOG) */

/** @brief PNG header magic */
static uint8_t pngmagic[] = {137, 80, 78, 71, 13, 10, 26, 10};

/** @brief ISO CRC32 lookup table */
static uint32_t isocrc[] = {
	0x00000000L,0x77073096L,0xee0e612cL,0x990951baL,
	0x076dc419L,0x706af48fL,0xe963a535L,0x9e6495a3L,
	0x0edb8832L,0x79dcb8a4L,0xe0d5e91eL,0x97d2d988L,
	0x09b64c2bL,0x7eb17cbdL,0xe7b82d07L,0x90bf1d91L,
	0x1db71064L,0x6ab020f2L,0xf3b97148L,0x84be41deL,
	0x1adad47dL,0x6ddde4ebL,0xf4d4b551L,0x83d385c7L,
	0x136c9856L,0x646ba8c0L,0xfd62f97aL,0x8a65c9ecL,
	0x14015c4fL,0x63066cd9L,0xfa0f3d63L,0x8d080df5L,
	0x3b6e20c8L,0x4c69105eL,0xd56041e4L,0xa2677172L,
	0x3c03e4d1L,0x4b04d447L,0xd20d85fdL,0xa50ab56bL,
	0x35b5a8faL,0x42b2986cL,0xdbbbc9d6L,0xacbcf940L,
	0x32d86ce3L,0x45df5c75L,0xdcd60dcfL,0xabd13d59L,
	0x26d930acL,0x51de003aL,0xc8d75180L,0xbfd06116L,
	0x21b4f4b5L,0x56b3c423L,0xcfba9599L,0xb8bda50fL,
	0x2802b89eL,0x5f058808L,0xc60cd9b2L,0xb10be924L,
	0x2f6f7c87L,0x58684c11L,0xc1611dabL,0xb6662d3dL,
	0x76dc4190L,0x01db7106L,0x98d220bcL,0xefd5102aL,
	0x71b18589L,0x06b6b51fL,0x9fbfe4a5L,0xe8b8d433L,
	0x7807c9a2L,0x0f00f934L,0x9609a88eL,0xe10e9818L,
	0x7f6a0dbbL,0x086d3d2dL,0x91646c97L,0xe6635c01L,
	0x6b6b51f4L,0x1c6c6162L,0x856530d8L,0xf262004eL,
	0x6c0695edL,0x1b01a57bL,0x8208f4c1L,0xf50fc457L,
	0x65b0d9c6L,0x12b7e950L,0x8bbeb8eaL,0xfcb9887cL,
	0x62dd1ddfL,0x15da2d49L,0x8cd37cf3L,0xfbd44c65L,
	0x4db26158L,0x3ab551ceL,0xa3bc0074L,0xd4bb30e2L,
	0x4adfa541L,0x3dd895d7L,0xa4d1c46dL,0xd3d6f4fbL,
	0x4369e96aL,0x346ed9fcL,0xad678846L,0xda60b8d0L,
	0x44042d73L,0x33031de5L,0xaa0a4c5fL,0xdd0d7cc9L,
	0x5005713cL,0x270241aaL,0xbe0b1010L,0xc90c2086L,
	0x5768b525L,0x206f85b3L,0xb966d409L,0xce61e49fL,
	0x5edef90eL,0x29d9c998L,0xb0d09822L,0xc7d7a8b4L,
	0x59b33d17L,0x2eb40d81L,0xb7bd5c3bL,0xc0ba6cadL,
	0xedb88320L,0x9abfb3b6L,0x03b6e20cL,0x74b1d29aL,
	0xead54739L,0x9dd277afL,0x04db2615L,0x73dc1683L,
	0xe3630b12L,0x94643b84L,0x0d6d6a3eL,0x7a6a5aa8L,
	0xe40ecf0bL,0x9309ff9dL,0x0a00ae27L,0x7d079eb1L,
	0xf00f9344L,0x8708a3d2L,0x1e01f268L,0x6906c2feL,
	0xf762575dL,0x806567cbL,0x196c3671L,0x6e6b06e7L,
	0xfed41b76L,0x89d32be0L,0x10da7a5aL,0x67dd4accL,
	0xf9b9df6fL,0x8ebeeff9L,0x17b7be43L,0x60b08ed5L,
	0xd6d6a3e8L,0xa1d1937eL,0x38d8c2c4L,0x4fdff252L,
	0xd1bb67f1L,0xa6bc5767L,0x3fb506ddL,0x48b2364bL,
	0xd80d2bdaL,0xaf0a1b4cL,0x36034af6L,0x41047a60L,
	0xdf60efc3L,0xa867df55L,0x316e8eefL,0x4669be79L,
	0xcb61b38cL,0xbc66831aL,0x256fd2a0L,0x5268e236L,
	0xcc0c7795L,0xbb0b4703L,0x220216b9L,0x5505262fL,
	0xc5ba3bbeL,0xb2bd0b28L,0x2bb45a92L,0x5cb36a04L,
	0xc2d7ffa7L,0xb5d0cf31L,0x2cd99e8bL,0x5bdeae1dL,
	0x9b64c2b0L,0xec63f226L,0x756aa39cL,0x026d930aL,
	0x9c0906a9L,0xeb0e363fL,0x72076785L,0x05005713L,
	0x95bf4a82L,0xe2b87a14L,0x7bb12baeL,0x0cb61b38L,
	0x92d28e9bL,0xe5d5be0dL,0x7cdcefb7L,0x0bdbdf21L,
	0x86d3d2d4L,0xf1d4e242L,0x68ddb3f8L,0x1fda836eL,
	0x81be16cdL,0xf6b9265bL,0x6fb077e1L,0x18b74777L,
	0x88085ae6L,0xff0f6a70L,0x66063bcaL,0x11010b5cL,
	0x8f659effL,0xf862ae69L,0x616bffd3L,0x166ccf45L,
	0xa00ae278L,0xd70dd2eeL,0x4e048354L,0x3903b3c2L,
	0xa7672661L,0xd06016f7L,0x4969474dL,0x3e6e77dbL,
	0xaed16a4aL,0xd9d65adcL,0x40df0b66L,0x37d83bf0L,
	0xa9bcae53L,0xdebb9ec5L,0x47b2cf7fL,0x30b5ffe9L,
	0xbdbdf21cL,0xcabac28aL,0x53b39330L,0x24b4a3a6L,
	0xbad03605L,0xcdd70693L,0x54de5729L,0x23d967bfL,
	0xb3667a2eL,0xc4614ab8L,0x5d681b02L,0x2a6f2b94L,
	0xb40bbe37L,0xc30c8ea1L,0x5a05df1bL,0x2d02ef8dL
};

/**
 * @brief reset the CRC32
 *
 * @param pcrc pointer to a CRC word
 */
void isocrc_reset(uint32_t *pcrc)
{
	*pcrc = ~0;
}

/**
 * @brief append a byte to the CRC32
 *
 * @param pcrc pointer to a CRC word
 * @param b byte to append
 */
void isocrc_byte(uint32_t *pcrc, uint8_t b)
{
	uint32_t crc = *pcrc;

	*pcrc = isocrc[(uint8_t)(crc ^ b)] ^ (crc >> 8);
}

/**
 * @brief append a number of bytes to the CRC32
 *
 * @param pcrc pointer to a CRC word
 * @param buff array of bytes to append
 * @param size number of bytes to append
 */
void isocrc_bytes(uint32_t *pcrc, uint8_t *buff, size_t size)
{
	size_t i;
	uint32_t crc = *pcrc;

	for (i = 0; i < size; i++)
		crc = isocrc[(uint8_t)(crc ^ buff[i])] ^ (crc >> 8);
	*pcrc = crc;
}

/**
 * @brief get the cookie from a xng_t struct
 *
 * @param ptr pointer to a mng_t or png_t context
 * @result returns the cookie supplied to png_create() or mng_create() calls.
 */
void *xng_get_cookie(void *ptr)
{
	xng_t *xng = (xng_t *)ptr;

	if (NULL == xng)
		return NULL;
	return xng->cookie;
}

/**
 * @brief write a PNG/MNG size header
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param size unsigned 32 bit size value to write
 * @result returns 0 on success, -1 on error
 */
int xng_write_size(void *ptr, uint32_t size)
{
	xng_t *xng = (xng_t *)ptr;
	uint8_t bytes[4];
	LOG((log_MISC,1,"xng_write_size(%p,%#x)\n",
		xng, (unsigned)size));

	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}
	bytes[0] = (uint8_t)(size >> 24);
	bytes[1] = (uint8_t)(size >> 16);
	bytes[2] = (uint8_t)(size >>  8);
	bytes[3] = (uint8_t)(size >>  0);
	if (0 != (*xng->output)(xng->cookie, bytes, 4))
		return -1;
	isocrc_reset(&xng->crc);
	return 0;
}

/**
 * @brief read a PNG/MNG size header
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param psize pointer to an unsigned 32 bit size value to read
 * @result returns 0 on success, -1 on error
 */
int xng_read_size(void *ptr, uint32_t *psize)
{
	xng_t *xng = (xng_t *)ptr;
	uint8_t bytes[4];

	LOG((log_MISC,1,"xng_read_size(%p,%#x)\n",
		xng, (unsigned)psize));

	if (NULL == xng || NULL == xng->input || NULL == psize) {
		errno = EINVAL;
		return -1;
	}

	if (0 != (*xng->input)(xng->cookie, bytes, 4))
		return -1;
	*psize =
		((uint32_t)bytes[0] << 24) |
		((uint32_t)bytes[1] << 16) |
		((uint32_t)bytes[2] << 8) |
		((uint32_t)bytes[3]);
	isocrc_reset(&xng->crc);
	return 0;
}

/**
 * @brief write a PNG/MNG string
 *
 * @param ptr pointer to a mng_t or png_t context
 * @param src pointer to a NUL terminated string to write
 * @result returns 0 on success, -1 on error
 */
int xng_write_string(void *ptr, const char *src)
{
	xng_t *xng = (xng_t *)ptr;
	LOG((log_MISC,1,"xng_write_string(%p,'%s')\n",
		xng, src));
	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}
	isocrc_bytes(&xng->crc, (uint8_t *)src, strlen(src));
	if (0 != (*xng->output)(xng->cookie, (uint8_t *)src, strlen(src)))
		return -1;
	return 0;
}

/**
 * @brief read a PNG/MNG string
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param dst pointer to a string buffer to read to
 * @param size maximum size of the string to read
 * @result returns 0 on success, -1 on error
 */
int xng_read_string(void *ptr, char *dst, uint32_t size)
{
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_read_string(%p,%p,%#x)\n",
		xng, dst, (unsigned)size));

	if (NULL == xng || NULL == xng->input || NULL == dst || 0 == size) {
		errno = EINVAL;
		return -1;
	}

	if (0 != (*xng->input)(xng->cookie, (uint8_t *)dst, size))
		return -1;
	dst[size-1] = '\0';

	isocrc_bytes(&xng->crc, (uint8_t *)dst, size);

	return 0;
}

/**
 * @brief write a PNG/MNG array of bytes
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param buff pointer to an array of bytes
 * @param size number of bytes in buff
 * @result returns 0 on success, -1 on error
 */
int xng_write_bytes(void *ptr, uint8_t *buff, uint32_t size)
{
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_write_bytes(%p,%p,%d)\n",
		xng, buff, size));

	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}

	isocrc_bytes(&xng->crc, buff, size);
	if (0 != (*xng->output)(xng->cookie, buff, size))
		return -1;

	return 0;
}

/**
 * @brief read a PNG/MNG array of bytes
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param buff pointer to an array of bytes
 * @param size number of bytes in buff
 * @result returns 0 on success, -1 on error
 */
int xng_read_bytes(void *ptr, uint8_t *buff, uint32_t size)
{
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_read_bytes(%p,%p,%#x)\n",
		xng, buff, (unsigned)size));

	if (NULL == xng || NULL == xng->input || NULL == buff || 0 == size) {
		errno = EINVAL;
		return -1;
	}

	if (0 != (*xng->input)(xng->cookie, buff, size))
		return -1;

	isocrc_bytes(&xng->crc, buff, size);

	return 0;
}

/**
 * @brief write a PNG/MNG byte
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param b byte to write
 * @result returns 0 on success, -1 on error
 */
int xng_write_byte(void *ptr, uint8_t b)
{
	xng_t *xng = (xng_t *)ptr;
	LOG((log_MISC,1,"xng_write_byte(%p,%u)\n",
		xng, b));

	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}

	isocrc_byte(&xng->crc, b);
	if (0 != (*xng->output)(xng->cookie, &b, 1))
		return -1;

	return 0;
}

/**
 * @brief read a PNG/MNG byte
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param pbyte pointer to byte to read
 * @result returns 0 on success, -1 on error
 */
int xng_read_byte(void *ptr, uint8_t *pbyte)
{
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_read_byte(%p,%p)\n",
		xng, pbyte));

	if (NULL == xng || NULL == xng->input || NULL == pbyte) {
		errno = EINVAL;
		return -1;
	}

	if (0 != (*xng->input)(xng->cookie, pbyte, 1))
		return -1;

	isocrc_byte(&xng->crc, *pbyte);

	return 0;
}

/**
 * @brief write a PNG/MNG unsigned integer
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param val unsigned integer to write
 * @result returns 0 on success, -1 on error
 */
int xng_write_uint(void *ptr, uint32_t val)
{
	uint8_t bytes[4];
	xng_t *xng = (xng_t *)ptr;
	LOG((log_MISC,1,"xng_write_uint(%p,%#x)\n",
		xng, (unsigned)val));

	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}
	bytes[0] = (uint8_t)(val >> 24);
	bytes[1] = (uint8_t)(val >> 16);
	bytes[2] = (uint8_t)(val >>  8);
	bytes[3] = (uint8_t)(val >>  0);

	if (0 != xng_write_bytes(xng, bytes, 4))
		return -1;

	return 0;
}

/**
 * @brief read a PNG/MNG unsigned integer
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param pval pointer to an unsigned integer to read
 * @result returns 0 on success, -1 on error
 */
int xng_read_uint(void *ptr, uint32_t *pval)
{
	uint8_t bytes[4];
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_read_uint(%p,%#x)\n",
		xng, (unsigned)pval));
	if (NULL == xng || NULL == xng->input || NULL == pval) {
		errno = EINVAL;
		return -1;
	}
	if (0 != xng_read_bytes(xng, bytes, 4))
		return -1;
	*pval = ((uint32_t)bytes[0] << 24) |
		((uint32_t)bytes[1] << 16) |
		((uint32_t)bytes[2] <<  8) |
		((uint32_t)bytes[3] <<  0);

	return 0;
}


/**
 * @brief write a PNG/MNG 16-bit unsigned integer
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param val unsigned integer to write (16-bit)
 * @result returns 0 on success, -1 on error
 */
int xng_write_uint16(void *ptr, uint16_t val)
{
	uint8_t bytes[2];
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_write_uint16(%p,%#x)\n",
		xng, (unsigned)val));
	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}
	bytes[0] = (uint8_t)(val >> 8);
	bytes[1] = (uint8_t)(val >> 0);

	if (0 != xng_write_bytes(xng, bytes, 2))
		return -1;

	return 0;
}

/**
 * @brief read a PNG/MNG 16-bit unsigned integer
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param pval pointer to an 16-bit unsigned integer to read
 * @result returns 0 on success, -1 on error
 */
int xng_read_uint16(void *ptr, uint16_t *pval)
{
	uint8_t bytes[2];
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_read_uint16(%p,%#x)\n",
		xng, (unsigned)pval));
	if (NULL == xng || NULL == xng->input || NULL == pval) {
		errno = EINVAL;
		return -1;
	}
	if (0 != xng_read_bytes(xng, bytes, 2))
		return -1;
	*pval =	((uint32_t)bytes[0] <<  8) |
		((uint32_t)bytes[1] <<  0);

	return 0;
}

/**
 * @brief write a PNG/MNG signed integer
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param val signed integer to write (32-bit)
 * @result returns 0 on success, -1 on error
 */
int xng_write_int(void *ptr, int32_t val)
{
	uint8_t bytes[4];
	xng_t *xng = (xng_t *)ptr;
	LOG((log_MISC,1,"xng_write_int(%p,%#x)\n",
		xng, (unsigned)val));

	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}
	bytes[0] = (uint8_t)(val >> 24);
	bytes[1] = (uint8_t)(val >> 16);
	bytes[2] = (uint8_t)(val >>  8);
	bytes[3] = (uint8_t)(val >>  0);

	if (0 != xng_write_bytes(xng, bytes, 4))
		return -1;

	return 0;
}

/**
 * @brief read a PNG/MNG signed integer
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @param pval pointer to an signed integer to read (32-bit)
 * @result returns 0 on success, -1 on error
 */
int xng_read_int(void *ptr, int32_t *pval)
{
	uint8_t bytes[4];
	xng_t *xng = (xng_t *)ptr;

	LOG((log_MISC,1,"xng_read_int(%p,%#x)\n",
		xng, (unsigned)pval));
	if (NULL == xng || NULL == xng->input || NULL == pval) {
		errno = EINVAL;
		return -1;
	}
	if (0 != xng_read_bytes(xng, bytes, 4))
		return -1;
	*pval = ((uint32_t)bytes[0] << 24) |
		((uint32_t)bytes[1] << 16) |
		((uint32_t)bytes[2] <<  8) |
		((uint32_t)bytes[3] <<  0);

	return 0;
}

/**
 * @brief write the current PNG/MNG CRC32
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @result returns 0 on success, -1 on error
 */
int xng_write_crc(void *ptr)
{
	xng_t *xng = (xng_t *)ptr;
	uint32_t crc;
	uint8_t bytes[4];

	LOG((log_MISC,1,"xng_write_crc(%p) (crc:%#x)\n",
		xng, xng->crc));

	if (NULL == xng || NULL == xng->output) {
		errno = EINVAL;
		return -1;
	}

	crc = ~xng->crc;
	bytes[0] = (uint8_t)(crc >> 24);
	bytes[1] = (uint8_t)(crc >> 16);
	bytes[2] = (uint8_t)(crc >>  8);
	bytes[3] = (uint8_t)(crc >>  0);
	if (0 != (*xng->output)(xng->cookie, bytes, 4))
		return -1;
	return 0;
}

/**
 * @brief read the PNG/MNG CRC32 and compare with the calculated crc
 *
 * @param ptr pointer to a mng_t or png_t context (the common part xng_t)
 * @result returns 0 on success, -1 on error
 */
int xng_read_crc(void *ptr)
{
	xng_t *xng = (xng_t *)ptr;
	uint32_t crc;
	uint8_t bytes[4];

	LOG((log_MISC,1,"xng_read_crc(%p)\n",
		xng));

	if (NULL == xng || NULL == xng->input) {
		errno = EINVAL;
		return -1;
	}

	xng->crc = ~xng->crc;

	if (0 != (*xng->input)(xng->cookie, bytes, 4))
		return -1;
	crc = ((uint32_t)bytes[0] << 24) |
		((uint32_t)bytes[1] << 16) |
		((uint32_t)bytes[2] << 8) |
		((uint32_t)bytes[3]);

	LOG((log_MISC,5,"got CRC %08x, expect CRC %08x\n",
		crc, xng->crc));
	if (xng->crc != crc) {
		LOG((log_MISC,1,"CRC mismatch\n"));
		return -1;
	}

	return 0;
}

/**
 * @brief write a PNG IHDR (image header)
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_IHDR(png_t *png)
{
	int rc;
	LOG((log_MISC,1,"png_write_IHDR(%p)\n",
		png));

	if (0 != (rc = xng_write_size(png,13)))			/* 13 bytes */
		return rc;
	if (0 != (rc = xng_write_string(png,"IHDR")))	/* image header */
		return rc;
	if (0 != (rc = xng_write_uint(png,png->w)))
		return rc;
	if (0 != (rc = xng_write_uint(png,png->h)))
		return rc;
	if (0 != (rc = xng_write_byte(png,png->depth)))
		return rc;
	if (0 != (rc = xng_write_byte(png,png->color)))
		return rc;
	if (0 != (rc = xng_write_byte(png,png->compression)))
		return rc;
	if (0 != (rc = xng_write_byte(png,png->filter)))
		return rc;
	if (0 != (rc = xng_write_byte(png,png->interlace)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG IHDR (image header)
 *
 * @param png pointer to a png_t context
 * @param size expected size of the IHDR
 * @result returns 0 on success, -1 on error
 */
int png_read_IHDR(png_t *png, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"png_read_IHDR(%p,%#x)\n",
		png, (unsigned)size));

	/* already got size and 'IHDR' tag */
	part = 13;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	if (0 != (rc = xng_read_uint(png,&png->w))) {
		LOG((log_MISC,1,"failed to read width\n"));
		return rc;
	}
	if (0 != (rc = xng_read_uint(png,&png->h))) {
		LOG((log_MISC,1,"failed to read height\n"));
		return rc;
	}
	if (0 != (rc = xng_read_byte(png,&png->depth))) {
		LOG((log_MISC,1,"failed to read depth\n"));
		return rc;
	}
	if (0 != (rc = xng_read_byte(png,&png->color))) {
		LOG((log_MISC,1,"failed to read color mode\n"));
		return rc;
	}
	if (0 != (rc = xng_read_byte(png,&png->compression))) {
		LOG((log_MISC,1,"failed to read compression mode\n"));
		return rc;
	}
	if (0 != (rc = xng_read_byte(png,&png->filter))) {
		LOG((log_MISC,1,"failed to read filter mode\n"));
		return rc;
	}
	if (0 != (rc = xng_read_byte(png,&png->interlace))) {
		LOG((log_MISC,1,"failed to read interlace mode\n"));
		return rc;
	}
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(png,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(png))) {
		LOG((log_MISC,1,"CRC check failed\n"));
		return rc;
	}
	return 0;
}

/**
 * @brief write a PNG sRGB header (standard RGB)
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_sRGB(png_t *png)
{
	size_t size = sizeof(png->srgb);
	int rc;
	LOG((log_MISC,1,"png_write_sRGB(%p)\n",
		png));

	if (0 != (rc = xng_write_size(png, size)))	
		return rc;
	if (0 != (rc = xng_write_string(png, "sRGB")))
		return rc;
	if (0 != (rc = xng_write_bytes(png, png->srgb, size)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG sRGB header (standard RGB)
 *
 * @param png pointer to a png_t context
 * @param size expected size of a sRGB header
 * @result returns 0 on success, -1 on error
 */
int png_read_sRGB(png_t *png, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"png_read_sRGB(%p,%#x)\n",
		png, (unsigned)size));

	part = 1;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	/* already got size and 'sRGB' tag */
	if (0 != (rc = xng_read_byte(png, png->srgb)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(png,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG gAMA header (image gamma)
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_gAMA(png_t *png)
{
	size_t size = sizeof(png->gamma);
	int rc;
	LOG((log_MISC,1,"png_write_gAMA(%p)\n",
		png));

	if (0 != (rc = xng_write_size(png, size)))	
		return rc;
	if (0 != (rc = xng_write_string(png, "gAMA")))
		return rc;
	if (0 != (rc = xng_write_uint(png, png->gamma)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG gAMA header (image gamma)
 *
 * @param png pointer to a png_t context
 * @param size expected size of a gAMA header
 * @result returns 0 on success, -1 on error
 */
int png_read_gAMA(png_t *png, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"png_read_gAMA(%p,%#x)\n",
		png, (unsigned)size));

	part = 4;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	/* already got size and 'gAMA' tag */
	if (0 != (rc = xng_read_uint(png, &png->gamma)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(png,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG pHYs header (physical dimensions)
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_pHYs(png_t *png)
{
	size_t size = sizeof(png->px) + sizeof(png->py) + sizeof(png->unit);
	int rc;
	LOG((log_MISC,1,"png_write_pHYs(%p)\n",
		png));

	if (0 != (rc = xng_write_size(png, size)))	
		return rc;
	if (0 != (rc = xng_write_string(png, "pHYs")))
		return rc;
	if (0 != (rc = xng_write_uint(png, png->px)))
		return rc;
	if (0 != (rc = xng_write_uint(png, png->py)))
		return rc;
	if (0 != (rc = xng_write_byte(png, png->unit)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG pHYs header (physical dimensions)
 *
 * @param png pointer to a png_t context
 * @param size expected size of a pHYs header
 * @result returns 0 on success, -1 on error
 */
int png_read_pHYs(png_t *png, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"png_read_pHYs(%p,%#x)\n",
		png, (unsigned)size));

	part = 9;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	/* already got size and 'pHYs' tag */
	if (0 != (rc = xng_read_uint(png, &png->px)))
		return rc;
	if (0 != (rc = xng_read_uint(png, &png->py)))
		return rc;
	if (0 != (rc = xng_read_byte(png, &png->unit)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(png,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG tIME header (date and time)
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_tIME(png_t *png)
{
	uint32_t size = sizeof(png->time);
	int rc;
	LOG((log_MISC,1,"png_write_tIME(%p)\n",
		png));

	if (0 != (rc = xng_write_size(png, size)))
		return rc;
	if (0 != (rc = xng_write_string(png, "tIME")))
		return rc;
	if (0 != (rc = xng_write_bytes(png, png->time, size)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG tIME header (date and time)
 *
 * @param png pointer to a png_t context
 * @param size expected size of a tIME header
 * @result returns 0 on success, -1 on error
 */
int png_read_tIME(png_t *png, uint32_t size)
{
	uint32_t part = sizeof(png->time);
	int rc;
	LOG((log_MISC,1,"png_read_tIME(%p,%#x)\n",
		png, (unsigned)size));

	/* already got size and 'tIME' tag */
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	if (0 != (rc = xng_read_bytes(png, png->time, part)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(png,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG tEXt extension header (comment, author, ...)
 *
 * @param png pointer to a png_t context
 * @param keyword pointer to a keyword to write (including a \0 delimiter)
 * @param text pointer to a text string to write
 * @result returns 0 on success, -1 on error
 */
int png_write_tEXt(png_t *png, const char *keyword, const char *text)
{
	uint32_t size = 0;
	int rc;
	LOG((log_MISC,1,"png_write_tEXt(%p,'%s','%s')\n",
		png, keyword, text));

	if (keyword)
		size += strlen(keyword) + 1;
	if (text)
		size += strlen(text);

	if (0 != (rc = xng_write_size(png, size)))	
		return rc;
	if (0 != (rc = xng_write_string(png, "tEXt")))
		return rc;
	if (keyword) {
		if (0 != (rc = xng_write_string(png, keyword)))
			return rc;
		if (0 != (rc = xng_write_byte(png, 0)))
			return rc;
	}
	if (text) {
		if (0 != (rc = xng_write_string(png, text)))
			return rc;
	}
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG tEXt extension header (comment, author, ...)
 *
 * @param png pointer to a png_t context
 * @param dst pointer to a string buffer to read to
 * @param size maximum number of bytes in the string buffer
 * @result returns 0 on success, -1 on error
 */
int png_read_tEXt(png_t *png, char *dst, uint32_t size)
{
	int rc;
	LOG((log_MISC,1,"png_read_tEXt(%p,%p,%#x)\n",
		png, dst, (unsigned)size));

	/* already got size and 'tEXt' tag */
	if (0 != (rc = xng_read_string(png, dst, size)))
		return rc;
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG PLTE header and palette
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_PLTE(png_t *png)
{
	uint32_t entries = 1 << png->bpp;
	uint32_t size = 3 * entries;
	int rc;
	LOG((log_MISC,1,"png_write_PLTE(%p) (%u entries)\n",
		png, entries));

	if (0 != (rc = xng_write_size(png, size)))	
		return rc;
	if (0 != (rc = xng_write_string(png, "PLTE")))
		return rc;
	if (0 != (rc = xng_write_bytes(png, png->pal, size)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG PLTE header with size 0
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_0_PLTE(png_t *png)
{
	uint32_t entries = 0;
	uint32_t size = 3 * entries;
	int rc;
	LOG((log_MISC,1,"png_write_PLTE(%p) (%u entries)\n",
		png, entries));

	if (0 != (rc = xng_write_size(png, size)))	
		return rc;
	if (0 != (rc = xng_write_string(png, "PLTE")))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG PLTE header and palette
 *
 * @param png pointer to a png_t context
 * @param size maximum size of the palette (entries)
 * @result returns 0 on success, -1 on error
 */
int png_read_PLTE(png_t *png, uint32_t size)
{
	uint32_t part = sizeof(png->pal);
	uint32_t entries = 1 << png->bpp;
	int rc;
	LOG((log_MISC,1,"png_read_PLTE(%pd) (max %u entries)\n",
		png, entries));

	if (entries > size / 3) {
		LOG((log_MISC,1,"read only %u of %u palette entries\n",
			size / 3, entries));
		entries = size / 3;
	}
	if (3 * entries < part) {
		part = 3 * entries;
	}

	/* already got size and 'PLTE' tag */
	if (0 != (rc = xng_read_bytes(png, png->pal, part)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(png,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG tRNS header and transparency map
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_tRNS(png_t *png)
{
	uint32_t size = png->trns_size;
	int rc;

	LOG((log_MISC,1,"png_write_tRNS(%p) (%u bytes)\n",
		png, size));
	if (0 != (rc = xng_write_size(png, size)))	
		return rc;
	if (0 != (rc = xng_write_string(png, "tRNS")))
		return rc;
	if (0 != (rc = xng_write_bytes(png, png->trns, size)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG tRNS header and transparency map
 *
 * @param png pointer to a png_t context
 * @param size expected size of a transparency map
 * @result returns 0 on success, -1 on error
 */
int png_read_tRNS(png_t *png, uint32_t size)
{
	uint32_t part = png->trns_size;
	int rc;

	LOG((log_MISC,1,"png_read_tRNS(%p) -> %u bytes\n",
		png, part));
	if (part > size) {
		part = size;
	}
	if (0 != (rc = xng_read_bytes(png, png->trns, part)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(png,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG bKGD header and background color
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_bKGD(png_t *png)
{
	int rc;

	LOG((log_MISC,1,"png_write_bKGD(%p)\n",
		png));
	if (0 != (rc = xng_write_size(png,png->bkgd_size)))	
		return rc;
	if (0 != (rc = xng_write_string(png,"bKGD")))	/* background header */
		return rc;
	if (0 != (rc = xng_write_bytes(png,png->bkgd,png->bkgd_size)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG bKGD header and background color
 *
 * @param png pointer to a png_t context
 * @param size maximum size of the background color
 * @result returns 0 on success, -1 on error
 */
int png_read_bKGD(png_t *png, uint32_t size)
{
	int rc;
	uint32_t i;

	LOG((log_MISC,1,"png_read_bKGD(%p,%#x)\n",
		png, (unsigned)size));
	png->bkgd_size = (size > sizeof(png->bkgd)) ? sizeof(png->bkgd) : size;

	/* already got size and 'bKGD' tag */
	if (0 != (rc = xng_read_bytes(png, png->bkgd, png->bkgd_size)))
		return rc;
	for (i = png->bkgd_size; i < size; i++) {
		uint8_t ignore;
		if (0 != (rc = xng_read_byte(png,&ignore)))
			return rc;
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief write a PNG IDAT header and a chunk of compressed image data
 *
 * @param png pointer to a png_t context
 * @param idat pointer to image data to write
 * @param size number of bytes of image data
 * @result returns 0 on success, -1 on error
 */
int png_write_IDAT(png_t *png, uint8_t *idat, uint32_t size)
{
	int rc;

	LOG((log_MISC,1,"png_write_IDAT(%p,%p,%#x)\n",
		png, idat, (unsigned)size));
	if (0 != (rc = xng_write_size(png, size)))
		return rc;
	if (0 != (rc = xng_write_string(png,"IDAT")))	/* image data */
		return rc;
	if (0 != (rc = xng_write_bytes(png, idat, size)))
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG IDAT header and compressed image data
 *
 * @param png pointer to a png_t context
 * @param size maximum number of bytes to read
 * @result returns 0 on success, -1 on error
 */
int png_read_IDAT(png_t *png, uint32_t size)
{
	int rc;

	LOG((log_MISC,1,"png_read_IDAT(%p,%p,%#x) %#x/%#x\n",
		png, &png->idat[png->ioffs], (unsigned)size,
		(unsigned)png->ioffs, (unsigned)png->isize));
	/* already got size and 'IDAT' tag */
	if (0 != (rc = xng_read_bytes(png, &png->idat[png->ioffs], size)))
		return rc;
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	png->ioffs += size;
	return 0;
}

/**
 * @brief write a PNG IEND header
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_write_IEND(png_t *png)
{
	int rc;

	LOG((log_MISC,1,"png_write_IEND(%p)\n",
		png));
	if (0 != (rc = xng_write_size(png,0)))	
		return rc;
	if (0 != (rc = xng_write_string(png,"IEND")))	/* end of PNG */
		return rc;
	if (0 != (rc = xng_write_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief read a PNG IEND header
 *
 * @param png pointer to a png_t context
 * @param size expected number of bytes of header data
 * @result returns 0 on success, -1 on error
 */
int png_read_IEND(png_t *png, uint32_t size)
{
	uint32_t i;
	int rc;

	LOG((log_MISC,1,"png_read_IEND(%p,%#x)\n",
		png, (unsigned)size));
	/* already got size and 'IEND' tag */
	for (i = 0; i < size; i++) {
		uint8_t ignore;
		if (0 != (rc = xng_read_byte(png,&ignore)))
			return rc;
	}
	if (0 != (rc = xng_read_crc(png)))	
		return rc;
	return 0;
}

/**
 * @brief finish a PNG image (read or created) and write to output
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_finish(png_t *png)
{
	uLong gzsize;
	int rc = -1;

	if (NULL == png) {
		errno = EINVAL;
		goto bailout;
	}

	LOG((log_MISC,1,"png_finish(%p)\n",
		png));

	if (NULL == png->x.output)
		goto bailout;

	/* write PNG magic */
	if (0 != (*png->x.output)(png->x.cookie, pngmagic, 8)) {
		goto bailout;
	}

	if (0 != (rc = png_write_IHDR(png))) {
		goto bailout;
	}
	/* write sRGB chunk */
	if (png->srgb_size > 0) {
		if (0 != (rc = png_write_sRGB(png)))
			goto bailout;
		/* also write the gAMA chunk */
		if (0 != (rc = png_write_gAMA(png)))
			goto bailout;
	}
	/* write author text, if any */
	if (NULL != png->author) {
		if (0 != (rc = png_write_tEXt(png, "Author", png->author)))
			goto bailout;
	}
	/* write comment text, if any */
	if (NULL != png->comment) {
		if (0 != (rc = png_write_tEXt(png, "Comment", png->comment)))
			goto bailout;
	}
	/* write palette, if color mode is palette */
	if (png->color == COLOR_PALETTE) {
		if (0 != (rc = png_write_PLTE(png)))
			goto bailout;
		if (0 != (rc = png_write_bKGD(png)))
			goto bailout;
	}
	/* write pHYs chunk, if physical dimensions are defined */
	if (0 != png->px && 0 != png->py) {
		if (0 != (rc = png_write_pHYs(png)))
			goto bailout;
	}

	gzsize = 16 + png->size + png->size / 4;
	png->idat = malloc(gzsize);
	if (NULL == png->idat) {
		LOG((log_MISC,1,"malloc(%d) call failed (%s)\n",
			gzsize, strerror(errno)));
		rc = -1;
		goto bailout;
	}

	if (Z_OK != (rc = compress2(png->idat, &gzsize, png->img, png->size, 9))) {
		LOG((log_MISC,1,"compress2(%p,%#x,%#p,%#x,%d) call failed (%d)\n",
			png->idat, gzsize,
			png->img, png->size, 9, rc));
		goto bailout;
	}
	png->isize = gzsize;
	if (0 != (rc = png_write_IDAT(png,png->idat,png->isize)))
		goto bailout;
	if (0 != (rc = png_write_IEND(png)))
		goto bailout;

bailout:
	if (NULL != png && NULL != png->idat) {
		free(png->idat);
		png->idat = NULL;
	}
	if (NULL != png && NULL != png->img) {
		free(png->img);
		png->img = NULL;
	}
	if (NULL != png) {
		free(png);
		png = NULL;
	}
	return rc;
}

/**
 * @brief finish a PNG image to be written to a MNG stream
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
int png_finish_mng(png_t *png)
{
	uLong gzsize;
	int rc = -1;

	if (NULL == png) {
		errno = EINVAL;
		goto bailout;
	}

	LOG((log_MISC,1,"png_finish_mng(%p)\n",
		png));

	if (NULL == png->x.output) {
		goto bailout;
	}

	if (0 != (rc = png_write_IHDR(png))) {
		goto bailout;
	}

	/* TODO: does MNG actually _expect_ a 0 length palette from us? */
	if (png->color == COLOR_PALETTE) {
		if (0 != (rc = png_write_0_PLTE(png)))
			goto bailout;
	}

	if (0 != png->px && 0 != png->py) {
		if (0 != (rc = png_write_pHYs(png)))
			goto bailout;
	}

	gzsize = 16 + png->size + png->size / 4;
	png->idat = malloc(gzsize);
	if (NULL == png->idat) {
		LOG((log_MISC,1,"malloc(%d) call failed (%s)\n",
			gzsize, strerror(errno)));
		rc = -1;
		goto bailout;
	}

	/* use a medium compression level to speed things up */
	if (Z_OK != (rc = compress2(png->idat, &gzsize, png->img, png->size, 5))) {
		LOG((log_MISC,1,"compress2(%p,%#x,%#p,%#x,%d) call failed (%d)\n",
			png->idat, gzsize,
			png->img, png->size, 5, rc));
		goto bailout;
	}
	png->isize = gzsize;
	if (0 != (rc = png_write_IDAT(png,png->idat,png->isize)))
		goto bailout;
	if (0 != (rc = png_write_IEND(png)))
		goto bailout;

bailout:
	if (NULL != png) {
		if (NULL != png->idat) {
			free(png->idat);
			png->idat = NULL;
			png->isize = 0;
			png->ioffs = 0;
		}
		if (NULL != png->img) {
			free(png->img);
			png->img = NULL;
			png->size = 0;
		}
		free(png);
		png = NULL;
	}
	return rc;
}

/**
 * @brief discard a PNG image without writing it to an output
 *
 * @param png pointer to a png_t context
 * @result returns 0 on success, -1 on error
 */
extern int png_discard(png_t *png)
{
	int rc = -1;

	LOG((log_MISC,1,"png_discard(%p)\n",
		png));

	/* nothing else to do */
	rc = 0;

	if (NULL != png) {
		if (NULL != png->idat) {
			free(png->idat);
			png->idat = NULL;
			png->isize = 0;
			png->ioffs = 0;
		}
		if (NULL != png->img) {
			free(png->img);
			png->img = NULL;
			png->size = 0;
		}
		free(png);
		png = NULL;
	}
	return rc;
}

/**
 * @brief read a PNG byte from a file descriptor
 *
 * @param cookie pointer to an integer (fd)
 * @result returns byte read, or -1 on EOF
 */
int fp_read_bytes(void *cookie, uint8_t *buff, int size)
{
	FILE *fp = (FILE *)cookie;
	if (size != fread(buff, 1, size, fp))
		return -1;
	return 0;
}

/**
 * @brief read a PNG file and setup a handler to write it on png_finish()
 *
 * @param filename filename of the PNG file to read
 * @param cookie argument passed to the function to write bytes at png_finish() time
 * @param output function to write an array of bytes at png_finish() time
 * @result returns pointer new png_t context on success, NULL on error
 */
png_t *png_read(const char *filename,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size))
{
	png_t *png = NULL;
	uint8_t magic[8];
	char tag[4+1];
	uint32_t size;
	uint32_t i, x, y, sub, up;
	FILE *fp;
	int iend, rc;

	LOG((log_MISC,1,"png_read('%s')\n",
		filename));

	fp = fopen(filename, "rb");
	if (NULL == fp) {
		LOG((log_MISC,1,"fopen('%s','rb') call failed (%s)\n",
			filename, strerror(errno)));
		return NULL;
	}
	png = (png_t *)calloc(1, sizeof(png_t));
	if (NULL == png) {
		LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
			1, sizeof(png_t), strerror(errno)));
		return NULL;
	}

	png->x.cookie = fp;
	png->x.input = fp_read_bytes;

	/* read PNG magic */
	if (0 != (*png->x.input)(png->x.cookie, magic, sizeof(magic))) {
		LOG((log_MISC,1,"PNG magic read failed\n", i));
		free(png);
		fclose(fp);
		return NULL;
	}
	if (memcmp(magic, pngmagic, sizeof(magic))) {
		LOG((log_MISC,1,"PNG magic mismatch\n"));
		free(png);
		fclose(fp);
		return NULL;
	}

	iend = 0;
	while (0 == iend) {
		if (0 != (rc = xng_read_size(png, &size))) {
			LOG((log_MISC,1,"PNG read size failed (%s)\n",
				strerror(errno)));
			goto bailout;
		}
		if (0 != (rc = xng_read_string(png, tag, 4))) {
			LOG((log_MISC,1,"PNG read tag failed (%s)\n",
				strerror(errno)));
			goto bailout;
		}
		LOG((log_MISC,5,"PNG '%s' size %#x\n", tag, size));
		if (0 == strcmp(tag, "IHDR")) {
			if (0 != (rc = png_read_IHDR(png, size))) {
				LOG((log_MISC,1,"PNG read IHDR failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
			switch (png->color) {
			case COLOR_GRAYSCALE:
				switch (png->depth) {
				case 1:
					png->stride = 1 + (png->w + 7) / 8;
					png->bpp = 1;
					png->trns_size = 2;
					LOG((log_MISC,5,"PNG COLOR_GRAYSCALE 1bpp: %#x\n",
						png->stride));
					break;
				case 2:
					png->stride = 1 + (png->w + 3) / 4;
					png->bpp = 2;
					png->trns_size = 2;
					LOG((log_MISC,5,"PNG COLOR_GRAYSCALE 2bpp: %#x\n",
						png->stride));
					break;
				case 4:
					png->stride = 1 + (png->w + 1) / 2;
					png->bpp = 4;
					png->trns_size = 2;
					LOG((log_MISC,5,"PNG COLOR_GRAYSCALE 4bpp: %#x\n",
						png->stride));
					break;
				case 8:
					png->stride = 1 + png->w;
					png->bpp = 8;
					png->trns_size = 2;
					LOG((log_MISC,5,"PNG COLOR_GRAYSCALE 8bpp: %#x\n",
						png->stride));
					break;
				case 16:
					png->stride = 1 + 2 * png->w;
					png->bpp = 16;
					png->trns_size = 2;
					LOG((log_MISC,5,"PNG COLOR_GRAYSCALE 16bpp: %#x\n",
						png->stride));
					break;
				default:
					LOG((log_MISC,1,"unsupported COLOR_GRAYSCALE depth %d\n",
						png->depth));
					rc = -1;
					goto bailout;
				}
				break;
			case COLOR_RGBTRIPLE:
				switch (png->depth) {
				case 8:
					png->stride = 1 + 3 * png->w;
					png->bpp = 3 * 8;
					png->trns_size = 6;
					LOG((log_MISC,5,"PNG COLOR_RGBTRIPLE 8bpp: %#x\n",
						png->stride));
					break;
				case 16:
					png->stride = 1 + 6 * png->w;
					png->bpp = 3 * 16;
					png->trns_size = 6;
					LOG((log_MISC,5,"PNG COLOR_RGBTRIPLE 16bpp: %#x\n",
						png->stride));
					break;
				default:
					LOG((log_MISC,1,"unsupported COLOR_RGBTRIPLE depth %d\n",
						png->depth));
					rc = -1;
					goto bailout;
				}
				break;
			case COLOR_PALETTE:
				switch (png->depth) {
				case 1:
					png->stride = 1 + (png->w + 7) / 8;
					png->bpp = 1;
					png->trns_size = 2;
					LOG((log_MISC,5,"PNG COLOR_PALETTE 1bpp: %#x\n",
						png->stride));
					break;
				case 2:
					png->stride = 1 + (png->w + 3) / 4;
					png->bpp = 2;
					png->trns_size = 4;
					LOG((log_MISC,5,"PNG COLOR_PALETTE 2bpp: %#x\n",
						png->stride));
					break;
				case 4:
					png->stride = 1 + (png->w + 1) / 2;
					png->bpp = 4;
					png->trns_size = 16;
					LOG((log_MISC,5,"PNG COLOR_PALETTE 4bpp: %#x\n",
						png->stride));
					break;
				case 8:
					png->stride = 1 + png->w;
					png->bpp = 8;
					png->trns_size = 256;
					LOG((log_MISC,5,"PNG COLOR_PALETTE 8bpp: %#x\n",
						png->stride));
					break;
				default:
					LOG((log_MISC,1,"unsupported COLOR_PALETTE depth %d\n",
						png->depth));
					rc = -1;
					goto bailout;
				}
				break;
			case COLOR_GRAYALPHA:
				switch (png->depth) {
				case 8:
					png->stride = 1 + 2 * png->w;
					png->bpp = 2 * 8;
					png->trns_size = 0;
					LOG((log_MISC,5,"PNG COLOR_GRAYALPHA 8bpp: %#x\n",
						png->stride));
					break;
				case 16:
					png->stride = 1 + 4 * png->w;
					png->bpp = 2 * 16;
					png->trns_size = 0;
					LOG((log_MISC,5,"PNG COLOR_GRAYALPHA 16bpp: %#x\n",
						png->stride));
					break;
				default:
					LOG((log_MISC,1,"unsupported COLOR_GRAYALPHA depth %d\n",
						png->depth));
					rc = -1;
					goto bailout;
				}
				break;
			case COLOR_RGBALPHA:
				switch (png->depth) {
				case 8:
					png->stride = 1 + 4 * png->w;
					png->bpp = 4 * 8;
					png->trns_size = 0;
					LOG((log_MISC,5,"PNG COLOR_RGBALPHA 8bpp: %#x\n",
						png->stride));
					break;
				case 16:
					png->stride = 1 + 2 * 4 * png->w;
					png->bpp = 4 * 16;
					png->trns_size = 0;
					LOG((log_MISC,5,"PNG COLOR_RGBALPHA 16bpp: %#x\n",
						png->stride));
					break;
				default:
					LOG((log_MISC,1,"unsupported COLOR_RGBALPHA depth %d\n",
						png->depth));
					rc = -1;
					goto bailout;
				}
				break;
			default:
				LOG((log_MISC,1,"unsupported color model %d, depth %d\n",
					png->color, png->depth));
				rc = -1;
				goto bailout;
			}
			png->size = png->h * png->stride;
			LOG((log_MISC,5,"PNG image size is %#x, %ux%ux%u, %u bytes/row\n",
				png->size, png->w, png->h, png->bpp, png->stride));
			png->img = calloc(png->size, sizeof(uint8_t));
			if (NULL == png->img) {
				LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
					png->size, sizeof(uint8_t), strerror(errno)));
				rc = -1;
				goto bailout;
			}
		} else if (0 == strcmp(tag, "tEXt")) {
			char *tmp = calloc(size + 1, sizeof(char));
			if (NULL == tmp) {
				LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
					size + 1, sizeof(char), strerror(errno)));
				rc = -1;
				goto bailout;
			}
			if (0 != (rc = png_read_tEXt(png, tmp, size))) {
				LOG((log_MISC,1,"PNG read 'tEXt' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
			LOG((log_MISC,1,"tEXt: '%s'\n", tmp));
			free(tmp);
		} else if (0 == strcmp(tag, "sRGB")) {
			if (0 != (rc = png_read_sRGB(png, size))) {
				LOG((log_MISC,1,"PNG read 'sRGB' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "gAMA")) {
			if (0 != (rc = png_read_gAMA(png, size))) {
				LOG((log_MISC,1,"PNG read 'gAMA' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "PLTE")) {
			if (0 != (rc = png_read_PLTE(png, size))) {
				LOG((log_MISC,1,"PNG read 'PLTE' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "bKGD")) {
			if (0 != (rc = png_read_bKGD(png, size))) {
				LOG((log_MISC,1,"PNG read 'bKGD' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "pHYs")) {
			if (0 != (rc = png_read_pHYs(png, size))) {
				LOG((log_MISC,1,"PNG read 'pHYs' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "tIME")) {
			if (0 != (rc = png_read_tIME(png, size))) {
				LOG((log_MISC,1,"PNG read 'tIME' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "tRNS")) {
			if (0 != (rc = png_read_tRNS(png, size))) {
				LOG((log_MISC,1,"PNG read 'tRNS' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "IDAT")) {
			if (0 == png->isize) {
				png->isize = size;
				png->idat = malloc(png->isize);
			} else {
				if (png->ioffs + size >= png->isize) {
					if (png->isize * 2 >= png->ioffs + size) {
						png->isize *= 2;
					} else {
						png->isize += size;
					}
					png->idat = realloc(png->idat, png->isize);
				}
			}
			if (NULL == png->idat) {
				LOG((log_MISC,1,"malloc/realloc(%d) call failed (%s)\n",
					png->isize, strerror(errno)));
				rc = -1;
				goto bailout;
			}
			if (0 != (rc = png_read_IDAT(png,size))) {
				LOG((log_MISC,1,"PNG read 'IDAT' failed (%s)\n",
					strerror(errno)));
				free(png->idat);
				goto bailout;
			}
		} else if (0 == strcmp(tag, "IEND")) {
			if (0 != (rc = png_read_IEND(png, size))) {
				LOG((log_MISC,1,"PNG read IEND failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
			iend = 1;
			if (png->isize > 0) {
				uLong gzsize = png->size;
				rc = uncompress(png->img, &gzsize, png->idat, png->isize);
				if (Z_OK != rc) {
					LOG((log_MISC,1,"uncompress(%p,%#x,%p,%#x) failed (%d)\n",
						png->img, (unsigned)gzsize,
						png->idat, (unsigned)png->isize, rc));
					goto bailout;
				}
				LOG((log_MISC,3,"PNG","uncompress(%p,%#x,%p,%#x) success (%d)\n",
					png->img, (unsigned)gzsize,
					png->idat, (unsigned)png->isize, rc));
				png->offs += gzsize;
			}
		} else {
			LOG((log_MISC,1,"PNG ignore tag '%s'\n", tag));
			for (i = 0; i < size; i++) {
				uint8_t ignore;
				xng_read_byte(png, &ignore);
			}
			if (0 != (rc = xng_read_crc(png))) {
				LOG((log_MISC,1,"PNG crc of unknown tag '%s' failed\n",
					tag));
				goto bailout;
			}
		}
	}
	fclose(fp);
	fp = NULL;

	/* undo the filters */
	for (y = 0; y < (uint32_t)png->h; y++) {
		uint8_t *row = png->img + png->stride * y;
		switch (row[0]) {
		case 0:
			/* no filter */
			break;
		case 1:
			/* sub filter */
			row[0] = 0;
			sub = (png->bpp + 7) / 8;
			for (x = 1; x < png->stride; x++)
				row[x] = row[x] + (x >= sub ? row[x - sub] : 0);
			break;
		case 2:
			/* up filter */
			row[0] = 0;
			if (y > 0) {
				up = png->stride;
				for (x = 1; x < png->stride; x++)
					row[x] = row[x] + row[x - up];
			}
			break;
		case 3:
			/* average filter */
			row[0] = 0;
			sub = (png->bpp + 7) / 8;
			up = png->stride;
			if (y > 0) {
				for (x = 1; x < png->stride; x++)
					if (x >= sub)
						row[x] = row[x] + (row[x - sub] + row[x - up]) / 2;
					else
						row[x] = row[x] + row[x - up] / 2;
			} else {
				for (x = 1; x < png->stride; x++)
					if (x >= sub)
						row[x] = row[x] + row[x - sub] / 2;
			}
			break;
		case 4:
			/* Paeth filter */
			row[0] = 0;
			sub = (png->bpp + 7) / 8;
			up = png->stride;
			for (x = 1; x < png->stride; x++) {
				uint8_t a = 0, b = 0, c = 0;
				int p, pa, pb, pc;
				if (x >= sub)
					a = row[x - sub];
				if (y > 0)
					b = row[x - up];
				if (x >= sub && y > 0)
					c = row[x - sub - up];
				p = a + b - c;
				pa = abs(p - a);
				pb = abs(p - b);
				pc = abs(p - c);
				if (pa <= pb && pa <= pc) {
					row[x] = row[x] + a;
				} else if (pb <= pc) {
					row[x] = row[x] + b;
				} else {
					row[x] = row[x] + c;
				}
			}
			break;
		default:
			LOG((log_MISC,1,"unknown filter %#x in row %d\n",
				row[0], y));
		}
	}

	/* done with input */
	png->x.input = NULL;
	/* set the output cookie data and function */
	png->x.cookie = cookie;
	png->x.output = output;
	rc = 0;

bailout:
	if (NULL != fp) {
		fclose(fp);
		fp = NULL;
	}
	if (NULL != png && NULL != png->idat) {
		free(png->idat);
		png->idat = NULL;
		png->isize = 0;
		png->ioffs = 0;
	}
	if (0 != rc) {
		if (NULL != png && NULL != png->img) {
			free(png->img);
			png->img = NULL;
		}
		if (NULL != png) {
			free(png);
			png = NULL;
		}
	}
	return png;
}

/**
 * @brief create a fresh PNG context and setup a handler to write it on png_finish()
 *
 * @param w width of the image in pixels
 * @param h height of the image in pixels
 * @param color PNG color mode to use for the image
 * @param depth number of bits per color component
 * @param cookie argument passed to the function to write bytes at png_finish() time
 * @param output function to write an array of bytes at png_finish() time
 * @result returns pointer new png_t context on success, NULL on error
 */
png_t *png_create(int w, int h, int color, int depth,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size))
{
	png_t *png;

	LOG((log_MISC,1,"png_create(%d,%d,%d,%d,%p,%p)\n",
		w, h, color, depth, cookie, output));

	png = (png_t *)calloc(1, sizeof(png_t));
	if (NULL == png) {
		LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
			1, sizeof(png_t), strerror(errno)));
		return NULL;
	}
	png->w = w;
	png->h = h;
	png->color = color;
	png->depth = depth;

	/* default sRGB chunk (?) */
	png->srgb_size = sizeof(png->srgb);
	/*
	 * set standard RGB to "saturation":
	 * Saturation intent is for images preferring preservation
	 * of saturation at the expense of hue and lightness, like
	 * charts and graphs.
	 */
	png->srgb[0] = 2;
	/*
	 * set default gamma
	 * An application that writes the sRGB chunk should also write
	 * a gAMA chunk (and perhaps a cHRM chunk) for compatibility
	 * with applications that do not use the sRGB chunk.
	 * In this situation, only the following values may be used: 
	 *    gAMA:
	 *    Gamma: 45455
	 */
	png->gamma = 45455;

	switch (color) {
	case COLOR_GRAYSCALE:
		switch (depth) {
		case 1:
			png->stride = 1 + (png->w + 7) / 8;
			png->bpp = 1;
			png->bkgd_size = 2;
			png->trns_size = 2;
			break;
		case 2:
			png->stride = 1 + (png->w + 3) / 4;
			png->bpp = 2;
			png->bkgd_size = 2;
			png->trns_size = 2;
			break;
		case 4:
			png->stride = 1 + (png->w + 1) / 2;
			png->bpp = 4;
			png->bkgd_size = 2;
			png->trns_size = 2;
			break;
		case 8:
			png->stride = 1 + png->w;
			png->bpp = 8;
			png->bkgd_size = 2;
			png->trns_size = 2;
			break;
		case 16:
			png->stride = 1 + 2 * png->w;
			png->bpp = 16;
			png->bkgd_size = 2;
			png->trns_size = 2;
			break;
		default:
			free(png);
			return NULL;
		}
		LOG((log_MISC,1,"Grayscale bpp:%d stride:%d\n",
			png->bpp, png->stride));
		break;
	case COLOR_RGBTRIPLE:
		switch (depth) {
		case 8:
			png->stride = 1 + 3 * png->w;
			png->bpp = 24;
			png->bkgd_size = 6;
			png->trns_size = 6;
			break;
		case 16:
			png->stride = 1 + 6 * png->w;
			png->bpp = 48;
			png->bkgd_size = 6;
			png->trns_size = 6;
			break;
		default:
			free(png);
			return NULL;
		}
		LOG((log_MISC,1,"RGB triple bpp:%d stride:%d\n",
			png->bpp, png->stride));
		break;
	case COLOR_PALETTE:
		switch (depth) {
		case 1:
			png->stride = 1 + (png->w + 7) / 8;
			png->bpp = 1;
			png->bkgd_size = 1;
			png->trns_size = 2;
			break;
		case 2:
			png->stride = 1 + (png->w + 3) / 4;
			png->bpp = 2;
			png->bkgd_size = 1;
			png->trns_size = 4;
			break;
		case 4:
			png->stride = 1 + (png->w + 1) / 2;
			png->bpp = 4;
			png->bkgd_size = 1;
			png->trns_size = 16;
			break;
		case 8:
			png->stride = 1 + png->w;
			png->bpp = 8;
			png->bkgd_size = 1;
			png->trns_size = 256;
			break;
		default:
			free(png);
			return NULL;
		}
		LOG((log_MISC,1,"Palette bpp:%d stride:%d\n",
			png->bpp, png->stride));
		break;
	case COLOR_GRAYALPHA:
		switch (depth) {
		case 8:
			png->stride = 1 + 2 * png->w;
			png->bpp = 16;
			png->bkgd_size = 2;
			png->trns_size = 0;
			break;
		case 16:
			png->stride = 1 + 4 * png->w;
			png->bpp = 32;
			png->bkgd_size = 2;
			png->trns_size = 0;
			break;
		default:
			free(png);
			return NULL;
		}
		LOG((log_MISC,1,"Gray + Alpha bpp:%d stride:%d\n",
			png->bpp, png->stride));
		break;
	case COLOR_RGBALPHA:
		switch (depth) {
		case 8:
			png->stride = 1 + 4 * png->w;
			png->bpp = 32;
			png->bkgd_size = 6;
			png->trns_size = 0;
			break;
		case 16:
			png->stride = 1 + 8 * png->w;
			png->bpp = 64;
			png->bkgd_size = 6;
			png->trns_size = 0;
			break;
		default:
			free(png);
			return NULL;
		}
		LOG((log_MISC,1,"RGB + Alpha bpp:%d stride:%d\n",
			png->bpp, png->stride));
		break;
	default:
		free(png);
		return NULL;
	}
	png->size = png->h * png->stride;
	png->img = calloc(png->size, sizeof(uint8_t));
	if (NULL == png->img) {
		LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
			png->size, sizeof(uint8_t), strerror(errno)));
		free(png);
		return NULL;
	}
	LOG((log_MISC,1,"image is %d bytes (width:%d height:%d stride:%d)\n",
			png->size, png->w, png->h, png->stride));
	/* set the output cookie data and function */
	png->x.cookie = cookie;
	png->x.output = output;

	return png;
}

/**
 * @brief set a palette entry
 *
 * @param png pointer to a png_t context
 * @param idx palette index (0 to 255, or 1, 3, 15 for smaller palette)
 * @param color PNG_RGB() color value for the palette entry
 * @result returns 0 on success, -1 on error
 */
int png_set_palette(png_t *png, int idx, int color)
{
	uint32_t entry;

	if (idx < 0 || idx >= 256)
		return -EINVAL;
	entry = 3 * idx;
	png->pal[entry+0] = color >> 16;	/* red */
	png->pal[entry+1] = color >> 8;		/* green */
	png->pal[entry+2] = color;		/* blue */

	return 0;
}

/**
 * @brief set a pixel in a png_t context
 *
 * @param png pointer to a png_t context
 * @param x x coordinate of the pixel (0 .. png->w - 1)
 * @param y y coordinate of the pixel (0 .. png->h - 1)
 * @param color color value (format depends on PNG color format; -1 is background color)
 * @param alpha alpha value (only used in color formats with alpha channel)
 * @result returns 0 on success, -1 on error
 */
int png_put_pixel(png_t *png, int x, int y, int color, int alpha)
{
	uint32_t offs, bitn;
	uint8_t mask;

	if (NULL == png || NULL == png->img) {
		errno = EINVAL;
		return -1;
	}

	/* out of bounds check */
	if (x < 0 || (uint32_t)x >= png->w || y < 0 || (uint32_t)y >= png->h) {
		LOG((log_MISC,5,"png_put_pixel(%p,%d,%d,%d,%d) out of bounds\n",
			png, x, y, color, alpha));
		return 0;
	}

	if (-1 == color) {
		switch (png->bkgd_size) {
		case 1:
			color = png->bkgd[0];
			break;
		case 2:
			color = (png->bkgd[0] << 8) | png->bkgd[1];
			break;
		case 3:
			color = (png->bkgd[0] << 16) | (png->bkgd[1] << 8) | png->bkgd[2];
			break;
		case 6:
			color = (png->bkgd[0] << 16) | (png->bkgd[2] << 8) | png->bkgd[4];
			break;
		default:
			color = png->bkgd[0];
		}
	}

	switch (png->color) {
	case COLOR_GRAYSCALE:
		switch (png->depth) {
		case 1:
			offs = 1 + y * png->stride + x / 8;
			bitn = x % 8;
			mask = 0x80 >> bitn;
			png->img[offs] = (png->img[offs] & ~mask) |
				((color & 1) << (7 - bitn));
			break;
		case 2:
			offs = 1 + y * png->stride + x / 4;
			bitn = 2 * (x % 4);
			mask = 0xc0 >> bitn;
			png->img[offs] = (png->img[offs] & ~mask) |
				((color & 3) << (6 - bitn));
			break;
		case 4:
			offs = 1 + y * png->stride + x / 2;
			bitn = 4 * (x % 2);
			mask = 0xf0 >> bitn;
			png->img[offs] = (png->img[offs] & ~mask) |
				((color & 15) << (4 - bitn));
			break;
		case 8:
			offs = 1 + y * png->stride + x;
			png->img[offs] = color;
			break;
		case 16:
			offs = 1 + y * png->stride + x * 2;
			png->img[offs+0] = color >> 8;
			png->img[offs+1] = color;
			break;
		}
		break;
	case COLOR_RGBTRIPLE:
		switch (png->depth) {
		case 8:
			offs = 1 + y * png->stride + x * 3;
			png->img[offs+0] = color >> 16;
			png->img[offs+1] = color >> 8;
			png->img[offs+2] = color;
			break;
		case 16:
			offs = 1 + y * png->stride + x * 6;
			png->img[offs+0] = color >> 16;
			png->img[offs+1] = 0;
			png->img[offs+2] = color >> 8;
			png->img[offs+3] = 0;
			png->img[offs+4] = color;
			png->img[offs+5] = 0;
			break;
		}
		break;
	case COLOR_PALETTE:
		switch (png->depth) {
		case 1:
			offs = 1 + y * png->stride + x / 8;
			bitn = x % 8;
			mask = 0x80 >> bitn;
			png->img[offs] = (png->img[offs] & ~mask) |
				((color & 1) << (7 - bitn));
			break;
		case 2:
			offs = 1 + y * png->stride + x / 4;
			bitn = 2 * (x % 4);
			mask = 0xc0 >> bitn;
			png->img[offs] = (png->img[offs] & ~mask) |
				((color & 3) << (6 - bitn));
			break;
		case 4:
			offs = 1 + y * png->stride + x / 2;
			bitn = 4 * (x % 2);
			mask = 0xf0 >> bitn;
			png->img[offs] = (png->img[offs] & ~mask) |
				((color & 15) << (4 - bitn));
			break;
		case 8:
			offs = 1 + y * png->stride + x;
			png->img[offs] = color;
			break;
		}
		break;
	case COLOR_GRAYALPHA:
		switch (png->depth) {
		case 8:
			offs = 1 + y * png->stride + x * 2;
			png->img[offs+0] = color;
			png->img[offs+1] = alpha;
			break;
		case 16:
			offs = 1 + y * png->stride + x * 4;
			png->img[offs+0] = color >> 8;
			png->img[offs+1] = color;
			png->img[offs+2] = alpha >> 8;
			png->img[offs+3] = alpha;
			break;
		}
		break;
	case COLOR_RGBALPHA:
		switch (png->depth) {
		case 8:
			offs = 1 + y * png->stride + x * 4;
			png->img[offs+0] = color >> 16;
			png->img[offs+1] = color >> 8;
			png->img[offs+2] = color;
			png->img[offs+3] = alpha;
			break;
		case 16:
			offs = 1 + y * png->stride + x * 8;
			png->img[offs+0] = color >> 16;
			png->img[offs+1] = 0;
			png->img[offs+2] = color >> 8;
			png->img[offs+3] = 0;
			png->img[offs+4] = color;
			png->img[offs+5] = 0;
			png->img[offs+6] = alpha >> 8;
			png->img[offs+7] = alpha;
			break;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return 0;
}

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
int png_get_pixel(png_t *png, int x, int y, int *color, int *alpha)
{
	uint32_t offs;
	int bkgd;

	if (NULL == png || NULL == png->img || NULL == color || NULL == alpha) {
		errno = EINVAL;
		return -1;
	}
	*color = 0;
	*alpha = 0;

	if (x < 0 || (uint32_t)x >= png->w || y < 0 || (uint32_t)y >= png->h) {
		LOG((log_MISC,5,"png_get_pixel(%p,%d,%d,%d,%d) out of bounds\n",
			png, x, y, color, alpha));
		return 0;
	}

	switch (png->bkgd_size) {
	case 1:
		bkgd = png->bkgd[0];
		break;
	case 2:
		bkgd = (png->bkgd[0] << 8) | png->bkgd[1];
		break;
	case 3:
		bkgd = (png->bkgd[0] << 16) | (png->bkgd[1] << 8) | png->bkgd[2];
		break;
	case 6:
		bkgd = (png->bkgd[0] << 16) | (png->bkgd[2] << 8) | png->bkgd[4];
		break;
	default:
		bkgd = png->bkgd[0];
	}

	switch (png->color) {
	case COLOR_GRAYSCALE:
		switch (png->depth) {
		case 1:
			offs = 1 + y * png->stride + x / 8;
			*color = (png->img[offs] >> (x % 8)) & 1;
			break;
		case 2:
			offs = 1 + y * png->stride + x / 4;
			*color = (png->img[offs] >> (2 * (x % 4))) & 3;
			break;
		case 4:
			offs = 1 + y * png->stride + x / 2;
			*color = (png->img[offs] >> (4 * (x % 2))) & 15;
			break;
		case 8:
			offs = 1 + y * png->stride + x;
			*color = png->img[offs];
			break;
		case 16:
			offs = 1 + y * png->stride + x * 2;
			*color = (png->img[offs+0] << 8) | png->img[offs+1];
			break;
		}
		break;
	case COLOR_RGBTRIPLE:
		switch (png->depth) {
		case 8:
			offs = 1 + y * png->stride + x * 3;
			*color =
				(png->img[offs+0] << 16) |
				(png->img[offs+1] <<  8) |
				(png->img[offs+2] <<  0);
			break;
		case 16:
			offs = 1 + y * png->stride + x * 6;
			*color =
				(png->img[offs+0] << 16) |
				(png->img[offs+2] <<  8) |
				(png->img[offs+4] <<  0);
			break;
		}
		break;
	case COLOR_PALETTE:
		switch (png->depth) {
		case 1:
			offs = 1 + y * png->stride + x / 8;
			*color = (png->img[offs] >> (x % 8)) & 1;
			break;
		case 2:
			offs = 1 + y * png->stride + x / 4;
			*color = (png->img[offs] >> (2 * (x % 4))) & 3;
			break;
		case 4:
			offs = 1 + y * png->stride + x / 2;
			*color = (png->img[offs] >> (4 * (x % 2))) & 15;
			break;
		case 8:
			offs = 1 + y * png->stride + x;
			*color = png->img[offs];
			break;
		}
		break;
	case COLOR_GRAYALPHA:
		switch (png->depth) {
		case 8:
			offs = 1 + y * png->stride + x * 2;
			*color = png->img[offs+0];
			*alpha = png->img[offs+1];
			break;
		case 16:
			offs = 1 + y * png->stride + x * 4;
			*color = (png->img[offs+0] << 8) | png->img[offs+1];
			*alpha = (png->img[offs+2] << 8) | png->img[offs+3];
			break;
		}
		break;
	case COLOR_RGBALPHA:
		switch (png->depth) {
		case 8:
			offs = 1 + y * png->stride + x * 4;
			*color =
				(png->img[offs+0] << 16) |
				(png->img[offs+1] <<  8) |
				(png->img[offs+2] <<  0);
			*alpha = png->img[offs+3];
			break;
		case 16:
			offs = 1 + y * png->stride + x * 8;
			*color =
				(png->img[offs+0] << 16) |
				(png->img[offs+2] <<  8) |
				(png->img[offs+4] <<  0);
			*alpha = (png->img[offs+6] << 8) | png->img[offs+7];
			break;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (bkgd == *color) {
		*color = -1;
	}

	return 0;
}

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
 * @param sxor source offset XOR value (to address bytes of little endian words)
 * @param colors array of integers with colors for source pixels
 * @param alpha alpha value (used only if png->color is a alpha-mode)
 * @result returns 0 on success, -1 on error
 */
int png_blit_1bpp(png_t *png, int dx, int dy, int sx, int sy,
	int w, int h, uint8_t *src, int stride, int sxor,
	int colors[], int alpha)
{
	int x, y, sbit, dbit;
	uint32_t soff, doff, sacc, dacc, mask;

	/* clipping to PNG dimensions */
	if (dx < 0) {
		w += dx;
		sx += dx;
		dx = 0;
	}
	if (dy < 0) {
		h += dy;
		sy += dy;
		dy = 0;
	}
	if (dx + w > png->w) {
		w = png->w - dx;
	}
	if (dy + h > png->h) {
		h = png->h - dy;
	}
	if (w <= 0 || h <= 0)
		return 0;

	switch (png->color) {
	case COLOR_GRAYSCALE:
		switch (png->depth) {
		case 1:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx / 8;
				dbit = dx & 7;
				dacc = png->img[doff];
				mask = 0x80 >> dbit;
				for (x = 0; x < w; x++) {
					dacc = (dacc & ~mask) |
						((colors[(sacc>>7)&1] << (7-dbit)) & mask);
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
					if (dbit == 7) {
						dbit = 0;
						png->img[doff++] = dacc;
						dacc = png->img[doff];
						mask = 0x80;
					} else {
						dbit++;
						mask >>= 1;
					}
				}
				png->img[doff] = dacc;
			}
			break;
		case 2:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << (7 - sbit);
				doff = 1 + (dy + y) * png->stride + dx / 4;
				dbit = (dx & 3) << 1;
				dacc = png->img[doff];
				mask = 0xc0 >> dbit;
				for (x = 0; x < w; x++) {
					dacc = (dacc & ~mask) |
						((colors[(sacc>>7)&1] << (6-dbit)) & mask);
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
					if (dbit == 6) {
						dbit = 0;
						png->img[doff++] = dacc;
						dacc = png->img[doff];
						mask = 0xc0;
					} else {
						dbit += 2;
						mask >>= 2;
					}
				}
				png->img[doff] = dacc;
			}
			break;
		case 4:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx / 2;
				dbit = (dx & 1) << 2;
				dacc = png->img[doff];
				mask = 0xf0 >> dbit;
				for (x = 0; x < w; x++) {
					dacc = (dacc & ~mask) |
						((colors[(sacc>>7)&1] << (4-dbit)) & mask);
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
					if (dbit == 4) {
						dbit = 0;
						png->img[doff++] = dacc;
						dacc = png->img[doff];
						mask = 0xf0;
					} else {
						dbit += 4;
						mask >>= 4;
					}
				}
				png->img[doff] = dacc;
			}
			break;
		case 8:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx;
				mask = 0xff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		case 16:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx * 2;
				mask = 0xffff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc >> 8;
					png->img[doff++] = dacc;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	case COLOR_RGBTRIPLE:
		switch (png->depth) {
		case 8:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx * 3;
				mask = 0xffffff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc >> 16;
					png->img[doff++] = dacc >> 8;
					png->img[doff++] = dacc;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		case 16:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx * 6;
				mask = 0xffffff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc >> 16;
					png->img[doff++] = 0;
					png->img[doff++] = dacc >> 8;
					png->img[doff++] = 0;
					png->img[doff++] = dacc;
					png->img[doff++] = 0;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	case COLOR_PALETTE:
		switch (png->depth) {
		case 1:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx / 8;
				dbit = dx & 7;
				dacc = png->img[doff];
				mask = 0x80 >> dbit;
				for (x = 0; x < w; x++) {
					dacc = (dacc & ~mask) |
						((colors[(sacc>>7)&1] << (7-dbit)) & mask);
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
					if (dbit == 7) {
						dbit = 0;
						png->img[doff++] = dacc;
						dacc = png->img[doff];
						mask = 0x80;
					} else {
						dbit++;
						mask >>= 1;
					}
				}
				png->img[doff] = dacc;
			}
			break;
		case 2:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx / 4;
				dbit = (dx & 3) << 1;
				dacc = png->img[doff];
				mask = 0xc0 >> dbit;
				for (x = 0; x < w; x++) {
					dacc = (dacc & ~mask) |
						((colors[(sacc>>7)&1] << (6-dbit)) & mask);
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
					if (dbit == 6) {
						dbit = 0;
						png->img[doff++] = dacc;
						dacc = png->img[doff];
						mask = 0xc0;
					} else {
						dbit += 2;
						mask >>= 2;
					}
				}
				png->img[doff] = dacc;
			}
			break;
		case 4:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx / 2;
				dbit = (dx & 1) << 2;
				dacc = png->img[doff];
				mask = 0xf0 >> dbit;
				for (x = 0; x < w; x++) {
					dacc = (dacc & ~mask) |
						((colors[(sacc>>7)&1] << (4-dbit)) & mask);
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
					if (dbit == 4) {
						dbit = 0;
						png->img[doff++] = dacc;
						dacc = png->img[doff];
						mask = 0xf0;
					} else {
						dbit += 4;
						mask >>= 4;
					}
				}
				png->img[doff] = dacc;
			}
			break;
		case 8:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx;
				mask = 0xff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	case COLOR_GRAYALPHA:
		switch (png->depth) {
		case 8:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx * 2;
				mask = 0xff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc;
					png->img[doff++] = alpha;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		case 16:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx * 4;
				mask = 0xffff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc >> 8;
					png->img[doff++] = dacc;
					png->img[doff++] = alpha >> 8;
					png->img[doff++] = alpha;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	case COLOR_RGBALPHA:
		switch (png->depth) {
		case 8:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx * 4;
				mask = 0xffffff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc >> 16;
					png->img[doff++] = dacc >> 8;
					png->img[doff++] = dacc;
					png->img[doff++] = alpha;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		case 16:
			for (y = 0; y < h; y++) {
				soff = (sy + y) * stride + sx / 8;
				sbit = sx & 7;
				sacc = src[sxor ^ soff++] << sbit;
				doff = 1 + (dy + y) * png->stride + dx * 8;
				mask = 0xffffff;
				for (x = 0; x < w; x++) {
					dacc = colors[(sacc>>7)&1] & mask;
					png->img[doff++] = dacc >> 16;
					png->img[doff++] = 0;
					png->img[doff++] = dacc >> 8;
					png->img[doff++] = 0;
					png->img[doff++] = dacc;
					png->img[doff++] = 0;
					if (sbit == 7) {
						sbit = 0;
						sacc = src[sxor ^ soff++];
					} else {
						sbit++;
						sacc <<= 1;
					}
				}
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return 0;
}
