/*****************************************************************************
 * Multiple Image Network Grapics - LC (low complexity)
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
 * $Id: mng.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_MNG_H_INCLUDED_)
#define	_MNG_H_INCLUDED_

#include "png.h"

/** @brief structure of a PNG context */
typedef	struct mng_s {
	/** @brief common part of structure shared with MNG */
	xng_t x;

	/** @brief bytes per scanline */
	uint32_t stride;

	/** @brief image width in pixels */
	uint32_t w;

	/** @brief image height in pixels */
	uint32_t h;

	/** @brief ticks per second
	 *
	 * The ticks_per_second field gives the unit used by the FRAM chunk
	 * to specify interframe delay and timeout. It must be nonzero
	 * if the datastream contains a sequence of images.
	 * When the datastream contains exactly one frame, this field should
	 * be set to zero. When this field is zero, the length of a tick is
	 * infinite, and decoders will ignore any attempt to define interframe
	 * delay, timeout, or any other variable that depends on the length of
	 * a tick. If the frames are intended to be displayed one at a time
	 * under user control, such as a slide show or a multi-page FAX, the
	 * tick length can be set to any positive number and a FRAM chunk can
	 * be used to set an infinite interframe delay and a zero timeout.
	 * Unless the user intervenes, viewers will only display the first
	 * frame in the datastream.
	 */
	uint32_t ticks;

	/** @brief nominal layer count
	 *
	 * If the nominal_layer_count field contains a zero, the layer count
	 * is unspecified. If it is nonzero, it contains the number of layers
	 * (including all background layers) in the datastream, ignoring any
	 * effects of the TERM chunk. If the layer count is greater than 2^31-1,
	 * encoders should write 231-1, representing an infinite layer count.
	 */
	uint32_t lcount;

	/** @brief nominal frame count
	 *
	 * If the frame count field contains a zero, the frame count is
	 * unspecified. If it is nonzero, it contains the number of frames that
	 * would be displayed, ignoring the TERM chunk. If the frame count is
	 * greater than 2^31-1, encoders should write 2^31-1, representing an
	 * infinite frame count.
	 */
	uint32_t fcount;

	/** @brief nominal play time
	 *
	 * If the nominal_play_time field contains a zero, the nominal play time
	 * is unspecified. Otherwise, it gives the play time, in ticks, when the
	 * file is displayed ignoring the TERM chunk. Authors who write this
	 * field should choose a value of ticks_per_second that will allow the
	 * nominal play time to be expressed in a four-bit integer.
	 * If the nominal play time is greater than 2^31-1 ticks, encoders should
	 * write 2^31-1, representing an infinite nominal play time.
	 */
	uint32_t ptime;

	/** @brief MNG profile flags
	 *
	 * When bit 0 of the simplicity_profile field is zero, the simplicity
	 * (or complexity) of the MNG datastream is unspecified, and all bits
	 * of the simplicity profile must be zero.
	 * The simplicity profile must be nonzero in MNG-LC datastreams. 
	 * If the simplicity profile is nonzero, it can be regarded as a 32-bit
	 * profile, with bit 0 (the least significant bit) being a
	 * "profile-validity" flag, bit 1 being a "simple MNG" flag, bit 2 being
	 * a "complex MNG" flag, bits 3, 7, and 8 being "transparency" flags,
	 * bit 4 being a "JNG" flag, bit 5 being a "Delta-PNG" flag, and bit 9
	 * being a "stored object buffers" flag. Bit 6 is a "validity" flag for
	 * bits 7, 8, and 9, which were added at version 0.98 of this
	 * specification. These three flags mean nothing if bit 6 is zero.
	 *
	 * If a bit is zero, the corresponding feature is guaranteed to be
	 * absent or if it is present there is no effect on the appearance of
	 * any frame if the feature is ignored.
	 * If a bit is one, the corresponding feature may be present in the MNG
	 * datastream. 
	 *
	 * Bits 10 through 15 of the simplicity profile are reserved for future
	 * MNG versions, and must be zero in this version.
	 *
	 * Bits 16 through 30 are available for private test or experimental
	 * versions. The most significant bit (bit 31) must be zero.
	 *
	 * When bit 1 is zero ("simple" MNG features are absent), the datastream
	 * does not contain the DEFI, FRAM, MAGN, or global PLTE and tRNS chunks,
	 * and filter method 64 is not used in any embedded PNG datastream. 
	 *
	 * "Transparency is absent or can be ignored" means that either the MNG
	 * or PNG tRNS chunk is not present and no PNG or JNG image has an alpha
	 * channel, or if they are present they have no effect on the final
	 * appearance of any frame and can be ignored (e.g., if the only
	 * transparency in a MNG datastream appears in a thumbnail that is never
	 * displayed in a frame, or is in some pixels that are overlaid by opaque
	 * pixels before being displayed, the transparency bit should be set to zero). 
	 *
	 * "Semitransparency is absent" means that if the MNG or PNG tRNS chunk
	 * is present or if any PNG or JNG image has an alpha channel, they only
	 * contain the values 0 and the maximum (opaque) value. It also means
	 * that the JDAA chunk is not present. The "semitransparency" flag means
	 * nothing and must be 0 if bit 3 is 0 or bit 6 is 0. 
	 *
	 * "Background transparency is absent" means that the first layer of every
	 * segment fills the entire frame with opaque pixels, and that nothing
	 * following the first layer causes any frame to become transparent.
	 * Whatever is behind the first layer does not show through. 
	 *
	 * When "Background transparency" is present, the application is
	 * responsible for supplying a background color or image against which the
	 * MNG background layer is composited, and if the MNG is being displayed
	 * against a changing scene, the application should refresh the entire MNG
	 * frame against a new copy of the background layer whenever the
	 * application's background scene changes. The "background transparency"
	 * flag means nothing and must be 0 if bit 6 is 0.
	 * Note that bit 3 does not make any promises about background transparency. 
	 *
	 * The "stored object buffers" flag must be zero in MNG-VLC and MNG-LC
	 * datastreams.
	 */
	uint32_t profile;

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

	/** @brief size of the BACK chunk (background color) */
	size_t back_size;

	/** @brief background color RGB (16bits each), mandatory flag, alpha (16bits), ??? */
	uint8_t back[10];

	/** @brief size of the transparency map */
	size_t trns_size;

	/** @brief transparency map */
	uint8_t trns[256];

	/** @brief palette entries */
	uint8_t pal[3*256];

	/** @brief number of used palette entries */
	uint32_t palsize;

	/** @brief termination action
	 * 0: show last frame indefinitely
	 * 1: cease displaying anything
	 * 2: show the first frame after the TERM chunk
	 * 3: repeat the sequence starting immediately after
	 *    the TERM chunk and ending with the MEND chunk
	 */
	uint8_t term_action;

	/** @brief action after iterations
	 * 0: show last frame indefinitely after
	 *    term_maxiter iterations have been done
	 * 1: cease displaying anything
	 * 2: show the first frame after the TERM chunk
	 */
	uint8_t term_after;

	/** @brief delay, in ticks, before repeating the sequence */
	uint32_t term_delay;

	/** @brief maximum number of times to execute the sequence */
	uint32_t term_maxiter;

	/** @brief image bitmap size in bytes */
	size_t size;

	/** @brief image bitmap data */
	uint8_t *img;

	/** @brief comment string */
	char *comment;

	/** @brief author string */
	char *author;

	/** @brief interframe delay default (1 at start) */
	int ifdelay_default;

	/** @brief timeout default (0x7fffffff at start) */
	int timeout_default;

	/** @brief left clipping boundary of the layer (default value) */
	int ll_cb_default;

	/** @brief right clipping boundary of the layer (default value) */
	int lr_cb_default;

	/** @brief top clipping boundary of the layer (default value) */
	int lt_cb_default;

	/** @brief bottom clipping boundary of the layer (default value) */
	int lb_cb_default;

	/** @brief timeout (next frame) */
	int timeout;

	/** @brief interframe delay (next frame) */
	int ifdelay;

	/** @brief left clipping boundary of the layer (next frame) */
	int ll_cb;

	/** @brief right clipping boundary of the layer (next frame) */
	int lr_cb;

	/** @brief top clipping boundary of the layer (next frame) */
	int lt_cb;

	/** @brief bottom clipping boundary of the layer (next frame) */
	int lb_cb;

	/** @brief x location (defined object) */
	int xloc;

	/** @brief y location (defined object) */
	int yloc;

	/** @brief left clipping boundary (defined object) */
	int l_cb;

	/** @brief right clipping boundary (defined object) */
	int r_cb;

	/** @brief top clipping boundary (defined object) */
	int t_cb;

	/** @brief bottom clipping boundary (defined object) */
	int b_cb;

	/** @brief number of elements in the syncid array */
	size_t syncid_size;

	/** @brief array of sync IDs for the next frame */
	uint32_t *syncid;

	/** @brief current PNG to write to the MNG stream */
	png_t *png;

	/** @brief set non-zero if MHDR is yet to be written out */
	int write_mhdr;

}	mng_t;

/**
 * @brief read a MNG file and setup a handler to write it somewhere
 *
 * @param filename filename of the PNG file to read
 * @param cookie argument passed to the function to write bytes at mng_finish() time
 * @param output function to write an array of bytes at mng_finish() time
 * @result returns byte read, or -1 on EOF
 */
extern mng_t *mng_read(const char *filename,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size));

/**
 * @brief create a fresh PNG context and setup a handler to write it on mng_finish()
 *
 * @param w width of the image in pixels
 * @param h height of the image in pixels
 * @param ticks ticks per second
 * @param cookie argument passed to the function to write bytes at mng_finish() time
 * @param output function to write an array of bytes at mng_finish() time
 * @result returns 0 on success, -1 on error
 */
extern mng_t *mng_create(int w, int h, int ticks,
	void *cookie, int (*output)(void *cookie, uint8_t *data, int size));

/**
 * @brief finish a MNG stream (read or created) and write to output
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
extern int mng_finish(mng_t *mng);

/**
 * @brief get the current frame count
 *
 * @param mng pointer to a mng_t context
 * @result returns current frame count, -1 on error
 */
extern int mng_get_fcount(mng_t *mng);

/**
 * @brief get the current layer count
 *
 * @param mng pointer to a mng_t context
 * @result returns current layer count, -1 on error
 */
extern int mng_get_lcount(mng_t *mng);

/**
 * @brief get the current play time in seconds
 *
 * @param mng pointer to a mng_t context
 * @result returns current play time in seconds, -1 on error
 */
extern double mng_get_ptime(mng_t *mng);

/**
 * @brief set a palette entry
 *
 * @param mng pointer to a mng_t context
 * @param idx palette index (0 to 255)
 * @param color PNG_RGB() color value for the palette entry
 * @result returns 0 on success, -1 on error
 */
extern int mng_set_palette(mng_t *mng, int idx, int color);

/**
 * @brief append a new PNG image context to the MNG stream
 *
 * @param w width of the image in pixels
 * @param h height of the image in pixels
 * @param color PNG color mode to use for the image
 * @param depth number of bits per color component
 * @result returns pointer to new png_t context on success, NULL on error
 */
extern png_t *mng_append_png(mng_t *mng, int x, int y, int w, int h, int color, int depth);

/* ======================================================================== *
 *
 * The following functions are public, even though the average application
 * doesn't need them. They are usually called only by the mng.c code.
 *
 * ======================================================================== */

/**
 * @brief write a MNG MHDR (stream header)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
extern int mng_write_MHDR(mng_t *mng);

/**
 * @brief read a MNG MHDR (stream header)
 *
 * @param mng pointer to a mng_t context
 * @param size size of the MHDR chunk found
 * @result returns 0 on success, -1 on error
 */
extern int mng_read_MHDR(mng_t *mng, uint32_t size);

/**
 * @brief write a MNG TERM header
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
extern int mng_write_TERM(mng_t *mng);

/**
 * @brief read a MNG TERM header
 *
 * @param mng pointer to a mng_t context
 * @param size size of the TERM chunk found
 * @result returns 0 on success, -1 on error
 */
extern int mng_read_TERM(mng_t *mng, uint32_t size);

/**
 * @brief write a MNG BACK header and background color
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
extern int mng_write_BACK(mng_t *mng);

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
 * @param size size of the BACK chunk found
 * @result returns 0 on success, -1 on error
 */
extern int mng_read_BACK(mng_t *mng, uint32_t size);

/**
 * @brief write a MNG FRAM header
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
extern int mng_write_FRAM(mng_t *mng);

/**
 * @brief write a MNG DEFI header (object definition)
 *
 * @param mng pointer to a mng_t context
 * @result returns 0 on success, -1 on error
 */
extern int mng_write_DEFI(mng_t *mng);

/**
 * @brief read a MNG DEFI header and background color
 *
 * @param mng pointer to a mng_t context
 * @param size size of the DEFI chunk found
 * @result returns 0 on success, -1 on error
 */
extern int mng_read_DEFI(mng_t *mng, uint32_t size);

#endif	/* !defined(_MNG_H_INCLUDED_) */
