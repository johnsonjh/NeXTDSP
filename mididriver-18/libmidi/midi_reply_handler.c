/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 07-Jun-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#import <midi/midi_reply_handler.h>
#import "midi_replyServer.c"

/*
 * The port argument in each of the following is actually a pointer
 * to a structure containing pointers to functions to call for each of
 * the following messages.
 */

kern_return_t midi_ret_raw_data (
	port_t port,
	midi_raw_t midi_raw_data,
	unsigned int midi_raw_dataCnt)
{
	midi_reply_t *midi_reply = (midi_reply_t *)port;

	if (midi_reply->ret_raw_data == 0)
		return MIG_BAD_ID;
	return (*midi_reply->ret_raw_data)(
		midi_reply->arg, midi_raw_data, midi_raw_dataCnt);
}

kern_return_t midi_ret_cooked_data (
	port_t port,
	midi_cooked_t midi_cooked_data,
	unsigned int midi_cooked_dataCnt)
{
	midi_reply_t *midi_reply = (midi_reply_t *)port;

	if (midi_reply->ret_cooked_data == 0)
		return MIG_BAD_ID;
	return (*midi_reply->ret_cooked_data)(
		midi_reply->arg, midi_cooked_data, midi_cooked_dataCnt);
}

/* SimpleRoutine midi_ret_packed_data */
kern_return_t midi_ret_packed_data (
	port_t port,
	u_int quanta,
	midi_packed_t midi_packed_data,
	unsigned int midi_packed_dataCnt)
{
	midi_reply_t *midi_reply = (midi_reply_t *)port;

	if (midi_reply->ret_packed_data == 0)
		return MIG_BAD_ID;
	return (*midi_reply->ret_packed_data)(
		midi_reply->arg, quanta,
		midi_packed_data, midi_packed_dataCnt);
}

/* SimpleRoutine midi_queue_notify */
kern_return_t midi_queue_notify (
	port_t port,
	u_int queue_size)
{
	midi_reply_t *midi_reply = (midi_reply_t *)port;

	if (midi_reply->queue_notify == 0)
		return MIG_BAD_ID;
	return (*midi_reply->queue_notify)(
		midi_reply->arg, queue_size);
}

kern_return_t midi_reply_handler (
	msg_header_t *msg,
	midi_reply_t *midi_reply)
{
	char out_msg_buf[MIDI_REPLY_OUTMSG_SIZE];
	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} Reply;
	Reply *out_msg = (Reply *)out_msg_buf;
	kern_return_t ret_code;
	port_t local_port = msg->msg_local_port;

	msg->msg_local_port = (port_t)midi_reply;

	midi_reply_server(msg, (msg_header_t *)out_msg);
	ret_code = out_msg->RetCode;

	if (out_msg->RetCode == MIG_NO_REPLY)
		ret_code = KERN_SUCCESS;

	return ret_code;
}

