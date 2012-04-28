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
 * Winchester drive emulation
 *
 * $Id: drive.h,v 1.1.1.1 2008/07/22 19:02:06 pm Exp $
 *****************************************************************************/
#if !defined(_DRIVE_H_INCLUDED_)
#define	_DRIVE_H_INCLUDED_

/** @brief Winchester bus signals */
typedef enum {
/* A */		A_READ_CLOCK,
/* B */		B_WRITE_DATA_AND_CLOCK,
/* C */		C_READ_DATA,
/* D */		D_GROUND,
/* E */		E_READ_GATE,
/* F */		F_S_R_W,
/* H */		H_WRITE_PROTECT_INPUT_ATTENTION,
/* J */		J_CYL_5,	/* bug in schematics says CYL_8 */
/* K */		K_ERASE_GATE,
/* L */		L_SELECT_LINE_UNIT_1,
/* M */		M_HIGH_DENSITY,
/* N */		N_CYL_7,
/* P */		P_WRITE_PROTECT_IND,
/* R */		R_SELECT_LINE_UNIT_2,
/* S */		S_PSEUDO_SECTOR_MARK_ATTENTION,
/* T */		T_CYL_2,
/* U */		U_FILE_READY,
/* V */		V_SELECT_LINE_UNIT_3,
/* W */		W_SECTOR_MARK,
/* X */		X_CYL_4,
/* Y */		Y_INDEX_MARK,
/* Z */		Z_CYL_0,	/* schematics says Z_SELECT_LINE_UNIT_4 */
/* AA */	AA_HEAD_SELECT,
/* BB */	BB_CYL_1,
/* CC */	CC_BIT_SECTOR_ADDX,
/* DD */	DD_GROUND_DD,
/* EE */	EE_WRITE_GATE,
/* FF */	FF_CYL_3,
/* HH */	HH_WRITE_CHK,
/* JJ */	JJ_BIT_2,
/* KK */	KK_BIT_4,
/* LL */	LL_CYL_8,
/* MM */	MM_BIT_8,
/* NN */	NN_ADDX_ACKNOWLEDGE,
/* RR */	RR_CYL_6,
/* SS */	SS_STROBE,
/* TT */	TT_SEEK_INCOMPLETE,
/* UU */	UU_BIT_16,
/* VV */	VV_RESTORE,
/* WW */	WW_GROUND,
/* XX */	XX_LOG_ADDX_INTERLOCK
}	winchester_bus_t;

/** @brief max number of drive units */
#define	DRIVE_MAX		2

/** @brief number of cylinders per drive */
#define	DRIVE_CYLINDERS		203

/** @brief bit maks for cylinder number (9 bits) */
#define	DRIVE_CYLINDER_MASK	0777

/** @brief number of sectors per track */
#define	DRIVE_SPT		12

/** @brief bit maks for cylinder number (4 bits) */
#define	DRIVE_SECTOR_MASK	017

/** @brief number of heads per drive */
#define	DRIVE_HEADS		2

/** @brief bit maks for cylinder number (4 bits) */
#define	DRIVE_HEAD_MASK		1


/** 
 * @brief return number of bitclk edges for a raw sector
 *
 * @result number of bitclk edges for a raw sector
 */
extern int drive_bits_per_sector(void);

/** 
 * @brief return a pointer to a drive's description
 *
 * @param unit unit number
 * @result a pointer to the string description
 */
extern const char *drive_description(int unit);

/** 
 * @brief return the number of a drive unit
 *
 * This is usually a no-op, but drives may be swapped?
 *
 * @param unit unit number
 * @result the unit number
 */
extern int drive_unit(int unit);

/** 
 * @brief return the time for a full rotation
 *
 * @param unit unit number
 * @result the time for a full track rotation
 */
extern ntime_t drive_rotation_time(int unit);

/** 
 * @brief return the time for a data bit
 *
 * @param unit unit number
 * @result the time in nano seconds per bit clock
 */
extern ntime_t drive_bit_time(int unit);

/** 
 * @brief return the seek / read / write status of a drive
 *
 * @param unit unit number
 * @result the seek / read / write status for the drive unit (0: ready, 1: not ready)
 */
extern int drive_seek_read_write_0(int unit);

/** 
 * @brief return the ready status of a drive
 *
 * @param unit unit number
 * @result the ready status for the drive unit (0: ready, 1: not ready)
 */
extern int drive_ready_0(int unit);

/** 
 * @brief return the address acknowledge state
 *
 * @param unit unit number
 * @result the address acknowledge state (0: acknowledge, 1: no acknowledge)
 */
extern int drive_addx_acknowledge_0(int unit);

/** 
 * @brief return the log address interlock state
 *
 * @param unit unit number
 * @result the log address interlock state (0: interlock, 1: no interlock)
 */
extern int drive_log_addx_interlock_0(int unit);

/** 
 * @brief return the seek incomplete state
 *
 * @param unit unit number
 * @result the seek incomplete state (0: incomplete, 1: not incomplete)
 */
extern int drive_seek_incomplete_0(int unit);

/** 
 * @brief return the current cylinder of a drive unit
 *
 * @param unit unit number
 * @result the current cylinder number for the drive unit
 */
extern int drive_cylinder(int unit);

/** 
 * @brief return the current head of a drive unit
 *
 * @param unit unit number
 * @result the currently selected head for the drive unit
 */
extern int drive_head(int unit);

/** 
 * @brief select the head of a drive unit
 *
 * @param unit unit number
 * @param head head number
 */
extern void drive_select(int unit, int head);

/** 
 * @brief return the current sector of a drive unit
 *
 * @param unit unit number
 * @result the current sector for the drive unit
 */
extern int drive_sector(int unit);

/** 
 * @brief return the current sector mark status of a drive unit
 *
 * The sector mark is derived from the offset into the current sector.
 *
 * @param unit unit number
 * @result the current sector for the drive unit (0: mark, 1: no mark)
 */
extern int drive_sector_mark_0(int unit);

/** 
 * @brief return the current record of a drive unit
 *
 * Note: the drive itself has not really notion of a record,
 * but just a sector. This function is a helper for the debug output.
 *
 * @param unit unit number
 * @result the current record number inside the current sector
 */
extern int drive_record(int unit);

/** 
 * @brief return the name for the current record of a drive unit
 *
 * Note: the drive itself has not really notion of a record,
 * but just a sector. This function is a helper for the debug output.
 *
 * @param unit unit number
 * @result pointer to a string descibing the record
 */
extern const char *drive_record_name(int unit);

/** 
 * @brief return the current offset into the record of a drive unit
 *
 * Note: the drive itself has not really notion of a record,
 * but just a sector. This function is a helper for the debug output.
 *
 * @param unit unit number
 * @result the current offset into the current record
 */
extern int drive_offset(int unit);

/** 
 * @brief initiate a seek operation
 *
 * Seek to the cylinder cylinder, or restore.
 *
 * @param unit unit number
 * @param cylinder cylinder number to seek to
 * @param restore flag if the drive should restore to cylinder 0
 */
extern void drive_strobe(int unit, int cylinder, int restore, int strobe);

/** 
 * @brief get the sector's bit by index
 *
 * @param unit is the drive index
 * @param index is the sector relative bit index
 * @result returns the sector's bit by index
 */
extern int drive_rddata(int unit, int index);

/** 
 * @brief write a sector's clock or bit bit at index
 *
 * @param unit is the drive index
 * @param index is the sector relative bit index
 * @param wrdata is the clock or write data bit
 */
extern void drive_wrdata(int unit, int index, int data);

/** 
 * @brief install a callback to be called whenever a drive sector starts
 *
 * @param unit is the drive index
 * @result returns 0 on success
 */
extern int drive_sector_callback(int unit, void (*callback)(int));

/** 
 * @brief pass down command line arguments to the drive emulation
 *
 * @param arg command line argument
 * @result returns 0 if this was a disk image, -1 on error
 */
extern int drive_args(const char *arg);

/** 
 * @brief initiate a seek operation
 *
 * Initialize the drive emulation
 *
 * @result returns 0 on success
 */
extern int drive_init(void);

#endif	/* !defined(_DRIVE_H_INCLUDED_) */
