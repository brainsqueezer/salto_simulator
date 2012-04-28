/*****************************************************************************
 * Multiple Image Network Grapics - VLC (very low complexity)
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
 * $Id: mng.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/

#include "png.h"
#include "mng.h"

/** @brief check if standalone logging */
#if !defined(LOG)
#if	DEBUG
#define	LOG(x)	logprintf x
#define	log_MISC 0

/** @brief print a log message */
static void logprintf(int task, int level, const char *fmt, ...)
{
	va_list ap;

	if (level > 0)
		return;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}
#else	/* DEBUG */

#define	LOG(x)

#endif	/* !DEBUG */
#endif	/* !defined(LOG) */

/** @brief MNG header magic */
static uint8_t mngmagic[] = {138, 77, 78, 71, 13, 10, 26, 10};


/**
 * @brief write a MNG MHDR (stream header)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_MHDR(mng_t *mng)
{
	size_t size = 7 * 4;
	int rc;
	LOG((log_MISC,1,"mng_write_MHDR(%p)\n",
		mng));

	if (0 != (rc = xng_write_size(mng,size)))		/* 28 bytes */
		return rc;
	if (0 != (rc = xng_write_string(mng,"MHDR")))		/* stream header */
		return rc;
	if (0 != (rc = xng_write_uint(mng,mng->w)))
		return rc;
	if (0 != (rc = xng_write_uint(mng,mng->h)))
		return rc;
	if (0 != (rc = xng_write_uint(mng,mng->ticks)))
		return rc;
	if (0 != (rc = xng_write_uint(mng,mng->lcount)))
		return rc;
	if (0 != (rc = xng_write_uint(mng,mng->fcount)))
		return rc;
	if (0 != (rc = xng_write_uint(mng,mng->ptime)))
		return rc;
	if (0 != (rc = xng_write_uint(mng,mng->profile)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG MHDR (stream header)
 *
 * @param mng pointer to a mng_t context
 * @param size size of the MHDR chunk found
 * @result returns 0 on success, -1 on error
 */
int mng_read_MHDR(mng_t *mng, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"mng_read_MHDR(%p,%#x)\n",
		mng, (unsigned)size));

	/* already got size and 'MHDR' tag */
	part = 7 * 4;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	if (0 != (rc = xng_read_uint(mng,&mng->w))) {
		LOG((log_MISC,1,"failed to read width\n"));
		return rc;
	}
	if (0 != (rc = xng_read_uint(mng,&mng->h))) {
		LOG((log_MISC,1,"failed to read height\n"));
		return rc;
	}
	if (0 != (rc = xng_read_uint(mng,&mng->ticks))) {
		LOG((log_MISC,1,"failed to read ticks per second\n"));
		return rc;
	}
	if (0 != (rc = xng_read_uint(mng,&mng->lcount))) {
		LOG((log_MISC,1,"failed to read nominal layer count\n"));
		return rc;
	}
	if (0 != (rc = xng_read_uint(mng,&mng->fcount))) {
		LOG((log_MISC,1,"failed to read nominal frame count\n"));
		return rc;
	}
	if (0 != (rc = xng_read_uint(mng,&mng->ptime))) {
		LOG((log_MISC,1,"failed to read nominal play time\n"));
		return rc;
	}
	if (0 != (rc = xng_read_uint(mng,&mng->profile))) {
		LOG((log_MISC,1,"failed to read simplicty profile flags\n"));
		return rc;
	}
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng))) {
		LOG((log_MISC,1,"CRC check failed\n"));
		return rc;
	}
	return 0;
}

/**
 * @brief write a MNG TERM header
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_TERM(mng_t *mng)
{
	uint32_t size = sizeof(mng->term_action) + sizeof(mng->term_after) +
		sizeof(mng->term_delay) + sizeof(mng->term_maxiter);
	int rc;

	LOG((log_MISC,1,"mng_write_TERM(%p)\n",
		mng));
	if (0 != (rc = xng_write_size(mng,size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng,"TERM")))
		return rc;
	if (0 != (rc = xng_write_byte(mng, mng->term_action)))
		return rc;
	if (0 != (rc = xng_write_byte(mng, mng->term_after)))
		return rc;
	if (0 != (rc = xng_write_uint(mng, mng->term_delay)))
		return rc;
	if (0 != (rc = xng_write_uint(mng, mng->term_maxiter)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG TERM header
 *
 * @param mng pointer to a mng_t context
 * @param size expected number of bytes of header data
 * @result returns 0 on success, -1 on error
 */
int mng_read_TERM(mng_t *mng, uint32_t size)
{
	uint32_t part;
	int rc;

	LOG((log_MISC,1,"mng_read_TERM(%p,%#x)\n",
		mng, (unsigned)size));

	part = 10;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	/* already got size and 'TERM' tag */
	if (0 != (rc = xng_read_byte(mng, &mng->term_action)))
		return rc;
	if (0 != (rc = xng_read_byte(mng, &mng->term_after)))
		return rc;
	if (0 != (rc = xng_read_uint(mng, &mng->term_delay)))
		return rc;
	if (0 != (rc = xng_read_uint(mng, &mng->term_maxiter)))
		return rc;

	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG BACK header and background color
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_BACK(mng_t *mng)
{
	int rc;

	LOG((log_MISC,1,"mng_write_BACK(%p)\n",
		mng));
	if (0 != (rc = xng_write_size(mng,mng->back_size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng,"BACK")))	/* background header */
		return rc;
	if (0 != (rc = xng_write_bytes(mng,mng->back,mng->back_size)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG BACK header and background color
 *
 * possible sizes:
 *  6	R(16), G(16), B(16)
 *  7	R(16), G(16), B(16), mandatory flag
 *  9	R(16), G(16), B(16), mandatory flag, A(16)
 *  10	R(16), G(16), B(16), mandatory flag, A(16), ??
 *
 * @param mng pointer to a mng_t context
 * @param size maximum size of the background color
 * @result returns 0 on success, -1 on error
 */
int mng_read_BACK(mng_t *mng, uint32_t size)
{
	int rc;
	uint32_t i;

	LOG((log_MISC,1,"mng_read_BACK(%p,%#x)\n",
		mng, (unsigned)size));
	mng->back_size = (size > sizeof(mng->back)) ? sizeof(mng->back) : size;

	/* already got size and 'BACK' tag */
	if (0 != (rc = xng_read_bytes(mng, mng->back, mng->back_size)))
		return rc;
	for (i = mng->back_size; i < size; i++) {
		uint8_t ignore;
		if (0 != (rc = xng_read_byte(mng,&ignore)))
			return rc;
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG FRAM header
 *
 * Framing_mode:  1 byte.
 *   
 *  0:  Do not change framing mode.
 *   
 *  1:  No background layer is generated, except for one
 *      ahead of the very first foreground layer in the
 *      datastream.  The interframe delay is associated
 *      with each foreground layer in the subframe.
 *   
 *  2:  No background layer is generated, except for one
 *      ahead of the very first image in the datastream.
 *      The interframe delay is associated only with the
 *      final layer in the subframe.  A zero interframe
 *      delay is associated with the other layers in the
 *      subframe.
 *   
 *  3:  A background layer is generated ahead of each
 *      foreground layer.  The interframe delay is
 *      associated with each foreground layer, and a zero
 *      delay is associated with each background layer.
 *   
 *  4:  The background layer is generated only ahead of
 *      the first foreground layer in the subframe.  The
 *      interframe delay is associated only with the final
 *      foreground layer in the subframe.  A zero
 *      interframe delay is associated with the background
 *      layers, except when there is no foreground layer
 *      in the subframe, in which case the interframe delay
 *      is associated with the sole background layer.
 *   
 * Subframe_name:
 *      0 or more bytes (Latin-1 Text).
 *      Can be omitted; if so, the subframe is nameless.
 *   
 * Separator:
 *      1 byte: (null). 
 *      Must be omitted if the subsequent fields are also omitted.
 *   
 * Change_interframe_delay:
 *      1 byte.
 *        0:  No.
 *        1:  Yes, for the upcoming subframe only.
 *        2:  Yes, also reset default.
 *      This field and all subsequent fields can be omitted as a
 *      group if no frame parameters other than the framing mode
 *      or the subframe name are changed.
 *   
 * Change_timeout_and_termination:
 *      1 byte
 *        0:  No.
 *        1:  Deterministic, for the upcoming subframe only.
 *        2:  Deterministic, also reset default.
 *        3:  Decoder-discretion, for the upcoming subframe only.
 *        4:  Decoder-discretion, also reset default.
 *        5:  User-discretion, for the upcoming subframe only.
 *        6:  User-discretion, also reset default.
 *        7:  External-signal, for the upcoming subframe only.
 *        8:  External-signal, also reset default.
 *      This field can be omitted only if the previous field is
 *      also omitted.
 *   
 * Change_layer_clipping_boundaries:
 *      1 byte.
 *        0:  No.
 *        1:  Yes, for the upcoming subframe only.
 *        2:  Yes, also reset default.
 *      This field can be omitted only if the previous field is
 *      also omitted.
 *   
 * Change_sync_id_list:
 *      1 byte.
 *        0:  No.
 *        1:  Yes, for the upcoming subframe only.
 *        2:  Yes, also reset default list.
 *      This field can be omitted only if the previous field is
 *      also omitted.
 *   
 * Interframe_delay:
 *      4 bytes (unsigned integer).  This field must be omitted
 *        if the change_interframe_delay field is zero or is
 *        omitted.  The range is [0..2^31-1] ticks.
 *   
 * Timeout:
 *      4 bytes (unsigned integer).  This field must be omitted
 *        if the change_timeout_and_termination field is zero or
 *        is omitted.  The range is [0..2^31-1].  The value
 *        2^31-1 (0x7fffffff) ticks represents an infinite
 *        timeout period.
 *   
 * Layer_clipping_boundary_delta_type:
 *      1 byte (unsigned integer).
 *        0: Layer clipping boundary values are given directly.
 *        1: Layer clipping boundaries are determined by adding
 *           the FRAM data to the values from the previous
 *           subframe.
 *      This and the following four fields must be omitted if the
 *      change_layer_clipping_boundaries field is zero or is
 *      omitted.
 *   
 * Left_layer_cb or Delta_left_layer_cb:
 *      4 bytes (signed integer).
 *   
 * Right_layer_cb or Delta_right_layer_cb:
 *      4 bytes (signed integer).
 *   
 * Top_layer_cb or Delta_top_layer_cb:
 *      4 bytes (signed integer).
 *   
 * Bottom_layer_cb or Delta_bottom_layer_cb:
 *      4 bytes (signed integer).
 *   
 * Sync_id:
 *      4 bytes (unsigned integer).  Must be omitted if
 *        change_sync_id_list=0 and can be omitted if the new
 *        list is empty; repeat until all sync_ids have been
 *        listed.  The range is [0..2^31-1].
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_FRAM(mng_t *mng)
{
	uint32_t size;
	int w_ifdelay = mng->ifdelay != mng->ifdelay_default;
	int w_timeout = mng->timeout != mng->timeout_default;
	int w_clipchg = mng->ll_cb != mng->ll_cb_default ||
		mng->lr_cb != mng->lr_cb_default ||
		mng->lt_cb != mng->lt_cb_default ||
		mng->lb_cb != mng->lb_cb_default;
	int w_syncids = mng->syncid_size > 0;
	int w_any = w_ifdelay || w_timeout || w_clipchg || w_syncids;
	int rc;

	LOG((log_MISC,1,"mng_write_FRAM(%p)\n",
		mng));

	/* if the the interframe delay is changed, also change the default */
	w_ifdelay = w_ifdelay ? 2 : 0;
	/* if the the timeout is changed, also change the default */
	w_timeout = w_timeout ? 2 : 0;
	/* if the clipping is changed, also change the default */
	w_clipchg = w_clipchg ? 2 : 0;

	size = 	1 +				/* framing mode */
		0;				/* subframe name (none) */

	/* see if we need to write anything but the framing mode */
	if (w_any) {
		size = size +
			1 +			/* separator */
			1 +			/* change interframe delay flag */
			1 +			/* change timeout and termination flag */
			1 +			/* change clipping flag */
			1;			/* change sync id list flag */
		if (w_ifdelay) { 		/* new interframe delay */
			size = size + 4;
		}
		if (w_timeout) {		/* new timeout value */
			size = size + 4;
		}
		if (w_clipchg) {		/* clipping type flag and coordinates*/
			size = size + 1 + 4 * 4;
		}
		if (w_syncids) {		/* sync id list elements */
			size = size + mng->syncid_size * 4;
		}
	}

	if (0 != (rc = xng_write_size(mng,size)))
		return rc;
	if (0 != (rc = xng_write_string(mng,"FRAM")))		/* frame header */
		return rc;
	if (0 != (rc = xng_write_byte(mng,1)))			/* framing mode: 1 */
		return rc;
	/* subframe name: omitted */
	if (w_any) {
		if (0 != (rc = xng_write_byte(mng,0)))		/* separator after subframe name */
			return rc;
		if (0 != (rc = xng_write_byte(mng,w_ifdelay)))	/* change interfame delay flag */
			return rc;
		if (0 != (rc = xng_write_byte(mng,w_timeout)))	/* change timeout + termination flag */
			return rc;
		if (0 != (rc = xng_write_byte(mng,w_clipchg ? 2 : 0)))	/* change clipping flag */
			return rc;
		if (0 != (rc = xng_write_byte(mng,w_syncids)))	/* change sync id list */
			return rc;
		if (w_ifdelay) {				/* new interframe delay in ticks */
			if (0 != (rc = xng_write_uint(mng,mng->ifdelay)))
				return rc;
			if (w_ifdelay == 2) {
				/* change default */
				mng->ifdelay_default = mng->ifdelay;
			}
		}
		if (w_timeout) {				/* new timout */
			if (0 != (rc = xng_write_int(mng,mng->timeout)))
				return rc;
			if (w_timeout == 2) {
				/* change defaults */
				mng->timeout_default = mng->timeout;
			}
		}
		if (w_clipchg) {				/* clipping type: 0 direct */
			if (0 != (rc = xng_write_byte(mng,0)))
				return rc;
			if (0 != (rc = xng_write_int(mng,mng->ll_cb)))
				return rc;
			if (0 != (rc = xng_write_int(mng,mng->lr_cb)))
				return rc;
			if (0 != (rc = xng_write_int(mng,mng->lt_cb)))
				return rc;
			if (0 != (rc = xng_write_int(mng,mng->lb_cb)))
				return rc;
			if (w_clipchg == 2) {
				/* change defaults */
				mng->ll_cb_default = mng->ll_cb;
				mng->lr_cb_default = mng->lr_cb;
				mng->lt_cb_default = mng->lt_cb;
				mng->lb_cb_default = mng->lb_cb;
			}
		}
		if (w_syncids) {
			int i;
			for (i = 0; i < mng->syncid_size; i++)
				if (0 != (rc = xng_write_int(mng,mng->syncid[i])))
					return rc;
		}
	}

	if (0 != (rc = xng_write_crc(mng)))	
		return rc;

	/* success; adjust playtime (is specified in ticks) */
	mng->ptime += mng->ifdelay;

	return 0;
}

/**
 * @brief write a MNG DEFI header (object definition)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_DEFI(mng_t *mng)
{
	uint32_t size;
	int w_clipchg = mng->l_cb != mng->ll_cb ||
		mng->r_cb != mng->lr_cb ||
		mng->t_cb != mng->lt_cb ||
		mng->b_cb != mng->lb_cb;
	int w_xyloc = w_clipchg || mng->xloc != 0 || mng->yloc != 0;
	int w_concrete = w_xyloc || 0;			/* not yet */
	int w_do_not_show = w_concrete || 0;		/* not yet */
	int w_any = w_do_not_show || w_concrete || w_xyloc || w_clipchg;
	int rc;

	/* don't need to write anything if neither x/y location,
	 * nor clipping is changed
	 */
	if (!w_any)
		return 0;

	size = 2;			/* object ID (must be 0 in LC profile) */
	if (w_do_not_show) {		/* do_not_show flag */
		size = size + 1;
	}
	if (w_concrete) {		/* concrete_flag */
		size = size + 1;
	}
	if (w_xyloc) {			/* x and y location: 2 coordinates */
		size = size + 2 * 4;
	}
	if (w_clipchg) {		/* change clipping: 4 coordinates */
		size = size + 4 * 4;
	}
	LOG((log_MISC,1,"mng_write_DEFI(%p)\n",
		mng));
	if (0 != (rc = xng_write_size(mng,size)))
		return rc;
	if (0 != (rc = xng_write_string(mng,"DEFI")))		/* object definition header */
		return rc;
	if (0 != (rc = xng_write_uint16(mng,0)))		/* object ID must be 0 in LC profile */
		return rc;
	if (w_do_not_show) {
		if (0 != (rc = xng_write_byte(mng,0)))		/* do_not_show flag = 0 */
			return rc;
	}
	if (w_concrete) {
		if (0 != (rc = xng_write_byte(mng,0)))		/* concrete_flag = 0 */
			return rc;
	}
	if (w_xyloc) {
		if (0 != (rc = xng_write_int(mng,mng->xloc)))	/* x location */
			return rc;
		if (0 != (rc = xng_write_int(mng,mng->yloc)))	/* y location */
			return rc;
	}
	if (w_clipchg) {
		if (0 != (rc = xng_write_int(mng,mng->l_cb)))	/* left clipping boundary */
			return rc;
		if (0 != (rc = xng_write_int(mng,mng->r_cb)))	/* right clipping boundary */
			return rc;
		if (0 != (rc = xng_write_int(mng,mng->t_cb)))	/* top clipping boundary */
			return rc;
		if (0 != (rc = xng_write_int(mng,mng->b_cb)))	/* bottom clipping boundary */
			return rc;
	}
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG DEFI header and background color
 *
 * The DEFI chunk contains 2, 3, 4, 12, or 28 bytes.
 * If any field is omitted, all subsequent fields must also be omitted.
 *
 *  Object_id:
 *    2 bytes (unsigned integer) identifier to be given to the
 *      objects that follow the DEFI chunk.  This field must be
 *      zero in MNG-LC files.
 *    
 *  Do_not_show:
 *    1 byte (unsigned integer)
 *      0:  Make the objects potentially visible.
 *      1:  Make the objects not potentially visible.
 *    
 *  Concrete_flag:
 *    1 byte (unsigned integer)
 *      0:  Make the objects "abstract" (image cannot be the
 *          source for a Delta-PNG)
 *      1:  Make the objects "concrete" (object can be the
 *          source for a Delta-PNG).
 *    MNG-LC decoders can ignore this flag.
 *    
 *  X_location:
 *    4 bytes (signed integer).
 *     The X_location and Y_location fields can be omitted as
 *     a pair.
 *    
 *  Y_location:
 *    4 bytes (signed integer).
 *    
 *  Left_cb:
 *    4 bytes (signed integer).  Left clipping boundary. The
 *      l_cb, r_cb, t_cb, and b_cb fields can be
 *      omitted as a group.
 *    
 *  Right_cb:
 *    4 bytes (signed integer).
 *    
 *  Top_cb:
 *    4 bytes (signed integer).
 *    
 *  Bottom_cb:
 *    4 bytes (signed integer).
 *
 * @param mng pointer to a mng_t context
 * @param size maximum size of the background color
 * @result returns 0 on success, -1 on error
 */
int mng_read_DEFI(mng_t *mng, uint32_t size)
{
	uint8_t defi[28];
	uint32_t part;
	int rc;
	uint32_t i;

	LOG((log_MISC,1,"mng_read_DEFI(%p,%#x)\n",
		mng, (unsigned)size));
	part = (size > sizeof(defi)) ? sizeof(defi) : size;

	memset(defi, 0, sizeof(defi));

	/* 16: r_cb default is width */
	defi[16] = (uint8_t)(mng->w >> 24);
	defi[17] = (uint8_t)(mng->w >> 16);
	defi[18] = (uint8_t)(mng->w >>  8);
	defi[19] = (uint8_t)mng->w;

	/* 24: b_cb default is height */
	defi[24] = (uint8_t)(mng->h >> 24);
	defi[25] = (uint8_t)(mng->h >> 16);
	defi[26] = (uint8_t)(mng->h >>  8);
	defi[27] = (uint8_t)mng->h;

	/* already got size and 'DEFI' tag */
	if (0 != (rc = xng_read_bytes(mng, defi, part)))
		return rc;
	for (i = part; i < size; i++) {
		uint8_t ignore;
		if (0 != (rc = xng_read_byte(mng,&ignore)))
			return rc;
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;

	/* extract (some) values from the DEFI
	 * 00: ignore ID
	 * 02: ignore do_not_show
	 * 03: ignore concrete_flag
	 */

	/* 04: get x location */
	mng->xloc = (defi[ 4] << 24) | (defi[ 5] << 16) |
		(defi[ 6] << 16) | defi[ 7];
	/* 08: get y location */
	mng->yloc = (defi[ 8] << 24) | (defi[ 9] << 16) |
		(defi[10] << 16) | defi[11];
	/* 12: get l_cb */
	mng->l_cb = (defi[12] << 24) | (defi[13] << 16) |
		(defi[14] << 16) | defi[15];
	/* 16: get r_cb */
	mng->r_cb = (defi[16] << 24) | (defi[17] << 16) |
		(defi[18] << 16) | defi[19];
	/* 20: get t_cb */
	mng->t_cb = (defi[20] << 24) | (defi[21] << 16) |
		(defi[22] << 16) | defi[23];
	/* 24: get b_cb */
	mng->b_cb = (defi[24] << 24) | (defi[25] << 16) |
		(defi[26] << 16) | defi[27];

	return 0;
}

/**
 * @brief write a MNG MEND header
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_MEND(mng_t *mng)
{
	int rc;

	LOG((log_MISC,1,"mng_write_MEND(%p)\n",
		mng));
	if (0 != (rc = xng_write_size(mng,0)))	
		return rc;
	if (0 != (rc = xng_write_string(mng,"MEND")))	/* end of MNG */
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG MEND header
 *
 * @param mng pointer to a mng_t context
 * @param size expected number of bytes of header data
 * @result returns 0 on success, -1 on error
 */
int mng_read_MEND(mng_t *mng, uint32_t size)
{
	uint32_t i;
	int rc;

	LOG((log_MISC,1,"mng_read_MEND(%p,%#x)\n",
		mng, (unsigned)size));
	/* already got size and 'MEND' tag */
	for (i = 0; i < size; i++) {
		uint8_t ignore;
		if (0 != (rc = xng_read_byte(mng,&ignore)))
			return rc;
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}


/**
 * @brief write a MNG sRGB header (standard RGB)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_sRGB(mng_t *mng)
{
	size_t size = sizeof(mng->srgb);
	int rc;
	LOG((log_MISC,1,"mng_write_sRGB(%p)\n",
		mng));

	if (0 != (rc = xng_write_size(mng, size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng, "sRGB")))
		return rc;
	if (0 != (rc = xng_write_bytes(mng, mng->srgb, size)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG sRGB header (standard RGB)
 *
 * @param mng pointer to a mng_t context
 * @param size expected size of a sRGB header
 * @result returns 0 on success, -1 on error
 */
int mng_read_sRGB(mng_t *mng, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"mng_read_sRGB(%p,%#x)\n",
		mng, (unsigned)size));

	part = 1;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	/* already got size and 'sRGB' tag */
	if (0 != (rc = xng_read_byte(mng, mng->srgb)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG gAMA header (image gamma)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_gAMA(mng_t *mng)
{
	size_t size = sizeof(mng->gamma);
	int rc;
	LOG((log_MISC,1,"mng_write_gAMA(%p)\n",
		mng));

	if (0 != (rc = xng_write_size(mng, size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng, "gAMA")))
		return rc;
	if (0 != (rc = xng_write_uint(mng, mng->gamma)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG gAMA header (image gamma)
 *
 * @param mng pointer to a mng_t context
 * @param size expected size of a gAMA header
 * @result returns 0 on success, -1 on error
 */
int mng_read_gAMA(mng_t *mng, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"mng_read_gAMA(%p,%#x)\n",
		mng, (unsigned)size));

	part = 1;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	/* already got size and 'gAMA' tag */
	if (0 != (rc = xng_read_uint(mng, &mng->gamma)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG pHYg header (physical dimensions global)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_pHYg(mng_t *mng)
{
	size_t size = sizeof(mng->px) + sizeof(mng->py) + sizeof(mng->unit);
	int rc;
	LOG((log_MISC,1,"mng_write_pHYg(%p)\n",
		mng));

	if (0 != (rc = xng_write_size(mng, size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng, "pHYg")))
		return rc;
	if (0 != (rc = xng_write_uint(mng, mng->px)))
		return rc;
	if (0 != (rc = xng_write_uint(mng, mng->py)))
		return rc;
	if (0 != (rc = xng_write_byte(mng, mng->unit)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG pHYg header (physical dimensions global)
 *
 * @param mng pointer to a mng_t context
 * @param size expected size of a pHYg header
 * @result returns 0 on success, -1 on error
 */
int mng_read_pHYg(mng_t *mng, uint32_t size)
{
	uint32_t part;
	int rc;
	LOG((log_MISC,1,"mng_read_pHYg(%p,%#x)\n",
		mng, (unsigned)size));

	part = 9;
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	/* already got size and 'pHYg' tag */
	if (0 != (rc = xng_read_uint(mng, &mng->px)))
		return rc;
	if (0 != (rc = xng_read_uint(mng, &mng->py)))
		return rc;
	if (0 != (rc = xng_read_byte(mng, &mng->unit)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG tIME header (date and time)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_tIME(mng_t *mng)
{
	uint32_t size = sizeof(mng->time);
	int rc;
	LOG((log_MISC,1,"mng_write_tIME(%p)\n",
		mng));

	if (0 != (rc = xng_write_size(mng, size)))
		return rc;
	if (0 != (rc = xng_write_string(mng, "tIME")))
		return rc;
	if (0 != (rc = xng_write_bytes(mng, mng->time, size)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG tIME header (date and time)
 *
 * @param mng pointer to a mng_t context
 * @param size expected size of a tIME header
 * @result returns 0 on success, -1 on error
 */
int mng_read_tIME(mng_t *mng, uint32_t size)
{
	uint32_t part = sizeof(mng->time);
	int rc;
	LOG((log_MISC,1,"mng_read_tIME(%p,%#x)\n",
		mng, (unsigned)size));

	/* already got size and 'tIME' tag */
	if (size < part) {
		LOG((log_MISC,1,"size is less than %u\n", part));
		errno = EINVAL;
		return -1;
	}
	if (0 != (rc = xng_read_bytes(mng, mng->time, part)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG tEXt extension header (comment, author, ...)
 *
 * @param mng pointer to a mng_t context
 * @param keyword pointer to a keyword to write (including a \0 delimiter)
 * @param text pointer to a text string to write
 * @result returns 0 on success, -1 on error
 */
int mng_write_tEXt(mng_t *mng, const char *keyword, const char *text)
{
	uint32_t size = 0;
	int rc;
	LOG((log_MISC,1,"mng_write_tEXt(%p,'%s','%s')\n",
		mng, keyword, text));

	if (keyword)
		size += strlen(keyword) + 1;
	if (text)
		size += strlen(text);

	if (0 != (rc = xng_write_size(mng, size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng, "tEXt")))
		return rc;
	if (keyword) {
		if (0 != (rc = xng_write_string(mng, keyword)))
			return rc;
		if (0 != (rc = xng_write_byte(mng, 0)))
			return rc;
	}
	if (text) {
		if (0 != (rc = xng_write_string(mng, text)))
			return rc;
	}
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG tEXt extension header (comment, author, ...)
 *
 * @param mng pointer to a mng_t context
 * @param dst pointer to a string buffer to read to
 * @param size maximum number of bytes in the string buffer
 * @result returns 0 on success, -1 on error
 */
int mng_read_tEXt(mng_t *mng, char *dst, uint32_t size)
{
	int rc;
	LOG((log_MISC,1,"mng_read_tEXt(%p,%p,%#x)\n",
		mng, dst, (unsigned)size));

	/* already got size and 'tEXt' tag */
	if (0 != (rc = xng_read_string(mng, dst, size)))
		return rc;
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG PLTE header and palette
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_PLTE(mng_t *mng)
{
	uint32_t size = 3 * mng->palsize;
	int rc;
	LOG((log_MISC,1,"mng_write_PLTE(%p) (%u entries)\n",
		mng, mng->palsize));

	if (0 != (rc = xng_write_size(mng, size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng, "PLTE")))
		return rc;
	if (0 != (rc = xng_write_bytes(mng, mng->pal, size)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG PLTE header and palette
 *
 * @param mng pointer to a mng_t context
 * @param size maximum size of the palette (entries)
 * @result returns 0 on success, -1 on error
 */
int mng_read_PLTE(mng_t *mng, uint32_t size)
{
	uint32_t part = sizeof(mng->pal);
	uint32_t entries = 256;
	int rc;
	LOG((log_MISC,1,"mng_read_PLTE(%pd) (max %u entries)\n",
		mng, entries));

	if (entries > size / 3) {
		LOG((log_MISC,1,"read only %u of %u palette entries\n",
			size / 3, entries));
		entries = size / 3;
	}
	if (3 * entries < part) {
		part = 3 * entries;
	}

	/* already got size and 'PLTE' tag */
	if (0 != (rc = xng_read_bytes(mng, mng->pal, part)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write a MNG tRNS header and transparency map
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_tRNS(mng_t *mng)
{
	uint32_t size = mng->trns_size;
	int rc;

	LOG((log_MISC,1,"mng_write_tRNS(%p) (%u bytes)\n",
		mng, size));
	if (0 != (rc = xng_write_size(mng, size)))	
		return rc;
	if (0 != (rc = xng_write_string(mng, "tRNS")))
		return rc;
	if (0 != (rc = xng_write_bytes(mng, mng->trns, size)))
		return rc;
	if (0 != (rc = xng_write_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief read a MNG tRNS header and transparency map
 *
 * @param mng pointer to a mng_t context
 * @param size expected size of a transparency map
 * @result returns 0 on success, -1 on error
 */
int mng_read_tRNS(mng_t *mng, uint32_t size)
{
	uint32_t part = mng->trns_size;
	int rc;

	LOG((log_MISC,1,"mng_read_tRNS(%p) -> %u bytes\n",
		mng, part));
	if (part > size) {
		part = size;
	}
	if (0 != (rc = xng_read_bytes(mng, mng->trns, part)))
		return rc;
	if (size > part) {
		uint32_t i;
		LOG((log_MISC,5,"skipping %u bytes\n", size - part));
		for (i = part; i < size; i++) {
			uint8_t ignore;
			if (0 != (rc = xng_read_byte(mng,&ignore)))
				return rc;
		}
	}
	if (0 != (rc = xng_read_crc(mng)))	
		return rc;
	return 0;
}

/**
 * @brief write the MHDR, if this has not yet been done
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_write_mhdr(mng_t *mng)
{
	int rc = 0;

	if (!mng->write_mhdr)
		return rc;

	if (0 != (rc = mng_write_MHDR(mng)))
		goto bailout;
	if (NULL != mng->author) {
		if (0 != (rc = mng_write_tEXt(mng, "Author", mng->author)))
			goto bailout;
	}
	if (NULL != mng->comment) {
		if (0 != (rc = mng_write_tEXt(mng, "Comment", mng->comment)))
			goto bailout;
	}
	/* write sRGB chunk */
	if (mng->srgb_size > 0) {
		if (0 != (rc = mng_write_sRGB(mng))) {
			goto bailout;
		}
		/* and also write the gAMA chunk */
		if (0 != (rc = mng_write_gAMA(mng))) {
			goto bailout;
		}
	}
	if (mng->palsize > 0) {
		if (0 != (rc = mng_write_PLTE(mng)))
			goto bailout;
	}
	if (0 != mng->px && 0 != mng->py) {
		if (0 != (rc = mng_write_pHYg(mng)))
			goto bailout;
	}
	if (mng->term_action != 0 || mng->term_after != 0 ||
		mng->term_delay != 0 || mng->term_maxiter != 0) {
		if (0 != (rc = mng_write_TERM(mng)))
				goto bailout;
	}

	/* TODO: is this a MUST field? */
	if (mng->back_size > 0) {
		if (0 != (rc = mng_write_BACK(mng)))
				goto bailout;
	}

	/* reset MHDR write flag */
	mng->write_mhdr = 0;

	/* count background as 1 layer (?) */
	mng->lcount++;

bailout:
	return rc;
}

/**
 * @brief finish a PNG in an MNG stream and write to output
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_finish_png(mng_t *mng)
{
	int rc = 0;

	/* finish previous png, if any */
	if (NULL == mng->png)
		return rc;

	/* write a FRAM chunk */
	if (0 != (rc = mng_write_FRAM(mng))) {
		goto bailout;
	}
	/* need to write a DEFI chunk? */
	if (0 != (rc = mng_write_DEFI(mng))) {
		goto bailout;
	}
	/* now write the PNG */
	if (0 != (rc = png_finish_mng(mng->png))) {
		mng->png = NULL;
		goto bailout;
	}
	mng->png = NULL;

	/*
	 * count as one frame, and one layer
	 * TODO: watch out for the distinction between layers and frames.
	 *
	 * A layer is one of:
	 *	+ A visible embedded image, located with respect to the frame
	 *	  boundaries and clipped with respect to the layer clipping
	 *	  boundaries and the image's own clipping boundaries.
	 *	+ The background that is displayed before the first image
	 *	  in the entire datastream is displayed. Its contents can be
	 *	  defined by the application or by the BACK chunk.
	 *	+ A solid rectangle filled with the background color and
	 *	  clipped to the subframe boundaries, that is used as a
	 *	  background when the framing mode is 3 or 4.
	 *
	 * A frame is:
	 *	A composition of zero or more layers that have zero interframe
	 *	delay time followed by a layer with a specified nonzero delay
	 *	time or by the MEND chunk. A frame is to be displayed as a
	 *	still picture or as part of a sequence of still images or an
	 *	animation. An animation would ideally appear to a perfect
	 *	observer (with an inhumanly fast visual system) as a sequence
	 *	of still pictures.
	 */
	mng->fcount++;
	mng->lcount++;

bailout:
	return rc;
}

/**
 * @brief get the current frame count
 *
 * @param png pointer to a mng_t context
 * @result returns current frame count, -1 on error
 */
int mng_get_fcount(mng_t *mng)
{
	if (NULL == mng) {
		errno = EINVAL;
		return -1;
	}
	return mng->fcount;
}

/**
 * @brief get the current layer count
 *
 * @param png pointer to a mng_t context
 * @result returns current layer count, -1 on error
 */
int mng_get_lcount(mng_t *mng)
{
	if (NULL == mng) {
		errno = EINVAL;
		return -1;
	}
	return mng->lcount;
}

/**
 * @brief get the current play time in seconds
 *
 * @param png pointer to a mng_t context
 * @result returns current play time in seconds
 */
double mng_get_ptime(mng_t *mng)
{
	if (NULL == mng) {
		errno = EINVAL;
		return -1;
	}
	if (mng->ticks > 0)
		return (double)mng->ptime / mng->ticks;
	return -1;
}

/**
 * @brief finish a MNG stream and write to output
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
int mng_finish(mng_t *mng)
{
	int rc = -1;

	if (NULL == mng || NULL == mng->x.output) {
		errno = EINVAL;
		goto bailout;
	}

	LOG((log_MISC,1,"mng_finish(%p)\n",
		mng));

	if (NULL == mng->x.output)
		goto bailout;

	/* write MHDR, if that has not yet been done */
	if (0 != (rc = mng_write_mhdr(mng))) {
		goto bailout;
	}

	/* finish previous png, if any */
	if (0 != (rc = mng_finish_png(mng))) {
		goto bailout;
	}

	if (0 != (rc = mng_write_MEND(mng))) {
		goto bailout;
	}

bailout:
	if (NULL != mng) {
		if (NULL != mng->png) {
			png_discard(mng->png);
			mng->png = NULL;
		}
		if (NULL != mng->img) {
			free(mng->img);
			mng->img = NULL;
			mng->size = 0;
		}
		free(mng);
		mng = NULL;
	}
	return rc;
}

/**
 * @brief read a MNG byte from a FILE pointer
 *
 * @param cookie pointer to an integer (fd)
 * @result returns byte read, or -1 on EOF
 */
static int mng_read_bytes(void *cookie, uint8_t *buff, int size)
{
	FILE *fp = (FILE *)cookie;
	if (size != fread(buff, 1, size, fp))
		return -1;
	return 0;
}

/**
 * @brief read a MNG file and setup a handler to write it on mng_finish()
 *
 * @param filename filename of the MNG file to read
 * @param cookie argument passed to the function to write bytes at mng_finish() time
 * @param output function to write an array of bytes at mng_finish() time
 * @result returns a pointer to an allocated mng_t context, or NULL on error
 */
mng_t *mng_read(const char *filename,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size))
{
	mng_t *mng = NULL;
	uint8_t magic[8];
	char tag[4+1];
	uint32_t size;
	uint32_t i;
	FILE *fp;
	int mend, rc;

	LOG((log_MISC,1,"mng_read('%s')\n",
		filename));

	fp = fopen(filename, "rb");
	if (NULL == fp) {
		LOG((log_MISC,1,"fopen('%s','rb') call failed (%s)\n",
			filename, strerror(errno)));
		return NULL;
	}
	mng = (mng_t *)calloc(1, sizeof(mng_t));
	if (NULL == mng) {
		LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
			1, sizeof(mng_t), strerror(errno)));
		return NULL;
	}

	mng->x.cookie = fp;
	mng->x.input = mng_read_bytes;

	/* read MNG magic */
	if (0 != (*mng->x.input)(mng->x.cookie, magic, sizeof(magic))) {
		LOG((log_MISC,1,"MNG magic read failed\n", i));
		free(mng);
		fclose(fp);
		return NULL;
	}
	if (memcmp(magic, mngmagic, sizeof(magic))) {
		LOG((log_MISC,1,"MNG magic mismatch\n"));
		free(mng);
		fclose(fp);
		return NULL;
	}

	mend = 0;
	while (0 == mend) {
		if (0 != (rc = xng_read_size(mng, &size))) {
			LOG((log_MISC,1,"MNG read size failed (%s)\n",
				strerror(errno)));
			goto bailout;
		}
		if (0 != (rc = xng_read_string(mng, tag, 4))) {
			LOG((log_MISC,1,"MNG read tag failed (%s)\n",
				strerror(errno)));
			goto bailout;
		}
		LOG((log_MISC,5,"MNG '%s' size %#x\n", tag, size));
		if (0 == strcmp(tag, "MHDR")) {
			if (0 != (rc = mng_read_MHDR(mng, size))) {
				LOG((log_MISC,1,"MNG read MHDR failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
			/* 4 bytes per pixel */
			mng->stride = 4 * mng->w;
			mng->size = mng->h * mng->stride;
			LOG((log_MISC,5,"MNG image size is %#x, %ux%ux%u, %u bytes/row\n",
				mng->size, mng->w, mng->h, 32, mng->stride));
			mng->img = calloc(mng->size, sizeof(uint8_t));
			if (NULL == mng->img) {
				LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
					mng->size, sizeof(uint8_t), strerror(errno)));
				rc = -1;
				goto bailout;
			}
		} else if (0 == strcmp(tag, "TERM")) {
			if (0 != (rc = mng_read_TERM(mng, size))) {
				LOG((log_MISC,1,"MNG read TERM failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "BACK")) {
			if (0 != (rc = mng_read_BACK(mng, size))) {
				LOG((log_MISC,1,"MNG read BACK failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "pHYg")) {
			if (0 != (rc = mng_read_pHYg(mng, size))) {
				LOG((log_MISC,1,"MNG read pHYg failed (%s)\n",
					strerror(errno)));
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
			if (0 != (rc = mng_read_tEXt(mng, tmp, size))) {
				LOG((log_MISC,1,"MNG read 'tEXt' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
			LOG((log_MISC,1,"tEXt: '%s'\n", tmp));
			free(tmp);
		} else if (0 == strcmp(tag, "PLTE")) {
			if (0 != (rc = mng_read_PLTE(mng, size))) {
				LOG((log_MISC,1,"MNG read 'PLTE' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "pHYg")) {
			if (0 != (rc = mng_read_pHYg(mng, size))) {
				LOG((log_MISC,1,"MNG read 'pHYg' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "tIME")) {
			if (0 != (rc = mng_read_tIME(mng, size))) {
				LOG((log_MISC,1,"MNG read 'tIME' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "tRNS")) {
			if (0 != (rc = mng_read_tRNS(mng, size))) {
				LOG((log_MISC,1,"MNG read 'tRNS' failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
		} else if (0 == strcmp(tag, "MEND")) {
			if (0 != (rc = mng_read_MEND(mng, size))) {
				LOG((log_MISC,1,"MNG read MEND failed (%s)\n",
					strerror(errno)));
				goto bailout;
			}
			mend = 1;
		} else {
			LOG((log_MISC,1,"MNG ignore tag '%s'\n", tag));
			for (i = 0; i < size; i++) {
				uint8_t ignore;
				xng_read_byte(mng, &ignore);
			}
			if (0 != (rc = xng_read_crc(mng))) {
				LOG((log_MISC,1,"MNG crc of unknown tag '%s' failed\n",
					tag));
				goto bailout;
			}
		}
	}
	fclose(fp);
	fp = NULL;

	/* set the output cookie data and function */
	mng->x.cookie = cookie;
	mng->x.output = output;

	/* need to write MHDR once */
	mng->write_mhdr = 1;

	rc = 0;

bailout:
	if (NULL != fp) {
		fclose(fp);
		fp = NULL;
	}
	if (0 != rc) {
		if (NULL != mng && NULL != mng->img) {
			free(mng->img);
			mng->img = NULL;
		}
		if (NULL != mng) {
			free(mng);
			mng = NULL;
		}
	}
	return mng;
}

/**
 * @brief create a fresh MNG context and setup a handler to write it on mng_finish()
 *
 * @param w width of the image in pixels
 * @param h height of the image in pixels
 * @param ticks ticks per second
 * @param cookie argument passed to the function to write bytes at mng_finish() time
 * @param output function to write an array of bytes at mng_finish() time
 * @result returns 0 on success, -1 on error
 */
mng_t *mng_create(int w, int h, int ticks,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size))
{
	mng_t *mng;
	int rc = -1;

	LOG((log_MISC,1,"mng_create(%d,%d,%d,%p,%p)\n",
		w, h, ticks, cookie, output));

	mng = (mng_t *)calloc(1, sizeof(mng_t));
	if (NULL == mng) {
		LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
			1, sizeof(mng_t), strerror(errno)));
		goto bailout;
	}

	/* set width */
	mng->w = w;

	/* set height */
	mng->h = h;

	/* set ticks per second */
	mng->ticks = ticks;

	/* unknown number of layers */
	mng->lcount = 0;

	/* unknown number of frames */
	mng->fcount = 0;

	/* unknown play time */
	mng->ptime = 0;

	/**
	 * <PRE>
	 *   Simplicity_profile:  4 bytes:(unsigned integer).
	 *
	 *   31 30-16 15-10 9 8 7 6 5 4 3 2 1 0   function
	 *   ------------------------------------------------------------------------------------
	 *    |     |     | | | | | | | | | | |
	 *    |     |     | | | | | | | | | | +-- Profile Validity
	 *    |     |     | | | | | | | | | |     1: Absence of certain features is specified by
	 *    |     |     | | | | | | | | | |        the remaining bits of the simplicity profile.
	 *    |     |     | | | | | | | | | |        (must be 1 in MNG-LC datastreams)
	 *    |     |     | | | | | | | | | |
	 *    |     |     | | | | | | | | | +---- Simple MNG features
	 *    |     |     | | | | | | | | |       0: Simple MNG features are absent.
	 *    |     |     | | | | | | | | |       1: Simple MNG features may be present.
	 *    |     |     | | | | | | | | |
	 *    |     |     | | | | | | | | +------ Complex MNG features
	 *    |     |     | | | | | | | |         0: Complex MNG features are absent.
	 *    |     |     | | | | | | | |         (must be 0 in MNG-LC datastreams)
	 *    |     |     | | | | | | | |
	 *    |     |     | | | | | | | +-------- Internal transparency
	 *    |     |     | | | | | | |           0: Transparency is absent or can be ignored.
	 *    |     |     | | | | | | |              All images in the datastream are opaque or
	 *    |     |     | | | | | | |              can be rendered as opaque without affecting
	 *    |     |     | | | | | | |              the final appearance of any frame.
	 *    |     |     | | | | | | |           1: Transparency may be present.
	 *    |     |     | | | | | | |
	 *    |     |     | | | | | | +---------- JNG
	 *    |     |     | | | | | |             0: JNG and JDAA are absent.
	 *    |     |     | | | | | |             1: JNG or JDAA may be present.
	 *    |     |     | | | | | |             (must be 0 in MNG-LC datastreams)
	 *    |     |     | | | | | |
	 *    |     |     | | | | | +------------ Delta-PNG
	 *    |     |     | | | | |               0: Delta-PNG is absent.
	 *    |     |     | | | | |               (must be 0 in MNG-LC datastreams)
	 *    |     |     | | | | |
	 *    |     |     | | | | +-------------- Validity flag for bits 7, 8, and 9
	 *    |     |     | | | |                 0: The absence of background transparency,
	 *    |     |     | | | |                    semitransparency, and stored object buffers
	 *    |     |     | | | |                    is unspecified; bits 7, 8, and 9 have no
	 *    |     |     | | | |                    meaning and must be 0.
	 *    |     |     | | | |                 1: The absence or possible presence of
	 *    |     |     | | | |                    background transparency is expressed by
	 *    |     |     | | | |                    bit 7, of semitransparency by bit 8, and of
	 *    |     |     | | | |                    stored object buffers by bit 9.
	 *    |     |     | | | |
	 *    |     |     | | | +---------------- Background transparency
	 *    |     |     | | |                   0: Background transparency is absent (i.e., the
	 *    |     |     | | |                      first layer fills the entire MNG frame with
	 *    |     |     | | |                      opaque pixels).
	 *    |     |     | | |                   1: Background transparency may be present.
	 *    |     |     | | |
	 *    |     |     | | +------------------ Semi-transparency
	 *    |     |     | |                     0: Semitransparency (i.e., an image with an
	 *    |     |     | |                        alpha channel that has values that are
	 *    |     |     | |                        neither 0 nor the maximum value) is absent.
	 *    |     |     | |                     1: Semitransparency may be present.
	 *    |     |     | |                     If bit 3 is zero this field has no meaning.
	 *    |     |     | |
	 *    |     |     | +-------------------- Stored object buffers
	 *    |     |     |                       0: Object buffers need not be stored.
	 *    |     |     |                          (must be 0 in MNG-LC and MNG-VLC datastreams)
	 *    |     |     |                       If bit 2 is zero, this field has no meaning.
	 *    |     |     |
	 *    |     |     +---------------------- Reserved bits
	 *    |     |                             Reserved for public expansion.
	 *    |     |                             Must be zero in this version.
	 *    |     |
	 *    |     +---------------------------- Private bits
	 *    |                                   Available for private or experimental
	 *    |                                   expansion. Undefined in this version
	 *    |                                   and can be ignored.
	 *    |
	 *    +---------------------------------- Reserved bit. Must be zero.
	 * </PRE>
	 */
	mng->profile = (1 << 0) | (1 << 1) | (1 << 6);


	/* default sRGB chunk (?)
	 * set standard RGB to "saturation":
	 * Saturation intent is for images preferring preservation
	 * of saturation at the expense of hue and lightness, like
	 * charts and graphs.
	 */
	mng->srgb[0] = 2;
	mng->srgb_size = sizeof(mng->srgb);
	/*
	 * set default gamma
	 * An application that writes the sRGB chunk should also write
	 * a gAMA chunk (and perhaps a cHRM chunk) for compatibility
	 * with applications that do not use the sRGB chunk.
	 * In this situation, only the following values may be used: 
	 *    gAMA:
	 *    Gamma: 45455
	 */
	mng->gamma = 45455;

	/* reset background to all 0s ? */
	memset(mng->back, 0, sizeof(mng->back));

	/* always write a BACK chunk (?) */
	mng->back_size = 6;

	/* default interframe delay: 1 tick */
	mng->ifdelay = mng->ifdelay_default = 1;

	/* default timeout: 0x7fffffff = infinite */
	mng->timeout = mng->timeout_default = 0x7fffffff;

	/* default clipping: left=0, right=width, top=0, bottom=height */
	mng->ll_cb = 0;
	mng->lr_cb = mng->w;
	mng->lt_cb = 0;
	mng->lb_cb = mng->h;

	/* when done, show last frame indefinitely */
	mng->term_action = 0;

	/* no iteration flags */
	mng->term_after = 0;

	/* no iteration, no delay */
	mng->term_delay = 0;

	/* no iterations */
	mng->term_maxiter = 0;

	/* 4 bytes per pixel */
	mng->stride = 4 * mng->w;
	mng->size = mng->h * mng->stride;


#if	0
	/* FIXME: we don't really need (yet) a mng frame buffer
	 * maybe implement delta frame search, but that may be
	 * easier to do on the previous PNG in the MNG
	 */
	mng->img = calloc(mng->size, sizeof(uint8_t));

	if (NULL == mng->img) {
		LOG((log_MISC,1,"calloc(%d,%d) call failed (%s)\n",
			mng->size, sizeof(uint8_t), strerror(errno)));
		rc = -1;
		goto bailout;
	}
	LOG((log_MISC,1,"image is %d bytes (width:%d height:%d stride:%d)\n",
		mng->size, mng->w, mng->h, mng->stride));
#endif

	mng->x.cookie = cookie;
	mng->x.output = output;

	/* write MNG magic */
	rc = (*mng->x.output)(mng->x.cookie, mngmagic, 8);
	if (0 != rc)
		goto bailout;

	/* set flag to write MHDR once */
	mng->write_mhdr = 1;

	/* success */
	rc = 0;

bailout:
	if (0 != rc) {
		if (NULL != mng && NULL != mng->img) {
			free(mng->img);
			mng->img = NULL;
		}
		if (NULL != mng) {
			free(mng);
			mng = NULL;
		}
	}
	return mng;
}

/**
 * @brief set a palette entry
 *
 * @param mng pointer to a mng_t context
 * @param idx palette index (0 to 255)
 * @param color PNG_RGB() color value for the palette entry
 * @result returns 0 on success, -1 on error
 */
int mng_set_palette(mng_t *mng, int idx, int color)
{
	uint32_t entry;

	if (NULL == mng || NULL == mng->x.output) {
		errno = EINVAL;
		return -1;
	}

	if (idx < 0 || idx >= 256)
		return -EINVAL;

	entry = 3 * idx;
	mng->pal[entry+0] = color >> 16;	/* red */
	mng->pal[entry+1] = color >> 8;		/* green */
	mng->pal[entry+2] = color;		/* blue */

	/* HACK: assume that index 0 is the background ? */
	if (0 == idx) {
		/* set RGB16 background from this color */
		mng->back[0] = (color >> 16);
		mng->back[1] = 0;
		mng->back[2] = (color >> 8);
		mng->back[3] = 0;
		mng->back[4] = color;
		mng->back[5] = 0;
		mng->back_size = 6;
	}

	/* increase palette size to max. color stored */
	if (idx >= mng->palsize)
		mng->palsize = idx + 1;

	return 0;
}

/**
 * @brief append a new PNG image context to the MNG stream
 *
 * @param mng pointer to a struct mng_t
 * @param x x location of the PNG in the frame
 * @param y y location of the PNG in the frame
 * @param w width of the image in pixels
 * @param h height of the image in pixels
 * @param color PNG color mode to use for the image
 * @param depth number of bits per color component
 * @result returns pointer to new png_t context on success, NULL on error
 */
png_t *mng_append_png(mng_t *mng, int x, int y, int w, int h, int color, int depth)
{
	int rc;

	if (NULL == mng || NULL == mng->x.output) {
		errno = EINVAL;
		return NULL;
	}

	if (0 != (rc = mng_write_mhdr(mng))) {
		return NULL;
	}

	if (0 != (rc = mng_finish_png(mng))) {
		return NULL;
	}

	/* set xloc, yloc and clipping boundaries */
	mng->xloc = x;
	mng->yloc = y;

	mng->l_cb = x;
	mng->r_cb = x + w;
	mng->t_cb = y;
	mng->b_cb = y + h;

	/* create the new png */
	mng->png = png_create(w, h, color, depth,
		mng->x.cookie, mng->x.output);

	return mng->png;
}
