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
 * Salto main entry and SDL interface
 *
 * $Id: salto.c,v 1.2 2008/08/19 14:07:22 pm Exp $
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <SDL.h>

#include "alto.h"
#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include "display.h"
#include "mouse.h"
#include "disk.h"
#include "drive.h"
#include "ether.h"
#include "debug.h"
#include "hardware.h"
#include "keyboard.h"
#include "printer.h"
#include "eia.h"
#include "png.h"
#include "mng.h"

#ifndef	GRABKEYS
/** @brief keys to grab/release mouse input */
#define	GRABKEYS	(KMOD_LCTRL|KMOD_LALT)
#endif

/** @brief non-zero to draw LED icons at the frontend's bottom border */
#ifndef	FRONTEND_ICONS
#define	FRONTEND_ICONS	0
#endif

#if	FRONTEND_ICONS
/* XXX: hackish, should have a header */
extern uint8_t ppm_led_blank[];
extern uint8_t ppm_led_0[];
extern uint8_t ppm_led_1[];
extern uint8_t ppm_led_2[];
extern uint8_t ppm_led_3[];
extern uint8_t ppm_led_4[];
extern uint8_t ppm_led_5[];
extern uint8_t ppm_led_6[];
extern uint8_t ppm_led_7[];
extern uint8_t ppm_led_8[];
extern uint8_t ppm_led_9[];
extern uint8_t ppm_green_off[];
extern uint8_t ppm_green_on[];
extern uint8_t ppm_red_off[];
extern uint8_t ppm_red_on[];
extern uint8_t ppm_yellow_off[];
extern uint8_t ppm_yellow_on[];
extern uint8_t ppm_mouse_off[];
extern uint8_t ppm_mouse_on[];
extern uint8_t ppm_mng_off[];
extern uint8_t ppm_mng_on0[];
extern uint8_t ppm_mng_on1[];
extern uint8_t ppm_mng_on2[];
extern uint8_t ppm_mng_on3[];
#endif

/** @brief SDL window title bar text */
char title[256] = "SALTO - Simulating the Xerox Alto II";

/** @brief non-zero if SDL cursor is also shown inside the alto surface */
int sdl_cursor;

/** @brief non-zero if simualtion shall shut down */
int halted;

/** @brief non-zero if simualtion shall pause */
int paused;

/** @brief non-zero next step in pause mode */
int step;

/** @brief non-zero if the core memory shall be dumped at exit */
int dump;

/** @brief non-zero if unknown/unmapped keys should be reported to stderr */
int report_key;

/** @brief MNG context if full frame snapshots are active */
mng_t *mng;

/** @brief number of bytes written to the MNG */
size_t xngsize;

#if	FRONTEND_ICONS
#define	BORDER_H	(30+4)
#define	BORDER_Y	30
#else
#define	BORDER_H	8
#define	BORDER_Y	(BORDER_H/2)
#endif
#define	BORDER_W	8
#define	BORDER_X	(BORDER_W/2)

static SDL_Surface *screen = NULL;
static SDL_Surface *alto = NULL;
static SDL_Surface *debug = NULL;
static SDL_Surface *pixels = NULL;
static SDL_Surface *charmap = NULL;
static SDL_Cursor *cursor = NULL;
#if	FRONTEND_ICONS
static SDL_Surface *iconmap = NULL;
#endif

#define	MOUSE_ICON_X	BORDER_X
#define	MOUSE_ICON_Y	DBG_FONT_H
#define	MNG_ICON_X	(BORDER_X + 20)
#define	MNG_ICON_Y	DBG_FONT_H

#define	BORDER_RGB	224,224,224
#define	BACKGROUND_RGB	255,255,255
#define	FOREGROUND_RGB	  0,  0,  0
#define	HIGHLIGHT_RGB	208,208,255

static int mousex;
static int mousey;
static int mouseb;
static char *bootimg_name = NULL;
static int minx = -1;
static int miny = -1;
static int maxx = -1;
static int maxy = -1;

/**
 * @brief make a cursor from a string of "pixels"
 *
 * A string of w*h characters defines the shape and mask of a cursor.
 * . is for an empty pixel (pix = 0, msk = 0)
 * x is for a set pixel (pix = 1, msk = 1)
 * # is for a masked pixel (pix = 0, msk = 1)
 * o is for a xor pixel (pix = 1, msk = 0)
 *
 * @param pix pointer to a buffer to receive the pixel data ((w*h+7)/8 bytes)
 * @param msk pointer to a buffer to receive the mask data ((w*h+7)/8 bytes)
 * @param w width of the cursor
 * @param h height of the cursor
 * @param src pointer to a string representing the cursor
 * @result 0 on success
 */
static int sdl_make_cursor(uint8_t *pix, uint8_t *msk, int w, int h, const char *src)
{
	int x, y;

	for (y = 0; y < h; y++) {
		*pix = 0;
		*msk = 0;
		for (x = 0; x < w; x++) {
			switch (*src++) {
			case '.':	/* both 0 */
				break;
			case 'x':
				*pix |= 0x80 >> (x % 8);
				*msk |= 0x80 >> (x % 8);
				break;
			case '#':
				*msk |= 0x80 >> (x % 8);
				break;
			case 'o':
				*pix |= 0x80 >> (x % 8);
				break;
			}
			if (7 == (x & 7)) {
				pix++;
				*pix = 0;
				msk++;
				*msk = 0;
			}
		}
		if (w & 7) {
			pix++;
			msk++;
		}
	}
	return 0;
}

/**
 * @brief SDL bit block transfer
 *
 * @param dst destination surface
 * @param src source surface
 * @param dx destination x
 * @param dy destination y
 * @param sx source x
 * @param sy source y
 * @param w width in pixels
 * @param h height in scanlines
 */
static __inline void sdl_blit(SDL_Surface *ds, SDL_Surface *ss,
	int dx, int dy, int sx, int sy, int w, int h)
{
	SDL_Rect sr;
	SDL_Rect dr;

	sr.x = sx;
	sr.y = sy;
	dr.x = dx;
	dr.y = dy;
	dr.w = sr.w = w;
	dr.h = sr.h = h;

	SDL_BlitSurface(ss, &sr, ds, &dr);
}

/**
 * @brief write a character to the surface border
 *
 * @param x x coordinate where to write the character
 * @param y y coordinate where to write the character
 * @param ch character code to write
 * @result 0 on success, -1 on error
 */
int border_putch(int x, int y, uint8_t ch)
{
	int sx = ch * DBG_FONT_W;
	int sy = 0;
	SDL_Rect sr;
	SDL_Rect dr;

	if (x < 0 || y < 0 || x >= screen->w || y >= screen->h)
		return -1;

	sr.x = sx;
	sr.y = sy;
	dr.x = x;
	dr.y = y;
	dr.w = sr.w = DBG_FONT_W;
	dr.h = sr.h = DBG_FONT_H;

	SDL_FillRect(screen, &dr, SDL_MapRGB(screen->format,BORDER_RGB));
	SDL_BlitSurface(charmap, &sr, screen, &dr);
	return 0;
}

/**
 * @brief print a string to the surface border
 *
 * @param x x coordinate where to write the character
 * @param y y coordinate where to write the character
 * @param fmt format string and additional arguments to print
 * @result 0 on success, -1 on error
 */
int border_printf(int x, int y, const char *fmt, ...)
{
	static char buff[256];
	int i, len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);
	for (i = 0; i < len; i++) {
		border_putch(x, y, buff[i]);
		x += DBG_FONT_W;
	}
	return len;
}

/**
 * @brief report a unknown key was pressed
 */
static void unknown_key(int sym)
{
	int i;
	for (i = 0; i < KEY_MAP_SIZE; i++)
		if (key_map[i].sym == sym)
			break;
	if (i < KEY_MAP_SIZE)
		printf("unmapped key [%s]\n", key_map[i].name);
	else
		printf("unknown key symbol #%d\n", sym);
}

/**
 * @brief shut down the SDL allocated surfaces and cursors
 */
static int sdl_close_display(void)
{
	if (NULL != screen) {
		SDL_FreeSurface(screen);
		screen = NULL;
	}
	if (NULL != alto) {
		SDL_FreeSurface(alto);
		alto = NULL;
	}
	if (NULL != debug) {
		SDL_FreeSurface(debug);
		debug = NULL;
	}
	if (NULL != cursor) {
		SDL_FreeCursor(cursor);
		cursor = NULL;
	}
	return 0;
}


/**
 * @brief callback for mng_finish() or png_finish() to emit a byte
 *
 * @param cookie the FILE pointer we supplied to png_create()
 * @param byte byte to write to the file
 * @result returns 0 on success, -1 on error
 */
static int fp_write_bytes(void *cookie, uint8_t *bytes, int size)
{
	FILE *fp = (FILE *)cookie;

	if (size != fwrite(bytes, 1, size, fp))
		return -1;

	/* count bytes written */
	xngsize += size;
	return 0;
}

/**
 * @brief stop MNG screenshot recording
 */
void screenmng_stop(void)
{
	size_t kb;
	FILE *fp;
	off_t pos;

	fp = xng_get_cookie(mng);
	if (!fp)
		return;

	/* remember where we are and seek to the file pos where the MHDR is */
	pos = ftell(fp);
	fseek(fp, 8, SEEK_SET);

	/* re-write the MHDR chunk with the (now) known counts */
	mng_write_MHDR(mng);

	/* seek to where we were and finish the mng */
	fseek(fp, pos, SEEK_SET);
	mng_finish(mng);

	mng = NULL;

	fclose(fp);
	kb = (xngsize + 1023) / 1024;
	sdl_draw_icon(MNG_ICON_X, MNG_ICON_Y, mng_off);
	border_printf(MNG_ICON_X, 0, "(%dKB)", kb);
	return;
}

/**
 * @brief start MNG screenshot recording
 */
void screenmng_start(void)
{
	static char filename[FILENAME_MAX] = "alto.mng";
	FILE *fp;

	if (mng)
		screenmng_stop();

	fp = fopen(filename, "wb");
	if (!fp)
		fatal(1, "Cannot create screenmng file '%s'\n", filename);

	mng = mng_create(DISPLAY_WIDTH, DISPLAY_HEIGHT,
		30 /* ticks per second */, fp, fp_write_bytes);

	if (!mng)
		fatal(1, "Cannot create MNG stream (%s)\n", strerror(errno));

	mng_set_palette(mng, 0, PNG_RGB(255,255,255));
	mng_set_palette(mng, 1, PNG_RGB(  0,  0,  0));
	mng->comment = "SALTO screen MNG";
	mng->author = "$Id: salto.c,v 1.2 2008/08/19 14:07:22 pm Exp $";
	xngsize = 0;
}

/**
 * @brief write a screen frame to the MNG stream
 */
void screenmng_frame(void)
{
	size_t kb;
	png_t *png;
	int left, right, top, bottom, w, h;

	/* recording is off, or simulation is paused */
	if (NULL == mng || paused)
		return;

	left = minx < DISPLAY_WIDTH - 1 ? minx : DISPLAY_WIDTH - 1;
	top = miny < DISPLAY_HEIGHT - 1 ? miny : DISPLAY_HEIGHT - 1;
	right = maxx < DISPLAY_WIDTH ? maxx : DISPLAY_WIDTH;
	bottom = maxy < DISPLAY_HEIGHT ? maxy : DISPLAY_HEIGHT;

	minx = -1;
	miny = -1;
	maxx = -1;
	maxy = -1;

	if (-1 == left || -1 == top || -1 == right || -1 == bottom) {
		/* write first 16 pixels rather than nothing (?) */
		left = 0;
		top = 0;
		right = 16;
		bottom = 1;
	}

	w = right - left;
	h = bottom - top;

	png = mng_append_png(mng, left, top, w, h, COLOR_PALETTE, 1);
	if (!png)
		fatal(1, "mng_append_png() failed (%s)\n",
			strerror(errno));

	/* append the screenshot */
	display_screenshot(png, left, top, right, bottom);

	/* write the progress info to the border */
	kb = (xngsize + 1023) / 1024;
	sdl_draw_icon(MNG_ICON_X, MNG_ICON_Y, mng_on0 + (mng_get_fcount(mng) & 3));
	border_printf(MNG_ICON_X, 0, "%dKB", kb);
}

/**
 * @brief create a screenshot PNG file
 */
static void screenshot(void)
{
	static uint16_t id;
	static char filename[FILENAME_MAX] = "alto0000.png";
	FILE *fp;
	png_t *png;

	snprintf(filename, sizeof(filename), "alto%04x.png", id++);

	fp = fopen(filename, "wb");
	if (!fp)
		fatal(1, "Cannot create screenshot file '%s'\n", filename);

	png = png_create(DISPLAY_WIDTH, DISPLAY_HEIGHT,
		COLOR_PALETTE, 1, fp, fp_write_bytes);
	if (!png)
		fatal(1, "Cannot create PNG image (%s)\n", strerror(errno));

	png_set_palette(png, 0, PNG_RGB(255,255,255));
	png_set_palette(png, 1, PNG_RGB(  0,  0,  0));
	png->comment = "SALTO screenshot";
	png->author = "$Id: salto.c,v 1.2 2008/08/19 14:07:22 pm Exp $";

	/* copy the display to the PNG */
	display_screenshot(png, 0, 0, DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1);

	/* and finish it */
	if (0 != png_finish(png))
		fatal(1, "Finishing PNG image failed (%s)\n", strerror(errno));

	fclose(fp);
}

#define	PC_START 0
static void bootimg(void)
{
	FILE *fp;
	int pc = PC_START;

	if (!bootimg_name) {
		printf("no boot image was specified\n");
		alto_soft_reset();
		return;
	}

	fp = fopen(bootimg_name, "rb");
	if (!fp)
		fatal(1, "Cannot fopen(%s)\n", bootimg_name);
	while (!feof(fp)) {
		int cl, ch;
		cl = fgetc(fp);
		ch = fgetc(fp);
		if (cl == -1 || ch == -1)
			break;
		debug_write_mem(pc, (ch << 8) | cl);
		pc++;
	}
	fclose(fp);

	alto_soft_reset();
	/* HACK: force r6 to the new program counter contained in 0 */
	cpu.r[6] = debug_read_mem(0);
	printf("loaded %s from %#o to %#o; changed PC to %#o\n",
		bootimg_name, PC_START, pc, cpu.r[6]);
}

/**
 * @brief switch between Alto bitmap view and debug output view
 *
 * @param which which surface to draw to screen (0 Alto, else debug)
 */
void debug_view(int which)
{
	dbg.visible = which;
	if (dbg.visible) {
		sdl_blit(screen, debug,
			BORDER_X, BORDER_Y, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
	} else {
		sdl_blit(screen, alto,
			BORDER_X, BORDER_Y, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
	}
	SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
}

/**
 * @brief update changes to the off screen surface to the screen, poll events
 *
 * Clips the changes that were made to the surface since the last call
 * to sdl_update() and blits them to the screen.
 *
 * Polls the SDL events and handles key presses and releases, mouse
 * movements and button clicks, and close window (exit) event.
 *
 * @param full set to non zero, if a full frame update was done (clears screen)
 * @result 0 on success
 */
int sdl_update(int full)
{
	static SDLMod mod_old;
	SDLMod mod_new;
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_VIDEORESIZE:
			if (NULL != screen)
				SDL_FreeSurface(screen);
			screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h,
				0, SDL_HWSURFACE | SDL_RESIZABLE);
			SDL_ShowCursor(sdl_cursor);
			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,BORDER_RGB));
			sdl_update(1);
			break;

		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym) {
			case SDLK_INSERT:
				bootimg();
				break;
			case SDLK_F10:
				halted = 1;
				break;
				break;
			case SDLK_RETURN:
				if (paused && ev.key.keysym.sym == SDLK_RETURN)
					step = 1;
				kbd_key(&ev.key.keysym, 1);
				break;
			case SDLK_PRINT:
				if (SDL_GetModState() & KMOD_LCTRL) {
					/* CTRL+PRINT starts/stops MNG recording */
					if (mng)
						screenmng_stop();
					else
						screenmng_start();
				} else {
					/* PRINT takes a single PNG screenshot */
					screenshot();
				}
				break;
			default:
				if (dbg.visible) {
					dbg_key(&ev.key.keysym, 1);
					break;
				}
				if (0 == kbd_key(&ev.key.keysym, 1))
					break;
				if (report_key)
					unknown_key(ev.key.keysym.sym);
			}
			break;

		case SDL_KEYUP:
			switch (ev.key.keysym.sym) {
			case SDLK_PAUSE:
				paused ^= 1;
				break;
			case SDLK_SCROLLOCK:
				debug_view(dbg.visible ^ 1);
				break;
			default:
				if (dbg.visible) {
					dbg_key(&ev.key.keysym, 0);
					break;
				}
				if (0 == kbd_key(&ev.key.keysym, 0))
					break;
			}
			break;

		case SDL_MOUSEMOTION:
			mousex = ev.motion.x;
			mousey = ev.motion.y;
			if (mousex < BORDER_X || mousey < BORDER_Y ||
				mousex >= BORDER_X + DISPLAY_WIDTH ||
				mousey >= BORDER_Y + DISPLAY_HEIGHT) {
				SDL_ShowCursor(1);
			} else {
				SDL_ShowCursor(sdl_cursor ^ 1);
			}
			if (sdl_cursor) {
				mouse_motion(mousex - BORDER_X, mousey - BORDER_Y);
				if ((mouseb ^ ev.motion.state) & 1) {
					mouseb = (mouseb & ~1) | (ev.motion.state & 1);
					mouse_button(mouseb);
				}
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (mouse.y < BORDER_Y) {
				/* click on mouse icon ? */
				if (mouse.x >= MOUSE_ICON_X && mouse.x < MOUSE_ICON_X + 16) {
					sdl_cursor ^= 1;
					if (sdl_cursor) {
						sdl_draw_icon(MOUSE_ICON_X, MOUSE_ICON_Y, mouse_on);
					} else {
						sdl_draw_icon(MOUSE_ICON_X, MOUSE_ICON_Y, mouse_off);
					}
				}
				/* click on mouse the MNG icon ? */
				if (mouse.x >= MNG_ICON_X && mouse.x <= MNG_ICON_X + 16) {
					if (mng)
						screenmng_stop();
					else
						screenmng_start();
				}
			}
			if (sdl_cursor) {
				mouseb |= SDL_BUTTON(ev.button.button);
				mouse_button(mouseb);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			if (sdl_cursor) {
				mouseb &= ~SDL_BUTTON(ev.button.button);
				mouse_button(mouseb);
			}
			break;

		case SDL_QUIT:
			halted = 1;
		}
	}

	/* check for GRABKEYS combination (Ctrl+Alt) */
	mod_new = SDL_GetModState();
	if ((mod_new & GRABKEYS) == GRABKEYS) {
		if ((mod_old & GRABKEYS) != GRABKEYS) {
			if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON) {
				char buff[256];
				snprintf(buff, sizeof(buff),
					"%s (Ctrl+Alt to grab mouse)", title);
				SDL_WM_SetCaption(buff, buff);
				sdl_cursor = 0;
				sdl_draw_icon(MOUSE_ICON_X, MOUSE_ICON_Y, mouse_off);
			} else {
				char buff[256];
				snprintf(buff, sizeof(buff),
					"%s (Ctrl+Alt to release mouse)", title);
				SDL_WM_SetCaption(buff, buff);
				sdl_cursor = 1;
				sdl_draw_icon(MOUSE_ICON_X, MOUSE_ICON_Y, mouse_on);
			}
		}
	}
	mod_old = mod_new;

	if (full) {
		SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
		screenmng_frame();
	}

	return 0;
}

/**
 * @brief draw a LED icon to the bottom of the SDL surface
 *
 * @param x x coordinate where to draw the icon
 * @param y y coordinate where to draw the icon
 * @param type one of the led_t enum
 * @result 0 on success, -1 on error
 */
int sdl_draw_icon(int x, int y, int type)
{
#if	FRONTEND_ICONS
	if (type >= led_count)
		return -1;
	sdl_blit(screen, iconmap, x, y + 1, type * 16, 0, 16, 16);
	SDL_UpdateRect(screen, x, y + 1, 16, 16);
#endif
	return 0;
}

/**
 * @brief write a word with 16 pixels to the SDL surface
 *
 * @param x x coordinate where to write the word
 * @param y y coordinate where to write the word
 * @param pixel the 16 lower bits, MSB to LSB, define the 16 pixels
 * @result 0 on success, -1 on error
 */
int sdl_write(int x, int y, uint32_t pixel)
{
	int sx = (pixel / 256) * 16;
	int sy = pixel % 256;

	if (x < 0 || y < 0 || y >= DISPLAY_HEIGHT || x >= DISPLAY_WIDTH)
		return -1;

	if (-1 == minx || x < minx)
		minx = x;
	if (-1 == maxx || (x + 16) > maxx)
		maxx = x + 16;
	if (-1 == miny || y < miny)
		miny = y;
	if (-1 == maxy || (y + 1) > maxy)
		maxy = y + 1;

	sdl_blit(alto, pixels, x, y, sx, sy, 16, 1);
	if (!dbg.visible) {
		sdl_blit(screen, pixels, x + BORDER_X, y + BORDER_Y,
			sx, sy, 16, 1);
	}
	return 0;
}

/**
 * @brief count the bits at and around a bit at a charmap
 *
 * @param cg pointer to the character generator
 * @param x pixel column
 * @param y pixel row
 * @result matrix weighted value for pixel, 0 if outside char
 */
static int bitcount(const chargen_t *cg, int i, int x, int y)
{
	int shift;
	int tl, t, tr, l, c, r, bl, b, br;
	const uint8_t *p = &cg->data[i + y];

	if (x < 0 || x >= cg->width || y < 0 || y >= cg->height)
		return 0;

	if (x > 0) {
		shift = 8 - x;
		l = (p[0] >> shift) & 1;
		tl = (y > 0) ? (p[-1] >> shift) & 1 : 0;
		bl = (y + 1 < cg->height) ? (p[+1] >> shift) & 1 : 0;
	} else {
		l = tl = bl = 0;
	}

	if (x + 1 < cg->width) {
		shift = 6 - x;
		r = (p[0] >> shift) & 1;
		tr = (y > 0) ? (p[-1] >> shift) & 1 : 0;
		br = (y + 1 < cg->height) ? (p[+1] >> shift) & 1 : 0;
	} else {
		r = tr = br = 0;
	}

	shift = 7 - x;
	t = (y > 0) ? (p[-1] >> shift) & 1 : 0;
	b = (y + 1 < cg->height) ? (p[+1] >> shift) & 1 : 0;
	c = (p[0] >> shift) & 1;

	return	tl * c * 2 + t * 3 + tr * c * 2 +
		l      * 3 + c *10 + r      * 3 +
		bl * c * 2 + b * 3 + br * c * 2;
}

/**
 * @brief render character generator bitmaps for one color
 *
 * @param cg pointer to a character generator
 * @param col color index (0 to DBG_COLORS-1)
 * @param rgb values for red, green, blue
 * @result 0 on sucess, -1 on error
 */
int sdl_chargen_alpha(const chargen_t *cg, int col, uint32_t rgb)
{
	SDL_PixelFormat *fmt;
	SDL_Rect rc;
	uint32_t ramp[8];
	int r = sdl_get_r(rgb);
	int g = sdl_get_g(rgb);
	int b = sdl_get_b(rgb);
	int i, ch, x, y;

	/* convert chargen to font */
	fmt = charmap->format;

	ramp[0] = SDL_MapRGBA(fmt, r, g, b,0*255/7);
	ramp[1] = SDL_MapRGBA(fmt, r, g, b,1*255/7);
	ramp[2] = SDL_MapRGBA(fmt, r, g, b,2*255/7);
	ramp[3] = SDL_MapRGBA(fmt, r, g, b,3*255/7);
	ramp[4] = SDL_MapRGBA(fmt, r, g, b,4*255/7);
	ramp[5] = SDL_MapRGBA(fmt, r, g, b,5*255/7);
	ramp[6] = SDL_MapRGBA(fmt, r, g, b,6*255/7);
	ramp[7] = SDL_MapRGBA(fmt, r, g, b,7*255/7);

	rc.w = 1;
	rc.h = 1;
	for (i = 0; i < cg->size; i += cg->height + 1) {
		/* first byte is character code */
		ch = cg->data[i];
		rc.y = col * cg->height;
		for (y = 0; y < cg->height; y++, rc.y++) {
			rc.x = ch * cg->width;
			for (x = 0; x < cg->width; x++, rc.x++) {
				switch (bitcount(cg, i + 1, x, y)) {
				case 0: case 1: case 2:
					SDL_FillRect(charmap, &rc, ramp[0]);
					break;
				case 3:
					SDL_FillRect(charmap, &rc, ramp[1]);
					break;
				case 4:
					SDL_FillRect(charmap, &rc, ramp[2]);
					break;
				case 5: case 6:
					SDL_FillRect(charmap, &rc, ramp[3]);
					break;
				case 7: case 8:
					SDL_FillRect(charmap, &rc, ramp[4]);
					break;
				case 9: case 10:
					SDL_FillRect(charmap, &rc, ramp[5]);
					break;
				case 11: case 12: case 13:
					SDL_FillRect(charmap, &rc, ramp[6]);
					break;
				default:
					SDL_FillRect(charmap, &rc, ramp[7]);
					break;
				}
			}
		}
	}

	return 0;
}

/**
 * @brief write a character to the SDL debug surface
 *
 * @param x x coordinate where to write the word
 * @param y y coordinate where to write the word
 * @param ch character code to write
 * @result 0 on success, -1 on error
 */
int sdl_debug(int x, int y, int ch, int color)
{
	int sx = ch * DBG_FONT_W;
	int sy = (color % DBG_COLORS) * DBG_FONT_H;
	SDL_Rect sr;
	SDL_Rect dr;

	if (x < 0 || y < 0 || x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
		return -1;

	sr.x = sx;
	sr.y = sy;
	dr.x = x;
	dr.y = y;
	dr.w = sr.w = DBG_FONT_W;
	dr.h = sr.h = DBG_FONT_H;

	if (color & 8) {
		SDL_FillRect(debug, &dr, SDL_MapRGB(debug->format,HIGHLIGHT_RGB));
	} else {
		SDL_FillRect(debug, &dr, SDL_MapRGB(debug->format,BACKGROUND_RGB));
	}
	SDL_BlitSurface(charmap, &sr, debug, &dr);
	if (dbg.visible) {
		sr.x = dr.x;
		sr.y = dr.y;
		dr.x = BORDER_X + x;
		dr.y = BORDER_Y + y;
		SDL_BlitSurface(debug, &sr, screen, &dr);
	}
	return 0;
}


/**
 * @brief shut down SDL
 */
static void sdl_exit(void)
{
	screenmng_stop();
	sdl_close_display();
	SDL_Quit();
}

/**
 * @brief open a SDL display and set its window title
 *
 * @param width width of screen buffer in pixels
 * @param height height of screen buffer in pixels
 * @param depth desired bits per pixel (0 = use default)
 * @param title pointer to a string to put in the window title bar
 * @result 0 on success
 */
static int sdl_open_display(int width, int height, int depth, const char *title)
{
	char caption[256];
	uint8_t buff[256];
	uint32_t rmask, gmask, bmask, amask;
	uint32_t flags;

#if	SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#elif	SDL_BYTEORDER == SDL_LIL_ENDIAN
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#else
#error SDL byte order not defined
#endif

	flags = SDL_HWSURFACE | SDL_RESIZABLE /* | SDL_FULLSCREEN */;
	screen = SDL_SetVideoMode(width, height, 0, flags);
	if (NULL == screen) {
		fatal(1,"SDL_SetVideoMode(%d,%d,%d,0x%x) failed\n",
			width, height, depth, flags);
	}

	fprintf(stdout, "SDL_SetVideoMode(%d,%d,%d,0x%x) ok\n",
			screen->w, screen->h, screen->format->BitsPerPixel, flags);

	/* fill rect */
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,BORDER_RGB));
	SDL_UpdateRect(screen, 0, 0, width, height);

	if (NULL != title)
		snprintf(caption, sizeof(caption), "%s", title);
	else
		snprintf(caption, sizeof(caption), "Unknown");
	SDL_WM_SetCaption(caption, caption);

	sdl_make_cursor(&buff[0x00], &buff[0x80], 10, 16,
		"xx........" \
		"x#x......." \
		"x##x......" \
		"x###x....." \
		"x####x...." \
		"x#####x..." \
		"x######x.." \
		"x#######x." \
		"x########x" \
		"x#####xxxx" \
		"x##x##x..." \
		"x#x.x##x.." \
		"xx..x##x.." \
		".....x##x." \
		".....x##x." \
		"......xx..");

	cursor = SDL_CreateCursor(&buff[0x00], &buff[0x80], 10, 16, 0, 0);
	SDL_SetCursor(cursor);

	return 0;
}

/**
 * @brief initialize SDL window and set a title
 */
static int sdl_init(int width, int height, int depth, const char *title)
{
	int n, x, y;
	SDL_Rect dst;
	uint32_t rmask, gmask, bmask, amask;
	uint32_t colors[2];

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
	atexit(sdl_exit);

	SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
	SDL_EventState(SDL_KEYUP, SDL_ENABLE);
	SDL_EventState(SDL_QUIT, SDL_ENABLE);

	SDL_EnableKeyRepeat(250, 25);

	if (-1 == width)
		width = 606;
	if (-1 == height)
		height = 808;
	if (-1 == depth)
		depth = 1;

	if (0 != sdl_open_display(width + BORDER_W, height + BORDER_H, depth, title))
		fatal(1, "sdl_open_display() failed\n");

	/* take the screen's bits per pixel as depth for the surface, too */
	depth = 32;

#if	SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#elif	SDL_BYTEORDER == SDL_LIL_ENDIAN
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#else
#error SDL byte order not defined
#endif
	alto = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_ASYNCBLIT,
		width, height, depth, rmask, gmask, bmask, amask);
	if (NULL == alto)
		return -1;

	/* create a 16x256xdepth surface as blit source for words */
	pixels = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_ASYNCBLIT,
		16*256, 256, depth, rmask, gmask, bmask, amask);
	if (NULL == pixels)
		return -1;

	debug = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_ASYNCBLIT,
		width, height, depth, rmask, gmask, bmask, amask);
	if (NULL == debug)
		return -1;

	/* create a RGBA surface as blit source for characters */
	charmap = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_SRCALPHA,
		DBG_FONT_W * 257, DBG_FONT_H * DBG_COLORS, 32, rmask, gmask, bmask, amask);
	if (NULL == charmap)
		return -1;
	SDL_SetAlpha(charmap, SDL_SRCALPHA, 255);

#if	FRONTEND_ICONS
	/* create a led_count*16x16xdepth surface as blit source for LED icons */
	iconmap = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_ASYNCBLIT,
		led_count*16, 16, 32, rmask, gmask, bmask, amask);
	if (NULL == iconmap)
		return -1;
#endif
	dst.x = 0;
	dst.y = 0;
	dst.w = width;
	dst.h = height;

	colors[0] = SDL_MapRGB(alto->format,BACKGROUND_RGB);
	colors[1] = SDL_MapRGB(alto->format,FOREGROUND_RGB);

	SDL_SetClipRect(alto, &dst);
	SDL_FillRect(alto, NULL, SDL_MapRGB(alto->format,BACKGROUND_RGB));

	SDL_SetClipRect(debug, &dst);
	SDL_FillRect(debug, NULL, SDL_MapRGB(debug->format,BACKGROUND_RGB));

	dst.w = 1;
	dst.h = 1;
	for (y = 0; y < 256; y++) {
		dst.y = y;
		for (x = 0; x < 256; x++) {
			dst.x = x * 16;
			for (n = 0; n < 8; n++) {
				SDL_FillRect(pixels, &dst, colors[(x >> (7-n)) & 1]);
				dst.x += dst.w;
			}
			for (n = 0; n < 8; n++) {
				SDL_FillRect(pixels, &dst, colors[(y >> (7-n)) & 1]);
				dst.x += dst.w;
			}
		}
	}

#if	FRONTEND_ICONS
	dst.w = 1;
	dst.h = 1;
	for (n = 0; n < led_count; n++) {
		uint8_t *src = NULL;
		switch (n) {
		case led_blank:      src = ppm_led_blank;  break;
		case led_0:          src = ppm_led_0;      break;
		case led_1:          src = ppm_led_1;      break;
		case led_2:          src = ppm_led_2;      break;
		case led_3:          src = ppm_led_3;      break;
		case led_4:          src = ppm_led_4;      break;
		case led_5:          src = ppm_led_5;      break;
		case led_6:          src = ppm_led_6;      break;
		case led_7:          src = ppm_led_7;      break;
		case led_8:          src = ppm_led_8;      break;
		case led_9:          src = ppm_led_9;      break;
		case led_green_off:  src = ppm_green_off;  break;
		case led_green_on:   src = ppm_green_on;   break;
		case led_red_off:    src = ppm_red_off;    break;
		case led_red_on:     src = ppm_red_on;     break;
		case led_yellow_off: src = ppm_yellow_off; break;
		case led_yellow_on:  src = ppm_yellow_on;  break;
		case mouse_off:      src = ppm_mouse_off;  break;
		case mouse_on:       src = ppm_mouse_on;   break;
		case mng_off:        src = ppm_mng_off;    break;
		case mng_on0:        src = ppm_mng_on0;    break;
		case mng_on1:        src = ppm_mng_on1;    break;
		case mng_on2:        src = ppm_mng_on2;    break;
		case mng_on3:        src = ppm_mng_on3;    break;
		default:
			fatal(1, "not all LED icons handled\n");
		}
		for (y = 0; y < 16; y++) {
			for (x = 0; x < 16; x++) {
				uint32_t col;
				col = SDL_MapRGB(iconmap->format, src[0], src[1], src[2]);
				dst.x = 16 * n + x;
				dst.y = y;
				SDL_FillRect(iconmap, &dst, col);
				src += 3;
			}
		}
	}
#endif

	sdl_cursor = 1;
	sdl_draw_icon(MOUSE_ICON_X, MOUSE_ICON_Y, mouse_on);
	sdl_draw_icon(MNG_ICON_X, MNG_ICON_Y, mng_off);
	return 0;
}

/**
 * @brief print fatal error and exit(exitcode)
 *
 * @param exitcode return code from application
 * @param fmt format string and optional arguments
 */
void fatal(int exitcode, const char *fmt, ...)
{
	va_list ap;

	va_start(ap,fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);

	exit(exitcode);
}

/**
 * @brief print usage info and exit(0)
 *
 * @param argc argument count
 * @param argv array of argument strings
 */
static void usage(int argc, char **argv)
{
	char *exe = argv[0];

	if (strrchr(exe, '/'))
		exe = strrchr(exe, '/') + 1;
	printf("usage: %s [options] binary_image\n", exe);
	printf("options can be one or more of\n");
	dbg_usage(argc, argv);
	drive_usage(argc, argv);
	ether_usage(argc, argv);
	printf("-dc		dump (Alto) core to file 'alto.dump' at exit\n");
	printf("-kr		report unkown/unhandled key press (to stderr)\n");
	printf("-b key		set a boot key (5,4,6,e,7,d,u,v,0,k,-,p,/,\\,lf,bs)\n");
	printf("-d		start in paused mode and debug view\n");
	printf("-h		display this help\n");
	exit(0);
}


/**
 * @brief Salto main entry
 *
 * Initializes the SDL window, loads (P)ROM images, intializes the
 * various subsystems and goes into a loop polling for timer events
 * and executing CPU time slices.
 *
 * @param argc argument count
 * @param argv array of argument strings
 */
int main(int argc, char **argv)
{
	int i, drive;

	/* initialize SDL to 606x808x[default] screen */
	sdl_init(DISPLAY_WIDTH, DISPLAY_HEIGHT, -1, title);

	alto_init("roms");
	timer_init();
	init_memory();
	init_hardware();
	init_kbd();
	init_printer();
	init_eia();
	drive_init();
	disk_init();
	display_init();
	mouse_init();
	debug_init();

	for (i = 1, drive = 0; i < argc; i++) {
		if (argv[i][0] == '-' || argv[i][0] == '+') {
#if	DEBUG
			if (0 == dbg_args(argv[i])) {

			} else
#endif
			if (0 == ether_args(argv[i])) {
				/* ethernet accepted the switch */
			} else if (0 == drive_args(argv[i])) {
				/* drive code accepted the switch */
			} else if (!strcmp(argv[i], "-dc")) {
				dump = 1;	/* dump core at exit */
			} else if (!strcmp(argv[i], "-kr")) {
				report_key = 1;	/* report unknown keys */
			} else if (!strcmp(argv[i], "-d")) {
				paused = 1;	/* start paused */
				step = 0;	/* don't step */
				debug_view(1);	/* view debug surface */
			} else if (!strcmp(argv[i], "-b")) {
				/* next arg is key name */
				++i;
				kbd_boot(argv[i]);
			} else if (argv[i][1] == 'h') {
				usage(argc, argv);
			} else {
				fprintf(stderr, "warning: ignored unknown switch: %s\n", argv[i]);
			}
		} else if (0 == drive_args(argv[i])) {
			disk_show_image_name(drive++);
		} else if (0 == ether_args(argv[i])) {
			/* ether accepted a name */
		} else {
			bootimg_name = argv[i];
		}
	}
	drive_select(0, 0);
	alto_reset();

#if	DEBUG
	while (!halted) {
		ntime_t run, ran;
		while ((run = timer_next_time()) < CPU_MICROCYCLE_TIME)
			timer_fire();
		if (run <= 0 || run > CPU_MICROCYCLE_TIME)
			run = CPU_MICROCYCLE_TIME;

		global_ntime += run;
		ran = alto_execute(run);
		global_ntime += ran - run;

		if (dbg.visible && ll[cpu.task].level > 0) {
			dbg_dump_regs();
			sdl_update(0);
		}
		step = 0;
		/* In debug mode update often, so we can easily break out of SDL */
		while (paused && !step && !halted) {
			dbg_dump_regs();
			sdl_update(0);
		}
	}
#else
	while (!halted) {
		ntime_t run, ran;
		while ((run = timer_next_time()) < CPU_MICROCYCLE_TIME)
			timer_fire();
		if (run <= 0)
			run = 5000 * CPU_MICROCYCLE_TIME;
		global_ntime += run;
		ran = alto_execute(run);
		global_ntime += ran - run;
		while (paused && !halted) {
			dbg_dump_regs();
			sdl_update(1);
		}
	}
#endif
	if (dump) {
		FILE *fp;
		int pc;

		fp = fopen("alto.dump", "wb");
		for (pc = 0; pc < RAM_SIZE; pc++) {
			uint8_t bytes[2];
			int data = debug_read_mem(pc);
			bytes[0] = data % 256;
			bytes[1] = data / 256;
			fwrite(bytes, 1, 2, fp);
		}
		fclose(fp);
	}
	return 0;
}
