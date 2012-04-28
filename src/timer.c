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
 * $Id: timer.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alto.h"
#include "cpu.h"
#include "debug.h"
#include "timer.h"

#define	DEBUG_TIMER	0

/** @brief total nano seconds */
ntime_t global_ntime;

/** @brief Structure of a timer (running in the simulated time domain) */
typedef struct atimer_s {
	/* next timer in atime order */
	struct atimer_s *next;
	/* id of this timer */
	int id;
	/* nano time when timer is due to fire */
	ntime_t atime;
	/* callback to call when firing */
	void (*callback)(int id, int arg);
	/* argument to callback */
	int arg;
	/* argument to callback */
	const char *name;
}	atimer_t;


/** @brief linked list of free timer resources */
static atimer_t *timer_free;

/** @brief linked list of pending timer events */
static atimer_t *timer_head;

/** @brief next timer id */
static int timer_id;

/**
 * @brief allocate a new timer resource, or use one from the free list
 *
 * @returns pointer to a atimer_t, zeroed
 */
static atimer_t *timer_alloc (void)
{
	atimer_t *atimer = timer_free;

	if (atimer) {
		timer_free = atimer->next;
	} else {
		atimer = malloc(sizeof(atimer_t));
		if (!atimer)
			return NULL;
	}
	memset(atimer, 0, sizeof (atimer_t));
	return atimer;
}

/**
 * @brief free a timer by inserting it to the head of the free list
 *
 * @param atimer pointer to a atimer_t struct
 */
static void timer_release(atimer_t *atimer)
{
	LOG((log_TMR,5,"release timer id:%d (%s)\n",
		atimer->id, atimer->name));
	atimer->id = 0;
	atimer->next = timer_free;
	timer_free = atimer;
}

/**
 * @brief return time of the next timer event, -1 if none
 *
 * @result time in nano seconds
 */
ntime_t timer_next_time(void)
{
	atimer_t *head = timer_head;
	ntime_t next;

	if (head) {
		next = head->atime - ntime();
		LOG((log_TMR,5,"next timer %s(%d,%d) fires @ %+lld ns\n",
			head->name, head->id, head->arg, next));
	} else {
		next = 0;
		LOG((log_TMR,5,"no timer pending\n"));
	}
	return next;
}

/**
 * @brief peek at then n'th timer, return its name and set *atime
 *
 * @param n peek at the n'th timer
 * @param id pointer to int receiving the id of the timer
 * @param arg pointer to int receiving the argument for the timer callback
 * @param atime put the n'ths timer's atime here
 * @result name of the timer, NULL if none
 */
const char *timer_peek(int n, int *id, int *arg, ntime_t *atime)
{
	atimer_t *head = timer_head;

	while (n > 0 && head) {
		head = head->next;
		n--;
	}
	if (head) {
		*id = head->id;
		*arg = head->arg;
		*atime = head->atime;
		return head->name;
	}
	return NULL;
}


/**
 * @brief remove a timer by its id
 *
 * @param id timer id as returned by timer_insert()
 * @result 0 on success, -1 on error (id not found)
 */
int timer_remove(int id)
{
	atimer_t *this = timer_head, *last = NULL;

	while (this && this->id != id) {
		last = this;
		this = last->next;
	}
	if (!this)
		return -1;

	if (last)
		last->next = this->next;
	else
		timer_head = this->next;

	timer_release(this);

	return 0;
}

/**
 * @brief insert a new timer
 *
 * @param atime time offset from now, ntime(), when the timer should fire
 * @param callback function to call at fire time
 * @param arg second argument to pass to the callback function
 * @param name name of the timer
 * @result timer id on success, -1 on error (negative time offset)
 */
int timer_insert(ntime_t atime, void (*callback)(int,int), int arg, const char *name)
{
	atimer_t *head = timer_head;
	atimer_t *last = NULL;
	atimer_t *this;

	if (atime < 0)
		fatal(3, "negative time (%lld)\n", atime);
	atime += ntime();

	this = timer_alloc();
	if (!this)
		fatal(3, "failed to allocated timer resources\n");

	this->id = ++timer_id;
	/* don't use 0 as a timer id, as 0 is used for "no timer" */
	if (!timer_id)
		this->id = ++timer_id;
	this->atime = atime;
	this->callback = callback;
	this->arg = arg;
	this->name = name;

	while (head && atime > head->atime) {
		last = head;
		head = last->next;
	}

	if (last) {
		this->next = last->next;
		last->next = this;
	} else {
		this->next = timer_head;
		timer_head = this;
		if (atime <= global_ntime) {
			/*
			 * This timer is becoming first in the timer chain
			 * and fires before the CPU timeslice ends.
			 * Set a flag so that alto_execute() leaves the loop
			 */
			alto_leave = 1;
			LOG((log_TMR,3,"@%lld: inserting '%s' @%lld <= %lld\n",
				ntime(), name, atime, global_ntime));
		}
	}

#if	DEBUG
	LOG((log_TMR,7,"*************\n"));
	head = timer_head;
	while (head) {
		LOG((log_TMR,7,"timer %p%s id:%d time:%lld callback:%p arg:%d\n",
			head, head == this ? "*" : " ",
			head->id, head->atime, head->callback, head->arg));
		head = head->next;
	}
	LOG((log_TMR,7,"*************\n"));
#endif

	return timer_id;
}

/**
 * @brief fire a timer
 *
 * @result timer id of timer that was fired, or -1 if none was due
 */
int timer_fire(void)
{
	atimer_t *this = timer_head;
	void (*callback)(int, int);
	ntime_t atime;
	int id, arg;

	if (!this)
		return -1;

	atime = this->atime - ntime();
	if (atime >= CPU_MICROCYCLE_TIME)
		return -1;

	id = this->id;
	arg = this->arg;
	callback = this->callback;
	timer_head = this->next;

	LOG((log_TMR,5,"fire timer %p(%d,%d) @ %+lld ns\n",
		callback, id, arg, atime));

	if (callback) {
		/* leap forward in time to exact timer event */
		global_ntime += atime;
		(*callback)(id, arg);
		/* back in time */
		global_ntime -= atime;
	} else {
		LOG((log_TMR,0,"fire timer %p(%d,%d) @ %+lld ns - callback is NULL?\n",
			callback, id, arg, atime));
	}

	timer_release(this);

	return id;
}


/**
 * @brief initialize the timer functions
 *
 */
void timer_init(void)
{
	global_ntime = 0;

	timer_free = NULL;
	timer_head = NULL;
	timer_id = 0;
}
