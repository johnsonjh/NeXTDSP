/* 
 * Mach Operating System
 * Copyright (c) 1988 NeXT, Inc.
 * All rights reserved.	 The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */
/*
 * HISTORY
 * 10-Dec-88  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#ifndef _SND_MSGS_USER_
#define _SND_MSGS_USER_
#include <nextdev/snd_msgs.h>

msg_header_t *_DSP_stream_msg (
	msg_header_t *msg,		// message pointer to reuse or malloc
	port_t	stream_port,		// valid stream port
	port_t	reply_port,		// task port or other
	int	data_tag);		// tag associated with request
msg_header_t *_DSP_stream_play_data (
	msg_header_t	*msg,		// message frame to add request to
	pointer_t	data,		// data to play
	boolean_t	started_msg,	// want's a message when started
	boolean_t	completed_msg,	// want's a message when completed
	boolean_t	aborted_msg,	// want's a message when aborted
	boolean_t	paused_msg,	// want's a message when paused
	boolean_t	resumed_msg,	// want's a message when resumed
	boolean_t	underflow_msg,	// want's a message on underflow
	boolean_t	preempt,	// play preemptively
	boolean_t	deallocate,	// deallocate data when sent?
	port_t		reg_port,	// port for region events
	int		nbytes);	// number of bytes of data to send
msg_header_t *_DSP_stream_record_data (
	msg_header_t	*msg,		// message frame to add request to
	boolean_t	started_msg,	// want's a message when started
	boolean_t	completed_msg,	// want's a message when completed
	boolean_t	aborted_msg,	// want's a message when aborted
	boolean_t	paused_msg,	// want's a message when paused
	boolean_t	resumed_msg,	// want's a message when resumed
	boolean_t	overflow_msg,	// want's a message on overflow
	int		nbytes,		// number of bytes of data to record
	port_t		reg_port,	// port for region events
	char		*filename);	// file for backing store (or null)
msg_header_t *_DSP_stream_control (
	msg_header_t	*msg,		// message frame to add request to
	int		control);	// await/abort/pause/resume
kern_return_t _DSP_stream_nsamples (
	port_t		stream_port,	// valid stream port
	int		*nsamples);	// OUT number of samples played/rec'd
kern_return_t _DSP_get_stream (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// valid soundout/in/dsp owner port
	port_t		*stream_port,	// returned stream_port
	u_int		stream);	// stream to/from what?
kern_return_t _DSP_set_dsp_owner_port (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// dsp owner port
	port_t		*neg_port);	// dsp negotiation port
kern_return_t _DSP_set_sndin_owner_port (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// sound in owner port
	port_t		*neg_port);	// sound in negotiation port
kern_return_t _DSP_set_sndout_owner_port (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// sound out owner port
	port_t		*neg_port);	// sound out negotiation port
kern_return_t _DSP_get_dsp_cmd_port (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// valid dsp owner port
	port_t		*cmd_port);	// returned cmd_port
kern_return_t _DSP_dsp_proto (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// valid dsp owner port
	int		proto);		// what protocol to use.	
kern_return_t _DSP_dspcmd_event (
	port_t		cmd_port,	// valid dsp command port
	u_int		mask,		// mask of flags to inspect
	u_int		flags,		// set of flags that must be on
	msg_header_t	*msg);		// message to send (simple only)
kern_return_t _DSP_dspcmd_chandata (
	port_t		cmd_port,	// valid dsp command port
	int		addr,		// .. of dsp buffer
	int		size,		// .. of dsp buffer
	int		skip,		// dma skip factor
	int		space,		// dsp space of buffer
	int		mode,		// mode of dma [1..5]
	int		chan);		// channel for dma
kern_return_t _DSP_dspcmd_dmaout (
	port_t		cmd_port,	// valid dsp command port
	int		addr,		// .. in dsp
	int		size,		// # dsp words to transfer
	int		skip,		// dma skip factor
	int		space,		// dsp space of buffer
	int		mode,		// mode of dma [1..5]
	pointer_t	data);		// data to output
kern_return_t _DSP_dspcmd_dmain (
	port_t		cmd_port,	// valid dsp command port
	int		addr,		// .. of dsp buffer
	int		size,		// .. of dsp buffer
	int		skip,		// dma skip factor
	int		space,		// dsp space of buffer
	int		mode,		// mode of dma [1..5]
	pointer_t	*data);		// where data is put
kern_return_t _DSP_dspcmd_abortdma (
	port_t		cmd_port,	// valid dsp command port
	int		*dma_state,	// returned dma state
	vm_address_t	*start,		// returned dma start address
	vm_address_t	*stop,		// returned dma stop address
	vm_address_t	*next);		// returned dma next address
kern_return_t _DSP_dspcmd_req_msg (
	port_t		cmd_port,	// valid dsp command port
	port_t		reply_port);	// where to recieve messages
kern_return_t _DSP_dspcmd_req_err (
	port_t		cmd_port,	// valid dsp command port
	port_t		reply_port);	// where to recieve messages
void _DSP_dspcmd_msg_data (
	snd_dsp_msg_t	*msg,		// message containing returned data
	int		**buf_addr,	// INOUT address of returned data
	int		*buf_size);	// INOUT # ints returned

msg_header_t *_DSP_dspcmd_msg_reset (
	msg_header_t *msg,		// Existing message header
	port_t	cmd_port,		// valid dsp command port
	port_t	reply_port,		// where to send reply message(s)
	int	priority,		// DSP_MSG_{LOW,MED,HIGH}
	int	atomic);		// message may not be preempted

msg_header_t *_DSP_dspcmd_msg (
	port_t	cmd_port,		// valid dsp command port
	port_t	reply_port,		// where to send reply message(s)
	int	priority,		// DSP_MSG_{LOW,MED,HIGH}
	int	atomic);		// message may not be preempted
msg_header_t *_DSP_dsprcv_msg (
	port_t	cmd_port,		// valid dsp command port
	port_t	reply_port);		// where to send reply message(s)

msg_header_t *_DSP_dsprcv_msg_reset (
	msg_header_t *msg,		// message frame to reset
	port_t cmd_port,		// valid dsp command port
	port_t reply_port);		// where to send reply message(s)

msg_header_t *_DSP_dspreply_msg (
	port_t	reply_port);		// where to send reply message

msg_header_t *_DSP_dspreply_msg_reset (
	msg_header_t *msg,		// Existing message header
	port_t	reply_port);		// where to send reply message

msg_header_t *_DSP_dsp_condition (
	msg_header_t	*msg,		// message frame to add request to
	u_int		mask,		// mask of flags to inspect
	u_int		flags);		// set of flags that must be on
msg_header_t *_DSP_dsp_data (
	msg_header_t	*msg,		// message frame to add request to
	pointer_t	data,		// data to play
	int		eltsize,	// 1, 2, or 4 byte data
	int		nelts);		// number of elements of data to send
msg_header_t *_DSP_dsp_host_command (
	msg_header_t	*msg,		// message frame to add request to
	u_int		host_command);	// host command to execute
msg_header_t *_DSP_dsp_host_flag (
	msg_header_t	*msg,		// message frame to add request to
	u_int		mask,		// mask of flags to inspect
	u_int		flags);		// set of flags that must be on
msg_header_t *_DSP_dsp_ret_msg (
	msg_header_t	*msg,		// message frame to add request to
	msg_header_t	*ret_msg);	// message to sent to reply port
msg_header_t *_DSP_dspreset (
	msg_header_t	*msg);		// message frame to add request to
msg_header_t *_DSP_dspregs (
	msg_header_t	*msg);		// message frame to add request to
msg_header_t *_DSP_stream_options (
	msg_header_t	*msg,		// message frame to add request to
	int		high_water,
	int		low_water,
	int		dma_size);
#endif _SND_MSGS_USER_

