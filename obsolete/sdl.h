/************************************************************************
 *
 * Linux DVD copy
 *
 * Copyright (C) 2007, Juergen Buchmueller <pullmoll@t-online.de>
 *
 * Partially based on ShrinkTo5, Copyright (C) 2004-2005
 * Ocrana <info@shrinkto5.com> and Andrei Grecu <info@shrinkto5.com>
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
 * SDL interface
 *
 ************************************************************************/
#if !defined(_sdl_h_included_)
#define	_sdl_h_included_

#include "config.h"

#if	defined(RENDER_TO_SDL) && RENDER_TO_SDL
/** @brief non-zero if a sdl is non-NULL and contains the sdl tag */
#define	SDL_VALID(sdl) CMPTAG(sdl,SDL_TAG)

/** @brief structure of a sdl context palette entry */
typedef struct {
	/** @brief red value 0 to 255 */
	uint8_t r;
	/** @brief green value 0 to 255 */
	uint8_t g;
	/** @brief blue value 0 to 255 */
	uint8_t b;
	/** @brief dummy (used for alpha in ARGB modes) */
	uint8_t dummy;
}	sdl_pal_t;

/** @brief structure of a sdl context rectangle */
typedef struct {
	int x;
	int y;
	int w;
	int h;
}	rect_t;

/** @brief the structure of a surface's format */
typedef struct {
	/** @brief bits per pixel */
	uint8_t bpp;

	/** @brief pixel bytes */
	uint8_t pb;

	/** @brief red loss */
	uint8_t rl;

	/** @brief green loss */
	uint8_t gl;

	/** @brief blue loss */
	uint8_t bl;

	/** @brief alpha loss */
	uint8_t al;

	/** @brief red shift */
	uint8_t rs;

	/** @brief green shift */
	uint8_t gs;

	/** @brief blue shift */
	uint8_t bs;

	/** @brief alpha shift */
	uint8_t as;

	/** @brief red mask */
	uint32_t rm;

	/** @brief green mask */
	uint32_t gm;

	/** @brief blue mask */
	uint32_t bm;

	/** @brief alpha mask */
	uint32_t am;

	/** @brief color key */
	uint32_t ck;

	/** @brief alpha value information (per-surface alpha) */
	uint8_t alpha;

}	sdl_format_t;

/** @brief text color names */
typedef enum {
	/** @brief black on white background */
	TEXT_BLACK,
	/** @brief blue on white background */
	TEXT_BLUE,
	/** @brief green on white background */
	TEXT_GREEN,
	/** @brief cyan on white background */
	TEXT_CYAN,
	/** @brief red on white background */
	TEXT_RED,
	/** @brief magenta on white background */
	TEXT_MAGENTA,
	/** @brief yellow on white background */
	TEXT_YELLOW,
	/** @brief gray on white background */
	TEXT_GRAY,
	/** @brief mask for selected colors (light blue background, brighter colors) */
	TEXT_SEL = 8,
	/** @brief mask for button colors (gray background; may be combined with TEXT_SEL) */
	TEXT_BUTTON = 16,
	/** @brief text color count */
	TEXT_COLORS = 32,
	/** @brief transparency flag (may be combined with color) */
	TEXT_TRANSPARENT = 32
}	txt_color_t;

/** @brief structure of an sdl text pane context */
typedef struct {
	/** @brief pixel x origin */
	int xorg;

	/** @brief pixel y origin */
	int yorg;

	/** @brief pixel width of the pane */
	int width;

	/** @brief pixel height of the pane */
	int height;

	/** @brief pixel x offset of the text output */
	int tx;

	/** @brief pixel y offset of the text output */
	int ty;

	/** @brief width of the text buffer */
	ssize_t tw;

	/** @brief height of the text buffer */
	ssize_t th;

	/** @brief cursor x */
	int cx;

	/** @brief cursor y */
	int cy;

	/** @brief clip width of the text buffer */
	int cw;

	/** @brief clip height of the text buffer */
	int ch;

	/** @brief text buffer */
	char *text;

	/** @brief color map buffer */
	char *cmap;

	/** @brief background color RGB (default, selected, button face, selected button face) */
	uint32_t rgb_bg[4];

	/** @brief top, left border color RGB  */
	uint32_t rgb_tl;

	/** @brief bottom, right border color RGB  */
	uint32_t rgb_br;

}	sdl_text_t;

/** @brief enumeration of the text panes */
typedef enum {
	SDL_MENUE_PANE,
	SDL_MAIN_PANE,
	SDL_STATUS_PANE,
	SDL_PANES
}	sdl_pane_t;

/** @brief structure of an SDL context */
typedef	struct {
	/** @brief tag to verify validity of a sdl_t pointer */
	DEFTAG

	/** @brief non-zero when closing down */
	int closing;

	/** @brief width of SDL surface */
	int width;

	/** @brief height of SDL surface */
	int height;

	/** @brief additional height of SDL surface (border) */
	int border;

	/** @brief bits per pixel of SDL surface */
	int bpp;

	/** @brief last reported mouse x */
	int mousex;

	/** @brief last reported mouse y */
	int mousey;

	/** @brief last reported mouse buttons */
	int mouseb;

	/** @brief SDL screen surface */
	SDL_Surface *scr;

	/** @brief SDL bitmap surface */
	SDL_Surface *bmap;

	/** @brief SDL font surface */
	SDL_Surface *font;

	/** @brief font character cell width in pixel columns */
	int font_w;

	/** @brief font character cell height in scanlines */
	int font_h;

	/** @brief SDL logo surface */
	SDL_Surface *logo;

	/** @brief SDL cursor image */
	SDL_Cursor *cur;

	/** @brief the various text panes */
	sdl_text_t pane[SDL_PANES];

}	sdl_t;

/** @brief structure of the (internal) character generator data */
typedef struct {
	/** @brief width of characters in this generator */
	int width;
	/** @brief height of characters in this generator */
	int height;
	/** @brief character code and bitmap data */
	const uint8_t *data;
	/** @brief number of bytes of dtaa */
	size_t size;
}	chargen_t;

/** @brief enumeration of the button types */
typedef enum {
	/** @brief button with a little + or - inside */
	SDL_BTYPE_SELECT,

	/** @brief check box with or without a mark inside */
	SDL_BTYPE_CHECKBOX,

	/** @brief decrement "<" */
	SDL_BTYPE_DEC,

	/** @brief increment ">" */
	SDL_BTYPE_INC,

	/** @brief double decrement "<<" */
	SDL_BTYPE_DEC2,

	/** @brief double increment ">>" */
	SDL_BTYPE_INC2,

	/** @brief left arrow button */
	SDL_BTYPE_LEFT,

	/** @brief right arrow button */
	SDL_BTYPE_RIGHT,

	/** @brief up arrow button */
	SDL_BTYPE_UP,

	/** @brief down arrow button */
	SDL_BTYPE_DOWN

}	sdl_button_type_t;

/** @brief structure of one set of graph data */
typedef struct sdl_graph_s {
	/** @brief pointer to the next graph on the same canvas, if any */
	struct sdl_graph_s *next;

	/** @brief color of the graph */
	uint32_t col;

	/** @brief current number of data points */
	ssize_t num;

	/** @brief maximum number of data points, i.e. size of value array */
	ssize_t max;

	/** @brief minimum scale value (bottom) */
	double minval;

	/** @brief maximum scale value (top) */
	double maxval;

	/** @brief array of num values */
	double *value;

}	sdl_graph_t;	

/** @brief structure of a slider context */
typedef struct sdl_slider_s {
	/** @brief slider type (horizontal or vertical) */
	int type;

	/** @brief minimum value */
	int64_t min;

	/** @brief maximum value */
	int64_t max;

	/** @brief current value */
	int64_t cur;

	/** @brief range of the slider, i.e. size of the thumb-track button */
	int64_t range;

	/** @brief left or top of the thumb-track button (read-only) */
	int pos0;

	/** @brief right or bottom of the thumb-track button (read-only) */
	int pos1;

}	sdl_slider_t;

#define	sdl_rgb(r,g,b) (((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|((uint32_t)(b)<<0))
#define	sdl_get_r(rgb) ((uint8_t)((rgb)>>16))
#define	sdl_get_g(rgb) ((uint8_t)((rgb)>> 8))
#define	sdl_get_b(rgb) ((uint8_t)((rgb)>> 0))

#ifdef	__cplusplus
extern "C" {
#endif

/** @brief allocate a dynamic sdl_t context */
extern sdl_t *sdl_alloc(void);

/** @brief free a dynamic sdl_t context */
extern int sdl_free(sdl_t *sdl);

/** @brief initialize a sdl_t context */
extern int sdl_init(sdl_t *sdl, int width, int height, int bwidth, int bheight);

/** @brief zap a sdl_t context, i.e. make it invalid */
extern int sdl_zap(sdl_t *sdl);

/** @brief set a color in the palette */
extern int sdl_setcol(sdl_t *sdl, int idx, uint8_t r, uint8_t g, uint8_t b);

/** @brief render one font with alpha blending */
extern int sdl_chargen_alpha(sdl_t *sdl, const chargen_t *cg, int col, uint32_t rgb);

/** @brief render one font ramped between two colors */
extern int sdl_chargen(sdl_t *sdl, int which);

/** @brief get (and lock) the screen surface bitmap pixels address for coordinates x, y */
extern uint8_t *sdl_lock_bitmap(sdl_t *sdl, int x, int y);

/** @brief unlock screen surface bitmap pixels */
extern int sdl_unlock_bitmap(sdl_t *sdl);

/** @brief clear the screen surface bitmap */
extern int sdl_clear_bitmap(sdl_t *sdl, uint32_t rgb);

/** @brief load a PPM file and create a surface from it */
extern SDL_Surface *sdl_load_ppm(sdl_t *sdl, const char *filename);

/** @brief blit a surface to the screen (to preview area width, height) */
extern int sdl_blit(sdl_t *sdl, SDL_Surface *src);

/** @brief get the bitmap surface stride, i.e. the scanline pitch */
extern ssize_t sdl_get_stride(sdl_t *sdl);

/** @brief get the bitmap surface format (Alpha, Red, Green, and Blue losses and shifts etc.) */
extern int sdl_get_format(sdl_t *sdl, sdl_format_t *format);

/** @brief get the mouse cursor text area x and y coordinates and button status */
extern int sdl_get_mouse(sdl_t *sdl, sdl_pane_t *pane, int *x, int *y, int *b);

/** @brief print a string on the border area (va_list variant) */
extern ssize_t sdl_vprintf(sdl_t *sdl, sdl_pane_t pane, rect_t *prc,
	int x, int y, int col, const char *fmt, va_list ap);

/** @brief print a string on the border area (var arg variant) */
extern ssize_t sdl_printf(sdl_t *sdl, sdl_pane_t pane, rect_t *prc,
	int x, int y, int col, const char *fmt, ...);

/** @brief draw a (two color) frame around a pane */
extern int sdl_frame_pane(sdl_t *sdl, sdl_pane_t pane, int n);

/** @brief draw a (two color) frame around a text rectangle */
extern int sdl_frame_rect(sdl_t *sdl, sdl_pane_t pane, rect_t *prc, int n, uint32_t rgb0, uint32_t rgb1);

/** @brief fill a text rectangle with a solid color */
extern int sdl_fill_rect(sdl_t *sdl, sdl_pane_t pane, rect_t *prc, uint32_t rgb);

/** @brief underline a text rectangle with a solid color */
extern int sdl_underline(sdl_t *sdl, sdl_pane_t pane, rect_t *prc, uint32_t rgb);

/** @brief draw a button */
extern int sdl_draw_button(sdl_t *sdl, sdl_pane_t pane, rect_t *prc, sdl_button_type_t type,
	int on, uint32_t rgb0, uint32_t rgb1, uint32_t rgb2, uint32_t rgb3);

/** @brief draw a slider */
extern int sdl_draw_slider(sdl_t *sdl, sdl_pane_t pane, rect_t *prc, sdl_slider_t *slider,
	int border, uint32_t rgb0, uint32_t rgb1, uint32_t rgb2, uint32_t rgb3);

/** @brief draw a graph on a pane */
extern int sdl_draw_graph(sdl_t *sdl, sdl_pane_t pane, rect_t *prc, sdl_graph_t *graph);

/** @brief plot a pixel on a pane */
extern int sdl_plot(sdl_t *sdl, sdl_pane_t pane, int x, int y, uint32_t rgb);

/** @brief dirty a text rectangle */
extern int sdl_dirty_rect(sdl_t *sdl, sdl_pane_t pane, rect_t *prc);

/** @brief draw a progress bar in a text rectangle */
extern int sdl_progress_bar(sdl_t *sdl, sdl_pane_t pane, rect_t *prc,
	uint32_t rgb0, uint32_t rgb1, uint32_t rgb2, int min, int cur, int max);

/** @brief update the SDL event queue and display */
extern int sdl_update(sdl_t *sdl, int what);

/** @brief delay a number of milliseconds */
extern int sdl_delay(sdl_t *sdl, int ms);

/** @brief start a one-shot timer to call callback(cookie) */
extern int sdl_set_timer(sdl_t *sdl, int ms, void *cookie, void (*callback)(void *cookie));

#ifdef	__cplusplus
}
#endif
#endif	/* RENDER_TO_SDL */

#endif	/* !defined(_sdl_h_included_) */
