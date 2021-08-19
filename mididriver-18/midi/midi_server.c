/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 18-Jun-90	Gregg Kellogg (gk) at NeXT
 *	Don't use kern_server instance var directly.
 *
 * 06-Feb-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

/*
 * Kernel MIDI server.
 *
 * This implements the server interface to MIDI protocol on the 2 serial ports
 *
 * Each device is represented by two different ports
 *	Device port - Establish device ownership, Control device parameters,
 *		Request input from MIDI in, obtain MIDI out port.
 *
 *	MIDI out port - Messages containing MIDI data (raw bytes or MIDI
 *		messages as specified by parameter) are sent to this
 *		device and sent to the output device.
 */
#import "midi_var.h"
#import "midi_server_server.h"

midi_var_t midi_var = {};	// force it to be in data segment

#if	DEBUG
extern boolean_t midi_output_debug = TRUE;
extern boolean_t midi_input_debug = TRUE;
extern boolean_t midi_server_debug = TRUE;
extern boolean_t midi_timer_debug = TRUE;
extern boolean_t midi_stack_debug = FALSE;
#endif	DEBUG

#if	MIDI_MCOUNT | DEBUG
/* mcount and stack logging stuff */
#define PC_RING_SIZE	100

#import <next/pmap.h>

extern void (*mc_func)();
extern vm_offset_t pmap_resident_extract(pmap_t pmap, vm_offset_t va);

void midi_mcount();
void midi_dopanic(char *str, void *p1);
void midi_trigger();

int midi_doing_mcount=0;
int pc_ring[PC_RING_SIZE];
int *pc_ring_cur = pc_ring;
int *pc_ring_end = pc_ring + PC_RING_SIZE;
int ring_dump_size=16;
#endif	MIDI_MCOUNT | DEBUG

/*
 * Tell the world we're here.
 */
void midi_announce(void)
{
	extern char VERS_NUM[];
	int i;
	
	printf("MIDI driver version %s%s loaded\n", VERS_NUM,
#ifdef	DEBUG
		"(DEBUG)"
#else	DEBUG
		""
#endif	DEBUG
		);
#ifdef	DEBUG
	printf("\tsp = 0x%x\n", getsp());
#endif	DEBUG
	midi_var.slockp = simple_lock_alloc();
#ifdef	MIDI_MCOUNT
#ifdef	notdef
	mc_func = midi_mcount;		/* use this for mcount with kernel
					 * profiling */
#endif	notdef
#endif	MIDI_MCOUNT
#if	DEBUG | MIDI_MCOUNT
	for(i=0; i<PC_RING_SIZE; i++)
		pc_ring[i] = 0;
#if	MIDI_MCOUNT
	printf("midi_trigger & 0x%x\n", 
	    pmap_resident_extract(pmap_kernel(), (vm_offset_t)&midi_trigger));
#endif	MIDI_MCOUNT
#endif	DEBUG | MIDI_MCOUNT
}

void midi_signoff(void)
{
	simple_lock_free(midi_var.slockp);
	printf("MIDI driver unloaded\n");
}

/*
 * Device port messages.
 */
/*
 * Establish ownership.
 */
kern_return_t midi_set_owner (
	midi_dev_t	midi_dev,
	port_t		owner_port,
	port_t		*negotiation_port)
{
	if (!midi_dev || !owner_port)
		return MIDI_BAD_PORT;

	midi_slog("midi_set_owner[%d] owner %d neg %d\n",
		midi_dev - midi_var.dev, owner_port, *negotiation_port, 4, 5);
	midi_stack_log("midi_set_owner");
	if (midi_dev->owner == owner_port) {
		midi_slog("midi_set_owner[%d] owner_port == owner, reset\n",
			midi_dev - midi_var.dev, 2, 3, 4, 5);
		midi_free_unit(midi_dev - midi_var.dev);	// reset MIDI
		midi_dev->negotiation = *negotiation_port;
	} else if (midi_dev->owner != PORT_NULL) {
		/*
		 * Return the owner of the port in the error message.
		 */
		*negotiation_port = midi_dev->negotiation;
		midi_slog("midi_set_owner[%d] owner_port != owner, ret %d\n",
			midi_dev - midi_var.dev, midi_dev->negotiation,
			3, 4, 5);
		return MIDI_PORT_BUSY;
	}

	/*
	 * Make sure we can actually get the device.
	 */
	if (!midi_device_init(midi_dev - midi_var.dev)) {
		midi_slog("midi_set_owner[%d] dev_init failed, free unit\n",
			midi_dev - midi_var.dev, 2, 3, 4, 5);
		midi_free_unit(midi_dev - midi_var.dev);

		/*
		 * Return the owner of the port in the error message.
		 */
		midi_dev->owner = midi_dev->negotiation =
		*negotiation_port = PORT_NULL;
		return MIDI_PORT_BUSY;
	}

	midi_timer_init(midi_dev - midi_var.dev);

	/*
	 * Initialize time.
	 */
	midi_slog("midi_set_owner[%d] get current time\n",
		midi_dev - midi_var.dev, 2, 3, 4, 5);

	midi_dev->dir[MIDI_DIR_XMIT].timer_pend = TRUE;
	midi_timer_quanta_req(midi_dev->dir[MIDI_DIR_XMIT].in_timer,
		midi_dev->dir[MIDI_DIR_XMIT].in_timer_reply, 0, TRUE);
	ASSERT(curipl() == 0);

	midi_dev->dir[MIDI_DIR_RECV].timer_pend = TRUE;
	midi_timer_quanta_req(midi_dev->dir[MIDI_DIR_RECV].in_timer,
		midi_dev->dir[MIDI_DIR_RECV].in_timer_reply, 0, TRUE);
	ASSERT(curipl() == 0);

	midi_dev->owner = owner_port;
	midi_dev->negotiation = *negotiation_port;

	/*
	 * Enable the device.
	 */
	midi_io_enable(midi_dev - midi_var.dev);

	midi_slog("midi_set_owner[%d] done\n", midi_dev - midi_var.dev,
		2, 3, 4, 5);
	return KERN_SUCCESS;
}

/*
 * Get the xmit port for this device.
 */
kern_return_t midi_get_xmit (
	midi_dev_t	midi_dev,
	port_t		owner_port,
	port_t		*xmit_port)
{
	midi_stack_log("midi_get_xmit");
	if (!midi_dev)
		return MIDI_BAD_PORT;

	if (owner_port != midi_dev->owner)
		return MIDI_NOT_OWNER;

	*xmit_port = midi_dev->dir[MIDI_DIR_XMIT].port;
	return KERN_SUCCESS;
}

/*
 * Get the receive port for this device.
 */
kern_return_t midi_get_recv (
	midi_dev_t	midi_dev,
	port_t		owner_port,
	port_t		*recv_port)
{
	midi_stack_log("midi_get_recv");
	if (!midi_dev)
		return MIDI_BAD_PORT;

	if (owner_port != midi_dev->owner)
		return MIDI_NOT_OWNER;

	*recv_port = midi_dev->dir[MIDI_DIR_RECV].port;
	return KERN_SUCCESS;
}

/*
 * Get the timer port provided by this device.
 */
kern_return_t midi_get_out_timer_port (
	midi_dev_t	midi_dev,
	port_t		*timer_port)
{
	midi_stack_log("midi_get_out_timer_port");
	if (!midi_dev)
		return MIDI_BAD_PORT;

	if (midi_dev->owner == PORT_NULL)
		return MIDI_NO_OWNER;

	*timer_port = midi_dev->timer.port;
	midi_slog("midi_get_out_timer_port[%d] return %d\n",
		midi_dev - midi_var.dev, midi_dev->timer.port, 3, 4, 5);
	return KERN_SUCCESS;
}

/*
 * Specify number of quanta per received MIDI clock.
 */
kern_return_t midi_set_quanta_per_clock (
	midi_dev_t	midi_dev,
	port_t		owner_port,
	int		quanta_per_clock)
{
	midi_stack_log("midi_set_quanta_per_clock");
	if (!midi_dev)
		return MIDI_BAD_PORT;

	if (owner_port != midi_dev->owner)
		return MIDI_NOT_OWNER;

	midi_dev->timer.q_per_clock = quanta_per_clock;
	midi_slog("midi_set_quanta_per_clock[%d] to %d\n",
		midi_dev - midi_var.dev, quanta_per_clock, 3, 4, 5);
	return KERN_SUCCESS;
}

/*
 * Return number of quanta per received MIDI clock.
 */
kern_return_t midi_get_quanta_per_clock (
	midi_dev_t	midi_dev,
	port_t		owner_port,
	int		*quanta_per_clock)
{
	midi_stack_log("midi_get_quanta_per_clock");
	if (!midi_dev)
		return MIDI_BAD_PORT;

	if (owner_port != midi_dev->owner)
		return MIDI_NOT_OWNER;

	*quanta_per_clock = midi_dev->timer.q_per_clock;
	midi_slog("midi_get_quanta_per_clock[%d] return %d\n",
		midi_dev - midi_var.dev, midi_dev->timer.q_per_clock,
		3, 4, 5);
	return KERN_SUCCESS;
}

/*
 * Output queue manipulation
 * These messages are sent on the device port, rather than the transmit
 * port, because the server may stop listening to the transmit port
 * when it's received enough messages.
 */
/*
 * Returns the number of elements in each of the enqueued
 * output buffers.
 */
kern_return_t midi_output_queue_size (
	midi_dev_t	midi_dev,
	u_int		*queue_size,	// number of messages in queue
	u_int		*queue_max)	// max # messages q can hold
{
	u_int unit;

	midi_stack_log("midi_output_queue_size");
	if (!midi_dev)
		return MIDI_BAD_PORT;

	unit = midi_dev - midi_var.dev;

	midi_slog("midi_output_queue_size[%d] size %d, max %d\n",
		unit, midi_dev->queue_size, midi_dev->queue_max, 4, 5);

	*queue_size =  midi_dev->queue_size;
	*queue_max =  midi_dev->queue_max;

	return KERN_SUCCESS;
}

/*
 * Specify port to call when the message count in the output queue
 * is less than or equal to queue_size.  The port is set to
 * PORT_NULL after the message is sent.
 */
kern_return_t midi_output_queue_notify (
	midi_dev_t	midi_dev,
	port_t		owner_port,
	port_t		reply_port,
	u_int		queue_size)
{
	u_int unit;
	int s;

	midi_stack_log("midi_output_queue_notify");
	if (!midi_dev)
		return MIDI_BAD_PORT;

	unit = midi_dev - midi_var.dev;

	if (owner_port != midi_dev->owner)
		return MIDI_NOT_OWNER;

	midi_slog("midi_output_queue_notify[%d] port %d, size %d, cur %d\n",
		unit, reply_port, queue_size,  midi_dev->queue_size, 5);

	s = splmidi();
	simple_lock(midi_var.slockp);
	if ( midi_dev->queue_size > queue_size) {
		midi_dev->dir[MIDI_DIR_XMIT].reply_port = reply_port;
		midi_dev->queue_req_size = queue_size;
	} else
		midi_queue_notify(reply_port, midi_dev->queue_size);

	simple_unlock(midi_var.slockp);
	splx(s);

	return KERN_SUCCESS;
}


/*
 * Message on the device transmit port.
 */
/*
 * Send MIDI data.
 * If dont_block is TRUE the call will return an error when the queue's full.
 * otherwise, it just stops listening to it's input queue until the number
 * of enqueued messages is less than the maximum queue size.
 */
kern_return_t midi_send_raw_data (
	midi_rx_t	xmit,
	midi_raw_t	midi_raw_data,
	u_int		count,
	boolean_t	dont_block)
{
	int unit;
	midi_data_t midi_data;
	struct midi_dev *midi_dev;

	midi_stack_log("midi_send_raw_data");
	if (xmit == &midi_var.dev[0].dir[MIDI_DIR_XMIT])
		unit = 0;
	else if (xmit == &midi_var.dev[1].dir[MIDI_DIR_XMIT])
		unit = 1;
	else
		return MIDI_BAD_PORT;

	midi_slog("midi_send_raw_data[%d] count %d\n", unit, count, 3, 4, 5);

	if (count == 0)
		return KERN_SUCCESS;
	midi_dev = &midi_var.dev[unit];
	midi_data.raw = midi_raw_data;
	midi_device_enqueue_msg(midi_data, count, MIDI_TYPE_RAW, unit);

	if (   dont_block
	    && midi_dev->queue_size >= midi_dev->queue_max)
	{
		midi_slog("midi_send_raw_data: don't listen to port %d\n",
			midi_dev->dir[MIDI_DIR_XMIT].port,
			2, 3, 4, 5);
		port_set_remove(
			(task_t)task_self(),
			midi_dev->dir[MIDI_DIR_XMIT].port);
		midi_dev->port_disabled = 1;
		return MIDI_WILL_BLOCK;
	}

	return KERN_SUCCESS;
}

kern_return_t midi_send_cooked_data (
	midi_rx_t	xmit,
	midi_cooked_t	midi_cooked_data,
	u_int		count,
	boolean_t	dont_block)
{
	int unit;
	midi_data_t midi_data;
	struct midi_dev *midi_dev;

	midi_stack_log("midi_send_cooked_data");
	if (xmit == &midi_var.dev[0].dir[MIDI_DIR_XMIT])
		unit = 0;
	else if (xmit == &midi_var.dev[1].dir[MIDI_DIR_XMIT])
		unit = 1;
	else
		return MIDI_BAD_PORT;

	midi_slog("midi_send_cooked_data[%d] count %d\n",
		unit, count, 3, 4, 5);

	if (count == 0)
		return KERN_SUCCESS;

	midi_dev = &midi_var.dev[unit];
	midi_data.cooked = midi_cooked_data;
	midi_slog("midi_send_cooked_data: 1st (0x%x:0x%x:0x%x)@%d\n",
		midi_cooked_data[0].data[0], midi_cooked_data[0].data[1],
		midi_cooked_data[0].data[2], midi_cooked_data[0].quanta, 5);
	midi_device_enqueue_msg(midi_data, count, MIDI_TYPE_COOKED, unit);

	if (   dont_block
	    && midi_dev->queue_size >= midi_dev->queue_max)
	{
		midi_slog("midi_send_raw_data: don't listen to port %d\n",
			midi_dev->dir[MIDI_DIR_XMIT].port,
			2, 3, 4, 5);
		port_set_remove(
			(task_t)task_self(),
			midi_dev->dir[MIDI_DIR_XMIT].port);
		midi_dev->port_disabled = 1;
		return MIDI_WILL_BLOCK;
	}

	return KERN_SUCCESS;
}

kern_return_t midi_send_packed_data (
	midi_rx_t	xmit,
	u_int		quanta,
	midi_packed_t	midi_packed_data,
	u_int		count,
	boolean_t	dont_block)
{
	int unit;
	midi_data_t midi_data;
	struct midi_dev *midi_dev;
	
	midi_stack_log("midi_send_packed_data");
	if (xmit == &midi_var.dev[0].dir[MIDI_DIR_XMIT])
		unit = 0;
	else if (xmit == &midi_var.dev[1].dir[MIDI_DIR_XMIT])
		unit = 1;
	else
		return MIDI_BAD_PORT;

	midi_slog("midi_send_packed_data[%d] count %d\n",
		unit, count, 3, 4, 5);
	if (count == 0)
		return KERN_SUCCESS;

	midi_dev = &midi_var.dev[unit];
	midi_data.packed = midi_packed_data;
	midi_device_enqueue_msg(midi_data, count,
		MIDI_TYPE_PACKED|(quanta&MIDI_TIMESTAMP_MASK), unit);

	if (   dont_block
	    && midi_dev->queue_size >= midi_dev->queue_max)
	{
		midi_slog("midi_send_raw_data: don't listen to port %d\n",
			midi_dev->dir[MIDI_DIR_XMIT].port,
			2, 3, 4, 5);
		port_set_remove(
			(task_t)task_self(),
			midi_dev->dir[MIDI_DIR_XMIT].port);
		midi_dev->port_disabled = 1;
		return MIDI_WILL_BLOCK;
	}

	return KERN_SUCCESS;
}

/*
 * Message on the device receive port.
 */
/*
 * Request that data be sent back to the reply_port when it is available.
 */
kern_return_t midi_get_data (
	midi_rx_t	receive,
	port_t		reply_port)
{
	int unit, s;
	midi_dev_t midi_dev;
	typedef void (*void_fun_t)(void *);

	midi_stack_log("midi_get_data");
	if (receive == &midi_var.dev[0].dir[MIDI_DIR_RECV])
		unit = 0;
	else if (receive == &midi_var.dev[1].dir[MIDI_DIR_RECV])
		unit = 1;
	else
		return MIDI_BAD_PORT;
	midi_dev = &midi_var.dev[unit];
	
	midi_dev->dir[MIDI_DIR_RECV].reply_port = reply_port;
	if(midi_dev->pinput_pend) {
		/* there's data ready to go. Take it. */
		midi_slog("midi_get_data[%d] send\n", unit, 2, 3, 4, 5);
		midi_device_send_parsed_data(midi_dev);
	} else {
		if ((midi_var.dev[unit].msgin != midi_var.dev[unit].msgout) ||
		    (midi_dev->parsed_input.nelts)) {
			midi_slog("midi_get_data[%d] parse\n", 
				unit, 2, 3, 4, 5);
			/*
			 * the data in msgbuf/parsed_input may or may not be
			 * ready (it might not be a full frame). This will 
			 * take care of parsing and framing.
			 */
			receive->timer_pend = TRUE;
			midi_timer_quanta_req(receive->in_timer,
					      receive->in_timer_reply, 
					      0, TRUE);
		}
	}
	
	return KERN_SUCCESS;
}

/*
 * Specify (or return) the set of MIDI system messages that the device will
 * ignore (not forward to the user).  Note that ignoring MIDI Clock or MIDI
 * Time Code still allows the messages to be used by the driver for deriving
 * a time base.
 */
kern_return_t midi_set_sys_ignores (
	midi_rx_t	receive,
	u_int		sys_ignores)
{
	int unit;

	midi_stack_log("midi_set_sys_ignores");
	if (receive == &midi_var.dev[0].dir[MIDI_DIR_RECV])
		unit = 0;
	else if (receive == &midi_var.dev[1].dir[MIDI_DIR_RECV])
		unit = 1;
	else
		return MIDI_BAD_PORT;

	midi_slog("midi_set_sys_ignores[%d] to 0x%x\n",
		unit, sys_ignores, 3, 4, 5);
	midi_device_set_sys_ignores(sys_ignores, unit);

	return KERN_SUCCESS;
}

kern_return_t midi_get_sys_ignores (
	midi_rx_t	receive,
	u_int		*sys_ignores)
{
	int unit;

	midi_stack_log("midi_get_sys_ignores");
	if (receive == &midi_var.dev[0].dir[MIDI_DIR_RECV])
		unit = 0;
	else if (receive == &midi_var.dev[1].dir[MIDI_DIR_RECV])
		unit = 1;
	else
		return MIDI_BAD_PORT;

	*sys_ignores = midi_var.dev[unit].system_ignores;
	midi_slog("midi_get_sys_ignores[%d] return 0x%x\n",
		unit, midi_var.dev[unit].system_ignores, 3, 4, 5);
	return KERN_SUCCESS;
}

/*
 * Message on either the device transmit or receive ports.
 */
/*
 * Specify (or return) the timer port to use for timing input (output).
 * Defaults to devices system MIDI port.
 */
kern_return_t midi_set_in_timer_port (
	midi_rx_t	rx,
	port_t		timer_port)
{
	int unit, dir;

	midi_stack_log("midi_set_in_timer_port");
	if (rx == &midi_var.dev[0].dir[MIDI_DIR_RECV]) {
		unit = 0; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_RECV]) {
		unit = 1; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[0].dir[MIDI_DIR_XMIT]) {
		unit = 0; dir = MIDI_DIR_XMIT;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_XMIT]) {
		unit = 1; dir = MIDI_DIR_XMIT;
	} else
		return MIDI_BAD_PORT;

	/*
	 * Specify the timer to use for this device/direction.
	 */
	midi_var.dev[unit].dir[dir].in_timer = timer_port;

	return KERN_SUCCESS;
}

kern_return_t midi_get_in_timer_port (
	midi_rx_t	rx,
	port_t		*timer_port)
{
	int unit, dir;

	midi_stack_log("midi_get_in_timer_port");
	if (rx == &midi_var.dev[0].dir[MIDI_DIR_RECV]) {
		unit = 0; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_RECV]) {
		unit = 1; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[0].dir[MIDI_DIR_XMIT]) {
		unit = 0; dir = MIDI_DIR_XMIT;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_XMIT]) {
		unit = 1; dir = MIDI_DIR_XMIT;
	} else
		return MIDI_BAD_PORT;

	*timer_port = midi_var.dev[unit].dir[dir].in_timer;

	return KERN_SUCCESS;
}

/*
 * Specify (or return) device protocol information.
 *	Raw, Cooked, or Packed midi data.
 *	Timestamps relative or absolute
 *	Device clock synced to input MIDI Clocks (receive port)
 *	Generate MIDI Clock (transmit port)
 *	Device clock synced to input MIDI Time Code (receive port only)
 *	Generate MIDI Time Code (transmit port)
 */
kern_return_t midi_set_proto (			// set protocol
	midi_rx_t	rx,			// MIDI recv or xmit port
	u_int		data_format,		// raw, cooked, or packed
	boolean_t	time_relative,		// relative timestamps
	u_int		clock_source,		// system, MIDI clock, or MTC
	u_int		inter_msg_quanta,	// number of quanta to wait
						// for the next message to
						// arrive (receive only)
	u_int		msg_frame_quanta,	// maximum number of quanta
						// to wait from the first
						// queued message before
						// sending (receive only)
	u_int		queue_max)		// number of messages
						// (in all enqueued buffers)
						// that can be enqueued without
						// causing send to block
{
	u_int unit, dir;
	int proto;
	enum midi_timer_mode timer_m;

	midi_stack_log("midi_set_proto");
	if (rx == &midi_var.dev[0].dir[MIDI_DIR_RECV]) {
		unit = 0; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_RECV]) {
		unit = 1; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[0].dir[MIDI_DIR_XMIT]) {
		unit = 0; dir = MIDI_DIR_XMIT;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_XMIT]) {
		unit = 1; dir = MIDI_DIR_XMIT;
	} else
		return MIDI_BAD_PORT;

	

	switch (clock_source) {
	case MIDI_PROTO_SYNC_CLOCK:
		return MIDI_UNSUP;
	case MIDI_PROTO_SYNC_MTC:
		return MIDI_UNSUP;
	default:
		timer_m = mt_system;
		break;
	}

	midi_device_set_proto(data_format, time_relative,
			      inter_msg_quanta, msg_frame_quanta,
			      queue_max, unit, dir);

	midi_timer_set_proto(unit, timer_m);

	return KERN_SUCCESS;
}

kern_return_t midi_get_proto (			// get protocol
	midi_rx_t	rx,			// MIDI recv or xmit port
	u_int		*data_format,		// raw, cooked, or packed.
	boolean_t	*time_relative,		// relative timestamps
	u_int		*clock_source,		// system, MIDI clock, or MTC
	u_int		*inter_msg_quanta,	// number of quanta to wait
						// for the next message to
						// arrive (receive only)
	u_int		*msg_frame_quanta,	// maximum number of quanta
						// to wait from the first
						// queued message before
						// sending (receive only)
	u_int		*queue_max)		// number of messages
						// (in all enqueued buffers)
						// that can be enqueued without
						// causing send to block
{
	int unit, dir;

	midi_stack_log("midi_get_proto");
	if (rx == &midi_var.dev[0].dir[MIDI_DIR_RECV]) {
		unit = 0; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_RECV]) {
		unit = 1; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[0].dir[MIDI_DIR_XMIT]) {
		unit = 0; dir = MIDI_DIR_XMIT;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_XMIT]) {
		unit = 1; dir = MIDI_DIR_XMIT;
	} else
		return MIDI_BAD_PORT;

	*data_format = midi_var.dev[unit].rcv_mode;
	*time_relative = midi_var.dev[unit].dir[dir].relative;
	*inter_msg_quanta = midi_var.dev[unit].inter_msg_q;
	*msg_frame_quanta = midi_var.dev[unit].msg_frame_q;
	*queue_max = midi_var.dev[unit].queue_max;

	switch (midi_var.dev[unit].timer.mode) {
	case mt_clock:
		*clock_source = MIDI_PROTO_SYNC_CLOCK;
		break;
	case mt_MTC:
		*clock_source = MIDI_PROTO_SYNC_MTC;
		break;
	default:
		*clock_source = MIDI_PROTO_SYNC_SYS;
		break;
	}

	return KERN_SUCCESS;
}

/*
 * Clear the input or output queues.
 */
kern_return_t midi_clear_queue(midi_rx_t rx)
{
	int unit, dir;

	midi_stack_log("midi_clear_queue");
	if (rx == &midi_var.dev[0].dir[MIDI_DIR_RECV]) {
		unit = 0; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_RECV]) {
		unit = 1; dir = MIDI_DIR_RECV;
	} else if (rx == &midi_var.dev[0].dir[MIDI_DIR_XMIT]) {
		unit = 0; dir = MIDI_DIR_XMIT;
	} else if (rx == &midi_var.dev[1].dir[MIDI_DIR_XMIT]) {
		unit = 1; dir = MIDI_DIR_XMIT;
	} else
		return MIDI_BAD_PORT;

	if (dir == MIDI_DIR_XMIT)
		midi_device_clear_output(unit);
	else
		midi_device_clear_input(unit);

	return KERN_SUCCESS;
}

/*
 * Got notification that port we have send rights on has gone away, clean up.
 */
boolean_t midi_port_gone(port_name_t port)
{
	register int i;

	midi_slog("midi_port_gone: port %d\n", port, 2, 3, 4, 5);
	midi_stack_log("midi_port_gone");
	for (i = NMIDI-1; i >= 0; i--) {
		if (port == midi_var.dev[i].owner) {
			midi_free_unit(i);
			return TRUE;			
		}
		/*
		 * FIXME: handle in_timer going away?
		 */
	}
	return FALSE;
}

/*
 * Reset and free the specified unit
 */
void midi_free_unit(u_int unit)
{
	midi_slog("midi_free_unit[%d] reset dev & timer\n", unit, 2, 3, 4, 5);
	midi_stack_log("midi_free_unit");
	midi_device_reset(unit);			// reset MIDI
	midi_timer_reset(unit);				// reset timer

	midi_var.dev[unit].owner = PORT_NULL;
}

#ifdef	DEBUG
#define INT_STACK_TOP	0x04100000
static int min_interrupt_sp=INT_STACK_TOP;
static int min_thread_sp=0x70000000;
static int interrupt_sp_thresh=0x4000000;	/* tweakable */
static int interrupt_sp_panic=0x4000080;
static int stack_msg=0;
#define THREAD_STACK_RED_ZONE	(420 + 118 + 100)	/* pcb + uthread */

int getsp() {
	asm("movl sp, d0");
}

void midi_stack_log(char *msg)
{
	int cur_sp = getsp();
	
	if(cur_sp < interrupt_sp_panic) {
		panic("mididriver interrupt stack overflow");
	}
	/*
	 * Check for 4k stack 
	 */
	if(cur_sp > INT_STACK_TOP) {
		if((cur_sp & 0xfff) < THREAD_STACK_RED_ZONE)
			panic("mididriver thread stack overflow");
		if(cur_sp < min_thread_sp) {
			if(stack_msg)
			    printf("midi_stack_log: min thread sp 0x%x @ %s\n",
				cur_sp, msg);
			min_thread_sp = cur_sp;
		}
	}
	else {
		if(cur_sp < min_interrupt_sp) {
			if(stack_msg)
			    printf("midi_stack_log: min interrupt sp 0x%x "
			    	"@ %s\n", cur_sp, msg);
			min_interrupt_sp = cur_sp;
			if((cur_sp < interrupt_sp_thresh) && stack_msg){
				printf("minimum interrupt sp reached\n");
			}
		}
	}
	midi_stklog("sp=0x%x @ %s\n", cur_sp, msg, 3,4,5);
}

#endif	DEBUG

#ifdef	MIDI_MCOUNT

#ifdef	panic
#undef	panic
#endif	panic

void midi_zilch() {}

void midi_foobar()
{
	/* ensure no fetch of midi_trigger */
	printf("\n");
	printf("      ");
}

void midi_trigger()
{
	printf("midi_trigger\n");
}

void midi_dopanic(char *str, void *p1)
{
	int i;
	int *pcp=pc_ring_cur;	/* points to log location of NEXT pc */
	
	/*
	 * Log last n pc's
	 */
	for(i=0; i<ring_dump_size; i++) {
		if(pcp == pc_ring)
			pcp = &pc_ring[PC_RING_SIZE - 1];
		else
			pcp--;
		printf("mcount called from 0x%x\n", *pcp);
	}
	if((*(int *)p1 > 0x0f000000) && (*(int *)p1 < 0x0fff0000)) {
	    printf("BOGUS POINTER: va 0x%x physical adrs 0x%x data 0x%x\n",
		p1, pmap_resident_extract(pmap_kernel(), (vm_offset_t)p1),
		*(int *)p1);
	}
	else
		printf("midi_dopanic - p1 = 0x%x\n", p1);
	panic(str);
}

#import <next/cframe.h>

void *getppc (void)
{
	register struct frame *fp, *parentfp, *grandpfp;

	asm("	.text");
	asm("	movl a6,%0" : "=a" (fp));
	
	parentfp = fp->f_fp;		/* mcount's frame pointer */
	grandpfp = parentfp->f_fp;	/* mcount caller's fp */
	return (grandpfp->f_pc);	/* mcount caller's pc */
}

void midi_mcount_checkq(queue_head_t *qh) 
{
	enqueued_output_t *ob, *ob_next, *ob_prev;
	
	ob = (enqueued_output_t *)(qh->next);
	/*
	 * first check integrity of queue head 
	 */
	if(qh->next != qh->prev) {
		if(ob->link.prev != qh) {
			midi_trigger();
			printf("ob 0x%x ob->link.prev 0x%x qh 0x%x\n",
				ob, ob->link.prev, qh);
			midi_dopanic("midi_mcount - output_q bad "
				"(1)\n", &ob->link.prev);
		} 
		ob_prev = (enqueued_output_t *)(qh->prev);
		if(ob_prev->link.next != qh) {
			midi_trigger();
			printf("ob_prev 0x%x ob_prev->link.next 0x%x" 
				"qh 0x%x\n", ob, ob_prev->link.next,
				qh);
			midi_dopanic("midi_mcount - output_q bad "
				"(2)\n", &ob_prev->link.next);
		} 
	}
	/*
	 * Ensure integrity of each link in the queue
	 */
	ob_next = (enqueued_output_t *)(ob->link.next);
	while(qh != (queue_t)ob_next){
		if(ob_next->link.prev != (queue_t)ob) {
			midi_trigger();
			printf("ob 0x%x ob_next 0x%x "
			"ob_next->link.prev 0x%x\n", 
			ob, ob_next, ob_next->link.prev);
			midi_dopanic("midi_mcount - output_q bad "
				"(3)\n", &ob_next->link.prev);
		} 
		ob = ob_next;
		ob_next = (enqueued_output_t *)(ob->link.next);

	}
}

int midi_mcount_caller() {
	/*
	 * Simulate stack frame from kernel's mcount...
	 */
	midi_mcount();
}

/*
 * mcount will call this...
 */
void midi_mcount()
{
	/*
	 * search thru all of the enqueued_output_t's on both midi_dev's 
	 * output_q's.
	 */
	midi_dev_t midi_dev = &midi_var.dev[0];
	queue_head_t *qh;
	enqueued_output_t *ob, *ob_next, *ob_prev;
	int i;
	int s;
	
	if(midi_doing_mcount)
		return;				/* avoid recursion */
	s = splmidi();
	midi_doing_mcount = 1;
	/*
	 * Log calling pc
	 */
	*pc_ring_cur++ = (int)getppc();
	if(pc_ring_cur == pc_ring_end)
		pc_ring_cur = pc_ring;

	for(i=0; i<NMIDI; i++, midi_dev++) {
		if(midi_dev->owner == PORT_NULL)
			continue;		/* skip this one */
		/*
		 * Check for nelts == 0 at head of output_q
		 */
		ob = (enqueued_output_t *)midi_dev->output_q.next;
		if(ob != (enqueued_output_t *)(&midi_dev->output_q)) {
			if(ob->nelts == 0) {
				midi_dopanic("midi_mcount: nelts == 0\n", ob);
			}
		}
		midi_mcount_checkq(&midi_dev->output_q);
		midi_mcount_checkq(&midi_dev->output_fq);
	}
	midi_doing_mcount = 0;
	splx(s);
}

int midi_look_q(queue_t qh, enqueued_output_t *ob, int head_ok)
{
	enqueued_output_t *ob_work;
	int count=0;
	
	if(queue_empty(qh))
		return(0);
	ob_work = (enqueued_output_t *)qh->next;
	if(head_ok) {
		if(ob = (enqueued_output_t *)qh->next)
			ob_work = (enqueued_output_t *)ob_work->link.next;
	}
	while(qh != ob_work->link.next) {
		if((ob >= ob_work) &&
		   (ob < (enqueued_output_t *)ob_work->edata.raw)) {
		   	printf("Bogus ob (0x%x) in q 0x%x count %d\n",
				ob, qh, count);
			midi_dopanic("BOGUS OB", ob);
		}
		ob_work = (enqueued_output_t *)ob_work->link.next;
		count++;

	}
	
}
/*
 * Verify that ob is not with the address range of anything in 
 * midi_dev->output_q or output->fq. head_ok true means it's OK
 * if ob is at the head of output_q.
 */
int midi_check_outputq(midi_dev_t midi_dev, enqueued_output_t *ob,
	int head_ok)
{
	midi_look_q(&midi_dev->output_q, ob, head_ok);
	midi_look_q(&midi_dev->output_fq, ob, FALSE);
	return(0);
}


#endif	MIDI_MCOUNT