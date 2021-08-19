/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 18-Jun-90	Gregg Kellogg (gk) at NeXT
 *	Don't use kern_server instance var directly.
 *
 * 04-Apr-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

/*
 * Midi device driver for the Zilog 8530 SCC
 * Low-level device specific code.
 */

#import "midi_var.h"
#import <sys/callout.h>
#import <nextdev/zscom.h>
#import <nextdev/zsreg.h>
#import <next/eventc.h>

extern midi_var_t midi_var;

#define ZS_ON (WR5_RTS | WR5_DTR)
#define ZS_OFF WR5_RTS

#define midi_ilog2	midi_ilog
#define midi_olog2	midi_olog
//#define midi_ilog2(m, a1, a2, a3, a4, a5)
//#define midi_olog2(m, a1, a2, a3, a4, a5)
/* #define CONSOLE_DEBUG	1	/* */

/*
 * proc declarations
 */
/* interrupt vectors */
static int midi_io_xmit_intr(u_int chan);
static int midi_io_recv_intr(u_int chan);
static int midi_io_status_intr(u_int chan);

/*
 * switch to vector interrupts via zscom.c
 */
static struct zs_intrsw zi_midi = {
	(int (*)(int))midi_io_xmit_intr,
	(int (*)(int))midi_io_recv_intr,
	(int (*)(int))midi_io_status_intr };

/*
 * device I/O routines
 */
static void midi_io_sched_rcv_time(u_int unit);

/*
 * midi_zsinit -- initialize Zilog 8530 SCC
 */
boolean_t midi_io_init(u_int unit)
{
	struct zs_com *zcp;
	volatile struct zsdevice *zsaddr;
	int reset, baudrate, s;

	/*
	 * zs_com is common data shared between all drivers
	 * which utilize the SCC
	 */
	midi_stack_log("midi_io_init");
	zcp = &zs_com[unit];
	s = splsched();
	if (zcp->zc_user != ZCUSER_NONE && zcp->zc_user != ZCUSER_MIDI) {
	    	splx(s);
		return FALSE;
	}
	zcp->zc_user = ZCUSER_MIDI;
	zcp->zc_intrsw = &zi_midi;
	splx(s);

	/*
	 * reset channel, errors, and status
	 * setup wr1 (interrupt enables & dma), wr9 (no vector),
	 * wr11 (clock source), wr14 (br gen enable & dma),
	 * and wr15 (interrupt enables), wr0 (error and status reset),
	 * wr9 (Master Interrupt Enable)
	 */
	if (unit == 0) {
 		zsaddr = midi_var.dev[unit].addr = ZSADDR_A;
		reset = WR9_RESETA;
	} else {
 		zsaddr = midi_var.dev[unit].addr = ZSADDR_B;
		reset = WR9_RESETB;
	}
	/*
	 * FIXME: make sure we get all the appropriate enables on here
	 */
	baudrate = zs_tc(MIDI_BAUD, 16);

	s = splmidi();
	ZSWRITE(zsaddr, 9, reset);
	DELAY(10);	/* FIXME, is this necessary see 4.1.10 in man */

	ZSWRITE(zsaddr, 9, WR9_NV | WR9_MIE);
	ZSWRITE(zsaddr, 10, WR10_NRZ);
	ZSWRITE(zsaddr, 11, WR11_TXCLKBRGEN|WR11_RXCLKBRGEN);
	/* no DCD or break interrupt enable */
	ZSWRITE(zsaddr, 15, 0);
	DELAY(1);
	zsaddr->zs_ctrl = WR0_RESET;
	DELAY(1);
	zsaddr->zs_ctrl = WR0_RESET_STAT;
	DELAY(1);
	zsaddr->zs_ctrl = WR0_RESETTXPEND;

	ZSWRITE(zsaddr, 4, WR4_X16CLOCK|WR4_STOP1);
	ZSWRITE(zsaddr, 3, WR3_RX8);	/* receiver parameters */
	ZSWRITE(zsaddr, 5, WR5_TX8 | ZS_ON); /* transmiter parms */
	/*
	 * baud rate generator has to be disabled while switching
	 * time constants and clock sources
	 */
	ZSWRITE(zsaddr, 14, 0);		/* Baud rate generator disabled */
	ZSWRITE(zsaddr, 12, baudrate);	/* low bits of time constant */
	ZSWRITE(zsaddr, 13, baudrate >> 8); /* high bits of time constant */
	ZSWRITE(zsaddr, 14, baudrate >> 16);/* clock source */
	DELAY(10);
	ZSWRITE(zsaddr, 14, WR14_BRENABLE|(baudrate >> 16));
	ZSWRITE(zsaddr, 3, WR3_RX8 | WR3_RXENABLE);
	ZSWRITE(zsaddr, 5, WR5_TX8 |WR5_RTS | WR5_TXENABLE);
	splx(s);

	midi_var.dev[unit].dir[MIDI_DIR_XMIT].intren = FALSE;
	midi_var.dev[unit].dir[MIDI_DIR_RECV].intren = FALSE;

	return TRUE;
}

void midi_io_reset(u_int unit)
{
	int s;
	struct zs_com *zcp = &zs_com[unit];
	boolean_t rcv_intren = midi_var.dev[unit].dir[MIDI_DIR_RECV].intren;
	volatile struct zsdevice *zsaddr = midi_var.dev[unit].addr;

	midi_stack_log("midi_io_reset");
	midi_var.dev[unit].dir[MIDI_DIR_XMIT].intren = FALSE;
	midi_var.dev[unit].dir[MIDI_DIR_RECV].intren = FALSE;

	ZSWRITE(zsaddr, 1, WR1_EXTIE | ((rcv_intren)?WR1_RXALLIE:0));

	s = splmidi();
	ZSWRITE(zsaddr, 5, WR5_TX8 | ZS_OFF); /* clear DTR */
	ZSWRITE(zsaddr, 1, 0);	/* clear interrupt enables */
	splx(s);

	zcp->zc_intrsw = &zi_null;
	zcp->zc_user = ZCUSER_NONE;
}

/*
 * Enable the device.
 */
void midi_io_enable(u_int unit)
{
	volatile struct zsdevice *zsaddr = midi_var.dev[unit].addr;

	midi_stack_log("midi_io_enable");
	midi_var.dev[unit].dir[MIDI_DIR_XMIT].intren = TRUE;
	midi_var.dev[unit].dir[MIDI_DIR_RECV].intren = TRUE;
	ZSWRITE(zsaddr, 1, WR1_EXTIE|WR1_RXALLIE|WR1_TXIE);
}

/*
 * Start up I/O on a device.
 */
boolean_t midi_io_start_output(u_int unit)
{
	int s;
	midi_dev_t midi_dev = &midi_var.dev[unit];
	midi_rx_t midi_rx = &midi_dev->dir[MIDI_DIR_XMIT];
	volatile struct zsdevice *zsaddr = midi_dev->addr;
	enqueued_output_t *ob;
	u_char data;
	u_int now = midi_device_curtime(unit, MIDI_DIR_XMIT);
	u_int req;
	typedef void (*void_fun_t)(void *);

	midi_olog("midi_io_start_output(%d)\n",unit,2,3,4,5);
	/* midi_stack_log("midi_io_start_output");   1 - in fails
						     2 - in works
						     3 - out fails */
#ifdef	DEBUG
	if ((int)now < 0) {
		midi_olog("midi_io_start_output: clock screwed up? "
			"now %d, timestamp 0x%x, last_time %d\n",
			now, midi_rx->timestamp, midi_rx->last_time, 4, 5);
	}
#endif	DEBUG

	/*
	 * We have to stay at splmidi for this entire routine to avoid
	 * a Tx empty interrupt coming in and smashing what we're about to
	 * output...
	 */
	s = splmidi();
	simple_lock(midi_var.slockp);
	if (!(zsaddr->zs_ctrl & RR0_TXEMPTY)) {
		/*
		 * If there's still something in the transmit register
		 * things'll start up on their own.
		 */
		simple_unlock(midi_var.slockp);
		splx(s);
		midi_olog2("midi_io_start_output[%d]: tx full, exit\n",
			unit, 2, 3, 4, 5);
		return FALSE;
	}

	if (queue_empty(&midi_dev->output_q)) {
		ASSERT(midi_dev->queue_size == 0);
		midi_olog2("midi_io_start_output[%d]: output_q empty, exit\n",
			unit, 2, 3, 4, 5);
		simple_unlock(midi_var.slockp);
		splx(s);
		return FALSE;
	}
	
	ob = (enqueued_output_t *)queue_first(&midi_dev->output_q);
	req = midi_data_time(ob->data, ob->type);
	midi_olog2("midi_io_start_output[%d]: now %d, req %d\n",
		unit, now, req, 4, 5);

	if (req > now) {
		midi_olog2("midi_io_start_output[%d]: not yet time, "
			"wait %d quanta\n", unit, req-now, 3, 4, 5);
		simple_unlock(midi_var.slockp);
		splx(s);
		return FALSE;
	}
	simple_unlock(midi_var.slockp);
#ifdef	MIDI_MCOUNT
	if(ob->nelts == 0) {
		midi_dopanic("midi_io_start_output (1): ob->nelts == 0\n", ob);
	}
#endif	MIDI_MCOUNT
	/*
	 * Get the byte we need to output.
	 */
	switch (ob->type&MIDI_TYPE_MASK) {
	case MIDI_TYPE_RAW:
		data = ob->data.raw->data;
		ob->data.raw++;
		ob->nelts--;
		midi_dev->queue_size--;
		break;
	case MIDI_TYPE_COOKED:
		data = ob->data.cooked->data[0];
#ifdef	MIDI_MCOUNT
		if(ob->data.cooked->ndata == 0x10) {
			midi_dopanic("AHA (1): ndata = 0xf\n", ob);
		}
#endif	MIDI_MCOUNT
		switch (ob->data.cooked->ndata--) {
		case 1:
			ob->data.cooked++;
			ob->nelts--;
			midi_dev->queue_size--;
			break;
		case 2:
			ob->data.cooked->data[0] = ob->data.cooked->data[1];
			break;
		case 3:
			ob->data.cooked->data[0] = ob->data.cooked->data[1];
			ob->data.cooked->data[1] = ob->data.cooked->data[2];
			break;
		default:
			// Through this packed away.
			ob->data.cooked = ob->edata.cooked;
			data = 0;
			break;
		}
#ifdef	MIDI_MCOUNT
		if(ob->data.cooked->ndata == 0xf) {
			midi_dopanic("AHA (2): ndata = 0xf\n", ob);
		}
#endif	MIDI_MCOUNT
		break;

	case MIDI_TYPE_PACKED:
		data = *ob->data.packed++;
		ob->nelts--;
		midi_dev->queue_size--;
		break;
	}

	midi_olog2("midi_io_start_output[%d]: data 0x%x, type %s\n",
		unit, data,
		  ob->type == MIDI_TYPE_RAW
		? "raw"
		: (  ob->type == MIDI_TYPE_COOKED
		   ? "cooked"
		   : "packed"), 4, 5);

	/*
	 * Free the queue element if the element's
	 * exhausted.  Enqueue it on the free list.
	 */
	if (ob->data.raw >= ob->edata.raw) {
		s = splmidi();
		queue_remove(&midi_dev->output_q, ob, enqueued_output_t *,
			link);
		midi_olog("midi_io_start_output: free buf\n", 1, 2, 3, 4, 5);
		queue_enter(&midi_dev->output_fq, ob, enqueued_output_t *,
			link);
		splx(s);
	}
	else if (ob->nelts == 0) {
		midi_panic("midi_io_start_output: ob->nelts == 0!\n");
	}

	/*
	 * Output the data byte. 
	 */
	/* DELAY(1); */
	zsaddr->zs_data = data;

	/*
	 * If someone's waiting for the queue to get down to (or below)
	 * the current size, let him know now. The q_notify_pend mechanism
	 * handles a race condition in which we come thru this way again
	 * before the midi_device_send_queue_req routine NULLs out reply_port.
	 */
	if ((midi_rx->reply_port) &&
	    (!midi_dev->q_notify_pend) &&
	    (midi_dev->queue_size <= midi_dev->queue_req_size))
	{
		midi_olog2("midi_io_start_output: sched send_q_req\n",
			1, 2, 3, 4, 5);
		midi_dev->q_notify_pend = 1;
		kern_serv_callout(&midi_var.kern_server,
			(void_fun_t)midi_device_send_queue_req,
			(void *)midi_dev);
	}

	/*
	 * If we have more room in the queue, start listening to the
	 * port again. q_listen_pend is similar to q_notify_pend, above.
	 * The port_disabled flag prevents us from doing a callout every time
	 * thru here (25-Jul-90 dmitch)!
	 */
	if ((!midi_dev->q_listen_pend) &&
	    (midi_dev->port_disabled) &&
	    (midi_dev->queue_size <= midi_dev->queue_max)) {
		midi_olog2("midi_io_start_output: sched q_listen\n",
			1, 2, 3, 4, 5);
		midi_dev->q_listen_pend = 1;
		kern_serv_callout(&midi_var.kern_server,
			(void_fun_t)midi_device_queue_listen,
			(void *)midi_dev);
	}
	splx(s);
	return TRUE;
}

/*
 * Get ourselves a timer interrupt immediately.  Called when we're at
 * too high an IPL to do it directly.
 */
static void midi_io_sched_rcv_time(u_int unit)
{
	midi_rx_t midi_rx = &midi_var.dev[unit].dir[MIDI_DIR_RECV];

	midi_ilog("midi_io_sched_rcv_time[%d]\n", unit, 2, 3, 4, 5);
	midi_stack_log("midi_io_sched_rcv_time");

	midi_var.dev[unit].callout_pend = 0;
	midi_rx->timer_pend = TRUE;
	midi_timer_quanta_req(midi_rx->in_timer, midi_rx->in_timer_reply,
		0, TRUE);
}

/*
 * device interrupt support routines
 */

/*
 * midi_io_xmit_intr -- handle SCC transmitter interrupts for SCC channel
 * without dma hardware
 * MUST BE CALLED AT splmidi()
 */
static int midi_io_xmit_intr(u_int unit)
{
	midi_dev_t midi_dev = &midi_var.dev[unit];
	volatile struct zsdevice *zsaddr = midi_dev->addr;

	midi_stack_log("midi_io_xmit_intr");
	ASSERT(curipl() == MIDI_IPL);

	DELAY(1);
	if (!(zsaddr->zs_ctrl & RR0_TXEMPTY)) {
		printf("midi_io_xmit_intr: xmit reg not empty!\n");
		return;
	}

	midi_olog2("midi_io_xmit_intr[%d]\n", unit, 2, 3, 4, 5);

	/*
	 * Output the next byte if we have one or reset the
	 * interrupt if we don't.
	 */
	if (   midi_dev->dir[MIDI_DIR_XMIT].pause
	    || !midi_io_start_output(unit))
	{
		DELAY(1);
		midi_olog2("midi_io_xmit_intr[%d] reset txpend\n",
			unit, 2, 3, 4, 5);
		zsaddr->zs_ctrl = WR0_RESETTXPEND;
	}
#ifdef	MIDI_MCOUNT
	/* midi_mcount_caller(); */
#endif	MIDI_MCOUNT
	return 0;
}

/*
 * midi_io_recv_intr -- handle SCC receiver interrupts
 * drop a time stamp in circular buffer and then queue in characters
 * MUST BE CALLED AT splmidi()
 *
 * Data is recorded using the local machine's timestamp.  This is converted
 * to the timer-relative time when the input queue is parsed.
 *
 * Incoming data goes to *midi_dev->msgin. The input buffer is full when
 * midi_dev->msgout = midi_dev->msgin+1 (modulo buffer size). We detect this
 * before writing to *msgin. The buffer can only hold (buffer size - 1)
 * elements.
 */
static int midi_io_recv_intr(u_int unit)
{
	midi_dev_t midi_dev = &midi_var.dev[unit];
	volatile struct zsdevice *zsaddr = midi_dev->addr;
	midi_rx_t midi_rx = &midi_dev->dir[MIDI_DIR_RECV];
	register int rr0, rr1, data;
	int ship_data=0;
	
	midi_stack_log("midi_io_recv_intr");
	ASSERT(curipl() == MIDI_IPL);

	DELAY(1);
	if (!(midi_dev->owner)) {
		printf("spurrious midi recieve interrupt\n");
		ZSWRITE(zsaddr, 1, WR1_EXTIE|(midi_rx->intren?WR1_TXIE:0));
	}

	midi_ilog2("midi_io_recv_intr[%d] msgin = 0x%x msgout = 0x%x\n",
		unit, midi_dev->msgin - midi_dev->msgbuf, 
		midi_dev->msgout - midi_dev->msgbuf, 4, 5);

	/*
	 * Get as much as we can out of the interface.
	 */
	while ((rr0 = zsaddr->zs_ctrl)&RR0_RXAVAIL) {
		DELAY(1);
		ZSREAD(zsaddr, rr1, 1);
		DELAY(1);
		data = zsaddr->zs_data;
		if (rr1&RR1_RXOVER) {
			/* this error is our fault */
			DELAY(1);
			zsaddr->zs_ctrl = WR0_RESET;
			midi_dev->dev_oflow = 1;
			midi_ilog2("midi_io_recv_intr: rxover\n",
				1, 2, 3, 4, 5);
		}
		if(rr1 & RR1_PARITY) {
			DELAY(1);
			zsaddr->zs_ctrl = WR0_RESET;
			continue;	/* parity error - throw it away */
		}
		if(rr1&RR1_FRAME)
			continue;	/* Junk character */

		midi_ilog2("midi_io_recv_intr: received 0x%x\n",
			data, 2, 3, 4, 5);

		/*
		 * Don't receive anything if we're paused.
		 * If our input queue's full, just keep eating input.
		 */
		if (midi_rx->pause || 
		    (MSGBUF_INCR(midi_dev, msgin) == midi_dev->msgout)) {
			midi_ilog("midi_io_recv_intr: continue %s\n",
				midi_rx->pause ? "paused" : "rx buf full",
				2, 3, 4, 5);
			if(!midi_rx->pause) {
				/* this overflow is the caller's fault. We're
				 * out of space to put data.
				 */
#ifdef	CONSOLE_DEBUG
				printf("Midi Buffer Overflow  msgin = "
				    "%XH   msgout = %XH\n",
				    midi_dev->msgin - midi_dev->msgbuf,
				    midi_dev->msgout - midi_dev->msgbuf);
				printf("    callout_pend = %d pinput_pend "
				    "= %d   parsed_input_nelts = %XH\n",
				    midi_dev->callout_pend, 
				    midi_dev->pinput_pend,
				    midi_dev->parsed_input.nelts);
#endif	CONSOLE_DEBUG 
				midi_dev->buf_oflow = 1;
			}
			continue;
		}

		/*
		 * Deal with status bytes.
		 */
		if (MIDI_TYPE_STATUS(data)) {
			/*
			 * SYSTEM messages (other than realtime) clear
			 * running status.
			 */
			if (   MIDI_TYPE_SYSTEM(data)
			    && !MIDI_TYPE_SYSTEM_REALTIME(data))
				midi_dev->run_status.ndata = 0;

			/*
			 * Receiving any status byte terminates
			 * system exclusive message.
			 */
			midi_dev->sys_exclusive = FALSE;

			/*
			 * Stash the data someplace.
			 */
			if (MIDI_TYPE_SYSTEM_EXCL(data)) {
				midi_dev->sys_exclusive = TRUE;
				if ( midi_dev->system_ignores
				    & MIDI_IGNORE_EXCLUSIVE)
				{
					/*
					 * Ignored system exclusive messages
					 * Get put in the running status
					 * buffer so that we can eat the
					 * rest of the bytes in the message.
					 */
					goto running_status;
				} else {
					/*
					 * System exclusive is output in
					 * packed or raw modes only.
					 */
					goto one_byte;
				}
			} else if (   !MIDI_TYPE_1BYTE(data)
				   && (   midi_dev->rcv_mode==MIDI_PROTO_COOKED
				       || (   MIDI_TYPE_SYSTEM(data)
					   && (  MIDI_IGNORE_BIT(data)
					       & midi_dev->system_ignores))))
			{
				/*
				 * Stash 2-3 byte messages in cooked mode
				 * or 2-3 byte SYSTEM messages that we're
				 * ignoring.
				 */
			    running_status:
				midi_dev->run_status.data[0] = data;
				midi_dev->run_status.quanta = event_get();
				midi_dev->run_status.ndata = 1;
				midi_ilog2("midi_io_recv_intr: run_status "
				           "quanta = %d\n",
					   midi_dev->run_status.quanta,
					   2, 3, 4, 5);
			} else if (    MIDI_TYPE_1BYTE(data)
				    && (  MIDI_IGNORE_BIT(data)
					& midi_dev->system_ignores))
			{
				/*
				 * If it's a 1 byte system message that
				 * we're ignoring, through it away.
				 */
				midi_ilog2("midi_io_recv_intr: "
					  "ignore 1byte sys\n",
					  1, 2, 3, 4, 5);
			} else {
				/*
				 * Output the byte directly to the queue.
				 */
				goto one_byte;
			}
		} else if (midi_dev->run_status.ndata > 0) {
			int status = midi_dev->run_status.data[0];

			/*
			 * Third byte in a three-byte message.
			 */
			if (midi_dev->run_status.ndata == 2) {
				/*
				 * Don't output ignored SYSTEM messages.
				 */
				if (   MIDI_TYPE_SYSTEM(status)
				    && (  midi_dev->system_ignores
					& (1<<MIDI_IGNORE_BIT(status))))
				{
					/*
					 * Through away the ignored
					 * SYSTEM message.
					 */
					midi_dev->run_status.ndata = 0;
					midi_ilog2("midi_io_recv_intr: "
						  "ignore 3byte sys\n",
						  1, 2, 3, 4, 5);
				} else {
					/*
					 * Add this message to the output
					 * queue.
					 */
					*midi_dev->msgin = 
						midi_dev->run_status;
					midi_dev->msgin->data[2] = data;
					midi_dev->msgin->ndata = 3;
					midi_dev->msgin = 
						MSGBUF_INCR(midi_dev, msgin);
					midi_dev->run_status.quanta = 0;
					ship_data++;
					/*
					 * Common realtime SYSTEM messages
					 * clear running status.
					 */
					midi_dev->run_status.ndata =
						!MIDI_TYPE_SYSTEM(status);
					midi_ilog2("midi_io_recv_intr: "
						  "3byte data%s\n",
						  midi_dev->run_status.ndata
						  	? " save run_status"
							: " no run_status",
						  2, 3, 4, 5);
				}
			} else {
				/*
				 * 2 or 3 byte cooked messages.
				 *
				 * Get a timestamp if we need a new one.
				 */
				if (midi_dev->run_status.quanta == 0)
					midi_dev->run_status.quanta =
						event_get();

				/*
				 * Second byte in a 2 byte sequence.
				 */
				if (MIDI_TYPE_2BYTE(status))
				{
					if (   MIDI_TYPE_SYSTEM(status)
					    && (  midi_dev->system_ignores
						& MIDI_IGNORE_BIT(status)))
					{
						/*
						 * Ignore this message and
						 * clear running status.
						 */
						midi_dev->run_status.ndata = 0;
						midi_ilog2("midi_io_recv_intr:"
						      " ignore 2byte sys\n",
						      1, 2, 3, 4, 5);
					} else {
					    /*
					     * Add this message to the
					     * output queue.
					     */
					    *midi_dev->msgin = 
					    	midi_dev->run_status;
					    midi_dev->msgin->data[1] = data;
					    midi_dev->msgin->ndata = 2;
					    midi_dev->msgin = 
					    	MSGBUF_INCR(midi_dev, msgin);
					    midi_dev->run_status.quanta= 0;
					    ship_data++;
					    
					    /*
					     * Non-RT SYSTEM messages clear
					     * running status.
					     */
					    midi_dev->run_status.ndata =
						!MIDI_TYPE_SYSTEM(status);
					    midi_ilog2("midi_io_recv_intr:"
						" 2byte data%s\n",
						midi_dev->run_status.ndata
						    ? " save run_status"
						    : " no run_status",
					        2, 3, 4, 5);
					}
				} else  if (MIDI_TYPE_3BYTE(status)) {
					/*
					 * Second byte in a 3 byte sequence.
					 */
					midi_dev->run_status.data[1] = data;
					midi_dev->run_status.ndata = 2;
					midi_ilog2("midi_io_recv_intr: "
					    "2nd of 3bytes\n",
					    1, 2, 3, 4, 5);
					continue;
				} else {
					/*
					 * Lastly, this must be part of a
					 * system exclusive message that we're
					 * ignoring.
					 */
					ASSERT(MIDI_TYPE_SYSTEM_EXCL(status));
				}
			}
		} else {
			/*
			 * We must be in raw or packed mode processing
			 * something other than a a SYSTEM message.
			 */
			if (   midi_dev->rcv_mode == MIDI_PROTO_COOKED
			    && !midi_dev->sys_exclusive)
				continue;	// Bogus byte

		    one_byte:
			midi_ilog2("midi_io_recv_intr: 1byte data/sys\n",
				1, 2, 3, 4, 5);
			midi_dev->msgin->data[0] = data;
			midi_dev->msgin->quanta = event_get();
			midi_dev->msgin->ndata = 1;
			midi_dev->msgin = MSGBUF_INCR(midi_dev, msgin);
			ship_data++;
		}
	} /* while data available */
	
	/*
	 * Send accumulated data (if any)
	 */
	midi_ilog2("midi_io_recv_intr end: msgin = 0x%X\n",
		midi_dev->msgin - midi_dev->msgbuf, 2, 3, 4, 5);
	if (ship_data && !midi_dev->callout_pend) {
		midi_dev->callout_pend = 1;
		softint_sched(CALLOUT_PRI_SOFTINT1,
			midi_io_sched_rcv_time, unit);
	}
	return 0;
} /* midi_io_recv_intr() */

/*
 * handle SCC status interrupts
 */
static int midi_io_status_intr(u_int unit)
{
	volatile struct zsdevice *zsaddr = midi_var.dev[unit].addr;
	int rr0;

	midi_stack_log("midi_io_status_intr");
	ASSERT(curipl() == MIDI_IPL);

	DELAY(1);
	rr0 = zsaddr->zs_ctrl;
	DELAY(1);
	zsaddr->zs_ctrl = WR0_RESET_STAT;
}



