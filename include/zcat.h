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
 * zcat compatible functions
 *
 * $Id: zcat.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_ZCAT_H_INCLUDED_)
#define	_ZCAT_H_INCLUDED_

/**
 * @brief copy a block of memory and uncompress it (LZW) on the fly
 *
 * @param op pointer to output buffer
 * @param osize number of bytes in output buffer
 * @param ip pointer to input buffer
 * @param isize number of bytes in input buffer
 * @result returns the size of the inflate()
 */
off_t z_copy(uint8_t *op, size_t osize, uint8_t *ip, size_t isize);

/**
 * @brief copy a block of memory and uncompress it (libz) on the fly
 *
 * @param op pointer to output buffer
 * @param osize number of bytes in output buffer
 * @param ip pointer to input buffer
 * @param isize number of bytes in input buffer
 * @result returns the size of the inflate()
 */
off_t gz_copy(uint8_t *op, size_t osize, uint8_t *ip, size_t isize);

/**
 * @brief analyze a compress()d header and return the number of bytes to skip
 *
 * @param ip input buffer pointer
 * @param size size of input buffer in bytes
 * @param pskip optional pointer to a size_t receiving the header bytes to skip
 * @result return 0 for unknown, 1 for compress, 2 for gzip
 */
extern int z_header(uint8_t *ip, size_t size, size_t *pskip);

#endif	/* !defined(_ZCAT_H_INCLUDED_) */
