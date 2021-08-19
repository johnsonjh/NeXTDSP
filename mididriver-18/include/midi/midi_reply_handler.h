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

#ifndef _MIDI_REPLY_HANDLER_
#define _MIDI_REPLY_HANDLER_

#include <sys/kern_return.h>
#include <sys/port.h>
#include <sys/message.h>

#include <midi/midi_types.h>

/*
 * Functions to call for handling messages returned
 */
typedef struct midi_reply {
	kern_return_t	(*ret_raw_data)(
				void *		arg,
				midi_raw_t	midi_raw_data,
				u_int		midi_raw_dataCnt);
	kern_return_t	(*ret_cooked_data)(
				void *		arg,
				midi_cooked_t	midi_cooked_data,
				u_int		midi_cooked_dataCnt);
	kern_return_t	(*ret_packed_data)(
				void *		arg,
				u_int		quanta,
				midi_packed_t	midi_packed_data,
				u_int		midi_packed_dataCnt);
	kern_return_t	(*queue_notify)(
				void *		arg,
				u_int		queue_size);
	void *		arg;		// argument to pass to function
	int		timeout;	// timeout for RPC return msg_send
} midi_reply_t;

/*
 * Sizes of messages structures for send and receive.
 */
union midi_reply_request {
	struct {
		msg_header_t Head;
		msg_type_t midi_raw_dataType;
		midi_raw_data_t midi_raw_data[1000];
	} midi_ret_raw_data;
	struct {
		msg_header_t Head;
		msg_type_t midi_cooked_dataType;
		midi_cooked_data_t midi_cooked_data[500];
	} midi_ret_cooked_data;
	struct {
		msg_header_t Head;
		msg_type_t quantaType;
		u_int quanta;
		msg_type_t midi_packed_dataType;
		midi_packed_data_t midi_packed_data[4000];
	} midi_ret_packed_data;
	struct {
		msg_header_t Head;
		msg_type_t queue_sizeType;
		u_int queue_size;
	} midi_queue_notify;
};
#define MIDI_REPLY_INMSG_SIZE sizeof(union midi_reply_request)

union midi_reply_reply {
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} midi_ret_raw_data;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} midi_ret_cooked_data;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} midi_ret_packed_data;
};
#define MIDI_REPLY_OUTMSG_SIZE sizeof(union midi_reply_reply)

/*
 * Handler routine to call when receiving messages from midi driver.
 */
kern_return_t midi_reply_handler (
	msg_header_t *msg,
	midi_reply_t *midi_reply);

#endif	_MIDI_REPLY_HANDLER_

