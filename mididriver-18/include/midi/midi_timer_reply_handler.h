/* 
 * Mach Operating System
 * Copyright (c) 1988 NeXT, Inc.
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */
/*
 * HISTORY
 * 08-Jun-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#ifndef _MIDI_TIMER_REPLY_HANDLER_
#define _MIDI_TIMER_REPLY_HANDLER_

#include <sys/kern_return.h>
#include <sys/port.h>
#include <sys/message.h>

#include <midi/midi_types.h>

/*
 * Functions to call for handling messages returned.
 */
typedef struct midi_timer_reply {
	kern_return_t	(*timer_event)(
				void *		arg,
				timeval_t	timeval,
				u_int		quanta,
				u_int		usec_per_quantum,
				u_int		real_usec_per_quantum,
				boolean_t	timer_expired,
				boolean_t	timer_stopped,
				boolean_t	timer_forward);
	void *		arg;		// argument to pass to function
	msg_timeout_t	timeout;	// timeout for RPC return msg_send
} midi_timer_reply_t;

/*
 * Sizes of messages structures for send and receive.
 */
union midi_timer_reply_request {
	struct {
		msg_header_t Head;
		msg_type_t timevalType;
		timeval_t timeval;
		msg_type_t quantaType;
		u_int quanta;
		msg_type_t usec_per_quantumType;
		u_int usec_per_quantum;
		msg_type_t real_usec_per_quantumType;
		u_int real_usec_per_quantum;
		msg_type_t timer_expiredType;
		boolean_t timer_expired;
		msg_type_t timer_stoppedType;
		boolean_t timer_stopped;
		msg_type_t timer_forwardType;
		boolean_t timer_forward;
	} timer_event;
};
#define MIDI_TIMER_REPLY_INMSG_SIZE sizeof(union midi_timer_reply_request)

union midi_timer_reply_reply {
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} timer_event;
};
#define MIDI_TIMER_REPLY_OUTMSG_SIZE sizeof(union midi_timer_reply_reply)

/*
 * Handler routine to call when receiving messages from midi driver.
 */
kern_return_t midi_timer_reply_handler (
	msg_header_t *msg,
	midi_timer_reply_t *midi_timer_reply);

#endif	_MIDI_TIMER_REPLY_HANDLER_

