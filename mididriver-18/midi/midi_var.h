/* 
 * Mach Operating System
 * Copyright (c) 1988 NeXT, Inc.
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 * HISTORY
 * 18-Jun-90	Gregg Kellogg (gk) at NeXT
 *	Don't use kern_server instance var directly.
 *
 * 13-Mar-90	dmitch
 *	Fixed d_midi_log macro - passes &midi_var.kern_server
 * 23-Oct-89	dmitch at NeXT
 *	Modified parsed_input; added some flags to midi_var
 * 06-Feb-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#ifndef _MIDI_VAR_
#define _MIDI_VAR_
/* #define MIDI_MCOUNT 1  	/* */

#define NMIDI 2

#import <kernserv/kern_server_types.h>
#import <nextdev/zscom.h>
#import <nextdev/zsreg.h>
#import "midi_timer_var.h"

/*
 * Priority to run softint stuff at.
 */
#define MIDI_SOFTPRI	1

#define MIDI_IPL	5
#define MIDI_SOFTIPL	2

#define splmidisoft() spl2()
#define splmidi() splscc()

#undef	simple_lock
#define simple_lock(l)
#undef	simple_unlock
#define simple_unlock(l)

/*
 * High-level device data structure
 */
typedef struct midi_var {
	kern_server_t		kern_server;	// generic instance info
	struct midi_dev {
		port_name_t	owner;		// port of midi device owner
		port_name_t	negotiation;	// to communicate with owner
		port_name_t	dev_port;	// my device port
		midi_timer_t	timer;		// timer info
		volatile
		struct zsdevice *addr;		// address of dev registers
		midi_cooked_data_t run_status;	// running status
		/* 
		 * msgbuf is a circular buffer; Rx interrupt handler stuffs
		 * data in *msgin and data is parsed at *msgout. msgbuf and
		 * emsgbuf (end of buffer) are fixed.
		 */
		midi_cooked_t 	msgbuf;		// interrupt level receive buf
		midi_cooked_t	emsgbuf;
		midi_cooked_t	msgin;
		midi_cooked_t	msgout;
#define	MIDI_RINP_BUFSIZE	1024		// size of raw input buffer in
						// midi_cooked_data_t's. This
						// size is determined 
						// empirically. 
		struct parsed_input {
		    u_int	nelts;		// # messages in buffer
		    u_int	type;		// msg type (for xmit)
		    u_int	first_quanta;	// in absolute time, for
		    u_int 	last_quanta;	//   framing.
		    midi_data_t	buf;		// actual data
		    midi_data_t ebuf;		// end of data buffer
#define	MIDI_PINP_BUFSIZE	256		// Size of parsed_input buffer
						// in midi_cooked_data_t's. 
						// This needs to be small
						// enough so that we won't
						// try to send too large of
						// a message back (inline).
						// 256 cooked elts = 2048 bytes
		} parsed_input;			// parsed messages (rcv side)
		queue_head_t	output_q;	// queue of bufs to output
		queue_head_t	output_fq;	// queue of free output bufs
		u_int		system_ignores;	// system msgs to not pass on
		u_int		inter_msg_q;	// wait how long for next msg
		u_int		msg_frame_q;	// wait no longer than for all
		u_int		queue_size;	// # messages in queue
		u_int		queue_req_size;	// awaited queue size
		u_int		queue_max;	// max # messages till q full
#define MIDI_DEF_QUEUE_MAX	(MIDI_PACKED_DATA_MAX*2)
		u_int		rcv_mode:2,	// raw, cooked, packed
				pushed_packed:1,// packed mode pushed on cooked
				sys_exclusive:1,// receiving sys exclusive data
		      		frame_time_req:1,
		    				// true if a timer req has been
						// made for the end of this
						// parsed_input 
				pinput_pend:1,	// parsed_input is full; 
						// awaiting transmission to 
						// caller
				callout_pend:1,	// Rx interrupt handler has
						// scheduled a callout to 
						// parse incoming data.
				dev_oflow:1,	// Rx device overflow
				buf_oflow:1,	// Rx buffer overflow
				q_notify_pend:1,
						// callout of 
						// midi_device_send_queue_req
						// pending
				q_listen_pend:1,
						// callout of 
						// midi_device_queue_listen
						// pending
				port_disabled:1;
						// port removed from portset
						// due to queue full
		struct midi_rx {
		    port_name_t	port;		// recv/xmit port
		    port_name_t	in_timer;	// timer used by device.
		    port_name_t	in_timer_reply;	// port used in timer req's
		    port_name_t	reply_port;	// for sending parsed MIDI rcv
		    				// for sending queue len xmit
		    u_int	relative:1,	// relative timestamps
				intren:1,	// interrupts enabled
				pause:1,	// interface paused
				timeout_pend:1,	// system timeout pending
				timer_pend:1;	// timer event pending
				
		    /* last_time and time_stamp are records of the last quanta
		     * and microsecond (from event counter) we knew about. We
		     * get last_time from the timer. Any time we need to
		     * interpolate the time in the driver, we use the 
		     * microseconds elapsed since timestamp and offset 
		     * last_time by the difference / micrseconds per quanta.
		     */
		    u_int	last_time;	// time of last operation
		    u_int	timestamp;	// system timestamp of last op
		    u_int	act_usec_per_q;	// for aproximating cur_time
		    u_int	last_quanta_io;	// quanta of last midi_data_t
		    				//   in or out (for relative
						//   timing)
		} dir[2];
#define MIDI_DIR_XMIT	0
#define MIDI_DIR_RECV	1
	} dev[NMIDI];
	simple_lock_t	slockp;			// locking
} midi_var_t;

typedef struct enqueued_output {
	queue_chain_t	link;		// struct link structure
	u_int		nelts;		// # messages in buffer
	u_int		type;		// msg buffer type
	midi_data_t	data;		// pointer to actual data (follows)
	midi_data_t	edata;		// pointer to end of data following
} enqueued_output_t;

#if	KERNEL
extern midi_var_t midi_var;

#define panic(s) (  curipl() == 0 \
		  ? kern_serv_panic(kern_serv_bootstrap_port(&midi_var.kern_server), s) \
		  : printf("can't panic: %s\n", s))
#if	DEBUG
extern boolean_t midi_output_debug;
extern boolean_t midi_input_debug;
extern boolean_t midi_server_debug;
extern boolean_t midi_timer_debug;
extern boolean_t midi_stack_debug;
extern int getsp();
extern void midi_stack_log(char *msg);
#define d_midi_log(l) \
	static inline void (midi_log ## l)( \
		char *msg, int arg1, int arg2, \
	    	int arg3, int arg4, int arg5) \
	{ \
		XPR(XPR_MIDI, (msg, arg1, arg2, arg3, arg4, arg5)); \
		kern_serv_log(&midi_var.kern_server, l, msg, arg1, \
			arg2, arg3, arg4, arg5); \
	}
d_midi_log(1)
d_midi_log(2)
d_midi_log(4)
d_midi_log(8)
#define midi_olog(m, a1, a2, a3, a4, a5) \
	{if (midi_output_debug)	\
		midi_log1(m, (int)(a1), (int)(a2), (int)(a3), \
			(int)(a4), (int)(a5));}
#define midi_ilog(m, a1, a2, a3, a4, a5) \
	{if (midi_input_debug)	\
		midi_log2(m, (int)(a1), (int)(a2), (int)(a3), \
			(int)(a4), (int)(a5));}
#define midi_slog(m, a1, a2, a3, a4, a5) \
	{if (midi_server_debug)	\
		midi_log4(m, (int)(a1), (int)(a2), (int)(a3), \
			(int)(a4), (int)(a5));}
#define midi_tlog(m, a1, a2, a3, a4, a5) \
	{if (midi_timer_debug)	\
		midi_log8(m, (int)(a1), (int)(a2), (int)(a3), \
			(int)(a4), (int)(a5));}
#define midi_stklog(m, a1, a2, a3, a4, a5) \
	{if (midi_stack_debug)	\
		midi_log8(m, (int)(a1), (int)(a2), (int)(a3), \
			(int)(a4), (int)(a5));}
#else	DEBUG
#define midi_olog(m, a1, a2, a3, a4, a5)
#define midi_ilog(m, a1, a2, a3, a4, a5)
#define midi_slog(m, a1, a2, a3, a4, a5)
#define midi_tlog(m, a1, a2, a3, a4, a5)
#define midi_stklog(m, a1, a2, a3, a4, a5)
#ifdef	MIDI_MCOUNT
#define midi_stack_log(msg) midi_mcount_caller(); 
#else	MIDI_MCOUNT
#define midi_stack_log(msg)
#endif	MIDI_MCOUNT
#endif	DEBUG

/*
 * circular buffer increment
 */
#define MSGBUF_INCR(d, p) (((((d)->p)+1) == (d)->emsgbuf) ? \
	(d)->msgbuf : (((d)->p)+1))

#endif	KERNEL

typedef struct midi_rx *midi_rx_t;
typedef struct midi_dev *midi_dev_t;

static inline midi_dev_t convert_port_to_midi_dev(port_t port)
{
	if ((int)port < 0 || (int)port >= NMIDI)
		return 0;
	else
		return &midi_var.dev[(int)port];
}

static inline port_t convert_midi_dev_to_port(midi_dev_t midi_dev)
{
	return midi_dev->dev_port;
}

static inline midi_rx_t convert_port_to_midi_rx(port_t port)
{
	if ((int)port < 0 || port >= NMIDI*2)
		return 0;
	else
		return &midi_var.dev[(int)port>>1].dir[(int)port&1];
}

static inline port_t convert_midi_dev_to_rx(midi_rx_t midi_rx)
{
	return midi_rx->port;
}

static inline u_int midi_data_size(u_int type)
{
	switch (type&MIDI_TYPE_MASK) {
	case MIDI_TYPE_RAW:
		return sizeof(midi_raw_data_t);
	case MIDI_TYPE_COOKED:
		return sizeof(midi_cooked_data_t);
	case MIDI_TYPE_PACKED:
		return sizeof(midi_packed_data_t);
	}
}

static inline void midi_free_ob(enqueued_output_t *ob)
{
	u_int size = (char *)ob->edata.raw - (char *)ob;
	kfree(ob, size);
}

static inline enqueued_output_t *midi_alloc_ob(u_int nelts, u_int type)
{
	u_int dsize = midi_data_size(type)*(nelts);

	enqueued_output_t *ob = (enqueued_output_t *)kalloc(sizeof(*ob)+dsize);
	ob->nelts = nelts;
	ob->type = type;
	ob->data.packed = (midi_packed_data_t *)(ob+1);
	ob->edata.packed = ob->data.packed + dsize;
	return ob;
}

static inline u_int midi_device_curtime(u_int unit, u_int dir)
{
	midi_rx_t midi_rx = &midi_var.dev[unit].dir[dir];
	u_int delta;

	if (!midi_var.dev[unit].dir[dir].pause)
		delta =   event_delta(midi_rx->timestamp)
			/ midi_rx->act_usec_per_q;
	else
		delta = 0;
	return midi_rx->last_time + delta;
}

/*
 * Extract the time from the message field.
 */
static inline u_int midi_data_time(midi_data_t data, u_int type)
{
	switch (type&MIDI_TYPE_MASK) {
	case MIDI_TYPE_RAW:
		return data.raw->quanta;
	case MIDI_TYPE_COOKED:
		return data.cooked->quanta;
	case MIDI_TYPE_PACKED:
		return type&MIDI_TIMESTAMP_MASK;
	}
}

/*
 * baud rate for midi input and output
 */
#define	MIDI_BAUD	31250

/*
 * Routine prototypes
 */
#if	KERNEL
boolean_t midi_port_gone(port_name_t port);
void midi_free_unit(u_int unit);
boolean_t midi_device_init(u_int unit);
void midi_device_reset(u_int unit);
void midi_device_set_proto (
	u_int		rcv_mode,
	boolean_t	relative,
	u_int		inter_msg_q,
	u_int		msg_frame_q,
	u_int		queue_max,
	u_int		unit,
	u_int		dir);
void midi_device_enqueue_msg(
	midi_data_t mdata,
	u_int count,
	u_int type,
	u_int unit);
void midi_device_parse_rcvd_data(u_int unit);
void midi_device_send_parsed_data(midi_dev_t midi_dev);
void midi_device_set_sys_ignores(u_int sys_ignores, u_int unit);
void midi_device_recv_timer_event (
	u_int		quanta,
	u_int		real_usec_per_quantum,
	boolean_t	timer_expired,
	boolean_t	timer_stopped,
	boolean_t	timer_forward,
	u_int		unit);
void midi_device_xmit_timer_event (
	u_int		quanta,
	u_int		real_usec_per_quantum,
	boolean_t	timer_expired,
	boolean_t	timer_stopped,
	boolean_t	timer_forward,
	u_int		unit);
void midi_device_clear_output(u_int unit);
void midi_device_clear_input(u_int unit);
void midi_device_send_queue_req(midi_dev_t midi_dev);
void midi_device_queue_listen(midi_dev_t midi_dev);

boolean_t midi_io_init(u_int unit);
void midi_io_reset(u_int unit);
void midi_io_enable(u_int unit);
boolean_t midi_io_start_output(u_int unit);

#define midi_panic(s) kern_serv_panic(kern_serv_bootstrap_port(&midi_var.kern_server), s)

#ifdef	MIDI_MCOUNT
int midi_check_outputq(midi_dev_t midi_dev, enqueued_output_t *ob,
	int head_ok);
#endif	MIDI_MCOUNT

#endif	KERNEL
#endif	_MIDI_VAR_
