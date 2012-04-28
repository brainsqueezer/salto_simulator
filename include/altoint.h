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
 * A try to get round the common int types mess that's out there
 *
 * $Id: altoint.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_ALTOINT_H_INCLUDED_)
#define	_ALTOINT_H_INCLUDED_

#if defined(HAVE_STDINT_H) && HAVE_STDINT_H
#include <unistd.h>
#include <stdint.h>

#else	/* HAVE_STDINT_H */

#ifndef	int8_t
typedef	signed char		_my_int8_t;
#define	int8_t			_my_int8_t
#endif
#ifndef	uint8_t
typedef	unsigned char		_my_uint8_t;
#define	uint8_t			_my_uint8_t
#endif
#ifndef	int16_t
typedef	short int		_my_int16_t;
#define	int16_t			_my_int16_t
#endif
#ifndef	uint16_t
typedef	unsigned short int	_my_uint16_t;
#define	uint16_t		_my_uint16_t
#endif
#ifndef	int32_t
typedef	int			_my_int32_t;
#define	int32_t			_my_int32_t
#endif
#ifndef	uint32_t
typedef	unsigned int		_my_uint32_t;
#define	uint32_t		_my_uint32_t
#endif

#ifdef	__COMPILER_INT64__
#ifndef	int64_t
typedef	__COMPILER_INT64__	_my_int64_t;
#define	int64_t			_my_int64_t
#endif
#else	/* __COMPILER_INT64__ */
#ifndef	int64_t
typedef	long long int		_my_int64_t;
#define	int64_t			_my_int64_t
#endif
#endif	/* !__COMPILER_INT64__ */

#ifdef	__COMPILER_UINT64__
#ifndef	uint64_t
typedef	__COMPILER_UINT64__	_my_uint64_t;
#define	uint64_t		_my_uint64_t
#endif
#else	/* __COMPILER_UINT64__ */
#ifndef	uint64_t
typedef	unsigned long long int	_my_uint64_t;
#define	uint64_t		_my_uint64_t
#endif
#endif	/* !__COMPILER_UINT64__ */

#endif	/* !HAVE_STDINT */

/**
 * @brief bit field macros
 * Some macros to make it easier to access variable by the
 * bit-reversed notation that the Xerox Alto docs use all
 * over the place.
 * Bit number 0 is the most significant there, and number
 * width-1 is the least significant.
 */

/** @brief get the left shift required to access bit 'to' in a word of 'width' bits */
#define BITSHIFT(width,to) ((width) - 1 - (to))

/** @brief build a least significant bit mask for bits 'from' to 'to' (inclusive) */
#define BITMASK(from,to) ((1 << ((to) + 1 - (from))) - 1)

/** @brief get a bit field from 'reg', a word of 'width' bits, starting at 'from' until 'to' */
#define	ALTO_GET(reg,width,from,to) \
	(((reg) >> BITSHIFT(width,to)) & BITMASK(from,to))

/** @brief put a value 'val' into 'reg', a word of 'width' bits, starting at 'from' until 'to' */
#define	ALTO_PUT(reg,width,from,to,val) do { \
	(reg) = ((reg) & ~(BITMASK(from,to) << BITSHIFT(width,to))) | \
		(((val) & BITMASK(from,to)) << BITSHIFT(width,to)); \
} while (0)

/** @brief get a single bit number 'bit' value from 'reg', a word of 'width' bits */
#define	ALTO_BIT(reg,width,bit) \
	(((reg) >> BITSHIFT(width,bit)) & 1)

#endif /* !defined(_ALTOINT_H_INCLUDED_) */
