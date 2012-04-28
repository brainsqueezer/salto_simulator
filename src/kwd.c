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
 * Disk word task functions
 *
 * $Id: kwd.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>

#include "alto.h"
#include "cpu.h"
#include "kwd.h"
#include "disk.h"

/** @brief block the disk word task
 *
 * This is most probably the worst place to write down some of my thoughts,
 * but then, who's reading through the comments of any source code anyways?
 *
 * Listen, children, if you happen to stumble across this piece of shitty
 * source intended to emulate some long time ago obsoleted piece of hardware:
 * you won't change nothing until you turn up the noise and voice: LOUD!
 *
 * They have been cheating upon you, they've been telling lies and they've been
 * trying to abuse you. For me there's no doubt, since it's been this way all
 * the time of my life. As soon as you're doing something "useful", you _will_
 * be exploited by some or another.
 *
 * If you want to be decent and not contributive to this world of liars and
 * betrayers, just don't do anything exploitable or useful. Don't care about
 * the future of anything, just take care of the past. Look at those who laid
 * the founding stones of the buildings you're living and working at today.
 *
 * I don't really know what I wanted to tell you, I just remembered it was
 * potentially important - it was, at least before I tried to find the words
 * to give a name to this shit. Nah.. just ignore me for now and forever.
 */
static void f1_block_0(void)
{
	LOG((0,2,"	BLOCK %s\n", task_name[cpu.task]));
	disk_block(cpu.task);
}

/**
 * @brief disk word task slot initialization
 */
int init_kwd(int task)
{
	SET_FN(bs, kwd_read_kstat,	bs_read_kstat_0, NULL);
	SET_FN(bs, kwd_read_kdata,	bs_read_kdata_0, NULL);

	SET_FN(f1, block,		f1_block_0,	NULL);

	SET_FN(f1, kwd_strobe,		NULL, f1_strobe_1);
	SET_FN(f1, kwd_load_kstat,	NULL, f1_load_kstat_1);
	SET_FN(f1, kwd_increcno,	NULL, f1_increcno_1);
	SET_FN(f1, kwd_clrstat,		NULL, f1_clrstat_1);
	SET_FN(f1, kwd_load_kcom,	NULL, f1_load_kcom_1);
	SET_FN(f1, kwd_load_kadr,	NULL, f1_load_kadr_1);
	SET_FN(f1, kwd_load_kdata,	NULL, f1_load_kdata_1);

	SET_FN(f2, kwd_init,		NULL, f2_init_1);
	SET_FN(f2, kwd_rwc,		NULL, f2_rwc_1);
	SET_FN(f2, kwd_recno,		NULL, f2_recno_1);
	SET_FN(f2, kwd_xfrdat,		NULL, f2_xfrdat_1);
	SET_FN(f2, kwd_swrnrdy,		NULL, f2_swrnrdy_1);
	SET_FN(f2, kwd_nfer,		NULL, f2_nfer_1);
	SET_FN(f2, kwd_strobon,		NULL, f2_strobon_1);

	return 0;
}
