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
 * Disk sector task functions
 *
 * $Id: ksec.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>

#include "alto.h"
#include "cpu.h"
#include "ksec.h"
#include "disk.h"

/** @brief block the disk sector task */
static void f1_block_0(void)
{
	LOG((0,2,"	BLOCK %s\n", task_name[cpu.task]));
	disk_block(cpu.task);
}

/**
 * @brief disk sector task slot initialization
 */
int init_ksec(int task)
{
	SET_FN(bs, ksec_read_kstat,	bs_read_kstat_0, NULL);
	SET_FN(bs, ksec_read_kdata,	bs_read_kdata_0, NULL);

	SET_FN(f1, block,		f1_block_0,	NULL);

	SET_FN(f1, task_10,		NULL, NULL);
	SET_FN(f1, ksec_strobe,		NULL, f1_strobe_1);
	SET_FN(f1, ksec_load_kstat,	NULL, f1_load_kstat_1);
	SET_FN(f1, ksec_increcno,	NULL, f1_increcno_1);
	SET_FN(f1, ksec_clrstat,	NULL, f1_clrstat_1);
	SET_FN(f1, ksec_load_kcom,	NULL, f1_load_kcom_1);
	SET_FN(f1, ksec_load_kadr,	NULL, f1_load_kadr_1);
	SET_FN(f1, ksec_load_kdata,	NULL, f1_load_kdata_1);

	SET_FN(f2, ksec_init,		NULL, f2_init_1);
	SET_FN(f2, ksec_rwc,		NULL, f2_rwc_1);
	SET_FN(f2, ksec_recno,		NULL, f2_recno_1);
	SET_FN(f2, ksec_xfrdat,		NULL, f2_xfrdat_1);
	SET_FN(f2, ksec_swrnrdy,	NULL, f2_swrnrdy_1);
	SET_FN(f2, ksec_nfer,		NULL, f2_nfer_1);
	SET_FN(f2, ksec_strobon,	NULL, f2_strobon_1);
	SET_FN(f2, task_17,		NULL, NULL);

	CPU_SET_TASK_WAKEUP(task);

	return 0;
}
