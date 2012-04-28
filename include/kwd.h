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
 * Bus source F1, and F2 handling for disk word task
 *
 * $Id: kwd.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_KWD_H_INCLUDED_)
#define	_KWD_H_INCLUDED_

/** @brief bs task specific: kwd */
#define	bs_kwd_read_kstat	bs_task_3
#define	bs_kwd_read_kdata	bs_task_4

/** @brief f1 task specific: kwd */
#define	f1_kwd_strobe		f1_task_11
#define	f1_kwd_load_kstat	f1_task_12
#define	f1_kwd_increcno		f1_task_13
#define	f1_kwd_clrstat		f1_task_14
#define	f1_kwd_load_kcom	f1_task_15
#define	f1_kwd_load_kadr	f1_task_16
#define	f1_kwd_load_kdata	f1_task_17

/** @brief f2 task specific: kwd */
#define	f2_kwd_init		f2_task_10	/* branches NEXT[5-9] on WDTASKACT && WDINIT */
#define	f2_kwd_rwc		f2_task_11	/* branches NEXT[8-9] on READ/WRITE/CHECK for record */
#define	f2_kwd_recno		f2_task_12	/* branches NEXT[8-9] on RECNO[0-1] */
#define	f2_kwd_xfrdat		f2_task_13	/* branches NEXT[9] on !SEEKONLY */
#define	f2_kwd_swrnrdy		f2_task_14	/* branches NEXT[9] on !SWRDY */
#define	f2_kwd_nfer		f2_task_15	/* branches NEXT[9] on !KFER */
#define	f2_kwd_strobon		f2_task_16	/* branches NEXT[9] on STROBE */

/** @brief initialize disk word task */
extern int init_kwd(int task);

#endif	/* !defined(_KWD_H_INCLUDED_) */
