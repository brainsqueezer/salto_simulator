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
 * Display functions
 *
 * $Id: display.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_DISPLAY_H_INCLUDED_)
#define	_DISPLAY_H_INCLUDED_

/**
 * @brief get the pixel clock speed from a SETMODE<- bus value
 */
#define	GET_SETMODE_SPEEDY(mode)	ALTO_GET(mode,16,0,0)

/**
 * @brief get the inverse video flag from a SETMODE<- bus value
 */
#define	GET_SETMODE_INVERSE(mode)	ALTO_GET(mode,16,1,1)


/**
 * @brief start value for the horizontal line counter
 *
 * This value is loaded into the three 4 bit counters (type 9316)
 * with numbers 65, 67, and 75.
 * 65: A=0 B=1 C=1 D=0
 * 67: A=1 B=0 C=0 D=1
 * 75: A=0 B=0 C=0 D=0
 *
 * The value is 150
 */
#define	DISPLAY_HLC_START	(2+4+16+128)

/**
 * @brief end value for the horizontal line counter
 *
 * This is decoded by H30, an 8 input NAND gate.
 * The value is 1899; horz. line count range 150-1899 = 1750.
 * 1750 / 2 = 875 = total scanlines.
 */
#define	DISPLAY_HLC_END		(1+2+8+32+64+256+512+1024)

/**
 * @brief display total height, including overscan (vertical blanking and synch)
 *
 * The display is interleaved in two fields, alternatingly drawing the even and odd
 * scanlines to the monitor. The frame rate is 60Hz, which is actually the rate
 * of the half-frames. The rate for full frames is thus 30Hz.
 */
#define	DISPLAY_TOTAL_HEIGHT	((DISPLAY_HLC_END+1-DISPLAY_HLC_START)/2)

/**
 * @brief display total width, including horizontal blanking
 *
 * Known facts:
 *
 * We have 606x808 visible pixels, and the pixel clock is said to be 50ns
 * (20MHz), while the crystal in the schematics is labeled 20.16MHz,
 * so the pixel clock would actually be 49.6031ns.
 *
 * The total number of scanlines is, according to the docs, 875.
 *
 * 875 scanlines at 30 frames per second, thus the scanline rate is 26250Hz.
 *
 * If I divide 20.16MHz by 26250Hz, I get 768 pixels for the total width
 * of a scanline in pixels.
 *
 * The horizontal blanking time would then be 768-606 = 162 pixels, and
 * 162 * 49.6031ns ~= 8036ns = 8.036us for the HBLANK time.
 *
 * In the display schematics there is a divide by 24 logic, and by dividing
 * the 768 pixels per scanline by 24, we have 32 phases of a scanline.
 * A S8223 PROM (a63) with 32x8 bits contains the status of the HBLANK and
 * HSYNC signals, the SCANEND and HLCGATE signals, as well as its own
 * next address A0-A3!
 *
 */
#define	DISPLAY_TOTAL_WIDTH	768

/**
 * @brief words per scanline
*/
#define	DISPLAY_SCANLINE_WORDS	(DISPLAY_TOTAL_WIDTH/16)

/**
 * @brief number of visible scanlines per frame
 * 808 really, but there are some empty lines?
 */
#define	DISPLAY_HEIGHT		808

/** @brief horizontal lince counter bit 0 */
#define	HLC1	ALTO_BIT(dsp.hlc,11,10)

/** @brief horizontal lince counter bit 1 */
#define	HLC2	ALTO_BIT(dsp.hlc,11,9)

/** @brief horizontal lince counter bit 2 */
#define	HLC4	ALTO_BIT(dsp.hlc,11,8)

/** @brief horizontal lince counter bit 3 */
#define	HLC8	ALTO_BIT(dsp.hlc,11,7)

/** @brief horizontal lince counter bit 4 */
#define	HLC16	ALTO_BIT(dsp.hlc,11,6)

/** @brief horizontal lince counter bit 5 */
#define	HLC32	ALTO_BIT(dsp.hlc,11,5)

/** @brief horizontal lince counter bit 6 */
#define	HLC64	ALTO_BIT(dsp.hlc,11,4)

/** @brief horizontal lince counter bit 7 */
#define	HLC128	ALTO_BIT(dsp.hlc,11,3)

/** @brief horizontal lince counter bit 8 */
#define	HLC256	ALTO_BIT(dsp.hlc,11,2)

/** @brief horizontal lince counter bit 9 */
#define	HLC512	ALTO_BIT(dsp.hlc,11,1)

/** @brief horizontal lince counter bit 10 */
#define	HLC1024	ALTO_BIT(dsp.hlc,11,0)

/**
 * @brief visible width of the display
 */
#define	DISPLAY_WIDTH		606

/**
 * @brief visible words per scanline
*/
#define	DISPLAY_VISIBLE_WORDS	((DISPLAY_WIDTH+15)/16)

/**
 * @brief display bit clock in in Hertz (20.16MHz)
 */
#define	DISPLAY_BITCLOCK	20160000ll

/**
 * @brief display bit time in in nano seconds (~= 49.6031ns)
 */
#define	DISPLAY_BITTIME(n)	(TIME_S(n) / DISPLAY_BITCLOCK)

/**
 * @brief time for a scanline in nano seconds (768 * 49.6031ns)
 */
#define	DISPLAY_SCANLINE_TIME	DISPLAY_BITTIME(DISPLAY_TOTAL_WIDTH)

/**
 * @brief time of the visible part of a scanline (606 * 49.6031ns)
 */
#define	DISPLAY_VISIBLE_TIME	DISPLAY_BITTIME(DISPLAY_WIDTH)

/**
 * @brief time for a word (16 pixels * 49.6031ns)
 */
#define	DISPLAY_WORD_TIME	DISPLAY_BITTIME(16)

/**
 * @brief the display fifo has 16 words
 */
#define	DISPLAY_FIFO_SIZE	16

/**
 * @brief structure of the display context
 *
 * Schematics of the task clear and wakeup signal generators
 * <PRE>
 * A quote (') appended to a signal name means inverted signal.
 *
 *  AND |     NAND|      NOR |       FF | Q    N174
 * -----+--- -----+---  -----+---  -----+---   -----
 *  0 0 | 0   0 0 | 1    0 0 | 1    S'\0| 1    delay
 *  0 1 | 0   0 1 | 1    0 1 | 0    R'\0| 0
 *  1 0 | 0   1 0 | 1    1 0 | 0
 *  1 1 | 1   1 1 | 0    1 1 | 0
 *
 *
 *                                                       DVTAC'
 *                                                      >-------.  +-----+
 *                                                              |  |  FF |
 * VBLANK'+----+ DELVBLANK' +---+  DELVBLANK   +----+           `--|S'   |
 * >------|N174|------+-----|inv|--------------|NAND| VBLANKPULSE  |     |              WAKEDVT'
 *        +----+      |     +---+              |    o--+-----------|R'  Q|---------------------->
 *                    |                      .-|    |  |           |     |
 *        +----+      |     DDELVBLANK'      | +----+  |           +-----+
 *      .-|N174|-----------------------------´         |      +---+
 *      | +----+      |                                +------oAND|
 *      |             |                      DSP07.01  |      |   o----------.
 *      `-------------´                      >---------|------o   |          |
 *                                                     |      +---+          |
 *                                                     |                     |
 *                                                     | +-----+             |
 *                                                     | |  FF |             |  +-----+
 *        DHTAC'       +---+                           | |     |             |  |  FF |
 *      >--------------oNOR|  *07.25       +----+      `-|S'   |   DHTBLK'   |  |     |
 *        BLOCK'       |   |---------------|NAND|        |    Q|--+----------|--|S1'  | WAKEDHT'
 *      >--------------o   |     DCSYSCLK  |    o--------|R'   |  | >--------|--|S2' Q|--------->
 *                     +---+     >---------|    |        +-----+  |  DHTAC'  `--|R'   |
 *                                         +----+                 |             +-----+
 *                                                   .------------´
 *                                                   |
 *        DWTAC'       +---+                         |   +-----+
 *      >--------------oNOR|  *07.26 +----+          |   |  FF |
 *        BLOCK'       |   |---------|NAND| DSP07.01 |   |     |
 *      >--------------o   | DCSYSCLK|    o----------|---|S1'  | DWTCN' +---+        DWTCN
 *                     +---+ >-------|    |          `---|S2' Q|--------|inv|-----------+----
 *                                   +----+          .---|R'   |        +---+           |
 *                                                   |   +-----+                        |
 *                 SCANEND     +----+                |                                  |
 *               >-------------|NAND|  CLRBUF'       |           .----------------------´
 *                 DCLK        |    o----------------´           |
 *               >-------------|    |                            |  +-----+
 *                             +----+                            `--| NAND|
 *                                                       STOPWAKE'  |     |preWake +----+ WAKEDWT'
 *                                                      >-----------|     o--------|N174|--------->
 *                                                        VBLANK'   |     |        +----+
 *                                                      >-----------|     |
 *                                                                  +-----+
 *                                                     a40c
 *                                        VBLANKPULSE +----+
 *                                       -------------|NAND|     
 *                                                    |    o--.
 *                                                 .--|    |  |
 *                                                 |  +----+  |
 *                                                 `----------|-.
 *                                                 .----------´ |
 *        CURTAC'      +---+                       |  +----+    |     a20d
 *      >--------------oNOR|  *07.27 +----+        `--|NAND|    |    +----+
 *        BLOCK'       |   |---------|NAND| DSP07.07  |    o----+----o NOR| preWK  +----+ WAKECURT'
 *      >--------------o   | DCSYSCLK|    o-----------|    |         |    |--------|N174|--------->
 *                     +---+ >-------|    |           +----+    +----o    |        +----+
 *                                   +----+            a40d     |    +----+
 *                                          a30c                |
 *                              CURTAC'    +----+               |
 *                            >------------|NAND|    DSP07.03   |
 *                                         |    o--+------------´
 *                                      .--|    |  |
 *                                      |  +----+  |
 *                                      `----------|-.
 *                                      .----------´ |
 *                                      |  +----+    |
 *                                      `--|NAND|    |
 *                              CLRBUF'    |    o----´
 *                            >------------|    |
 *                                         +----+
 *                                          a30d
 * </PRE>
 */
typedef struct {
	/** @brief horizontal line counter */
	int hlc;

	/** @brief most recent value read from the PROM a63 */
	int a63;

	/** @brief most recent value read from the PROM a66 */
	int a66;

	/** @brief value written by last SETMODE<- */
	int setmode;

	/** @brief set to 0xffff if line is inverse, 0x0000 otherwise */
	int inverse;

	/** @brief set 0 for normal pixel clock, 1 for half pixel clock */
	int halfclock;

	/** @brief set non-zero if any of VBLANK or HBLANK is active (a39a 74S08) */
	int clr;

	/** @brief display word fifo */
	int fifo[DISPLAY_FIFO_SIZE];

	/** @brief fifo input pointer (4-bit) */
	int fifo_wr;

	/** @brief fifo output pointer (4-bit) */
	int fifo_rd;

	/** @brief set non-zero, if the DHT executed BLOCK */
	int dht_blocks;

	/** @brief set non-zero, if the DWT executed BLOCK */
	int dwt_blocks;

	/** @brief set non-zero, if the CURT executed BLOCK */
	int curt_blocks;

	/** @brief set non-zero, if CURT wakeups are generated */
	int curt_wakeup;

	/** @brief most recent HLC with VBLANK still high (11-bit) */
	int vblank;

	/** @brief cursor cursor x position register (10-bit) */
	int xpreg;

	/** @brief cursor shift register (16-bit) */
	int csr;

	/** @brief helper: first cursor word in current scanline */
	int curword;

	/** @brief helper: shifted cursor data (32-bit) */
	uint32_t curdata;

	/** @brief array of words of the raw bitmap that is displayed */
	uint16_t raw_bitmap[DISPLAY_HEIGHT][DISPLAY_VISIBLE_WORDS];

}	display_t;

extern display_t dsp;

/**
 * @brief PROM a38 contains the STOPWAKE' and MBEMBPTY' signals for the FIFO
 * <PRE>
 * The inputs to a38 are the UNLOAD counter RA[0-3] and the DDR<- counter
 * WA[0-3], and the designer decided to reverse the address lines :-)
 *
 *	a38  counter FIFO counter
 *	--------------------------
 *	 A0  RA[0]   fifo_rd
 *	 A1  RA[1]
 *	 A2  RA[2]
 *	 A3  RA[3]
 *	 A4  WA[0]   fifo_wr
 *	 A5  WA[1]
 *	 A6  WA[2]
 *	 A7  WA[3]
 *
 * Only two bits of a38 are used:
 * 	O1 (002) = STOPWAKE'
 * 	O3 (010) = MBEMPTY'
 * </PRE>
 */
extern uint8_t displ_a38[256];

/** @brief PROM a38 bit O1 is STOPWAKE' (stop DWT if bit is zero) */
#define	FIFO_STOPWAKE_0	(displ_a38[dsp.fifo_rd*16+dsp.fifo_wr] & 002)

/** @brief PROM a38 bit O3 is MBEMPTY' (FIFO is empty if bit is zero) */
#define	FIFO_MBEMPTY_0	(displ_a38[dsp.fifo_rd*16+dsp.fifo_wr] & 010)

/**
 * @brief emulation of PROM a63 in the display schematics page 8
 * <PRE>
 * The PROM's address lines are driven by a clock CLK, which is
 * pixel clock / 24, and an inverted half-scanline signal H[1]'.
 *
 * It is 32x8 bits and its output bits (B) are connected to the
 * signals, as well as its own address lines (A) through a latch
 * of the type SN74774 like this:
 *
 * B    174     A   others
 * ------------------------
 * 0     5      -   HBLANK
 * 1     0      -   HSYNC
 * 2     4      0
 * 3     1      1
 * 4     3      2
 * 5     2      3
 * 6     -      -   SCANEND
 * 7     -      -   HLCGATE
 * ------------------------
 * H[1]' -      4
 *
 * The display_state_machine() is called by the CPU at a rate of pixelclock/24,
 * which happens to be very close to every 7th CPU micrcocycle.
 * </PRE>
 */
extern uint8_t displ_a63[32];

/** @brief PROM a63 B0 is latched as HBLANK signal */
#define	A63_HBLANK	0001

/** @brief PROM a63 B1 is latched as HSYNC signal */
#define	A63_HSYNC	0002

/** @brief PROM a63 B2 is the latched next address bit A0 */
#define	A63_A0		0004

/** @brief PROM a63 B3 is the latched next address bit A1 */
#define	A63_A1		0010

/** @brief PROM a63 B4 is the latched next address bit A2 */
#define	A63_A2		0020

/** @brief PROM a63 B5 is the latched next address bit A3 */
#define	A63_A3		0040

/** @brief PROM a63 B6 SCANEND signal, which resets the FIFO counters */
#define	A63_SCANEND	0100

/** @brief PROM a63 B7 HLCGATE signal, which enables counting the HLC */
#define	A63_HLCGATE	0200

/** @brief helper to extract A3-A0 from a PROM a63 value */
#define	A63_NEXT(n)	(((n) >> 2) & 017)

/** @brief test the HBLANK (horizontal blanking) signal in PROM a63 being high */
#define	HBLANK_HI(a)	(0!=((a)&A63_HBLANK))

/** @brief test the HBLANK (horizontal blanking) signal in PROM a63 being low */
#define	HBLANK_LO(a)	(0==((a)&A63_HBLANK))

/** @brief test the HSYNC (horizontal synchonisation) signal in PROM a63 being high */
#define	HSYNC_HI(a)	(0!=((a)&A63_HSYNC))

/** @brief test the HSYNC (horizontal synchonisation) signal in PROM a63 being low */
#define	HSYNC_LO(a)	(0==((a)&A63_HSYNC))

/** @brief test the SCANEND (scanline end) signal in PROM a63 being high */
#define	SCANEND_HI(a)	(0!=((a)&A63_SCANEND))

/** @brief test the SCANEND (scanline end) signal in PROM a63 being low */
#define	SCANEND_LO(a)	(0==((a)&A63_SCANEND))

/** @brief test the HLCGATE (horz. line counter gate) signal in PROM a63 being high */
#define	HLCGATE_HI(a)	(0!=((a)&A63_HLCGATE))

/** @brief test the HLCGATE (horz. line counter gate) signal in PROM a63 being low */
#define	HLCGATE_LO(a)	(0==((a)&A63_HLCGATE))

/**
 * @brief vertical blank and synch PROM
 *
 * PROM a66 is a 256x4 bit (type 3601), containing the vertical blank + synch.
 * Address lines are driven by H[1] to H[128] of the the horz. line counters.
 * The PROM is enabled whenever H[256] and H[512] are both 0.
 *
 * Q1 (001) is VSYNC for the odd field (with H1024=1)
 * Q2 (002) is VSYNC for the even field (with H1024=0)
 * Q3 (004) is VBLANK for the odd field (with H1024=1)
 * Q4 (010) is VBLANK for the even field (with H1024=0)
 */
extern uint8_t displ_a66[256];

/** @brief test mask for the VSYNC signal (depends on field, i.e. HLC1024) */
#define	A66_VSYNC	(HLC1024 ? 001 : 002)

/** @brief test mask the VBLANK signal (depends on field, i.e. HLC1024) */
#define	A66_VBLANK	(HLC1024 ? 004 : 010)

/** @brief test the VSYNC (vertical synchronisation) signal in PROM a66 being high */
#define	VSYNC_HI(a)	(0==((a)&A66_VSYNC))

/** @brief test the VSYNC (vertical synchronisation) signal in PROM a66 being low */
#define	VSYNC_LO(a)	(0!=((a)&A66_VSYNC))

/** @brief test the VBLANK (vertical blanking) signal in PROM a66 being high */
#define	VBLANK_HI(a)	(0==((a)&A66_VBLANK))

/** @brief test the VBLANK (vertical blanking) signal in PROM a66 being low */
#define	VBLANK_LO(a)	(0!=((a)&A66_VBLANK))

/**
 * @brief unload the next word from the display FIFO and shift it to the screen
 */
extern int unload_word(int x);

/**
 * @brief function called by the CPU to enter the next display state
 *
 * There are 32 states per scanline and 875 scanlines per frame.
 *
 * @param arg the current displ_a63 PROM address
 * @result returns the next state of the display state machine
 */
extern int display_state_machine(int arg);

/** @brief branch on the evenfield flip-flop */
extern void f2_evenfield_1(void);

/**
 * @brief create a PNG file screenshot from the current raw bitmap
 *
 * @param ptr pointer to a png_t context
 * @param left left clipping boundary
 * @param top top clipping boundary
 * @param right right clipping boundary (inclusive)
 * @param bottom bottom clipping boundary (inclusive)
 * @result returns 0 on success, -1 on error
 */
extern int display_screenshot(void *ptr, int left, int top, int right, int bottom);

/** @brief initialize the display context */
extern int display_init(void);

#endif	/* !defined(_DISPLAY_H_INCLUDED_) */
