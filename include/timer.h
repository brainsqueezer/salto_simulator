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
 * Timer functions
 *
 * $Id: timer.h,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#if !defined(_TIMER_H_INCLUDED_)
#define	_TIMER_H_INCLUDED_

/** @brief cast x to a time in nanoseconds */
#define	TIME_NS(x)	((ntime_t)(x))

/** @brief cast x to a time in microseconds */
#define	TIME_US(x)	((ntime_t)1000ll * (x))

/** @brief cast x to a time in milliseconds */
#define	TIME_MS(x)	((ntime_t)1000000ll * (x))

/** @brief cast x to a time in seconds */
#define	TIME_S(x)	((ntime_t)1000000000ll * (x))

/** @brief total nano seconds simulation time */
extern ntime_t global_ntime;

/**
 * @brief return the current time - implemented as macro for speed.
 *
 * Take ntime and subtract the nano seconds that the current CPU slice still has to run.
 *
 * @result time in nano seconds
 */
#define	ntime() (global_ntime - alto_ntime)

/**
 * @brief return the current cycle number - implemented as macro for speed.
 *
 * Take ntime and subtract the nano seconds that the current CPU slice still has to run,
 * divide by number of nano seconds per cycle.
 *
 * @result cycle number
 */
#define	cycle() ((global_ntime - alto_ntime) / CPU_MICROCYCLE_TIME)

/** @brief return the time of the next timer event, or -1 if none */
extern ntime_t timer_next_time(void);

/** @brief peek at then n'th timer, return its name and set *atime */
extern const char *timer_peek(int n, int *id, int *arg, ntime_t *atime);

/** @brief remove timer with identifier 'id' */
extern int timer_remove(int id);

/** @brief insert a timer to fire at 'time', calling 'callback' with 'id' and 'arg' */
extern int timer_insert(ntime_t time, void (*callback)(int,int), int arg, const char *name);

/** @brief fire next timer, if it is due; return -1 if none, id otherwise */
extern int timer_fire(void);

/** @brief initialize timer functions */
extern void timer_init(void);

#endif	/* !defined(_TIMER_H_INCLUDED_) */
