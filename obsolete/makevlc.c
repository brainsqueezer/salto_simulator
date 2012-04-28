/************************************************************************
 *
 * VLC
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
 * VLC code generator
 *
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"
#include "util.h"
#include "debug.h"

static const char *project = "Variable Length Codes";
static const char *copyright = "Copyright (C) 2007, Juergen Buchmueller <pullmoll@t-online.de>";
static const char *warning = "VLC codes - DO NOT EDIT! THIS IS GENERATED SOURCE CODE";

/** @brief include (currently unused) vlc_entry_t tables */
#define	VLC_TABLES	1

/** @brief generated header file */
FILE *fh;
/** @brief generated source file */
FILE *fc;

typedef struct {
	const char *vlc;
	int value;
	const char *comment;
}	vlc_t;

/* just a shorthand to get to the table sizes */
#define	S	sizeof(vlc_t)

static vlc_t macroblock_address_increment[] = {
	{"1",			1,			NULL},
	{"011",			2,			NULL},
	{"010",			3,			NULL},
	{"0011",		4,			NULL},
	{"0010",		5,			NULL},
	{"00011",		6,			NULL},
	{"00010",		7,			NULL},
	{"0000111",		8,			NULL},
	{"0000110",		9,			NULL},
	{"00001011",		10,			NULL},
	{"00001010",		11,			NULL},
	{"00001001",		12,			NULL},
	{"00001000",		13,			NULL},
	{"00000111",		14,			NULL},
	{"00000110",		15,			NULL},
	{"0000010111",		16,			NULL},
	{"0000010110",		17,			NULL},
	{"0000010101",		18,			NULL},
	{"0000010100",		19,			NULL},
	{"0000010011",		20,			NULL},
	{"0000010010",		21,			NULL},
	{"00000100011",		22,			NULL},
	{"00000100010",		23,			NULL},
	{"00000100001",		24,			NULL},
	{"00000100000",		25,			NULL},
	{"00000011111",		26,			NULL},
	{"00000011110",		27,			NULL},
	{"00000011101",		28,			NULL},
	{"00000011100",		29,			NULL},
	{"00000011011",		30,			NULL},
	{"00000011010",		31,			NULL},
	{"00000011001",		32,			NULL},
	{"00000011000",		33,			NULL},
	{"00000001000",		-1,			"Escape"}
};

/** @brief encode flags in binary
 *  q macroblock_quant
 *  f macroblock_motion_forward
 *  b macroblock_motion_backward
 *  p macroblock_pattern
 *  i macroblock_intra
 *  s spatial_temporal_weight_code_flag
 *  c permitted spatial_temporal_weight classes (1:0, 2:123, 4:4)
 */
#define	MBF(q,f,b,p,i,s,c) \
	(((q)<<8)|((f)<<7)|((b)<<6)|((p)<<5)|((i)<<4)|((s)<<3)|(c))

static vlc_t macroblock_type_i[] = {
	{"1",			MBF(0,0,0,0,1,0,1),	"Intra"},
	{"01",			MBF(1,0,0,0,1,0,1),	"Intra, Quant"}
};


static vlc_t macroblock_type_p[] = {
	{"1",			MBF(0,1,0,1,0,0,1),	"MC, Coded"},
	{"01",			MBF(0,0,0,1,0,0,1),	"No MC, Coded"},
	{"001",			MBF(0,1,0,0,0,0,1),	"MC, Not Coded"},
	{"00011",		MBF(0,0,0,0,1,0,1),	"Intra"},
	{"00010",		MBF(1,1,0,1,0,0,1),	"MC, Coded, Quant"},
	{"00001",		MBF(1,0,0,1,0,0,1),	"No MC, Coded, Quant"},
	{"000001",		MBF(1,0,0,0,1,0,1),	"Intra, Quant"}
};

static vlc_t macroblock_type_b[] = {
	{"10",			MBF(0,1,1,0,0,0,1),	"Interp, Not Coded"},
	{"11",			MBF(0,1,1,1,0,0,1),	"Interp, Coded"},
	{"010",			MBF(0,0,1,0,0,0,1),	"Bwd, Not Coded"},
	{"011",			MBF(0,0,1,1,0,0,1),	"Bwd, Coded"},
	{"0010",		MBF(0,1,0,0,0,0,1),	"Fwd, Not Coded"},
	{"0011",		MBF(0,1,0,1,0,0,1),	"Fwd, Coded"},
	{"00011",		MBF(0,0,0,0,1,0,1),	"Intra"},
	{"00010",		MBF(1,1,1,1,0,0,1),	"Interp, Coded, Quant"},
	{"000011",		MBF(1,1,0,1,0,0,1),	"Fwd, Coded, Quant"},
	{"000010",		MBF(1,0,1,1,0,0,1),	"Bwd, Coded, Quant"},
	{"000001",		MBF(1,0,0,0,1,0,1),	"Intra, Quant"}
};

static vlc_t macroblock_type_is[] = {
	{"1",			MBF(0,0,0,1,0,0,4),	"Coded, Compatible"},
	{"01",			MBF(1,0,0,1,0,0,4),	"Coded, Compatible, Quant"},
	{"0011",		MBF(0,0,0,0,1,0,1),	"Intra"},
	{"0010",		MBF(1,0,0,0,1,0,1),	"Intra, Quant"},
	{"0001",		MBF(0,0,0,0,0,0,4),	"Not Coded, Compatible"}
};

static vlc_t macroblock_type_ps[] = {
	{"10",			MBF(0,1,0,1,0,0,1),	"MC, Coded"},
	{"011",			MBF(0,1,0,1,0,1,2),	"MC, Coded, Compatible"},
	{"0000100",		MBF(0,0,0,1,0,0,1),	"No MC, Coded"},
	{"000111",		MBF(0,0,0,1,0,1,2),	"No MC, Coded, Compatible"},
	{"0010",		MBF(0,1,0,0,0,0,1),	"MC, Not Coded"},
	{"0000111",		MBF(0,0,0,0,1,0,1),	"Intra"},
	{"0011",		MBF(0,1,0,0,0,1,2),	"MC, Not Coded, Compatible"},
	{"010",			MBF(1,1,0,1,0,0,1),	"MC, Coded, Quant"},
	{"000100",		MBF(1,0,0,1,0,0,1),	"No MC, Coded, Quant"},
	{"0000110",		MBF(1,0,0,0,1,0,1),	"Intra, Quant"},
	{"11",			MBF(1,1,0,1,0,1,2),	"MC, Coded, Compatible, Quant"},
	{"000101",		MBF(1,0,0,1,0,1,2),	"No MC, Coded, Compatible, Quant"},
	{"000110",		MBF(0,0,0,0,0,1,2),	"No MC, Not Coded, Compatible"},
	{"0000101",		MBF(0,0,0,1,0,0,4),	"Coded, Compatible"},
	{"0000010",		MBF(1,0,0,1,0,0,4),	"Coded, Compatible, Quant"},
	{"0000011",		MBF(0,0,0,0,0,0,4),	"Not Coded, Compatible"}
};

static vlc_t macroblock_type_bs[] = {
	{"10",			MBF(0,1,1,0,0,0,1),	"Interp, Not coded"},
	{"11",			MBF(0,1,1,1,0,0,1),	"Interp, Coded"},
	{"010",			MBF(0,0,1,0,0,0,1),	"Back, Not coded"},
	{"011",			MBF(0,0,1,1,0,0,1),	"Back, Coded"},
	{"0010",		MBF(0,1,0,0,0,0,1),	"For, Not coded"},
	{"0011",		MBF(0,1,0,1,0,0,1),	"For, Coded"},
	{"000110",		MBF(0,0,1,0,0,1,2),	"Back, Not Coded, Compatible"},
	{"000111",		MBF(0,0,1,1,0,1,2),	"Back, Coded, Compatible"},
	{"000100",		MBF(0,1,0,0,0,1,2),	"For, Not Coded, Compatible"},
	{"000101",		MBF(0,1,0,1,0,1,2),	"For, Coded, Compatible"},
	{"0000110",		MBF(0,0,0,0,1,0,1),	"Intra"},
	{"0000111",		MBF(1,1,1,1,0,0,1),	"Interp, Coded, Quant"},
	{"0000100",		MBF(1,1,0,1,0,0,1),	"For, Coded, Quant"},
	{"0000101",		MBF(1,0,1,1,0,0,1),	"Back, Coded, Quant"},
	{"00000100",		MBF(1,0,0,0,1,0,1),	"Intra, Quant"},
	{"00000101",		MBF(1,1,0,1,0,1,2),	"For, Coded, Compatible, Quant"},
	{"000001100",		MBF(1,0,1,1,0,1,2),	"Back, Coded, Compatible, Quant"},
	{"000001110",		MBF(0,0,0,0,0,0,4),	"Not Coded, Compatible"},
	{"000001101",		MBF(1,0,0,1,0,0,4),	"Coded, Compatible, Quant"},
	{"000001111",		MBF(0,0,0,1,0,0,4),	"Coded, Compatible"}
};

static vlc_t macroblock_type_snr[] = {
	{"1",			MBF(0,0,0,1,0,0,1),	"Coded"},
	{"01",			MBF(1,0,0,1,0,0,1),	"Coded, Quant"},
	{"001",			MBF(0,0,0,0,0,0,1),	"Not Coded"}
};

static vlc_t macroblock_pattern[] = {
	{"111",			60,			NULL},
	{"1101",		4,			NULL},
	{"1100",		8,			NULL},
	{"1011",		16,			NULL},
	{"1010",		32,			NULL},
	{"10011",		12,			NULL},
	{"10010",		48,			NULL},
	{"10001",		20,			NULL},
	{"10000",		40,			NULL},
	{"01111",		28,			NULL},
	{"01110",		44,			NULL},
	{"01101",		52,			NULL},
	{"01100",		56,			NULL},
	{"01011",		1,			NULL},
	{"01010",		61,			NULL},
	{"01001",		2,			NULL},
	{"01000",		62,			NULL},
	{"001111",		24,			NULL},
	{"001110",		36,			NULL},
	{"001101",		3,			NULL},
	{"001100",		63,			NULL},
	{"0010111",		5,			NULL},
	{"0010110",		9,			NULL},
	{"0010101",		17,			NULL},
	{"0010100",		33,			NULL},
	{"0010011",		6,			NULL},
	{"0010010",		10,			NULL},
	{"0010001",		18,			NULL},
	{"0010000",		34,			NULL},
	{"00011111",		7,			NULL},
	{"00011110",		11,			NULL},
	{"00011101",		19,			NULL},
	{"00011100",		35,			NULL},
	{"00011011",		13,			NULL},
	{"00011010",		49,			NULL},
	{"00011001",		21,			NULL},
	{"00011000",		41,			NULL},
	{"00010111",		14,			NULL},
	{"00010110",		50,			NULL},
	{"00010101",		22,			NULL},
	{"00010100",		42,			NULL},
	{"00010011",		15,			NULL},
	{"00010010",		51,			NULL},
	{"00010001",		23,			NULL},
	{"00010000",		43,			NULL},
	{"00001111",		25,			NULL},
	{"00001110",		37,			NULL},
	{"00001101",		26,			NULL},
	{"00001100",		38,			NULL},
	{"00001011",		29,			NULL},
	{"00001010",		45,			NULL},
	{"00001001",		53,			NULL},
	{"00001000",		57,			NULL},
	{"00000111",		30,			NULL},
	{"00000110",		46,			NULL},
	{"00000101",		54,			NULL},
	{"00000100",		58,			NULL},
	{"000000111",		31,			NULL},
	{"000000110",		47,			NULL},
	{"000000101",		55,			NULL},
	{"000000100",		59,			NULL},
	{"000000011",		27,			NULL},
	{"000000010",		39,			NULL},
	{"000000001",		0,			"do not use with 4:2:0 chroma"}
};

static vlc_t motion_vectors[] = {
	{"00000011001",		-16,			NULL},
	{"00000011011",		-15,			NULL},
	{"00000011101",		-14,			NULL},
	{"00000011111",		-13,			NULL},
	{"00000100001",		-12,			NULL},
	{"00000100011",		-10,			NULL},
	{"0000010101",		-9,			NULL},
	{"0000010111",		-8,			NULL},
	{"00000111",		-7,			NULL},
	{"00001001",		-6,			NULL},
	{"00001011",		-5,			NULL},
	{"0000111",		-4,			NULL},
	{"00011",		-3,			NULL},
	{"0011",		-2,			NULL},
	{"011",			-1,			NULL},
	{"1",			0,			NULL},
	{"010",			1,			NULL},
	{"0010",		2,			NULL},
	{"00010",		3,			NULL},
	{"0000110",		4,			NULL},
	{"00001010",		5,			NULL},
	{"00001000",		6,			NULL},
	{"00000110",		7,			NULL},
	{"0000010110",		8,			NULL},
	{"0000010100",		9,			NULL},
	{"0000010010",		10,			NULL},
	{"00000100010",		11,			NULL},
	{"00000100000",		12,			NULL},
	{"00000011110",		13,			NULL},
	{"00000011100",		14,			NULL},
	{"00000011010",		15,			NULL},
	{"00000011000",		16,			NULL}
};

static vlc_t dmvector[] = {
	{"11",			-1,			NULL},
	{"0",			0,			NULL},
	{"10",			1,			NULL}
};

static vlc_t dct_dc_size_luma[] = {
	{"100",			0,			NULL},
	{"00",			1,			NULL},
	{"01",			2,			NULL},
	{"101",			3,			NULL},
	{"110",			4,			NULL},
	{"1110",		5,			NULL},
	{"11110",		6,			NULL},
	{"111110",		7,			NULL},
	{"1111110",		8,			NULL},
	{"11111110",		9,			NULL},
	{"111111110",		10,			NULL},
	{"111111111",		11,			NULL}
};

static vlc_t dct_dc_size_chroma[] = {
	{"00",			0,			NULL},
	{"01",			1,			NULL},
	{"10",			2,			NULL},
	{"110",			3,			NULL},
	{"1110",		4,			NULL},
	{"11110",		5,			NULL},
	{"111110",		6,			NULL},
	{"1111110",		7,			NULL},
	{"11111110",		8,			NULL},
	{"111111110",		9,			NULL},
	{"1111111110",		10,			NULL},
	{"1111111111",		11,			NULL}
};


#define	RLE(run,length)	(((run)<<8)|(uint8_t)(length))

static vlc_t dct_coeff_0[] = {
	{"1s",			RLE(-1, 1),		"run:0 level:1 for first (DC) coeff, or EOB"},
	{"11s",			RLE(0,	1),		"run:0 level:1 for all other coeffs"},
	{"011s",		RLE(1,	1),		NULL},
	{"0100s",		RLE(0,	2),		NULL},
	{"0101s",		RLE(2,	1),		NULL},
	{"00101s",		RLE(0,	3),		NULL},
	{"00111s",		RLE(3,	1),		NULL},
	{"00110s",		RLE(4,	1),		NULL},
	{"000110s",		RLE(1,	2),		NULL},
	{"000111s",		RLE(5,	1),		NULL},
	{"000101s",		RLE(6,	1),		NULL},
	{"000100s",		RLE(7,	1),		NULL},
	{"0000110s",		RLE(0,	4),		NULL},
	{"0000100s",		RLE(2,	2),		NULL},
	{"0000111s",		RLE(8,	1),		NULL},
	{"0000101s",		RLE(9,	1),		NULL},
	{"000001",		RLE(-2, 0),		"Escape"},
	{"00100110s",		RLE(0,	5),		NULL},
	{"00100001s",		RLE(0,	6),		NULL},
	{"00100101s",		RLE(1,	3),		NULL},
	{"00100100s",		RLE(3,	2),		NULL},
	{"00100111s",		RLE(10,	1),		NULL},
	{"00100011s",		RLE(11,	1),		NULL},
	{"00100010s",		RLE(12,	1),		NULL},
	{"00100000s",		RLE(13,	1),		NULL},
	{"0000001010s",		RLE(0,	7),		NULL},
	{"0000001100s",		RLE(1,	4),		NULL},
	{"0000001011s",		RLE(2,	3),		NULL},
	{"0000001111s",		RLE(4,	2),		NULL},
	{"0000001001s",		RLE(5,	2),		NULL},
	{"0000001110s",		RLE(14,	1),		NULL},
	{"0000001101s",		RLE(15,	1),		NULL},
	{"0000001000s",		RLE(16,	1),		NULL},
	{"000000011101s",	RLE(0,	8),		NULL},
	{"000000011000s",	RLE(0,	9),		NULL},
	{"000000010011s",	RLE(0,	10),		NULL},
	{"000000010000s",	RLE(0,	11),		NULL},
	{"000000011011s",	RLE(1,	5),		NULL},
	{"000000010100s",	RLE(2,	4),		NULL},
	{"000000011100s",	RLE(3,	3),		NULL},
	{"000000010010s",	RLE(4,	3),		NULL},
	{"000000011110s",	RLE(6,	2),		NULL},
	{"000000010101s",	RLE(7,	2),		NULL},
	{"000000010001s",	RLE(8,	2),		NULL},
	{"000000011111s",	RLE(17,	1),		NULL},
	{"000000011010s",	RLE(18,	1),		NULL},
	{"000000011001s",	RLE(19,	1),		NULL},
	{"000000010111s",	RLE(20,	1),		NULL},
	{"000000010110s",	RLE(21,	1),		NULL},
	{"0000000011010s",	RLE(0,	12),		NULL},
	{"0000000011001s",	RLE(0,	13),		NULL},
	{"0000000011000s",	RLE(0,	14),		NULL},
	{"0000000010111s",	RLE(0,	15),		NULL},
	{"0000000010110s",	RLE(1,	6),		NULL},
	{"0000000010101s",	RLE(1,	7),		NULL},
	{"0000000010100s",	RLE(2,	5),		NULL},
	{"0000000010011s",	RLE(3,	4),		NULL},
	{"0000000010010s",	RLE(5,	3),		NULL},
	{"0000000010001s",	RLE(9,	2),		NULL},
	{"0000000010000s",	RLE(10,	2),		NULL},
	{"0000000011111s",	RLE(22,	1),		NULL},
	{"0000000011110s",	RLE(23,	1),		NULL},
	{"0000000011101s",	RLE(24,	1),		NULL},
	{"0000000011100s",	RLE(25,	1),		NULL},
	{"0000000011011s",	RLE(26,	1),		NULL},
	{"00000000011111s",	RLE(0,	16),		NULL},
	{"00000000011110s",	RLE(0,	17),		NULL},
	{"00000000011101s",	RLE(0,	18),		NULL},
	{"00000000011100s",	RLE(0,	19),		NULL},
	{"00000000011011s",	RLE(0,	20),		NULL},
	{"00000000011010s",	RLE(0,	21),		NULL},
	{"00000000011001s",	RLE(0,	22),		NULL},
	{"00000000011000s",	RLE(0,	23),		NULL},
	{"00000000010111s",	RLE(0,	24),		NULL},
	{"00000000010110s",	RLE(0,	25),		NULL},
	{"00000000010101s",	RLE(0,	26),		NULL},
	{"00000000010100s",	RLE(0,	27),		NULL},
	{"00000000010011s",	RLE(0,	28),		NULL},
	{"00000000010010s",	RLE(0,	29),		NULL},
	{"00000000010001s",	RLE(0,	30),		NULL},
	{"00000000010000s",	RLE(0,	31),		NULL},
	{"000000000011000s",	RLE(0,	32),		NULL},
	{"000000000010111s",	RLE(0,	33),		NULL},
	{"000000000010110s",	RLE(0,	34),		NULL},
	{"000000000010101s",	RLE(0,	35),		NULL},
	{"000000000010100s",	RLE(0,	36),		NULL},
	{"000000000010011s",	RLE(0,	37),		NULL},
	{"000000000010010s",	RLE(0,	38),		NULL},
	{"000000000010001s",	RLE(0,	39),		NULL},
	{"000000000010000s",	RLE(0,	40),		NULL},
	{"000000000011111s",	RLE(1,	8),		NULL},
	{"000000000011110s",	RLE(1,	9),		NULL},
	{"000000000011101s",	RLE(1,	10),		NULL},
	{"000000000011100s",	RLE(1,	11),		NULL},
	{"000000000011011s",	RLE(1,	12),		NULL},
	{"000000000011010s",	RLE(1,	13),		NULL},
	{"000000000011001s",	RLE(1,	14),		NULL},
	{"0000000000010011s",	RLE(1,	15),		NULL},
	{"0000000000010010s",	RLE(1,	16),		NULL},
	{"0000000000010001s",	RLE(1,	17),		NULL},
	{"0000000000010000s",	RLE(1,	18),		NULL},
	{"0000000000010100s",	RLE(6,	3),		NULL},
	{"0000000000011010s",	RLE(11,	2),		NULL},
	{"0000000000011001s",	RLE(12,	2),		NULL},
	{"0000000000011000s",	RLE(13,	2),		NULL},
	{"0000000000010111s",	RLE(14,	2),		NULL},
	{"0000000000010110s",	RLE(15,	2),		NULL},
	{"0000000000010101s",	RLE(16,	2),		NULL},
	{"0000000000011111s",	RLE(27,	1),		NULL},
	{"0000000000011110s",	RLE(28,	1),		NULL},
	{"0000000000011101s",	RLE(29,	1),		NULL},
	{"0000000000011100s",	RLE(30,	1),		NULL},
	{"0000000000011011s",	RLE(31,	1),		NULL}
};

static vlc_t dct_coeff_1[] = {
	{"0110",		RLE(-1, 0),		"EOB"},
	{"10s",			RLE(0,	1),		NULL},
	{"010s",		RLE(1,	1),		NULL},
	{"110s",		RLE(0,	2),		NULL},
	{"00101s",		RLE(2,	1),		NULL},
	{"0111s",		RLE(0,	3),		NULL},
	{"00111s",		RLE(3,	1),		NULL},
	{"000110s",		RLE(4,	1),		NULL},
	{"00110s",		RLE(1,	2),		NULL},
	{"000111s",		RLE(5,	1),		NULL},
	{"0000110s",		RLE(6,	1),		NULL},
	{"0000100s",		RLE(7,	1),		NULL},
	{"11100s",		RLE(0,	4),		NULL},
	{"0000111s",		RLE(2,	2),		NULL},
	{"0000101s",		RLE(8,	1),		NULL},
	{"1111000s",		RLE(9,	1),		NULL},
	{"000001",		RLE(-2, 0),		"Escape"},
	{"11101s",		RLE(0,	5),		NULL},
	{"000101s",		RLE(0,	6),		NULL},
	{"1111001s",		RLE(1,	3),		NULL},
	{"00100110s",		RLE(3,	2),		NULL},
	{"1111010s",		RLE(10,	1),		NULL},
	{"00100001s",		RLE(11,	1),		NULL},
	{"00100101s",		RLE(12,	1),		NULL},
	{"00100100s",		RLE(13,	1),		NULL},
	{"000100s",		RLE(0,	7),		NULL},
	{"00100111s",		RLE(1,	4),		NULL},
	{"11111100s",		RLE(2,	3),		NULL},
	{"11111101s",		RLE(4,	2),		NULL},
	{"000000100s",		RLE(5,	2),		NULL},
	{"000000101s",		RLE(14,	1),		NULL},
	{"000000111s",		RLE(15,	1),		NULL},
	{"0000001101s",		RLE(16,	1),		NULL},
	{"1111011s",		RLE(0,	8),		NULL},
	{"1111100s",		RLE(0,	9),		NULL},
	{"00100011s",		RLE(0,	10),		NULL},
	{"00100010s",		RLE(0,	11),		NULL},
	{"00100000s",		RLE(1,	5),		NULL},
	{"0000001100s",		RLE(2,	4),		NULL},
	{"000000011100s",	RLE(3,	3),		NULL},
	{"000000010010s",	RLE(4,	3),		NULL},
	{"000000011110s",	RLE(6,	2),		NULL},
	{"000000010101s",	RLE(7,	2),		NULL},
	{"000000010001s",	RLE(8,	2),		NULL},
	{"000000011111s",	RLE(17,	1),		NULL},
	{"000000011010s",	RLE(18,	1),		NULL},
	{"000000011001s",	RLE(19,	1),		NULL},
	{"000000010111s",	RLE(20,	1),		NULL},
	{"000000010110s",	RLE(21,	1),		NULL},
	{"11111010s",		RLE(0,	12),		NULL},
	{"11111011s",		RLE(0,	13),		NULL},
	{"11111110s",		RLE(0,	14),		NULL},
	{"11111111s",		RLE(0,	15),		NULL},
	{"0000000010110s",	RLE(1,	6),		NULL},
	{"0000000010101s",	RLE(1,	7),		NULL},
	{"0000000010100s",	RLE(2,	5),		NULL},
	{"0000000010011s",	RLE(3,	4),		NULL},
	{"0000000010010s",	RLE(5,	3),		NULL},
	{"0000000010001s",	RLE(9,	2),		NULL},
	{"0000000010000s",	RLE(10,	2),		NULL},
	{"0000000011111s",	RLE(22,	1),		NULL},
	{"0000000011110s",	RLE(23,	1),		NULL},
	{"0000000011101s",	RLE(24,	1),		NULL},
	{"0000000011100s",	RLE(25,	1),		NULL},
	{"0000000011011s",	RLE(26,	1),		NULL},
	{"00000000011111s",	RLE(0,	16),		NULL},
	{"00000000011110s",	RLE(0,	17),		NULL},
	{"00000000011101s",	RLE(0,	18),		NULL},
	{"00000000011100s",	RLE(0,	19),		NULL},
	{"00000000011011s",	RLE(0,	20),		NULL},
	{"00000000011010s",	RLE(0,	21),		NULL},
	{"00000000011001s",	RLE(0,	22),		NULL},
	{"00000000011000s",	RLE(0,	23),		NULL},
	{"00000000010111s",	RLE(0,	24),		NULL},
	{"00000000010110s",	RLE(0,	25),		NULL},
	{"00000000010101s",	RLE(0,	26),		NULL},
	{"00000000010100s",	RLE(0,	27),		NULL},
	{"00000000010011s",	RLE(0,	28),		NULL},
	{"00000000010010s",	RLE(0,	29),		NULL},
	{"00000000010001s",	RLE(0,	30),		NULL},
	{"00000000010000s",	RLE(0,	31),		NULL},
	{"000000000011000s",	RLE(0,	32),		NULL},
	{"000000000010111s",	RLE(0,	33),		NULL},
	{"000000000010110s",	RLE(0,	34),		NULL},
	{"000000000010101s",	RLE(0,	35),		NULL},
	{"000000000010100s",	RLE(0,	36),		NULL},
	{"000000000010011s",	RLE(0,	37),		NULL},
	{"000000000010010s",	RLE(0,	38),		NULL},
	{"000000000010001s",	RLE(0,	39),		NULL},
	{"000000000010000s",	RLE(0,	40),		NULL},
	{"000000000011111s",	RLE(1,	8),		NULL},
	{"000000000011110s",	RLE(1,	9),		NULL},
	{"000000000011101s",	RLE(1,	10),		NULL},
	{"000000000011100s",	RLE(1,	11),		NULL},
	{"000000000011011s",	RLE(1,	12),		NULL},
	{"000000000011010s",	RLE(1,	13),		NULL},
	{"000000000011001s",	RLE(1,	14),		NULL},
	{"0000000000010011s",	RLE(1,	15),		NULL},
	{"0000000000010010s",	RLE(1,	16),		NULL},
	{"0000000000010001s",	RLE(1,	17),		NULL},
	{"0000000000010000s",	RLE(1,	18),		NULL},
	{"0000000000010100s",	RLE(6,	3),		NULL},
	{"0000000000011010s",	RLE(11,	2),		NULL},
	{"0000000000011001s",	RLE(12,	2),		NULL},
	{"0000000000011000s",	RLE(13,	2),		NULL},
	{"0000000000010111s",	RLE(14,	2),		NULL},
	{"0000000000010110s",	RLE(15,	2),		NULL},
	{"0000000000010101s",	RLE(16,	2),		NULL},
	{"0000000000011111s",	RLE(27,	1),		NULL},
	{"0000000000011110s",	RLE(28,	1),		NULL},
	{"0000000000011101s",	RLE(29,	1),		NULL},
	{"0000000000011100s",	RLE(30,	1),		NULL},
	{"0000000000011011s",	RLE(31,	1),		NULL}
};	

typedef enum {
	integer,
	flags,
	eob,
	escape,
	run_level
}	format_t;

typedef struct {
	int zeroes;
	int digits;
	int code;
	int value;
	format_t format;
	char comment[80];
}	vtbl_t;

vtbl_t *vtbl;
int vcnt;
int vmax;

int zeroes(const char *src)
{
	int z;

	for (z = 0; z < strlen(src); z++)
		if (src[z] != '0')
			break;
	return z;
}

int binval(const char *src, int s)
{
	int val = 0;

	while (*src) {
		val = val << 1;
		if (*src == 's') 
			val |= s;
		else if (*src == '1')
			val |= 1;
		src++;
	}
	return val;
}

int compare_vtbl(const void *e1, const void *e2)
{
	FUN("compare_vtbl");
	const vtbl_t *v1 = (const vtbl_t *)e1;
	const vtbl_t *v2 = (const vtbl_t *)e2;

	if (v1->zeroes != v2->zeroes)
		return v1->zeroes - v2->zeroes;
	if (v1->digits != v2->digits)
		return v1->digits - v2->digits;
	if (v1->format != v2->format)
		return v1->format - v2->format;
	if (v1->code == v2->code) {
		fprintf(stderr, "fatal: %s\nduplicate code zeroes %d, digits %d, code %d (%#x)\n",
			v1->comment, v1->zeroes, v1->digits, v1->code, v1->code);
		exit(1);
	}
	/* sort bigger codes first */
	return v2->code - v1->code;
}

void add_table(int zeroes, int digits, int code, int value, int format, const char *fmt, ...)
{
	FUN("add_table");
	va_list ap;

	if (vcnt >= vmax) {
		vmax = vmax ? vmax * 2 : 128;
		vtbl = xrealloc(vtbl, vmax * sizeof(vtbl_t));
		if (!vtbl) {
			perror("xrealloc()");
			exit(1);
		}
	}

	vtbl[vcnt].zeroes = zeroes;
	vtbl[vcnt].digits = digits;
	vtbl[vcnt].code = code;
	vtbl[vcnt].value = value;
	vtbl[vcnt].format = format;

	va_start(ap, fmt);
	vsnprintf(vtbl[vcnt].comment, sizeof(vtbl[vcnt].comment), fmt, ap);
	va_end(ap);

	vcnt++;
}

int vlc_table(const char *comment, const char *comment2,
	const char *name, vlc_t *vlc, int size, format_t format)
{
	FUN("vlc_table");
	int i, j, z, d, c, v, b;
	int max_zeroes;

	vcnt = 0;
	for (i = 0, vcnt = 0;  i < size; i++) {
		vlc_t *v = &vlc[i];
		int z = zeroes(v->vlc);
		int d = strlen(v->vlc);
		int val = v->value;
		int code;

		if (v->vlc[d-1] == 's') {
			char temp[32];
			snprintf(temp, sizeof(temp), "%s", v->vlc);
			/* with sign bit 0 */
			temp[d - 1] = '0';
			code = binval(v->vlc, 0);
			add_table(z, d - z, code, val, format,
				"/* %3d: %-18s %-36s */",
				i, temp, v->comment ? v->comment : "positive");

			/* with sign bit 1 */
			temp[d - 1] = '1';
			code = binval(v->vlc, 1);
			add_table(z, d - z, code, (val&~255)|(-val&255),  format,
				"/* %3d: %-18s %-36s */",
				i, temp, v->comment ? v->comment : "negative");
		} else {
			code = binval(v->vlc, 0);
			add_table(z, d - z, code, val,
				format != run_level ? format : val == -1 ? eob : escape,
				"/* %3d: %-18s %-36s */",
				i, v->vlc, v->comment ? v->comment : "");
		}
	}
	qsort(vtbl, vcnt, sizeof(vtbl_t), compare_vtbl);

#if	VLC_TABLES
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief %s\n", comment);
	fprintf(fc," * %s\n", comment2);
	fprintf(fc," */\n");
	fprintf(fc,"static const vlc_entry_t %s[%d] = {\n", name, vcnt);
	for (i = 0, max_zeroes = 0; i < vcnt; i++) {
		z = vtbl[i].zeroes;
		d = vtbl[i].digits;
		c = vtbl[i].code;
		v = vtbl[i].value;
		switch (vtbl[i].format) {
		case integer:
			fprintf(fc,"	{%2d,%2d,%3d,%16d},	%s\n",
				z, d, c, v, vtbl[i].comment);
			break;
		case flags:
			fprintf(fc,"	{%2d,%2d,%3d,MBF(%c,%c,%c,%c,%c,%c,%c)},	%s\n",
				z, d, c,
				((v >> 8) & 1) + '0',
				((v >> 7) & 1) + '0',
				((v >> 6) & 1) + '0',
				((v >> 5) & 1) + '0',
				((v >> 4) & 1) + '0',
				((v >> 3) & 1) + '0',
				((v >> 0) & 7) + '0',
				vtbl[i].comment);
			break;
		case eob:
		case escape:
		case run_level:
			fprintf(fc,"	{%2d,%2d,%3d,  RLE(%4d,%+4d)},	%s\n",
				z, d, c, v >> 8, (int8_t)v,
				vtbl[i].comment);
			break;
		}
		if (z > max_zeroes)
			max_zeroes = z;
	}
	fprintf(fc,"};\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
#else
	/* just find the max. number of leading zeroes */
	for (i = 0, max_zeroes = 0; i < vcnt; i++) {
		z = vtbl[i].zeroes;
		if (z > max_zeroes)
			max_zeroes = z;
	}
#endif
	switch (format) {
	case integer:
		fprintf(fh,"/** @brief read VLC for %s */\n", comment);
		fprintf(fh,"extern int vlc_read_%s(vlc_t *vlc, int *value);\n", name);
		fprintf(fh,"\n");
		fprintf(fc,"/**\n");
		fprintf(fc," * @brief read VLC for %s\n", comment);
		fprintf(fc," * %s\n", comment2);
		fprintf(fc," * @param vlc pointer to a variable length code context\n");
		fprintf(fc," * @param value pointer to an int to receive the value\n");
		fprintf(fc," * @result 0 on sucess, -1 on error\n");
		fprintf(fc," */\n");
		fprintf(fc,"int vlc_read_%s(vlc_t *vlc, int *value)\n", name);
		break;
	case flags:
		fprintf(fh,"/** @brief read VLC for %s */\n", comment);
		fprintf(fh,"extern int vlc_read_%s(vlc_t *vlc, int *flags);\n", name);
		fprintf(fh,"\n");
		fprintf(fc,"/**\n");
		fprintf(fc," * @brief read VLC for %s\n", comment);
		fprintf(fc," * %s\n", comment2);
		fprintf(fc," * @param vlc pointer to a variable length code context\n");
		fprintf(fc," * @param flags pointer to an and array of chars to receive the flags\n");
		fprintf(fc," * @result 0 on sucess, -1 on error\n");
		fprintf(fc," */\n");
		fprintf(fc,"int vlc_read_%s(vlc_t *vlc, int *flags)\n", name);
		break;
	case run_level:
		fprintf(fh,"/** @brief read VLC for %s */\n", comment);
		fprintf(fh,"extern int vlc_read_%s(vlc_t *vlc, int *run, int *level);\n", name);
		fprintf(fh,"\n");
		fprintf(fc,"/**\n");
		fprintf(fc," * @brief read VLC for %s\n", comment);
		fprintf(fc," * %s\n", comment2);
		fprintf(fc," * @param vlc pointer to a variable length code context\n");
		fprintf(fc," * @param run pointer to an int to receive the run\n");
		fprintf(fc," * @param level pointer to an int to receive the level\n");
		fprintf(fc," * @result 0 on sucess, -1 on error\n");
		fprintf(fc," */\n");
		fprintf(fc,"int vlc_read_%s(vlc_t *vlc, int *run, int *level)\n", name);
		break;
	default:
		fprintf(stderr, "wrong format in call to vlc_table()\n");
		exit(1);
	}
	fprintf(fc,"{\n");
	fprintf(fc,"	FUN(\"vlc_read_%s\");\n", name);
	fprintf(fc,"	register bstream_t *bs;\n");
	fprintf(fc,"	register uint32_t accu;\n");
	fprintf(fc,"	register int8_t bno;\n");
	fprintf(fc,"	register int8_t zeroes;\n");
	fprintf(fc,"	register int8_t digits;\n");
	fprintf(fc,"	register int8_t shift;\n");
	fprintf(fc,"	register int code;\n");
	fprintf(fc,"\n");
	fprintf(fc,"	if (!VLC_VALID(vlc)) {\n");
	fprintf(fc,"		LOG(ERROR,(_fun,\"invalid vlc_t* %%p\\n\", vlc));\n");
	fprintf(fc,"		errno = EINVAL;\n");
	fprintf(fc,"		return -1;\n");
	fprintf(fc,"	}\n");
	fprintf(fc,"	bs = &vlc->bs;\n");
	fprintf(fc,"	if (!BSTREAM_VALID(bs)) {\n");
	fprintf(fc,"		LOG(ERROR,(_fun,\"invalid bstream_t* %%p\\n\", bs));\n");
	fprintf(fc,"		errno = EINVAL;\n");
	fprintf(fc,"		return -1;\n");
	fprintf(fc,"	}\n");
	fprintf(fc,"\n");
	fprintf(fc,"	/* first check if we need to advance pbuf */\n");
	fprintf(fc,"	if (bs->bno == -1) {\n");
	fprintf(fc,"		bs->bno = 7;\n");
	fprintf(fc,"		bs->pbuf++;\n");
	fprintf(fc,"	}\n");
	fprintf(fc,"	/* bit position in most significant byte */\n");
	fprintf(fc,"	bno = 24 + bs->bno;\n");
	fprintf(fc,"	/* get the next 24 bits, pad with eight 1s */\n");
	fprintf(fc,"	accu = (bs->pbuf[0] << 24) | (bs->pbuf[1] << 16) | (bs->pbuf[2] << 8) | 0xff;\n");
	fprintf(fc,"	/* mask between 17 to 24 bits in the most significant bits */\n");
	fprintf(fc,"	accu &= mmask[bno + 1];\n");
	fprintf(fc,"	/* count leading zeroes, starting from bno */\n");
	fprintf(fc,"	zeroes = lzc32(accu, bno + 1);\n");
	fprintf(fc,"#if	DEBUG\n");
	fprintf(fc,"	vlc->zeroes = zeroes;\n");
	fprintf(fc,"#endif\n");
	fprintf(fc,"\n");

	fprintf(fc,"	switch (zeroes) {\n");
	/* generate the cases for this table */
	for (i = 0, z = -1; i < vcnt; i++) {
		if (z == vtbl[i].zeroes)
			continue;
		z = vtbl[i].zeroes;
		fprintf(fc,"	case %d:\n", z);
		d = vtbl[i].digits;
		if (d > 0) {
			fprintf(fc,"		/* get %d digit%s from accu */\n",
				d, d > 1 ? "s" : "");
			fprintf(fc,"		shift = bno + 1 - %d - %d;\n", z, d);
			fprintf(fc,"		code = (accu >> shift) & %#x;\n", (1 << d) - 1);
		} else {
			fprintf(fc,"		/* get only leading zeroes from accu */\n");
			fprintf(fc,"		shift = bno + 1 - %d;\n", z);
			fprintf(fc,"		code = accu >> shift;\n");
		}

		/* generate comparisons for this case */
		for (j = 0; j < vcnt; j++) {
			if (z != vtbl[j].zeroes)
				continue;
			if (vtbl[j].digits > d) {
				/* get more bits from the accu */
				b = vtbl[j].digits - d;
				fprintf(fc,"		/* get %d more digit%s into code */\n",
					b, b > 1 ? "s" : "");
				fprintf(fc,"		shift -= %d;\n", b);
				fprintf(fc,"		code = (code << %d) | ((accu >> shift) & %#x);\n",
					b, (1 << b) - 1);
				d += b;
			}
			c = vtbl[j].code;
			v = vtbl[j].value;
			fprintf(fc,"		%s\n", vtbl[j].comment);
			fprintf(fc,"		if (code == %d) {\n", c);
			switch (vtbl[i].format) {
			case integer:
				fprintf(fc,"#if	DEBUG\n");
				fprintf(fc,"			vlc->digits = %d;\n", d);
				fprintf(fc,"			vlc->code = %d;\n", c);
				fprintf(fc,"			vlc->value = %d;\n", v);
				fprintf(fc,"#endif\n");
				fprintf(fc,"			*value = %d;\n", v);
				fprintf(fc,"			return bs_seek(bs, %d);\n", z + d);
				fprintf(fc,"		}\n");
				break;
			case flags:
				fprintf(fc,"#if	DEBUG\n");
				fprintf(fc,"			vlc->digits = %d;\n", d);
				fprintf(fc,"			vlc->code = %d;\n", c);
				fprintf(fc,"			vlc->value = %d;\n", v);
				fprintf(fc,"#endif\n");
				fprintf(fc,"			*flags = MBF(%c,%c,%c,%c,%c,%c,%c);\n",
				((v >> 8) & 1) + '0',
				((v >> 7) & 1) + '0',
				((v >> 6) & 1) + '0',
				((v >> 5) & 1) + '0',
				((v >> 4) & 1) + '0',
				((v >> 3) & 1) + '0',
				((v >> 0) & 7) + '0');
				fprintf(fc,"			return bs_seek(bs, %d);\n", z + d);
				fprintf(fc,"		}\n");
				break;
			case eob:
				fprintf(fc,"			*run = -1;\n");
				fprintf(fc,"			*level = 0;\n");
				fprintf(fc,"#if	DEBUG\n");
				fprintf(fc,"			vlc->digits = %d;\n", d);
				fprintf(fc,"			vlc->code = %d;\n", c);
				fprintf(fc,"			vlc->value = %d;\n", v);
				fprintf(fc,"#endif\n");
				fprintf(fc,"			return bs_seek(bs, %d);\n", z + d);
				fprintf(fc,"		}\n");
				break;
			case escape:
				fprintf(fc,"			*run = -2;\n");
				fprintf(fc,"			*level = 0;\n");
				fprintf(fc,"#if	DEBUG\n");
				fprintf(fc,"			vlc->digits = %d;\n", d);
				fprintf(fc,"			vlc->code = %d;\n", c);
				fprintf(fc,"			vlc->value = %d;\n", v);
				fprintf(fc,"#endif\n");
				fprintf(fc,"			return bs_seek(bs, %d);\n", z + d);
				fprintf(fc,"		}\n");
				break;
			case run_level:
				fprintf(fc,"			*run = %d;\n",
					vtbl[j].value >> 8);
				fprintf(fc,"			*level = %+d;\n",
					(int8_t)vtbl[j].value);
				fprintf(fc,"#if	DEBUG\n");
				fprintf(fc,"			vlc->digits = %d;\n", d);
				fprintf(fc,"			vlc->code = %d;\n", c);
				fprintf(fc,"			vlc->value = %d;\n", vtbl[j].value);
				fprintf(fc,"#endif\n");
				fprintf(fc,"			return bs_seek(bs, %d);\n", z + d);
				fprintf(fc,"		}\n");
			}
		}
		fprintf(fc,"		/* invalid variable length code */\n");
		fprintf(fc,"		digits = %d;\n", d);
		fprintf(fc,"		break;\n");
	}
	fprintf(fc,"	default:\n");
	fprintf(fc,"#if	DEBUG\n");
	fprintf(fc,"		vlc->digits = -1;\n");
	fprintf(fc,"		vlc->code = -1;\n");
	fprintf(fc,"		vlc->value = -1;\n");
	fprintf(fc,"#endif\n");
	fprintf(fc,"		/* invalid variable length code */\n");
	fprintf(fc,"#if	DEBUG\n");
	fprintf(fc,"		LOG(ERROR,(_fun,\"invalid leading zeroes %%d, max %d\\n\",\n",
		max_zeroes);
	fprintf(fc,"			zeroes));\n");
	fprintf(fc,"		LOG(ERROR,(_fun,\"accu %%08x, bno %%d\\n\",\n");
	fprintf(fc,"			accu, bno));\n");
	fprintf(fc,"		bs_dump(bs);\n");
	fprintf(fc,"#endif\n");
	fprintf(fc,"		return -1;\n");
	fprintf(fc,"	}\n");
	fprintf(fc,"	/* invalid variable length code */\n");
	fprintf(fc,"#if	DEBUG\n");
	fprintf(fc,"	vlc->digits = digits;\n");
	fprintf(fc,"	vlc->code = code;\n");
	fprintf(fc,"	vlc->value = -1;\n");
	fprintf(fc,"	LOG(ERROR,(_fun,\"invalid code %%d, zeroes %%d, digits %%d\\n\",\n");
	fprintf(fc,"		code, zeroes, digits));\n");
	fprintf(fc,"	LOG(ERROR,(_fun,\"accu %%08x, bno %%d\\n\",\n");
	fprintf(fc,"		accu, bno));\n");
	fprintf(fc,"	bs_dump(bs);\n");
	fprintf(fc,"#endif\n");
	fprintf(fc,"	return -1;\n");
	fprintf(fc,"}\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
	switch (format) {
	case integer:
		fprintf(fh,"/** @brief write VLC for %s */\n", comment);
		fprintf(fh,"extern int vlc_write_%s(vlc_t *vlc, int value);\n", name);
		fprintf(fh,"\n");
		fprintf(fc,"/**\n");
		fprintf(fc," * @brief write VLC for %s\n", comment);
		fprintf(fc," * %s\n", comment2);
		fprintf(fc," * @param vlc pointer to a variable length code context\n");
		fprintf(fc," * @param value pointer to an int to receive the value\n");
		fprintf(fc," * @result 0 on sucess, -1 on error\n");
		fprintf(fc," */\n");
		fprintf(fc,"int vlc_write_%s(vlc_t *vlc, int value)\n", name);
		break;
	case flags:
		fprintf(fh,"/** @brief write VLC for %s */\n", comment);
		fprintf(fh,"extern int vlc_write_%s(vlc_t *vlc, int flags);\n", name);
		fprintf(fh,"\n");
		fprintf(fc,"/**\n");
		fprintf(fc," * @brief write VLC for %s\n", comment);
		fprintf(fc," * %s\n", comment2);
		fprintf(fc," * @param vlc pointer to a variable length code context\n");
		fprintf(fc," * @param flags int containing the flags, possibly encoded using MBF()\n");
		fprintf(fc," * @result 0 on sucess, -1 on error\n");
		fprintf(fc," */\n");
		fprintf(fc,"int vlc_write_%s(vlc_t *vlc, int flags)\n", name);
		break;
	case run_level:
		fprintf(fh,"/** @brief write VLC for %s */\n", comment);
		fprintf(fh,"extern int vlc_write_%s(vlc_t *vlc, int run, int level);\n", name);
		fprintf(fh,"\n");
		fprintf(fc,"/**\n");
		fprintf(fc," * @brief write VLC for %s\n", comment);
		fprintf(fc," * %s\n", comment2);
		fprintf(fc," * @param vlc pointer to a variable length code context\n");
		fprintf(fc," * @param run the run of this DCT VLC\n");
		fprintf(fc," * @param level the level of this DCT VLC\n");
		fprintf(fc," * @result 0 on sucess, -1 on error\n");
		fprintf(fc," */\n");
		fprintf(fc,"int vlc_write_%s(vlc_t *vlc, int run, int level)\n", name);
		break;
	default:
		fprintf(stderr, "wrong format in call to vlc_table()\n");
		exit(1);
	}
	fprintf(fc,"{\n");
	fprintf(fc,"	FUN(\"vlc_write_%s\");\n", name);
	fprintf(fc,"	bstream_t *bs;\n");
	fprintf(fc,"\n");
	fprintf(fc,"	if (!VLC_VALID(vlc)) {\n");
	fprintf(fc,"		LOG(ERROR,(_fun,\"invalid vlc_t* %%p\\n\", vlc));\n");
	fprintf(fc,"		errno = EINVAL;\n");
	fprintf(fc,"		return -1;\n");
	fprintf(fc,"	}\n");
	fprintf(fc,"	bs = &vlc->bs;\n");
	fprintf(fc,"	if (!BSTREAM_VALID(bs)) {\n");
	fprintf(fc,"		LOG(ERROR,(_fun,\"invalid bstream_t* %%p\\n\", bs));\n");
	fprintf(fc,"		errno = EINVAL;\n");
	fprintf(fc,"		return -1;\n");
	fprintf(fc,"	}\n");
	fprintf(fc,"\n");

	switch (format) {
	case integer:
		fprintf(fc,"	switch (value) {\n");
		break;
	case flags:
		fprintf(fc,"	switch (flags) {\n");
		break;
	case run_level:
		fprintf(fc,"	switch (RLE(run,level)) {\n");
		break;
	default:
		fprintf(stderr, "wrong format in call to vlc_table()\n");
		exit(1);
	}
	/* generate the cases for this table */
	for (i = 0, z = -1; i < vcnt; i++) {
		z = vtbl[i].zeroes;
		d = vtbl[i].digits;
		c = vtbl[i].code;
		v = vtbl[i].value;
		switch (format) {
		case integer:
			fprintf(fc,"	case %d:\n", v);
			break;
		case flags:
			fprintf(fc,"	case MBF(%c,%c,%c,%c,%c,%c,%c):\n",
				((v >> 8) & 1) + '0',
				((v >> 7) & 1) + '0',
				((v >> 6) & 1) + '0',
				((v >> 5) & 1) + '0',
				((v >> 4) & 1) + '0',
				((v >> 3) & 1) + '0',
				((v >> 0) & 7) + '0');
			break;
		case eob:
		case escape:
		case run_level:
			fprintf(fc,"	case RLE(%d,%d):\n",
				v >> 8, (int8_t)v);
			break;
		}
		fprintf(fc,"		%s\n", vtbl[i].comment);
		fprintf(fc,"#if	DEBUG\n");
		fprintf(fc,"		vlc->zeroes = %d;\n", z);
		fprintf(fc,"		vlc->digits = %d;\n", d);
		fprintf(fc,"		vlc->code = %d;\n", c);
		fprintf(fc,"		vlc->value = %d;\n", v);
		fprintf(fc,"#endif\n");
		fprintf(fc,"		return bs_bits_w(bs, %d, %d);\n", c, z + d);
	}
	fprintf(fc,"	}\n");
	switch (format) {
	case integer:
		fprintf(fc,"	/* impossible to encode value as VLC */\n");
		fprintf(fc,"	LOG(ERROR,(_fun,\"invalid value %%d (%%#x)\\n\",\n");
		fprintf(fc,"		value, value));\n");
		fprintf(fc,"#if	DEBUG\n");
		fprintf(fc,"	vlc->zeroes = -1;\n");
		fprintf(fc,"	vlc->digits = -1;\n");
		fprintf(fc,"	vlc->code = -1;\n");
		fprintf(fc,"	vlc->value = value;\n");
		fprintf(fc,"#endif\n");
		break;
	case flags:
		fprintf(fc,"	/* impossible to encode flags as VLC */\n");
		fprintf(fc,"	LOG(ERROR,(_fun,\"invalid flags %%d (%%#x)\\n\",\n");
		fprintf(fc,"		flags, flags));\n");
		fprintf(fc,"#if	DEBUG\n");
		fprintf(fc,"	vlc->zeroes = -1;\n");
		fprintf(fc,"	vlc->digits = -1;\n");
		fprintf(fc,"	vlc->code = -1;\n");
		fprintf(fc,"	vlc->value = flags;\n");
		fprintf(fc,"#endif\n");
		break;
	case run_level:
		fprintf(fc,"	/* impossible to encode run,level as VLC */\n");
		fprintf(fc,"	LOG(ERROR,(_fun,\"invalid run/level %%d/%%d (%%#x/%%#x)\\n\",\n");
		fprintf(fc,"		run, level, run, level));\n");
		fprintf(fc,"#if	DEBUG\n");
		fprintf(fc,"	vlc->zeroes = -1;\n");
		fprintf(fc,"	vlc->digits = -1;\n");
		fprintf(fc,"	vlc->code = -1;\n");
		fprintf(fc,"	vlc->value = RLE(run,level);\n");
		fprintf(fc,"#endif\n");
		break;
	default:
		fprintf(stderr, "wrong format in call to vlc_table()\n");
		exit(1);
	}
	fprintf(fc,"	return -1;\n");
	fprintf(fc,"}\n");
	fprintf(fc,"\n");

	return 0;
}

int main(int argc, char **argv)
{
	fh = fopen("vlcnew.h", "w");
	fc = fopen("vlcnew.c", "w");

	fprintf(fh,"/************************************************************************\n");
	fprintf(fh," *\n");
	fprintf(fh," * %s\n", project);
	fprintf(fh," *\n");
	fprintf(fh," * %s\n", copyright);
	fprintf(fh," *\n");
	fprintf(fh," * %s\n", warning);
	fprintf(fh," *\n");
	fprintf(fh," ************************************************************************/\n");
	fprintf(fh,"#if !defined(_vlc_h_included_)\n");
	fprintf(fh,"#define	_vlc_h_included_\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"#include \"config.h\"\n");
	fprintf(fh,"#include \"bstream.h\"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/** @brief macroblock type flags */\n");
	fprintf(fh,"typedef struct {\n");
	fprintf(fh,"	/** @brief macroblock quantizer */\n");
	fprintf(fh,"	char mb_quant;\n");
	fprintf(fh,"	/** @brief macro block forward motion */\n");
	fprintf(fh,"	char mb_motion_forward;\n");
	fprintf(fh,"	/** @brief macroblock backward motion */\n");
	fprintf(fh,"	char mb_motion_backward;\n");
	fprintf(fh,"	/** @brief macroblock pattern */\n");
	fprintf(fh,"	char macroblock_pattern;\n");
	fprintf(fh,"	/** @brief macroblock intra matrix */\n");
	fprintf(fh,"	char mb_intra;\n");
	fprintf(fh,"	/** @brief macroblock spatial temporal weight code flag */\n");
	fprintf(fh,"	char mb_stwc_flag;\n");
	fprintf(fh,"	/** @brief macroblock spatial temporal weight classes */\n");
	fprintf(fh,"	char mb_stwc_classes;\n");
	fprintf(fh,"}	mb_type_t;\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief encode macroblock flags as bits\n");
	fprintf(fh," *  q macroblock_quant (0 or 1)\n");
	fprintf(fh," *  f macroblock_motion_forward (0 or 1)\n");
	fprintf(fh," *  b macroblock_motion_backward (0 or 1)\n");
	fprintf(fh," *  p macroblock_pattern (0 or 1)\n");
	fprintf(fh," *  i macroblock_intra (0 or 1)\n");
	fprintf(fh," *  s spatial_temporal_weight_code_flag (0 or 1)\n");
	fprintf(fh," *  c permitted spatial_temporal_weight classes (1=0; 2=1,2,3; 4=4)\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	MBF(c,q,f,b,p,i,s) \\\n");
	fprintf(fh,"	(((q)<<8)|((f)<<7)|((b)<<6)|((p)<<5)|((i)<<4)|((s)<<3)|(c))\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief pack an mb_type_t entry into a flags integer\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	macroblock_type_pACK(flag,mbt) do { \\\n");
	fprintf(fh,"	flag = MBF( \\\n");
	fprintf(fh,"		(mbt)->mb_quant, \\\n");
	fprintf(fh,"		(mbt)->mb_motion_forward, \\\n");
	fprintf(fh,"		(mbt)->mb_motion_backward, \\\n");
	fprintf(fh,"		(mbt)->macroblock_pattern, \\\n");
	fprintf(fh,"		(mbt)->mb_intra, \\\n");
	fprintf(fh,"		(mbt)->mb_stwc_flag, \\\n");
	fprintf(fh,"		(mbt)->mb_stwc_classes); \\\n");
	fprintf(fh,"} while (0)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get macroblock_quant from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	MB_QUANT(n)	(((n)>>8)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get macroblock_forward from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	MB_FORWARD(n)	(((n)>>7)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get macroblock_backward from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	MB_BACK(n)	(((n)>>6)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get macroblock_pattern from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	MB_PATTERN(n)	(((n)>>5)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get macroblock_intra from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	MB_INTRA(n)	(((n)>>4)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get spatial_temporal_weight_code_flag from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	STWCF(n)	(((n)>>3)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get spatial_temporal_weight_class from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	STWC(n)		(((n)>>0)&7)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get spatial_temporal_weight_class 4 permitted from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	STWC4(n)	(((n)>>2)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get spatial_temporal_weight_classes 1,2,3 permitted from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	STWC123(n)	(((n)>>1)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief get spatial_temporal_weight_class 0 permitted from flags value\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	STWC0(n)	(((n)>>0)&1)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/**\n");
	fprintf(fh," * @brief unpack a flags integer into an mb_type_t entry\n");
	fprintf(fh," */\n");
	fprintf(fh,"#define	MBTYPE_UNPACK(mbt,flag) do { \\\n");
	fprintf(fh,"	(mbt)->mb_quant = MB_QUANT(flag); \\\n");
	fprintf(fh,"	(mbt)->mb_motion_forward = MB_FORWARD(flag); \\\n");
	fprintf(fh,"	(mbt)->mb_motion_backward = MB_BACKWARD(flag); \\\n");
	fprintf(fh,"	(mbt)->macroblock_pattern = MB_PATTERN(flag); \\\n");
	fprintf(fh,"	(mbt)->mb_intra = MB_INTRA(flag); \\\n");
	fprintf(fh,"	(mbt)->mb_stwc_flag = STWCF(flag); \\\n");
	fprintf(fh,"	(mbt)->mb_stwc_classes = STWC(flag); \\\n");
	fprintf(fh,"} while (0)\n");
	fprintf(fh,"\n");
	fprintf(fh,"\n");
	fprintf(fh,"/** @brief non-zero if a vlc is non-NULL and contains the vlc tag */\n");
	fprintf(fh,"#define	VLC_VALID(vlc) CMPTAG(vlc,VLC_TAG)\n");
	fprintf(fh,"\n");
	fprintf(fh,"/** @brief variable length code context */\n");
	fprintf(fh,"typedef struct {\n");
	fprintf(fh,"	/** @brief tag to verify validity of a vlc_t pointer */\n");
	fprintf(fh,"	tag_t tag;\n");
	fprintf(fh,"\n");
	fprintf(fh,"	/** @brief bit stream */\n");
	fprintf(fh,"	bstream_t bs;\n");
	fprintf(fh,"\n");
	fprintf(fh,"#if	DEBUG\n");
	fprintf(fh,"	/** @brief only for debugging: last number of leading zeroes */\n");
	fprintf(fh,"	int zeroes;\n");
	fprintf(fh,"	/** @brief only for debugging: last number of digits */\n");
	fprintf(fh,"	int digits;\n");
	fprintf(fh,"	/** @brief only for debugging: last VLC */\n");
	fprintf(fh,"	int code;\n");
	fprintf(fh,"	/** @brief only for debugging: last value (flags, run/level ...) */\n");
	fprintf(fh,"	int value;\n");
	fprintf(fh,"#endif\n");
	fprintf(fh,"}	vlc_t;\n");
	fprintf(fh,"\n");

	fprintf(fc,"/************************************************************************\n");
	fprintf(fc," *\n");
	fprintf(fc," * %s\n", project);
	fprintf(fc," *\n");
	fprintf(fc," * %s\n", copyright);
	fprintf(fc," *\n");
	fprintf(fc," * %s\n", warning);
	fprintf(fc," *\n");
	fprintf(fc," ************************************************************************/\n");
	fprintf(fc,"#include <stdio.h>\n");
	fprintf(fc,"#include <stdlib.h>\n");
	fprintf(fc,"#include <string.h>\n");
	fprintf(fc,"#include <errno.h>\n");
	fprintf(fc,"#include \"debug.h\"\n");
	fprintf(fc,"#include \"vlcnew.h\"\n");
	fprintf(fc,"\n");
#if	VLC_TABLES
	fprintf(fc,"/** @brief variable length code table entry (with leading zeroes) */\n");
	fprintf(fc,"typedef struct {\n");
	fprintf(fc,"	/** @brief number of leading zeroes */\n");
	fprintf(fc,"	int zeroes;\n");
	fprintf(fc,"	/** @brief number of digits */\n");
	fprintf(fc,"	int digits;\n");
	fprintf(fc,"	/** @brief code of the VLC */\n");
	fprintf(fc,"	int code;\n");
	fprintf(fc,"	/** @brief value of the VLC */\n");
	fprintf(fc,"	int value;\n");
	fprintf(fc,"}	vlc_entry_t;\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
#endif
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief encode run length and level into one integer\n");
	fprintf(fc," *  r run\n");
	fprintf(fc," *  l level\n");
	fprintf(fc," */\n");
	fprintf(fc,"#define	RLE(r,l)	(((r)<<8)|(uint8_t)(l))\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief get run length from table value\n");
	fprintf(fc," */\n");
	fprintf(fc,"#define	RUN(n)		(((n)>>8)\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief get signed run level from table value\n");
	fprintf(fc," */\n");
	fprintf(fc,"#define	LEVEL(n)	((int8_t)n)\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief special code EOB (end of block), also used for first (DC) coeff run\n");
	fprintf(fc," */\n");
	fprintf(fc,"#define	EOB		-1\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief special code ESCAPE\n");
	fprintf(fc," */\n");
	fprintf(fc,"#define	ESCAPE		-2\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
	fprintf(fc,"/** @brief bit masks for the rightmost 0 to 32 bits */\n");
	fprintf(fc,"static const uint32_t mmask[33] = {\n");
	fprintf(fc,"	0x00000000, 0x00000001, 0x00000003, 0x00000007,\n");
	fprintf(fc,"	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,\n");
	fprintf(fc,"	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,\n");
	fprintf(fc,"	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,\n");
	fprintf(fc,"	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,\n");
	fprintf(fc,"	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,\n");
	fprintf(fc,"	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,\n");
	fprintf(fc,"	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,\n");
	fprintf(fc,"	0xffffffff\n");
	fprintf(fc,"};\n");
	fprintf(fc,"\n");
	fprintf(fc,"\n");
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief number of one bits in a word\n");
	fprintf(fc," *\n");
	fprintf(fc," * 32-bit recursive reduction using SWAR...\n");
	fprintf(fc," * but first step is mapping 2-bit values\n");
	fprintf(fc," * into sum of 2 1-bit values in sneaky way\n");
	fprintf(fc," */\n");
	fprintf(fc,"static __inline int8_t ones32(uint32_t x)\n");
	fprintf(fc,"{\n");
	fprintf(fc,"        x -= ((x >> 1) & 0x55555555);\n");
	fprintf(fc,"        x = ((x >> 2) & 0x33333333) + (x & 0x33333333);\n");
	fprintf(fc,"        x = ((x >> 4) + x) & 0x0f0f0f0f;\n");
	fprintf(fc,"        x += (x >> 8);\n");
	fprintf(fc,"        x += (x >> 16);\n");
	fprintf(fc,"\n");
	fprintf(fc,"        return (int8_t)(x & 0x3f);\n");
	fprintf(fc,"}\n");
	fprintf(fc,"\n");
	fprintf(fc,"/**\n");
	fprintf(fc," * @brief number of leading zeroes in a 32 bit word\n");
	fprintf(fc," */\n");
	fprintf(fc,"static __inline int8_t lzc32(uint32_t x, int8_t first)\n");
	fprintf(fc,"{\n");
	fprintf(fc,"	x |= (x >> 1);\n");
	fprintf(fc,"	x |= (x >> 2);\n");
	fprintf(fc,"	x |= (x >> 4);\n");
	fprintf(fc,"	x |= (x >> 8);\n");
	fprintf(fc,"	x |= (x >> 16);\n");
	fprintf(fc,"	return (int8_t)(first - ones32(x));\n");
	fprintf(fc,"}\n");
	fprintf(fc,"\n");

	vlc_table("macroblock address increment",
		"according to ISO-18318-2 Table B-1",
		"macroblock_address_increment",
		macroblock_address_increment,
		sizeof(macroblock_address_increment)/S,
		integer);

	vlc_table("macroblock type in I-pictures",
		"according to ISO-18318-2 Table B-2",
		"macroblock_type_i",
		macroblock_type_i,
		sizeof(macroblock_type_i)/S,
		flags);

	vlc_table("macroblock type in P-pictures",
		"according to ISO-18318-2 Table B-3",
		"macroblock_type_p",
		macroblock_type_p,
		sizeof(macroblock_type_p)/S,
		flags);

	vlc_table("macroblock type in B-pictures",
		"according to ISO-18318-2 Table B-4",
		"macroblock_type_b",
		macroblock_type_b,
		sizeof(macroblock_type_b)/S,
		flags);

	vlc_table("macroblock type in I-pictures with spatial scalability",
		"according to ISO-18318-2 Table B-5",
		"macroblock_type_is",
		macroblock_type_is,
		sizeof(macroblock_type_is)/S,
		flags);

	vlc_table("macroblock type in P-pictures with spatial scalability",
		"according to ISO-18318-2 Table B-6",
		"macroblock_type_ps",
		macroblock_type_ps,
		sizeof(macroblock_type_ps)/S,
		flags);

	vlc_table("macroblock type in B-pictures with spatial scalability",
		"according to ISO-18318-2 Table B-7",
		"macroblock_type_bs",
		macroblock_type_bs,
		sizeof(macroblock_type_bs)/S,
		flags);

	vlc_table("macroblock type in I-, P-, B-pictures with SNR scalability",
		"according to ISO-18318-2 Table B-8",
		"macroblock_type_snr",
		macroblock_type_snr,
		sizeof(macroblock_type_snr)/S,
		flags);

	vlc_table("macroblock pattern - coded block pattern",
		"according to ISO-18318-2 Table B-9",
		"macroblock_pattern",
		macroblock_pattern,
		sizeof(macroblock_pattern)/S,
		integer);

	vlc_table("motion vectors - motion code",
		"according to ISO-18318-2 Table B-10",
		"motion_vectors",
		motion_vectors,
		sizeof(motion_vectors)/S,
		integer);

	vlc_table("motion vectors - DM vector",
		"according to ISO-18318-2 Table B-11",
		"dmvector",
		dmvector,
		sizeof(dmvector)/S,
		integer);

	vlc_table("DCT coefficients - DCT DC size luminance",
		"according to ISO-18318-2 Table B-12",
		"dct_dc_size_luma",
		dct_dc_size_luma,
		sizeof(dct_dc_size_luma)/S,
		integer);

	vlc_table("DCT coefficients - DCT DC size chrominance",
		"according to ISO-18318-2 Table B-13",
		"dct_dc_size_chroma",
		dct_dc_size_chroma,
		sizeof(dct_dc_size_chroma)/S,
		integer);

	vlc_table("DCT coefficients - table zero",
		"according to ISO-18318-2 Table B-14",
		"dct_coeff_0",
		dct_coeff_0,
		sizeof(dct_coeff_0)/S,
		run_level);

	vlc_table("DCT coefficients - table one",
		"according to ISO-18318-2 Table B-15",
		"dct_coeff_1",
		dct_coeff_1,
		sizeof(dct_coeff_1)/S,
		run_level);

	fclose(fc);

	fprintf(fh,"\n");
	fprintf(fh,"#endif	/* !defined(_vlc_h_included_) */\n");
	fprintf(fh,"\n");
	fclose(fh);

	return 0;
}
