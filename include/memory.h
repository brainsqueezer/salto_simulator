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
 * Main memory and memory mapped I/O read and write functions.
 *
 * $Id: memory.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_MEMORY_H_INCLUDED_)
#define	_MEMORY_H_INCLUDED_

/** @brief set non-zero to incorporate the Hamming code and Parity check */
#define	HAMMING_CHECK	0

#define	IO_PAGE_SIZE	512
#define	IO_PAGE_BASE	0177000

#define	MEM_NONE	000
#define	MEM_ODD		001
#define	MEM_RAM		002
#define	MEM_NIRVANA	004

#define	PUT_EVEN(dword,word)		ALTO_PUT(dword,32, 0,15,word)
#define	GET_EVEN(dword)			ALTO_GET(dword,32, 0,15)
#define	PUT_ODD(dword,word)		ALTO_PUT(dword,32,16,31,word)
#define	GET_ODD(dword)			ALTO_GET(dword,32,16,31)

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

typedef struct {
	/** @brief main memory organized as double-words */
	uint32_t ram[RAM_SIZE/2];

	/** @brief Hamming code and Parity bits */
	uint8_t hpb[RAM_SIZE/2];

	/** @brief memory address register */
	uint32_t mar;

	/** @brief read memory data double-word */
	uint32_t rmdd;

	/** @brief write memory data double-word */
	uint32_t wmdd;

	/** @brief memory data register */
	uint32_t md;

	/** @brief cycle when the memory address register was loaded */
	ntime_t cycle;

	/**
	 * @brief memory access und the way if non-zero
	 * 0: no memory access (MEM_NONE)
	 * 1: invalid
	 * 2: memory access even word (MEM_RAM)
	 * 3: memory access odd word (MEM_RAM | MEM_ODD)
	 * 4: refresh even word (MEM_REFRESH)
	 * 5: refresh odd word (MEM_REFRESH | MEM_ODD)
	 */
	int access;

	/**
	 * @brief non-zero after a memory error was detected
	 */
	int error;

	/**
	 * @brief memory error address register
	 */
	int mear;

	/**
	 * @brief memory error status register
	 */
	int mesr;

	/**
	 * @brief memory error control register
	 */
	int mecr;

#if	DEBUG
	/** @brief watch read function (debugging) */
	void (*watch_read)(int mar, int md);

	/** @brief watch write function (debugging) */
	void (*watch_write)(int mar, int md);
#endif
}	mem_t;

extern mem_t mem;

/**
 * @brief check if memory address register load is yet possible
 * suspend if accessing RAM and previous MAR<- was less than 5 cycles ago
 *
 * 1.  MAR<- ANY
 * 2.  REQUIRED
 * 3.  MD<- whatever
 * 4.  SUSPEND
 * 5.  SUSPEND
 * 6.  MAR<- ANY
 *
 * @result returns 0, if memory address can be loaded
 */
#define	check_mem_load_mar_stall(rsel) \
	(MEM_NONE == mem.access ? 0 : cycle() < mem.cycle+5)

/**
 * @brief check if memory read is yet possible
 * MAR<- = cycle #1, earliest read at cycle #5, i.e. + 4
 *
 * 1.  MAR<- ANY
 * 2.  REQUIRED
 * 3.  SUSPEND
 * 4.  SUSPEND
 * 5.  whereever <-MD
 *
 * @result returns 0, if memory can be read without wait cycle
 */
#define	check_mem_read_stall() \
	(MEM_NONE == mem.access ? 0 : cycle() < mem.cycle+4)

/**
 * @brief check if memory write is yet possible
 * MAR<- = cycle #1, earliest write at cycle #3, i.e. + 2
 *
 * 1.  MAR<- ANY
 * 2.  REQUIRED
 * 3.  OPTIONAL
 * 4.  MD<- whatever
 *
 * @result returns 0, if memory can be written without wait cycle
 */
#define check_mem_write_stall() \
	(MEM_NONE == mem.access ? 0 : cycle() < mem.cycle+2)

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
extern uint32_t hamming_code(int write, uint32_t dw_addr, uint32_t dw_data);

/**
 * @brief load the memory address register with some value
 *
 * @param rsel selected register (to detect refresh cycles)
 * @param addr memory address
 */
void load_mar(int rsel, int addr);

/**
 * @brief read memory or memory mapped I/O from the address in mar to md
 *
 * @result returns value from memory (RAM or MMIO)
 */
int read_mem(void);

/**
 * @brief write memory or memory mapped I/O from md to the address in mar
 *
 * @param data data to write to RAM or MMIO
 */
void write_mem(int data);

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
void install_mmio_fn (int first, int last, int (*rfn)(int), void (*wfn)(int, int));

/**
 * @brief debugger interface to read memory
 *
 * @param addr address to read
 * @result memory contents at address (16 bits)
 */
int debug_read_mem (int addr);

/**
 * @brief debugger interface to write memory
 *
 * @param addr address to write
 * @param data data to write (16 bits used)
 */
void debug_write_mem (int addr, int data);

/**
 * @brief initialize the memory system
 *
 * Zeroes the memory context, including RAM, installs dummy
 * handlers for the memory mapped I/O area, and finally
 * installs the per task bank register read/write handlers for
 * the MMIO range 0177740 to 0177757.
 *
 * @param addr address to write
 * @param data data to write (16 bits used)
 */
void init_memory(void);

#endif	/* !defined(_MEMORY_H_INCLUDED_) */

