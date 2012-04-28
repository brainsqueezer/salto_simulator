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
 * Disk I/O functions
 *
 * $Id: disk.h,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#if !defined(_DISK_H_INCLUDED_)
#define	_DISK_H_INCLUDED_

/** @brief bit fields of A[0-15] (hardware address register) */
#define	GET_KADDR_SECTOR(kaddr)			ALTO_GET(kaddr,16, 0, 3)
#define	PUT_KADDR_SECTOR(kaddr,val)		ALTO_PUT(kaddr,16, 0, 3,val)
#define	GET_KADDR_CYLINDER(kaddr)		ALTO_GET(kaddr,16, 4,12)
#define	PUT_KADDR_CYLINDER(kaddr,val)		ALTO_PUT(kaddr,16, 4,12,val)
#define	GET_KADDR_HEAD(kaddr)			ALTO_GET(kaddr,16,13,13)
#define	PUT_KADDR_HEAD(kaddr,val)		ALTO_PUT(kaddr,16,13,13,val)
#define	GET_KADDR_DRIVE(kaddr)			ALTO_GET(kaddr,16,14,14)
#define	PUT_KADDR_DRIVE(kaddr,val)		ALTO_PUT(kaddr,16,14,14,val)
#define	GET_KADDR_RESTORE(kaddr)		ALTO_GET(kaddr,16,15,15)
#define	PUT_KADDR_RESTORE(kaddr,val)		ALTO_PUT(kaddr,16,15,15,val)

/** @brief bit fields of C[0-15] (KADR) */
#define	GET_KADR_SEAL(kadr)			ALTO_GET(kadr,16, 0, 7)
#define	PUT_KADR_SEAL(kadr,val)			ALTO_PUT(kadr,16, 0, 7,val)
#define	GET_KADR_HEADER(kadr)			ALTO_GET(kadr,16, 8, 9)
#define	PUT_KADR_HEADER(kadr,val)		ALTO_PUT(kadr,16, 8, 9,val)
#define	GET_KADR_LABEL(kadr)			ALTO_GET(kadr,16,10,11)
#define	PUT_KADR_LABEL(kadr,val)		ALTO_PUT(kadr,16,10,11,val)
#define	GET_KADR_DATA(kadr)			ALTO_GET(kadr,16,12,13)
#define	PUT_KADR_DATA(kadr,val)			ALTO_PUT(kadr,16,12,13,val)
#define	GET_KADR_NOXFER(kadr)			ALTO_GET(kadr,16,14,14)
#define	PUT_KADR_NOXFER(kadr,val)		ALTO_PUT(kadr,16,14,14,val)
#define	GET_KADR_DRIVE(kadr)			ALTO_GET(kadr,16,15,15)
#define	PUT_KADR_DRIVE(kadr,val)		ALTO_PUT(kadr,16,15,15,val)

/** @brief bit fields of S[0-15] (KSTAT) */
#define	GET_KSTAT_SECTOR(kstat)			ALTO_GET(kstat,16,0,3)
#define	PUT_KSTAT_SECTOR(kstat,val)		ALTO_PUT(kstat,16,0,3,val)
#define	GET_KSTAT_DONE(kstat)			ALTO_GET(kstat,16,4,7)
#define	PUT_KSTAT_DONE(kstat,val)		ALTO_PUT(kstat,16,4,7,val)
#define	GET_KSTAT_SEEKFAIL(kstat)		ALTO_GET(kstat,16,8,8)
#define	PUT_KSTAT_SEEKFAIL(kstat,val)		ALTO_PUT(kstat,16,8,8,val)
#define	GET_KSTAT_SEEK(kstat)			ALTO_GET(kstat,16,9,9)
#define	PUT_KSTAT_SEEK(kstat,val)		ALTO_PUT(kstat,16,9,9,val)
#define	GET_KSTAT_NOTRDY(kstat)			ALTO_GET(kstat,16,10,10)
#define	PUT_KSTAT_NOTRDY(kstat,val)		ALTO_PUT(kstat,16,10,10,val)
#define	GET_KSTAT_DATALATE(kstat)		ALTO_GET(kstat,16,11,11)
#define	PUT_KSTAT_DATALATE(kstat,val)		ALTO_PUT(kstat,16,11,11,val)
#define	GET_KSTAT_IDLE(kstat)			ALTO_GET(kstat,16,12,12)
#define	PUT_KSTAT_IDLE(kstat,val)		ALTO_PUT(kstat,16,12,12,val)
#define	GET_KSTAT_CKSUM(kstat)			ALTO_GET(kstat,16,13,13)
#define	PUT_KSTAT_CKSUM(kstat,val)		ALTO_PUT(kstat,16,13,13,val)
#define	GET_KSTAT_COMPLETION(kstat)		ALTO_GET(kstat,16,14,15)
#define	PUT_KSTAT_COMPLETION(kstat,val)		ALTO_PUT(kstat,16,14,15,val)

/** @brief bit fields of KCOM (hardware command register) */
#define	GET_KCOM_XFEROFF(kcom)			ALTO_GET(kcom,16,1,1)
#define	PUT_KCOM_XFEROFF(kcom,val)		ALTO_PUT(kcom,16,1,1,val)
#define	GET_KCOM_WDINHIB(kcom)			ALTO_GET(kcom,16,2,2)
#define	PUT_KCOM_WDINHIB(kcom,val)		ALTO_PUT(kcom,16,2,2,val)
#define	GET_KCOM_BCLKSRC(kcom)			ALTO_GET(kcom,16,3,3)
#define	PUT_KCOM_BCLKSRC(kcom,val)		ALTO_PUT(kcom,16,3,3,val)
#define	GET_KCOM_WFFO(kcom)			ALTO_GET(kcom,16,4,4)
#define	PUT_KCOM_WFFO(kcom,val)			ALTO_PUT(kcom,16,4,4,val)
#define	GET_KCOM_SENDADR(kcom)			ALTO_GET(kcom,16,5,5)
#define	PUT_KCOM_SENDADR(kcom,val)		ALTO_PUT(kcom,16,5,5,val)

typedef enum {
	STATUS_COMPLETION_GOOD,
	STATUS_COMPLETION_HARDWARE_ERROR,
	STATUS_COMPLETION_CHECK_ERROR,
	STATUS_COMPLETION_ILLEGAL_SECTOR
}	completion_t;


/**
 * @brief enumeration of the inputs and outputs of a JK flip-flop type 74109
 *
 * 74109
 * Dual J-/K flip-flops with set and reset.
 * 
 *       +----------+           +-----------------------------+
 * /1RST |1  +--+ 16| VCC       | J |/K |CLK|/SET|/RST| Q |/Q |
 *    1J |2       15| /2RST     |---+---+---+----+----+---+---|
 *   /1K |3       14| 2J        | X | X | X |  0 |  0 | 1 | 1 |
 *  1CLK |4   74  13| /2K       | X | X | X |  0 |  1 | 1 | 0 |
 * /1SET |5  109  12| 2CLK      | X | X | X |  1 |  0 | 0 | 1 |
 *    1Q |6       11| /2SET     | 0 | 0 | / |  1 |  1 | 0 | 1 |
 *   /1Q |7       10| 2Q        | 0 | 1 | / |  1 |  1 | - | - |
 *   GND |8        9| /2Q       | 1 | 0 | / |  1 |  1 |/Q | Q |
 *       +----------+           | 1 | 1 | / |  1 |  1 | 1 | 0 |
 *                              | X | X |!/ |  1 |  1 | - | - |
 *                              +-----------------------------+
 * 
 * [This information is part of the GIICM]
 */
typedef enum {
	/** @brief clock signal */
	JKFF_CLK	= 0001,
	/** @brief J input */
	JKFF_J		= 0002,
	/** @brief K' input */
	JKFF_K		= 0004,
	/** @brief S' input */
	JKFF_S		= 0010,
	/** @brief C' input */
	JKFF_C		= 0020,
	/** @brief Q output */
	JKFF_Q		= 0040,
	/** @brief Q' output */
	JKFF_Q0		= 0100
}	jkff_t;

/**
 * @brief structure of the disk controller context
 */
typedef struct {
	/** @brief A[0-15] disk hardware address (sector, cylinder, head, drive, restore) */
	int	kaddr;

	/** @brief C[0-15] with read/write/check modes for header, label and data */
	int	kadr;

	/** @brief S[0-15] disk status */
	int	kstat;

	/** @brief disk command (5 bits kcom[1-5]) */
	int	kcom;

	/** @brief record number (2 bits indexing header, label, data, -/-) */
	int	krecno;

	/** @brief input shift register */
	int	shiftreg_in;

	/** @brief output shift register */
	int	shiftreg_out;

	/** @brief disk data in latch */
	int	data_in;

	/** @brief disk data out latch */
	int	data_out;

	/** @brief read/write/check for current record */
	int	krwc;

	/** @brief disk fatal error signal state */
	int	kfer;

	/** @brief disk word task enable (active low) */
	int	wdtskena;

	/** @brief disk word task init at the early microcycle */
	int	wdinit0;

	/** @brief disk word task init at the late microcycle*/
	int	wdinit;

	/** @brief strobe (still) active */
	int	strobe;

	/** @brief current bitclk state (either crystal clock, or rdclk from the drive) */
	int	bitclk;

	/** @brief current datin drom the drive */
	int	datin;

	/** @brief bit counter */
	int	bitcount;

	/** @brief carry output of the bitcounter */
	int	carry;

	/** @brief sector late (monoflop output) */
	int	seclate;

	/** @brief sector late timer id */
	int	seclate_id;

	/** @brief seekok state (SKINC' & LAI' & ff_44a.Q') */
	int	seekok;

	/** @brief ok to run signal (set to 1 some time after reset ?) */
	int	ok_to_run;

	/** @brief ready monoflop 31a */
	int	ready_mf31a;

	/**
	 * @brief JK flip-flop 21a (sector task)
	 * CLK	SECT4
	 * J	WAKEST'
	 * K'	1
	 * S'	ERRWAKE'
	 * C'	WAKEST'
	 * Q	to seclate monoflop
	 */
	jkff_t	ff_21a;

	/**
	 * @brief JK flip-flop 21b (sector task)
	 * CLK	SYSCLKB'
	 * J	from 21a Q
	 * K'	1
	 * S'	1
	 * C'	WAKEST'
	 * Q	to 22a J
	 */
	jkff_t	ff_21b;

	/**
	 * @brief JK flip-flop 22a (sector task)
	 * CLK	SYSCLKB'
	 * J	from 21b Q
	 * K'	1
	 * S'	1
	 * C'	WAKEST'
	 * Q	to 22b J
	 */
	jkff_t	ff_22a;

	/**
	 * @brief JK flip-flop 22b (sector task)
	 * CLK	SYSCLKB'
	 * J	from 22a Q
	 * K'	(BLOCK & STSKACT)'
	 * S'	1 (really it's RESET')
	 * C'	1
	 * Q	STSKENA; Q' WAKEKST'
	 */
	jkff_t	ff_22b;

	/**
	 * @brief JK flip-flop 43b (word task)
	 * CLK	WDDONE'
	 * J	1
	 * K'	1
	 * S'	1
	 * C'	WDTSKENA
	 * Q	to 53a J
	 */
	jkff_t	ff_43b;

	/**
	 * @brief JK flip-flop 53a (word task)
	 * CLK	SYSCLKB'
	 * J	from 43b Q
	 * K'	(BLOCK & WDTSKACT)'
	 * S'	1
	 * C'	WDALLOW
	 * Q	to 43a J
	 */
	jkff_t	ff_53a;

	/**
	 * @brief JK flip-flop 43a (word task)
	 * CLK	SYSCLKA'
	 * J	from 53a Q
	 * K'	from 53a Q
	 * S'	1
	 * C'	WDALLOW
	 * Q	WDTSKENA', Q' WDTSKENA
	 */
	jkff_t	ff_43a;


	/**
	 * @brief JK flip-flop 53b (word task)
	 * CLK	SYSCLKB'
	 * J	0
	 * K'	(BLOCK & WDTSKACT)'
	 * S'	WDALLOW
	 * C'	1
	 * Q	WDINIT
	 */
	jkff_t	ff_53b;

	/**
	 * @brief JK flip-flop 44a (LAI' clocked)
	 * CLK	(LAI')'
	 * J	1
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	to seekok
	 */
	jkff_t	ff_44a;

	/**
	 * @brief JK flip-flop 44b (CKSUM)
	 * CLK	SYSCLKA'
	 * J	BUS[13] on KSTAT<-
	 * K'	1
	 * S'	1
	 * C'	1
	 * Q	n.c.; Q' inverted to BUS[13] on <-KSTAT
	 */
	jkff_t	ff_44b;

	/**
	 * @brief JK flip-flop 45a (ready latch)
	 * CLK	SYSCLKA'
	 * J	READY' from drive
	 * K'	1
	 * S'	1
	 * C'	CLRSTAT'
	 * Q	RDYLAT'
	 */
	jkff_t	ff_45a;

	/**
	 * @brief JK flip-flop 45b (seqerr latch)
	 * CLK	SYSCLKA'
	 * J	1
	 * K'	SEQERR'
	 * S'	CLRSTAT'
	 * C'	1
	 * Q	to KSTAT[11] DATALATE
	 */
	jkff_t	ff_45b;

}	disk_t;

/**
 * @brief the disk controller context
 */
extern	disk_t dsk;

/**
 * @brief called if one of the disk tasks blocks
 *
 * @param task task that blocks (either task_ksec or task_kwd)
 */
extern void disk_block(int task);

/**
 * @brief bus driven by disk status register KSTAT
 */
extern void bs_read_kstat_0(void);

/**
 * @brief bus driven by disk data register KDATA input
 */
extern void bs_read_kdata_0(void);

/**
 * @brief initiates a disk seek
 *
 * Initiates a disk seek operation. The KDATA register must have
 * been loaded previously, and the SENDADR bit of the KCOM
 * register previously set to 1.
 */
extern void f1_strobe_1(void);

/**
 * @brief load disk status register
 *
 * KSTAT[12-15] are loaded from BUS[12-15], except that BUS[13] is
 * ORed into KSTAT[13].
 */
extern void f1_load_kstat_1(void);

/**
 * @brief load data out register
 *
 * KDATA_OUT is loaded from BUS.
 */
extern void f1_load_kdata_1(void);

/**
 * @brief advances shift registers holding KADR
 *
 * Advances the shift registers holding the KADR register so that they
 * present the number and read/write/check status of the next record
 * to the hardware.
 *
 * Sheet 10, shifters 36 and 37 (74195)
 *
 * Vcc, BUS[08], BUS[10], BUS[12] go to 36 A,B,C,D
 * Vcc, BUS[09], BUS[11], BUS[13] go to 37 A,B,C,D
 * A is connected to Vcc on both
 * Both shifters are loaded with KADR<-
 *
 * The QA outputs are 36 -> RECNO(0) and 37 -> RECNO(1)
 *
 * QA of 36 is inverted and goes to J/K' of 37
 * QA of 37 goes to J/K' of 36
 *
 */
extern void f1_increcno_1(void);

/**
 * @brief reset all error latches
 *
 * Causes all error latches in the disk controller hardware to reset,
 * clears KSTAT[13].
 */
extern void f1_clrstat_1(void);

/**
 * @brief load the KCOM register from bus
 *
 * This causes the KCOM register to be loaded from BUS[1-5]. The
 * KCOM register has the following interpretation:
 *	(1) XFEROFF = 1 inhibits data transmission to/from the disk.
 *	(2) WDINHIB = 1 prevents the disk word task from awakening.
 * 	(3) BCLKSRC = 0 takes bit clock from disk input or crystal clock,
 * 	    as appropriate. BCLKSRC = 1 force use of crystal clock.
 *	(4) WFFO = 0 holds the disk bit counter at -1 until a 1 bit is read.
 *	    WFFO = 1 allows the bit counter to proceed normally.
 *	(5) SENDADR = 1 causes KDATA[4-12] and KDATA[15] to be transmitted
 *	    to disk unit as track address. SENDADR = 0 inhibits such
 *	    transmission.
 */
extern void f1_load_kcom_1(void);

/**
 * @brief load the KADR register from bus
 *
 * The KADR register is loaded from BUS[8-14]. This register has the format
 * of word C in section 6.0 above. In addition, it causes the head address
 * bit to be loaded from KDATA[13].
 *
 */
extern void f1_load_kadr_1(void);

/**
 * @brief branch on disk word task active and init
 *
 * NEXT <- NEXT OR (WDTASKACT && WDINIT ? 037 : 0)
 */
extern void f2_init_1(void);

/**
 * @brief branch on read/write/check state of the current record
 *
 * NEXT <- NEXT OR (current record to be written ? 3 :
 *	current record to be checked ? 2 : 0);
 *
 */
extern void f2_rwc_1(void);

/**
 * @brief branch on the current record number by a lookup table
 *
 * NEXT <- NEXT OR MAP (current record number) where
 *   MAP(0) = 0
 *   MAP(1) = 2
 *   MAP(2) = 3
 *   MAP(3) = 1
 */
extern void f2_recno_1(void);

/**
 * @brief branch on the data transfer state
 *
 * NEXT <- NEXT OR (if current command wants data transfer ? 1 : 0)
 */
extern void f2_xfrdat_1(void);

/**
 * @brief branch on the disk ready signal
 *
 * NEXT <- NEXT OR (if disk not ready to accept command ? 1 : 0)
 */
extern void f2_swrnrdy_1(void);

/**
 * @brief branch on the disk fatal error condition
 *
 * NEXT <- NEXT OR (if fatal error in latches ? 0 : 1)
 */
extern void f2_nfer_1(void);

/**
 * @brief branch on the seek busy status
 *
 * NEXT <- NEXT OR (if seek strobe still on ? 1 : 0)
 */
extern void f2_strobon_1(void);

/** 
 * @brief pass down command line arguments to the disk emulation
 *
 * @param arg command line argument
 * @result returns 0 if this was a disk image, -1 on error
 */
extern int disk_args(const char *arg);

extern int disk_init(void);

#endif	/* !defined(_DISK_H_INCLUDED_) */
