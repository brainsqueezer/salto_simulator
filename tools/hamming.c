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
 * Hamming code testing
 *
 * $Id: hamming.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#define	RAM_SIZE	65536

/*
 * Some macros to have easier access to the bit-reversed notation
 * that the Xerox Alto docs use all over the place.
 */
#define BITSHIFT(width,to) ((width)-1-(to))

#define BITMASK(width,from,to) (((1<<((to)+1-(from)))-1) << BITSHIFT(width,to))

#define	ALTO_EQU(reg,width,from,to,val) \
	(((reg) & BITMASK(width,from,to)) == \
		(((val) << BITSHIFT(width,to)) & BITMASK(width,from,to)))

#define	ALTO_GET(reg,width,from,to) \
	(((reg) & BITMASK(width,from,to)) >> BITSHIFT(width,to))

#define	ALTO_PUT(reg,width,from,to,val) do { \
	(reg) = ((reg) & ~BITMASK(width,from,to)) | \
		(((val) << BITSHIFT(width,to)) & BITMASK(width,from,to)); \
} while (0)

#define	CPU_CLR_TASK_WAKEUP(x)
#define	CPU_SET_TASK_WAKEUP(x)

#define	GET_MESR_HAMMING(mesr)		ALTO_GET(mesr,16,0,5)
#define	PUT_MESR_HAMMING(mesr,val)	ALTO_PUT(mesr,16,0,5,val)
#define	GET_MESR_PERR(mesr)		ALTO_GET(mesr,16,6,6)
#define	PUT_MESR_PERR(mesr,val)		ALTO_PUT(mesr,16,6,6,val)
#define	GET_MESR_PARITY(mesr)		ALTO_GET(mesr,16,7,7)
#define	PUT_MESR_PARITY(mesr,val)	ALTO_PUT(mesr,16,7,7,val)
#define	GET_MESR_SYNDROME(mesr)		ALTO_GET(mesr,16,8,13)
#define	PUT_MESR_SYNDROME(mesr,val)	ALTO_PUT(mesr,16,8,13,val)
#define	GET_MESR_BANK(mesr)		ALTO_GET(mesr,16,14,15)
#define	PUT_MESR_BANK(mesr,val)		ALTO_PUT(mesr,16,14,15,val)

#define	GET_MECR_SPARE1(mecr,val)	ALTO_GET(mecr,16,0,3)
#define	PUT_MECR_SPARE1(mecr,val)	ALTO_PUT(mecr,16,0,3,val)
#define	GET_MECR_TEST_CODE(mecr)	ALTO_GET(mecr,16,4,10)
#define	PUT_MECR_TEST_CODE(mecr,val)	ALTO_PUT(mecr,16,4,10,val)
#define	GET_MECR_TEST_MODE(mecr)	ALTO_GET(mecr,16,11,11)
#define	PUT_MECR_TEST_MODE(mecr,val)	ALTO_PUT(mecr,16,11,11,val)
#define	GET_MECR_INT_SBERR(mecr)	ALTO_GET(mecr,16,12,12)
#define	PUT_MECR_INT_SBERR(mecr,val)	ALTO_PUT(mecr,16,12,12,val)
#define	GET_MECR_INT_DBERR(mecr)	ALTO_GET(mecr,16,13,13)
#define	PUT_MECR_INT_DBERR(mecr,val)	ALTO_PUT(mecr,16,13,13,val)
#define	GET_MECR_ERRCORR(mecr)		ALTO_GET(mecr,16,14,14)
#define	PUT_MECR_ERRCORR(mecr,val)	ALTO_PUT(mecr,16,14,14,val)
#define	GET_MECR_SPARE2(mecr)		ALTO_GET(mecr,16,15,15)
#define	PUT_MECR_SPARE2(mecr,val)	ALTO_PUT(mecr,16,15,15,val)

#define	LOG(x)	logprintf x

typedef struct {
	uint32_t ram[RAM_SIZE/2];	/* RAM double-words */
	uint32_t hpb[RAM_SIZE/2];	/* Hamming code and Parity check bits */
	uint32_t mar;
	uint32_t mear;
	uint32_t mesr;
	uint32_t mecr;
	int error;
}	mem_t;


mem_t mem;

static void logprintf(int task, int level, const char *fmt, ...)
{
	va_list ap;

	if (level > 0)
		return;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

/**
 *
 * AltoII Memory
 *
 * Address mapping
 *
 * The mapping of addresses to memory chips can be altered by the setting of
 * the "memory configuration switch". This switch is located at the top of the
 * backplane of the AltoII. If the switch is in the alternate position, the
 * first and second 32K portions of memory are exchanged.
 *
 * The AltoII memory system is organized around 32-bit doublewords. Stored
 * along with each doubleword is 6 bits of Hamming code and a Parity bit for
 * a total of 39 bits:
 *
 *	bits 0-15	even data word
 *	bits 16-31	odd data word
 *	bits 32-37	Hamming code
 *	bit 38		Parity bit
 *
 * Things are further complicated by the fact that two types of memory chips
 * are used: 16K chips in machines with extended memory and 4K chips for all
 * others.
 *
 * The bits in a 1-word deep slice of memory are called a group. A group
 * contains 4K oder 16K doublewords, depending on the chip type. The bits of
 * a group on a single board are called a subgroup. Thus a subgroup contains
 * 10 of the 40 bits in a group. There are 8 subgroups on a memory board.
 * Subgroups are numberer from the high 3 bits of the address; for 4K chips
 * this means MAR[0-2]; for 16K chips (i.e., an Alto with extended memory)
 * this means BANK,MAR[0]:
 *
 *	Subgroup	Chip Positions
 *	   7		  81-90
 *	   6		  71-80
 *	   5		  61-70
 *	   4		  51-60
 *	   3		  41-50
 *	   2		  31-40
 *	   1		  21-30
 *	   0		  11-20
 *
 * The location of the bits in group 0 is:
 *
 *	CARD 1		CARD2		CARD3		CARD4
 *	32 24 16 08 00	33 25 17 09 01	34 26 18 10 02	35 27 19 11 03
 *	36 28 20 12 04	37 29 21 13 05	38 30 22 14 06	xx 31 23 25 07
 *
 * Chips 15, 25, 35, 45, 65, 75 and 85 on board 4 aren't used. If you are out
 * of replacement memory chips, you can use one of these, but then the board
 * with the missing chips will only work in Slot 4.
 *
 *	o  WORD = 16 BITS
 *	o  ACCESS -> 2 WORDS AT A TIME
 *	o  -> 32 BITS + 6 BITS EC + PARITY + SPARE = 40 BITS
 *	o  10 BITS/MODULE    80 DRAMS/MODULE
 *	o  4 MODULES/ALTO   320 DRAMS/ALTO
 *
 *	ADDRESS A0-6, WE, CAS'
 *		| TO ALL DEVICES
 *		v
 *	+-----------------------------------------+
 *	| ^ 8 DEVICES (32K OR 128K FOR XM)        |
 *	| |                                       | CARD 1
 *     /| v  <------------ DATA OUT ---------->   |
 *    / |  0   1   2   3   4   5   6   7   8   9  |
 *   /	+-----------------------------------------+
 *  |	   H4  H0  28  24  20  16  12  8   4   0
 *  |
 *  |	+-----------------------------------------+
 *  |  /|                                         | CARD 2
 *  | /	+-----------------------------------------+
 * RAS     H5  H1  29  25  21  17  13  9   5   1
 * 0-7
 *  | \	+-----------------------------------------+
 *  |  \|                                         | CARD 3
 *  |	+-----------------------------------------+
 *  |	   P   H2  30  26  22  18  14  10  6   2
 *   \
 *    \	+-----------------------------------------+
 *     \|                                         | CARD 4
 *	+-----------------------------------------+
 *	   X   H3  31  27  23  19  15  11  7   3
 *
 *	           [  ODD WORD  ]  [ EVEN WORD ]
 *
 *
 * ------------------------------------------------------------------------------
 *
 *		32K x 10 STORAGE MODULE
 *
 * 			Table I
 *
 * 	+-------+-------+-------+---------------+-------+
 *	|CIRCUIT| INPUT | SIGNAL| INVERTER	|	|
 *	|  NO.  | PINS  | NAME  | DEF?? ???	|RESIST.|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|   71	| RAS0	| A1  1 ->  2	| ?? R2 |
 *	|   1	+-------+-------+---------------+-------+
 *	|	|  110	| CS0	| A1  3 ->  4	| ?? R3	|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|   79	| RAS1	| A2  1 ->  2	| ?? R4	|
 *	|   2	+-------+-------+---------------+-------+
 *	|	|  110	| CS1	| A2  3 ->  4	| ?? R5	|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|   90	| RAS2	| A3  1 ->  2	| ?? R7	|
 *	|   3	+-------+-------+---------------+-------+
 *	|	|  110	| CS2	| A3  3 ->  4	| ?? R8	|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|   86	| RAS3	| A3 11 -> 10	| ?? R9	|
 *	|   4	+-------+-------+---------------+-------+
 *	|	|  110	| CS3	| A4 11 -> 10	| ?? R7	|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|  102	| RAS4	| A4  1 ->  2	| ?? R4	|
 *	|   5	+-------+-------+---------------+-------+
 *	|	|  110	| CS4	| A3 13 -> 12	| ?? R5	|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|  106	| RAS5	| A5 11 -> 10	| ?? R3	|
 *	|   6	+-------+-------+---------------+-------+
 *	|	|  110	| CS5	| A5  3 ->  4	| ?? R2	|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|  111	| RAS6	| A5  1 ->  2	| ?? R8	|
 *	|   7	+-------+-------+---------------+-------+
 *	|	|  110	| CS6	| A5 13 -> 12	| ?? R9	|
 * 	+-------+-------+-------+---------------+-------+
 *	|	|   99	| RAS7	| A4 13 -> 12	| ?? R5	|
 *	|   8	+-------+-------+---------------+-------+
 *	|	|  110	| CS7	| A4  3 ->  4	| ?? R5	|
 * 	+-------+-------+-------+---------------+-------+
 *
 * 			Table II
 *
 * 		MEMORY CHIP REFERENCE DESIGNATOR
 *
 *			CIRCUIT NO.
 *	 ROW NO.    1	    2	    3	    4	    5	    6	    7	    8
 *	+-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *      |   1	| 15 20	| 25 30	| 35 40	| 45 50	| 55 60	| 65 70	| 75 80	| 85 90	|
 *      +-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *      |   2	| 14 19	| 24 29	| 34 39	| 44 49	| 54 59	| 64 69	| 64 79	| 84 89	|
 *      +-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *      |   3	| 13 18	| 23 28	| 33 38	| 43 48	| 53 58	| 63 68	| 73 78	| 83 88	|
 *      +-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *      |   4	| 12 17	| 22 27	| 32 37	| 42 47	| 52 57	| 62 67	| 72 77	| 82 87	|
 *      +-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *      |   5	| 11 16	| 21 26	| 31 36	| 41 46	| 52 56	| 61 66	| 71 76	| 81 86	|
 *      +-------+-------+-------+-------+-------+-------+-------+-------+-------+
 *
 *
 * The Hamming code generator:
 *
 * WDxx is write data bit xx.
 * H(x) is Hammming code bit x.
 * HC(x) is generated Hamming code bit x.
 * HC(x/y) is an intermediate value.
 * HC(x)A and HC(x)B are also intermediate values.
 *
 * Chips used are:
 * 74S280 9-bit parity generator (A-I inputs, even and odd outputs)
 * 74S135 EX-OR/EX-NOR gates (5 inputs, 2 outputs)
 * 74S86  EX-OR gates (2 inputs, 1 output)
 *
 * chip A      B      C      D      E      F      G      H      I     even    odd
 * ---------------------------------------------------------------------------------
 * a75: WD01   WD04   WD08   WD11   WD15   WD19   WD23   WD26   WD30  ---     HC(0)A
 * a76: WD00   WD03   WD06   WD10   WD13   WD17   WD21   WD25   WD28  HC(0B1) ---
 * a86: WD02   WD05   WD09   WD12   WD16   WD20   WD24   WD27   WD31  HC(1)A  ---
 * a64: WD01   WD02   WD03   WD07   WD08   WD09   WD10   WD14   WD15  ---     HC(2)A
 * a85: WD16   WD17   WD22   WD23   WD24   WD25   WD29   WD30   WD31  HC(2)B  ---
 *
 * H(0)   ^ HC(0)A  ^ HC(0B1) -> HC(0)
 * H(1)   ^ HC(1)A  ^ HC(0B1) -> HC(1)
 * HC(2)A ^ HC(2)B  ^ H(2)    -> HC(2)
 * H(0)   ^ H(1)    ^ H(2)    -> H(0/2)
 *
 * chip A      B      C      D      E      F      G      H      I     even    odd
 * ---------------------------------------------------------------------------------
 * a66: WD04   WD05   WD06   WD07   WD08   WD09   WD10   H(3)   0     ---     HC(3)A
 * a84: WD18   WD19   WD20   WD21   WD22   WD23   WD24   WD25   0     HC(3/4) HCPA
 * a63: WD11   WD12   WD13   WD14   WD15   WD16   WD17   H(4)   0     ---     HC(4)A
 * a87: WD26   WD27   WD28   WD29   WD30   WD31   H(5)   0      0     HC(5)   HCPB
 *
 * HC(3)A ^ HC(3/4) -> HC(3)
 * HC(4)A ^ HC(3/4) -> HC(4)
 *
 * WD00 ^ WD01 -> XX01
 *
 * chip A      B      C      D      E      F      G      H      I     even    odd
 * ---------------------------------------------------------------------------------
 * a54: HC(3)A HC(4)A HCPA   HCPB   H(0/2) XX01   WD02   WD03   RP    PERR    ---
 * a65: WD00   WD01   WD02   WD04   WD05   WD07   WD10   WD11   WD12  ---     PCA
 * a74: WD14   WD17   WD18   WD21   WD23   WD24   WD26   WD27   WD29  PCB     ---
 *
 * PCA ^ PCB -> PC
 *
 * Whoa ;-)
 *
 */

#define	WD(x) (1ul<<(31-x))

/* a75: WD01   WD04   WD08   WD11   WD15   WD19   WD23   WD26   WD30 */
#define	A75 (WD( 1)|WD( 4)|WD( 8)|WD(11)|WD(15)|WD(19)|WD(23)|WD(26)|WD(30))

/* a76: WD00   WD03   WD06   WD10   WD13   WD17   WD21   WD25   WD29 */
#define	A76 (WD( 0)|WD( 3)|WD( 6)|WD(10)|WD(13)|WD(17)|WD(21)|WD(25)|WD(28))

/* a86: WD02   WD05   WD09   WD12   WD16   WD20   WD24   WD27   WD31 */
#define	A86 (WD( 2)|WD( 5)|WD( 9)|WD(12)|WD(16)|WD(20)|WD(24)|WD(27)|WD(31))

/* a64: WD01   WD02   WD03   WD07   WD08   WD09   WD10   WD14   WD15 */
#define	A64 (WD( 1)|WD( 2)|WD( 3)|WD( 7)|WD( 8)|WD( 9)|WD(10)|WD(14)|WD(15))

/* a85: WD16   WD17   WD22   WD23   WD24   WD25   WD29   WD30   WD31 */
#define	A85 (WD(16)|WD(17)|WD(22)|WD(23)|WD(24)|WD(25)|WD(29)|WD(30)|WD(31))

/* a66: WD04   WD05   WD06   WD07   WD08   WD09   WD10   H(3)   0    */
#define	A66 (WD( 4)|WD( 5)|WD( 6)|WD( 7)|WD( 8)|WD( 9)|WD(10))

/* a84: WD18   WD19   WD20   WD21   WD22   WD23   WD24   WD25   0    */
#define	A84 (WD(18)|WD(19)|WD(20)|WD(21)|WD(22)|WD(23)|WD(24)|WD(25))

/* a63: WD11   WD12   WD13   WD14   WD15   WD16   WD17   H(4)   0    */
#define	A63 (WD(11)|WD(12)|WD(13)|WD(14)|WD(15)|WD(16)|WD(17))

/* a87: WD26   WD27   WD28   WD29   WD30   WD31   H(5)   0      0    */
#define	A87 (WD(26)|WD(27)|WD(28)|WD(29)|WD(30)|WD(31))

/* a54: HC(3)A HC(4)A HCPA   HCPB   H(0/2) XX01   WD02   WD03   P    */
#define	A54 (WD( 2)|WD( 3))

/* a65: WD00   WD01   WD02   WD04   WD05   WD07   WD10   WD11   WD12 */
#define	A65 (WD( 0)|WD( 1)|WD( 2)|WD( 4)|WD( 5)|WD( 7)|WD(10)|WD(11)|WD(12))

/* a74: WD14   WD17   WD18   WD21   WD23   WD24   WD26   WD27   WD29 */
#define	A74 (WD(14)|WD(17)|WD(18)|WD(21)|WD(23)|WD(24)|WD(26)|WD(27)|WD(29))

/** @brief get Hamming code bit 0 from hpb data (really bit 32)*/
#define	H0(hpb) ALTO_GET(hpb,8,0,0)

/** @brief get Hamming code bit 1 from hpb data (really bit 33) */
#define	H1(hpb) ALTO_GET(hpb,8,1,1)

/** @brief get Hamming code bit 2 from hpb data (really bit 34) */
#define	H2(hpb) ALTO_GET(hpb,8,2,2)

/** @brief get Hamming code bit 3 from hpb data (really bit 35) */
#define	H3(hpb) ALTO_GET(hpb,8,3,3)

/** @brief get Hamming code bit 4 from hpb data (really bit 36) */
#define	H4(hpb) ALTO_GET(hpb,8,4,4)

/** @brief get Hamming code bit 5 from hpb data (really bit 37) */
#define	H5(hpb) ALTO_GET(hpb,8,5,5)

/** @brief get Hamming code from hpb data (bits 32 to 37) */
#define	RH(hpb) ALTO_GET(hpb,8,0,5)

/** @brief get parity bit from hpb data  (really bit 38) */
#define	RP(hpb) ALTO_GET(hpb,8,6,6)

/** @brief return even parity of a (masked) 32 bit value */
static __inline uint8_t parity_even(uint32_t val)
{
        val -= ((val >> 1) & 0x55555555);
        val = (((val >> 2) & 0x33333333) + (val & 0x33333333));
        val = (((val >> 4) + val) & 0x0f0f0f0f);
        val += (val >> 8);
        val += (val >> 16);
        return (val & 1);
}

/** @brief return odd parity of a (masked) 32 bit value */
#define	parity_odd(val) (parity_even(val)^1)

/**
 * @brief lookup table to convert a hamming syndrome into a bit number to correct
 */
static const int hamming_syndrome[64] = {
	-1, -1, -1,  0, -1,  1,  2,  3,	/* A69: HR(5):0 HR(4):0 HR(3):0 */
	-1,  4,  5,  6,  7,  8,  9, 10,	/* A79: HR(5):0 HR(4):0 HR(3):1 */
	-1, 11, 12, 13, 14, 15, 16, 17,	/* A67: HR(5):0 HR(4):1 HR(3):0 */
	-1, -1, -1, -1, -1,  1, -1, -1,	/* non chip selected */
	-1, 26, 27, 28, 29, 30, 31, -1,	/* A68: HR(5):1 HR(4):0 HR(3):0 */
	-1, -1, -1, -1, -1,  1, -1, -1,	/* non chip selected */
	18, 19, 20, 21, 22, 23, 24, 25,	/* A78: HR(5):1 HR(4):1 HR(3):0 */
	-1, -1, -1, -1, -1,  1, -1, -1	/* non chip selected */
}; 

/**
 * @brief Eat this, GCC!
 *
 * Hamming code generation according to the schematics
 * described above. It's certainly overkill to do this
 * on a moder PC, but I think we'll need it for perfect
 * emulation anyways (Hamming code hardware checking).
 *
 * @param dw_addr the double-word address
 * @param dw_data the double-word data to write
 */
static uint32_t hamming_code(int write, uint32_t dw_addr, uint32_t dw_data)
{
	/* NB: register - being an optimist always helps, or does it? */
	register uint8_t hpb = write ? 0 : mem.hpb[dw_addr];
	register uint8_t hc_0_a;
	register uint8_t hc_0b1;
	register uint8_t hc_1_a;
	register uint8_t hc_2_a;
	register uint8_t hc_2_b;
	register uint8_t hc_0;
	register uint8_t hc_1;
	register uint8_t hc_2;
	register uint8_t h_0_2;
	register uint8_t hc_3_a;
	register uint8_t hc_3_4;
	register uint8_t hcpa;
	register uint8_t hc_4_a;
	register uint8_t hc_3;
	register uint8_t hc_4;
	register uint8_t hc_5;
	register uint8_t hcpb;
	register uint8_t perr;
	register uint8_t pca;
	register uint8_t pcb;
	register uint8_t pc;

	/* a75: WD01   WD04   WD08   WD11   WD15   WD19   WD23   WD26   WD30 ---     HC(0)A */
	hc_0_a = parity_odd (dw_data & A75);
	/* a76: WD00   WD03   WD06   WD10   WD13   WD17   WD21   WD25   WD29 HC(0B1) ---    */
	hc_0b1 = parity_even(dw_data & A76);
	/* a86: WD02   WD05   WD09   WD12   WD16   WD20   WD24   WD27   WD31 HC(1)A  ---    */
	hc_1_a = parity_even(dw_data & A86);
	/* a64: WD01   WD02   WD03   WD07   WD08   WD09   WD10   WD14   WD15 ---     HC(2)A */
	hc_2_a = parity_odd (dw_data & A64);
	/* a85: WD16   WD17   WD22   WD23   WD24   WD25   WD29   WD30   WD31 HC(2)B  ---    */
	hc_2_b = parity_even(dw_data & A85);

	hc_0  = H0(hpb) ^ hc_0_a ^ hc_0b1;
	hc_1  = H1(hpb) ^ hc_1_a ^ hc_0b1;
	hc_2  = hc_2_a ^ hc_2_b ^ H2(hpb);
	h_0_2 = H0(hpb) ^ H1(hpb) ^ H2(hpb);

	/* a66: WD04   WD05   WD06   WD07   WD08   WD09   WD10   H(3)   0    ---     HC(3)A */
	hc_3_a = parity_odd ((dw_data & A66) ^ H3(hpb));
	/* a84: WD18   WD19   WD20   WD21   WD22   WD23   WD24   WD25   0    HC(3/4) HCPA   */
	hcpa   = parity_odd (dw_data & A84);
	hc_3_4 = hcpa ^ 1;
	/* a63: WD11   WD12   WD13   WD14   WD15   WD16   WD17   H(4)   0    ---     HC(4)A */
	hc_4_a = parity_odd ((dw_data & A63) ^ H4(hpb));
	/* a87: WD26   WD27   WD28   WD29   WD30   WD31   H(5)   0      0    HC(5)   HCPB   */
	hcpb   = parity_even((dw_data & A87) ^ H5(hpb));

	hc_3   = hc_3_a ^ hc_3_4;
	hc_4   = hc_4_a ^ hc_3_4;
	hc_5   = hcpb ^ 1;

	/*
	 * Note: here i XOR all the non dw_data inputs into bit 0,
	 * which has the same effect as spreading them over some bits
	 * and then counting them... I hope ;-)
	 */
	/* a54: HC(3)A HC(4)A HCPA   HCPB   H(0/2) XX01   WD02   WD03   P    PERR    ---    */
	perr   = parity_even(hc_3_a ^ hc_4_a ^ hcpa ^ hcpb ^ h_0_2 ^
		(ALTO_GET(dw_data,32,0,0) ^ ALTO_GET(dw_data,32,1,1)) ^
		(dw_data & A54) ^ RP(hpb));

	/* a65: WD00   WD01   WD02   WD04   WD05   WD07   WD10   WD11   WD12 ---     PCA    */
	pca    = parity_odd (dw_data & A65);
	/* a74: WD14   WD17   WD18   WD21   WD23   WD24   WD26   WD27   WD29 PCB     ---    */
	pcb    = parity_even(dw_data & A74);
	pc     = pca ^ pcb;

	if (write) {
		/* write data double-word */
		mem.ram[dw_addr] = dw_data;
		/* update the hamming code and parity bit store */
		mem.hpb[dw_addr] = ((hc_0<<7) | (hc_1<<6) | (hc_2<<5) |
			(hc_3<<4) | (hc_4<<3) |	(hc_5<<2) | (perr<<1));
		return dw_data;

	}
	/* A22 (74H30) check for error
	 * input	signal
	 * -------------------------
	 *     1	NER(06) = PERR'
	 *     4	NER(08) = HC(0)'
	 *     3	NER(09) = HC(1)'
	 *     2	NER(10) = HC(2)'
	 *     6	NER(11) = HC(3)'
	 *     5	NER(12) = HC(4)'
	 *     12	NER(13) = HC(5)'
	 *     11	1 (VPUL3)
	 */
	if (perr || hc_0 || hc_1 || hc_2 || hc_3 || hc_4 || hc_5) {
		int syn = (hc_0<<5)|(hc_1<<4)|(hc_2<<3)|(hc_3<<2)|(hc_4<<1)|(hc_5);

		/* latch data on the first error */
		if (!mem.error) {
			mem.error = 1;
			PUT_MESR_HAMMING(mem.mesr, RH(hpb));
			PUT_MESR_PERR(mem.mesr, perr);
			PUT_MESR_PARITY(mem.mesr, RP(hpb));
			PUT_MESR_SYNDROME(mem.mesr, syn);
			PUT_MESR_BANK(mem.mesr, (dw_addr >> 15));
			/* latch memory address register */
			mem.mear = mem.mar & 0177777;
			LOG((0,0,"	memory error at (even) address:%07o data:%011o check:%03o\n",
				dw_addr * 2, dw_data, hpb));
			LOG((0,0,"	MEAR: %06o\n", mem.mear));
			LOG((0,0,"	MESR: %06o\n", mem.mesr));
			LOG((0,0,"		Hamming code read    : %#o\n",
				GET_MESR_HAMMING(mem.mesr)));
			LOG((0,0,"		Parity error         : %o\n",
				GET_MESR_PERR(mem.mesr)));
			LOG((0,0,"		Memory parity bit    : %o\n",
				GET_MESR_PARITY(mem.mesr)));
			LOG((0,0,"		Hamming syndrome     : %#o (bit #%d)\n",
				GET_MESR_SYNDROME(mem.mesr),
				hamming_syndrome[GET_MESR_SYNDROME(mem.mesr)]));
			LOG((0,0,"		Memory bank          : %#o\n",
				GET_MESR_BANK(mem.mesr)));
			LOG((0,0,"	MECR: %06o\n", mem.mecr));
			LOG((0,0,"		Test Hamming code    : %#o\n",
				GET_MECR_TEST_CODE(mem.mecr)));
			LOG((0,0,"		Test mode            : %s\n",
				0 == GET_MECR_TEST_MODE(mem.mecr) ? "on" : "off"));
			LOG((0,0,"		INT on single-bit err: %s\n",
				0 == GET_MECR_INT_SBERR(mem.mecr) ? "on" : "off"));
			LOG((0,0,"		INT on double-bit err: %s\n",
				0 == GET_MECR_INT_DBERR(mem.mecr) ? "on" : "off"));
			LOG((0,0,"		Error correction     : %s\n",
				GET_MECR_ERRCORR(mem.mecr) ? "on" : "ff"));
		}
		if (-1 == hamming_syndrome[syn]) {
			/* double-bit error: wake task_part, if we're told so */
			if (0 == GET_MECR_INT_DBERR(mem.mecr))
				CPU_SET_TASK_WAKEUP(task_part);
		} else {
			/* single-bit error: wake task_part, if we're told so */
			if (0 == GET_MECR_INT_SBERR(mem.mecr))
				CPU_SET_TASK_WAKEUP(task_part);
			/* should we correct the single bit error ? */
			if (GET_MECR_ERRCORR(mem.mecr))
				dw_data ^= 1ul << hamming_syndrome[syn];
		}
	}

	return dw_data;
}

int main(int argc, char **argv)
{
	uint32_t i;

	memset(&mem, 0, sizeof(mem));
	mem.mear = 0177777;
	mem.mesr = 0177777;
	mem.mecr = 0177777;

	for (i = 0; i < RAM_SIZE; i += 2)
		hamming_code(1, i/2, (i/2) * 0x00010001);

	/* try reads */
	for (i = 0; i < RAM_SIZE; i += 2) {
		hamming_code(0, i/2, mem.ram[i/2]);
		if (mem.error)
			break;
	}

	return 0;
}
