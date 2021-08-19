/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 18-Jun-90	Gregg Kellogg (gk) at NeXT
 *	Don't use kern_server instance var directly.
 *
 * 13-Mar-90	Doug Mitchell at NeXT
 *	Changed kern_serv_port_proc() calls to kern_serv_port_serv().
 *
 * 23-Aug-89  Gregg Kellogg (gk) at NeXT
 *	midi_device_clear_output was not setting the queue_size to zero
 *	when done.
 *
 * 04-Apr-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 *
 */ 

/*
 * Midi device driver for the Zilog 8530 SCC
 * High-level device specific code.
 */

#import "midi_var.h"
#import "midi_server_server.h"
#import <next/eventc.h>

extern midi_var_t midi_var;

/* Time conversion */
static u_int midi_time_rel_to_abs (
	midi_data_t	data,
	u_int		cur_time,
	u_int		count,
	u_int		*type);
static u_int midi_time_abs_to_rel (
	midi_data_t	data,
	u_int		cur_time,
	u_int		count,
	u_int		*type);

#define midi_ilog2	midi_ilog
#define midi_olog2	midi_ilog
//#define midi_ilog2(m, a1, a2, a3, a4, a5)
//#define midi_olog2(m, a1, a2, a3, a4, a5)
/* #define CONSOLE_DEBUG	1	/* */

/*
 * midi_device_init -- initialize midi device
 * initializes driver state, obtains buffers, programs SCC,
 * and establishes interrupt linkage.
 *
 * The device can be opened on any of two different minor numbers.
 * This allows one connection to be read only and the other write
 * only.
 */
boolean_t midi_device_init(u_int unit)
{
	struct zs_com *zcp;
	int s;
	kern_return_t r;
	midi_dev_t midi_dev = &midi_var.dev[unit];
	midi_rx_t rcv_rxp = &midi_dev->dir[MIDI_DIR_RECV];
	midi_rx_t xmit_rxp = &midi_dev->dir[MIDI_DIR_XMIT];
	
	midi_stack_log("midi_device_init");
	if (!midi_io_init(unit))
		return FALSE;

	/*
	 * Allocate midi transmit port.
	 */
	r = port_allocate((task_t)task_self(),
		&midi_dev->dir[MIDI_DIR_XMIT].port);
	if (r != KERN_SUCCESS)
		midi_panic("can't allocate xmit_port");
	r = kern_serv_port_serv(&midi_var.kern_server,
		midi_dev->dir[MIDI_DIR_XMIT].port,
		(port_map_proc_t)midi_server_server, 
		(unit<<1|MIDI_DIR_XMIT));
	if (r != KERN_SUCCESS)
		midi_panic("can't add xmit_port to set");
    
	/*
	 * Allocate midi receive port.
	 */
	r = port_allocate((task_t)task_self(),
		&midi_dev->dir[MIDI_DIR_RECV].port);
	if (r != KERN_SUCCESS)
		midi_panic("can't allocate recv_port");
	r = kern_serv_port_serv(&midi_var.kern_server,
		midi_dev->dir[MIDI_DIR_RECV].port,
		(port_map_proc_t)midi_server_server, 
		(unit<<1|MIDI_DIR_RECV));
	if (r != KERN_SUCCESS)
		midi_panic("can't add recv_port to set");

	queue_init(&midi_var.dev[unit].output_q);
	midi_var.dev[unit].queue_size = 0;
	queue_init(&midi_var.dev[unit].output_fq);

	/*
	 * Allocate receive buffers.
	 */
	if (!midi_dev->msgbuf) {
		midi_dev->msgbuf = (midi_cooked_data_t *)
			kalloc(  MIDI_RINP_BUFSIZE *
			         sizeof(midi_cooked_data_t));
		midi_dev->emsgbuf = midi_dev->msgbuf + MIDI_RINP_BUFSIZE;
	}
	midi_dev->msgin = midi_dev->msgout = midi_dev->msgbuf;

	if (!midi_dev->parsed_input.buf.cooked) {
		midi_dev->parsed_input.buf.cooked = (midi_cooked_data_t *)
			kalloc(  MIDI_PINP_BUFSIZE *
			         sizeof(midi_cooked_data_t));
		midi_dev->parsed_input.ebuf.cooked = 
			midi_dev->parsed_input.buf.cooked + MIDI_PINP_BUFSIZE;
		midi_dev->parsed_input.type = 0;
	}

	/*
	 * Clear flags.
	 */
	midi_dev->system_ignores = 0;
	midi_dev->rcv_mode = MIDI_PROTO_RAW;
	rcv_rxp->relative       = xmit_rxp->relative       = 0;
	rcv_rxp->intren         = xmit_rxp->intren         = 0;
	rcv_rxp->timeout_pend   = xmit_rxp->timeout_pend   = 0;
	rcv_rxp->timer_pend     = xmit_rxp->timer_pend     = 0;
	rcv_rxp->last_quanta_io = xmit_rxp->last_quanta_io = 0;
	rcv_rxp->reply_port     = xmit_rxp->reply_port     = PORT_NULL;
	midi_dev->inter_msg_q = 1;
	midi_dev->msg_frame_q = 5;
	midi_dev->run_status.ndata = 0;
	midi_dev->sys_exclusive = FALSE;
	midi_dev->queue_max = MIDI_DEF_QUEUE_MAX;
	midi_dev->queue_size = 0;
	midi_dev->parsed_input.nelts = 0;
	midi_dev->frame_time_req = 0;
	midi_dev->pinput_pend = 0;
	midi_dev->callout_pend = 0;
	midi_dev->dev_oflow = 0;
	midi_dev->buf_oflow = 0;
	midi_dev->q_notify_pend = 0;
	midi_dev->q_listen_pend = 0;
	midi_dev->port_disabled = 0;
	return TRUE;
}

/*
 * midi_device_reset -- free resources, reset hardware connection.
 */
void midi_device_reset(u_int unit)
{
	midi_dev_t midi_dev = &midi_var.dev[unit];
	volatile struct zsdevice *zsaddr = midi_dev->addr;
	int s;

	midi_stack_log("midi_device_reset");
	if (!midi_dev->owner)
		return;		// never initialized

	midi_io_reset(unit);

	/*
	 * Deallocate midi transmit port.
	 */
	kern_serv_port_gone(&midi_var.kern_server,
		midi_dev->dir[MIDI_DIR_XMIT].port);

	(void) port_deallocate((task_t)task_self(),
		midi_dev->dir[MIDI_DIR_XMIT].port);
	
	midi_dev->dir[MIDI_DIR_XMIT].port = PORT_NULL;

	/*
	 * Deallocate midi receive port.
	 */
	kern_serv_port_gone(&midi_var.kern_server,
		midi_dev->dir[MIDI_DIR_RECV].port);
	
	(void) port_deallocate((task_t)task_self(),
		midi_dev->dir[MIDI_DIR_RECV].port);
	
	midi_dev->dir[MIDI_DIR_RECV].port = PORT_NULL;

	/*
	 * Clear enqueued output buffers.
	 */
	midi_device_clear_output(unit);

	/*
	 * Remove freed buffers.
	 */
	s = splmidi();		/* in case I/O in progress...*/
	while (!queue_empty(&midi_dev->output_fq)) {
		enqueued_output_t *ob;
		queue_remove_first(&midi_dev->output_fq, ob,
			enqueued_output_t *, link);
		midi_free_ob(ob);
	}
	splx(s);
	
	/*
	 * Free receive buffers.
	 */
	if (midi_dev->parsed_input.buf.cooked) {
		kfree((caddr_t)midi_dev->parsed_input.buf.cooked,
			(char *)midi_dev->parsed_input.ebuf.cooked -
			(char *)midi_dev->parsed_input.buf.cooked);
		midi_dev->parsed_input.buf.cooked = 0;
		midi_dev->parsed_input.nelts = 0;
	}

	if (midi_dev->msgbuf) {
		kfree((caddr_t)midi_dev->msgbuf,
			(char *)midi_dev->emsgbuf - (char *)midi_dev->msgbuf);
		midi_dev->msgbuf = 0;
		midi_dev->emsgbuf  = 0;
		midi_dev->msgin = midi_dev->msgout = 0;
	}
	midi_dev->frame_time_req = 0;
	midi_dev->pinput_pend = 0;
	midi_dev->callout_pend = 0;
	midi_dev->dev_oflow = 0;
	midi_dev->buf_oflow = 0;
	midi_dev->q_notify_pend = 0;
	midi_dev->q_listen_pend = 0;
	if(midi_dev->port_disabled) {
		/*
		 * We're resetting a channel which has removed its xmit port
		 * from the task's port_set due to a full queue. Restore.
		 */
		midi_device_queue_listen(midi_dev);
		midi_dev->port_disabled = 0;
	}
}

/*
 * Set up device protocol type things.
 */
void midi_device_set_proto (
	u_int		rcv_mode,
	boolean_t	relative,
	u_int		inter_msg_q,
	u_int		msg_frame_q,
	u_int		queue_max,
	u_int		unit,
	u_int		dir)
{
	midi_dev_t midi_dev = &midi_var.dev[unit];

	/*
	 * Nothing interesting to do when updating the rcv_mode.  That's
	 * atomic WRT parsing input.
	 */
	midi_stack_log("midi_device_set_proto");
	midi_dev->rcv_mode = rcv_mode;
	midi_dev->dir[dir].relative = relative;

	/*
	 * Changing inter_msg_q and nth_msg_q might cause something
	 * received to be sent immediately.  We'll punt on this one
	 * for now.
	 */
	midi_dev->inter_msg_q = inter_msg_q;
	midi_dev->msg_frame_q = msg_frame_q;
	midi_dev->queue_max = queue_max ? queue_max : MIDI_DEF_QUEUE_MAX;
}

/*
 * midi_device_enqueue_msg -- enqueue this message to be sent out the SCC.
 */
void midi_device_enqueue_msg (
	midi_data_t mdata,
	u_int count,
	u_int type,
	u_int unit)
{
	int s;
	u_int event_time;
	enqueued_output_t *ob;
	midi_dev_t midi_dev = &midi_var.dev[unit];
	midi_rx_t midi_rx = &midi_dev->dir[MIDI_DIR_XMIT];

	midi_stack_log("midi_device_enqueue_msg");
	s = splmidi();
	/*
	 * Free anything waiting to be freed.
	 */
	while (!queue_empty(&midi_dev->output_fq)) {
		queue_remove_first(&midi_dev->output_fq, ob,
			enqueued_output_t *, link);
		midi_free_ob(ob);
	}
	if(count == 0) {
		/*
		 * Nothing to do. This is a weird thing to ask of us, but
		 * it's not illegal.
		 */
		return;
	}
	splx(s);
	ob = midi_alloc_ob(count, type);
	ASSERT(ob);
	splmidi();
	
	/*
	 * Copy the data into a local structure.
	 */
	bcopy(mdata.packed, ob->data.packed, count*midi_data_size(type));

	if (midi_rx->relative) {
		/*
		 * Make everything that's in the queue in absolute time.
		 */
		midi_rx->last_quanta_io = midi_time_rel_to_abs(ob->data,
				          midi_rx->last_quanta_io, 
					  count, 
					  &ob->type);
	}

	/*
	 * Update the output queue size.
	 */
	midi_dev->queue_size += count;

	midi_olog("midi_dev_enq_msg: new q_size %d\n",
		midi_dev->queue_size, 2, 3, 4, 5);

	/*
	 * If the queue's not empty then we just need to add this to the
	 * queue.  Otherwise, if we're paused we can't do much either.
	 */
	if (   midi_rx->pause
	    || (!queue_empty(&midi_dev->output_q) && midi_rx->timer_pend))
	{
		/*
		 * FIXME: might worry about sorting packed into
		 * it's proper place in the queue.
		 */
		midi_olog2("midi_dev_enq_msg: queue (%s)\n",
			midi_rx->pause ? " paused" : "pending timer",
			2, 3, 4, 5);

		queue_enter(&midi_dev->output_q, ob, enqueued_output_t *,
			link);
		splx(s);
		return;
	}

	queue_enter(&midi_dev->output_q, ob, enqueued_output_t *, link);

	/*
	 * Set a timer to get us to output it when it's time comes
	 * (could be now).
	 */
	ob = (enqueued_output_t *)queue_first(&midi_dev->output_q);
	event_time = midi_data_time(ob->data, type);
	midi_olog2("midi_dev_enq_msg: req_timer\n", 1, 2, 3, 4, 5);
	midi_rx->timer_pend = TRUE;
	midi_timer_quanta_req(midi_rx->in_timer,
		midi_rx->in_timer_reply, event_time, FALSE);

	splx(s);
	return;
}

/*
 * Turn the interrupt-level buffer of received data into the proper format.
 *
 * Called either as a result of received data with a valid reply_port, or
 * a midi_get_data() when msgbuf has valid data.
 *
 * We take data from *msgout until we reach msgin.
 */
 
/* #define PROTECT_PARSE	1 	/* */

void midi_device_parse_rcvd_data(u_int unit)
{
	midi_dev_t midi_dev = &midi_var.dev[unit];
	struct parsed_input *pip = &midi_dev->parsed_input;
	int s;
	midi_rx_t midi_rx = &midi_dev->dir[MIDI_DIR_RECV];
	int uspq = (int)midi_rx->act_usec_per_q;
	int size;
	int rcv_mode;
	
	midi_stack_log("midi_device_parse_rcvd_data");
	ASSERT(midi_dev->pinput_pend == 0);

	/* I don't think we need this anymore...*/
#ifdef	PROTECT_PARSE	
	s = splmidi();
	simple_lock(midi_var.slockp);
#endif	PROTECT_PARSE
	
	ASSERT(midi_dev->msgin != midi_dev->msgout);

	/*
	 * Figure out what time it is. now is the current time in quanta. (The
	 * last time we knew this was in midi_device_recv_timer_event().)
	 */

	midi_ilog("midi_dev_parse: rcv'd %d elts last_time %d"
		"  timestamp %d\n",
		(midi_dev->msgin >= midi_dev->msgout) ?
			midi_dev->msgin - midi_dev->msgout :
			(MIDI_RINP_BUFSIZE - 
				(midi_dev->msgout - midi_dev->msgin) + 1),
		midi_dev->dir[MIDI_DIR_RECV].last_time, 
		midi_dev->dir[MIDI_DIR_RECV].timestamp,
		4, 5);
	midi_ilog2("midi_dev_parse in: msgin = 0x%x msgout = 0x%x\n",
		midi_dev->msgin - midi_dev->msgbuf, 
		midi_dev->msgout - midi_dev->msgbuf, 3, 4, 5);
		
	/*
	 * Move data from the input buffer to our send buffer.
	 * If we're in cooked mode stop and compact the input buffer
	 * if we see a system exclusive message.
	 * If the first byte in the input buffer's a system exclusive
	 * message then output the buffer in packed mode.
	 *
	 * Note that the last time we knew the time in quanta was in 
	 * midi_device_recv_timer_event().
	 */
    again:
    	rcv_mode = midi_dev->pushed_packed ? 
		MIDI_PROTO_PACKED : midi_dev->rcv_mode;
	switch (rcv_mode) {
	case MIDI_PROTO_RAW: {
		/* start at end of current data */
		midi_raw_data_t *out_buf = pip->buf.raw + pip->nelts;
		midi_data_t raw_data;
		int nelts=0;
		
		pip->type = MIDI_TYPE_RAW;
		raw_data.raw = out_buf;
		/* 
		 * exit loop at end of either input or output buffer
		 */
		while ((midi_dev->msgin != midi_dev->msgout) &&
		       (out_buf < pip->ebuf.raw)) {
			ASSERT(midi_dev->msgout->ndata == 1);
			/* note msgout->quanta can actually be greater than
			 * midi_rx_timestamp if some Rx interrupts squeaked
			 * thru when this routine was being called...
			 */
			out_buf->quanta = pip->last_quanta = 
			          (int)midi_rx->last_time -  
				(((int)midi_rx->timestamp - 
				  (int)midi_dev->msgout->quanta)) / (int)uspq;
			(out_buf++)->data = midi_dev->msgout->data[0];
			pip->nelts++;
			nelts++;
			midi_dev->msgout = MSGBUF_INCR(midi_dev, msgout);
		}
		if(nelts == pip->nelts)
			pip->first_quanta = pip->buf.raw->quanta;
		if (midi_rx->relative) {
			midi_rx->last_quanta_io = 
			    midi_time_abs_to_rel(
				raw_data,
				midi_rx->last_quanta_io, 
				nelts, 
				&pip->type);
		}
		break;
	}

	case MIDI_PROTO_PACKED: {
		/*
		 * each midi_cooked_data_t in msgbuf contains one packed
		 * byte in msgout->data[0]. Extract these until we get to
		 * midi_dev->msgin, adding them to the end of 
		 * parsed_input.buf.packed[].
		 */
		midi_packed_data_t *out_buf = pip->buf.packed + pip->nelts;
		u_int quanta;
		int last_timestamp;
		int end_of_sysex=0;
		
		if(!pip->nelts) {
			/* skip this if we're just appending...*/
			quanta = (int)midi_rx->last_time -  
			       (((int)midi_rx->timestamp - 
				 (int)midi_dev->msgout->quanta)) / (int)uspq;
			pip->type = MIDI_TYPE_PACKED |
				    (quanta&MIDI_TIMESTAMP_MASK);
			pip->first_quanta = quanta;

			if (midi_rx->relative) {
			    /*
			     * Tricky: this is the only place where 
			     * parsed_input.last_quanta and last_quanta_io 
			     * are different. last_quanta_io is used for 
			     * relative timestamping; parsed_input.last_quanta
			     * is used for message framimg and is actually the 
			     * timestamp of the last byte we append to 
			     * parsed_input.packed[]. 
			     */
			    midi_rx->last_quanta_io = 
			       midi_time_abs_to_rel(
				    pip->buf,
				    midi_rx->last_quanta_io, 
				    1, 
				    &pip->type);
			}
		}
		/* 
		 * exit loop at end of either input or output buffer, or
		 * status byte detected
		 */
		while ((midi_dev->msgin != midi_dev->msgout) &&
   		       (out_buf <  pip->ebuf.packed)) {
		       
		       u_char dbyte = midi_dev->msgout->data[0];
		       
		       if (   midi_dev->pushed_packed
			   && MIDI_TYPE_STATUS(dbyte)
			   && !MIDI_TYPE_SYSTEM_EXCL(dbyte)) {
			   	end_of_sysex = 1;
			   	break;
			}
			ASSERT(midi_dev->msgout->ndata == 1);
			last_timestamp = midi_dev->msgout->quanta;
			
			*out_buf++ = dbyte;
			pip->nelts++;
			midi_dev->msgout = MSGBUF_INCR(midi_dev, msgout);
		}
		pip->last_quanta = (int)midi_rx->last_time -  
		      	 	 (((int)midi_rx->timestamp - 
		      		   last_timestamp)) / (int)uspq;
		if (end_of_sysex) {	

			/*
			 * we are in 'pushed_packed' mode and have gotten to 
			 * the end of a packed (sysex) message. msgout points
			 * to the first status byte after the start of
			 * the sysex message. If this byte is an EOX byte,
			 * copy it to out_buf; else save it for the next
			 * message. Note that not all sysex messages will end
			 * in EOX; that's not our problem.
			 */
			if(midi_dev->msgout->data[0] == MIDI_EOX) {
				*out_buf++ = MIDI_EOX;
				midi_dev->msgout = 
					MSGBUF_INCR(midi_dev, msgout);
			}
			pip->nelts++;
			midi_dev->pushed_packed = FALSE;
			midi_ilog2("midi_dev_parse: exit pushed_packed\n",
				   1, 2, 3, 4, 5);
		}
		break;
	}
	case MIDI_PROTO_COOKED: {
		midi_cooked_data_t *out_buf = pip->buf.cooked + pip->nelts;
		midi_data_t cooked_data;
		int nelts=0;
		int start_of_sysex = 0;
		
		pip->type = MIDI_TYPE_COOKED;
		cooked_data.cooked = out_buf;
		
		/*
		 * If we see a system exclusive message, we'll
		 * need to push on a packed mode to deal with it.
		 */
		while ((midi_dev->msgin != midi_dev->msgout) &&
		       (out_buf < pip->ebuf.cooked))
		{
			if(MIDI_TYPE_SYSTEM_EXCL(midi_dev->msgout->data[0])) {
				start_of_sysex++;
				break;
			}
			midi_ilog2("midi_dev_parse: msgout->quanta was %d\n",
				midi_dev->msgout->quanta, 2, 3, 4, 5);
			midi_dev->msgout->quanta = pip->last_quanta = 
				  (int)midi_rx->last_time -  
				(((int)midi_rx->timestamp - 
				  (int)midi_dev->msgout->quanta)) / (int)uspq;
			midi_ilog2("midi_dev_parse: msgout->quanta is  %d\n",
				midi_dev->msgout->quanta, 2, 3, 4, 5);
			*out_buf++ = *midi_dev->msgout;
			midi_dev->msgout = MSGBUF_INCR(midi_dev, msgout);
			pip->nelts++;
			nelts++;
		}
		if(nelts == pip->nelts)
			pip->first_quanta = pip->buf.cooked->quanta;
		if (midi_rx->relative && nelts) {
			/* note we skip this if we haven't processed anything; 
			 * that would be the case if the first byte in *msgout 
			 * was a SysEx. We either have some cooked data in 
			 * parsed_input, or we'll timestamp parsed_inpput
			 * the next time thru - in pushed_packed mode.
			 */
			midi_rx->last_quanta_io =
			   midi_time_abs_to_rel(
				cooked_data,
				midi_rx->last_quanta_io, 
				nelts, 
				&pip->type);
		}
		if (start_of_sysex) {

			/* Temporarily go into packed mode 
			 * to handle the sysex message. We'll exit
			 * "pushed_packed" mode the next time we see a status
			 * byte. We go to the top of the loop now if the sysex
			 * was the first thing we saw here, else leave the data
			 * in msgbuf for the next time we come here.
			 */
			midi_ilog2("midi_dev_parse in: enter pushed_packed\n",
				1, 2, 3, 4, 5);
			midi_dev->pushed_packed = TRUE;
			if (!pip->nelts) {
				goto again;	/* first byte = sysex */
			}

		} 
		break;
	}
	} /* switch rcv_mode */
	
#ifdef	PROTECT_PARSE	
	simple_unlock(midi_var.slockp);
	splx(s);
#endif	PROTECT_PARSE	
} /* midi_device_parse_rcvd_data() */

/*
 * Called at ipl0 from the msg receive loop to send data that's been
 * parsed.
 */
void midi_device_send_parsed_data(midi_dev_t midi_dev)
{
	kern_return_t r;
	char *s;
	midi_rx_t midi_rx=&midi_dev->dir[MIDI_DIR_RECV];
	
	midi_stack_log("midi_device_send_parsed_data");
#ifdef	CONSOLE_DEBUG
	printf("send_parsed_data: now 0x%x\n", event_get());
#endif	CONSOLE_DEBUG
	midi_ilog("midi_dev_send_p_data ENTERED midi_dev=0x%x\n", 
		midi_dev, 2,3,4,5);
	if(!midi_dev->pinput_pend) {
		/* this happens if we were called by midi_get_data between
		 * a scheduled callout and the time of the callout. No
		 * big deal; the user got their data, so we're done.
		 */
		 midi_olog("midi_dev_send_p_data: !pinput_pend\n",
		 	1,2,3,4,5);
		 return;
	}
	if(!midi_rx->reply_port) {
		midi_ilog("midi_dev_send_p_data: NULL reply_port\n",
			1,2,3,4,5);
		return;
	}
	if(midi_dev->dev_oflow) {
		printf("midi[%d]: Device Overflow\n", midi_dev-midi_var.dev);
		midi_dev->dev_oflow = 0;
	}
	if(midi_dev->buf_oflow) {
		printf("midi[%d]: Buffer Overflow\n", midi_dev-midi_var.dev);
		midi_dev->buf_oflow = 0;
	}
	switch (midi_dev->parsed_input.type&MIDI_TYPE_MASK) {
	case MIDI_TYPE_RAW:
		r = midi_ret_raw_data(midi_rx->reply_port,
			midi_dev->parsed_input.buf.raw,
			midi_dev->parsed_input.nelts);
#ifdef	DEBUG
		s = "RAW";
#endif	DEBUG
		break;
	case MIDI_TYPE_PACKED:
		r = midi_ret_packed_data(midi_rx->reply_port,
			midi_dev->parsed_input.type&MIDI_TIMESTAMP_MASK,
			midi_dev->parsed_input.buf.packed,
			midi_dev->parsed_input.nelts);
#ifdef	DEBUG
		s = "PACKED";
#endif	DEBUG
		break;
	case MIDI_TYPE_COOKED:
		r = midi_ret_cooked_data(midi_rx->reply_port,
			midi_dev->parsed_input.buf.cooked,
			midi_dev->parsed_input.nelts);
#ifdef	DEBUG
		s = "COOKED";
#endif	DEBUG
		break;
#ifdef	DEBUG
	default:
		printf("midi_device_send_parsed_data(): data type = %XH\n",
			 midi_dev->parsed_input.type&MIDI_TYPE_MASK);
		ASSERT(0);
#endif	DEBUG
	}

	midi_ilog("midi_dev_send_p_data: "
		"send %d %s elts to port %d returned %d\n",
		midi_dev->parsed_input.nelts, s,
		midi_rx->reply_port, r, 5);
#ifdef	DEBUG
	if(r) {
		printf("midi_ret_ %s _data returned %XH\n", s, r);
	}
#endif	DEBUG
	ASSERT(r == 0);
	midi_rx->reply_port = PORT_NULL;
	midi_dev->parsed_input.nelts = 0;	/* now empty */
	midi_dev->frame_time_req = 0;
		
	/* let driver know that parsed_input is now available */
	midi_dev->pinput_pend = 0;
	midi_rx->timer_pend = TRUE;
	midi_timer_quanta_req(midi_rx->in_timer, 
		              midi_rx->in_timer_reply,
			      0, 	/* now */
			      TRUE);	/* relative */
			      
} /* midi_device_send_parsed_data() */

/*
 * Called at ipl0 from the msg receive loop to send queue size notification.
 */
void midi_device_send_queue_req(midi_dev_t midi_dev)
{
	midi_stack_log("midi_device_send_queue_req");
	if (midi_dev->dir[MIDI_DIR_XMIT].reply_port == PORT_NULL)
		return;

	midi_queue_notify(midi_dev->dir[MIDI_DIR_XMIT].reply_port,
		midi_dev->queue_size);
	midi_dev->dir[MIDI_DIR_XMIT].reply_port = PORT_NULL;
	midi_dev->q_notify_pend = 0;
}

/*
 * Called at ipl0 from the msg receive loop to enable listening on device port.
 */
void midi_device_queue_listen(midi_dev_t midi_dev)
{
	midi_stack_log("midi_device_queue_listen");
	if (midi_dev->dir[MIDI_DIR_XMIT].port == PORT_NULL)
		return;

	midi_olog2("midi_dev_q_listen[%d]: port %d\n",
		midi_dev - midi_var.dev,
		midi_dev->dir[MIDI_DIR_XMIT].port, 3, 4, 5);
	midi_dev->q_listen_pend = 0;
	port_set_add((task_t)task_self(),
		kern_serv_port_set(&midi_var.kern_server),
		midi_dev->dir[MIDI_DIR_XMIT].port);
	midi_dev->port_disabled = 0;
}

/*
 * Set the midi system messages we're going to ignore.
 * We must scan (and compress) our input buffer to remove all messages
 * that we're now not receiving.
 */
void midi_device_set_sys_ignores(u_int sys_ignores, u_int unit)
{
	/*
	 * FIXME: get rid of stored stuff.
	 */
	midi_stack_log("midi_device_set_sys_ignores");
	midi_var.dev[unit].system_ignores = sys_ignores;
}

/*
 * Something triggered a receive timer event.
 * For now we only handle expired timer events.
 *
 * An expired timer means one of the following things has occurred:
 *
 * -- the interrupt handler detected the end of single message (last byte
 *    of a 2- or 3-byte cooked message or any other byte)
 * -- midi_device_send_parsed_data() has sent the data in 
 *    midi_dev->parsed_input to the caller, freeing up parsed_input.
 * -- either an inter_msg_quanta timeout or a msg_frame_quanta timeout
 *    has expired.
 * -- midi_get_data() was called when pinput_pending was true, meaning we
 *    have data ready to go which was awaiting a reply_port.
 *
 * This can get called at ipl0 (from midi_get_data()) or at ipl2 (softint
 * scheduled by the rx interrupt handler).
 */
void midi_device_recv_timer_event (
	u_int		quanta,
	u_int		real_usec_per_quantum,
	boolean_t	timer_expired,
	boolean_t	timer_stopped,
	boolean_t	timer_forward,
	u_int		unit)
{
	u_char do_callout;
	u_char go_again;
	typedef void (*void_fun_t)(void *);
	midi_dev_t midi_dev = &midi_var.dev[unit];
	midi_rx_t midi_rx = &midi_dev->dir[MIDI_DIR_RECV];
	struct parsed_input *pip = &midi_dev->parsed_input;
	int s;
	
	midi_stack_log("midi_device_recv_timer_event");
	s = curipl(); 
	if(s > IPLNET)
		ASSERT(s <= IPLNET);
	s = splnet();
	midi_rx->pause = timer_stopped;
	midi_rx->act_usec_per_q = real_usec_per_quantum;
	midi_rx->last_time = quanta;
	midi_rx->timer_pend = !timer_expired;
	midi_rx->timestamp = event_get();

	midi_ilog("midi_dev_r_timer[%d], entered r_port %d%s%s last_time %d\n",
		unit, midi_rx->reply_port,
		timer_stopped ? " paused" : " !paused",
		timer_expired ? " expired" : " !expired", 
		quanta);

	/*
	 * Startup the thing that's next in the queue if parsed_input is 
	 * available and there's something to parse. Note we parse even if
	 * we don't have a reply port; when we get a full frame, we'll stop
	 * parsing (via pinput_pend).
	 */
	if (   !timer_stopped
	    && timer_expired
	    && !midi_dev->pinput_pend
	    && midi_dev->msgin != midi_dev->msgout)
	{
		midi_device_parse_rcvd_data(unit);
	}

	/*
	 * Queue up a request to send data back to the user (at ipl0). We do
	 * this in the following cases:
	 *
	 * -- msg_frame_quanta has elapsed since quanta of first elt in
	 *    parsed_input
	 * -- inter_mag_quanta has elapsed since quanta of last elt in
	 *    parsed_input
	 * -- pushed_packed mode true and parsed_input has cooked data (start
	 *    of pushed_packed; packed data ready to be parsed in msgbuf). 
	 * -- pushed_packed false, rcv_mode = COOKED, and parsed_input has 
	 *    packed data (end of pushed_packed; cooked data ready to be 
	 *    parsed in msgbuf). 
	 * -- parsed_input.nelts == max size of buffer
	 */
	do_callout = 0;
	if(pip->nelts && !midi_dev->pinput_pend) {
		switch(pip->type&MIDI_TYPE_MASK) {
		    case MIDI_TYPE_RAW:
			break;
		    case MIDI_TYPE_COOKED:
		    	if(midi_dev->pushed_packed) {
				/* this cooked data must go */
				do_callout++;
#ifdef	CONSOLE_DEBUG
				printf("force cooked now 0x%x\n", event_get());
#endif	CONSOLE_DEBUG
	   			midi_ilog("midi_dev_r_timer[%d], force cooked "
				          "data\n",
					  unit, 2, 3, 4, 5);
			}
			break;
		    case MIDI_TYPE_PACKED:
		    	if((midi_dev->rcv_mode == MIDI_PROTO_COOKED) &&
			   (!midi_dev->pushed_packed)) {
			 	/* this packed data must go */
				do_callout++;
#ifdef	CONSOLE_DEBUG
				printf("force packed now 0x%x\n", event_get());
#endif	CONSOLE_DEBUG
	   			midi_ilog("midi_dev_r_timer[%d], force packed "
				          "data\n",
					  unit, 2, 3, 4, 5);
			}
		        break;
		} /* switch type */
		if((quanta > pip->first_quanta) &&
		   (quanta - pip->first_quanta >= midi_dev->msg_frame_q)) {
			midi_ilog("midi_dev_r_timer[%d], msg_frame_q "
				  "expired; first_quanta = %d  quanta = %d\n",
				   unit, pip->first_quanta, quanta, 4, 5);
#ifdef	CONSOLE_DEBUG
			printf("msg_frame_q quanta = %d first_quanta = %d"
				"  last_time = %d   timestamp = %d\n",
				quanta, pip->last_quanta, midi_rx->last_time,
				midi_rx->timestamp);
			printf("   now 0x%x\n", event_get());
#endif	CONSOLE_DEBUG
			do_callout++; 
		}
		if((quanta > pip->last_quanta) &&
		   (quanta - pip->last_quanta >= midi_dev->inter_msg_q)) {
			midi_ilog("midi_dev_r_timer[%d], inter_msg_q "
				  "expired; last_quanta = %d  quanta = %d\n",
				   unit, pip->last_quanta, quanta, 4, 5);
#ifdef	CONSOLE_DEBUG
			printf("inter_msg_q quanta = %d last_quanta = %d"
				"  last_time = %d   timestamp = %d\n",
				quanta, pip->last_quanta, midi_rx->last_time,
				midi_rx->timestamp);
			printf("   now 0x%x\n", event_get());
#endif	CONSOLE_DEBUG
			do_callout++; 
		}
	} /* if nelts */
	if(pip->nelts * (midi_data_size(pip->type&MIDI_TYPE_MASK)) >=
		MIDI_PINP_BUFSIZE * sizeof(midi_cooked_data_t)) {
	   	midi_ilog("midi_dev_r_timer[%d], input buf full\n",
			unit, 2, 3, 4, 5);
#ifdef	CONSOLE_DEBUG
		printf("parsed_input size now 0x%x\n", event_get());
#endif	CONSOLE_DEBUG
		do_callout++;
	}
	if(do_callout && !midi_dev->pinput_pend) {
#ifdef	CONSOLE_DEBUG
		printf("pinput_pend = 1 now = 0x%x us\n", event_get());
#endif	CONSOLE_DEBUG
		midi_ilog("midi_dev_r_timer; doing callout\n", 1, 2, 3, 4, 5);
		midi_dev->pinput_pend = 1;
	    	kern_serv_callout(&midi_var.kern_server,
		   (void_fun_t)midi_device_send_parsed_data, (void *)midi_dev);
	}
				
	/*
    	 * schedule a timer event for:
         *
         * quanta of last elt in parsed_input + inter_msg_quanta, 
	 *	if nothing got sent. 
	 * quanta of first elt in parsed_input + msg_frame_quanta,
	 *	if we haven't already done so for this parsed_input.
	 * otherwise, 20 seconds in future, to be notified of any state 
	 * 	changes.
	 *
	 * We don't set up a timer request if parsed_input is full and 
	 * awaiting transmission to caller since we can't do anything
	 * until that happens. (midi_device_send_parsed_data() will give
	 * us a timer req then.)
	 */
	if(pip->nelts && !midi_dev->pinput_pend) {
		/* We're holding onto the data we have. */
		midi_ilog("midi_dev_r_timer[%d], requesting inter_msg "
			  "timer @ %d\n",
			  unit, 
			  pip->last_quanta + midi_dev->inter_msg_q, 
			  3, 4, 5);
		midi_rx->timer_pend = TRUE;
		midi_timer_quanta_req(midi_rx->in_timer,
			midi_rx->in_timer_reply,
			pip->last_quanta + midi_dev->inter_msg_q,
			FALSE);	
		if(!midi_dev->frame_time_req) {
#ifdef	CONSOLE_DEBUG
			printf("frame_time req now 0x%x us req = %d quanta\n",
				event_get(), 
				pip->first_quanta + midi_dev->msg_frame_q);
#endif	CONSOLE_DEBUG
			midi_ilog("midi_dev_r_timer[%d], requesting frame "
				  "timer @ %d\n",
				  unit, 
				  pip->first_quanta + midi_dev->msg_frame_q, 
				  3, 4, 5);
			midi_timer_quanta_req(midi_rx->in_timer,
				midi_rx->in_timer_reply,
				pip->first_quanta + midi_dev->msg_frame_q,
				FALSE);	
			midi_dev->frame_time_req = 1;
		}
	}
	else if (timer_expired) {
		midi_rx->timer_pend = TRUE;
		midi_timer_quanta_req(midi_rx->in_timer,
			midi_rx->in_timer_reply,
			quanta + 20000000 / real_usec_per_quantum, /* 20 sec */
			FALSE);
	}
	splx(s);
	midi_ilog("midi_dev_r_timer[%d], done\n",
		unit, 2, 3, 4, 5);
		
} /* midi_device_recv_timer_event() */

/*
 * Something triggered a transmit timer event.
 * For now we only handle expired timer events.
 */
void midi_device_xmit_timer_event (
	u_int		quanta,
	u_int		real_usec_per_quantum,
	boolean_t	timer_expired,
	boolean_t	timer_stopped,
	boolean_t	timer_forward,
	u_int		unit)
{
	int s;
	midi_dev_t midi_dev = &midi_var.dev[unit];
	midi_rx_t midi_rx = &midi_dev->dir[MIDI_DIR_XMIT];
	enqueued_output_t *ob;
	u_int then;

	midi_stack_log("midi_device_xmit_timer_event");
	ASSERT(midi_rx->timer_pend);

	midi_rx->pause = timer_stopped;
	midi_rx->act_usec_per_q = real_usec_per_quantum;
	midi_rx->last_time = quanta;
	midi_rx->timer_pend = !timer_expired;
	midi_rx->timestamp = event_get();

	midi_olog2("midi_dev_x_timer[%d], last_time %d entered\n",
		unit, quanta, 3, 4, 5);

	if (timer_stopped && !timer_expired) {
		midi_olog("midi_dev_x_timer: "
			   "stopped && !expired, exit\n", 1, 2, 3, 4, 5);
		return;
	}

	/* midi_olog2("midi_dev_x_timer[%d], start io out\n",
		unit, 2, 3, 4, 5);
	 */
	midi_io_start_output(unit);

	/*
	 * Request a timer for the next time we have something to output.
	 * Avoid making duplicate requests by only paying attention to
	 * events whose quanta are greater than the last time we were
	 * here.
	 */
	then = quanta;
	s = splmidi();
	ob = (enqueued_output_t *)queue_first(&midi_dev->output_q);
	while (!queue_end(&midi_dev->output_q, (queue_t)ob) && then == quanta)
	{
		u_int type = ob->type;
		switch (type&MIDI_TYPE_MASK) {
		case MIDI_TYPE_RAW: {
			midi_raw_data_t *rd = ob->data.raw;
			while (rd < ob->edata.raw && rd->quanta <= quanta)
				rd++;
			if (rd < ob->edata.raw)
				then = rd->quanta;
			break;
		}
		case MIDI_TYPE_COOKED: {
			midi_cooked_data_t *cd = ob->data.cooked;
			while (cd < ob->edata.cooked && cd->quanta <= quanta)
				cd++;
			if (cd < ob->edata.cooked)
				then = cd->quanta;
			break;
		}
		case MIDI_TYPE_PACKED:
			/* avoid multiple requests for this packed data */
			if((type&MIDI_TIMESTAMP_MASK) > quanta)
				then = type&MIDI_TIMESTAMP_MASK;
			break;
		}
		ob = (enqueued_output_t *)queue_next(&ob->link);
	}
	splx(s);

	/* the next event may be scheduled prior to now (if we've gotten
	 * behind). There's no need to schedule a timer event in that case;
	 * the interrupt handler and midi_io_start_output() will take it
	 * from here.
	 */
	 
	/*
	 * Re-schedule another interrupt for either 
	 * 	the next event after the quanta at which we were called, or
	 *     	a few seconds in the future (if there aren't any more
	 *  	messages), just so that we'll be notified of any state changes.
	 */
	if (then == quanta)
		then += 20000000 / real_usec_per_quantum;  /* 20 seconds */

	midi_olog2("midi_dev_x_timer[%d], req timer then = %d\n",
		unit, then, 3, 4, 5);

	midi_rx->timer_pend = TRUE;
	midi_timer_quanta_req(midi_rx->in_timer,
		midi_rx->in_timer_reply, then, FALSE);

	midi_olog2("midi_dev_x_timer[%d], done\n",
		unit, 2, 3, 4, 5);
		
} /* midi_device_xmit_timer_event() */

/*
 * Free everything that's enqueued to be output.
 */
void midi_device_clear_output(u_int unit)
{
	midi_dev_t midi_dev = &midi_var.dev[unit];
	int s;
	
	midi_stack_log("midi_device_clear_output");
	s = splmidi();
	while (!queue_empty(&midi_dev->output_q)) {
		enqueued_output_t *ob;
		queue_remove_first(&midi_dev->output_q, ob,
			enqueued_output_t *, link);
		midi_free_ob(ob);
	}
	splx(s);
	midi_dev->queue_size = 0;
}

/*
 * Reset the input buffers.
 */
void midi_device_clear_input(u_int unit)
{
	midi_dev_t midi_dev = &midi_var.dev[unit];
	
	midi_stack_log("midi_device_clear_input");
	midi_dev->msgin = midi_dev->msgout = midi_dev->msgbuf;
	midi_dev->frame_time_req = 0;
	midi_dev->pinput_pend = 0;
	midi_dev->callout_pend = 0;
	midi_dev->parsed_input.nelts = 0;
	midi_dev->dev_oflow = 0;
}

static u_int midi_time_rel_to_abs (	
	midi_data_t	data,
	u_int		cur_time,
	u_int		count,
	u_int		*type)
{
	/* convert each element in *data to absolute time. Returns absolute
	 * quanta of last element in queue.
	 */
	u_int i;
	u_int last_quanta = cur_time;
	
	midi_stack_log("midi_time_rel_to_abs");
	switch ((*type)&MIDI_TYPE_MASK) {
	case MIDI_TYPE_RAW:
		for (i = 0; i < count; i++) {
			data.raw[i].quanta += last_quanta;
			last_quanta = data.raw[i].quanta;
		}
		break;
	case MIDI_TYPE_COOKED:
		for (i = 0; i < count; i++) {
			data.cooked[i].quanta += last_quanta;
			last_quanta = data.cooked[i].quanta;   /* dpm */
		}
		break;
	case MIDI_TYPE_PACKED:
		last_quanta = ((*type)&MIDI_TIMESTAMP_MASK) + cur_time;
		*type &= MIDI_TYPE_MASK;
		*type |= last_quanta;
		break;
	}
	midi_olog2("rel_to_abs: *type = %X cur_time = %d"
		   " last_quanta = %d\n",
		   *type, cur_time, last_quanta, 4, 5);
	return(last_quanta);
	
} /* midi_time_rel_to_abs() */

static u_int midi_time_abs_to_rel (	
	midi_data_t	data,
	u_int		cur_time,
	u_int		count,
	u_int		*type)
{
	/* convert each element in *data to relative time. Returns absolute
	 * quanta of last element in queue.
	 */
	u_int i;
	u_int last_quanta;
	
	midi_stack_log("midi_time_abs_to_rel");
	switch ((*type)&MIDI_TYPE_MASK) {
	case MIDI_TYPE_RAW:
		for (i = 0; i < count; i++) {
			last_quanta = data.raw[i].quanta;
			data.raw[i].quanta -= cur_time;
			cur_time = last_quanta;
		}
		break;
	case MIDI_TYPE_COOKED:
		for (i = 0; i < count; i++) {
			last_quanta = data.cooked[i].quanta;
			data.cooked[i].quanta -= cur_time;
			cur_time = last_quanta;
		}
		break;
	case MIDI_TYPE_PACKED:
		last_quanta = (*type)&MIDI_TIMESTAMP_MASK;
		*type &= MIDI_TYPE_MASK;
		*type |= (last_quanta - cur_time);
		break;
	}
	midi_olog2("abs_to_rel: *type = %X cur_time = %d"
		   " last_quanta = %d\n",
		   *type, cur_time, last_quanta, 4, 5);
	return(last_quanta);
	
} /* midi_time_abs_to_rel() */





