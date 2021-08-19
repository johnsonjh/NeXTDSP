/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 07-Jun-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#import <midi/midi_timer_reply_handler.h>
#import "midi_timer_replyServer.c"

/*
 * The port argument in each of the following is actually a pointer
 * to a structure containing pointers to functions to call for each of
 * the following messages.
 */

kern_return_t timer_event
(
	port_t reply_port,
	timeval_t timeval,
	u_int quanta,
	u_int usec_per_quantum,
	u_int real_usec_per_quantum,
	boolean_t timer_expired,
	boolean_t timer_stopped,
	boolean_t timer_forward)
{
	midi_timer_reply_t *midi_timer_reply =
		(midi_timer_reply_t *)reply_port;

	if (midi_timer_reply->timer_event == 0)
		return MIG_BAD_ID;
	return (*midi_timer_reply->timer_event)(
		midi_timer_reply->arg, timeval, quanta, usec_per_quantum,
		real_usec_per_quantum, timer_expired, timer_stopped,
		timer_forward);
}

kern_return_t midi_timer_reply_handler (
	msg_header_t *msg,
	midi_timer_reply_t *midi_timer_reply)
{
	char out_msg_buf[MIDI_TIMER_REPLY_OUTMSG_SIZE];
	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} Reply;
	Reply *out_msg = (Reply *)out_msg_buf;
	kern_return_t ret_code;
	port_t local_port = msg->msg_local_port;

	msg->msg_local_port = (port_t)midi_timer_reply;

	midi_timer_reply_server(msg, (msg_header_t *)out_msg);
	ret_code = out_msg->RetCode;

	if (out_msg->RetCode == MIG_NO_REPLY)
		ret_code = KERN_SUCCESS;

	return ret_code;
}

