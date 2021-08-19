/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 18-Jun-90	Gregg Kellogg (gk) at NeXT
 *	Use us_untimeout() instead of untimeout().
 *	Don't use kern_server instance var directly.
 *
 * 04-Apr-90	Doug Mitchell
 *	replaced queue_enter_head with queue_enter_first
 *
 * 13-Mar-90	Doug Mitchell at NeXT
 *	Changed kern_serv_port_proc() calls to kern_serv_port_serv().
 *
 * 19-May-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

/*
 * Midi timer services.
 *
 * Implements a timer port based on either system time, input MIDI Time Code,
 * or input MIDI Clocks.
 *
 * For System mode:
 *	actual_usec_per_quanta == specified usec_per_quanta
 *	Timebase is set to timer-initialization time.
 *	If the timer is paused (by user action) a pause time is recorded;
 *	the delta between resume and pause is added to the timebase.
 *	Events are scheduled absolutely WRT the timebase.
 *
 * For MIDI Time Code (MTC):
 *	Actual_usec_per_quanta calculated based upon MTC quarter frame
 *	reception rate and frame-rate specified in MTC messages.
 *	Time paused and resumed externally only when MTC quarter frames
 *	stop coming (or start again).  (FIXME: ?? or MIDI Pause/Resume seen?).
 *	Time can be reversed.
 *	Events are scheduled using us_timeout() for inter-quarter-frame
 *	quanta.  Events synchronized on quarter-frame boundaries are
 *	evaluated when the quarter-frame messages are received.  Events
 *	destined for inter-quarter-frames beyond the next quarter frame
 *	aren't scheduled until the quarter-frame message immediately prior
 *	to the event is received.
 *
 * For MIDI Clock:
 *	Actual_usec_per_quanta calculated based on MIDI Clock reception
 *	rate and quanta_per_clock specified.
 *	Time is paused when MIDI Pause is received and resumed when
 *	MIDI Resume is received.
 *	Time is never reversed.
 */

#import <midi/midi_types.h>
#import "midi_var.h"
#import "midi_timer_server.h"
#import "midi_timer_reply_server.h"
#import <sys/callout.h>


extern midi_var_t midi_var;

/*
 * prototypes for static functions 
 */
static kern_return_t midi_timer_req(midi_timer_t *timer, 
	port_name_t reply_port, timeval_t *tvp);
static void midi_timer_notify_all(midi_timer_t	*timer);
static void midi_timer_insert_req(midi_timer_t *timer, midi_timer_req_t *tr);
static midi_timer_req_t *midi_timer_req_from_port(queue_t q, port_name_t port);
static midi_timer_req_t *midi_timer_req_alloc(midi_timer_t *timer);
static void midi_timer_req_free(midi_timer_t * timer, midi_timer_req_t *tr);
static void midi_timer_timeout(midi_timer_t *timer);
static void midi_timer_update_time(midi_timer_t *timer);
static void midi_timer_run_event_queue(midi_timer_t *timer);
static void midi_timer_run_req_queue(midi_timer_t *timer);
static void midi_timer_sched_event(port_t port);
static void midi_timer_sched_event_no_expire(port_t port);
static midi_timer_t *midi_timer_from_reply_port(port_t port);

#define tv_to_quanta(tv) ( \
	  (tv)->tv_sec * (1000000/timer->usec_per_q) \
	+ (tv)->tv_usec/timer->usec_per_q \
)

#define quanta_to_tv(quanta, tv) ( \
	((tv)->tv_sec =    ((quanta/1000000)*timer->usec_per_q \
			+ ((quanta%1000000)*timer->usec_per_q)/1000000)), \
	((tv)->tv_usec =   (((quanta%timer->usec_per_q)*1000000) \
			/ timer->usec_per_q)) \
)

void midi_timer_init(int unit)
{
	midi_timer_t	*timer = &midi_var.dev[unit].timer;
	kern_return_t r;

	midi_tlog("midi_timer_init[%d] start\n", unit, 2, 3, 4, 5);

	/*
	 * Allocate output timer port.
	 */
	r = port_allocate((task_t)task_self(), &timer->port);
	if (r != KERN_SUCCESS)
		midi_panic("can't allocate out_timer port");
	r = kern_serv_port_serv(&midi_var.kern_server, timer->port,
		(port_map_proc_t)midi_timer_server, (int)timer->port);
	if (r != KERN_SUCCESS)
		midi_panic("can't add out_timer to set");

	midi_tlog("midi_timer_init[%d] timer_port %d\n", unit, timer->port,
		3, 4, 5);

	queue_init(&timer->timer_q);
	queue_init(&timer->free_q);
	queue_init(&timer->event_q);
	queue_init(&timer->req_q);
	timer->free_q_size = 0;

	midi_tlog("midi_timer_init[%d] alloc timer req bufs\n",
		unit, 2, 3, 4, 5);

	midi_timer_req_free(timer, midi_timer_req_alloc(timer));
	timer->mode = mt_system;
	timer->act_usec_per_q = timer->usec_per_q = 1000;
	timer->forward = TRUE;
	timer->paused = TRUE;
	timer->events_queued = FALSE;

	timer->q_of_cur_req = 0;
	timerclear(&timer->cur_time);
	microboot(&timer->req_time);

	midi_tlog("midi_timer_init[%d] req_time %d.%d\n", unit,
		timer->req_time.tv_sec, timer->req_time.tv_usec, 4, 5);

	/*
	 * Allocate input timer reply ports.
	 */
	r = port_allocate((task_t)task_self(),
		&midi_var.dev[unit].dir[MIDI_DIR_XMIT].in_timer_reply);
	if (r != KERN_SUCCESS)
		midi_panic("can't allocate in_timer_reply[xmit] port");
	r = kern_serv_port_serv(&midi_var.kern_server,
		midi_var.dev[unit].dir[MIDI_DIR_XMIT].in_timer_reply,
		(port_map_proc_t)midi_timer_reply_server,
		(int)midi_var.dev[unit].dir[MIDI_DIR_XMIT].in_timer_reply);
	if (r != KERN_SUCCESS)
		midi_panic("can't add in_timer_reply[xmit] to set");

	r = port_allocate((task_t)task_self(),
		&midi_var.dev[unit].dir[MIDI_DIR_RECV].in_timer_reply);
	if (r != KERN_SUCCESS)
		midi_panic("can't allocate in_timer_reply[recv] port");
	r = kern_serv_port_serv(&midi_var.kern_server,
		midi_var.dev[unit].dir[MIDI_DIR_RECV].in_timer_reply,
		(port_map_proc_t)midi_timer_reply_server,
		(int)midi_var.dev[unit].dir[MIDI_DIR_RECV].in_timer_reply);
	if (r != KERN_SUCCESS)
		midi_panic("can't add in_timer_reply[recv] to set");

	/*
	 * Default the input timer ports to use this timer.
	 */
	midi_tlog("midi_timer_init[%d] set in_timers to %d\n", unit,
		timer->port, 3, 4, 5);

	midi_set_in_timer_port(&midi_var.dev[unit].dir[MIDI_DIR_XMIT],
		timer->port);
	midi_set_in_timer_port(&midi_var.dev[unit].dir[MIDI_DIR_RECV],
		timer->port);

	midi_tlog("midi_timer_init[%d] done\n", unit, 2, 3, 4, 5);
}

void midi_timer_reset(int unit)
{
	midi_timer_t		*timer = &midi_var.dev[unit].timer;
	midi_timer_req_t	*tr;
	int s;

	midi_tlog("midi_timer_reset[%d] start\n", unit, 2, 3, 4, 5);
	ASSERT(curipl() == 0);

	if (timer->port == PORT_NULL)
		return;		// never initialized

	/*
	 * Deallocate output timer port.
	 */
	kern_serv_port_gone(&midi_var.kern_server, timer->port);
	(void) port_deallocate((task_t)task_self(),
		timer->port);
	timer->port = PORT_NULL;

	/*
	 * Deallocate input timer reply ports.
	 */
	kern_serv_port_gone(&midi_var.kern_server,
		midi_var.dev[unit].dir[MIDI_DIR_XMIT].in_timer_reply);
	(void) port_deallocate((task_t)task_self(),
		midi_var.dev[unit].dir[MIDI_DIR_XMIT].in_timer_reply);
	midi_var.dev[unit].dir[MIDI_DIR_XMIT].in_timer_reply = PORT_NULL;

	kern_serv_port_gone(&midi_var.kern_server,
		midi_var.dev[unit].dir[MIDI_DIR_RECV].in_timer_reply);
	(void) port_deallocate((task_t)task_self(),
		midi_var.dev[unit].dir[MIDI_DIR_RECV].in_timer_reply);
	midi_var.dev[unit].dir[MIDI_DIR_RECV].in_timer_reply = PORT_NULL;

	s = splmidisoft();
	simple_lock(midi_var.slockp);
	
	midi_tlog("midi_timer_reset[%d] free timer_req bufs\n", unit,
		2, 3, 4, 5);

	while (!queue_empty(&timer->timer_q)) {
		queue_remove_first(&timer->timer_q, tr, midi_timer_req_t *,
				   link);
		midi_timer_req_free(timer, tr);
	}

	us_untimeout((int (*)())midi_timer_timeout, (int)timer);

	simple_unlock(midi_var.slockp);
	splx(s);

	midi_timer_run_event_queue(timer);
	ASSERT(!timer->events_queued && queue_empty(&timer->event_q));

	while (!queue_empty(&timer->req_q)) {
		queue_remove_first(&timer->req_q, tr,
			midi_timer_req_t *, link);
		kfree(tr, sizeof(*tr));
	}

	while (!queue_empty(&timer->free_q)) {
		queue_remove_first(&timer->free_q, tr,
			midi_timer_req_t *, link);
		kfree(tr, sizeof(*tr));
		timer->free_q_size--;
	}
	ASSERT(timer->free_q_size == 0);
}

void midi_timer_set_proto(u_int unit, enum midi_timer_mode clock_source)
{
	/*
	 * For the time being, we can't set the mode.
	 */
	return;
}

/*
 * Schedule a timer event using the timers time space.
 * Can't be called above MIDI_SOFTIPL.
 */
static kern_return_t midi_timer_req (
	midi_timer_t	*timer,
	port_name_t	reply_port,
	timeval_t	*tvp)
{
	midi_timer_req_t *tr = 0;
	int s;

	midi_tlog("midi_timer_req: sec %d, usec %d\n", tvp->tv_sec,
		tvp->tv_usec, 3, 4, 5);

	/*
	 * Extract any existing entry for this port.
	 *
	 * (I hate to do this at spl, but I can't think of any other way.)
	 */
	if (curipl() <= MIDI_SOFTIPL) {
		s = splmidisoft();
		simple_lock(midi_var.slockp);
	}

	tr = midi_timer_req_from_port(&timer->timer_q, reply_port);
	if (tr) {
		midi_tlog("midi_timer_req: free old req\n", 1, 2, 3, 4, 5);

		queue_remove(&timer->timer_q, tr,
			midi_timer_req_t *, link);
	}

	if (curipl() == MIDI_SOFTIPL) {
		simple_unlock(midi_var.slockp);
		splx(s);
	}

	midi_tlog("midi_timer_req: alloc timer_req buf\n", 1, 2, 3, 4, 5);

	if (!tr)
		tr = midi_timer_req_alloc(timer);

	if (!tr)
		return KERN_RESOURCE_SHORTAGE;

	tr->port = reply_port;
	tr->timeval = *tvp;
	midi_timer_insert_req(timer, tr);
	
	midi_tlog("midi_timer_req: done\n", 1, 2, 3, 4, 5);

	return KERN_SUCCESS;
}

/*
 * Set the time to specified value (sys clock only).
 * Causes the request list to be re-scanned.
 */
kern_return_t midi_timer_set (
	port_t		timer_port,
	port_t		control_port,
	timeval_t	tv)
{
	midi_timer_t *timer;
	int s, unit;

	ASSERT(curipl() == 0);
	if (timer_port == midi_var.dev[0].timer.port) {
		timer = &midi_var.dev[0].timer;
		unit = 0;
	} else if (timer_port == midi_var.dev[1].timer.port) {
		timer = &midi_var.dev[1].timer;
		unit = 1;
	} else
		return timer_set(timer_port, control_port, tv);

	if (control_port != midi_var.dev[unit].owner)
		return TIMER_BAD_CNTRL;

	midi_tlog("midi_timer_set[%d]: %d.%d\n", unit, tv.tv_sec,
		tv.tv_usec, 4, 5);

	if (timer->mode != mt_system)
		return TIMER_MODE_WRONG;

	s = splmidi();
	simple_lock(midi_var.slockp);
	if (timer->q_of_cur_req) {
		timer->q_of_cur_req = 0;
		midi_tlog("midi_timer_set: remove timeout entry\n",
			1, 2, 3, 4, 5);
		us_untimeout((int (*)())midi_timer_timeout, (int)timer);
	}
	timer->cur_time = tv;
	microboot(&timer->req_time);

	simple_unlock(midi_var.slockp);
	splx(s);

	/*
	 * Since we've changed things, let everyone know.
	 */
	midi_timer_notify_all(timer);

	return KERN_SUCCESS;
}

/*
 * Update number of microseconds per quantum.
 */
kern_return_t midi_timer_set_quantum (
	port_t		timer_port,
	port_t		control_port,
	u_int		usec_per_quantum)
{
	midi_timer_t *timer;
	int s, unit;

	ASSERT(curipl() == 0);

	if (timer_port == midi_var.dev[0].timer.port) {
		timer = &midi_var.dev[0].timer;
		unit = 0;
	} else if (timer_port == midi_var.dev[1].timer.port) {
		timer = &midi_var.dev[1].timer;
		unit = 0;
	} else
		return timer_set_quantum(timer_port, control_port,
					 usec_per_quantum);

	if (control_port != midi_var.dev[unit].owner)
		return TIMER_BAD_CNTRL;

	midi_tlog("midi_timer_set_quantum[%d]: %d\n", unit,
		usec_per_quantum, 3, 4, 5);

	s = splmidi();
	simple_lock(midi_var.slockp);

	/*
	 * Update the current time and set the new usec/quantum value.
	 */
	midi_timer_update_time(timer);

	if (timer->mode != mt_system) {
		/*
		 * Calculate a new act_usec_per_q using the old ratio on the
		 * new value.
		 */
		timer->act_usec_per_q =   (  timer->act_usec_per_q
					   * usec_per_quantum)
					/ timer->usec_per_q;
	} else
		timer->act_usec_per_q = usec_per_quantum;

	simple_unlock(midi_var.slockp);
	splx(s);

	/*
	 * Since we've changed things, let everyone know.
	 */
	midi_timer_notify_all(timer);

	return KERN_SUCCESS;
}

/*
 * Pause the timer (sys mode only).
 */
kern_return_t midi_timer_stop(port_t timer_port, port_t control_port)
{
	midi_timer_t *timer;
	int s, unit;

	ASSERT(curipl() == 0);

	if (timer_port == midi_var.dev[0].timer.port) {
		timer = &midi_var.dev[0].timer;
		unit = 0;
	} else if (timer_port == midi_var.dev[1].timer.port) {
		timer = &midi_var.dev[1].timer;
		unit = 1;
	} else
		return timer_stop(timer_port, control_port);

	if (control_port != midi_var.dev[unit].owner)
		return TIMER_BAD_CNTRL;

	midi_tlog("midi_timer_stop[%d]\n", unit, 2, 3, 4, 5);

	if (timer->paused)
		return KERN_SUCCESS;

	s = splmidi();
	simple_lock(midi_var.slockp);
	
	/*
	 * Update cur_time based un number of quanta elapsed
	 * since last request was made.
	 */
	midi_timer_update_time(timer);
	timer->paused = TRUE;

	simple_unlock(midi_var.slockp);
	splx(s);

	/*
	 * Since we've changed things, let everyone know.
	 */
	midi_timer_notify_all(timer);

	return KERN_SUCCESS;
}

/*
 * Resume the clock (sys clock only).
 */
kern_return_t midi_timer_start(port_t timer_port, port_t control_port)
{
	midi_timer_t *timer;
	int unit;

	ASSERT(curipl() == 0);

	if (timer_port == midi_var.dev[0].timer.port) {
		timer = &midi_var.dev[0].timer;
		unit = 0;
	} else if (timer_port == midi_var.dev[1].timer.port) {
		timer = &midi_var.dev[1].timer;
		unit = 1;
	} else
		return timer_start(timer_port, control_port);

	if (control_port != midi_var.dev[unit].owner)
		return TIMER_BAD_CNTRL;

	if (!timer->paused)
		return KERN_SUCCESS;

	midi_tlog("midi_timer_start[%d]\n", unit, 2, 3, 4, 5);

	microboot(&timer->req_time);
	timer->paused = FALSE;

	/*
	 * Since we've changed things, let everyone know.
	 */
	midi_timer_notify_all(timer);

	return KERN_SUCCESS;
}

/*
 * Change clock direction (sys clock only).
 */
kern_return_t midi_timer_direction (
	port_t		timer_port,
	port_t		control_port,
	boolean_t	forward)
{
	queue_head_t	q;
	int s, unit;
	midi_timer_t *timer;

	ASSERT(curipl() == 0);

	if (timer_port == midi_var.dev[0].timer.port) {
		timer = &midi_var.dev[0].timer;
		unit = 0;
	} else if (timer_port == midi_var.dev[1].timer.port) {
		timer = &midi_var.dev[1].timer;
		unit = 1;
	} else
		return timer_direction(timer_port, control_port, forward);

	if (control_port != midi_var.dev[unit].owner)
		return TIMER_BAD_CNTRL;

	if (timer != &midi_var.dev[unit].timer)
		return TIMER_BAD_CNTRL;

	midi_tlog("midi_timer_direction[%d] %s\n", unit,
		forward ? "forward" : "backwards", 3, 4, 5);

	if (timer->forward == forward)
		return KERN_SUCCESS;
	else
		return TIMER_UNSUP;

	s = splmidi();
	simple_lock(midi_var.slockp);

	midi_timer_update_time(timer);
	timer->forward = ~timer->forward;

	/*
	 * Reverse the entries in the timeout queue.
	 */
	q.next = timer->timer_q.next;
	q.prev = timer->timer_q.prev;
	((midi_timer_req_t *)(q.next))->link.prev = &q;
	((midi_timer_req_t *)(q.prev))->link.next = &q;
	queue_init(&timer->timer_q);

	while (!queue_empty(&q)) {
		midi_timer_req_t *tr;
		queue_remove_first(&q, tr, midi_timer_req_t *, link);
		queue_enter_first(&timer->timer_q, tr, midi_timer_req_t *, 
			link);
	}

	simple_unlock(midi_var.slockp);
	splx(s);

	/*
	 * Since we've changed things, let everyone know.
	 */
	midi_timer_notify_all(timer);

	return KERN_SUCCESS;
}

/*
 * Add an entry to the MIDI timer's queue
 */
kern_return_t midi_timer_timeval_req (	// request a message at specified time
	port_name_t	timer_port,	// timer port
	port_name_t	reply_port,	// where to send timer event to
	timeval_t	timeval,	// secs/usecs of (until) request
	boolean_t	relative_time)	// relative (!absolute) timer
{
	midi_timer_t *timer;
	int unit;
	int s;

	if (timer_port == midi_var.dev[0].timer.port)
		timer = &midi_var.dev[unit = 0].timer;
	else if (timer_port == midi_var.dev[1].timer.port)
		timer = &midi_var.dev[unit = 1].timer;
	else if (curipl() != 0) {
		midi_timer_req_t *tr;

		timer = &midi_var.dev[0].timer;
		tr = midi_timer_req_alloc(timer);

		/*
		 * Queue this up to be sent later.
		 */
		tr->timeval = timeval;
		tr->port = timer_port;
		tr->reply_port = reply_port;
		tr->relative = relative_time;
		tr->quanta = FALSE;

		s = splmidisoft();
		simple_lock(midi_var.slockp);

		queue_enter(&timer->req_q, tr, midi_timer_req_t *, link);

		if (!timer->reqs_queued) {
			typedef void (*void_fun_t)(void *);
			timer->reqs_queued = TRUE;
			(void) kern_serv_callout(&midi_var.kern_server,
				(void_fun_t)midi_timer_run_req_queue,
				(void *)timer);
		}

		simple_unlock(midi_var.slockp);
		splx(s);

		return KERN_SUCCESS;
	} else
		return timer_timeval_req(timer_port, reply_port,
			timeval, relative_time);

	midi_tlog("midi_timer_timeval_req[%d] timeval %d.%d %s port %d\n",
		unit, timeval.tv_sec, timeval.tv_usec,
		relative_time ? "relative" : "absolute", reply_port);

	/*
	 * Find out what time it is so that we can see if the event's
	 * already expired, or to turn a relative event into absolute
	 * time.
	 */
	s = splmidi();
	simple_lock(midi_var.slockp);

	midi_timer_update_time(timer);

	simple_unlock(midi_var.slockp);
	splx(s);

	if (relative_time) {
		timevaladd(&timeval, &timer->cur_time);

		midi_tlog("midi_timer_timeval_req[%d] timeval %d.%d (abs)\n",
			unit, timeval.tv_sec, timeval.tv_usec, 4, 5);

	}

	/*
	 * If the timer's already expired (as it quite often has)
	 * send it now.
	 */
	if (!timercmp(&timeval, &timer->cur_time, >)) {
		kern_return_t (*te) (
			port_t		reply_port,
			timeval_t	timeval,
			u_int		quanta,
			u_int		usec_per_quantum,
			u_int		real_usec_per_quantum,
			boolean_t	timer_expired,
			boolean_t	timer_stopped,
			boolean_t	timer_forward);

		midi_tlog("midi_timer_timeval_req: expired, send\n",
			1, 2, 3, 4, 5);

		te = (  midi_timer_from_reply_port(reply_port)
		      ? midi_timer_event
		      : timer_event);
		(*te)(reply_port,
			timer->cur_time,
			tv_to_quanta(&timer->cur_time),
			timer->usec_per_q,
			timer->act_usec_per_q,
			TRUE,		// timer expired
			timer->paused,
			timer->forward);
		return KERN_SUCCESS;
	}

	return midi_timer_req(timer, reply_port, &timeval);
}

kern_return_t midi_timer_quanta_req (	// request a message at specified time
	port_name_t	timer_port,	// timer port
	port_name_t	reply_port,	// where to send timer event to
	int		quanta,		// quanta of (until) request
	boolean_t	relative_time)	// relative (!absolute) timer
{
	timeval_t timeval;
	midi_timer_t *timer;
	int s;
	int unit;

	if (timer_port == midi_var.dev[0].timer.port)
		timer = &midi_var.dev[unit = 0].timer;
	else if (timer_port == midi_var.dev[1].timer.port)
		timer = &midi_var.dev[unit = 1].timer;
	else if (curipl() != 0) {
		midi_timer_req_t *tr;

		timer = &midi_var.dev[0].timer;
		tr = midi_timer_req_alloc(timer);

		/*
		 * Queue this up to be sent later.
		 */
		tr->tr_quanta = quanta;
		tr->port = timer_port;
		tr->reply_port = reply_port;
		tr->relative = relative_time;
		tr->quanta = TRUE;

		s = splmidisoft();
		simple_lock(midi_var.slockp);

		queue_enter(&timer->req_q, tr, midi_timer_req_t *, link);

		if (!timer->reqs_queued) {
			typedef void (*void_fun_t)(void *);
			timer->reqs_queued = TRUE;
			(void) kern_serv_callout(&midi_var.kern_server,
				(void_fun_t)midi_timer_run_req_queue,
				(void *)timer);
		}

		simple_unlock(midi_var.slockp);
		splx(s);

		return KERN_SUCCESS;
	} else
		return timer_quanta_req(timer_port, reply_port,
			quanta, relative_time);

	midi_tlog("midi_timer_quanta_req[%d] quanta %d %s port %d\n",
		unit, quanta, relative_time ? "relative" : "absolute",
		reply_port, 5);

	quanta_to_tv(quanta, &timeval);

	/*
	 * Find out what time it is so that we can see if the event's
	 * already expired, or to turn a relative event into absolute
	 * time.
	 */
	s = splmidi();
	simple_lock(midi_var.slockp);

	midi_timer_update_time(timer);

	simple_unlock(midi_var.slockp);
	splx(s);

	if (relative_time) {
		timevaladd(&timeval, &timer->cur_time);

		midi_tlog("midi_timer_quanta_req[%d] timeval %d.%d (abs)\n",
			unit, timeval.tv_sec, timeval.tv_usec, 4, 5);

	}

	midi_tlog("midi_timer_quanta_req[%d] timeval %d.%d (abs)\n",
		unit, timeval.tv_sec, timeval.tv_usec, 4, 5);

	/*
	 * If the timer's already expired (as it quite often has)
	 * send it now.
	 */
	if (!timercmp(&timeval, &timer->cur_time, >)) {
		kern_return_t (*te) (
			port_t		reply_port,
			timeval_t	timeval,
			u_int		quanta,
			u_int		usec_per_quantum,
			u_int		real_usec_per_quantum,
			boolean_t	timer_expired,
			boolean_t	timer_stopped,
			boolean_t	timer_forward);

		midi_tlog("midi_timer_quanta_req: expired, send\n",
			1, 2, 3, 4, 5);

		te = (  midi_timer_from_reply_port(reply_port)
		      ? midi_timer_event
		      : timer_event);
		(*te)(reply_port,
			timer->cur_time,
			tv_to_quanta(&timer->cur_time),
			timer->usec_per_q,
			timer->act_usec_per_q,
			TRUE,		// timer expired
			timer->paused,
			timer->forward);
		return KERN_SUCCESS;
	}

	return midi_timer_req(timer, reply_port, &timeval);
}

/*
 * Process each each enqueued timer request that something's changed.
 * Also notifies and frees those that have expired.
 * Assumes that cur_time is up to date.
 */
static void midi_timer_notify_all(midi_timer_t *timer)
{
	midi_timer_req_t *tr;
	/* u_int quanta = tv_to_quanta(&timer->cur_time);...not used dpm */
	boolean_t expired = TRUE;
	int s;

	s = splmidisoft();
	simple_lock(midi_var.slockp);

	/*
	 * Scan the timeout queue processing any (newly) expired timers.
	 */
	tr = (midi_timer_req_t *)queue_first(&timer->timer_q);
	while (!queue_end(&timer->timer_q, (queue_t)tr)) {
		port_name_t port = tr->port;
		timeval_t *tvp = &tr->timeval;

		/*
		 * Check for a transition from expired to !expired
		 */
		if (   expired
		    && ((   timer->forward
			    && timercmp(tvp, &timer->cur_time, >))
			|| (   !timer->forward
			    && timercmp(tvp, &timer->cur_time, <))))
			expired = FALSE;
			
		/*
		 * Send the event.
		 */
		if (   port==midi_var.dev[0].dir[MIDI_DIR_XMIT].in_timer_reply
		    || port==midi_var.dev[0].dir[MIDI_DIR_RECV].in_timer_reply
		    || port==midi_var.dev[1].dir[MIDI_DIR_XMIT].in_timer_reply
		    || port==midi_var.dev[1].dir[MIDI_DIR_RECV].in_timer_reply)
		{
			/*
			 * Send the event.
			 */
			midi_tlog("midi_timer_notify_all: "
				  "send local event\n", 1, 2, 3, 4, 5);
	
			softint_sched(CALLOUT_PRI_SOFTINT1,
				  expired
				? midi_timer_sched_event
				: midi_timer_sched_event_no_expire,
				(int)port);
		} else {
			midi_timer_req_t *tr2 = midi_timer_req_alloc(timer);
			/* FIXME: midi_timer_req_alloc can return NULL */
			midi_tlog("midi_timer_notify_all: queue event\n",
				1, 2, 3, 4, 5);

			tr2->port = port;
			tr2->timeval = *tvp;

			queue_enter(&timer->event_q, tr2, midi_timer_req_t *,
				link);

			if (!timer->events_queued) {
				typedef void (*void_fun_t)(void *);
				timer->events_queued = TRUE;
				(void) kern_serv_callout(&midi_var.kern_server,
				    (void_fun_t)midi_timer_run_event_queue,
				    (void *)timer);
			}
		}

		if (expired && (queue_t)tr == queue_first(&timer->timer_q)) {
			midi_timer_req_t *tr2 = tr;
			tr = (midi_timer_req_t *)queue_next(&tr2->link);
			queue_remove(&timer->timer_q, tr2, midi_timer_req_t *,
				     link);
			midi_timer_req_free(timer, tr2);
		} else
			tr = (midi_timer_req_t *)queue_next(&tr->link);

		midi_tlog("midi_timer_notify_all: %s\n",
			  queue_end(&timer->timer_q, (queue_t)tr)
			? "queue_end"
			: "next_elt",
			2, 3, 4, 5);
	}

	/*
	 * Start a timer up for the next thing in the queue (maybe)
	 */
	us_untimeout((int (*)())midi_timer_timeout, (int)timer);
	midi_timer_timeout(timer);

	simple_unlock(midi_var.slockp);
	splx(s);
}

/*
 * Server routines for midi_timer_reply.
 */
kern_return_t midi_timer_event (
	port_t		reply_port,
	timeval_t	timeval,
	u_int		quanta,
	u_int		usec_per_quantum,
	u_int		real_usec_per_quantum,
	boolean_t	timer_expired,
	boolean_t	timer_stopped,
	boolean_t	timer_forward)
{
	int unit, dir;

	if (   reply_port
	    == midi_var.dev[0].dir[MIDI_DIR_XMIT].in_timer_reply)
	{
		unit = 0; dir = MIDI_DIR_XMIT;
	} else if (   reply_port
		   == midi_var.dev[0].dir[MIDI_DIR_RECV].in_timer_reply)
	{
		unit = 0; dir = MIDI_DIR_RECV;
	} else if (   reply_port
		   == midi_var.dev[1].dir[MIDI_DIR_XMIT].in_timer_reply)
	{
		unit = 1; dir = MIDI_DIR_XMIT;
	} else if (   reply_port
		   == midi_var.dev[1].dir[MIDI_DIR_RECV].in_timer_reply)
	{
		unit = 1; dir = MIDI_DIR_RECV;
	} else {
		ASSERT(0);
	}

	midi_tlog("midi_timer_event: quanta %d %s%s%s%s\n",
		quanta,
		dir == MIDI_DIR_XMIT ? "xmit" : "recv",
		timer_expired ? " expire" : "",
		timer_stopped ? " stopped" : "",
		timer_forward ? " forward" : "");

	/*
	 * This is a request for some internal timer
	 */
	if (dir == MIDI_DIR_XMIT)
		midi_device_xmit_timer_event(quanta, real_usec_per_quantum,
			timer_expired, timer_stopped, timer_forward, unit);
	else
		midi_device_recv_timer_event(quanta, real_usec_per_quantum,
			timer_expired, timer_stopped, timer_forward, unit);

	midi_tlog("midi_timer_event: done\n", 1, 2, 3, 4, 5);

	return KERN_SUCCESS;
}

/*
 * Place the request at the proper place within the queue.
 */
static void midi_timer_insert_req(midi_timer_t *timer, midi_timer_req_t *tr)
{
	midi_timer_req_t *tr2;
	boolean_t reload_timer = FALSE;
	int s;

	if (curipl() <= MIDI_SOFTIPL) {
		s = splmidisoft();
		simple_lock(midi_var.slockp);
	}

	tr2 = (midi_timer_req_t *)queue_first(&timer->timer_q);

	if (queue_empty(&timer->timer_q)) {
		queue_enter(&timer->timer_q, tr, midi_timer_req_t *, link);
		reload_timer = TRUE;
		midi_tlog("midi_timer_insert_req: (empty) "
			  "insert at head, reload\n", 1, 2, 3, 4, 5);
	} else if (timer->forward) {
		midi_tlog("midi_timer_insert_req: "
			"qhead time %d.%d, req %d.%d\n",
			tr2->timeval.tv_sec, tr2->timeval.tv_usec,
			tr->timeval.tv_sec, tr->timeval.tv_usec, 5);
		if (timercmp(&tr->timeval, &tr2->timeval, <)) {
			queue_enter_first(&timer->timer_q, tr,
				midi_timer_req_t *, link);
			reload_timer = TRUE;
			midi_tlog("midi_timer_insert_req: < insert at head, "
				  "reload\n", 1, 2, 3, 4, 5);
		} else {
			/*
			 * Insert from the end of the queue.
			 */
			tr2 = (midi_timer_req_t *)queue_last(&timer->timer_q);
			while (   !queue_end(&timer->timer_q, (queue_t)tr2)
			       && timercmp(&tr->timeval, &tr2->timeval, <))
				tr2 = (midi_timer_req_t *)
					queue_prev(&tr2->link);
			midi_tlog("midi_timer_insert_req: < insert in "
				  "middle\n", 1, 2, 3, 4, 5);
		}
	} else {
		if (timercmp(&tr->timeval, &tr2->timeval, >)) {
			queue_enter_first(&timer->timer_q, tr,
				midi_timer_req_t *, link);
			reload_timer = TRUE;
		} else {
			/*
			 * Insert from the end of the queue.
			 */
			tr2 = (midi_timer_req_t *)queue_last(&timer->timer_q);
			while (   !queue_end(&timer->timer_q, (queue_t)tr2)
			       && !timercmp(&tr->timeval, &tr2->timeval, >))
				tr2 = (midi_timer_req_t *)
					queue_prev(&tr2->link);
		}
	}

	if (reload_timer) {
		if (curipl() == MIDI_SOFTIPL) {
			simple_unlock(midi_var.slockp);
			splx(s);
		}

		midi_tlog("midi_timer_insert_req: sched timeout "
			  "for reload\n", 1, 2, 3, 4, 5);

		us_untimeout((int (*)())midi_timer_timeout, (int)timer);
		softint_sched(CALLOUT_PRI_SOFTINT1,
			midi_timer_timeout, (int)timer);
	} else {
		/*
		 * Append tr after tr2.
		 */
		midi_tlog("midi_timer_insert_req: enter in queue\n",
			1, 2, 3, 4, 5);
		ASSERT(!queue_end(&timer->timer_q, (queue_t)tr2));

		tr->link.prev = (queue_t)tr2;

		if ((tr->link.next = tr2->link.next) == &timer->timer_q)
			timer->timer_q.prev = (queue_t)tr;
		else
			((midi_timer_req_t *)(tr->link.next))->link.prev =
				(queue_t)tr;

		tr2->link.next = (queue_t)tr;

		if (curipl() == MIDI_SOFTIPL) {
			simple_unlock(midi_var.slockp);
			splx(s);
		}
	}

	midi_tlog("midi_timer_insert_req: done\n", 1, 2, 3, 4, 5);
}

/*
 * Find the queued request associated with the given port.
 */
static midi_timer_req_t *midi_timer_req_from_port(queue_t q, port_name_t port)
{
	midi_timer_req_t *tr;
	int s;

	ASSERT (curipl() >= MIDI_SOFTIPL);

	tr = (midi_timer_req_t *)queue_first(q);
	for (  tr = (midi_timer_req_t *)queue_first(q)
	     ; !queue_end(q, (queue_t)tr)
	     ; tr = (midi_timer_req_t *)queue_next(&tr->link))
	{
		if (tr->port == port) {
			return tr;
		}
	}

	return 0;
}

/* #define	NO_TIMER_ALLOC_IPL 1	/* no kalloc() at ipl > 0 */

static midi_timer_req_t *midi_timer_req_alloc(midi_timer_t *timer)
{
	midi_timer_req_t *tr;
	int s;

	if (timer->free_q_size == 0) {
		if (curipl() == 0) {
			midi_tlog("midi_timer_req_alloc: kalloc\n",
				1, 2, 3, 4, 5);
			while (timer->free_q_size < MIDI_TIMER_REQ_MIN) {
				tr = (midi_timer_req_t *)kalloc(sizeof(*tr));
				s = splmidisoft();
				simple_lock(midi_var.slockp);
				queue_enter(&timer->free_q, tr,
					midi_timer_req_t *, link);
				timer->free_q_size++;
				simple_unlock(midi_var.slockp);
				splx(s);
			}
			midi_tlog("midi_timer_req_alloc: fq size now %d\n",
				timer->free_q_size, 2, 3, 4, 5);
		} else {
#ifdef	NO_TIMER_ALLOC_IPL
			midi_tlog("midi_timer_req_alloc: ipl too high\n",
				1, 2, 3, 4, 5);
			panic("Could not alloc midi_timer_req_t\n");
#else	NO_TIMER_ALLOC_IPL
			/* well, we can allocate ONE timer... */
			tr = (midi_timer_req_t *)kalloc(sizeof(*tr));
			s = splmidisoft();
			simple_lock(midi_var.slockp);
			queue_enter(&timer->free_q, tr,
				midi_timer_req_t *, link);
			timer->free_q_size++;
			simple_unlock(midi_var.slockp);
			splx(s);
			return tr;
#endif	NO_TIMER_ALLOC_IPL
		}
	}

	if (curipl() <= MIDI_SOFTIPL) {
		s = splmidisoft();
		simple_lock(midi_var.slockp);
	}

	queue_remove_first(&timer->free_q, tr, midi_timer_req_t *, link);
	timer->free_q_size--;

	if (curipl() == MIDI_SOFTIPL) {
		simple_unlock(midi_var.slockp);
		splx(s);
	}

	midi_tlog("midi_timer_req_alloc: fq size now %d, return 0x%x\n",
		timer->free_q_size, tr, 3, 4, 5);

	return tr;
}

static void midi_timer_req_free(midi_timer_t * timer, midi_timer_req_t *tr)
{
	int enter_ipl = curipl();
	int s;

	midi_tlog("midi_timer_req_free(0x%x): fq size now %d\n", tr,
		timer->free_q_size + (tr ? 1 : 0), 3, 4, 5);
	if (enter_ipl < MIDI_SOFTIPL) {
		s = splmidisoft();
		simple_lock(midi_var.slockp);
	}

	if (tr) {
		queue_enter(&timer->free_q, tr, midi_timer_req_t *, link);
		timer->free_q_size++;
	}

	if (timer->free_q_size > MIDI_TIMER_REQ_MIN && enter_ipl == 0) {
		midi_tlog("midi_timer_req_free: fq too big, kfree\n",
			1, 2, 3, 4, 5);
		while (timer->free_q_size > MIDI_TIMER_REQ_MIN) {
			ASSERT(!queue_empty(&timer->free_q));
			queue_remove_first(&timer->free_q, tr,
				midi_timer_req_t *, link);
			simple_unlock(midi_var.slockp);
			splx(s);
			kfree(tr, sizeof(*tr));
			s = splmidisoft();
			simple_lock(midi_var.slockp);
		}
		midi_tlog("midi_timer_req_free: fq size now %d\n",
			timer->free_q_size, 2, 3, 4, 5);
	}

	if (enter_ipl < MIDI_SOFTIPL) {
		simple_unlock(midi_var.slockp);
		splx(s);
	}
}

/*
 * Process everything in the queue who's time has come
 */
static void midi_timer_timeout(midi_timer_t *timer)
{
	midi_timer_req_t *tr;
	int quanta, enter_ipl = curipl();
	int s;

	midi_tlog("midi_timer_timeout: start\n", 1, 2, 3, 4, 5);

	if (enter_ipl < MIDI_SOFTIPL) {
		s = splmidisoft();
		simple_lock(midi_var.slockp);
	}

	/*
	 * Update the current time based on the number of quanta that
	 * we needed to wait for.
	 */
	if (timer->q_of_cur_req && timer->mode != mt_system) {
		quanta = tv_to_quanta(&timer->cur_time);
		if (timer->forward)
			quanta += timer->q_of_cur_req;
		else
			quanta -= timer->q_of_cur_req;
		quanta_to_tv(quanta, &timer->cur_time);
		timer->q_of_cur_req = 0;
		microboot(&timer->req_time);
	} else if (!timer->paused) {
		timeval_t tv, tv2;

		microboot(&tv);
		midi_tlog("midi_timer_timeout: tv %d.%d, req_time %d.%d",
			tv.tv_sec, tv.tv_usec,
			timer->req_time.tv_sec, timer->req_time.tv_usec, 5);
		tv2 = tv;
		timevalsub(&tv, &timer->req_time);
		if (timer->forward)
			timevaladd(&timer->cur_time, &tv);
		else
			timevalsub(&timer->cur_time, &tv);
		midi_tlog(" delta %d.%d, cur_time %d.%d\n",
			tv.tv_sec, tv.tv_usec,
			timer->cur_time.tv_sec, timer->cur_time.tv_usec, 5);
		quanta = tv_to_quanta(&timer->cur_time);
		timer->q_of_cur_req = 0;
		timer->req_time = tv2;
	} else {
		quanta = tv_to_quanta(&timer->cur_time);
		timer->q_of_cur_req = 0;
	}

	/*
	 * Process all entries.
	 */
    	tr = (midi_timer_req_t *)queue_first(&timer->timer_q);
	while (!queue_end(&timer->timer_q, (queue_t)tr)) {
		midi_tlog("midi_timer_timeout: compare %d.%d to cur_time\n",
			tr->timeval.tv_sec, tr->timeval.tv_usec, 3, 4, 5);
		if (   (   timer->forward
			&& timercmp(&tr->timeval, &timer->cur_time, >))
		    || (   !timer->forward
			&& timercmp(&tr->timeval, &timer->cur_time, <)))
				break;
		queue_remove(&timer->timer_q, tr, midi_timer_req_t *,
			link);

		if (       tr->port
			== midi_var.dev[0].dir[MIDI_DIR_XMIT].in_timer_reply
		    ||     tr->port
			== midi_var.dev[0].dir[MIDI_DIR_RECV].in_timer_reply
		    ||     tr->port
			== midi_var.dev[1].dir[MIDI_DIR_XMIT].in_timer_reply
		    ||     tr->port
			== midi_var.dev[1].dir[MIDI_DIR_RECV].in_timer_reply)
		{
			/*
			 * Send the event.
			 */
			midi_tlog("midi_timer_timeout: local event "
				  "softint\n", 1, 2, 3, 4, 5);
	
			softint_sched(CALLOUT_PRI_SOFTINT1,
				midi_timer_sched_event, (int)tr->port);

			midi_timer_req_free(timer, tr);
		} else if (enter_ipl == 0) {
			simple_unlock(midi_var.slockp);
			splx(s);

			/*
			 * Send the event.
			 */
			midi_tlog("midi_timer_timeout: send event "
				  "on port %d\n", tr->port, 2, 3, 4, 5);
	
			timer_event(tr->port,
				timer->cur_time,
				quanta,
				timer->usec_per_q,
				timer->act_usec_per_q,
				TRUE,		// timer expired
				timer->paused,
				timer->forward);
	
			s = splmidisoft();
			simple_lock(midi_var.slockp);

			midi_timer_req_free(timer, tr);
		} else {
			midi_tlog("midi_timer_timeout: queue event\n",
				1, 2, 3, 4, 5);

			queue_enter(&timer->event_q, tr, midi_timer_req_t *,
				link);

			if (!timer->events_queued) {
				typedef void (*void_fun_t)(void *);
				timer->events_queued = TRUE;
				(void) kern_serv_callout(&midi_var.kern_server,
				    (void_fun_t)midi_timer_run_event_queue,
				    (void *)timer);
			}
		}

		tr = (midi_timer_req_t *)queue_first(&timer->timer_q);
	}

	/*
	 * Start a timer up for the next thing in the queue (maybe)
	 */
	if (!timer->paused && !queue_end(&timer->timer_q, (queue_t)tr)) {
		u_int delay;

		ASSERT(!timer->q_of_cur_req);
		if (timer->forward)
			delay = tv_to_quanta(&tr->timeval) - quanta;
		else
			delay = quanta - tv_to_quanta(&tr->timeval);
		ASSERT(delay);

		if (   timer->mode == mt_system
		    || (   quanta/timer->q_per_clock
			== (quanta+delay)/timer->q_per_clock))
		{
			timeval_t tv;
			tv.tv_sec = (delay*timer->act_usec_per_q) / 1000000;
			tv.tv_usec = (delay*timer->act_usec_per_q) % 1000000;
			timevaladd(&tv, &timer->req_time);
			midi_tlog("midi_timer_timeout: timeout at %d.%d\n",
				tv.tv_sec, tv.tv_usec, 3, 4, 5);
			us_abstimeout((int (*)())midi_timer_timeout,
				(caddr_t)timer, &tv, MIDI_SOFTPRI);
			timer->q_of_cur_req = delay;
		}
	}

	midi_tlog("midi_timer_timeout: done\n", 1, 2, 3, 4, 5);
	if (enter_ipl < MIDI_SOFTIPL) {
		simple_unlock(midi_var.slockp);
		splx(s);
	}
}

/*
 * Update the current time using act_usec_per_q.  Re-schedules timeout events.
 */
static void midi_timer_update_time(midi_timer_t *timer)
{
    	timeval_t tv;

	ASSERT(curipl() >= MIDI_SOFTIPL);

	midi_tlog("midi_timer_update_time: start\n", 1, 2, 3, 4, 5);

	if (timer->paused) {
		midi_tlog("midi_timer_update_time: (paused) time is %d.%d\n",
			timer->cur_time.tv_sec, timer->cur_time.tv_usec,
			3, 4, 5);
		return;
	}

	if (timer->mode != mt_system) {
		u_int quanta;
		u_int delta;

		/*
		 * Update cur_time based un number of quanta elapsed
		 * since last request was made.  Don't go over a clock
		 * tick boundary.
		 */
		/*
		 * Turn actual sec/microseconds into (old) quanta
		 */
		microboot(&tv);
		timevalsub(&tv, timer->req_time);
		delta = (tv.tv_usec + tv.tv_sec*1000000)/timer->act_usec_per_q;

		/*
		 * Get cur_time as quanta.
		 */
		quanta = tv_to_quanta(&timer->cur_time);

		if (timer->forward) {
			/*
			 * Don't update beyound (including) next
			 * external sync point.
			 */
			delta += quanta;
			if (   delta/timer->q_per_clock
			    == quanta/timer->q_per_clock)
				quanta = delta;
			else
				quanta =   (quanta/timer->q_per_clock + 1)
					 * timer->q_per_clock - 1;
		} else {
			int base;

			/*
			 * Don't update beyound (including) next
			 * external sync point.
			 */
			base =    ((quanta - delta)/timer->q_per_clock)
				* timer->q_per_clock + 1;
			if (base < quanta - delta)
				quanta -= delta;
			else
				quanta = base;
			
		}

		/*
		 * Turn quanta back into clock-based sec/microsecs
		 */
		quanta_to_tv(quanta, &timer->cur_time);

		/*
		 * Re-establish time-base.
		 */
		microboot(&timer->req_time);
	} else {
		timeval_t tv2;
		microboot(&tv);
		tv2 = tv;
		timevalsub(&tv, &timer->req_time);
		midi_tlog("midi_timer_update_time: add/sub %d.%d to %d.%d\n",
			tv.tv_sec, tv.tv_usec, timer->cur_time.tv_sec,
			timer->cur_time.tv_usec, 5);
		if (timer->forward)
			timevaladd(&timer->cur_time, &tv);
		else
			timevalsub(&timer->cur_time, &tv);
		timer->req_time = tv2;
	}

	if (timer->q_of_cur_req) {
		timer->q_of_cur_req = 0;
		midi_tlog("midi_timer_update_time: remove timeout entry\n",
			1, 2, 3, 4, 5);
		us_untimeout((int (*)())midi_timer_timeout, (int)timer);
		midi_timer_timeout(timer);
	}

	midi_tlog("midi_timer_update_time: time is %d.%d\n",
		timer->cur_time.tv_sec, timer->cur_time.tv_usec, 3, 4, 5);
}

/*
 * Called from the msg_receive thread in the generic kernel server
 * to send queued timer event messages.
 */
static void midi_timer_run_event_queue(midi_timer_t *timer)
{
	int s, quanta;

	ASSERT(curipl() == 0);

	s = splmidisoft();
	simple_lock(midi_var.slockp);

	timer->events_queued = FALSE;

	midi_timer_update_time(timer);
	quanta = tv_to_quanta(&timer->cur_time);

	while (!queue_empty(&timer->event_q)) {
		midi_timer_req_t *tr;

		queue_remove_first(&timer->event_q, tr,
			midi_timer_req_t *, link);

		simple_unlock(midi_var.slockp);
		splx(s);

		midi_tlog("midi_timer_run_event_queue: send event\n",
			1, 2, 3, 4, 5);

		timer_event(tr->port,
			    timer->cur_time,
			    quanta,
			    timer->usec_per_q,
			    timer->act_usec_per_q,
			    TRUE,		// timer expired
			    timer->paused,
			    timer->forward);

		midi_timer_req_free(timer, tr);

		s = splmidisoft();
		simple_lock(midi_var.slockp);
	}

	simple_unlock(midi_var.slockp);
	splx(s);
}

/*
 * Called from the msg_receive thread in the generic kernel server
 * to send queued timer request messages.
 */
static void midi_timer_run_req_queue(midi_timer_t *timer)
{
	int s, quanta;

	ASSERT(curipl() == 0);

	s = splmidisoft();
	simple_lock(midi_var.slockp);

	timer->reqs_queued = FALSE;

	while (!queue_empty(&timer->req_q)) {
		midi_timer_req_t *tr;

		queue_remove_first(&timer->req_q, tr, midi_timer_req_t *,
			link);

		simple_unlock(midi_var.slockp);
		splx(s);

		midi_tlog("midi_timer_run_req_queue: send event\n",
			1, 2, 3, 4, 5);

		if (tr->quanta)
			timer_quanta_req(tr->port,
				tr->reply_port,
				tr->tr_quanta,
				tr->relative);
		else
			timer_timeval_req(tr->port,
				tr->reply_port,
				tr->timeval,
				tr->relative);

		midi_timer_req_free(timer, tr);

		s = splmidisoft();
		simple_lock(midi_var.slockp);
	}

	simple_unlock(midi_var.slockp);
	splx(s);
}

/*
 * Run a local timer event from softint.
 */
static void midi_timer_sched_event(port_t port)
{
	midi_timer_t *timer;

	timer = midi_timer_from_reply_port(port);

	midi_timer_event(port,
		timer->cur_time,
		tv_to_quanta(&timer->cur_time),
		timer->usec_per_q,
		timer->act_usec_per_q,
		TRUE,		// timer expired
		timer->paused,
		timer->forward);
}

/*
 * Run a local timer event from softint.
 */
static void midi_timer_sched_event_no_expire(port_t port)
{
	midi_timer_t *timer;

	timer = midi_timer_from_reply_port(port);

	midi_timer_event(port,
		timer->cur_time,
		tv_to_quanta(&timer->cur_time),
		timer->usec_per_q,
		timer->act_usec_per_q,
		FALSE,		// timer didn't expired
		timer->paused,
		timer->forward);
}

static midi_timer_t *midi_timer_from_reply_port(port_t port)
{
	if (port == midi_var.dev[0].dir[MIDI_DIR_XMIT].in_timer_reply) {
		if (   midi_var.dev[0].dir[MIDI_DIR_XMIT].in_timer
		    == midi_var.dev[0].timer.port)
		    	return &midi_var.dev[0].timer;
		else
		    	return &midi_var.dev[1].timer;
	} else if (port == midi_var.dev[0].dir[MIDI_DIR_RECV].in_timer_reply) {
		if (   midi_var.dev[0].dir[MIDI_DIR_RECV].in_timer
		    == midi_var.dev[0].timer.port)
		    	return &midi_var.dev[0].timer;
		else
		    	return &midi_var.dev[1].timer;
	} else if (port == midi_var.dev[1].dir[MIDI_DIR_XMIT].in_timer_reply) {
		if (   midi_var.dev[1].dir[MIDI_DIR_XMIT].in_timer
		    == midi_var.dev[0].timer.port)
		    	return &midi_var.dev[0].timer;
		else
		    	return &midi_var.dev[1].timer;
	} else if (port == midi_var.dev[1].dir[MIDI_DIR_RECV].in_timer_reply) {
		if (   midi_var.dev[1].dir[MIDI_DIR_RECV].in_timer
		    == midi_var.dev[0].timer.port)
		    	return &midi_var.dev[0].timer;
		else
		    	return &midi_var.dev[1].timer;
	}
	return 0;
}



