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
 * Main memory and memory mapped I/O read and write
 *
 * $Id: memory.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alto.h"
#include "cpu.h"
#include "debug.h"
#include "timer.h"
#include "memory.h"

/** @brief the memory context */
mem_t mem;

/**
 * <PRE>
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
 * <HR>
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
 * </PRE>
 */
#if	HAMMING_CHECK

#define	WD(x) (1ul<<(31-x))

/* a75: WD01   WD04   WD08   WD11   WD15   WD19   WD23   WD26   WD30 ---     HC(0)A */
#define	A75 (WD( 1)|WD( 4)|WD( 8)|WD(11)|WD(15)|WD(19)|WD(23)|WD(26)|WD(30))

/* a76: WD00   WD03   WD06   WD10   WD13   WD17   WD21   WD25   WD29 HC(0B1) ---    */
#define	A76 (WD( 0)|WD( 3)|WD( 6)|WD(10)|WD(13)|WD(17)|WD(21)|WD(25)|WD(28))

/* a86: WD02   WD05   WD09   WD12   WD16   WD20   WD24   WD27   WD31 HC(1)A  ---    */
#define	A86 (WD( 2)|WD( 5)|WD( 9)|WD(12)|WD(16)|WD(20)|WD(24)|WD(27)|WD(31))

/* a64: WD01   WD02   WD03   WD07   WD08   WD09   WD10   WD14   WD15 ---     HC(2)A */
#define	A64 (WD( 1)|WD( 2)|WD( 3)|WD( 7)|WD( 8)|WD( 9)|WD(10)|WD(14)|WD(15))

/* a85: WD16   WD17   WD22   WD23   WD24   WD25   WD29   WD30   WD31 HC(2)B  ---    */
#define	A85 (WD(16)|WD(17)|WD(22)|WD(23)|WD(24)|WD(25)|WD(29)|WD(30)|WD(31))

/* a66: WD04   WD05   WD06   WD07   WD08   WD09   WD10   H(3)   0    ---     HC(3)A */
#define	A66 (WD( 4)|WD( 5)|WD( 6)|WD( 7)|WD( 8)|WD( 9)|WD(10))

/* a84: WD18   WD19   WD20   WD21   WD22   WD23   WD24   WD25   0    HC(3/4) HCPA   */
#define	A84 (WD(18)|WD(19)|WD(20)|WD(21)|WD(22)|WD(23)|WD(24)|WD(25))

/* a63: WD11   WD12   WD13   WD14   WD15   WD16   WD17   H(4)   0    ---     HC(4)A */
#define	A63 (WD(11)|WD(12)|WD(13)|WD(14)|WD(15)|WD(16)|WD(17))

/* a87: WD26   WD27   WD28   WD29   WD30   WD31   H(5)   0      0    HC(5)   HCPB   */
#define	A87 (WD(26)|WD(27)|WD(28)|WD(29)|WD(30)|WD(31))

/* a54: HC(3)A HC(4)A HCPA   HCPB   H(0/2) XX01   WD02   WD03   P    PERR    ---    */
#define	A54 (WD( 2)|WD( 3))

/* a65: WD00   WD01   WD02   WD04   WD05   WD07   WD10   WD11   WD12 ---     PCA    */
#define	A65 (WD( 0)|WD( 1)|WD( 2)|WD( 4)|WD( 5)|WD( 7)|WD(10)|WD(11)|WD(12))

/* a74: WD14   WD17   WD18   WD21   WD23   WD24   WD26   WD27   WD29 PCB     ---    */
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
static const int hamming_lut[64] = {
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
 * @brief read or write a memory double-word and caluclate its Hamming code
 *
 * Hamming code generation according to the schematics described above.
 * It's certainly overkill to do this on a moder PC, but I think we'll
 * need it for perfect emulation anyways (Hamming code hardware checking).
 *
 * @param write non-zero if this is a memory write (don't check for error)
 * @param dw_addr the double-word address
 * @param dw_data the double-word data to write
 */
uint32_t hamming_code(int write, uint32_t dw_addr, uint32_t dw_data)
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
	register int syndrome;

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
	hcpb   = parity_odd ((dw_data & A87) ^ H5(hpb));
	hc_3   = hc_3_a ^ hc_3_4;
	hc_4   = hc_4_a ^ hc_3_4;
	hc_5   = hcpb ^ 1;

	syndrome = (hc_0<<5)|(hc_1<<4)|(hc_2<<3)|(hc_3<<2)|(hc_4<<1)|(hc_5);

	/*
	 * Note: here i XOR all the non dw_data inputs into bit 0,
	 * which has the same effect as spreading them over some bits
	 * and then counting them... I hope ;-)
	 */
	/* a54: HC(3)A HC(4)A HCPA   HCPB   H(0/2) XX01   WD02   WD03   P    PERR    ---    */
	perr   = parity_even(hc_3_a ^ hc_4_a ^ hcpa ^ hcpb ^ h_0_2 ^
		(ALTO_GET(dw_data,32,0,0) ^ ALTO_GET(dw_data,32,1,1)) ^
		(dw_data & A54) ^ RP(hpb) ^ 1);

	/* a65: WD00   WD01   WD02   WD04   WD05   WD07   WD10   WD11   WD12 ---     PCA    */
	pca    = parity_odd (dw_data & A65);
	/* a74: WD14   WD17   WD18   WD21   WD23   WD24   WD26   WD27   WD29 PCB     ---    */
	pcb    = parity_even(dw_data & A74);
	pc     = pca ^ pcb;

	if (write) {
		/* update the hamming code and parity bit store */
		mem.hpb[dw_addr] = (syndrome << 2) | (pc << 1);
		return dw_data;

	}
	/**
	 * <PRE>
	 * A22 (74H30) 8-input NAND to check for error
	 * 	input	signal
	 * 	-------------------------
	 *	1	POK = PERR'
	 *	4	NER(08) = HC(0)'
	 *	3	NER(09) = HC(1)'
	 *	2	NER(10) = HC(2)'
	 *	6	NER(11) = HC(3)'
	 *	5	NER(12) = HC(4)'
	 *	12	NER(13) = HC(5)'
	 *	11	1 (VPUL3)
	 *
	 *	output	signal
	 *	-------------------------
	 *	8	ERROR
	 *
	 * Remembering De Morgan this can be simplified:
	 * ERROR is 0, whenever all of PERR and HC(0) to HC(5) are 0.
	 * Or the other way round: any of perr or syndrome non-zero means ERROR=1.
	 * </PRE>
	 */
	if (perr || syndrome) {
		/* latch data on the first error */
		if (!mem.error) {
			mem.error = 1;
			PUT_MESR_HAMMING(mem.mesr, RH(hpb));
			PUT_MESR_PERR(mem.mesr, perr);
			PUT_MESR_PARITY(mem.mesr, RP(hpb));
			PUT_MESR_SYNDROME(mem.mesr, syndrome);
			PUT_MESR_BANK(mem.mesr, (dw_addr >> 15));
			/* latch memory address register */
			mem.mear = mem.mar & 0177777;
			LOG((0,5,"	memory error at dword addr:%07o data:%011o check:%03o\n",
				dw_addr * 2, dw_data, hpb));
			LOG((0,6,"	MEAR: %06o\n", mem.mear));
			LOG((0,6,"	MESR: %06o\n", mem.mesr ^ 0177777));
			LOG((0,6,"		Hamming code read    : %#o\n",
				GET_MESR_HAMMING(mem.mesr)));
			LOG((0,6,"		Parity error         : %o\n",
				GET_MESR_PERR(mem.mesr)));
			LOG((0,6,"		Memory parity bit    : %o\n",
				GET_MESR_PARITY(mem.mesr)));
			LOG((0,6,"		Hamming syndrome     : %#o (bit #%d)\n",
				GET_MESR_SYNDROME(mem.mesr),
				hamming_lut[GET_MESR_SYNDROME(mem.mesr)]));
			LOG((0,6,"		Memory bank          : %#o\n",
				GET_MESR_BANK(mem.mesr)));
			LOG((0,6,"	MECR: %06o\n", mem.mecr ^ 0177777));
			LOG((0,6,"		Test Hamming code    : %#o\n",
				GET_MECR_TEST_CODE(mem.mecr)));
			LOG((0,6,"		Test mode            : %s\n",
				GET_MECR_TEST_MODE(mem.mecr) ? "on" : "off"));
			LOG((0,6,"		INT on single-bit err: %s\n",
				GET_MECR_INT_SBERR(mem.mecr) ? "on" : "off"));
			LOG((0,6,"		INT on double-bit err: %s\n",
				GET_MECR_INT_DBERR(mem.mecr) ? "on" : "off"));
			LOG((0,6,"		Error correction     : %s\n",
				GET_MECR_ERRCORR(mem.mecr) ? "off" : "on"));
		}
		if (-1 == hamming_lut[syndrome]) {
			/* double-bit error: wake task_part, if we're told so */
			if (GET_MECR_INT_DBERR(mem.mecr))
				CPU_SET_TASK_WAKEUP(task_part);
		} else {
			/* single-bit error: wake task_part, if we're told so */
			if (GET_MECR_INT_SBERR(mem.mecr))
				CPU_SET_TASK_WAKEUP(task_part);
			/* should we correct the single bit error ? */
			if (0 == GET_MECR_ERRCORR(mem.mecr)) {
				LOG((0,0,"	correct bit #%d addr:%07o data:%011o check:%03o\n",
					hamming_lut[syndrome], dw_addr * 2, dw_data, hpb));
				dw_data ^= 1ul << hamming_lut[syndrome];
			}
		}
	}

	return dw_data;
}
#endif	/* HAMMING_CHECK */

/** @brief memory mapped I/O read functions */
static int (*mmio_read_fn[IO_PAGE_SIZE])(int);

/** @brief memory mapped I/O write functions */
static void (*mmio_write_fn[IO_PAGE_SIZE])(int,int);


/**
 * @brief catch unmapped memory mapped I/O reads
 *
 * @param address address that is read from
 */
static int bad_mmio_read_fn (int address)
{
	LOG((0,1,"	stray I/O read of address %#o\n", address));
	return 0177777;
}

/**
 * @brief catch unmapped memory mapped I/O writes
 *
 * @param address address that is written to
 * @param data data that is written
 */
static void bad_mmio_write_fn (int address, int data)
{
	LOG((0,1,"	stray I/O write of address %06o, data %#o\n", address, data));
}

/**
 * @brief memory error address register read
 *
 * This register is a 'shadow MAR'; it holds the address of the
 * first error since the error status was last reset. If no error
 * has occured, MEAR reports the address of the most recent
 * memory access. Note that MEAR is set whenever an error of
 * _any kind_ (single-bit or double-bit) is detected.
 */
static int mear_r(int address)
{
	int data = mem.error ? mem.mear : mem.mar;
	LOG((0,2,"	MEAR read %07o\n", data));
	return data;
}

/**
 * @brief memory error status register read
 *
 * This register reports specifics of the first error that
 * occured since MESR was last reset. Storing anything into
 * this register resets the error logic and enables it to
 * detect a new error. Bits are "low true", i.e. if the bit
 * is 0, the conidition is true.
 * <PRE>
 * MESR[0-5]	Hamming code reported from error
 * MESR[6]	Parity error
 * MESR[7]	Memory parity bit
 * MESR[8-13]	Syndrome bits
 * MESR[14-15]	Bank number in which error occured
 * </PRE>
 */
static int mesr_r(int address)
{
	int data = mem.mesr ^ 0177777;
	LOG((0,2,"	MESR read %07o\n", data));
	LOG((0,6,"		Hamming code read    : %#o\n",
		GET_MESR_HAMMING(data)));
	LOG((0,6,"		Parity error         : %o\n",
		GET_MESR_PERR(data)));
	LOG((0,6,"		Memory parity bit    : %o\n",
		GET_MESR_PARITY(data)));
#if	HAMMING_CHECK
	LOG((0,6,"		Hamming syndrome     : %#o (bit #%d)\n",
		GET_MESR_SYNDROME(data),
		hamming_lut[GET_MESR_SYNDROME(data)]));
#else
	LOG((0,6,"		Hamming syndrome     : %#o\n",
		GET_MESR_SYNDROME(data)));
#endif
	LOG((0,6,"		Memory bank          : %#o\n",
		GET_MESR_BANK(data)));
	return data;
}

static void mesr_w(int address, int data)
{
	LOG((0,2,"	MESR write %07o (clear MESR; was %07o)\n",
		data, mem.mesr));
	/* set all bits to 0 */
	mem.mesr = 0;
	/* reset the error flag */
	mem.error = 0;
	/* clear the task wakeup for the parity error task */
	CPU_CLR_TASK_WAKEUP(task_part);
}

/**
 * @brief memory error control register write
 *
 * Storing into this register is the means for controlling
 * the memory error logic. This register is set to all ones
 * (disable all interrupts) when the alto is bootstrapped
 * and when the parity error task first detects an error.
 * When an error has occured, MEAR and MESR should be read
 * before setting MECR. Bits are "low true", i.e. a 0 bit
 * enables the condition.
 *
 * <PRE>
 * MECR[0-3]	Spare
 * MECR[4-10]	Test hamming code (used only for special diagnostics)
 * MECR[11]	Test mode (used only for special diagnostics)
 * MECR[12]	Cause interrupt on single-bit errors if zero
 * MECR[13]	Cause interrupt on double-bit errors if zero
 * MECR[14]	Do not use error correction if zero
 * MECR[15]	Spare
 * </PRE>
 */
void mecr_w(int address, int data)
{
	mem.mecr = data ^ 0177777;
	ALTO_PUT(mem.mecr,16, 0, 3,0);
	ALTO_PUT(mem.mecr,16,15,15,0);
	LOG((0,2,"	MECR write %07o\n", data));
	LOG((0,6,"		Test Hamming code    : %#o\n",
		GET_MECR_TEST_CODE(mem.mecr)));
	LOG((0,6,"		Test mode            : %s\n",
		GET_MECR_TEST_MODE(mem.mecr) ? "on" : "off"));
	LOG((0,6,"		INT on single-bit err: %s\n",
		GET_MECR_INT_SBERR(mem.mecr) ? "on" : "off"));
	LOG((0,6,"		INT on double-bit err: %s\n",
		GET_MECR_INT_DBERR(mem.mecr) ? "on" : "off"));
	LOG((0,6,"		Error correction     : %s\n",
		GET_MECR_ERRCORR(mem.mecr) ? "off" : "on"));
}

/**
 * @brief memory error control register read
 */
static int mecr_r(int address)
{
	int data = mem.mecr ^ 0177777;
	/* set all spare bits */
	LOG((0,2,"	MECR read %07o\n", data));
	LOG((0,6,"		Test Hamming code    : %#o\n",
		GET_MECR_TEST_CODE(data)));
	LOG((0,6,"		Test mode            : %s\n",
		GET_MECR_TEST_MODE(data) ? "on" : "off"));
	LOG((0,6,"		INT on single-bit err: %s\n",
		GET_MECR_INT_SBERR(data) ? "on" : "off"));
	LOG((0,6,"		INT on double-bit err: %s\n",
		GET_MECR_INT_DBERR(data) ? "on" : "off"));
	LOG((0,6,"		Error correction     : %s\n",
		GET_MECR_ERRCORR(data) ? "off" : "on"));
	return data;
}

/**
 * @brief load the memory address register with some value
 *
 * @param rsel selected register (to detect refresh cycles)
 * @param addr memory address
 */
void load_mar(int rsel, int addr)
{
	if (rsel == 037) {
		/*
		 * starting a memory refresh cycle
		 * currently we don't do anything special
		 */
		LOG((0,5, "	MAR<-; refresh cycle @ %#o\n", addr));
	} else if (addr < RAM_SIZE) {
		LOG((0,2, "	MAR<-; mar = %#o\n", addr));
		mem.access = MEM_RAM;
		mem.mar = addr;
		/* fetch memory double-word to read/write latches */
		mem.rmdd = mem.wmdd = mem.ram[mem.mar/2];
		mem.cycle = cycle();
	} else {
		mem.access = MEM_NIRVANA;
		mem.mar = addr;
		mem.rmdd = mem.wmdd = ~0;
	}
}

/**
 * @brief read memory or memory mapped I/O from the address in mar to md
 *
 * @result returns value from memory (RAM or MMIO)
 */
int read_mem(void)
{
	int base_addr;

	if (MEM_NONE == mem.access) {
		LOG((0,0,"	fatal: mem read with no preceding address\n"));
		return 0177777;
	}

	if (cycle() > mem.cycle + 4) {
		LOG((0,0,"	fatal: mem read (MAR %#o) too late (+%lld cyc)\n",
			mem.mar, cycle() - mem.cycle));
		mem.access = MEM_NONE;
		return 0177777;
	}

	base_addr = mem.mar & 0177777;
	if (base_addr >= IO_PAGE_BASE) {
		mem.md = (*mmio_read_fn[base_addr - IO_PAGE_BASE])(base_addr);
		LOG((0,6,"	MD = MMIO[%#o] (%#o)\n", base_addr, mem.md));
		mem.access = MEM_NONE;
#if	DEBUG
		if (mem.watch_read)
			(*mem.watch_read)(mem.mar, mem.md);
#endif
		return mem.md;
	}

#if	HAMMING_CHECK
	/* check for errors on the first access */
	if (!(mem.access & MEM_ODD))
		mem.rmdd = hamming_code(0, mem.mar/2, mem.rmdd);
#endif
	mem.md = (mem.mar & MEM_ODD) ? GET_ODD(mem.rmdd) : GET_EVEN(mem.rmdd);
	LOG((0,6,"	MD = RAM[%#o] (%#o)\n", mem.mar, mem.md));

#if	DEBUG
	if (mem.watch_read)
		(*mem.watch_read)(mem.mar, mem.md);
#endif

	if (mem.access & MEM_ODD) {
		mem.access = MEM_NONE;
	} else {
		mem.mar ^= MEM_ODD;
		mem.access ^= MEM_ODD;
		mem.cycle++;
	}
	return mem.md;
}

/**
 * @brief write memory or memory mapped I/O from md to the address in mar
 *
 * @param data data to write to RAM or MMIO
 */
void write_mem(int data)
{
	int base_addr;

	mem.md = data & 0177777;
	if (MEM_NONE == mem.access) {
		LOG((0,0,"	fatal: mem write with no preceding address\n"));
		return;
	}

	if (cycle() > mem.cycle + 4) {
		LOG((0,0,"	fatal: mem write (MAR %#o, data %#o) too late (+%lld cyc)\n",
			mem.mar, data, cycle() - mem.cycle));
		mem.access = MEM_NONE;
		return;
	}

	base_addr = mem.mar & 0177777;
	if (base_addr >= IO_PAGE_BASE) {
		LOG((0,6, "	MMIO[%#o] = MD (%#o)\n", base_addr, mem.md));
		(*mmio_write_fn[base_addr - IO_PAGE_BASE])(base_addr, mem.md);
		mem.access = MEM_NONE;
#if	DEBUG
		if (mem.watch_write)
			(*mem.watch_write)(mem.mar, mem.md);
#endif
		return;
	}

	LOG((0,6, "	RAM[%#o] = MD (%#o)\n", mem.mar, mem.md));
	if (mem.mar & MEM_ODD)
		PUT_ODD(mem.wmdd,mem.md);
	else
		PUT_EVEN(mem.wmdd,mem.md);

#if	HAMMING_CHECK
	if (mem.access & MEM_RAM)
		mem.ram[mem.mar/2] = hamming_code(1, mem.mar/2, mem.wmdd);
#else
	if (mem.access & MEM_RAM)
		mem.ram[mem.mar/2] = mem.wmdd;
#endif

#if	DEBUG
	if (mem.watch_write)
		(*mem.watch_write)(mem.mar, mem.md);
#endif
	/* don't reset mem.access to permit double word exchange */
	mem.mar ^= MEM_ODD;
	mem.access ^= MEM_ODD;
	mem.cycle++;
}

/**
 * @brief install read and/or writte memory mapped I/O handler(s) for a range first to last
 *
 * This function fatal()s, if you specify a bad address for first and/or last.
 *
 * @param first first memory address to map
 * @param last last memory address to map
 * @param rfn pointer to a read function of type 'int (*read)(int)'
 * @param wfn pointer to a write function of type 'void (*write)(int,int)'
 */
void install_mmio_fn (int first, int last, int (*rfn)(int), void (*wfn)(int, int))
{
	int address;

	if (first <= IO_PAGE_BASE || last >= (IO_PAGE_BASE + IO_PAGE_SIZE) || first > last) {
		fatal(3, "internal error - bad memory-mapped I/O address\n");
	}

	for (address = first; address <= last; address++) {
		mmio_read_fn  [address - IO_PAGE_BASE] = rfn ? rfn : & bad_mmio_read_fn;
		mmio_write_fn [address - IO_PAGE_BASE] = wfn ? wfn : & bad_mmio_write_fn;
	}
}


/**
 * @brief debugger interface to read memory
 *
 * @param addr address to read
 * @result memory contents at address (16 bits)
 */
int debug_read_mem (int addr)
{
	int base_addr = addr & 0177777;
	int data;
	if (base_addr >= IO_PAGE_BASE) {
		data = (*mmio_read_fn[base_addr - IO_PAGE_BASE])(addr);
	} else {
		data = (addr & MEM_ODD) ? GET_ODD(mem.ram[addr/2]) : GET_EVEN(mem.ram[addr/2]);
	}
	return data;
}

/**
 * @brief debugger interface to write memory
 *
 * @param addr address to write
 * @param data data to write (16 bits used)
 */
void debug_write_mem (int addr, int data)
{
	int base_addr = addr & 0177777;
	if (base_addr >= IO_PAGE_BASE) {
		(*mmio_write_fn[base_addr - IO_PAGE_BASE])(addr, data);
	} else if (addr & MEM_ODD) {
		PUT_ODD(mem.ram[addr/2], data);
	} else {
		PUT_EVEN(mem.ram[addr/2], data);
	}
}

/**
 * @brief initialize the memory system
 *
 * Zeroes the memory context, including RAM and installs dummy
 * handlers for the memory mapped I/O area.
 * Sets handlers for access to the memory error address, status,
 * and control registers at 0177024 to 0177026.
 */
void init_memory(void)
{
	int addr;
	memset(&mem, 0, sizeof(mem));

	for (addr = 0; addr < IO_PAGE_SIZE; addr++) {
		mmio_read_fn[addr] = bad_mmio_read_fn;
		mmio_write_fn[addr] = bad_mmio_write_fn;
	}

	/**
	 * <PRE>
	 * TODO: use madr.a65 and madr.a64 to determine
	 * the actual I/O address ranges
	 *
	 * madr.a65
	 *	address	line	connected to
	 *	-------------------------------
	 *	A0		MAR[11]
	 *	A1		KEYSEL
	 *	A2		MAR[7-10] == 0
	 *	A3		MAR[12]
	 *	A4		MAR[13]
	 *	A5		MAR[14]
	 *	A6		MAR[15]
	 *	A7		IOREF (MAR[0-6] == 1)
	 * 
	 *	output data	connected to
	 *	-------------------------------
	 *	D0		IOSEL0
	 *	D1		IOSEL1
	 *	D2		IOSEL2
	 *	D3		INTIO
	 *
	 * madr.a64 
	 *	address	line	connected to
	 *	-------------------------------
	 *	A0		STORE
	 *	A1		MAR[11]
	 *	A2		MAR[7-10] == 0
	 *	A3		MAR[12]
	 *	A4		MAR[13]
	 *	A5		MAR[14]
	 *	A6		MAR[15]
	 *	A7		IOREF (MAR[0-6] == 1)
	 * 
	 *	output data	connected to
	 *	-------------------------------
	 *	D0		& MISYSCLK -> SELP
	 *	D1		^ INTIO -> INTIOX
	 *	"		^ 1 -> NERRSEL
	 *	"		& WRTCLK -> NRSTE
	 *	D2		XREG'
	 *	D3		& MISYSCLK -> LOADERC
	 * </PRE>
	 */
	install_mmio_fn(0177024, 0177024, mear_r,	NULL);
	install_mmio_fn(0177025, 0177025, mesr_r,	mesr_w);
	install_mmio_fn(0177026, 0177026, mecr_r,	mecr_w);
}
