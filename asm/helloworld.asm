dcb_h		equ	32			; DCB row height
HTAB		equ	0			; indentation by 16*htab bits
NWRDS		equ	32			; number of words in DCB bitmap
SLC		equ	(dcb_h/2)		; scanline count in DCB bitmap (per frame)

		org	0
		dw	start
		dw	0
		dw	0
entryloc:	dw	entry
start:		jmp	@entryloc

		org	01000

message1:	db	"Hello world! This is my first Xerox Alto program."
		dw	0
message2:	db	"[Always read the fine print when taking up a job!]"
		dw	0
message3:	db	" !",33,"#$%&'()*+,-./0123456789:;<=>?"
		db	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_"
		db	"`abcdefghijklmnopqrstuvwxyz{|}~",127
		dw	0

		even
bitmap:		; dcb_h rows of bitmap data

		org	bitmap + NWRDS * dcb_h

dwaoffs1	equ	bitmap-NWRDS
dwaoffs2	equ	bitmap-NWRDS+14*NWRDS
dwaoffs3	equ	bitmap-NWRDS+22*NWRDS
dba		equ	12

MASKTABLOC:	0460
MASKTABEND:	0500
MYDCBLOC:	DCB				; address of my DCB
DASTARTLOC:	0420				; address of DASTART (display address)

entry:		lda	0 MYDCBLOC		; get our DCB address to AC0
		sta	0 @DASTARTLOC		; store new DASTART

;
; create the mask table from 0460 to 0477
; that is required for convert to work.
;
		sub	0 0			; AC0 = 0
		lda	2 MASKTABLOC		; AC2 = address of masktab
fill_masktab:	inc	0 0			; AC0++
		sta	0 0,2			; store into masktab
		movl	0 0			; AC0 <<= 1
		inc	2 2			; AC2++
		lda	3 MASKTABEND		; AC3 = end of masktab
		sub	2 3 SZR			; AC2 != MASKTABEND?
		  jmp	fill_masktab		; yes, fill more

; print first message with first font
		jsr	print
		dw	message1		; parameter 0
		dw	fontbase1		; parameter 1
		dw	dwaoffs1		; parameter 2
		dw	dba			; parameter 3

; print second message with small font
		jsr	print
		dw	message2		; parameter 0
		dw	fontbase2		; parameter 1
		dw	dwaoffs2		; parameter 2
		dw	dba			; parameter 3

; print third message with small font
		jsr	print
		dw	message3		; parameter 0
		dw	fontbase2		; parameter 1
		dw	dwaoffs3		; parameter 2
		dw	dba			; parameter 3

loop:		jmp	loop


CONVTABLELOC:	CONVTABLE
CONVTABLE:	NWRDS				; number of words in the "to" bitmap
		12				; DBA: destination bit address
DBAWIDTH:	16				; 16 bits per word
DWAOFFS:	bitmap-NWRDS			; convert needs this in AC0
FONTBASELOC:	0				; address of fontbase #1

argcnt:		4				; print takes 4 words as arguments
offset:		0				; offset into message text
bytemask:	0377				; mask for lower significant byte
whichhalf:	0				; flag for upper/lower byte

print:		sub	0 0
		sta	0 whichhalf

		lda	0 0,3			; parameter 0 is offset to text message
		sta	0 offset
		lda	0 1,3			; parameter 1 is font base
		sta	0 FONTBASELOC
		lda	0 2,3			; parameter 2 is display word address - offset
		sta	0 DWAOFFS
		lda	0 dba			; parameter 3 is destination bit address
		sta	0 CONVTABLE+1

		lda	0 argcnt		; number of arguments
		add	0 3			; AC3 += args
		sta	3 printret

		lda	2 CONVTABLELOC		; AC2 = convert table offset
printch:	lda	3 offset		; get offset into message
		lda	0 0,3			; next two characters from (message+AC3) to AC0

		lda	1 whichhalf
		com	1 1 SNR			; complement, skip if non-zero
		  jmp	printmsb		; swap AC0
		sta	1 whichhalf
		jmp	masklsb

printmsb:	movs	0 0			; AC0 = byte swap AC0
		sta	1 whichhalf
		inc	3 3			; AC3++

masklsb:	sta	3 offset		; save offset into message
		lda	1 bytemask		; get byte mask
		and	1 0 SNR			; and with byte mask, skip if no zero
		  jmp	done			; done
		lda	3 FONTBASELOC		; AC3 = fontbase
		add	0 3			; AC3 = fontbase + char
		;
		; Preparing accus for convert:
		; AC0 row address - NWRDS
		; AC1 -
		; AC2 base register for "convert disp,AC2"
		; AC3 pointer to fontbase + character code
		;
		lda	0 DWAOFFS		; AC0 = row address (DWA-NWRDS)
		convert	0,2			; convert font char into bitmap
		  jmp	done			; convert skips this, if no extension is required
		;
		; convert returns with
		; AC0 unchanged
		; AC1 DBA & 017
		; AC2 unchanged
		; AC3 width of the character in bits
		;
		sub	3 1 SZC			; AC1 -= AC3, i.e. DBA & 017 -= width, skip if no carry
		  jmp	nextword
		sta	1 CONVTABLE+1		; (CONVTABLE+1) = AC1
		jmp	printch			; next character

nextword:	lda	0 DBAWIDTH
		add	0 1			; AC1 += 16
		sta	1 CONVTABLE+1		; (CONVTABLE+1) = AC1
		lda	0 DWAOFFS		; AC0 = word offset into bitmap
		inc	0 0			; AC0++
		sta	0 DWAOFFS		; store new offset into bitmap
		jmp	printch

printret:	0				; return address stored here
done:		jmp	@printret		; return to caller


;
; DCB for the display
;
		even
DCB:		dw	DCB1			; DCB: pointer to next DCB; 0 = last
						; DCB+1: (NWRDS, HTAB, and flags)
						; bit	function
						; -----------------------------------------
						; 0	low resolution if set
						; 1	inverse if set
						; 2-7	HTAB: indentation by 16*htab bits
						; 8-15	NWRDS: number of words in bitmap (must be even)
						;
		dw	(0<<15)+(0<<14)+(HTAB<<8)+NWRDS
		dw	bitmap			; DCB+2: (SA) starting address of bitmap (must be even)
		dw	SLC			; DCB+3: (SLC) block defines 2*SLC scanlines, SLC in each field

DCB1:		dw	DCB2
		dw	(0<<15)+(1<<14)+((HTAB+2)<<8)+NWRDS
		dw	bitmap			; DCB+2: (SA) starting address of bitmap (must be even)
		dw	SLC			; DCB+3: (SLC) block defines 2*SLC scanlines, SLC in each field

DCB2:		dw	DCB3
		dw	(0<<15)+(0<<14)+(HTAB<<8)+NWRDS
		dw	bitmap			; DCB+2: (SA) starting address of bitmap (must be even)
		dw	SLC			; DCB+3: (SLC) block defines 2*SLC scanlines, SLC in each field

DCB3:		dw	DCB4
		dw	(1<<15)+(0<<14)+(HTAB<<8)+NWRDS
		dw	bitmap			; DCB+2: (SA) starting address of bitmap (must be even)
		dw	2*SLC			; DCB+3: (SLC) block defines 2*SLC scanlines, SLC in each field

DCB4:		dw	0
		dw	(0<<15)+(0<<14)+((HTAB+7)<<8)+NWRDS
		dw	bitmap			; DCB+2: (SA) starting address of bitmap (must be even)
		dw	SLC			; DCB+3: (SLC) block defines 2*SLC scanlines, SLC in each field

; Generated by convbdf on Sun Jun 10 22:21:29 2007.
; =====================================================================
; Font information
;   name        : 7x13
;   facename    : -Misc-Fixed-Medium-R-Normal--13-120-75-75-C-70-ISO10646-1
;   w x h       : 7x13
;   size        : 256
;   ascent      : 11
;   descent     : 2
;   first char  : 0 (0x00)
;   last char   : 255 (0xff)
;   default char: 0 (0x00)
;   proportional: no
;   Public domain font.  Share and enjoy.
; =====================================================================

; font height (header info)
fontheight1:	dw	13
; font type, baseline, maxwidth (header info)
fonttype1:	dw	(0 << 15) | (11 << 8) | 7
fontbase1:	; self relative offsets to character XW1_s
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_040-.,XW1_041-.,XW1_042-.,XW1_043-.
		dw	XW1_044-.,XW1_045-.,XW1_046-.,XW1_047-.
		dw	XW1_050-.,XW1_051-.,XW1_052-.,XW1_053-.
		dw	XW1_054-.,XW1_055-.,XW1_056-.,XW1_057-.
		dw	XW1_060-.,XW1_061-.,XW1_062-.,XW1_063-.
		dw	XW1_064-.,XW1_065-.,XW1_066-.,XW1_067-.
		dw	XW1_070-.,XW1_071-.,XW1_072-.,XW1_073-.
		dw	XW1_074-.,XW1_075-.,XW1_076-.,XW1_077-.
		dw	XW1_100-.,XW1_101-.,XW1_102-.,XW1_103-.
		dw	XW1_104-.,XW1_105-.,XW1_106-.,XW1_107-.
		dw	XW1_110-.,XW1_111-.,XW1_112-.,XW1_113-.
		dw	XW1_114-.,XW1_115-.,XW1_116-.,XW1_117-.
		dw	XW1_120-.,XW1_121-.,XW1_122-.,XW1_123-.
		dw	XW1_124-.,XW1_125-.,XW1_126-.,XW1_127-.
		dw	XW1_130-.,XW1_131-.,XW1_132-.,XW1_133-.
		dw	XW1_134-.,XW1_135-.,XW1_136-.,XW1_137-.
		dw	XW1_140-.,XW1_141-.,XW1_142-.,XW1_143-.
		dw	XW1_144-.,XW1_145-.,XW1_146-.,XW1_147-.
		dw	XW1_150-.,XW1_151-.,XW1_152-.,XW1_153-.
		dw	XW1_154-.,XW1_155-.,XW1_156-.,XW1_157-.
		dw	XW1_160-.,XW1_161-.,XW1_162-.,XW1_163-.
		dw	XW1_164-.,XW1_165-.,XW1_166-.,XW1_167-.
		dw	XW1_170-.,XW1_171-.,XW1_172-.,XW1_173-.
		dw	XW1_174-.,XW1_175-.,XW1_176-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_000-.,XW1_000-.,XW1_000-.,XW1_000-.
		dw	XW1_240-.,XW1_241-.,XW1_242-.,XW1_243-.
		dw	XW1_244-.,XW1_245-.,XW1_246-.,XW1_247-.
		dw	XW1_250-.,XW1_251-.,XW1_252-.,XW1_253-.
		dw	XW1_254-.,XW1_255-.,XW1_256-.,XW1_257-.
		dw	XW1_260-.,XW1_261-.,XW1_262-.,XW1_263-.
		dw	XW1_264-.,XW1_265-.,XW1_266-.,XW1_267-.
		dw	XW1_270-.,XW1_271-.,XW1_272-.,XW1_273-.
		dw	XW1_274-.,XW1_275-.,XW1_276-.,XW1_277-.
		dw	XW1_300-.,XW1_301-.,XW1_302-.,XW1_303-.
		dw	XW1_304-.,XW1_305-.,XW1_306-.,XW1_307-.
		dw	XW1_310-.,XW1_311-.,XW1_312-.,XW1_313-.
		dw	XW1_314-.,XW1_315-.,XW1_316-.,XW1_317-.
		dw	XW1_320-.,XW1_321-.,XW1_322-.,XW1_323-.
		dw	XW1_324-.,XW1_325-.,XW1_326-.,XW1_327-.
		dw	XW1_330-.,XW1_331-.,XW1_332-.,XW1_333-.
		dw	XW1_334-.,XW1_335-.,XW1_336-.,XW1_337-.
		dw	XW1_340-.,XW1_341-.,XW1_342-.,XW1_343-.
		dw	XW1_344-.,XW1_345-.,XW1_346-.,XW1_347-.
		dw	XW1_350-.,XW1_351-.,XW1_352-.,XW1_353-.
		dw	XW1_354-.,XW1_355-.,XW1_356-.,XW1_357-.
		dw	XW1_360-.,XW1_361-.,XW1_362-.,XW1_363-.
		dw	XW1_364-.,XW1_365-.,XW1_366-.,XW1_367-.
		dw	XW1_370-.,XW1_371-.,XW1_372-.,XW1_373-.
		dw	XW1_374-.,XW1_375-.,XW1_376-.,XW1_377-.

; Font bitmap data

; Character 0 (0x00):  width 7  char0
;	+-------+
;	|       |
;	|       |
;	|* * ** |
;	|       |
;	|*    * |
;	|       |
;	|*    * |
;	|       |
;	|*    * |
;	|       |
;	|** * * |
;	|       |
;	|       |
;	+-------+ 
		dw	0126000
		dw	0000000
		dw	0102000
		dw	0000000
		dw	0102000
		dw	0000000
		dw	0102000
		dw	0000000
		dw	0152000
XW1_000:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 32 (0x20):  width 7  space
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
XW1_040:		dw	2 * 7 + 1		; no extent
		dw	(13 << 8) | 0		; HD: 13, XH: 0

; Character 33 (0x21):  width 7  exclam
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0000000
		dw	0010000
XW1_041:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 34 (0x22):  width 7  quotedbl
;	+-------+
;	|       |
;	|       |
;	|  * *  |
;	|  * *  |
;	|  * *  |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0024000
		dw	0024000
		dw	0024000
XW1_042:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 3		; HD: 2, XH: 3

; Character 35 (0x23):  width 7  numbersign
;	+-------+
;	|       |
;	|       |
;	|       |
;	|  * *  |
;	|  * *  |
;	| ***** |
;	|  * *  |
;	| ***** |
;	|  * *  |
;	|  * *  |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0024000
		dw	0024000
		dw	0076000
		dw	0024000
		dw	0076000
		dw	0024000
		dw	0024000
XW1_043:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 7		; HD: 3, XH: 7

; Character 36 (0x24):  width 7  dollar
;	+-------+
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|  **** |
;	| * *   |
;	|  ***  |
;	|   * * |
;	| ****  |
;	|   *   |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0036000
		dw	0050000
		dw	0034000
		dw	0012000
		dw	0074000
		dw	0010000
XW1_044:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 7		; HD: 3, XH: 7

; Character 37 (0x25):  width 7  percent
;	+-------+
;	|       |
;	|       |
;	| *   * |
;	|* *  * |
;	| *  *  |
;	|   *   |
;	|   *   |
;	|  *    |
;	| *  *  |
;	|*  * * |
;	|*   *  |
;	|       |
;	|       |
;	+-------+ 
		dw	0042000
		dw	0122000
		dw	0044000
		dw	0010000
		dw	0010000
		dw	0020000
		dw	0044000
		dw	0112000
		dw	0104000
XW1_045:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 38 (0x26):  width 7  ampersand
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	| **    |
;	|*  *   |
;	|*  *   |
;	| **    |
;	|*  * * |
;	|*   *  |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
		dw	0112000
		dw	0104000
		dw	0072000
XW1_046:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 7		; HD: 4, XH: 7

; Character 39 (0x27):  width 7  quotesingle
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0010000
		dw	0010000
XW1_047:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 3		; HD: 2, XH: 3

; Character 40 (0x28):  width 7  parenleft
;	+-------+
;	|       |
;	|       |
;	|    *  |
;	|   *   |
;	|   *   |
;	|  *    |
;	|  *    |
;	|  *    |
;	|   *   |
;	|   *   |
;	|    *  |
;	|       |
;	|       |
;	+-------+ 
		dw	0004000
		dw	0010000
		dw	0010000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0010000
		dw	0010000
		dw	0004000
XW1_050:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 41 (0x29):  width 7  parenright
;	+-------+
;	|       |
;	|       |
;	|  *    |
;	|   *   |
;	|   *   |
;	|    *  |
;	|    *  |
;	|    *  |
;	|   *   |
;	|   *   |
;	|  *    |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0010000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0010000
		dw	0010000
		dw	0020000
XW1_051:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 42 (0x2a):  width 7  asterisk
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	| *  *  |
;	|  **   |
;	|****** |
;	|  **   |
;	| *  *  |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0030000
		dw	0176000
		dw	0030000
		dw	0044000
XW1_052:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 5		; HD: 4, XH: 5

; Character 43 (0x2b):  width 7  plus
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|   *   |
;	| ***** |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0010000
		dw	0076000
		dw	0010000
		dw	0010000
XW1_053:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 5		; HD: 4, XH: 5

; Character 44 (0x2c):  width 7  comma
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|  ***  |
;	|  **   |
;	| *     |
;	|       |
;	+-------+ 
		dw	0034000
		dw	0030000
		dw	0040000
XW1_054:		dw	2 * 7 + 1		; no extent
		dw	(9 << 8) | 3		; HD: 9, XH: 3

; Character 45 (0x2d):  width 7  hyphen
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ***** |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0076000
XW1_055:		dw	2 * 7 + 1		; no extent
		dw	(6 << 8) | 1		; HD: 6, XH: 1

; Character 46 (0x2e):  width 7  period
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|  ***  |
;	|   *   |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0034000
		dw	0010000
XW1_056:		dw	2 * 7 + 1		; no extent
		dw	(9 << 8) | 3		; HD: 9, XH: 3

; Character 47 (0x2f):  width 7  slash
;	+-------+
;	|       |
;	|       |
;	|     * |
;	|     * |
;	|    *  |
;	|    *  |
;	|   *   |
;	|  *    |
;	|  *    |
;	| *     |
;	| *     |
;	|       |
;	|       |
;	+-------+ 
		dw	0002000
		dw	0002000
		dw	0004000
		dw	0004000
		dw	0010000
		dw	0020000
		dw	0020000
		dw	0040000
		dw	0040000
XW1_057:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 48 (0x30):  width 7  zero
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| *  *  |
;	|  **   |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0044000
		dw	0030000
XW1_060:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 49 (0x31):  width 7  one
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  **   |
;	| * *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0030000
		dw	0050000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_061:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 50 (0x32):  width 7  two
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|     * |
;	|    *  |
;	|  **   |
;	| *     |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0002000
		dw	0004000
		dw	0030000
		dw	0040000
		dw	0100000
		dw	0176000
XW1_062:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 51 (0x33):  width 7  three
;	+-------+
;	|       |
;	|       |
;	|****** |
;	|     * |
;	|    *  |
;	|   *   |
;	|  ***  |
;	|     * |
;	|     * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0002000
		dw	0004000
		dw	0010000
		dw	0034000
		dw	0002000
		dw	0002000
		dw	0102000
		dw	0074000
XW1_063:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 52 (0x34):  width 7  four
;	+-------+
;	|       |
;	|       |
;	|    *  |
;	|   **  |
;	|  * *  |
;	| *  *  |
;	|*   *  |
;	|*   *  |
;	|****** |
;	|    *  |
;	|    *  |
;	|       |
;	|       |
;	+-------+ 
		dw	0004000
		dw	0014000
		dw	0024000
		dw	0044000
		dw	0104000
		dw	0104000
		dw	0176000
		dw	0004000
		dw	0004000
XW1_064:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 53 (0x35):  width 7  five
;	+-------+
;	|       |
;	|       |
;	|****** |
;	|*      |
;	|*      |
;	|* ***  |
;	|**   * |
;	|     * |
;	|     * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0100000
		dw	0100000
		dw	0134000
		dw	0142000
		dw	0002000
		dw	0002000
		dw	0102000
		dw	0074000
XW1_065:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 54 (0x36):  width 7  six
;	+-------+
;	|       |
;	|       |
;	|  ***  |
;	| *     |
;	|*      |
;	|*      |
;	|* ***  |
;	|**   * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0034000
		dw	0040000
		dw	0100000
		dw	0100000
		dw	0134000
		dw	0142000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_066:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 55 (0x37):  width 7  seven
;	+-------+
;	|       |
;	|       |
;	|****** |
;	|     * |
;	|    *  |
;	|   *   |
;	|   *   |
;	|  *    |
;	|  *    |
;	| *     |
;	| *     |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0002000
		dw	0004000
		dw	0010000
		dw	0010000
		dw	0020000
		dw	0020000
		dw	0040000
		dw	0040000
XW1_067:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 56 (0x38):  width 7  eight
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_070:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 57 (0x39):  width 7  nine
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|     * |
;	|     * |
;	|    *  |
;	| ***   |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
		dw	0002000
		dw	0002000
		dw	0004000
		dw	0070000
XW1_071:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 58 (0x3a):  width 7  colon
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|  ***  |
;	|   *   |
;	|       |
;	|       |
;	|   *   |
;	|  ***  |
;	|   *   |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0034000
		dw	0010000
		dw	0000000
		dw	0000000
		dw	0010000
		dw	0034000
		dw	0010000
XW1_072:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 8		; HD: 4, XH: 8

; Character 59 (0x3b):  width 7  semicolon
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|  ***  |
;	|   *   |
;	|       |
;	|       |
;	|  ***  |
;	|  **   |
;	| *     |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0034000
		dw	0010000
		dw	0000000
		dw	0000000
		dw	0034000
		dw	0030000
		dw	0040000
XW1_073:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 8		; HD: 4, XH: 8

; Character 60 (0x3c):  width 7  less
;	+-------+
;	|       |
;	|       |
;	|     * |
;	|    *  |
;	|   *   |
;	|  *    |
;	| *     |
;	|  *    |
;	|   *   |
;	|    *  |
;	|     * |
;	|       |
;	|       |
;	+-------+ 
		dw	0002000
		dw	0004000
		dw	0010000
		dw	0020000
		dw	0040000
		dw	0020000
		dw	0010000
		dw	0004000
		dw	0002000
XW1_074:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 61 (0x3d):  width 7  equal
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|****** |
;	|       |
;	|       |
;	|****** |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0000000
		dw	0000000
		dw	0176000
XW1_075:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 4		; HD: 5, XH: 4

; Character 62 (0x3e):  width 7  greater
;	+-------+
;	|       |
;	|       |
;	| *     |
;	|  *    |
;	|   *   |
;	|    *  |
;	|     * |
;	|    *  |
;	|   *   |
;	|  *    |
;	| *     |
;	|       |
;	|       |
;	+-------+ 
		dw	0040000
		dw	0020000
		dw	0010000
		dw	0004000
		dw	0002000
		dw	0004000
		dw	0010000
		dw	0020000
		dw	0040000
XW1_076:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 63 (0x3f):  width 7  question
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|     * |
;	|    *  |
;	|   *   |
;	|   *   |
;	|       |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0002000
		dw	0004000
		dw	0010000
		dw	0010000
		dw	0000000
		dw	0010000
XW1_077:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 64 (0x40):  width 7  at
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*  *** |
;	|* *  * |
;	|* * ** |
;	|*  * * |
;	|*      |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0116000
		dw	0122000
		dw	0126000
		dw	0112000
		dw	0100000
		dw	0074000
XW1_100:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 65 (0x41):  width 7  A
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
		dw	0102000
XW1_101:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 66 (0x42):  width 7  B
;	+-------+
;	|       |
;	|       |
;	|*****  |
;	| *   * |
;	| *   * |
;	| *   * |
;	| ****  |
;	| *   * |
;	| *   * |
;	| *   * |
;	|*****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0174000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0074000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0174000
XW1_102:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 67 (0x43):  width 7  C
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0102000
		dw	0074000
XW1_103:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 68 (0x44):  width 7  D
;	+-------+
;	|       |
;	|       |
;	|*****  |
;	| *   * |
;	| *   * |
;	| *   * |
;	| *   * |
;	| *   * |
;	| *   * |
;	| *   * |
;	|*****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0174000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0174000
XW1_104:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 69 (0x45):  width 7  E
;	+-------+
;	|       |
;	|       |
;	|****** |
;	|*      |
;	|*      |
;	|*      |
;	|****   |
;	|*      |
;	|*      |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0170000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0176000
XW1_105:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 70 (0x46):  width 7  F
;	+-------+
;	|       |
;	|       |
;	|****** |
;	|*      |
;	|*      |
;	|*      |
;	|****   |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0170000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
XW1_106:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 71 (0x47):  width 7  G
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*      |
;	|*      |
;	|*      |
;	|*  *** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0116000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_107:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 72 (0x48):  width 7  H
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
XW1_110:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 73 (0x49):  width 7  I
;	+-------+
;	|       |
;	|       |
;	| ***** |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0076000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_111:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 74 (0x4a):  width 7  J
;	+-------+
;	|       |
;	|       |
;	|   *** |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|*   *  |
;	| ***   |
;	|       |
;	|       |
;	+-------+ 
		dw	0016000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0104000
		dw	0070000
XW1_112:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 75 (0x4b):  width 7  K
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|*   *  |
;	|*  *   |
;	|* *    |
;	|**     |
;	|* *    |
;	|*  *   |
;	|*   *  |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0104000
		dw	0110000
		dw	0120000
		dw	0140000
		dw	0120000
		dw	0110000
		dw	0104000
		dw	0102000
XW1_113:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 76 (0x4c):  width 7  L
;	+-------+
;	|       |
;	|       |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0176000
XW1_114:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 77 (0x4d):  width 7  M
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|**  ** |
;	|**  ** |
;	|* ** * |
;	|* ** * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0146000
		dw	0146000
		dw	0132000
		dw	0132000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
XW1_115:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 78 (0x4e):  width 7  N
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|**   * |
;	|* *  * |
;	|*  * * |
;	|*   ** |
;	|*    * |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0142000
		dw	0122000
		dw	0112000
		dw	0106000
		dw	0102000
		dw	0102000
		dw	0102000
XW1_116:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 79 (0x4f):  width 7  O
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_117:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 80 (0x50):  width 7  P
;	+-------+
;	|       |
;	|       |
;	|*****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*****  |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|       |
;	|       |
;	+-------+ 
		dw	0174000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0174000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
XW1_120:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 81 (0x51):  width 7  Q
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|* *  * |
;	|*  * * |
;	| ****  |
;	|     * |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0122000
		dw	0112000
		dw	0074000
		dw	0002000
XW1_121:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 10		; HD: 2, XH: 10

; Character 82 (0x52):  width 7  R
;	+-------+
;	|       |
;	|       |
;	|*****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*****  |
;	|* *    |
;	|*  *   |
;	|*   *  |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0174000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0174000
		dw	0120000
		dw	0110000
		dw	0104000
		dw	0102000
XW1_122:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 83 (0x53):  width 7  S
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*      |
;	|*      |
;	| ****  |
;	|     * |
;	|     * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0100000
		dw	0100000
		dw	0074000
		dw	0002000
		dw	0002000
		dw	0102000
		dw	0074000
XW1_123:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 84 (0x54):  width 7  T
;	+-------+
;	|       |
;	|       |
;	| ***** |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0076000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
XW1_124:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 85 (0x55):  width 7  U
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_125:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 86 (0x56):  width 7  V
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	| *  *  |
;	| *  *  |
;	| *  *  |
;	|  **   |
;	|  **   |
;	|  **   |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0044000
		dw	0044000
		dw	0044000
		dw	0030000
		dw	0030000
		dw	0030000
XW1_126:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 87 (0x57):  width 7  W
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|* ** * |
;	|* ** * |
;	|**  ** |
;	|**  ** |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0132000
		dw	0132000
		dw	0146000
		dw	0146000
		dw	0102000
XW1_127:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 88 (0x58):  width 7  X
;	+-------+
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	| *  *  |
;	| *  *  |
;	|  **   |
;	| *  *  |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0044000
		dw	0044000
		dw	0030000
		dw	0044000
		dw	0044000
		dw	0102000
		dw	0102000
XW1_130:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 89 (0x59):  width 7  Y
;	+-------+
;	|       |
;	|       |
;	| *   * |
;	| *   * |
;	|  * *  |
;	|  * *  |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0042000
		dw	0042000
		dw	0024000
		dw	0024000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
XW1_131:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 90 (0x5a):  width 7  Z
;	+-------+
;	|       |
;	|       |
;	|****** |
;	|     * |
;	|    *  |
;	|   *   |
;	|  **   |
;	|  *    |
;	| *     |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0002000
		dw	0004000
		dw	0010000
		dw	0030000
		dw	0020000
		dw	0040000
		dw	0100000
		dw	0176000
XW1_132:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 91 (0x5b):  width 7  bracketleft
;	+-------+
;	|       |
;	| ****  |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	| ****  |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0074000
XW1_133:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 11		; HD: 1, XH: 11

; Character 92 (0x5c):  width 7  backslash
;	+-------+
;	|       |
;	|       |
;	| *     |
;	| *     |
;	|  *    |
;	|  *    |
;	|   *   |
;	|    *  |
;	|    *  |
;	|     * |
;	|     * |
;	|       |
;	|       |
;	+-------+ 
		dw	0040000
		dw	0040000
		dw	0020000
		dw	0020000
		dw	0010000
		dw	0004000
		dw	0004000
		dw	0002000
		dw	0002000
XW1_134:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 93 (0x5d):  width 7  bracketright
;	+-------+
;	|       |
;	| ****  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	|    *  |
;	| ****  |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0074000
XW1_135:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 11		; HD: 1, XH: 11

; Character 94 (0x5e):  width 7  asciicircum
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  * *  |
;	| *   * |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0024000
		dw	0042000
XW1_136:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 3		; HD: 2, XH: 3

; Character 95 (0x5f):  width 7  underscore
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|****** |
;	|       |
;	+-------+ 
		dw	0176000
XW1_137:		dw	2 * 7 + 1		; no extent
		dw	(11 << 8) | 1		; HD: 11, XH: 1

; Character 96 (0x60):  width 7  grave
;	+-------+
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
XW1_140:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 2		; HD: 1, XH: 2

; Character 97 (0x61):  width 7  a
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ****  |
;	|     * |
;	| ***** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0002000
		dw	0076000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_141:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 98 (0x62):  width 7  b
;	+-------+
;	|       |
;	|       |
;	|*      |
;	|*      |
;	|*      |
;	|* ***  |
;	|**   * |
;	|*    * |
;	|*    * |
;	|**   * |
;	|* ***  |
;	|       |
;	|       |
;	+-------+ 
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0134000
		dw	0142000
		dw	0102000
		dw	0102000
		dw	0142000
		dw	0134000
XW1_142:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 99 (0x63):  width 7  c
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*      |
;	|*      |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0100000
		dw	0100000
		dw	0102000
		dw	0074000
XW1_143:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 100 (0x64):  width 7  d
;	+-------+
;	|       |
;	|       |
;	|     * |
;	|     * |
;	|     * |
;	| *** * |
;	|*   ** |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0002000
		dw	0002000
		dw	0002000
		dw	0072000
		dw	0106000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_144:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 101 (0x65):  width 7  e
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|****** |
;	|*      |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0176000
		dw	0100000
		dw	0102000
		dw	0074000
XW1_145:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 102 (0x66):  width 7  f
;	+-------+
;	|       |
;	|       |
;	|  ***  |
;	| *   * |
;	| *     |
;	| *     |
;	|****   |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	|       |
;	|       |
;	+-------+ 
		dw	0034000
		dw	0042000
		dw	0040000
		dw	0040000
		dw	0170000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
XW1_146:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 103 (0x67):  width 7  g
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| *** * |
;	|*   *  |
;	|*   *  |
;	| ***   |
;	|*      |
;	| ****  |
;	|*    * |
;	| ****  |
;	+-------+ 
		dw	0072000
		dw	0104000
		dw	0104000
		dw	0070000
		dw	0100000
		dw	0074000
		dw	0102000
		dw	0074000
XW1_147:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 8		; HD: 5, XH: 8

; Character 104 (0x68):  width 7  h
;	+-------+
;	|       |
;	|       |
;	|*      |
;	|*      |
;	|*      |
;	|* ***  |
;	|**   * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0134000
		dw	0142000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
XW1_150:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 105 (0x69):  width 7  i
;	+-------+
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|       |
;	|  **   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0000000
		dw	0030000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_151:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 8		; HD: 3, XH: 8

; Character 106 (0x6a):  width 7  j
;	+-------+
;	|       |
;	|       |
;	|       |
;	|     * |
;	|       |
;	|    ** |
;	|     * |
;	|     * |
;	|     * |
;	|     * |
;	| *   * |
;	| *   * |
;	|  ***  |
;	+-------+ 
		dw	0002000
		dw	0000000
		dw	0006000
		dw	0002000
		dw	0002000
		dw	0002000
		dw	0002000
		dw	0042000
		dw	0042000
		dw	0034000
XW1_152:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 10		; HD: 3, XH: 10

; Character 107 (0x6b):  width 7  k
;	+-------+
;	|       |
;	|       |
;	|*      |
;	|*      |
;	|*      |
;	|*   *  |
;	|*  *   |
;	|***    |
;	|*  *   |
;	|*   *  |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0104000
		dw	0110000
		dw	0160000
		dw	0110000
		dw	0104000
		dw	0102000
XW1_153:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 108 (0x6c):  width 7  l
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_154:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 109 (0x6d):  width 7  m
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ** *  |
;	| * * * |
;	| * * * |
;	| * * * |
;	| * * * |
;	| *   * |
;	|       |
;	|       |
;	+-------+ 
		dw	0064000
		dw	0052000
		dw	0052000
		dw	0052000
		dw	0052000
		dw	0042000
XW1_155:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 110 (0x6e):  width 7  n
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|* ***  |
;	|**   * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0134000
		dw	0142000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
XW1_156:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 111 (0x6f):  width 7  o
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_157:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 112 (0x70):  width 7  p
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|* ***  |
;	|**   * |
;	|*    * |
;	|**   * |
;	|* ***  |
;	|*      |
;	|*      |
;	|*      |
;	+-------+ 
		dw	0134000
		dw	0142000
		dw	0102000
		dw	0142000
		dw	0134000
		dw	0100000
		dw	0100000
		dw	0100000
XW1_160:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 8		; HD: 5, XH: 8

; Character 113 (0x71):  width 7  q
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| *** * |
;	|*   ** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|     * |
;	|     * |
;	|     * |
;	+-------+ 
		dw	0072000
		dw	0106000
		dw	0102000
		dw	0106000
		dw	0072000
		dw	0002000
		dw	0002000
		dw	0002000
XW1_161:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 8		; HD: 5, XH: 8

; Character 114 (0x72):  width 7  r
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|* ***  |
;	| *   * |
;	| *     |
;	| *     |
;	| *     |
;	| *     |
;	|       |
;	|       |
;	+-------+ 
		dw	0134000
		dw	0042000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
XW1_162:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 115 (0x73):  width 7  s
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	| **    |
;	|   **  |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0060000
		dw	0014000
		dw	0102000
		dw	0074000
XW1_163:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 116 (0x74):  width 7  t
;	+-------+
;	|       |
;	|       |
;	|       |
;	| *     |
;	| *     |
;	|****   |
;	| *     |
;	| *     |
;	| *     |
;	| *   * |
;	|  ***  |
;	|       |
;	|       |
;	+-------+ 
		dw	0040000
		dw	0040000
		dw	0170000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0042000
		dw	0034000
XW1_164:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 8		; HD: 3, XH: 8

; Character 117 (0x75):  width 7  u
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_165:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 118 (0x76):  width 7  v
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| *   * |
;	| *   * |
;	| *   * |
;	|  * *  |
;	|  * *  |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0024000
		dw	0024000
		dw	0010000
XW1_166:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 119 (0x77):  width 7  w
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| *   * |
;	| *   * |
;	| * * * |
;	| * * * |
;	| * * * |
;	|  * *  |
;	|       |
;	|       |
;	+-------+ 
		dw	0042000
		dw	0042000
		dw	0052000
		dw	0052000
		dw	0052000
		dw	0024000
XW1_167:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 120 (0x78):  width 7  x
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|*    * |
;	| *  *  |
;	|  **   |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0044000
		dw	0030000
		dw	0030000
		dw	0044000
		dw	0102000
XW1_170:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 121 (0x79):  width 7  y
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|     * |
;	|*    * |
;	| ****  |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
		dw	0002000
		dw	0102000
		dw	0074000
XW1_171:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 8		; HD: 5, XH: 8

; Character 122 (0x7a):  width 7  z
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|****** |
;	|    *  |
;	|   *   |
;	|  *    |
;	| *     |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0176000
		dw	0004000
		dw	0010000
		dw	0020000
		dw	0040000
		dw	0176000
XW1_172:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 123 (0x7b):  width 7  braceleft
;	+-------+
;	|       |
;	|   *** |
;	|  *    |
;	|  *    |
;	|  *    |
;	|   *   |
;	| **    |
;	|   *   |
;	|  *    |
;	|  *    |
;	|  *    |
;	|   *** |
;	|       |
;	+-------+ 
		dw	0016000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0010000
		dw	0060000
		dw	0010000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0016000
XW1_173:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 11		; HD: 1, XH: 11

; Character 124 (0x7c):  width 7  bar
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
XW1_174:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 125 (0x7d):  width 7  braceright
;	+-------+
;	|       |
;	| ***   |
;	|    *  |
;	|    *  |
;	|    *  |
;	|   *   |
;	|    ** |
;	|   *   |
;	|    *  |
;	|    *  |
;	|    *  |
;	| ***   |
;	|       |
;	+-------+ 
		dw	0070000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0010000
		dw	0006000
		dw	0010000
		dw	0004000
		dw	0004000
		dw	0004000
		dw	0070000
XW1_175:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 11		; HD: 1, XH: 11

; Character 126 (0x7e):  width 7  asciitilde
;	+-------+
;	|       |
;	|       |
;	|  *  * |
;	| * * * |
;	| *  *  |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0022000
		dw	0052000
		dw	0044000
XW1_176:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 3		; HD: 2, XH: 3

; Character 160 (0xa0):  width 7  space
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
XW1_240:		dw	2 * 7 + 1		; no extent
		dw	(13 << 8) | 0		; HD: 13, XH: 0

; Character 161 (0xa1):  width 7  exclamdown
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|       |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0000000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
XW1_241:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 162 (0xa2):  width 7  cent
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  ***  |
;	| * * * |
;	| * *   |
;	| * *   |
;	| * * * |
;	|  ***  |
;	|   *   |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0034000
		dw	0052000
		dw	0050000
		dw	0050000
		dw	0052000
		dw	0034000
		dw	0010000
XW1_242:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 8		; HD: 2, XH: 8

; Character 163 (0xa3):  width 7  sterling
;	+-------+
;	|       |
;	|       |
;	|  ***  |
;	| *   * |
;	| *     |
;	| *     |
;	|***    |
;	| *     |
;	| *     |
;	| *   * |
;	|* ***  |
;	|       |
;	|       |
;	+-------+ 
		dw	0034000
		dw	0042000
		dw	0040000
		dw	0040000
		dw	0160000
		dw	0040000
		dw	0040000
		dw	0042000
		dw	0134000
XW1_243:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 164 (0xa4):  width 7  currency
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|*    * |
;	| ****  |
;	| *  *  |
;	| *  *  |
;	| ****  |
;	|*    * |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0074000
		dw	0044000
		dw	0044000
		dw	0074000
		dw	0102000
XW1_244:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 6		; HD: 4, XH: 6

; Character 165 (0xa5):  width 7  yen
;	+-------+
;	|       |
;	|       |
;	|*   *  |
;	|*   *  |
;	| * *   |
;	| * *   |
;	|*****  |
;	|  *    |
;	|*****  |
;	|  *    |
;	|  *    |
;	|       |
;	|       |
;	+-------+ 
		dw	0104000
		dw	0104000
		dw	0050000
		dw	0050000
		dw	0174000
		dw	0020000
		dw	0174000
		dw	0020000
		dw	0020000
XW1_245:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 166 (0xa6):  width 7  brokenbar
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0000000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
XW1_246:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 167 (0xa7):  width 7  section
;	+-------+
;	|       |
;	|  **   |
;	| *  *  |
;	| *     |
;	|  **   |
;	| *  *  |
;	| *  *  |
;	|  **   |
;	|    *  |
;	| *  *  |
;	|  **   |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0040000
		dw	0030000
		dw	0044000
		dw	0044000
		dw	0030000
		dw	0004000
		dw	0044000
		dw	0030000
XW1_247:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 168 (0xa8):  width 7  dieresis
;	+-------+
;	|       |
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
XW1_250:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 2		; HD: 2, XH: 2

; Character 169 (0xa9):  width 7  copyright
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|* ** * |
;	|* *  * |
;	|* *  * |
;	|* *  * |
;	|* ** * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0132000
		dw	0122000
		dw	0122000
		dw	0122000
		dw	0132000
		dw	0102000
		dw	0074000
XW1_251:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 170 (0xaa):  width 7  ordfeminine
;	+-------+
;	|       |
;	|       |
;	|  ***  |
;	|     * |
;	|  **** |
;	| *   * |
;	|  **** |
;	|       |
;	| ***** |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0034000
		dw	0002000
		dw	0036000
		dw	0042000
		dw	0036000
		dw	0000000
		dw	0076000
XW1_252:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 7		; HD: 2, XH: 7

; Character 171 (0xab):  width 7  guillemotleft
;	+-------+
;	|       |
;	|       |
;	|       |
;	|   * * |
;	|  * *  |
;	| * *   |
;	|* *    |
;	| * *   |
;	|  * *  |
;	|   * * |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0012000
		dw	0024000
		dw	0050000
		dw	0120000
		dw	0050000
		dw	0024000
		dw	0012000
XW1_253:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 7		; HD: 3, XH: 7

; Character 172 (0xac):  width 7  logicalnot
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ***** |
;	|     * |
;	|     * |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0076000
		dw	0002000
		dw	0002000
XW1_254:		dw	2 * 7 + 1		; no extent
		dw	(6 << 8) | 3		; HD: 6, XH: 3

; Character 173 (0xad):  width 7  hyphen
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ****  |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
XW1_255:		dw	2 * 7 + 1		; no extent
		dw	(6 << 8) | 1		; HD: 6, XH: 1

; Character 174 (0xae):  width 7  registered
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|* ** * |
;	|* * ** |
;	|* * ** |
;	|* ** * |
;	|* * ** |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0132000
		dw	0126000
		dw	0126000
		dw	0132000
		dw	0126000
		dw	0102000
		dw	0074000
XW1_256:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 175 (0xaf):  width 7  macron
;	+-------+
;	|       |
;	|       |
;	| ***** |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0076000
XW1_257:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 1		; HD: 2, XH: 1

; Character 176 (0xb0):  width 7  degree
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	| *  *  |
;	|  **   |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0044000
		dw	0030000
XW1_260:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 177 (0xb1):  width 7  plusminus
;	+-------+
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|   *   |
;	| ***** |
;	|   *   |
;	|   *   |
;	|       |
;	| ***** |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0010000
		dw	0076000
		dw	0010000
		dw	0010000
		dw	0000000
		dw	0076000
XW1_261:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 7		; HD: 3, XH: 7

; Character 178 (0xb2):  width 7  twosuperior
;	+-------+
;	|       |
;	|  *    |
;	| * *   |
;	|   *   |
;	|  *    |
;	| *     |
;	| ***   |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0050000
		dw	0010000
		dw	0020000
		dw	0040000
		dw	0070000
XW1_262:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 6		; HD: 1, XH: 6

; Character 179 (0xb3):  width 7  threesuperior
;	+-------+
;	|       |
;	| ***   |
;	|   *   |
;	|  *    |
;	|   *   |
;	| * *   |
;	|  *    |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0070000
		dw	0010000
		dw	0020000
		dw	0010000
		dw	0050000
		dw	0020000
XW1_263:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 6		; HD: 1, XH: 6

; Character 180 (0xb4):  width 7  acute
;	+-------+
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
XW1_264:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 2		; HD: 1, XH: 2

; Character 181 (0xb5):  width 7  mu
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|**  ** |
;	|* ** * |
;	|*      |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0146000
		dw	0132000
		dw	0100000
XW1_265:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 7		; HD: 5, XH: 7

; Character 182 (0xb6):  width 7  paragraph
;	+-------+
;	|       |
;	|       |
;	| ***** |
;	|*** *  |
;	|*** *  |
;	|*** *  |
;	| ** *  |
;	|  * *  |
;	|  * *  |
;	|  * *  |
;	|  * *  |
;	|       |
;	|       |
;	+-------+ 
		dw	0076000
		dw	0164000
		dw	0164000
		dw	0164000
		dw	0064000
		dw	0024000
		dw	0024000
		dw	0024000
		dw	0024000
XW1_266:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 183 (0xb7):  width 7  periodcentered
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|  **   |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
XW1_267:		dw	2 * 7 + 1		; no extent
		dw	(6 << 8) | 1		; HD: 6, XH: 1

; Character 184 (0xb8):  width 7  cedilla
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|  *    |
;	+-------+ 
		dw	0010000
		dw	0020000
XW1_270:		dw	2 * 7 + 1		; no extent
		dw	(11 << 8) | 2		; HD: 11, XH: 2

; Character 185 (0xb9):  width 7  onesuperior
;	+-------+
;	|       |
;	|  *    |
;	| **    |
;	|  *    |
;	|  *    |
;	|  *    |
;	| ***   |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW1_271:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 6		; HD: 1, XH: 6

; Character 186 (0xba):  width 7  ordmasculine
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	| *  *  |
;	|  **   |
;	|       |
;	| ****  |
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0044000
		dw	0030000
		dw	0000000
		dw	0074000
XW1_272:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 6		; HD: 2, XH: 6

; Character 187 (0xbb):  width 7  guillemotright
;	+-------+
;	|       |
;	|       |
;	|       |
;	|* *    |
;	| * *   |
;	|  * *  |
;	|   * * |
;	|  * *  |
;	| * *   |
;	|* *    |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0120000
		dw	0050000
		dw	0024000
		dw	0012000
		dw	0024000
		dw	0050000
		dw	0120000
XW1_273:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 7		; HD: 3, XH: 7

; Character 188 (0xbc):  width 7  onequarter
;	+-------+
;	|       |
;	| *     |
;	|**     |
;	| *     |
;	| *     |
;	| *   * |
;	|*** ** |
;	|   * * |
;	|   * * |
;	|   *** |
;	|     * |
;	|       |
;	|       |
;	+-------+ 
		dw	0040000
		dw	0140000
		dw	0040000
		dw	0040000
		dw	0042000
		dw	0166000
		dw	0012000
		dw	0012000
		dw	0016000
		dw	0002000
XW1_274:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 189 (0xbd):  width 7  onehalf
;	+-------+
;	|       |
;	| *     |
;	|**     |
;	| *     |
;	| *     |
;	| *  *  |
;	|**** * |
;	|     * |
;	|    *  |
;	|   *   |
;	|   *** |
;	|       |
;	|       |
;	+-------+ 
		dw	0040000
		dw	0140000
		dw	0040000
		dw	0040000
		dw	0044000
		dw	0172000
		dw	0002000
		dw	0004000
		dw	0010000
		dw	0016000
XW1_275:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 190 (0xbe):  width 7  threequarters
;	+-------+
;	|       |
;	|***    |
;	|  *    |
;	| *     |
;	|  *    |
;	|* *  * |
;	| *  ** |
;	|   * * |
;	|   * * |
;	|   *** |
;	|     * |
;	|       |
;	|       |
;	+-------+ 
		dw	0160000
		dw	0020000
		dw	0040000
		dw	0020000
		dw	0122000
		dw	0046000
		dw	0012000
		dw	0012000
		dw	0016000
		dw	0002000
XW1_276:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 191 (0xbf):  width 7  questiondown
;	+-------+
;	|       |
;	|       |
;	|  *    |
;	|       |
;	|  *    |
;	|  *    |
;	| *     |
;	|*      |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0000000
		dw	0020000
		dw	0020000
		dw	0040000
		dw	0100000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_277:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 192 (0xc0):  width 7  Agrave
;	+-------+
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
XW1_300:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 193 (0xc1):  width 7  Aacute
;	+-------+
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
XW1_301:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 194 (0xc2):  width 7  Acircumflex
;	+-------+
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
XW1_302:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 195 (0xc3):  width 7  Atilde
;	+-------+
;	|       |
;	| **  * |
;	|*  **  |
;	|       |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0062000
		dw	0114000
		dw	0000000
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
XW1_303:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 196 (0xc4):  width 7  Adieresis
;	+-------+
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
XW1_304:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 197 (0xc5):  width 7  Aring
;	+-------+
;	|       |
;	|  **   |
;	| *  *  |
;	|  **   |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|*    * |
;	|****** |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0030000
		dw	0030000
		dw	0044000
		dw	0102000
		dw	0102000
		dw	0176000
		dw	0102000
		dw	0102000
XW1_305:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 198 (0xc6):  width 7  AE
;	+-------+
;	|       |
;	|       |
;	| * *** |
;	|* *    |
;	|* *    |
;	|* *    |
;	|* ***  |
;	|***    |
;	|* *    |
;	|* *    |
;	|* **** |
;	|       |
;	|       |
;	+-------+ 
		dw	0056000
		dw	0120000
		dw	0120000
		dw	0120000
		dw	0134000
		dw	0160000
		dw	0120000
		dw	0120000
		dw	0136000
XW1_306:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 199 (0xc7):  width 7  Ccedilla
;	+-------+
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*      |
;	|*    * |
;	| ****  |
;	|   *   |
;	|  *    |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0102000
		dw	0074000
		dw	0010000
		dw	0020000
XW1_307:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 11		; HD: 2, XH: 11

; Character 200 (0xc8):  width 7  Egrave
;	+-------+
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	|****** |
;	|*      |
;	|*      |
;	|****   |
;	|*      |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0176000
		dw	0100000
		dw	0100000
		dw	0170000
		dw	0100000
		dw	0100000
		dw	0176000
XW1_310:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 201 (0xc9):  width 7  Eacute
;	+-------+
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	|****** |
;	|*      |
;	|*      |
;	|****   |
;	|*      |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0176000
		dw	0100000
		dw	0100000
		dw	0170000
		dw	0100000
		dw	0100000
		dw	0176000
XW1_311:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 202 (0xca):  width 7  Ecircumflex
;	+-------+
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	|****** |
;	|*      |
;	|*      |
;	|****   |
;	|*      |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0176000
		dw	0100000
		dw	0100000
		dw	0170000
		dw	0100000
		dw	0100000
		dw	0176000
XW1_312:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 203 (0xcb):  width 7  Edieresis
;	+-------+
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	|****** |
;	|*      |
;	|*      |
;	|****   |
;	|*      |
;	|*      |
;	|****** |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0176000
		dw	0100000
		dw	0100000
		dw	0170000
		dw	0100000
		dw	0100000
		dw	0176000
XW1_313:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 204 (0xcc):  width 7  Igrave
;	+-------+
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	| ***** |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0076000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_314:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 205 (0xcd):  width 7  Iacute
;	+-------+
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	| ***** |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0076000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_315:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 206 (0xce):  width 7  Icircumflex
;	+-------+
;	|       |
;	|   *   |
;	|  * *  |
;	|       |
;	| ***** |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0024000
		dw	0000000
		dw	0076000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_316:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 207 (0xcf):  width 7  Idieresis
;	+-------+
;	|       |
;	| *   * |
;	| *   * |
;	|       |
;	| ***** |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0042000
		dw	0042000
		dw	0000000
		dw	0076000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_317:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 208 (0xd0):  width 7  Eth
;	+-------+
;	|       |
;	|       |
;	|*****  |
;	| *   * |
;	| *   * |
;	| *   * |
;	|***  * |
;	| *   * |
;	| *   * |
;	| *   * |
;	|*****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0174000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0162000
		dw	0042000
		dw	0042000
		dw	0042000
		dw	0174000
XW1_320:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 209 (0xd1):  width 7  Ntilde
;	+-------+
;	|       |
;	| **  * |
;	|*  **  |
;	|       |
;	|*    * |
;	|**   * |
;	|* *  * |
;	|* *  * |
;	|*  * * |
;	|*   ** |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0062000
		dw	0114000
		dw	0000000
		dw	0102000
		dw	0142000
		dw	0122000
		dw	0122000
		dw	0112000
		dw	0106000
		dw	0102000
XW1_321:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 210 (0xd2):  width 7  Ograve
;	+-------+
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_322:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 211 (0xd3):  width 7  Oacute
;	+-------+
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_323:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 212 (0xd4):  width 7  Ocircumflex
;	+-------+
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_324:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 213 (0xd5):  width 7  Otilde
;	+-------+
;	|       |
;	| **  * |
;	|*  **  |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0062000
		dw	0114000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_325:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 214 (0xd6):  width 7  Odieresis
;	+-------+
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_326:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 215 (0xd7):  width 7  multiply
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|*    * |
;	| *  *  |
;	|  **   |
;	|  **   |
;	| *  *  |
;	|*    * |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0102000
		dw	0044000
		dw	0030000
		dw	0030000
		dw	0044000
		dw	0102000
XW1_327:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 6		; HD: 4, XH: 6

; Character 216 (0xd8):  width 7  Oslash
;	+-------+
;	|       |
;	|     * |
;	| ****  |
;	|*   ** |
;	|*  * * |
;	|*  * * |
;	|* *  * |
;	|* *  * |
;	|* *  * |
;	|**   * |
;	| ****  |
;	|*      |
;	|       |
;	+-------+ 
		dw	0002000
		dw	0074000
		dw	0106000
		dw	0112000
		dw	0112000
		dw	0122000
		dw	0122000
		dw	0122000
		dw	0142000
		dw	0074000
		dw	0100000
XW1_330:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 11		; HD: 1, XH: 11

; Character 217 (0xd9):  width 7  Ugrave
;	+-------+
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_331:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 218 (0xda):  width 7  Uacute
;	+-------+
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_332:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 219 (0xdb):  width 7  Ucircumflex
;	+-------+
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_333:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 220 (0xdc):  width 7  Udieresis
;	+-------+
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_334:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 221 (0xdd):  width 7  Yacute
;	+-------+
;	|       |
;	|    *  |
;	|   *   |
;	|       |
;	| *   * |
;	| *   * |
;	|  * *  |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	+-------+ 
		dw	0004000
		dw	0010000
		dw	0000000
		dw	0042000
		dw	0042000
		dw	0024000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
XW1_335:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 222 (0xde):  width 7  Thorn
;	+-------+
;	|       |
;	|       |
;	|*      |
;	|*****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*****  |
;	|*      |
;	|*      |
;	|*      |
;	|       |
;	|       |
;	+-------+ 
		dw	0100000
		dw	0174000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0174000
		dw	0100000
		dw	0100000
		dw	0100000
XW1_336:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 223 (0xdf):  width 7  germandbls
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	| *  *  |
;	| * *   |
;	| * *   |
;	| *  *  |
;	| *   * |
;	| *   * |
;	| * **  |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0044000
		dw	0050000
		dw	0050000
		dw	0044000
		dw	0042000
		dw	0042000
		dw	0054000
XW1_337:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 224 (0xe0):  width 7  agrave
;	+-------+
;	|       |
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	| ****  |
;	|     * |
;	| ***** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0074000
		dw	0002000
		dw	0076000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_340:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 225 (0xe1):  width 7  aacute
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	| ****  |
;	|     * |
;	| ***** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0074000
		dw	0002000
		dw	0076000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_341:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 226 (0xe2):  width 7  acircumflex
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	| ****  |
;	|     * |
;	| ***** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0002000
		dw	0076000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_342:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 227 (0xe3):  width 7  atilde
;	+-------+
;	|       |
;	|       |
;	| **  * |
;	|*  **  |
;	|       |
;	| ****  |
;	|     * |
;	| ***** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0062000
		dw	0114000
		dw	0000000
		dw	0074000
		dw	0002000
		dw	0076000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_343:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 228 (0xe4):  width 7  adieresis
;	+-------+
;	|       |
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	| ****  |
;	|     * |
;	| ***** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0002000
		dw	0076000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_344:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 229 (0xe5):  width 7  aring
;	+-------+
;	|       |
;	|  **   |
;	| *  *  |
;	|  **   |
;	|       |
;	| ****  |
;	|     * |
;	| ***** |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0030000
		dw	0000000
		dw	0074000
		dw	0002000
		dw	0076000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_345:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 230 (0xe6):  width 7  ae
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ** *  |
;	|   * * |
;	| ***** |
;	|*  *   |
;	|*  * * |
;	| ** *  |
;	|       |
;	|       |
;	+-------+ 
		dw	0064000
		dw	0012000
		dw	0076000
		dw	0110000
		dw	0112000
		dw	0064000
XW1_346:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 6		; HD: 5, XH: 6

; Character 231 (0xe7):  width 7  ccedilla
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|       |
;	| ****  |
;	|*    * |
;	|*      |
;	|*      |
;	|*    * |
;	| ****  |
;	|   *   |
;	|  *    |
;	+-------+ 
		dw	0074000
		dw	0102000
		dw	0100000
		dw	0100000
		dw	0102000
		dw	0074000
		dw	0010000
		dw	0020000
XW1_347:		dw	2 * 7 + 1		; no extent
		dw	(5 << 8) | 8		; HD: 5, XH: 8

; Character 232 (0xe8):  width 7  egrave
;	+-------+
;	|       |
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	| ****  |
;	|*    * |
;	|****** |
;	|*      |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0176000
		dw	0100000
		dw	0102000
		dw	0074000
XW1_350:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 233 (0xe9):  width 7  eacute
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	| ****  |
;	|*    * |
;	|****** |
;	|*      |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0176000
		dw	0100000
		dw	0102000
		dw	0074000
XW1_351:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 234 (0xea):  width 7  ecircumflex
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	| ****  |
;	|*    * |
;	|****** |
;	|*      |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0176000
		dw	0100000
		dw	0102000
		dw	0074000
XW1_352:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 235 (0xeb):  width 7  edieresis
;	+-------+
;	|       |
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	| ****  |
;	|*    * |
;	|****** |
;	|*      |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0176000
		dw	0100000
		dw	0102000
		dw	0074000
XW1_353:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 236 (0xec):  width 7  igrave
;	+-------+
;	|       |
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	|  **   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0030000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_354:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 237 (0xed):  width 7  iacute
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	|  **   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0030000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_355:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 238 (0xee):  width 7  icircumflex
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	|  **   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0030000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_356:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 239 (0xef):  width 7  idieresis
;	+-------+
;	|       |
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	|  **   |
;	|   *   |
;	|   *   |
;	|   *   |
;	|   *   |
;	| ***** |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0030000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0076000
XW1_357:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 240 (0xf0):  width 7  eth
;	+-------+
;	|       |
;	| *  *  |
;	|  **   |
;	| * *   |
;	|    *  |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0030000
		dw	0050000
		dw	0004000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_360:		dw	2 * 7 + 1		; no extent
		dw	(1 << 8) | 10		; HD: 1, XH: 10

; Character 241 (0xf1):  width 7  ntilde
;	+-------+
;	|       |
;	|       |
;	| **  * |
;	|*  **  |
;	|       |
;	|* ***  |
;	|**   * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|       |
;	|       |
;	+-------+ 
		dw	0062000
		dw	0114000
		dw	0000000
		dw	0134000
		dw	0142000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
XW1_361:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 242 (0xf2):  width 7  ograve
;	+-------+
;	|       |
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_362:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 243 (0xf3):  width 7  oacute
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_363:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 244 (0xf4):  width 7  ocircumflex
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_364:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 245 (0xf5):  width 7  otilde
;	+-------+
;	|       |
;	|       |
;	| **  * |
;	|*  **  |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0062000
		dw	0114000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_365:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 246 (0xf6):  width 7  odieresis
;	+-------+
;	|       |
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	| ****  |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	| ****  |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0074000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0074000
XW1_366:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 247 (0xf7):  width 7  divide
;	+-------+
;	|       |
;	|       |
;	|       |
;	|   *   |
;	|   *   |
;	|       |
;	| ***** |
;	|       |
;	|   *   |
;	|   *   |
;	|       |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0010000
		dw	0000000
		dw	0076000
		dw	0000000
		dw	0010000
		dw	0010000
XW1_367:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 7		; HD: 3, XH: 7

; Character 248 (0xf8):  width 7  oslash
;	+-------+
;	|       |
;	|       |
;	|       |
;	|       |
;	|     * |
;	| ****  |
;	|*   ** |
;	|*  * * |
;	|* *  * |
;	|**   * |
;	| ****  |
;	|*      |
;	|       |
;	+-------+ 
		dw	0002000
		dw	0074000
		dw	0106000
		dw	0112000
		dw	0122000
		dw	0142000
		dw	0074000
		dw	0100000
XW1_370:		dw	2 * 7 + 1		; no extent
		dw	(4 << 8) | 8		; HD: 4, XH: 8

; Character 249 (0xf9):  width 7  ugrave
;	+-------+
;	|       |
;	|       |
;	|  *    |
;	|   *   |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0020000
		dw	0010000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_371:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 250 (0xfa):  width 7  uacute
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_372:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 251 (0xfb):  width 7  ucircumflex
;	+-------+
;	|       |
;	|       |
;	|  **   |
;	| *  *  |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0030000
		dw	0044000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_373:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 252 (0xfc):  width 7  udieresis
;	+-------+
;	|       |
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|       |
;	|       |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
XW1_374:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 9		; HD: 2, XH: 9

; Character 253 (0xfd):  width 7  yacute
;	+-------+
;	|       |
;	|       |
;	|   *   |
;	|  *    |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|     * |
;	|*    * |
;	| ****  |
;	+-------+ 
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
		dw	0002000
		dw	0102000
		dw	0074000
XW1_375:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 11		; HD: 2, XH: 11

; Character 254 (0xfe):  width 7  thorn
;	+-------+
;	|       |
;	|       |
;	|       |
;	|*      |
;	|*      |
;	|* ***  |
;	|**   * |
;	|*    * |
;	|*    * |
;	|**   * |
;	|* ***  |
;	|*      |
;	|*      |
;	+-------+ 
		dw	0100000
		dw	0100000
		dw	0134000
		dw	0142000
		dw	0102000
		dw	0102000
		dw	0142000
		dw	0134000
		dw	0100000
		dw	0100000
XW1_376:		dw	2 * 7 + 1		; no extent
		dw	(3 << 8) | 10		; HD: 3, XH: 10

; Character 255 (0xff):  width 7  ydieresis
;	+-------+
;	|       |
;	|       |
;	| *  *  |
;	| *  *  |
;	|       |
;	|*    * |
;	|*    * |
;	|*    * |
;	|*   ** |
;	| *** * |
;	|     * |
;	|*    * |
;	| ****  |
;	+-------+ 
		dw	0044000
		dw	0044000
		dw	0000000
		dw	0102000
		dw	0102000
		dw	0102000
		dw	0106000
		dw	0072000
		dw	0002000
		dw	0102000
		dw	0074000
XW1_377:		dw	2 * 7 + 1		; no extent
		dw	(2 << 8) | 11		; HD: 2, XH: 11

; Generated by convbdf on Sat Jun 16 00:52:06 2007.

; =====================================================================
; Font information
;   name        : 5x7
;   facename    : -Misc-Fixed-Medium-R-Normal--7-70-75-75-C-50-ISO10646-1
;   w x h       : 5x7
;   size        : 256
;   ascent      : 6
;   descent     : 1
;   first char  : 0 (0x00)
;   last char   : 255 (0xff)
;   default char: 0 (0x00)
;   proportional: no
;   Public domain font.  Share and enjoy.
; =====================================================================

; font height (header info)
fontheight2:	dw	7
; font type, baseline, maxwidth (header info)
fonttype2:	dw	(0 << 15) | (6 << 8) | 5
fontbase2:	; self relative offsets to character XW2_s
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_040-.,XW2_041-.,XW2_042-.,XW2_043-.
		dw	XW2_044-.,XW2_045-.,XW2_046-.,XW2_047-.
		dw	XW2_050-.,XW2_051-.,XW2_052-.,XW2_053-.
		dw	XW2_054-.,XW2_055-.,XW2_056-.,XW2_057-.
		dw	XW2_060-.,XW2_061-.,XW2_062-.,XW2_063-.
		dw	XW2_064-.,XW2_065-.,XW2_066-.,XW2_067-.
		dw	XW2_070-.,XW2_071-.,XW2_072-.,XW2_073-.
		dw	XW2_074-.,XW2_075-.,XW2_076-.,XW2_077-.
		dw	XW2_100-.,XW2_101-.,XW2_102-.,XW2_103-.
		dw	XW2_104-.,XW2_105-.,XW2_106-.,XW2_107-.
		dw	XW2_110-.,XW2_111-.,XW2_112-.,XW2_113-.
		dw	XW2_114-.,XW2_115-.,XW2_116-.,XW2_117-.
		dw	XW2_120-.,XW2_121-.,XW2_122-.,XW2_123-.
		dw	XW2_124-.,XW2_125-.,XW2_126-.,XW2_127-.
		dw	XW2_130-.,XW2_131-.,XW2_132-.,XW2_133-.
		dw	XW2_134-.,XW2_135-.,XW2_136-.,XW2_137-.
		dw	XW2_140-.,XW2_141-.,XW2_142-.,XW2_143-.
		dw	XW2_144-.,XW2_145-.,XW2_146-.,XW2_147-.
		dw	XW2_150-.,XW2_151-.,XW2_152-.,XW2_153-.
		dw	XW2_154-.,XW2_155-.,XW2_156-.,XW2_157-.
		dw	XW2_160-.,XW2_161-.,XW2_162-.,XW2_163-.
		dw	XW2_164-.,XW2_165-.,XW2_166-.,XW2_167-.
		dw	XW2_170-.,XW2_171-.,XW2_172-.,XW2_173-.
		dw	XW2_174-.,XW2_175-.,XW2_176-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_000-.,XW2_000-.,XW2_000-.,XW2_000-.
		dw	XW2_240-.,XW2_241-.,XW2_242-.,XW2_243-.
		dw	XW2_244-.,XW2_245-.,XW2_246-.,XW2_247-.
		dw	XW2_250-.,XW2_251-.,XW2_252-.,XW2_253-.
		dw	XW2_254-.,XW2_255-.,XW2_256-.,XW2_257-.
		dw	XW2_260-.,XW2_261-.,XW2_262-.,XW2_263-.
		dw	XW2_264-.,XW2_265-.,XW2_266-.,XW2_267-.
		dw	XW2_270-.,XW2_271-.,XW2_272-.,XW2_273-.
		dw	XW2_274-.,XW2_275-.,XW2_276-.,XW2_277-.
		dw	XW2_300-.,XW2_301-.,XW2_302-.,XW2_303-.
		dw	XW2_304-.,XW2_305-.,XW2_306-.,XW2_307-.
		dw	XW2_310-.,XW2_311-.,XW2_312-.,XW2_313-.
		dw	XW2_314-.,XW2_315-.,XW2_316-.,XW2_317-.
		dw	XW2_320-.,XW2_321-.,XW2_322-.,XW2_323-.
		dw	XW2_324-.,XW2_325-.,XW2_326-.,XW2_327-.
		dw	XW2_330-.,XW2_331-.,XW2_332-.,XW2_333-.
		dw	XW2_334-.,XW2_335-.,XW2_336-.,XW2_337-.
		dw	XW2_340-.,XW2_341-.,XW2_342-.,XW2_343-.
		dw	XW2_344-.,XW2_345-.,XW2_346-.,XW2_347-.
		dw	XW2_350-.,XW2_351-.,XW2_352-.,XW2_353-.
		dw	XW2_354-.,XW2_355-.,XW2_356-.,XW2_357-.
		dw	XW2_360-.,XW2_361-.,XW2_362-.,XW2_363-.
		dw	XW2_364-.,XW2_365-.,XW2_366-.,XW2_367-.
		dw	XW2_370-.,XW2_371-.,XW2_372-.,XW2_373-.
		dw	XW2_374-.,XW2_375-.,XW2_376-.,XW2_377-.

; Font bitmap data

; Character 0 (0x00):  width 5  char0
;	+-----+
;	|     |
;	|* * *|
;	|     |
;	|*   *|
;	|     |
;	|* * *|
;	|     |
;	+-----+ 
		dw	0124000
		dw	0000000
		dw	0104000
		dw	0000000
		dw	0124000
XW2_000:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 32 (0x20):  width 5  space
;	+-----+
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
XW2_040:		dw	2 * 5 + 1		; no extent
		dw	(7 << 8) | 0		; HD: 7, XH: 0

; Character 33 (0x21):  width 5  exclam
;	+-----+
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|     |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0000000
		dw	0020000
XW2_041:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 34 (0x22):  width 5  quotedbl
;	+-----+
;	| * * |
;	| * * |
;	| * * |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0050000
		dw	0050000
XW2_042:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 3		; HD: 0, XH: 3

; Character 35 (0x23):  width 5  numbersign
;	+-----+
;	|     |
;	| * * |
;	|*****|
;	| * * |
;	|*****|
;	| * * |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0174000
		dw	0050000
		dw	0174000
		dw	0050000
XW2_043:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 36 (0x24):  width 5  dollar
;	+-----+
;	|     |
;	| *** |
;	|* *  |
;	| *** |
;	|  * *|
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0120000
		dw	0070000
		dw	0024000
		dw	0070000
XW2_044:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 37 (0x25):  width 5  percent
;	+-----+
;	|*    |
;	|*  * |
;	|  *  |
;	| *   |
;	|*  * |
;	|   * |
;	|     |
;	+-----+ 
		dw	0100000
		dw	0110000
		dw	0020000
		dw	0040000
		dw	0110000
		dw	0010000
XW2_045:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 38 (0x26):  width 5  ampersand
;	+-----+
;	|     |
;	| *   |
;	|* *  |
;	| *   |
;	|* *  |
;	| * * |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0120000
		dw	0040000
		dw	0120000
		dw	0050000
XW2_046:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 39 (0x27):  width 5  quotesingle
;	+-----+
;	|  *  |
;	|  *  |
;	|  *  |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0020000
		dw	0020000
XW2_047:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 3		; HD: 0, XH: 3

; Character 40 (0x28):  width 5  parenleft
;	+-----+
;	|  *  |
;	| *   |
;	| *   |
;	| *   |
;	| *   |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0020000
XW2_050:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 41 (0x29):  width 5  parenright
;	+-----+
;	| *   |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *   |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0040000
XW2_051:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 42 (0x2a):  width 5  asterisk
;	+-----+
;	|     |
;	| * * |
;	|  *  |
;	| *** |
;	|  *  |
;	| * * |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0020000
		dw	0070000
		dw	0020000
		dw	0050000
XW2_052:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 43 (0x2b):  width 5  plus
;	+-----+
;	|     |
;	|  *  |
;	|  *  |
;	|*****|
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0020000
		dw	0174000
		dw	0020000
		dw	0020000
XW2_053:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 44 (0x2c):  width 5  comma
;	+-----+
;	|     |
;	|     |
;	|     |
;	|     |
;	|  ** |
;	|  *  |
;	| *   |
;	+-----+ 
		dw	0030000
		dw	0020000
		dw	0040000
XW2_054:		dw	2 * 5 + 1		; no extent
		dw	(4 << 8) | 3		; HD: 4, XH: 3

; Character 45 (0x2d):  width 5  hyphen
;	+-----+
;	|     |
;	|     |
;	|     |
;	|**** |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0170000
XW2_055:		dw	2 * 5 + 1		; no extent
		dw	(3 << 8) | 1		; HD: 3, XH: 1

; Character 46 (0x2e):  width 5  period
;	+-----+
;	|     |
;	|     |
;	|     |
;	|     |
;	| **  |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0060000
XW2_056:		dw	2 * 5 + 1		; no extent
		dw	(4 << 8) | 2		; HD: 4, XH: 2

; Character 47 (0x2f):  width 5  slash
;	+-----+
;	|     |
;	|   * |
;	|  *  |
;	| *   |
;	|*    |
;	|     |
;	|     |
;	+-----+ 
		dw	0010000
		dw	0020000
		dw	0040000
		dw	0100000
XW2_057:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 4		; HD: 1, XH: 4

; Character 48 (0x30):  width 5  zero
;	+-----+
;	|  *  |
;	| * * |
;	| * * |
;	| * * |
;	| * * |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0050000
		dw	0050000
		dw	0050000
		dw	0050000
		dw	0020000
XW2_060:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 49 (0x31):  width 5  one
;	+-----+
;	|  *  |
;	| **  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_061:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 50 (0x32):  width 5  two
;	+-----+
;	| **  |
;	|*  * |
;	|   * |
;	|  *  |
;	| *   |
;	|**** |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0010000
		dw	0020000
		dw	0040000
		dw	0170000
XW2_062:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 51 (0x33):  width 5  three
;	+-----+
;	|**** |
;	|   * |
;	| **  |
;	|   * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0010000
		dw	0060000
		dw	0010000
		dw	0110000
		dw	0060000
XW2_063:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 52 (0x34):  width 5  four
;	+-----+
;	|  *  |
;	| **  |
;	|* *  |
;	|**** |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0060000
		dw	0120000
		dw	0170000
		dw	0020000
		dw	0020000
XW2_064:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 53 (0x35):  width 5  five
;	+-----+
;	|**** |
;	|*    |
;	|***  |
;	|   * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0100000
		dw	0160000
		dw	0010000
		dw	0110000
		dw	0060000
XW2_065:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 54 (0x36):  width 5  six
;	+-----+
;	| **  |
;	|*    |
;	|***  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0100000
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_066:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 55 (0x37):  width 5  seven
;	+-----+
;	|**** |
;	|   * |
;	|  *  |
;	|  *  |
;	| *   |
;	| *   |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0010000
		dw	0020000
		dw	0020000
		dw	0040000
		dw	0040000
XW2_067:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 56 (0x38):  width 5  eight
;	+-----+
;	| **  |
;	|*  * |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_070:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 57 (0x39):  width 5  nine
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	| *** |
;	|   * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0070000
		dw	0010000
		dw	0060000
XW2_071:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 58 (0x3a):  width 5  colon
;	+-----+
;	|     |
;	| **  |
;	| **  |
;	|     |
;	| **  |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0060000
		dw	0000000
		dw	0060000
		dw	0060000
XW2_072:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 59 (0x3b):  width 5  semicolon
;	+-----+
;	|     |
;	| **  |
;	| **  |
;	|     |
;	| **  |
;	| *   |
;	|*    |
;	+-----+ 
		dw	0060000
		dw	0060000
		dw	0000000
		dw	0060000
		dw	0040000
		dw	0100000
XW2_073:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 6		; HD: 1, XH: 6

; Character 60 (0x3c):  width 5  less
;	+-----+
;	|     |
;	|   * |
;	|  *  |
;	| *   |
;	|  *  |
;	|   * |
;	|     |
;	+-----+ 
		dw	0010000
		dw	0020000
		dw	0040000
		dw	0020000
		dw	0010000
XW2_074:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 61 (0x3d):  width 5  equal
;	+-----+
;	|     |
;	|     |
;	|**** |
;	|     |
;	|**** |
;	|     |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0000000
		dw	0170000
XW2_075:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 3		; HD: 2, XH: 3

; Character 62 (0x3e):  width 5  greater
;	+-----+
;	|     |
;	| *   |
;	|  *  |
;	|   * |
;	|  *  |
;	| *   |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0010000
		dw	0020000
		dw	0040000
XW2_076:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 63 (0x3f):  width 5  question
;	+-----+
;	|  *  |
;	| * * |
;	|   * |
;	|  *  |
;	|     |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0050000
		dw	0010000
		dw	0020000
		dw	0000000
		dw	0020000
XW2_077:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 64 (0x40):  width 5  at
;	+-----+
;	| **  |
;	|*  * |
;	|* ** |
;	|* ** |
;	|*    |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0130000
		dw	0130000
		dw	0100000
		dw	0060000
XW2_100:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 65 (0x41):  width 5  A
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_101:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 66 (0x42):  width 5  B
;	+-----+
;	|***  |
;	|*  * |
;	|***  |
;	|*  * |
;	|*  * |
;	|***  |
;	|     |
;	+-----+ 
		dw	0160000
		dw	0110000
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0160000
XW2_102:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 67 (0x43):  width 5  C
;	+-----+
;	| **  |
;	|*  * |
;	|*    |
;	|*    |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0100000
		dw	0100000
		dw	0110000
		dw	0060000
XW2_103:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 68 (0x44):  width 5  D
;	+-----+
;	|***  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	|***  |
;	|     |
;	+-----+ 
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0160000
XW2_104:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 69 (0x45):  width 5  E
;	+-----+
;	|**** |
;	|*    |
;	|***  |
;	|*    |
;	|*    |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0100000
		dw	0160000
		dw	0100000
		dw	0100000
		dw	0170000
XW2_105:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 70 (0x46):  width 5  F
;	+-----+
;	|**** |
;	|*    |
;	|***  |
;	|*    |
;	|*    |
;	|*    |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0100000
		dw	0160000
		dw	0100000
		dw	0100000
		dw	0100000
XW2_106:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 71 (0x47):  width 5  G
;	+-----+
;	| **  |
;	|*  * |
;	|*    |
;	|* ** |
;	|*  * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0100000
		dw	0130000
		dw	0110000
		dw	0070000
XW2_107:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 72 (0x48):  width 5  H
;	+-----+
;	|*  * |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
		dw	0110000
XW2_110:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 73 (0x49):  width 5  I
;	+-----+
;	| *** |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_111:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 74 (0x4a):  width 5  J
;	+-----+
;	|   * |
;	|   * |
;	|   * |
;	|   * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0110000
		dw	0060000
XW2_112:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 75 (0x4b):  width 5  K
;	+-----+
;	|*  * |
;	|* *  |
;	|**   |
;	|**   |
;	|* *  |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0120000
		dw	0140000
		dw	0140000
		dw	0120000
		dw	0110000
XW2_113:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 76 (0x4c):  width 5  L
;	+-----+
;	|*    |
;	|*    |
;	|*    |
;	|*    |
;	|*    |
;	|**** |
;	|     |
;	+-----+ 
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0170000
XW2_114:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 77 (0x4d):  width 5  M
;	+-----+
;	|*  * |
;	|**** |
;	|**** |
;	|*  * |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0170000
		dw	0170000
		dw	0110000
		dw	0110000
		dw	0110000
XW2_115:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 78 (0x4e):  width 5  N
;	+-----+
;	|*  * |
;	|** * |
;	|** * |
;	|* ** |
;	|* ** |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0150000
		dw	0150000
		dw	0130000
		dw	0130000
		dw	0110000
XW2_116:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 79 (0x4f):  width 5  O
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_117:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 80 (0x50):  width 5  P
;	+-----+
;	|***  |
;	|*  * |
;	|*  * |
;	|***  |
;	|*    |
;	|*    |
;	|     |
;	+-----+ 
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0160000
		dw	0100000
		dw	0100000
XW2_120:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 81 (0x51):  width 5  Q
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|** * |
;	| **  |
;	|   * |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0150000
		dw	0060000
		dw	0010000
XW2_121:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 82 (0x52):  width 5  R
;	+-----+
;	|***  |
;	|*  * |
;	|*  * |
;	|***  |
;	|* *  |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0160000
		dw	0120000
		dw	0110000
XW2_122:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 83 (0x53):  width 5  S
;	+-----+
;	| **  |
;	|*  * |
;	| *   |
;	|  *  |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0040000
		dw	0020000
		dw	0110000
		dw	0060000
XW2_123:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 84 (0x54):  width 5  T
;	+-----+
;	| *** |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
XW2_124:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 85 (0x55):  width 5  U
;	+-----+
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_125:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 86 (0x56):  width 5  V
;	+-----+
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	| **  |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
		dw	0060000
XW2_126:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 87 (0x57):  width 5  W
;	+-----+
;	|*  * |
;	|*  * |
;	|*  * |
;	|**** |
;	|**** |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0170000
		dw	0110000
XW2_127:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 88 (0x58):  width 5  X
;	+-----+
;	|*  * |
;	|*  * |
;	| **  |
;	| **  |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0060000
		dw	0060000
		dw	0110000
		dw	0110000
XW2_130:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 89 (0x59):  width 5  Y
;	+-----+
;	| * * |
;	| * * |
;	| * * |
;	|  *  |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0050000
		dw	0050000
		dw	0020000
		dw	0020000
		dw	0020000
XW2_131:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 90 (0x5a):  width 5  Z
;	+-----+
;	|**** |
;	|   * |
;	|  *  |
;	| *   |
;	|*    |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0010000
		dw	0020000
		dw	0040000
		dw	0100000
		dw	0170000
XW2_132:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 91 (0x5b):  width 5  bracketleft
;	+-----+
;	| *** |
;	| *   |
;	| *   |
;	| *   |
;	| *   |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0040000
		dw	0070000
XW2_133:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 92 (0x5c):  width 5  backslash
;	+-----+
;	|     |
;	|*    |
;	| *   |
;	|  *  |
;	|   * |
;	|     |
;	|     |
;	+-----+ 
		dw	0100000
		dw	0040000
		dw	0020000
		dw	0010000
XW2_134:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 4		; HD: 1, XH: 4

; Character 93 (0x5d):  width 5  bracketright
;	+-----+
;	| *** |
;	|   * |
;	|   * |
;	|   * |
;	|   * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0070000
XW2_135:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 94 (0x5e):  width 5  asciicircum
;	+-----+
;	|  *  |
;	| * * |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0050000
XW2_136:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 2		; HD: 0, XH: 2

; Character 95 (0x5f):  width 5  underscore
;	+-----+
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
XW2_137:		dw	2 * 5 + 1		; no extent
		dw	(5 << 8) | 1		; HD: 5, XH: 1

; Character 96 (0x60):  width 5  grave
;	+-----+
;	| *   |
;	|  *  |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
XW2_140:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 2		; HD: 0, XH: 2

; Character 97 (0x61):  width 5  a
;	+-----+
;	|     |
;	|     |
;	| *** |
;	|*  * |
;	|* ** |
;	| * * |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0110000
		dw	0130000
		dw	0050000
XW2_141:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 98 (0x62):  width 5  b
;	+-----+
;	|*    |
;	|*    |
;	|***  |
;	|*  * |
;	|*  * |
;	|***  |
;	|     |
;	+-----+ 
		dw	0100000
		dw	0100000
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0160000
XW2_142:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 99 (0x63):  width 5  c
;	+-----+
;	|     |
;	|     |
;	| **  |
;	|*    |
;	|*    |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0100000
		dw	0100000
		dw	0060000
XW2_143:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 100 (0x64):  width 5  d
;	+-----+
;	|   * |
;	|   * |
;	| *** |
;	|*  * |
;	|*  * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0010000
		dw	0010000
		dw	0070000
		dw	0110000
		dw	0110000
		dw	0070000
XW2_144:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 101 (0x65):  width 5  e
;	+-----+
;	|     |
;	|     |
;	| **  |
;	|* ** |
;	|**   |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0130000
		dw	0140000
		dw	0060000
XW2_145:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 102 (0x66):  width 5  f
;	+-----+
;	|  *  |
;	| * * |
;	| *   |
;	|***  |
;	| *   |
;	| *   |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0050000
		dw	0040000
		dw	0160000
		dw	0040000
		dw	0040000
XW2_146:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 103 (0x67):  width 5  g
;	+-----+
;	|     |
;	|     |
;	| *** |
;	|*  * |
;	| **  |
;	|*    |
;	| *** |
;	+-----+ 
		dw	0070000
		dw	0110000
		dw	0060000
		dw	0100000
		dw	0070000
XW2_147:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 5		; HD: 2, XH: 5

; Character 104 (0x68):  width 5  h
;	+-----+
;	|*    |
;	|*    |
;	|***  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0100000
		dw	0100000
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0110000
XW2_150:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 105 (0x69):  width 5  i
;	+-----+
;	|  *  |
;	|     |
;	| **  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0000000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_151:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 106 (0x6a):  width 5  j
;	+-----+
;	|   * |
;	|     |
;	|   * |
;	|   * |
;	|   * |
;	| * * |
;	|  *  |
;	+-----+ 
		dw	0010000
		dw	0000000
		dw	0010000
		dw	0010000
		dw	0010000
		dw	0050000
		dw	0020000
XW2_152:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 107 (0x6b):  width 5  k
;	+-----+
;	|*    |
;	|*    |
;	|* *  |
;	|**   |
;	|* *  |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0100000
		dw	0100000
		dw	0120000
		dw	0140000
		dw	0120000
		dw	0110000
XW2_153:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 108 (0x6c):  width 5  l
;	+-----+
;	| **  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_154:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 109 (0x6d):  width 5  m
;	+-----+
;	|     |
;	|     |
;	|* *  |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0120000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_155:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 110 (0x6e):  width 5  n
;	+-----+
;	|     |
;	|     |
;	|***  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0110000
XW2_156:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 111 (0x6f):  width 5  o
;	+-----+
;	|     |
;	|     |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_157:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 112 (0x70):  width 5  p
;	+-----+
;	|     |
;	|     |
;	|***  |
;	|*  * |
;	|*  * |
;	|***  |
;	|*    |
;	+-----+ 
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0160000
		dw	0100000
XW2_160:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 5		; HD: 2, XH: 5

; Character 113 (0x71):  width 5  q
;	+-----+
;	|     |
;	|     |
;	| *** |
;	|*  * |
;	|*  * |
;	| *** |
;	|   * |
;	+-----+ 
		dw	0070000
		dw	0110000
		dw	0110000
		dw	0070000
		dw	0010000
XW2_161:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 5		; HD: 2, XH: 5

; Character 114 (0x72):  width 5  r
;	+-----+
;	|     |
;	|     |
;	|***  |
;	|*  * |
;	|*    |
;	|*    |
;	|     |
;	+-----+ 
		dw	0160000
		dw	0110000
		dw	0100000
		dw	0100000
XW2_162:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 115 (0x73):  width 5  s
;	+-----+
;	|     |
;	|     |
;	| *** |
;	|**   |
;	|  ** |
;	|***  |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0140000
		dw	0030000
		dw	0160000
XW2_163:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 116 (0x74):  width 5  t
;	+-----+
;	| *   |
;	| *   |
;	|***  |
;	| *   |
;	| *   |
;	|  ** |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0040000
		dw	0160000
		dw	0040000
		dw	0040000
		dw	0030000
XW2_164:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 117 (0x75):  width 5  u
;	+-----+
;	|     |
;	|     |
;	|*  * |
;	|*  * |
;	|*  * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0070000
XW2_165:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 118 (0x76):  width 5  v
;	+-----+
;	|     |
;	|     |
;	| * * |
;	| * * |
;	| * * |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0050000
		dw	0050000
		dw	0020000
XW2_166:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 119 (0x77):  width 5  w
;	+-----+
;	|     |
;	|     |
;	|*  * |
;	|*  * |
;	|**** |
;	|**** |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0170000
XW2_167:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 120 (0x78):  width 5  x
;	+-----+
;	|     |
;	|     |
;	|*  * |
;	| **  |
;	| **  |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0060000
		dw	0060000
		dw	0110000
XW2_170:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 121 (0x79):  width 5  y
;	+-----+
;	|     |
;	|     |
;	|*  * |
;	|*  * |
;	| * * |
;	|  *  |
;	| *   |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0050000
		dw	0020000
		dw	0040000
XW2_171:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 5		; HD: 2, XH: 5

; Character 122 (0x7a):  width 5  z
;	+-----+
;	|     |
;	|     |
;	|**** |
;	|  *  |
;	| *   |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0020000
		dw	0040000
		dw	0170000
XW2_172:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 123 (0x7b):  width 5  braceleft
;	+-----+
;	|   * |
;	|  *  |
;	| **  |
;	|  *  |
;	|  *  |
;	|   * |
;	|     |
;	+-----+ 
		dw	0010000
		dw	0020000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0010000
XW2_173:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 124 (0x7c):  width 5  bar
;	+-----+
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
XW2_174:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 125 (0x7d):  width 5  braceright
;	+-----+
;	| *   |
;	|  *  |
;	|  ** |
;	|  *  |
;	|  *  |
;	| *   |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0030000
		dw	0020000
		dw	0020000
		dw	0040000
XW2_175:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 126 (0x7e):  width 5  asciitilde
;	+-----+
;	| * * |
;	|* *  |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0120000
XW2_176:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 2		; HD: 0, XH: 2

; Character 160 (0xa0):  width 5  space
;	+-----+
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
XW2_240:		dw	2 * 5 + 1		; no extent
		dw	(7 << 8) | 0		; HD: 7, XH: 0

; Character 161 (0xa1):  width 5  exclamdown
;	+-----+
;	|  *  |
;	|     |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0000000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
XW2_241:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 162 (0xa2):  width 5  cent
;	+-----+
;	|     |
;	|  *  |
;	| *** |
;	|* *  |
;	|* *  |
;	| *** |
;	|  *  |
;	+-----+ 
		dw	0020000
		dw	0070000
		dw	0120000
		dw	0120000
		dw	0070000
		dw	0020000
XW2_242:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 6		; HD: 1, XH: 6

; Character 163 (0xa3):  width 5  sterling
;	+-----+
;	|     |
;	|  ** |
;	| *   |
;	|***  |
;	| *   |
;	|* ** |
;	|     |
;	+-----+ 
		dw	0030000
		dw	0040000
		dw	0160000
		dw	0040000
		dw	0130000
XW2_243:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 164 (0xa4):  width 5  currency
;	+-----+
;	|     |
;	|*   *|
;	| *** |
;	| * * |
;	| *** |
;	|*   *|
;	|     |
;	+-----+ 
		dw	0104000
		dw	0070000
		dw	0050000
		dw	0070000
		dw	0104000
XW2_244:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 165 (0xa5):  width 5  yen
;	+-----+
;	| * * |
;	| * * |
;	|  *  |
;	| *** |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0050000
		dw	0020000
		dw	0070000
		dw	0020000
		dw	0020000
XW2_245:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 166 (0xa6):  width 5  brokenbar
;	+-----+
;	|     |
;	|  *  |
;	|  *  |
;	|     |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0020000
		dw	0000000
		dw	0020000
		dw	0020000
XW2_246:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 167 (0xa7):  width 5  section
;	+-----+
;	|  ** |
;	| *   |
;	| **  |
;	| * * |
;	|  ** |
;	|   * |
;	| **  |
;	+-----+ 
		dw	0030000
		dw	0040000
		dw	0060000
		dw	0050000
		dw	0030000
		dw	0010000
		dw	0060000
XW2_247:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 168 (0xa8):  width 5  dieresis
;	+-----+
;	| * * |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0050000
XW2_250:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 1		; HD: 0, XH: 1

; Character 169 (0xa9):  width 5  copyright
;	+-----+
;	| *** |
;	|*   *|
;	|* * *|
;	|**  *|
;	|* * *|
;	|*   *|
;	| *** |
;	+-----+ 
		dw	0070000
		dw	0104000
		dw	0124000
		dw	0144000
		dw	0124000
		dw	0104000
		dw	0070000
XW2_251:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 170 (0xaa):  width 5  ordfeminine
;	+-----+
;	| **  |
;	|* *  |
;	| **  |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0120000
		dw	0060000
XW2_252:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 3		; HD: 0, XH: 3

; Character 171 (0xab):  width 5  guillemotleft
;	+-----+
;	|     |
;	|     |
;	| *  *|
;	|*  * |
;	| *  *|
;	|     |
;	|     |
;	+-----+ 
		dw	0044000
		dw	0110000
		dw	0044000
XW2_253:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 3		; HD: 2, XH: 3

; Character 172 (0xac):  width 5  logicalnot
;	+-----+
;	|     |
;	|     |
;	|     |
;	|**** |
;	|   * |
;	|     |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0010000
XW2_254:		dw	2 * 5 + 1		; no extent
		dw	(3 << 8) | 2		; HD: 3, XH: 2

; Character 173 (0xad):  width 5  hyphen
;	+-----+
;	|     |
;	|     |
;	|     |
;	| *** |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0070000
XW2_255:		dw	2 * 5 + 1		; no extent
		dw	(3 << 8) | 1		; HD: 3, XH: 1

; Character 174 (0xae):  width 5  registered
;	+-----+
;	| *** |
;	|*   *|
;	|*** *|
;	|**  *|
;	|**  *|
;	|*   *|
;	| *** |
;	+-----+ 
		dw	0070000
		dw	0104000
		dw	0164000
		dw	0144000
		dw	0144000
		dw	0104000
		dw	0070000
XW2_256:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 175 (0xaf):  width 5  macron
;	+-----+
;	|**** |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0170000
XW2_257:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 1		; HD: 0, XH: 1

; Character 176 (0xb0):  width 5  degree
;	+-----+
;	|  *  |
;	| * * |
;	|  *  |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0050000
		dw	0020000
XW2_260:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 3		; HD: 0, XH: 3

; Character 177 (0xb1):  width 5  plusminus
;	+-----+
;	|  *  |
;	|  *  |
;	|*****|
;	|  *  |
;	|  *  |
;	|*****|
;	|     |
;	+-----+ 
		dw	0020000
		dw	0020000
		dw	0174000
		dw	0020000
		dw	0020000
		dw	0174000
XW2_261:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 178 (0xb2):  width 5  twosuperior
;	+-----+
;	| **  |
;	|  *  |
;	| *   |
;	| **  |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0020000
		dw	0040000
		dw	0060000
XW2_262:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 4		; HD: 0, XH: 4

; Character 179 (0xb3):  width 5  threesuperior
;	+-----+
;	| **  |
;	| **  |
;	|  *  |
;	| **  |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0060000
		dw	0020000
		dw	0060000
XW2_263:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 4		; HD: 0, XH: 4

; Character 180 (0xb4):  width 5  acute
;	+-----+
;	|  *  |
;	| *   |
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0040000
XW2_264:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 2		; HD: 0, XH: 2

; Character 181 (0xb5):  width 5  mu
;	+-----+
;	|     |
;	|     |
;	|*  * |
;	|*  * |
;	|*  * |
;	|***  |
;	|*    |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0160000
		dw	0100000
XW2_265:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 5		; HD: 2, XH: 5

; Character 182 (0xb6):  width 5  paragraph
;	+-----+
;	| *** |
;	|** * |
;	|** * |
;	| * * |
;	| * * |
;	| * * |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0150000
		dw	0150000
		dw	0050000
		dw	0050000
		dw	0050000
XW2_266:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 183 (0xb7):  width 5  periodcentered
;	+-----+
;	|     |
;	|     |
;	| **  |
;	| **  |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0060000
XW2_267:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 2		; HD: 2, XH: 2

; Character 184 (0xb8):  width 5  cedilla
;	+-----+
;	|     |
;	|     |
;	|     |
;	|     |
;	|     |
;	|  *  |
;	| *   |
;	+-----+ 
		dw	0020000
		dw	0040000
XW2_270:		dw	2 * 5 + 1		; no extent
		dw	(5 << 8) | 2		; HD: 5, XH: 2

; Character 185 (0xb9):  width 5  onesuperior
;	+-----+
;	|  *  |
;	| **  |
;	|  *  |
;	| *** |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0060000
		dw	0020000
		dw	0070000
XW2_271:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 4		; HD: 0, XH: 4

; Character 186 (0xba):  width 5  ordmasculine
;	+-----+
;	| *   |
;	|* *  |
;	| *   |
;	|     |
;	|     |
;	|     |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0120000
		dw	0040000
XW2_272:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 3		; HD: 0, XH: 3

; Character 187 (0xbb):  width 5  guillemotright
;	+-----+
;	|     |
;	|     |
;	|*  * |
;	| *  *|
;	|*  * |
;	|     |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0044000
		dw	0110000
XW2_273:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 3		; HD: 2, XH: 3

; Character 188 (0xbc):  width 5  onequarter
;	+-----+
;	|*    |
;	|*    |
;	|*    |
;	|*  * |
;	|  ** |
;	| *** |
;	|   * |
;	+-----+ 
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0110000
		dw	0030000
		dw	0070000
		dw	0010000
XW2_274:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 189 (0xbd):  width 5  onehalf
;	+-----+
;	|*    |
;	|*    |
;	|*    |
;	|* ** |
;	|   * |
;	|  *  |
;	|  ** |
;	+-----+ 
		dw	0100000
		dw	0100000
		dw	0100000
		dw	0130000
		dw	0010000
		dw	0020000
		dw	0030000
XW2_275:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 190 (0xbe):  width 5  threequarters
;	+-----+
;	|**   |
;	|**   |
;	| *   |
;	|** * |
;	|  ** |
;	| *** |
;	|   * |
;	+-----+ 
		dw	0140000
		dw	0140000
		dw	0040000
		dw	0150000
		dw	0030000
		dw	0070000
		dw	0010000
XW2_276:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 191 (0xbf):  width 5  questiondown
;	+-----+
;	|  *  |
;	|     |
;	|  *  |
;	| *   |
;	| * * |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0000000
		dw	0020000
		dw	0040000
		dw	0050000
		dw	0020000
XW2_277:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 192 (0xc0):  width 5  Agrave
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_300:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 193 (0xc1):  width 5  Aacute
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_301:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 194 (0xc2):  width 5  Acircumflex
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_302:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 195 (0xc3):  width 5  Atilde
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_303:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 196 (0xc4):  width 5  Adieresis
;	+-----+
;	|*  * |
;	| **  |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0060000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_304:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 197 (0xc5):  width 5  Aring
;	+-----+
;	| **  |
;	| **  |
;	|*  * |
;	|**** |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0060000
		dw	0110000
		dw	0170000
		dw	0110000
		dw	0110000
XW2_305:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 198 (0xc6):  width 5  AE
;	+-----+
;	| *** |
;	|* *  |
;	|* ** |
;	|***  |
;	|* *  |
;	|* ** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0120000
		dw	0130000
		dw	0160000
		dw	0120000
		dw	0130000
XW2_306:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 199 (0xc7):  width 5  Ccedilla
;	+-----+
;	| **  |
;	|*  * |
;	|*    |
;	|*    |
;	|*  * |
;	| **  |
;	| *   |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0100000
		dw	0100000
		dw	0110000
		dw	0060000
		dw	0040000
XW2_307:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 200 (0xc8):  width 5  Egrave
;	+-----+
;	|**** |
;	|*    |
;	|***  |
;	|*    |
;	|*    |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0100000
		dw	0160000
		dw	0100000
		dw	0100000
		dw	0170000
XW2_310:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 201 (0xc9):  width 5  Eacute
;	+-----+
;	|**** |
;	|*    |
;	|***  |
;	|*    |
;	|*    |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0100000
		dw	0160000
		dw	0100000
		dw	0100000
		dw	0170000
XW2_311:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 202 (0xca):  width 5  Ecircumflex
;	+-----+
;	|**** |
;	|*    |
;	|***  |
;	|*    |
;	|*    |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0100000
		dw	0160000
		dw	0100000
		dw	0100000
		dw	0170000
XW2_312:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 203 (0xcb):  width 5  Edieresis
;	+-----+
;	|**** |
;	|*    |
;	|***  |
;	|*    |
;	|*    |
;	|**** |
;	|     |
;	+-----+ 
		dw	0170000
		dw	0100000
		dw	0160000
		dw	0100000
		dw	0100000
		dw	0170000
XW2_313:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 204 (0xcc):  width 5  Igrave
;	+-----+
;	| *** |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_314:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 205 (0xcd):  width 5  Iacute
;	+-----+
;	| *** |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_315:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 206 (0xce):  width 5  Icircumflex
;	+-----+
;	| *** |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_316:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 207 (0xcf):  width 5  Idieresis
;	+-----+
;	| *** |
;	|  *  |
;	|  *  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_317:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 208 (0xd0):  width 5  Eth
;	+-----+
;	|***  |
;	| * * |
;	|** * |
;	| * * |
;	| * * |
;	|***  |
;	|     |
;	+-----+ 
		dw	0160000
		dw	0050000
		dw	0150000
		dw	0050000
		dw	0050000
		dw	0160000
XW2_320:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 209 (0xd1):  width 5  Ntilde
;	+-----+
;	|* ** |
;	|*  * |
;	|** * |
;	|* ** |
;	|* ** |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0130000
		dw	0110000
		dw	0150000
		dw	0130000
		dw	0130000
		dw	0110000
XW2_321:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 210 (0xd2):  width 5  Ograve
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_322:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 211 (0xd3):  width 5  Oacute
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_323:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 212 (0xd4):  width 5  Ocircumflex
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_324:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 213 (0xd5):  width 5  Otilde
;	+-----+
;	| **  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_325:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 214 (0xd6):  width 5  Odieresis
;	+-----+
;	|*  * |
;	| **  |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_326:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 215 (0xd7):  width 5  multiply
;	+-----+
;	|     |
;	|     |
;	|*  * |
;	| **  |
;	| **  |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0060000
		dw	0060000
		dw	0110000
XW2_327:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 216 (0xd8):  width 5  Oslash
;	+-----+
;	| *** |
;	|* ** |
;	|* ** |
;	|** * |
;	|** * |
;	|***  |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0130000
		dw	0130000
		dw	0150000
		dw	0150000
		dw	0160000
XW2_330:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 217 (0xd9):  width 5  Ugrave
;	+-----+
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_331:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 218 (0xda):  width 5  Uacute
;	+-----+
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_332:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 219 (0xdb):  width 5  Ucircumflex
;	+-----+
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_333:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 220 (0xdc):  width 5  Udieresis
;	+-----+
;	|*  * |
;	|     |
;	|*  * |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0110000
		dw	0000000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_334:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 221 (0xdd):  width 5  Yacute
;	+-----+
;	| * * |
;	| * * |
;	| * * |
;	|  *  |
;	|  *  |
;	|  *  |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0050000
		dw	0050000
		dw	0020000
		dw	0020000
		dw	0020000
XW2_335:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 222 (0xde):  width 5  Thorn
;	+-----+
;	|*    |
;	|***  |
;	|*  * |
;	|***  |
;	|*    |
;	|*    |
;	|     |
;	+-----+ 
		dw	0100000
		dw	0160000
		dw	0110000
		dw	0160000
		dw	0100000
		dw	0100000
XW2_336:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 223 (0xdf):  width 5  germandbls
;	+-----+
;	| **  |
;	|*  * |
;	|* *  |
;	|*  * |
;	|*  * |
;	|* *  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0110000
		dw	0120000
		dw	0110000
		dw	0110000
		dw	0120000
XW2_337:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 224 (0xe0):  width 5  agrave
;	+-----+
;	| *   |
;	|  *  |
;	| *** |
;	|*  * |
;	|* ** |
;	| * * |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0070000
		dw	0110000
		dw	0130000
		dw	0050000
XW2_340:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 225 (0xe1):  width 5  aacute
;	+-----+
;	|  *  |
;	| *   |
;	| *** |
;	|*  * |
;	|* ** |
;	| * * |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0040000
		dw	0070000
		dw	0110000
		dw	0130000
		dw	0050000
XW2_341:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 226 (0xe2):  width 5  acircumflex
;	+-----+
;	|  *  |
;	| * * |
;	| *** |
;	|*  * |
;	|* ** |
;	| * * |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0050000
		dw	0070000
		dw	0110000
		dw	0130000
		dw	0050000
XW2_342:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 227 (0xe3):  width 5  atilde
;	+-----+
;	| * * |
;	|* *  |
;	| *** |
;	|*  * |
;	|* ** |
;	| * * |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0120000
		dw	0070000
		dw	0110000
		dw	0130000
		dw	0050000
XW2_343:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 228 (0xe4):  width 5  adieresis
;	+-----+
;	| * * |
;	|     |
;	| *** |
;	|*  * |
;	|* ** |
;	| * * |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0000000
		dw	0070000
		dw	0110000
		dw	0130000
		dw	0050000
XW2_344:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 229 (0xe5):  width 5  aring
;	+-----+
;	| **  |
;	| **  |
;	| *** |
;	|*  * |
;	|* ** |
;	| * * |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0060000
		dw	0070000
		dw	0110000
		dw	0130000
		dw	0050000
XW2_345:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 230 (0xe6):  width 5  ae
;	+-----+
;	|     |
;	|     |
;	| *** |
;	|* ** |
;	|* *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0130000
		dw	0120000
		dw	0070000
XW2_346:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 231 (0xe7):  width 5  ccedilla
;	+-----+
;	|     |
;	|     |
;	|  ** |
;	| *   |
;	| *   |
;	|  ** |
;	|  *  |
;	+-----+ 
		dw	0030000
		dw	0040000
		dw	0040000
		dw	0030000
		dw	0020000
XW2_347:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 5		; HD: 2, XH: 5

; Character 232 (0xe8):  width 5  egrave
;	+-----+
;	| *   |
;	|  *  |
;	| **  |
;	|* ** |
;	|**   |
;	| **  |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0060000
		dw	0130000
		dw	0140000
		dw	0060000
XW2_350:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 233 (0xe9):  width 5  eacute
;	+-----+
;	|  *  |
;	| *   |
;	| **  |
;	|* ** |
;	|**   |
;	| **  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0040000
		dw	0060000
		dw	0130000
		dw	0140000
		dw	0060000
XW2_351:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 234 (0xea):  width 5  ecircumflex
;	+-----+
;	| *   |
;	|* *  |
;	| **  |
;	|* ** |
;	|**   |
;	| **  |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0120000
		dw	0060000
		dw	0130000
		dw	0140000
		dw	0060000
XW2_352:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 235 (0xeb):  width 5  edieresis
;	+-----+
;	|* *  |
;	|     |
;	| **  |
;	|* ** |
;	|**   |
;	| **  |
;	|     |
;	+-----+ 
		dw	0120000
		dw	0000000
		dw	0060000
		dw	0130000
		dw	0140000
		dw	0060000
XW2_353:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 236 (0xec):  width 5  igrave
;	+-----+
;	| *   |
;	|  *  |
;	| **  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_354:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 237 (0xed):  width 5  iacute
;	+-----+
;	|  *  |
;	| *   |
;	| **  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0040000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_355:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 238 (0xee):  width 5  icircumflex
;	+-----+
;	|  *  |
;	| * * |
;	| **  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0050000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_356:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 239 (0xef):  width 5  idieresis
;	+-----+
;	| * * |
;	|     |
;	| **  |
;	|  *  |
;	|  *  |
;	| *** |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0000000
		dw	0060000
		dw	0020000
		dw	0020000
		dw	0070000
XW2_357:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 240 (0xf0):  width 5  eth
;	+-----+
;	| *   |
;	|  ** |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0030000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_360:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 241 (0xf1):  width 5  ntilde
;	+-----+
;	| * * |
;	|* *  |
;	|***  |
;	|*  * |
;	|*  * |
;	|*  * |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0120000
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0110000
XW2_361:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 242 (0xf2):  width 5  ograve
;	+-----+
;	| *   |
;	|  *  |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_362:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 243 (0xf3):  width 5  oacute
;	+-----+
;	|  *  |
;	| *   |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0040000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_363:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 244 (0xf4):  width 5  ocircumflex
;	+-----+
;	| **  |
;	|     |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0000000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_364:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 245 (0xf5):  width 5  otilde
;	+-----+
;	| * * |
;	|* *  |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0120000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_365:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 246 (0xf6):  width 5  odieresis
;	+-----+
;	| * * |
;	|     |
;	| **  |
;	|*  * |
;	|*  * |
;	| **  |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0000000
		dw	0060000
		dw	0110000
		dw	0110000
		dw	0060000
XW2_366:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 247 (0xf7):  width 5  divide
;	+-----+
;	|     |
;	| **  |
;	|     |
;	|**** |
;	|     |
;	| **  |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0000000
		dw	0170000
		dw	0000000
		dw	0060000
XW2_367:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 5		; HD: 1, XH: 5

; Character 248 (0xf8):  width 5  oslash
;	+-----+
;	|     |
;	|     |
;	| *** |
;	|* ** |
;	|** * |
;	|***  |
;	|     |
;	+-----+ 
		dw	0070000
		dw	0130000
		dw	0150000
		dw	0160000
XW2_370:		dw	2 * 5 + 1		; no extent
		dw	(2 << 8) | 4		; HD: 2, XH: 4

; Character 249 (0xf9):  width 5  ugrave
;	+-----+
;	| *   |
;	|  *  |
;	|*  * |
;	|*  * |
;	|*  * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0040000
		dw	0020000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0070000
XW2_371:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 250 (0xfa):  width 5  uacute
;	+-----+
;	|  *  |
;	| *   |
;	|*  * |
;	|*  * |
;	|*  * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0020000
		dw	0040000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0070000
XW2_372:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 251 (0xfb):  width 5  ucircumflex
;	+-----+
;	| **  |
;	|     |
;	|*  * |
;	|*  * |
;	|*  * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0060000
		dw	0000000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0070000
XW2_373:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 252 (0xfc):  width 5  udieresis
;	+-----+
;	| * * |
;	|     |
;	|*  * |
;	|*  * |
;	|*  * |
;	| *** |
;	|     |
;	+-----+ 
		dw	0050000
		dw	0000000
		dw	0110000
		dw	0110000
		dw	0110000
		dw	0070000
XW2_374:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 6		; HD: 0, XH: 6

; Character 253 (0xfd):  width 5  yacute
;	+-----+
;	|  *  |
;	| *   |
;	|*  * |
;	|*  * |
;	| * * |
;	|  *  |
;	| *   |
;	+-----+ 
		dw	0020000
		dw	0040000
		dw	0110000
		dw	0110000
		dw	0050000
		dw	0020000
		dw	0040000
XW2_375:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

; Character 254 (0xfe):  width 5  thorn
;	+-----+
;	|     |
;	|*    |
;	|***  |
;	|*  * |
;	|*  * |
;	|***  |
;	|*    |
;	+-----+ 
		dw	0100000
		dw	0160000
		dw	0110000
		dw	0110000
		dw	0160000
		dw	0100000
XW2_376:		dw	2 * 5 + 1		; no extent
		dw	(1 << 8) | 6		; HD: 1, XH: 6

; Character 255 (0xff):  width 5  ydieresis
;	+-----+
;	| * * |
;	|     |
;	|*  * |
;	|*  * |
;	| * * |
;	|  *  |
;	| *   |
;	+-----+ 
		dw	0050000
		dw	0000000
		dw	0110000
		dw	0110000
		dw	0050000
		dw	0020000
		dw	0040000
XW2_377:		dw	2 * 5 + 1		; no extent
		dw	(0 << 8) | 7		; HD: 0, XH: 7

		end	0
