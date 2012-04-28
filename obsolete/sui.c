/************************************************************************
 *
 * Simple User Interface
 *
 * Copyright (C) 2007, Juergen Buchmueller <pullmoll@t-online.de>
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
 * This requires sdl.c/h to be included in the project as well
 *
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "config.h"
#include "util.h"
#include "debug.h"
#include "sui.h"

/** @brief RGB color to use for top and left edges of a 3D rectangle */
#define	RGB_3D_TOPLEFT		sdl_rgb(224,224,224)

/** @brief RGB color to use for bottom and right edges of a 3D rectangle */
#define	RGB_3D_BOTTOMRIGHT	sdl_rgb( 96, 96, 96)

/** @brief RGB color to use for the face of a button */
#define	RGB_BUTTON_FACE		sdl_rgb(192,192,192)

/** @brief RGB color to use for the text of a selbutton or checkbox */
#define	RGB_BUTTON_TEXT		sdl_rgb(  0,  0,  0)

/** @brief RGB color to use for the left section of a progress bar */
#define	RGB_PROGRESS_LEFT	sdl_rgb(  0, 64,255)

/** @brief RGB color to use for the right section of a progress bar */
#define	RGB_PROGRESS_RIGHT	sdl_rgb(255,255,255)

/** @brief width in character cells for SUI_SELBUTTON */
#define	SEL_WIDTH	3

/** @brief width in character cells for SUI_CHKBUTTON */
#define	CHK_WIDTH	2

/** @brief width in character cells for SUI_RANGEDEC and SUI_RANGEDEC2 */
#define	DEC_WIDTH	3

/** @brief width in character cells for SUI_RANGEINC and SUI_RANGEINC2 */
#define	INC_WIDTH	3

/** @brief which extra[] to use for SUI_SELBUTTON */
#define	EXTRA_SEL	0

/** @brief which extra[] to use for SUI_CHKBUTTON */
#define	EXTRA_CHK	0

/** @brief which extra[] to use for SUI_RANGEDEC2 */
#define	EXTRA_DEC2	0

/** @brief which extra[] to use for SUI_RANGEDEC */
#define	EXTRA_DEC	1

/** @brief which extra[] to use for SUI_RANGEVAL */
#define	VAL_EXTRA	2

/** @brief which extra[] to use for SUI_RANGEINC */
#define	EXTRA_INC	3

/** @brief which extra[] to use for SUI_RANGEINC2 */
#define	EXTRA_INC2	4

/** @brief which extra[] to use for SUI_HS_LEFT */
#define	EXTRA_HSL	0

/** @brief which extra[] to use for SUI_HS_SLIDER */
#define	EXTRA_HSS	1

/** @brief which extra[] to use for SUI_HS_RIGHT */
#define	EXTRA_HSR	2

/** @brief which extra[] to use for SUI_VS_UP */
#define	EXTRA_VSU	0

/** @brief which extra[] to use for SUI_VS_SLIDER */
#define	EXTRA_VSS	1

/** @brief which extra[] to use for SUI_VS_DOWN */
#define	EXTRA_VSD	2

/** @brief head of user interface elements */
static sui_t *sui;

/** @brief last hit of sui_find_id() */
static sui_t *last_hit;

/** @brief current simple user interface element id */
static int sid;

static int compare_tree(const void *p1, const void *p2);
static sui_t *sui_find_id(int id);

/**
 * @brief draw a simple user interface element
 *
 * @param s pointer to simple user interface element
 * @param prc rectangle that shall be set (may be NULL)
 */
int sui_draw(sui_t *s, rect_t *prc)
{
	FUN("sui_draw");
	uint32_t rgb0 = (s->flag & SUI_CLICKED) ? RGB_3D_BOTTOMRIGHT : RGB_3D_TOPLEFT;
	uint32_t rgb1 = (s->flag & SUI_CLICKED) ? RGB_3D_TOPLEFT : RGB_3D_BOTTOMRIGHT;

	/* don't draw invisible elements */
	if (!(s->flag & SUI_VISIBLE))
		return 0;

	switch (s->type) {
	case SUI_TEXT:
		if (NULL != s->text) {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%s", s->text);
		} else {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "NO TEXT");
		}
		break;
	case SUI_BUTTON:
		if (s->text) {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col | TEXT_BUTTON,
				"%s", s->text);
		} else {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col | TEXT_BUTTON,
				" ");
		}
		sdl_frame_rect(s->sdl, s->pane, &s->rc, -2, rgb0, rgb1);
		break;
	case SUI_SELTEXT:
		if (NULL != s->text) {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%s", s->text);
		}
		break;
	case SUI_SELBUTTON:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%*s", SEL_WIDTH, "");
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_SELECT, s->flag & SUI_SELECTED,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;

	case SUI_CHKTEXT:
		if (NULL != s->text) {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%s", s->text);
		}
		break;
	case SUI_CHKBUTTON:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%*s", CHK_WIDTH, "");
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_CHECKBOX, s->flag & SUI_CHECKED,
			RGB_3D_TOPLEFT, RGB_3D_BOTTOMRIGHT, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;

	case SUI_RANGESEL:
	case SUI_RANGESEL2:
		if (s->text) {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%s", s->text);
		}
		break;
	case SUI_RANGEDEC2:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%*s", DEC_WIDTH, "");
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_DEC2, 0,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	case SUI_RANGEDEC:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%*s", DEC_WIDTH, "");
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_DEC, 0,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	case SUI_RANGEVAL:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%6d", s->curval);
		sdl_frame_rect(s->sdl, s->pane, &s->rc, 1, RGB_3D_BOTTOMRIGHT, RGB_3D_TOPLEFT);
		break;
	case SUI_RANGEINC:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%*s", INC_WIDTH, "");
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_INC, 0,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	case SUI_RANGEINC2:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%*s", INC_WIDTH, "");
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_INC2, 0,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;

	case SUI_PROGRESS:
		sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, " ");
		s->rc.w = s->sdl->pane[s->pane].tw - s->rc.x;
		s->rc.h = 1;
		sdl_progress_bar(s->sdl, s->pane, &s->rc,
			RGB_PROGRESS_LEFT, RGB_PROGRESS_RIGHT, RGB_3D_BOTTOMRIGHT,
			s->minval, s->curval, s->maxval);
		break;
	case SUI_GRAPH:
		if (NULL != s->text) {
			sdl_printf(s->sdl, s->pane, prc, s->rc.x, s->rc.y, s->col, "%s", s->text);
		}
		sdl_draw_graph(s->sdl, s->pane, &s->rc, (sdl_graph_t *)s->data);
		break;

	case SUI_HSCROLL:
		if (prc)
			*prc = s->rc;
		/* draw just its extra elements */
		break;
	case SUI_HS_LEFT:
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_LEFT, 1,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	case SUI_HS_SLIDER:
		sdl_draw_slider(s->sdl, s->pane, &s->rc, (sdl_slider_t *)s->data, 1,
			RGB_3D_TOPLEFT, RGB_3D_BOTTOMRIGHT, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	case SUI_HS_RIGHT:
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_RIGHT, 1,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;

	case SUI_VSCROLL:
		if (prc)
			*prc = s->rc;
		/* draw just its extra elements */
		break;
	case SUI_VS_UP:
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_UP, 1,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	case SUI_VS_SLIDER:
		sdl_draw_slider(s->sdl, s->pane, &s->rc, (sdl_slider_t *)s->data, 1,
			RGB_3D_TOPLEFT, RGB_3D_BOTTOMRIGHT, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	case SUI_VS_DOWN:
		sdl_draw_button(s->sdl, s->pane, &s->rc, SDL_BTYPE_DOWN, 1,
			rgb0, rgb1, RGB_BUTTON_FACE, RGB_BUTTON_TEXT);
		break;
	}
	if (s->flag & SUI_BORDER)
		sdl_frame_rect(s->sdl, s->pane, &s->rc, 1, sdl_rgb(0,0,0), sdl_rgb(0,0,0));
	if (s->flag & SUI_UNDERLINE)
		sdl_underline(s->sdl, s->pane, &s->rc, sdl_rgb(0,0,0));
	return 0;
}

/**
 * @brief redraw all elements of SDL context sdl
 *
 * @param sdl pointer to the SDL context to redraw
 */
int sui_redraw(sdl_t *sdl)
{
	FUN("sui_redraw");
	sui_t *s;
	/* scan the list of elements */
	for (s = sui; s; s = s->next) {
		if (s->sdl != sdl)
			continue;
		sdl_dirty_rect(s->sdl, s->pane, &s->rc);
		sui_draw(s, NULL);
	}

	return 0;
}

/**
 * @brief redraw all elements of SDL context sdl that intersect with element s1
 *
 * @param sdl pointer to the SDL context to redraw
 */
int sui_redraw_intersect(sui_t *s1)
{
	FUN("sui_redraw_intersect");
	sui_t *s;
	int x0, y0, x1, y1;

	/* edges of the element */
	x0 = s1->rc.x;
	y0 = s1->rc.y;
	x1 = x0 + s1->rc.w - 1;
	y1 = y0 + s1->rc.h - 1;

	/* scan the list of elements */
	for (s = sui; s; s = s->next) {
		/* different sdl context */
		if (s->sdl != s1->sdl)
			continue;
		/* different pane */
		if (s->pane != s1->pane)
			continue;
		/* above or below */
		if (y1 < s->rc.y || y0 >= s->rc.y + s->rc.h)
			continue;
		/* left or right */
		if (x1 < s->rc.x || x0 >= s->rc.x + s->rc.w)
			continue;
		sui_draw(s, NULL);
	}

	return 0;
}

/**
 * @brief add a simple user interface element
 *
 * @param sdl pointer to the SDL context that contains this element
 * @param x column where to place this element
 * @param y row where to place this element
 * @param col color of the element's text
 * @param type type of the element
 * @param fmt format string, followed by optional arguments
 * @result the element id
 */
int sui_add(sdl_t *sdl, sdl_pane_t pane, int x, int y, int col,
	sui_type_t type, sui_flag_t flag, const char *fmt, ...)
{
	FUN("sui_add");
	char buff[256];
	sui_t *s = xcalloc(1, sizeof(sui_t)), *ps;
	va_list ap;
	int len = 0;

	LOG(INFO,(_fun, "sdl:%p pane:%d x:%d y:%d col:%d type:%d flag:%d\n",
		sdl, pane, x, y, col, type, flag));

	/* don't use 0 as element id */
	if (++sid == INT_MAX)
		sid = 1;

	switch (type) {
	case SUI_SELTEXT:
		/* add select button to the SUI_SELTEXT element */
		s->extra[EXTRA_SEL] = sui_add(sdl, pane, x, y, 0,
			SUI_SELBUTTON, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		/* set the SUI_SELBUTTON data to the parent's id */
		if (++sid == INT_MAX)
			sid = 1;
		sui_set_parent(s->extra[EXTRA_SEL], sid);
		x += SEL_WIDTH + 1;
		break;
	case SUI_HSCROLL:
	case SUI_VSCROLL:
		/* the parent elements are never enabled */
		flag &= ~SUI_ENABLED;
		break;
	default:
		/* others are fine */
		break;
	}
	s->sdl = sdl;
	s->pane = pane;
	s->id = sid;
	s->rc.x = x;
	s->rc.y = y;
	s->col = col;
	s->type = type;
	s->flag = flag;
	s->minval = 0;
	s->curval = 0;
	s->maxval = 1;

	if (NULL != fmt) {
		va_start(ap, fmt);
		len = vsnprintf(buff, sizeof(buff), fmt, ap);
		va_end(ap);
		s->text = xmalloc(len + 1);
		memcpy(s->text, buff, len + 1);
	}
	sui_draw(s, &s->rc);

	if (sui == NULL) {
		/* first element */
		sui = s;
	} else {
		/* find last */
		for (ps = sui; ps->next; ps = ps->next)
			;
		/* become successor of the last element */
		ps->next = s;
	}

	switch (type) {
	case SUI_CHKTEXT:
		/* add check button to the SUI_CHKTEXT element */
		s->extra[EXTRA_CHK] = sui_add(sdl, pane, x + s->rc.w, y, 0,
			SUI_CHKBUTTON, (flag & (SUI_VISIBLE | SUI_CHECKED)) | SUI_ENABLED, NULL);
		/* set the SUI_CHKBUTTON data to the parent's id */
		sui_set_parent(s->extra[EXTRA_CHK], s->id);
		break;
	case SUI_RANGESEL:
		/* add two buttons and the value text to the SUI_RANGESEL element */
		s->extra[EXTRA_DEC] = sui_add(sdl, pane, x + s->rc.w, y, 0,
			SUI_RANGEDEC, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_DEC], s->id);

		s->extra[VAL_EXTRA] = sui_add(sdl, pane, x + s->rc.w + INC_WIDTH, y, 0,
			SUI_RANGEVAL, (flag & SUI_VISIBLE), "%6d");
		sui_set_parent(s->extra[VAL_EXTRA], s->id);

		s->extra[EXTRA_INC] = sui_add(sdl, pane, x + s->rc.w + INC_WIDTH + 6, y, 0,
			SUI_RANGEINC, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_INC], s->id);
		break;
	case SUI_RANGESEL2:
		/* add four buttons and the value text to the SUI_RANGESEL element */
		s->extra[EXTRA_DEC2] = sui_add(sdl, pane, x + s->rc.w, y, 0,
			SUI_RANGEDEC2, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_DEC2], s->id);

		s->extra[EXTRA_DEC] = sui_add(sdl, pane, x + s->rc.w + INC_WIDTH, y, 0,
			SUI_RANGEDEC, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_DEC], s->id);

		s->extra[VAL_EXTRA] = sui_add(sdl, pane, x + s->rc.w + 2 * INC_WIDTH, y, 0,
			SUI_RANGEVAL, (flag & SUI_VISIBLE), "%6d");
		sui_set_parent(s->extra[VAL_EXTRA], s->id);

		s->extra[EXTRA_INC] = sui_add(sdl, pane, x + s->rc.w + 2 * INC_WIDTH + 6, y, 0,
			SUI_RANGEINC, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_INC], s->id);

		s->extra[EXTRA_INC2] = sui_add(sdl, pane, x + s->rc.w + 3 * INC_WIDTH + 6, y, 0,
			SUI_RANGEINC2, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_INC2], s->id);
		break;
	case SUI_HSCROLL:
		/* add left, slider, and right buttons */
		s->extra[EXTRA_HSL] = sui_add(sdl, pane, x, y, 0,
			SUI_HS_LEFT, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_HSL], s->id);

		s->extra[EXTRA_HSS] = sui_add(sdl, pane, x + 2, y, 0,
			SUI_HS_SLIDER, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_HSS], s->id);

		s->extra[EXTRA_HSR] = sui_add(sdl, pane, x + s->rc.w - 2, y, 0,
			SUI_HS_RIGHT, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_HSR], s->id);
		break;

	case SUI_VSCROLL:
		/* add up, slider, and down buttons */
		s->extra[EXTRA_VSU] = sui_add(sdl, pane, x, y, 0,
			SUI_VS_UP, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_VSU], s->id);

		s->extra[EXTRA_VSS] = sui_add(sdl, pane, x, y + 1, 0,
			SUI_VS_SLIDER, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_VSS], s->id);

		s->extra[EXTRA_VSD] = sui_add(sdl, pane, x, y + s->rc.h - 1, 0,
			SUI_VS_DOWN, (flag & SUI_VISIBLE) | SUI_ENABLED, NULL);
		sui_set_parent(s->extra[EXTRA_VSD], s->id);
		break;

	default:
		/* others are fine */
		break;
	}
	return s->id;
}

/**
 * @brief delete a simple user interface element
 *
 * @param id identifier of the element
 * @result 0 on sucess, -1 on error
 */
int sui_del(int id)
{
	FUN("sui_del");
	sui_t *s, *ps;
	sdl_graph_t *g;

	for (s = sui, ps = NULL; s; ps = s, s = s->next)
		if (s->id == id)
			break;

	/* element not found? */
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}

	if (ps) {
		/* element has a predecessor */
		ps->next = s->next;
	} else {
		/* first or only element */
		sui = s->next;
	}

	if (s->text) {
		xfree(s->text);
		s->text = NULL;
	}

	if (s->flag & SUI_VISIBLE) {
		sdl_fill_rect(s->sdl, s->pane, &s->rc, sdl_rgb(255,255,255));
		sdl_dirty_rect(s->sdl, s->pane, &s->rc);
	}
	switch (s->type) {
	case SUI_SELTEXT:
		sui_del(s->extra[EXTRA_SEL]);
		break;
	case SUI_CHKTEXT:
		sui_del(s->extra[EXTRA_CHK]);
		break;
	case SUI_RANGESEL:
		sui_del(s->extra[EXTRA_DEC]);
		sui_del(s->extra[VAL_EXTRA]);
		sui_del(s->extra[EXTRA_INC]);
		break;
	case SUI_RANGESEL2:
		sui_del(s->extra[EXTRA_DEC2]);
		sui_del(s->extra[EXTRA_DEC]);
		sui_del(s->extra[VAL_EXTRA]);
		sui_del(s->extra[EXTRA_INC]);
		sui_del(s->extra[EXTRA_INC2]);
		break;
	case SUI_GRAPH:
		if (NULL == s->data)
			break;
		g = (sdl_graph_t *)s->data;
		if (g->value) {
			/* free the value array */
			xfree(g->value);
		}
		/* free the graph context */
		xfree(s->data);
		s->data = NULL;
		break;
	case SUI_HSCROLL:
		sui_del(s->extra[EXTRA_HSL]);
		sui_del(s->extra[EXTRA_HSS]);
		sui_del(s->extra[EXTRA_HSR]);
		break;
	case SUI_HS_SLIDER:
		if (NULL == s->data)
			break;
		/* free the slider data */
		xfree(s->data);
		s->data = NULL;
		break;
	case SUI_VSCROLL:
		sui_del(s->extra[EXTRA_VSU]);
		sui_del(s->extra[EXTRA_VSS]);
		sui_del(s->extra[EXTRA_VSD]);
		break;
	case SUI_VS_SLIDER:
		if (NULL == s->data)
			break;
		/* free the slider data */
		xfree(s->data);
		s->data = NULL;
		break;
	default:
		break;
	}
	memset(s->extra, 0, sizeof(s->extra));
	xfree(s);

	/* reset last_hit, since it matches this element */
	last_hit = NULL;
	return 0;
}

/**
 * @brief hit test x, y coordinates
 *
 * Scans the list of elements for one that contains the coordinates
 * and returns its identifier on success. If the coordinates are
 * outside of all elements, -1 is returned.
 *
 * @param pane text pane being checked (-1 to check all)
 * @param x x coordinate to check
 * @param y y coordinate to check
 * @param phit pointer to an int to receive the parent element hit, if an
 * @result the id of the element containing this coordinate, -1 if none
 */
int sui_hit(sdl_pane_t pane, int x, int y, int *phit)
{
	FUN("sui_hit");
	sui_t *s;

	*phit = -1;
	/* scan the list of elements */
	for (s = sui; s; s = s->next) {
		/* skip disabled entries */
		if (!(s->flag & SUI_ENABLED))
			continue;
		/* skip invisible */
		if (!(s->flag & SUI_VISIBLE))
			continue;
		/* skip entries on another pane */
		if (-1 != pane && pane != s->pane)
			continue;
		if (y < s->rc.y)
			continue;
		if (y >= s->rc.y + s->rc.h)
			continue;
		if (x < s->rc.x)
			continue;
		if (x >= s->rc.x + s->rc.w)
			continue;
		switch (s->type) {
		case SUI_TEXT:
		case SUI_BUTTON:
		case SUI_SELTEXT:
		case SUI_CHKTEXT:
		case SUI_RANGESEL:
		case SUI_RANGESEL2:
		case SUI_PROGRESS:
		case SUI_GRAPH:
		case SUI_HSCROLL:
		case SUI_VSCROLL:
			break;
		case SUI_SELBUTTON:
		case SUI_CHKBUTTON:
		case SUI_RANGEDEC2:
		case SUI_RANGEDEC:
		case SUI_RANGEVAL:
		case SUI_RANGEINC:
		case SUI_RANGEINC2:
		case SUI_HS_LEFT:
		case SUI_HS_SLIDER:
		case SUI_HS_RIGHT:
		case SUI_VS_UP:
		case SUI_VS_SLIDER:
		case SUI_VS_DOWN:
			*phit = s->parent;
			break;
		}
		return s->id;
	}
	/* hit nothing */
	return -1;
}

/**
 * @brief find a simple user interface element by its identifier
 *
 * @param id simple user interface element identifier
 * @result a pointer to the element on success, NULL on error
 */
static sui_t *sui_find_id(int id)
{
	FUN("sui_get_find");
	sui_t *s;

	if (last_hit && id == last_hit->id)
		return last_hit;

	/* scan the list of elements */
	for (s = sui; s; s = s->next) {
		if (s->id == id) {
			last_hit = s;
			return s;
		}
	}

	return NULL;
}

/**
 * @brief get a simple user interface element's user data
 *
 * @param id simple user interface element identifier
 */
void *sui_get_data(int id)
{
	FUN("sui_get_data");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return NULL;
	}
	return s->data;
}

/**
 * @brief change a simple user interface element's user data
 *
 * @param id simple user interface element identifier
 * @param data user data
 */
int sui_set_data(int id, void *data)
{
	FUN("sui_set_data");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s->data = data;
	return 0;
}

/**
 * @brief change a simple user interface element's parent element id
 *
 * @param id simple user interface element identifier
 * @param pid parent id
 * @result 0 on sucess, -1 on error
 */
int sui_set_parent(int id, int pid)
{
	FUN("sui_set_parent");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s->parent = pid;
	return 0;
}

/**
 * @brief set maximum number of data points for a graph
 *
 * @param id simple user interface element identifier (must be type SUI_GRAPH)
 * @param max maximum number of data points
 */
int sui_set_graph_max(int id, ssize_t max)
{
	FUN("sui_set_graph_max");
	sui_t *s = sui_find_id(id);
	sdl_graph_t *g;
	ssize_t i;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_GRAPH)
		return -1;

	g = s->data;
	if (!g) {
		/* allocate a new graph data context */
		g = xcalloc(1, sizeof(sdl_graph_t));
		s->data = (void *)g;
		g->col = s->col;
	}

	/* (re)allocate values */
	g->value = xrealloc(g->value, max * sizeof(double));

	/* set new values to minval */
	for (i = g->max; i < max; i++)
		g->value[i] = g->minval;
	g->max = max;

	/* redraw the graph */
	return sui_draw(s, NULL);
}

/**
 * @brief add a data point to a graph
 *
 * @param id simple user interface element identifier (must be type SUI_GRAPH)
 * @param idx index of the value to set
 * @param value value to set
 * @result 0 on sucess, -1 on err
 */
int sui_set_graph_data(int id, ssize_t idx, double value)
{
	FUN("sui_set_graph_data");
	sui_t *s = sui_find_id(id);
	sdl_graph_t *g;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_GRAPH)
		return -1;

	g = s->data;
	if (!g) {
		/* allocate a new graph data context */
		g = xcalloc(1, sizeof(sdl_graph_t));
		s->data = (void *)g;
		g->col = s->col;
	}

	if (g->max == 0) {
		LOG(ERROR,(_fun,"max is 0\n"));
		errno = EINVAL;
		return -1;
	}

	/* negative value means: use next entry */
	if (idx < 0)
		idx = g->num;

	/* clip to max */
	if (idx >= g->max)
		idx = g->max - 1;

	/* set data */
	g->value[idx] = value;

	/* count and wrap at max */
	g->num = idx + 1;
	if (g->num == g->max)
		g->num = 0;

	/* update min and max */
	if (value < g->minval)
		g->minval = value;
	if (value > g->maxval)
		g->maxval = value;

	/* redraw the graph */
	return sui_draw(s, NULL);
}

/**
 * @brief set horizontal scroll bar min, max, cur and range
 */
int sui_set_hscroll(int id, rect_t *prc, int64_t min, int64_t max, int64_t cur, int64_t range)
{
	FUN("sui_set_hscroll");
	sui_t *s = sui_find_id(id);
	sui_t *s1;
	sdl_slider_t *slider;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_HSCROLL) {
		LOG(ERROR,(_fun,"not a SUI_HSCROLL (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s1 = sui_find_id(s->extra[EXTRA_HSS]);
	if (!s1) {
		LOG(ERROR,(_fun,"invalid EXTRA_HSS id (%d)\n", s->extra[EXTRA_HSS]));
		errno = EINVAL;
		return -1;
	}

	slider = (sdl_slider_t *)s1->data;
	if (!slider) {
		/* allocate a new slider context */
		slider = (sdl_slider_t *)xcalloc(1, sizeof(sdl_slider_t));
		s1->data = (void *)slider;
		slider->type = 0;
	}
	if (0 == memcmp(prc, &s->rc, sizeof(s->rc)) &&
		slider->min == min && slider->max == max &&
		slider->cur == cur && slider->range == range)
		return 0;
	slider->min = min;
	slider->max = max;
	slider->cur = cur;
	slider->range = range;
	sui_set_rect(id, prc);
	return sui_draw(s1, NULL);
}

/**
 * @brief get horizontal scroll bar current value
 */
int64_t sui_get_hscroll_cur(int id)
{
	FUN("sui_get_hscroll_cur");
	sui_t *s = sui_find_id(id);
	sui_t *s1;
	sdl_slider_t *slider;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_HSCROLL) {
		LOG(ERROR,(_fun,"not a SUI_HSCROLL (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s1 = sui_find_id(s->extra[EXTRA_HSS]);
	if (!s1) {
		LOG(ERROR,(_fun,"invalid EXTRA_HSS id (%d)\n", s->extra[EXTRA_HSS]));
		errno = EINVAL;
		return -1;
	}

	slider = s1->data;
	if (!slider)
		return 0;
	return slider->cur;
}

/**
 * @brief add to horizontal scroll bar current value
 */
int sui_add_hscroll_cur(int id, int step)
{
	FUN("sui_get_hscroll_cur");
	sui_t *s = sui_find_id(id);
	sui_t *s1;
	sdl_slider_t *slider;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_HSCROLL) {
		LOG(ERROR,(_fun,"not a SUI_HSCROLL (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s1 = sui_find_id(s->extra[EXTRA_HSS]);
	if (!s1) {
		LOG(ERROR,(_fun,"invalid EXTRA_HSS id (%d)\n", s->extra[EXTRA_HSS]));
		errno = EINVAL;
		return -1;
	}

	slider = s1->data;
	if (!slider)
		return 0;
	slider->cur += step;
	if (slider->cur < slider->min)
		slider->cur = slider->min;
	if (slider->cur > slider->max - slider->range)
		slider->cur = slider->max - slider->range;
	return sui_draw(s1, NULL);
}

/**
 * @brief set vertical scroll bar min, max, cur and range
 */
int sui_set_vscroll(int id, rect_t *prc, int64_t min, int64_t max, int64_t cur, int64_t range)
{
	FUN("sui_set_vscroll");
	sui_t *s = sui_find_id(id);
	sui_t *s1;
	sdl_slider_t *slider;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_VSCROLL) {
		LOG(ERROR,(_fun,"not a SUI_VSCROLL (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s1 = sui_find_id(s->extra[EXTRA_VSS]);
	if (!s1) {
		LOG(ERROR,(_fun,"invalid EXTRA_VSS id (%d)\n", s->extra[EXTRA_VSS]));
		errno = EINVAL;
		return -1;
	}

	slider = (sdl_slider_t *)s1->data;
	if (!slider) {
		/* allocate a new slider context */
		slider = (sdl_slider_t *)xcalloc(1, sizeof(sdl_slider_t));
		s1->data = (void *)slider;
		slider->type = 1;
	}
	if (0 == memcmp(prc, &s->rc, sizeof(s->rc)) &&
		slider->min == min && slider->max == max &&
		slider->cur == cur && slider->range == range)
		return 0;
	slider->min = min;
	slider->max = max;
	slider->cur = cur;
	slider->range = range;
	sui_set_rect(id, prc);
	return sui_draw(s1, NULL);
}

/**
 * @brief get vertical scroll bar current value
 */
int64_t sui_get_vscroll_cur(int id)
{
	FUN("sui_get_vscroll_cur");
	sui_t *s = sui_find_id(id);
	sui_t *s1;
	sdl_slider_t *slider;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_VSCROLL) {
		LOG(ERROR,(_fun,"not a SUI_VSCROLL (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s1 = sui_find_id(s->extra[EXTRA_VSS]);
	if (!s1) {
		LOG(ERROR,(_fun,"invalid EXTRA_VSS id (%d)\n", s->extra[EXTRA_HSS]));
		errno = EINVAL;
		return -1;
	}

	slider = (sdl_slider_t *)s1->data;
	if (!slider)
		return 0;
	return slider->cur;
}

/**
 * @brief add to vertical scroll bar current value
 */
int sui_add_vscroll_cur(int id, int step)
{
	FUN("sui_get_vscroll_cur");
	sui_t *s = sui_find_id(id);
	sui_t *s1;
	sdl_slider_t *slider;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_VSCROLL) {
		LOG(ERROR,(_fun,"not a SUI_VSCROLL (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	s1 = sui_find_id(s->extra[EXTRA_VSS]);
	if (!s1) {
		LOG(ERROR,(_fun,"invalid EXTRA_VSS id (%d)\n", s->extra[EXTRA_HSS]));
		errno = EINVAL;
		return -1;
	}

	slider = (sdl_slider_t *)s1->data;
	if (!slider)
		return 0;
	slider->cur += step;
	if (slider->cur < slider->min)
		slider->cur = slider->min;
	if (slider->cur > slider->max - slider->range)
		slider->cur = slider->max - slider->range;
	return sui_draw(s1, NULL);
}

/**
 * @brief get the text of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @result a pointer to the current text (may be NULL), or NULL on error
 */
const char *sui_get_text(int id)
{
	FUN("sui_get_text");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return NULL;
	}
	return s->text;
}

/**
 * @brief set the text of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param fmt format string and optional arguments following it
 * @result 0 on success, or -1 on error
 */
int sui_set_text(int id, const char *fmt, ...)
{
	FUN("sui_set_text");
	char buff[256];
	va_list ap;
	int len;
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}

	va_start(ap, fmt);
	len = vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	/* unchanged text ? */
	if (s->text && strlen(s->text) == len && 0 == strcmp(s->text, buff))
		return sui_draw(s, NULL);

	xfree(s->text);
	s->text = xmalloc(len + 1);
	memcpy(s->text, buff, len + 1);

	return sui_draw(s, &s->rc);
}

/** @brief change a simple user interface element's range status */
int sui_set_range(int id, int min, int cur, int max)
{
	FUN("sui_set_range");
	sui_t *s;
	sui_t *s1;

	s = sui_find_id(id);
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_RANGESEL && s->type != SUI_RANGESEL2)
		return -1;
	/* no change? */
	if (s->minval == min && s->curval == cur && s->maxval == max)
		return 0;
	s->minval = min;
	s->curval = cur;
	s->maxval = max;

	/* update the SUI_RANGEVAL element */
	s1 = sui_find_id(s->extra[VAL_EXTRA]);
	if (!s1)
		return -1;
	s1->minval = min;
	s1->curval = cur;
	s1->maxval = max;
	return sui_draw(s1, &s1->rc);
}

/** @brief get a simple user interface element's range current value */
int sui_get_range_cur(int id)
{
	FUN("sui_get_range_cur");
	sui_t *s;

	s = sui_find_id(id);
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_RANGESEL && s->type != SUI_RANGESEL2)
		return -1;
	return s->curval;
}

/** @brief add to a simple user interface element's range current value */
int sui_add_range_cur(int id, int add)
{
	FUN("sui_add_range_cur");
	sui_t *s;
	sui_t *s1;
	int cur;

	s = sui_find_id(id);
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_RANGESEL && s->type != SUI_RANGESEL2)
		return -1;
	/* no change? */
	cur = s->curval + add;
	cur = MAX(cur, s->minval);
	cur = MIN(cur, s->maxval);
	s->curval = cur;

	/* update the SUI_RANGEVAL element */
	s1 = sui_find_id(s->extra[VAL_EXTRA]);
	if (!s1)
		return -1;
	s1->curval = cur;
	return sui_draw(s1, &s1->rc);
}

/** @brief change a simple user interface element's progress status */
int sui_set_progress(int id, int min, int cur, int max)
{
	FUN("sui_set_progress");
	sui_t *s;

	s = sui_find_id(id);
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->type != SUI_PROGRESS)
		return -1;
	/* no change? */
	if (s->minval == min && s->curval == cur && s->maxval == max)
		return 0;
	s->minval = min;
	s->curval = cur;
	s->maxval = max;
	return sui_draw(s, &s->rc);
}

/**
 * @brief get a simple user interface element's status flag
 *
 * @param id identifier of the element
 * @result the type on success, or -1 on error
 */
sui_type_t sui_get_type(int id)
{
	FUN("sui_get_type");
	sui_t *s = sui_find_id(id);
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	return s->type;
}

/**
 * @brief get a simple user interface element's status flag
 *
 * @param id identifier of the element
 * @result the flag on success, or -1 on error
 */
sui_flag_t sui_get_flag(int id)
{
	FUN("sui_get_flag");
	sui_t *s = sui_find_id(id);
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	return s->flag;
}

/**
 * @brief change a simple user interface element's status flag
 *
 * @param id identifier of the element
 * @param flag new flag value to set
 * @result 0 on sucess, -1 on error
 */
int sui_set_flag(int id, sui_flag_t flag)
{
	FUN("sui_set_flag");
	sui_t *s = sui_find_id(id);
	int change;
	int res;
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (flag == s->flag)
		return 0;

	change = flag ^ s->flag;
	s->flag = flag;
	res = sui_draw(s, NULL);
	if (change & SUI_CLICKED) {
		if (flag & SUI_CLICKED) {
			switch (s->type) {
			case SUI_RANGEDEC2:
				sui_add_range_cur(s->parent, -10);
				break;
			case SUI_RANGEDEC:
				sui_add_range_cur(s->parent, -1);
				break;
			case SUI_RANGEINC:
				sui_add_range_cur(s->parent, +1);
				break;
			case SUI_RANGEINC2:
				sui_add_range_cur(s->parent, +10);
				break;
			case SUI_HS_LEFT:
				sui_add_hscroll_cur(s->parent, -1);
				break;
			case SUI_HS_RIGHT:
				sui_add_hscroll_cur(s->parent, +1);
				break;
			case SUI_VS_UP:
				sui_add_vscroll_cur(s->parent, -1);
				break;
			case SUI_VS_DOWN:
				sui_add_vscroll_cur(s->parent, +1);
				break;
			default:
				break;
			}
		}
	}
	if (change & SUI_ENABLED) {
		/* toggling enabled state */
		int on = (flag >> SUI_ENABLED_BIT) & 1;
		switch (s->type) {
		case SUI_SELTEXT:
			/* XXX: keep the sel button enabled */
			/* sui_set_enable(s->extra[EXTRA_SEL], on); */
			break;
		case SUI_CHKTEXT:
			sui_set_enable(s->extra[EXTRA_CHK], on);
			break;
		case SUI_RANGESEL:
			sui_set_enable(s->extra[EXTRA_DEC], on);
			sui_set_enable(s->extra[VAL_EXTRA], on);
			sui_set_enable(s->extra[EXTRA_INC], on);
			break;
		case SUI_RANGESEL2:
			sui_set_enable(s->extra[EXTRA_DEC2], on);
			sui_set_enable(s->extra[EXTRA_DEC], on);
			sui_set_enable(s->extra[VAL_EXTRA], on);
			sui_set_enable(s->extra[EXTRA_INC], on);
			sui_set_enable(s->extra[EXTRA_INC2], on);
			break;
		case SUI_HSCROLL:
			s->flag &= ~SUI_ENABLED;
			sui_set_enable(s->extra[EXTRA_HSL], on);
			sui_set_enable(s->extra[EXTRA_HSS], on);
			sui_set_enable(s->extra[EXTRA_HSR], on);
			break;
		case SUI_VSCROLL:
			s->flag &= ~SUI_ENABLED;
			sui_set_enable(s->extra[EXTRA_VSU], on);
			sui_set_enable(s->extra[EXTRA_VSS], on);
			sui_set_enable(s->extra[EXTRA_VSD], on);
			break;
		default:
			/* others are fine */
			break;
		}
	}
	if (change & SUI_VISIBLE) {
		/* toggling visibility */
		int on = (flag >> SUI_VISIBLE_BIT) & 1;
		if (!on) {
			sdl_dirty_rect(s->sdl, s->pane, &s->rc);
			sdl_fill_rect(s->sdl, s->pane, &s->rc, sdl_rgb(255,255,255));
			sui_redraw_intersect(s);
		}
		switch (s->type) {
		case SUI_SELTEXT:
			sui_set_visible(s->extra[EXTRA_SEL], on);
			break;
		case SUI_CHKTEXT:
			sui_set_visible(s->extra[EXTRA_CHK], on);
			break;
		case SUI_RANGESEL:
			sui_set_visible(s->extra[EXTRA_DEC], on);
			sui_set_visible(s->extra[VAL_EXTRA], on);
			sui_set_visible(s->extra[EXTRA_INC], on);
			break;
		case SUI_RANGESEL2:
			sui_set_visible(s->extra[EXTRA_DEC2], on);
			sui_set_visible(s->extra[EXTRA_DEC], on);
			sui_set_visible(s->extra[VAL_EXTRA], on);
			sui_set_visible(s->extra[EXTRA_INC], on);
			sui_set_visible(s->extra[EXTRA_INC2], on);
			break;
		case SUI_HSCROLL:
			sui_set_visible(s->extra[EXTRA_HSL], on);
			sui_set_visible(s->extra[EXTRA_HSS], on);
			sui_set_visible(s->extra[EXTRA_HSR], on);
			break;
		case SUI_VSCROLL:
			sui_set_visible(s->extra[EXTRA_VSU], on);
			sui_set_visible(s->extra[EXTRA_VSS], on);
			sui_set_visible(s->extra[EXTRA_VSD], on);
			break;
		default:
			/* others are fine */
			break;
		}
	}
	if (change & SUI_SELECTED) {
		/* toggling selection status */
		int on = (flag >> SUI_SELECTED_BIT) & 1;
		switch (s->type) {
		case SUI_SELTEXT:
			sui_set_selected(s->extra[EXTRA_SEL], on);
			break;
		default:
			/* others are fine */
			break;
		}
	}
	if (change & SUI_CHECKED) {
		/* toggling checked status */
		int on = (flag >> SUI_CHECKED_BIT) & 1;
		switch (s->type) {
		case SUI_CHKTEXT:
			sui_set_checked(s->extra[EXTRA_CHK], on);
			break;
		default:
			/* others are fine */
			break;
		}
	}
	return res;
}


/**
 * @brief toggle a simple user interface element's status flag
 *
 * @param id identifier of the element
 * @param toggle flag bits to toggle
 * @result 0 on sucess, -1 on error
 */
int sui_tog_flag(int id, int toggle)
{
	FUN("sui_set_flag");
	sui_t *s = sui_find_id(id);
	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (0 == toggle)
		return 0;
	return sui_set_flag(id, s->flag ^ toggle);
}

/**
 * @brief set the visibilty of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param on non-zero if visible, zero if invisible
 * @result 0 on sucess, -1 on error
 */
int sui_set_visible(int id, int on)
{
	FUN("sui_set_visible");
	sui_t *s = sui_find_id(id);
	int flag;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	flag = s->flag;
	if (on == (SUI_VISIBLE == (flag & SUI_VISIBLE)))
		return 0;
	if (on)
		flag |= SUI_VISIBLE;
	else
		flag &= ~SUI_VISIBLE;
	return sui_set_flag(id, flag);
}

/**
 * @brief get the selected status of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param on non-zero if selected, zero if not selected
 * @result 1 if selected, 0 if not selected, -1 on error
 */
int sui_get_selected(int id)
{
	FUN("sui_get_selected");
	sui_t *s = sui_find_id(id);
	sui_t *s1;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	/* find the SUI_SELBUTTON */
	s1 = sui_find_id(s->extra[EXTRA_SEL]);
	if (!s1)
		return -1;
	return (s1->flag & SUI_SELECTED) ? 1 : 0;
}

/**
 * @brief set the selected status of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param on non-zero if selected, zero if not selected
 * @result 0 on sucess, -1 on error
 */
int sui_set_selected(int id, int on)
{
	FUN("sui_set_selected");
	sui_t *s = sui_find_id(id);
	int flag;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	flag = s->flag;
	if (on == (SUI_SELECTED == (flag & SUI_SELECTED)))
		return 0;
	if (on)
		flag |= SUI_SELECTED;
	else
		flag &= ~SUI_SELECTED;
	return sui_set_flag(id, flag);
}

/**
 * @brief get the checked status of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param on non-zero if checked, zero if not checked
 * @result 1 if checked, 0 if not checked, -1 on error
 */
int sui_get_checked(int id)
{
	FUN("sui_get_checked");
	sui_t *s = sui_find_id(id);
	sui_t *s1;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	/* find the SUI_SELBUTTON */
	s1 = sui_find_id(s->extra[EXTRA_CHK]);
	if (!s1)
		return -1;
	return (s1->flag & SUI_CHECKED) ? 1 : 0;
}

/**
 * @brief set the checked status of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param on non-zero if checked, zero if not checked
 * @result 0 on sucess, -1 on error
 */
int sui_set_checked(int id, int on)
{
	FUN("sui_set_checked");
	sui_t *s = sui_find_id(id);
	int flag;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	flag = s->flag;
	if (on == (SUI_CHECKED == (flag & SUI_CHECKED)))
		return 0;
	if (on)
		flag |= SUI_CHECKED;
	else
		flag &= ~SUI_CHECKED;
	return sui_set_flag(id, flag);
}

/**
 * @brief set the enabled status of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param on non-zero if selected, zero if not selected
 * @result 0 on sucess, -1 on error
 */
int sui_set_enable(int id, int on)
{
	FUN("sui_set_enable");
	sui_t *s = sui_find_id(id);
	int flag;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	flag = s->flag;
	if (on == (SUI_ENABLED == (flag & SUI_ENABLED)))
		return 0;
	if (on)
		flag |= SUI_ENABLED;
	else
		flag &= ~SUI_ENABLED;
	return sui_set_flag(id, flag);
}

/**
 * @brief get a simple user interface element's text color
 * @param id simple user interface element identifier
 * @result the color on success, -1 on error
 */
int sui_get_col(int id)
{
	FUN("sui_get_col");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	return s->col;
}

/**
 * @brief change a simple user interface element's text color
 * @param id simple user interface element identifier
 * @param col text color (see sdl.h for the TEXT_... color enumeration)
 * @result 0 on sucess, -1 on error
 */
int sui_set_col(int id, int col)
{
	FUN("sui_set_col");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (col == s->col)
		return 0;
	s->col = col;
	return sui_draw(s, NULL);
}

/**
 * @brief get the rectangle of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @param prc pointer to a rectangle to receive the element's rectangle
 * @result 0 on sucess, -1 on error
 */
int sui_get_rect(int id, rect_t *prc)
{
	FUN("sui_get_rect");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	*prc = s->rc;
	return 0;
}

/**
 * @brief set the rectangle of a simple user element by id
 *
 * @param id simple user interface element identifier
 * @result 0 on sucess, -1 on error
 */
int sui_set_rect(int id, const rect_t *prc)
{
	FUN("sui_set_rect");
	sui_t *s = sui_find_id(id);
	int flag;
	rect_t rc;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}

	/* nothing changed? */
	if (0 == memcmp(&s->rc, prc, sizeof(s->rc)))
		return 0;
	flag = s->flag;
	if (flag & SUI_VISIBLE)
		sui_set_visible(id, 0);
	s->rc = *prc;
	switch (s->type) {
	case SUI_SELTEXT:
		rc = s->rc;
		rc.x = rc.x - SEL_WIDTH - 1;
		rc.w = SEL_WIDTH;
		sui_set_rect(s->extra[EXTRA_SEL], &rc);
		break;
	case SUI_CHKTEXT:
		rc = s->rc;
		rc.x = rc.x + rc.w;
		rc.w = CHK_WIDTH;
		sui_set_rect(s->extra[EXTRA_CHK], &rc);
		break;
	case SUI_RANGESEL:
		rc = s->rc;
		rc.x = rc.x + rc.w;
		rc.w = DEC_WIDTH;
		sui_set_rect(s->extra[EXTRA_DEC], &rc);
		rc.x = rc.x + rc.w;
		rc.w = 6;
		sui_set_rect(s->extra[VAL_EXTRA], &rc);
		rc.x = rc.x + rc.w;
		rc.w = INC_WIDTH;
		sui_set_rect(s->extra[EXTRA_INC], &rc);
		break;
	case SUI_RANGESEL2:
		rc = s->rc;
		rc.x = rc.x + rc.w;
		rc.w = DEC_WIDTH;
		sui_set_rect(s->extra[EXTRA_DEC2], &rc);
		rc.x = rc.x + rc.w;
		rc.w = DEC_WIDTH;
		sui_set_rect(s->extra[EXTRA_DEC], &rc);
		rc.x = rc.x + rc.w;
		rc.w = 6;
		sui_set_rect(s->extra[VAL_EXTRA], &rc);
		rc.x = rc.x + rc.w;
		rc.w = INC_WIDTH;
		sui_set_rect(s->extra[EXTRA_INC], &rc);
		rc.x = rc.x + rc.w;
		rc.w = INC_WIDTH;
		sui_set_rect(s->extra[EXTRA_INC2], &rc);
		break;
	case SUI_HSCROLL:
		rc = s->rc;
		rc.w = 2;
		sui_set_rect(s->extra[EXTRA_HSL], &rc);
		rc.x = rc.x + rc.w;
		rc.w = s->rc.w - 2 - 2;
		sui_set_rect(s->extra[EXTRA_HSS], &rc);
		rc.x = rc.x + rc.w;
		rc.w = 2;
		sui_set_rect(s->extra[EXTRA_HSR], &rc);
		break;
	case SUI_VSCROLL:
		rc = s->rc;
		rc.h = 1;
		sui_set_rect(s->extra[EXTRA_VSU], &rc);
		rc.y = rc.y + rc.h;
		rc.h = s->rc.h - 1 - 1;
		sui_set_rect(s->extra[EXTRA_VSS], &rc);
		rc.y = rc.y + rc.h;
		rc.h = 1;
		sui_set_rect(s->extra[EXTRA_VSD], &rc);
		break;
	default:
		break;
	}
	if (flag & SUI_VISIBLE)
		sui_set_visible(id, 1);
	s->flag = flag;
	return sui_draw(s, NULL);
}

/**
 * @brief get x coordinate of an element by id
 *
 * @param id simple user interface element identifier
 * @result x coordinate of element, or -1 on error
 */
int sui_get_x(int id)
{
	FUN("sui_get_x");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	return s->rc.x;
}

/**
 * @brief get y coordinate of an element by id
 *
 * @param id simple user interface element identifier
 * @result y coordinate of element, or -1 on error
 */
int sui_get_y(int id)
{
	FUN("sui_get_y");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	return s->rc.y;
}

/**
 * @brief get the (visible) width of an element by id
 *
 * @param id simple user interface element identifier
 * @result width, if the element is visible, 0 otherwise, -1 on error
 */
int sui_get_w(int id)
{
	FUN("sui_get_w");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->flag & SUI_VISIBLE)
		return s->rc.w;
	return 0;
}

/**
 * @brief get the (visible) height of an element by id
 *
 * @param id simple user interface element identifier
 * @result height, if the element is visible, 0 otherwise, -1 on error
 */
int sui_get_h(int id)
{
	FUN("sui_get_h");
	sui_t *s = sui_find_id(id);

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (s->flag & SUI_VISIBLE)
		return s->rc.h;
	return 0;
}

/**
 * @brief set x coordinate of an element by id
 *
 * @param id simple user interface element identifier
 * @param x new x coordinate to set
 * @result 0 on sucess, -1 on error
 */
int sui_set_x(int id, int x)
{
	FUN("sui_set_x");
	sui_t *s = sui_find_id(id);
	rect_t rc;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (x == s->rc.x)
		return 0;
	rc = s->rc;
	rc.x = x;
	return sui_set_rect(id, &rc);
}

/**
 * @brief set y coordinate of an element by id
 *
 * @param id simple user interface element identifier
 * @param y new y coordinate to set
 * @result 0 on sucess, -1 on error
 */
int sui_set_y(int id, int y)
{
	FUN("sui_set_y");
	sui_t *s = sui_find_id(id);
	rect_t rc;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (y == s->rc.y)
		return 0;
	rc = s->rc;
	rc.y = y;
	return sui_set_rect(id, &rc);
}

/**
 * @brief set the width of an element by id
 *
 * @param id simple user interface element identifier
 * @param w new width to set
 * @result 0 on sucess, -1 on error
 */
int sui_set_w(int id, int w)
{
	FUN("sui_set_w");
	sui_t *s = sui_find_id(id);
	rect_t rc;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (w == s->rc.w)
		return 0;
	rc = s->rc;
	rc.w = w;
	return sui_set_rect(id, &rc);
}

/**
 * @brief set the height of an element by id
 *
 * @param id simple user interface element identifier
 * @param h new height to set
 * @result 0 on sucess, -1 on error
 */
int sui_set_h(int id, int h)
{
	FUN("sui_set_h");
	sui_t *s = sui_find_id(id);
	rect_t rc;

	if (!s) {
		LOG(ERROR,(_fun,"invalid id (%d)\n", id));
		errno = EINVAL;
		return -1;
	}
	if (h == s->rc.h)
		return 0;
	rc = s->rc;
	rc.h = h;
	return sui_set_rect(id, &rc);
}

/**
 * @brief browser to select a path or filename
 *
 * @param sdl pointer to SDL context to fill
 * @param x x coordinate of output
 * @param y first y coordinate of output (is incremented for every entry)
 * @param path path name
 * @param pid pointer to an int pointer to receive the array of element ids
 * @result the number of elements filled, or -1 on error
 */
int sui_dirtree(sdl_t *sdl, sdl_pane_t pane, int x, int y, dirtree_flag_t flag, char *path, int **pid)
{
	FUN("sui_dirtree");
	char format[256];
	tree_t tree;
	int *id;
	int nid;
	int res;
	int num;
	int max;
	int len;
	int i;


#define	NAME_W	24
#define	SIZE_W	14
#define	CTIME_W	19
#define	MTIME_W	19
#define	ATIME_W	19
#define	MODE_W	10
#define	UID_W	5
#define	GID_W	5
#define	NLINK_W	5

	/* read the directory tree */
	memset(&tree, 0, sizeof(tree));
	res = dirtree(flag, path, 0, NULL, &tree);

	if (res < 0) {
		LOG(ERROR,(_fun,"dirtree(%d,%p,%d,%p,%p) failed (%s)\n",
			flag, path, 0, NULL, &tree, strerror(errno)));
		return -1;
	}

	/* sort the directory tree */
	qsort(tree.entry, tree.num, sizeof(tree_entry_t *), compare_tree);

	id = xcalloc(tree.num + 2, sizeof(int));
	for (i = 0, max = 0; i < tree.num; i++)
		if (tree.entry[i]->nlen - tree.entry[i]->plen > max)
			max = tree.entry[i]->nlen - tree.entry[i]->plen;
	max = MAX(max, NAME_W);

	len = snprintf(format, sizeof(format), " %-*s", max, "Name");
	if (flag & DIRTREE_SIZE)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", SIZE_W, "Size");
	if (flag & DIRTREE_CTIME)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", CTIME_W, "Created");
	if (flag & DIRTREE_MTIME)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", MTIME_W, "Modified");
	if (flag & DIRTREE_ATIME)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", ATIME_W, "Accessed");
	if (flag & DIRTREE_MODE)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", MODE_W, "Mode");
	if (flag & DIRTREE_UID)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", UID_W, "User");
	if (flag & DIRTREE_GID)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", GID_W, "Group");
	if (flag & DIRTREE_NLINK)
		len += snprintf(format + len, sizeof(format) - len, " %-*s", NLINK_W, "Links");
	len += snprintf(format + len, sizeof(format) - len, " \037");

	num = 0;
	nid = sui_add(sdl, pane, x, y, TEXT_BLUE | TEXT_BUTTON, SUI_TEXT, SUI_VISIBLE, format);
	y += sui_get_h(nid);
	id[num] = nid;
	num++;

	for (i = 0; i < tree.num; i++) {
		tree_entry_t *t = tree.entry[i];
		if (!strcmp(t->name + t->plen, "."))
			continue;
		len = snprintf(format, sizeof(format), " %-*s", max, t->name + t->plen);
		if (flag & DIRTREE_SIZE)
			len += snprintf(format + len, sizeof(format) - len, " %s",
				bignum_str(t->size, SIZE_W));
		if (flag & DIRTREE_CTIME)
			len += snprintf(format + len, sizeof(format) - len, " %s",
				filetime_str(t->ctime, CTIME_W));
		if (flag & DIRTREE_MTIME)
			len += snprintf(format + len, sizeof(format) - len, " %s",
				filetime_str(t->mtime, MTIME_W));
		if (flag & DIRTREE_ATIME)
			len += snprintf(format + len, sizeof(format) - len, " %s",
				filetime_str(t->atime, ATIME_W));
		if (flag & DIRTREE_MODE)
			len += snprintf(format + len, sizeof(format) - len, " %s",
				filemode_str(t->mode));
		if (flag & DIRTREE_UID)
			len += snprintf(format + len, sizeof(format) - len, " %-*d",
				UID_W, t->uid);
		if (flag & DIRTREE_GID)
			len += snprintf(format + len, sizeof(format) - len, " %-*d",
				GID_W, t->gid);
		if (flag & DIRTREE_NLINK)
			len += snprintf(format + len, sizeof(format) - len, " %-*d",
				NLINK_W, t->nlink);
		len += snprintf(format + len, sizeof(format) - len, " \037");
		nid = sui_add(sdl, pane, x + t->depth * 4, y, 0,
			SUI_TEXT, SUI_VISIBLE | SUI_ENABLED, "%s", format);

		y += sui_get_h(nid);
		sui_set_data(nid, t);
		id[num] = nid;
		num++;
	}
	*pid = id;
	return num;

#undef	NAME_W
#undef	SIZE_W
#undef	CTIME_W
#undef	MTIME_W
#undef	ATIME_W
#undef	MODE_W
#undef	UID_W
#undef	GID_W
#undef	NLINK_W
}

static int compare_tree(const void *p1, const void *p2)
{
	const tree_entry_t *t1 = *(const tree_entry_t **)p1;
	const tree_entry_t *t2 = *(const tree_entry_t **)p2;
	return strcmp(t1->name, t2->name);
}
