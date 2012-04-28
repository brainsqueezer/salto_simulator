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
 * Keyboard memory mapped I/O functions
 *
 * $Id: keyboard.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <SDL.h>

#include "alto.h"
#include "cpu.h"
#include "memory.h"
#include "keyboard.h"

static int kbd_matrix[4];
static int kbd_bootkey;

/** @brief defining names for the Alto keys */
alto_key_t alto_key[ALTO_KEY_SIZE] = {
{"none",	KEY_NONE,		KEY_NONE	},
{"0",		KEY_0,			KEY_NONE	},
{"rparen",	KEY_0,			KEY_LSHIFT	},
{"1",		KEY_1,			KEY_NONE	},
{"exclam",	KEY_1,			KEY_LSHIFT	},
{"2",		KEY_2,			KEY_NONE	},
{"at",		KEY_2,			KEY_LSHIFT	},
{"3",		KEY_3,			KEY_NONE	},
{"hash",	KEY_3,			KEY_LSHIFT	},
{"4",		KEY_4,			KEY_NONE	},
{"dollar",	KEY_4,			KEY_LSHIFT	},
{"5",		KEY_5,			KEY_NONE	},
{"percent",	KEY_5,			KEY_LSHIFT	},
{"6",		KEY_6,			KEY_NONE	},
{"tilde",	KEY_6,			KEY_LSHIFT	},
{"7",		KEY_7,			KEY_NONE	},
{"ampersand",	KEY_7,			KEY_LSHIFT	},
{"8",		KEY_8,			KEY_NONE	},
{"asterisk",	KEY_8,			KEY_LSHIFT	},
{"9",		KEY_9,			KEY_NONE	},
{"lparen",	KEY_9,			KEY_LSHIFT	},
{"a",		KEY_A,			KEY_NONE	},
{"b",		KEY_B,			KEY_NONE	},
{"c",		KEY_C,			KEY_NONE	},
{"d",		KEY_D,			KEY_NONE	},
{"e",		KEY_E,			KEY_NONE	},
{"f",		KEY_F,			KEY_NONE	},
{"g",		KEY_G,			KEY_NONE	},
{"h",		KEY_H,			KEY_NONE	},
{"i",		KEY_I,			KEY_NONE	},
{"j",		KEY_J,			KEY_NONE	},
{"k",		KEY_K,			KEY_NONE	},
{"l",		KEY_L,			KEY_NONE	},
{"m",		KEY_M,			KEY_NONE	},
{"n",		KEY_N,			KEY_NONE	},
{"o",		KEY_O,			KEY_NONE	},
{"p",		KEY_P,			KEY_NONE	},
{"q",		KEY_Q,			KEY_NONE	},
{"r",		KEY_R,			KEY_NONE	},
{"s",		KEY_S,			KEY_NONE	},
{"t",		KEY_T,			KEY_NONE	},
{"u",		KEY_U,			KEY_NONE	},
{"v",		KEY_V,			KEY_NONE	},
{"w",		KEY_W,			KEY_NONE	},
{"x",		KEY_X,			KEY_NONE	},
{"y",		KEY_Y,			KEY_NONE	},
{"z",		KEY_Z,			KEY_NONE	},
{"lbracket",	KEY_LBRACKET,		KEY_NONE	},
{"backslash",	KEY_BACKSLASH,		KEY_NONE	},
{"rbracket",	KEY_RBRACKET,		KEY_NONE	},
{"leftarrow",	KEY_LEFTARROW,		KEY_NONE	},
{"uparrow",	KEY_LEFTARROW,		KEY_LSHIFT	},
{"delete",	KEY_DEL,		KEY_NONE	},
{"bw",		KEY_BW,			KEY_NONE	},
{"blank_bot",	KEY_BLANK_BOT,		KEY_NONE	},
{"blank_mid",	KEY_BLANK_MID,		KEY_NONE	},
{"blank_top",	KEY_BLANK_TOP,		KEY_NONE	},
{"comma",	KEY_COMMA,		KEY_NONE	},
{"less",	KEY_COMMA,		KEY_LSHIFT	},
{"minus",	KEY_MINUS,		KEY_NONE	},
{"underscore",	KEY_MINUS,		KEY_LSHIFT	},
{"period",	KEY_PERIOD,		KEY_NONE	},
{"greater",	KEY_PERIOD,		KEY_LSHIFT	},
{"slash",	KEY_SLASH,		KEY_NONE	},
{"question",	KEY_SLASH,		KEY_LSHIFT	},
{"equals",	KEY_EQUALS,		KEY_NONE	},
{"plus",	KEY_EQUALS,		KEY_LSHIFT	},
{"semicolon",	KEY_SEMICOLON,		KEY_NONE	},
{"colon",	KEY_SEMICOLON,		KEY_LSHIFT	},
{"backspace",	KEY_BS,			KEY_NONE	},
{"tab",		KEY_TAB,		KEY_NONE	},
{"lock",	KEY_LOCK,		KEY_NONE	},
{"ctrl",	KEY_CTRL,		KEY_NONE	},
{"lshift",	KEY_LSHIFT,		KEY_NONE	},
{"rshift",	KEY_RSHIFT,		KEY_NONE	},
{"quote",	KEY_QUOTE,		KEY_NONE	},
{"quotedbl",	KEY_QUOTE,		KEY_LSHIFT	},
{"return",	KEY_RETURN,		KEY_NONE	},
{"lf",		KEY_LF,			KEY_NONE	},
{"escape",	KEY_ESCAPE,		KEY_NONE	},
{"space",	KEY_SPACE,		KEY_NONE	},
{"fl1",		KEY_FL1,		KEY_NONE	},
{"fl2",		KEY_FL2,		KEY_NONE	},
{"fl3",		KEY_FL3,		KEY_NONE	},
{"fl4",		KEY_FL4,		KEY_NONE	},
{"fr1",		KEY_FR1,		KEY_NONE	},
{"fr2",		KEY_FR2,		KEY_NONE	},
{"fr3",		KEY_FR3,		KEY_NONE	},
{"fr4",		KEY_FR4,		KEY_NONE	},
{"fr5",		KEY_FR5,		KEY_NONE	},
{"msw_2_17",	KEY_MSW_2_17,		KEY_NONE	},
{"msw_3_16",	KEY_MSW_3_16,		KEY_NONE	},
{"msw_3_17",	KEY_MSW_3_17,		KEY_NONE	}
};

/**
 * @brief keyboard mapping SDLK_xxx to Xerox Alto keyboard KEY_xxx
 */
key_map_t key_map[KEY_MAP_SIZE]= {
/* The keyboard syms have been cleverly chosen to map to ASCII */
{"unknown",		SDLK_UNKNOWN,		KEY_NONE,	KEY_NONE	},
{"backspace",		SDLK_BACKSPACE,		KEY_BS,		KEY_NONE	},
{"tab",			SDLK_TAB,		KEY_TAB,	KEY_NONE	},
{"clear",		SDLK_CLEAR,		KEY_NONE,	KEY_NONE	},
{"return",		SDLK_RETURN,		KEY_RETURN,	KEY_NONE	},
{"pause",		SDLK_PAUSE,		KEY_NONE,	KEY_NONE	},
{"escape",		SDLK_ESCAPE,		KEY_ESCAPE,	KEY_NONE	},
{"space",		SDLK_SPACE,		KEY_SPACE,	KEY_NONE	},
{"exclaim",		SDLK_EXCLAIM,		KEY_1,		KEY_NONE	},
{"quotedbl",		SDLK_QUOTEDBL,		KEY_QUOTE,	KEY_LSHIFT	},
{"hash",		SDLK_HASH,		KEY_3,		KEY_NONE	},
{"dollar",		SDLK_DOLLAR,		KEY_4,		KEY_NONE	},
{"ampersand",		SDLK_AMPERSAND,		KEY_7,		KEY_NONE	},
{"quote",		SDLK_QUOTE,		KEY_QUOTE,	KEY_NONE	},
{"leftparen",		SDLK_LEFTPAREN,		KEY_9,		KEY_NONE	},
{"rightparen",		SDLK_RIGHTPAREN,	KEY_0,		KEY_NONE	},
{"asterisk",		SDLK_ASTERISK,		KEY_8,		KEY_NONE	},
{"plus",		SDLK_PLUS,		KEY_EQUALS,	KEY_LSHIFT	},
{"comma",		SDLK_COMMA,		KEY_COMMA,	KEY_NONE	},
{"minus",		SDLK_MINUS,		KEY_SLASH,	KEY_NONE	},
{"period",		SDLK_PERIOD,		KEY_PERIOD,	KEY_NONE	},
{"slash",		SDLK_SLASH,		KEY_SLASH,	KEY_NONE	},
{"0",			SDLK_0,			KEY_0,		KEY_NONE	},
{"1",			SDLK_1,			KEY_1,		KEY_NONE	},
{"2",			SDLK_2,			KEY_2,		KEY_NONE	},
{"3",			SDLK_3,			KEY_3,		KEY_NONE	},
{"4",			SDLK_4,			KEY_4,		KEY_NONE	},
{"5",			SDLK_5,			KEY_5,		KEY_NONE	},
{"6",			SDLK_6,			KEY_6,		KEY_NONE	},
{"7",			SDLK_7,			KEY_7,		KEY_NONE	},
{"8",			SDLK_8,			KEY_8,		KEY_NONE	},
{"9",			SDLK_9,			KEY_9,		KEY_NONE	},
{"colon",		SDLK_COLON,		KEY_SEMICOLON,	KEY_LSHIFT	},
{"semicolon",		SDLK_SEMICOLON,		KEY_SEMICOLON,	KEY_NONE	},
{"less",		SDLK_LESS,		KEY_COMMA,	KEY_LSHIFT	},
{"equals",		SDLK_EQUALS,		KEY_EQUALS,	KEY_NONE	},
{"greater",		SDLK_GREATER,		KEY_PERIOD,	KEY_LSHIFT	},
{"question",		SDLK_QUESTION,		KEY_SLASH,	KEY_NONE	},
{"at",			SDLK_AT,		KEY_2,		KEY_LSHIFT	},
/* 
   Skip uppercase letters
 */
{"leftbracket",		SDLK_LEFTBRACKET,	KEY_LBRACKET,	KEY_NONE	},
{"backslash",		SDLK_BACKSLASH,		KEY_BACKSLASH,	KEY_NONE	},
{"rightbracket",	SDLK_RIGHTBRACKET,	KEY_RBRACKET,	KEY_NONE	},
{"caret",		SDLK_CARET,		KEY_LEFTARROW,	KEY_NONE	},
{"underscore",		SDLK_UNDERSCORE,	KEY_SLASH,	KEY_NONE	},
{"backquote",		SDLK_BACKQUOTE,		KEY_NONE,	KEY_NONE	},
{"a",			SDLK_a,			KEY_A,		KEY_NONE	},
{"b",			SDLK_b,			KEY_B,		KEY_NONE	},
{"c",			SDLK_c,			KEY_C,		KEY_NONE	},
{"d",			SDLK_d,			KEY_D,		KEY_NONE	},
{"e",			SDLK_e,			KEY_E,		KEY_NONE	},
{"f",			SDLK_f,			KEY_F,		KEY_NONE	},
{"g",			SDLK_g,			KEY_G,		KEY_NONE	},
{"h",			SDLK_h,			KEY_H,		KEY_NONE	},
{"i",			SDLK_i,			KEY_I,		KEY_NONE	},
{"j",			SDLK_j,			KEY_J,		KEY_NONE	},
{"k",			SDLK_k,			KEY_K,		KEY_NONE	},
{"l",			SDLK_l,			KEY_L,		KEY_NONE	},
{"m",			SDLK_m,			KEY_M,		KEY_NONE	},
{"n",			SDLK_n,			KEY_N,		KEY_NONE	},
{"o",			SDLK_o,			KEY_O,		KEY_NONE	},
{"p",			SDLK_p,			KEY_P,		KEY_NONE	},
{"q",			SDLK_q,			KEY_Q,		KEY_NONE	},
{"r",			SDLK_r,			KEY_R,		KEY_NONE	},
{"s",			SDLK_s,			KEY_S,		KEY_NONE	},
{"t",			SDLK_t,			KEY_T,		KEY_NONE	},
{"u",			SDLK_u,			KEY_U,		KEY_NONE	},
{"v",			SDLK_v,			KEY_V,		KEY_NONE	},
{"w",			SDLK_w,			KEY_W,		KEY_NONE	},
{"x",			SDLK_x,			KEY_X,		KEY_NONE	},
{"y",			SDLK_y,			KEY_Y,		KEY_NONE	},
{"z",			SDLK_z,			KEY_Z,		KEY_NONE	},
{"delete",		SDLK_DELETE,		KEY_DEL,	KEY_NONE	},
/* End of ASCII mapped keysyms */

/* International keyboard syms */
{"world_0",		SDLK_WORLD_0,		KEY_NONE,	KEY_NONE	},	/* 0xA0 */
{"world_1",		SDLK_WORLD_1,		KEY_NONE,	KEY_NONE	},
{"world_2",		SDLK_WORLD_2,		KEY_NONE,	KEY_NONE	},
{"world_3",		SDLK_WORLD_3,		KEY_NONE,	KEY_NONE	},
{"world_4",		SDLK_WORLD_4,		KEY_NONE,	KEY_NONE	},
{"world_5",		SDLK_WORLD_5,		KEY_NONE,	KEY_NONE	},
{"world_6",		SDLK_WORLD_6,		KEY_NONE,	KEY_NONE	},
{"world_7",		SDLK_WORLD_7,		KEY_NONE,	KEY_NONE	},
{"world_8",		SDLK_WORLD_8,		KEY_NONE,	KEY_NONE	},
{"world_9",		SDLK_WORLD_9,		KEY_NONE,	KEY_NONE	},
{"world_10",		SDLK_WORLD_10,		KEY_NONE,	KEY_NONE	},
{"world_11",		SDLK_WORLD_11,		KEY_NONE,	KEY_NONE	},
{"world_12",		SDLK_WORLD_12,		KEY_NONE,	KEY_NONE	},
{"world_13",		SDLK_WORLD_13,		KEY_NONE,	KEY_NONE	},
{"world_14",		SDLK_WORLD_14,		KEY_NONE,	KEY_NONE	},
{"world_15",		SDLK_WORLD_15,		KEY_NONE,	KEY_NONE	},
{"world_16",		SDLK_WORLD_16,		KEY_NONE,	KEY_NONE	},
{"world_17",		SDLK_WORLD_17,		KEY_NONE,	KEY_NONE	},
{"world_18",		SDLK_WORLD_18,		KEY_NONE,	KEY_NONE	},
{"world_19",		SDLK_WORLD_19,		KEY_NONE,	KEY_NONE	},
{"world_20",		SDLK_WORLD_20,		KEY_QUOTE,	KEY_NONE	},	/* ´ ` on my kbd */
{"world_21",		SDLK_WORLD_21,		KEY_NONE,	KEY_NONE	},
{"world_22",		SDLK_WORLD_22,		KEY_NONE,	KEY_NONE	},
{"world_23",		SDLK_WORLD_23,		KEY_NONE,	KEY_NONE	},
{"world_24",		SDLK_WORLD_24,		KEY_NONE,	KEY_NONE	},
{"world_25",		SDLK_WORLD_25,		KEY_NONE,	KEY_NONE	},
{"world_26",		SDLK_WORLD_26,		KEY_NONE,	KEY_NONE	},
{"world_27",		SDLK_WORLD_27,		KEY_NONE,	KEY_NONE	},
{"world_28",		SDLK_WORLD_28,		KEY_NONE,	KEY_NONE	},
{"world_29",		SDLK_WORLD_29,		KEY_NONE,	KEY_NONE	},
{"world_30",		SDLK_WORLD_30,		KEY_NONE,	KEY_NONE	},
{"world_31",		SDLK_WORLD_31,		KEY_NONE,	KEY_NONE	},
{"world_32",		SDLK_WORLD_32,		KEY_NONE,	KEY_NONE	},
{"world_33",		SDLK_WORLD_33,		KEY_NONE,	KEY_NONE	},
{"world_34",		SDLK_WORLD_34,		KEY_NONE,	KEY_NONE	},
{"world_35",		SDLK_WORLD_35,		KEY_NONE,	KEY_NONE	},
{"world_36",		SDLK_WORLD_36,		KEY_NONE,	KEY_NONE	},
{"world_37",		SDLK_WORLD_37,		KEY_NONE,	KEY_NONE	},
{"world_38",		SDLK_WORLD_38,		KEY_NONE,	KEY_NONE	},
{"world_39",		SDLK_WORLD_39,		KEY_NONE,	KEY_NONE	},
{"world_40",		SDLK_WORLD_40,		KEY_NONE,	KEY_NONE	},
{"world_41",		SDLK_WORLD_41,		KEY_NONE,	KEY_NONE	},
{"world_42",		SDLK_WORLD_42,		KEY_NONE,	KEY_NONE	},
{"world_43",		SDLK_WORLD_43,		KEY_NONE,	KEY_NONE	},
{"world_44",		SDLK_WORLD_44,		KEY_NONE,	KEY_NONE	},
{"world_45",		SDLK_WORLD_45,		KEY_NONE,	KEY_NONE	},
{"world_46",		SDLK_WORLD_46,		KEY_NONE,	KEY_NONE	},
{"world_47",		SDLK_WORLD_47,		KEY_NONE,	KEY_NONE	},
{"world_48",		SDLK_WORLD_48,		KEY_NONE,	KEY_NONE	},
{"world_49",		SDLK_WORLD_49,		KEY_NONE,	KEY_NONE	},
{"world_50",		SDLK_WORLD_50,		KEY_NONE,	KEY_NONE	},
{"world_51",		SDLK_WORLD_51,		KEY_NONE,	KEY_NONE	},
{"world_52",		SDLK_WORLD_52,		KEY_NONE,	KEY_NONE	},
{"world_53",		SDLK_WORLD_53,		KEY_NONE,	KEY_NONE	},
{"world_54",		SDLK_WORLD_54,		KEY_NONE,	KEY_NONE	},
{"world_55",		SDLK_WORLD_55,		KEY_NONE,	KEY_NONE	},
{"world_56",		SDLK_WORLD_56,		KEY_NONE,	KEY_NONE	},
{"world_57",		SDLK_WORLD_57,		KEY_NONE,	KEY_NONE	},
{"world_58",		SDLK_WORLD_58,		KEY_NONE,	KEY_NONE	},
{"world_59",		SDLK_WORLD_59,		KEY_NONE,	KEY_NONE	},
{"world_60",		SDLK_WORLD_60,		KEY_NONE,	KEY_NONE	},
{"world_61",		SDLK_WORLD_61,		KEY_NONE,	KEY_NONE	},
{"world_62",		SDLK_WORLD_62,		KEY_NONE,	KEY_NONE	},
{"world_63",		SDLK_WORLD_63,		KEY_MINUS,	KEY_NONE	},
{"world_64",		SDLK_WORLD_64,		KEY_NONE,	KEY_NONE	},
{"world_65",		SDLK_WORLD_65,		KEY_NONE,	KEY_NONE	},
{"world_66",		SDLK_WORLD_66,		KEY_NONE,	KEY_NONE	},
{"world_67",		SDLK_WORLD_67,		KEY_NONE,	KEY_NONE	},
{"world_68",		SDLK_WORLD_68,		KEY_NONE,	KEY_NONE	},
{"world_69",		SDLK_WORLD_69,		KEY_NONE,	KEY_NONE	},
{"world_70",		SDLK_WORLD_70,		KEY_NONE,	KEY_NONE	},
{"world_71",		SDLK_WORLD_71,		KEY_NONE,	KEY_NONE	},
{"world_72",		SDLK_WORLD_72,		KEY_NONE,	KEY_NONE	},
{"world_73",		SDLK_WORLD_73,		KEY_NONE,	KEY_NONE	},
{"world_74",		SDLK_WORLD_74,		KEY_NONE,	KEY_NONE	},
{"world_75",		SDLK_WORLD_75,		KEY_NONE,	KEY_NONE	},
{"world_76",		SDLK_WORLD_76,		KEY_NONE,	KEY_NONE	},
{"world_77",		SDLK_WORLD_77,		KEY_NONE,	KEY_NONE	},
{"world_78",		SDLK_WORLD_78,		KEY_NONE,	KEY_NONE	},
{"world_79",		SDLK_WORLD_79,		KEY_NONE,	KEY_NONE	},
{"world_80",		SDLK_WORLD_80,		KEY_NONE,	KEY_NONE	},
{"world_81",		SDLK_WORLD_81,		KEY_NONE,	KEY_NONE	},
{"world_82",		SDLK_WORLD_82,		KEY_NONE,	KEY_NONE	},
{"world_83",		SDLK_WORLD_83,		KEY_NONE,	KEY_NONE	},
{"world_84",		SDLK_WORLD_84,		KEY_NONE,	KEY_NONE	},
{"world_85",		SDLK_WORLD_85,		KEY_NONE,	KEY_NONE	},
{"world_86",		SDLK_WORLD_86,		KEY_SEMICOLON,	KEY_NONE	},
{"world_87",		SDLK_WORLD_87,		KEY_NONE,	KEY_NONE	},
{"world_88",		SDLK_WORLD_88,		KEY_NONE,	KEY_NONE	},
{"world_89",		SDLK_WORLD_89,		KEY_NONE,	KEY_NONE	},
{"world_90",		SDLK_WORLD_90,		KEY_NONE,	KEY_NONE	},
{"world_91",		SDLK_WORLD_91,		KEY_NONE,	KEY_NONE	},
{"world_92",		SDLK_WORLD_92,		KEY_NONE,	KEY_NONE	},
{"world_93",		SDLK_WORLD_93,		KEY_NONE,	KEY_NONE	},
{"world_94",		SDLK_WORLD_94,		KEY_NONE,	KEY_NONE	},
{"world_95",		SDLK_WORLD_95,		KEY_NONE,	KEY_NONE	},	/* 0xFF */

/* Numeric keypad */
{"kp0",			SDLK_KP0,		KEY_NONE,	KEY_NONE	},
{"kp1",			SDLK_KP1,		KEY_NONE,	KEY_NONE	},
{"kp2",			SDLK_KP2,		KEY_NONE,	KEY_NONE	},
{"kp3",			SDLK_KP3,		KEY_NONE,	KEY_NONE	},
{"kp4",			SDLK_KP4,		KEY_CTRL,	KEY_NONE	},/* "left" in some games */
{"kp5",			SDLK_KP5,		KEY_RETURN,	KEY_NONE	},/* "fire" in some games */
{"kp6",			SDLK_KP6,		KEY_A,		KEY_NONE	},/* "right" in some games */
{"kp7",			SDLK_KP7,		KEY_NONE,	KEY_NONE	},
{"kp8",			SDLK_KP8,		KEY_NONE,	KEY_NONE	},
{"kp9",			SDLK_KP9,		KEY_NONE,	KEY_NONE	},
{"kp_period",		SDLK_KP_PERIOD,		KEY_NONE,	KEY_NONE	},
{"kp_divide",		SDLK_KP_DIVIDE,		KEY_NONE,	KEY_NONE	},
{"kp_multiply",		SDLK_KP_MULTIPLY,	KEY_NONE,	KEY_NONE	},
{"kp_minus",		SDLK_KP_MINUS,		KEY_NONE,	KEY_NONE	},
{"kp_plus",		SDLK_KP_PLUS,		KEY_NONE,	KEY_NONE	},
{"kp_enter",		SDLK_KP_ENTER,		KEY_NONE,	KEY_NONE	},
{"kp_equals",		SDLK_KP_EQUALS,		KEY_NONE,	KEY_NONE	},

/* Arrows + Home/End pad */
{"up",			SDLK_UP,		KEY_NONE,	KEY_NONE	},
{"down",		SDLK_DOWN,		KEY_NONE,	KEY_NONE	},
{"right",		SDLK_RIGHT,		KEY_NONE,	KEY_NONE	},
{"left",		SDLK_LEFT,		KEY_NONE,	KEY_NONE	},
{"insert",		SDLK_INSERT,		KEY_NONE,	KEY_NONE	},
{"home",		SDLK_HOME,		KEY_NONE,	KEY_NONE	},
{"end",			SDLK_END,		KEY_NONE,	KEY_NONE	},
{"pageup",		SDLK_PAGEUP,		KEY_NONE,	KEY_NONE	},
{"pagedown",		SDLK_PAGEDOWN,		KEY_NONE,	KEY_NONE	},

/* Function keys */
{"f1",			SDLK_F1,		KEY_FL1,	KEY_NONE	},
{"f2",			SDLK_F2,		KEY_FL2,	KEY_NONE	},
{"f3",			SDLK_F3,		KEY_FL3,	KEY_NONE	},
{"f4",			SDLK_F4,		KEY_FL4,	KEY_NONE	},
{"f5",			SDLK_F5,		KEY_FR1,	KEY_NONE	},
{"f6",			SDLK_F6,		KEY_FR2,	KEY_NONE	},
{"f7",			SDLK_F7,		KEY_FR3,	KEY_NONE	},
{"f8",			SDLK_F8,		KEY_FR4,	KEY_NONE	},
{"f9",			SDLK_F9,		KEY_FR5,	KEY_NONE	},
{"f10",			SDLK_F10,		KEY_BW,		KEY_NONE	},
{"f11",			SDLK_F11,		KEY_NONE,	KEY_NONE	},
{"f12",			SDLK_F12,		KEY_NONE,	KEY_NONE	},
{"f13",			SDLK_F13,		KEY_NONE,	KEY_NONE	},
{"f14",			SDLK_F14,		KEY_NONE,	KEY_NONE	},
{"f15",			SDLK_F15,		KEY_NONE,	KEY_NONE	},

/* Key state modifier keys */
{"numlock",		SDLK_NUMLOCK,		KEY_NONE,	KEY_NONE	},
{"capslock",		SDLK_CAPSLOCK,		KEY_LOCK,	KEY_NONE	},
{"scrollock",		SDLK_SCROLLOCK,		KEY_NONE,	KEY_NONE	},
{"rshift",		SDLK_RSHIFT,		KEY_RSHIFT,	KEY_NONE	},
{"lshift",		SDLK_LSHIFT,		KEY_LSHIFT,	KEY_NONE	},
{"rctrl",		SDLK_RCTRL,		KEY_CTRL,	KEY_NONE	},
{"lctrl",		SDLK_LCTRL,		KEY_CTRL,	KEY_NONE	},
{"ralt",		SDLK_RALT,		KEY_NONE,	KEY_NONE	},
{"lalt",		SDLK_LALT,		KEY_NONE,	KEY_NONE	},
{"rmeta",		SDLK_RMETA,		KEY_NONE,	KEY_NONE	},
{"lmeta",		SDLK_LMETA,		KEY_NONE,	KEY_NONE	},
/* Left "Windows" key */
{"lsuper",		SDLK_LSUPER,		KEY_NONE,	KEY_NONE	},
/* Right "Windows" key */
{"rsuper",		SDLK_RSUPER,		KEY_NONE,	KEY_NONE	},
/* "Alt Gr" key */
{"mode",		SDLK_MODE,		KEY_NONE,	KEY_NONE	},
/* Multi-key compose key */
{"compose",		SDLK_COMPOSE,		KEY_NONE,	KEY_NONE	},

/* Miscellaneous function keys */
{"help",		SDLK_HELP,		KEY_NONE,	KEY_NONE	},
{"print",		SDLK_PRINT,		KEY_NONE,	KEY_NONE	},
{"sysreq",		SDLK_SYSREQ,		KEY_NONE,	KEY_NONE	},
{"break",		SDLK_BREAK,		KEY_NONE,	KEY_NONE	},
{"menu",		SDLK_MENU,		KEY_NONE,	KEY_NONE	},
/* Power Macintosh power key */
{"power",		SDLK_POWER,		KEY_NONE,	KEY_NONE	},
/* Some european keyboards */
{"euro",		SDLK_EURO,		KEY_NONE,	KEY_NONE	},
/* Atari keyboard has Undo */
{"undo",		SDLK_UNDO,		KEY_NONE,	KEY_NONE	}
};

static char keyboard_conf[FILENAME_MAX] = "keyboard.conf";

/**
 * @brief read the keyboard address matrix
 *
 * @param addr memory mapped I/O address to be read
 * @result keyboard matrix value for address modulo 4
 */
int kbd_ad_r(int addr)
{
	int val = kbd_matrix[addr & 03];
	LOG((0,2,"	read KBDAD+%o (%#o)\n", addr & 03, val));
	if (0 == (addr & 3) && (kbd_bootkey != 0177777)) {
		LOG((0,2,"	boot keys (%#o & %#o)\n", val, kbd_bootkey));
		val &= kbd_bootkey;
		kbd_bootkey = 0177777;
	}
	return val;
}

/**
 * @brief hook for the frontend to stuff key down + up events
 *
 * @param keysym pointer to a SDL key symbol
 * @param down non-zero if the key is pressed, zero if released
 * @result return 0 if key is handled, -1 otherwise
 */
int kbd_key(SDL_keysym *keysym, int down)
{
	int i, addr1, bit1, addr2, bit2;

	/* scan the keyboard map */
	for (i = 0; i < KEY_MAP_SIZE; i++) {
		if (keysym->sym != key_map[i].sym)
			continue;
		addr1 = key_map[i].key1 >> 4;
		bit1 = key_map[i].key1 & 017;
		addr2 = key_map[i].key2 >> 4;
		bit2 = key_map[i].key2 & 017;
		if (addr1 < 0) {
			/* unmapped key */
			return -1;
		}
		if (down) {
			kbd_matrix[addr1 & 3] &= ~(1 << bit1);
			if (addr2 >= 0)
				kbd_matrix[addr2 & 3] &= ~(1 << bit2);
		} else {
			kbd_matrix[addr1 & 3] |= 1 << bit1;
			if (addr2 >= 0)
				kbd_matrix[addr2 & 3] |= 1 << bit2;
		}
		return 0;
	}
	return -1;
}


/**
 * @brief configure a keyboard symbol to map to an Alto key
 *
 * @result return 0 on success, -1 on invalid symbol or key name
 */
static int kbd_map_symbol_to_key(const char *sym_name, const char *key_name)
{
	int sym;
	int key;

	for (sym = 0; sym < KEY_MAP_SIZE; sym++)
		if (!strcasecmp(key_map[sym].name, sym_name))
			break;
	if (sym == KEY_MAP_SIZE) {
		fprintf(stderr, "Unknown SDL key symbol: %s\n", sym_name);
		return -1;
	}

	for (key = 0; key < ALTO_KEY_SIZE; key++)
		if (!strcasecmp(alto_key[key].name, key_name))
			break;
	if (key == ALTO_KEY_SIZE) {
		fprintf(stderr, "Unknown Alto key name: %s\n", key_name);
		return -1;
	}

	key_map[sym].key1 = alto_key[key].key1;
	key_map[sym].key2 = alto_key[key].key2;
	return 0;
}

/**
 * @brief hook for the frontend to set boot keys
 *
 * One or more of the keys of KBDAD can be held down at boot to
 * select Ethernet vs. disk boot, or to choose a boot sector
 *
 * @param key a key name to initially make down (bit = 0)
 * @result returns 0 on success, -1 on unknown key name
 */
int kbd_boot(const char *key)
{
	if (!strcmp(key, "bs") || !strcmp(key, "backspace")) {
		kbd_bootkey &= ~(1 << 0);
		return 0;
	}
	if (!strcmp(key, "lf")) {
		kbd_bootkey &= ~(1 << 1);
		return 0;
	}
	if (!strcmp(key, "\\") || !strcmp(key, "backslash")) {
		kbd_bootkey &= ~(1 << 2);
		return 0;
	}
	if (!strcmp(key, "/") || !strcmp(key, "slash")) {
		kbd_bootkey &= ~(1 << 3);
		return 0;
	}
	if (!strcmp(key, "p")) {
		kbd_bootkey &= ~(1 << 4);
		return 0;
	}
	if (!strcmp(key, "-") || !strcmp(key, "minus")) {
		kbd_bootkey &= ~(1 << 5);
		return 0;
	}
	if (!strcmp(key, "k")) {
		kbd_bootkey &= ~(1 << 6);
		return 0;
	}
	if (!strcmp(key, "0")) {
		kbd_bootkey &= ~(1 << 7);
		return 0;
	}
	if (!strcmp(key, "v")) {
		kbd_bootkey &= ~(1 << 8);
		return 0;
	}
	if (!strcmp(key, "u")) {
		kbd_bootkey &= ~(1 << 9);
		return 0;
	}
	if (!strcmp(key, "d")) {
		kbd_bootkey &= ~(1 << 10);
		return 0;
	}
	if (!strcmp(key, "7")) {
		kbd_bootkey &= ~(1 << 11);
		return 0;
	}
	if (!strcmp(key, "e")) {
		kbd_bootkey &= ~(1 << 12);
		return 0;
	}
	if (!strcmp(key, "6")) {
		kbd_bootkey &= ~(1 << 13);
		return 0;
	}
	if (!strcmp(key, "4")) {
		kbd_bootkey &= ~(1 << 14);
		return 0;
	}
	if (!strcmp(key, "5")) {
		kbd_bootkey &= ~(1 << 15);
		return 0;
	}
	return -1;
}

/**
 * @brief print out a keyboard configuration file
 *
 * @result return 0 on success
 */
int kbd_config_write(void)
{
	int addr, bit;
	int i, j, w;
	FILE *fp;

	fp = fopen(keyboard_conf, "w");
	if (!fp) {
		fprintf(stderr, "Cannot create '%s'\n", keyboard_conf);
		return -1;
	}
	fprintf(stderr, "Created a '%s' file.\n", keyboard_conf);

	fprintf(fp, "# SALTO keyboard configuration file\n");
	fprintf(fp, "# Adjust to your needs, and put a copy in a safe place.\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# Unmapped SDL keys are inserted as comment lines.\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# SDL symbol\tAlto key 1\tAlto key 2 (e.g. lshift)\n");
	fprintf(fp, "\n");

	for (i = 1; i < KEY_MAP_SIZE; i++) {
		key_map_t *k = &key_map[i];
		if (k->key1 == KEY_NONE && k->key2 == KEY_NONE) {
			fprintf(fp, "# %s\n", k->name);
			continue;
		}
		w = fprintf(fp, "%s", k->name);
		while (w < 16) {
			fprintf(fp, "\t");
			w = (w | 7) + 1;
		}
		for (j = 0; j < ALTO_KEY_SIZE; j++)
			if (k->key1 == alto_key[j].key1)
				break;
		if (j == ALTO_KEY_SIZE)
			fatal(1, "Invalid key1 definition for '%s'\n", k->name);

		/* No need to print 'none' in second column */
		if (k->key2 == KEY_NONE) {
			w += fprintf(fp, "%s", alto_key[j].name);
		} else {
			w += fprintf(fp, "%s", alto_key[j].name);
			while (w < 32) {
				fprintf(fp, "\t");
				w = (w | 7) + 1;
			}
			for (j = 0; j < ALTO_KEY_SIZE; j++)
				if (k->key2 == alto_key[j].key1)
					break;
			if (j == ALTO_KEY_SIZE)
				fatal(1, "Invalid key2 definition for '%s'\n", k->name);
			w += fprintf(fp, "%s", alto_key[j].name);
		}
		while (w < 48) {
			fprintf(fp, "\t");
			w = (w | 7) + 1;
		}

		/* print first key address and bit as a comment */
		addr = k->key1 >> 4;
		bit = k->key1 & 017;
		fprintf(fp, "# row:%d bit:%d", addr, bit);
		if (k->key2 != KEY_NONE) {
			/* print second key address and bit as a comment */
			addr = k->key2 >> 4;
			bit = k->key2 & 017;
			fprintf(fp, "; row:%d bit:%d", addr, bit);
		}
		fprintf(fp, "\n");
	}
	return 0;
}

/**
 * @brief read a keyboard configuration file and setup the table
 *
 * @result return 0 on success
 */
int kbd_config_read(void)
{
	char line[256], *sym, *key;
	FILE *fp;
	int i;

	fp = fopen(keyboard_conf, "r");
	if (!fp) {
		fprintf(stderr, "Cannot read '%s', using defaults\n", keyboard_conf);
		kbd_config_write();
		return -1;
	}

	/* reset all entries to KEY_NONE */
	for (i = 0; i < KEY_MAP_SIZE; i++) {
		key_map[i].key1 = KEY_NONE;
		key_map[i].key2 = KEY_NONE;
	}

	while (!feof(fp)) {
		if (!fgets(line, sizeof(line), fp))
			break;
		if (*line == '#' || *line == '\r' || *line == '\n')
			continue;
		sym = strtok(line, "\t \n");
		key = strtok(NULL, "\t \n");
		kbd_map_symbol_to_key(sym, key);
	}
	fclose(fp);
	return 0;
}

/**
 * @brief clear all keys and install the mmio handler for KBDAD to KBDAD+3
 *
 * @result return 0 on success
 */
int init_kbd(void)
{
	kbd_config_read();

	kbd_bootkey = 0177777;
	kbd_matrix[0] = kbd_matrix[1] = kbd_matrix[2] = kbd_matrix[3] = 0177777;
	install_mmio_fn(0177034, 0177037, kbd_ad_r, NULL);

	return 0;
}
