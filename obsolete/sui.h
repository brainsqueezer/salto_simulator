/************************************************************************
 *
 * Simple User Interface
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
 ************************************************************************/
#if !defined(_sui_h_included_)
#define	_sui_h_included_

#include "config.h"
#include "sdl.h"

#if	defined(RENDER_TO_SDL) && RENDER_TO_SDL

/** @brief simple user interface element types */
typedef enum {
	/** @brief simple text */
	SUI_TEXT,
	/** @brief button */
	SUI_BUTTON,
	/** @brief text with a select box */
	SUI_SELTEXT,
	/** @brief select button of a SUI_SELTEXT */
	SUI_SELBUTTON,
	/** @brief text with a check button */
	SUI_CHKTEXT,
	/** @brief check button of a SUI_CHKTEXT */
	SUI_CHKBUTTON,
	/** @brief number range select with dec and inc */
	SUI_RANGESEL,
	/** @brief number range select with double-dec, dec, inc, and double-inc */
	SUI_RANGESEL2,
	/** @brief double-minus button of a SUI_RANGESEL */
	SUI_RANGEDEC2,
	/** @brief minus button of a SUI_RANGESEL */
	SUI_RANGEDEC,
	/** @brief value text of a SUI_RANGESEL  */
	SUI_RANGEVAL,
	/** @brief plus button of a SUI_RANGESEL */
	SUI_RANGEINC,
	/** @brief double-plus button of a SUI_RANGESEL */
	SUI_RANGEINC2,
	/** @brief progress bar */
	SUI_PROGRESS,
	/** @brief a graph to displaying an array of values */
	SUI_GRAPH,
	/** @brief a horizontal scroll bar */
	SUI_HSCROLL,
	/** @brief left button of a horizontal scroll bar */
	SUI_HS_LEFT,
	/** @brief slider of a horizontal scroll bar */
	SUI_HS_SLIDER,
	/** @brief right button of a horizontal scroll bar */
	SUI_HS_RIGHT,
	/** @brief a vertical scroll bar */
	SUI_VSCROLL,
	/** @brief up button of a vertical scroll bar */
	SUI_VS_UP,
	/** @brief slider of a vertical scroll bar */
	SUI_VS_SLIDER,
	/** @brief down button of a vertical scroll bar */
	SUI_VS_DOWN
}	sui_type_t;

/** @brief simple user interface element flags */
typedef enum {
	/** @brief element is visible */
	SUI_VISIBLE_BIT,
	/** @brief element is enabled, i.e. reported in hit test */
	SUI_ENABLED_BIT,
	/** @brief element is clicked */
	SUI_CLICKED_BIT,
	/** @brief element is selected */
	SUI_SELECTED_BIT,
	/** @brief element is checked */
	SUI_CHECKED_BIT,
	/** @brief draw a black border around element */
	SUI_BORDER_BIT,
	/** @brief underline element */
	SUI_UNDERLINE_BIT,
	/* use bit numbers to build the bit masks */
	SUI_VISIBLE	= (1 << SUI_VISIBLE_BIT),
	SUI_ENABLED	= (1 << SUI_ENABLED_BIT),
	SUI_CLICKED	= (1 << SUI_CLICKED_BIT),
	SUI_SELECTED	= (1 << SUI_SELECTED_BIT),
	SUI_CHECKED	= (1 << SUI_CHECKED_BIT),
	SUI_BORDER	= (1 << SUI_BORDER_BIT),
	SUI_UNDERLINE	= (1 << SUI_UNDERLINE_BIT)
}	sui_flag_t;

/** @brief simple user interface element */
typedef struct sui_s {

	/** @brief next element */
	struct sui_s *next;

	/** @brief element identifier */
	int id;

	/** @brief pointer to SDL context containing this element */
	sdl_t *sdl;

	/** @brief text pane number */
	sdl_pane_t pane;

	/** @brief text color (initial) */
	int col;

	/** @brief rectangle that acts on mouse input */
	rect_t rc;

	/** @brief interface element type */
	sui_type_t type;

	/** @brief status flag */
	sui_flag_t flag;

	/** @brief text to show, if any */
	char *text;

	/** @brief progress minimum value */
	int minval;

	/** @brief progress current value */
	int curval;

	/** @brief progress maximum value */
	int maxval;

	/** @brief user data associated with the element */
	void *data;

	/** @brief parent element id, if any */
	int parent;

	/** @brief extra ids of related elements */
	int extra[5];
}	sui_t;
	
#ifdef	__cplusplus
extern "C" {
#endif

/** @brief redraw all elements of SDL context sdl */
extern int sui_redraw(sdl_t *sdl);
	
/** @brief add a simple user interface element */
extern int sui_add(sdl_t *sdl, sdl_pane_t pane, int x, int y, int col,
	sui_type_t type, sui_flag_t flag, const char *fmt, ...);
	
/** @brief delete a simple user interface element */
extern int sui_del(int id);

/** @brief hit test x, y coordinates, returning element id that is hit or -1 */
extern int sui_hit(sdl_pane_t pane, int x, int y, int *phit);

/** @brief get a simple user interface element's user data */
extern void *sui_get_data(int id);

/** @brief change a simple user interface element's user data */
extern int sui_set_data(int id, void *data);

/** @brief change a simple user interface element's parent element id */
extern int sui_set_parent(int id, int pid);

/** @brief set maximum number of data points for a graph */
extern int sui_set_graph_max(int id, ssize_t max);

/** @brief add a data point to a graph */
extern int sui_set_graph_data(int id, ssize_t idx, double value);

/** @brief set horizontal scroll bar rectangle, min, max, cur and range */
extern int sui_set_hscroll(int id, rect_t *prc, int64_t min, int64_t max, int64_t cur, int64_t range);

/** @brief get horizontal scroll bar current value */
extern int64_t sui_get_hscroll_cur(int id);

/** @brief set vertical scroll bar rectangle, min, max, cur and range */
extern int sui_set_vscroll(int id, rect_t *prc, int64_t min, int64_t max, int64_t cur, int64_t range);

/** @brief get vertical scroll bar current value */
extern int64_t sui_get_vscroll_cur(int id);

/** @brief get a simple user interface element's text */
extern const char *sui_get_text(int id);

/** @brief change a simple user interface element's text */
extern int sui_set_text(int id, const char *fmt, ...);

/** @brief change a simple user interface element's range status */
extern int sui_set_range(int id, int min, int cur, int max);

/** @brief get a simple user interface element's range current value */
extern int sui_get_range_cur(int id);

/** @brief add to a simple user interface element's range current value */
extern int sui_add_range_cur(int id, int add);

/** @brief change a simple user interface element's progress status */
extern int sui_set_progress(int id, int min, int cur, int max);

/** @brief get a simple user interface element's type */
extern sui_type_t sui_get_type(int id);

/** @brief get a simple user interface element's status flag */
extern sui_flag_t sui_get_flag(int id);

/** @brief change a simple user interface element's status flag */
extern int sui_set_flag(int id, sui_flag_t flag);

/** @brief toggle a simple user interface element's status flags */
extern int sui_tog_flag(int id, int toggle);

/** @brief set the visibilty of a simple user element by id */
extern int sui_set_visible(int id, int on);

/** @brief get the selected status of a simple user element by id */
extern int sui_get_selected(int id);

/** @brief set the selected status of a simple user element by id */
extern int sui_set_selected(int id, int on);

/** @brief get the checked status of a simple user element by id */
extern int sui_get_checked(int id);

/** @brief set the checked status of a simple user element by id */
extern int sui_set_checked(int id, int on);

/** @brief set the enabled status of a simple user element by id */
extern int sui_set_enable(int id, int on);

/** @brief get a simple user interface element's text color */
extern int sui_get_col(int id);

/** @brief change a simple user interface element's text color */
extern int sui_set_col(int id, int col);

/** @brief get the rectangle of a simple user interface element by id */
extern int sui_get_rect(int id, rect_t *rc);

/** @brief set the rectangle of a simple user element by id */
extern int sui_set_rect(int id, const rect_t *rc);

/** @brief get x coordinate of element by id */
extern int sui_get_x(int id);

/** @brief get y coordinate of element by id */
extern int sui_get_y(int id);

/** @brief get (visible) width of element by id */
extern int sui_get_w(int id);

/** @brief get (visible) height of element by id */
extern int sui_get_h(int id);

/** @brief set x coordinate of element by id */
extern int sui_set_x(int id, int x);

/** @brief set y coordinate of element by id */
extern int sui_set_y(int id, int y);

/** @brief set width of element by id */
extern int sui_set_w(int id, int w);

/** @brief set height of element by id */
extern int sui_set_h(int id, int h);

/** @brief browser to select a path or filename */
extern int sui_dirtree(sdl_t *sdl, sdl_pane_t pane, int x, int y, dirtree_flag_t flag, char *path, int **pid);

#ifdef	__cplusplus
}
#endif
#endif	/* RENDER_TO_SDL */

#endif	/* !defined(_sui_h_included_) */
