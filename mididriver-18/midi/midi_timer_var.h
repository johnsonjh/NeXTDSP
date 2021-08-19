/* 
 * Mach Operating System
 * Copyright (c) 1988 NeXT, Inc.
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */
/*
 * HISTORY
 * 19-May-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#ifndef _MIDI_TIMER_
#define _MIDI_TIMER_
#import <midi/midi_types.h>
#import <sys/time.h>

/*
 * Information contained with in device instance varables for keeping
 * track of timer state.
 */

typedef struct midi_timer {
	port_name_t	port;		// generated timer port
	queue_head_t	timer_q;	// queue of timer requests
	queue_head_t	free_q;		// unused midi_timer_req_t's
	queue_head_t	event_q;	// queued events to send
	queue_head_t	req_q;		// queued requests to send
	u_int		free_q_size;	// number enqueued

	/* cur_time is the timer's current 'clock' value. req_time parallels
	 * cur_time, but it is in microboot() time reference. cur_time is
	 * updated by doing event_delta(last_req) and adding the offset to 
	 * cur_time.
	 */
	timeval_t	cur_time;	// what time is it now?
	timeval_t	req_time;	// time last request made.
	u_int		usec_per_q;	// number of usec per quantum
	u_int		act_usec_per_q;	// actual ""
	u_int		q_per_clock;	// (non-sys mode)
	u_int		q_of_cur_req;	// quanta delay currently requested
	enum midi_timer_mode {
		mt_system, mt_MTC, mt_clock
	} mode;				// how clock is operating.
	u_int		forward:1,	// timer direction.
			paused:1,	// is timer paused?
			events_queued:1,// are events already queued to be run?
			reqs_queued:1;	// are reqs already queued to be run?
} midi_timer_t;

/*
 * An enqueued timer request.
 */
typedef struct midi_timer_req {
	queue_chain_t	link;
	timeval_t	timeval;	// time of event.
#define tr_quanta timeval.tv_sec	// for quanta
	port_name_t	port;		// where to send events
	port_name_t	reply_port;	// reply port (req)
	u_int		relative:1,	// relative time (req)
			quanta:1;	// quanta req (req)
} midi_timer_req_t;

#define MIDI_TIMER_REQ_MIN	64	// was 32 - dpm
#if	KERNEL
/* 
 * exported function prototypes
 */
void midi_timer_init(int unit);
void midi_timer_reset(int unit);
void midi_timer_set_proto(u_int unit, enum midi_timer_mode clock_source);
kern_return_t midi_timer_set(port_t timer_port,	port_t control_port, 
	timeval_t tv);
kern_return_t midi_timer_set_quantum(port_t timer_port, port_t control_port,
	u_int usec_per_quantum);
kern_return_t midi_timer_stop(port_t timer_port, port_t control_port);
kern_return_t midi_timer_start(port_t timer_port, port_t control_port);
kern_return_t midi_timer_direction(port_t timer_port, port_t control_port,
	boolean_t forward);
kern_return_t midi_timer_timeval_req(port_name_t timer_port, 
	port_name_t reply_port, timeval_t timeval, boolean_t relative_time);
kern_return_t midi_timer_quanta_req(port_name_t	timer_port, 
	port_name_t reply_port, int quanta, boolean_t relative_time);
kern_return_t midi_timer_event(
	port_t		reply_port,
	timeval_t	timeval,
	u_int		quanta,
	u_int		usec_per_quantum,
	u_int		real_usec_per_quantum,
	boolean_t	timer_expired,
	boolean_t	timer_stopped,
	boolean_t	timer_forward);
#endif	KERNEL
#endif _MIDI_TIMER_

