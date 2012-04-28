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
 * Memory mapped I/O for keyboard
 *
 * $Id: keyboard.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_KEYBOARD_H_INCLUDED_)
#define	_KEYBOARD_H_INCLUDED_

/** @brief need to include SDL.h for the key symbols */
#include <SDL.h>


/** @brief make an Alto key int from address * 16 + bit */
#define	MAKE_KEY(a,b) (16 * (a) + (b))

/** @brief no key assigned is -1 */
#define	KEY_NONE	(-1)

/** @brief normal: 5    shifted: % */
#define	KEY_5		MAKE_KEY(0,017)
/** @brief normal: 4    shifted: $ */
#define	KEY_4		MAKE_KEY(0,016)
/** @brief normal: 6    shifted: ~ */
#define	KEY_6		MAKE_KEY(0,015)
/** @brief normal: e    shifted: E */
#define	KEY_E		MAKE_KEY(0,014)
/** @brief normal: 7    shifted: & */
#define	KEY_7		MAKE_KEY(0,013)
/** @brief normal: d    shifted: D */
#define	KEY_D		MAKE_KEY(0,012)
/** @brief normal: u    shifted: U */
#define	KEY_U		MAKE_KEY(0,011)
/** @brief normal: v    shifted: V */
#define	KEY_V		MAKE_KEY(0,010)
/** @brief normal: 0    shifted: ) */
#define	KEY_0		MAKE_KEY(0,007)
/** @brief normal: k    shifted: K */
#define	KEY_K		MAKE_KEY(0,006)
/** @brief normal: -    shifted: _ */
#define	KEY_MINUS	MAKE_KEY(0,005)
/** @brief normal: p    shifted: P */
#define	KEY_P		MAKE_KEY(0,004)
/** @brief normal: /    shifted: ? */
#define	KEY_SLASH	MAKE_KEY(0,003)
/** @brief normal: \    shifted: | */
#define	KEY_BACKSLASH	MAKE_KEY(0,002)
/** @brief normal: LF   shifted: ? */
#define	KEY_LF		MAKE_KEY(0,001)
/** @brief normal: BS   shifted: ? */
#define	KEY_BS		MAKE_KEY(0,000)

/** @brief: ADL right function key 2 */
#define	KEY_FR2		MAKE_KEY(0,002)
/** @brief: ADL left function key 1 */
#define	KEY_FL2		MAKE_KEY(0,001)

/** @brief normal: 3    shifted: # */
#define	KEY_3		MAKE_KEY(1,017)
/** @brief normal: 2    shifted: @ */
#define	KEY_2		MAKE_KEY(1,016)
/** @brief normal: w    shifted: W */
#define	KEY_W		MAKE_KEY(1,015)
/** @brief normal: q    shifted: Q */
#define	KEY_Q		MAKE_KEY(1,014)
/** @brief normal: s    shifted: S */
#define	KEY_S		MAKE_KEY(1,013)
/** @brief normal: a    shifted: A */
#define	KEY_A		MAKE_KEY(1,012)
/** @brief normal: 9    shifted: ( */
#define	KEY_9		MAKE_KEY(1,011)
/** @brief normal: i    shifted: I */
#define	KEY_I		MAKE_KEY(1,010)
/** @brief normal: x    shifted: X */
#define	KEY_X		MAKE_KEY(1,007)
/** @brief normal: o    shifted: O */
#define	KEY_O		MAKE_KEY(1,006)
/** @brief normal: l    shifted: L */
#define	KEY_L		MAKE_KEY(1,005)
/** @brief normal: ,    shifted: < */
#define	KEY_COMMA	MAKE_KEY(1,004)
/** @brief normal: '    shifted: " */
#define	KEY_QUOTE	MAKE_KEY(1,003)
/** @brief normal: ]    shifted: } */
#define	KEY_RBRACKET	MAKE_KEY(1,002)
/** @brief middle blank key */
#define	KEY_BLANK_MID	MAKE_KEY(1,001)
/** @brief top blank key */
#define	KEY_BLANK_TOP	MAKE_KEY(1,000)

/** @brief: ADL right funtion key 4 */
#define	KEY_FR4		MAKE_KEY(1,001)

/** @brief: ADL BW (?) */
#define	KEY_BW		MAKE_KEY(1,000)

/** @brief normal: 1    shifted: ! */
#define	KEY_1		MAKE_KEY(2,017)
/** @brief normal: ESC  shifted: ? */
#define	KEY_ESCAPE	MAKE_KEY(2,016)
/** @brief normal: TAB  shifted: ? */
#define	KEY_TAB		MAKE_KEY(2,015)
/** @brief normal: f    shifted: F */
#define	KEY_F		MAKE_KEY(2,014)
/** @brief CTRL */
#define	KEY_CTRL	MAKE_KEY(2,013)
/** @brief normal: c    shifted: C */
#define	KEY_C		MAKE_KEY(2,012)
/** @brief normal: j    shifted: J */
#define	KEY_J		MAKE_KEY(2,011)
/** @brief normal: b    shifted: B */
#define	KEY_B		MAKE_KEY(2,010)
/** @brief normal: z    shifted: Z */
#define	KEY_Z		MAKE_KEY(2,007)
/** @brief LSHIFT */
#define	KEY_LSHIFT	MAKE_KEY(2,006)
/** @brief normal: .    shifted: > */
#define	KEY_PERIOD	MAKE_KEY(2,005)
/** @brief normal: ;    shifted: : */
#define	KEY_SEMICOLON	MAKE_KEY(2,004)
/** @brief RETURN */
#define	KEY_RETURN	MAKE_KEY(2,003)
/** @brief normal: left arrow   shifted: up arrow (caret) */
#define	KEY_LEFTARROW	MAKE_KEY(2,002)
/** @brief normal: DEL  shifted: ? */
#define	KEY_DEL		MAKE_KEY(2,001)
/** @brief unused on Microswitch KDB */
#define	KEY_MSW_2_17	MAKE_KEY(2,000)

/** @brief ADL: right function key 3 */
#define	KEY_FR3		MAKE_KEY(2,002)
/** @brief ADL: left function key 1 */
#define	KEY_FL1		MAKE_KEY(2,001)
/** @brief ADL: left function key 3 */
#define	KEY_FL3		MAKE_KEY(2,000)

/** @brief normal: r    shifted: R */
#define	KEY_R		MAKE_KEY(3,017)
/** @brief normal: t    shifted: T */
#define	KEY_T		MAKE_KEY(3,016)
/** @brief normal: g    shifted: G */
#define	KEY_G		MAKE_KEY(3,015)
/** @brief normal: y    shifted: Y */
#define	KEY_Y		MAKE_KEY(3,014)
/** @brief normal: h    shifted: H */
#define	KEY_H		MAKE_KEY(3,013)
/** @brief normal: 8    shifted: * */
#define	KEY_8		MAKE_KEY(3,012)
/** @brief normal: n    shifted: N */
#define	KEY_N		MAKE_KEY(3,011)
/** @brief normal: m    shifted: M */
#define	KEY_M		MAKE_KEY(3,010)
/** @brief LOCK   */
#define	KEY_LOCK	MAKE_KEY(3,007)
/** @brief SPACE  */
#define	KEY_SPACE	MAKE_KEY(3,006)
/** @brief normal: [    shifted: { */
#define	KEY_LBRACKET	MAKE_KEY(3,005)
/** @brief normal: =    shifted: + */
#define	KEY_EQUALS	MAKE_KEY(3,004)
/** @brief RSHIFT */
#define	KEY_RSHIFT	MAKE_KEY(3,003)
/** @brief bottom blank key */
#define	KEY_BLANK_BOT	MAKE_KEY(3,002)
/** @brief unused on Microswitch KDB */
#define	KEY_MSW_3_16	MAKE_KEY(3,001)
/** @brief unused on Microswitch KDB */
#define	KEY_MSW_3_17	MAKE_KEY(3,000)

/** @brief ADL: right function key 4 */
#define	KEY_FR1		MAKE_KEY(3,002)
/** @brief ADL: left function key 4 */
#define	KEY_FL4		MAKE_KEY(3,001)
/** @brief ADL: right function key 5 */
#define	KEY_FR5		MAKE_KEY(3,000)

/** @brief structure to define names for the Alto keys */
typedef struct {
	/* symbolic name for the Alto key */
	const char *name;
	/* keyboard matrix value 1 */
	int key1;
	/* keyboard matrix value 2 (i.e. shift or control) */
	int key2;
}	alto_key_t;

/** @brief number of Alto keys (including dupe keys and shifted names) */
#define	ALTO_KEY_SIZE	93

/** @brief Alto key lookup table */
extern alto_key_t alto_key[ALTO_KEY_SIZE];

/** @brief structure to define SDLK_xxx to Xerox Alto keyboard KEY_xxx entries */
typedef struct {
	/* symbolic name for the SDLK key */
	const char *name;
	/* SDLK_xxx key symbol */
	int sym;
	/* 1st assigned alto_key_t entry */
	int key1;
	/* 2nd assigned alto_key_t entry (e.g. shift) */
	int key2;
}	key_map_t;

#define	KEY_MAP_SIZE	232

/** @brief keyboard map */
extern key_map_t key_map[KEY_MAP_SIZE];

/** @brief read a keyboard address */
extern int kbd_ad_r(int addr);

/** @brief hook for the frontend to stuff key down + up events */
extern int kbd_key(SDL_keysym *keysym, int down);

/** @brief hook for the frontend to set a boot key (may be called multiple times) */
extern int kbd_boot(const char *key);

/** @brief initialize the keyboard */
extern int init_kbd(void);

#endif	/* !defined(_KEYBOARD_H_INCLUDED_) */
