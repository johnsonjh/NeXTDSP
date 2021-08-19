/*
 * Send a bunch of data to MIDI out.
 */
#import <mach.h>
#import <stdio.h>
#import <stdlib.h>
#import <fcntl.h>
#import <mach_error.h>
#import <servers/netname.h>

#import <midi/midi_server.h>
#import <midi/midi_reply_handler.h>
#import <midi/midi_timer.h>
#import <midi/midi_timer_reply_handler.h>
#import <midi/midi_error.h>
#import <midi/midi_timer_error.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

/*
 * These routines should be prototyped someplace in /usr/include!
 */
int getopt(int argc, char **argv, char *optstring);
int read(int fd, char *data, int size);
int open(char *file, int options, int mode);

void usage(void);
void msg_rcv_loop(void);

port_t dev_port;
port_t owner_port;
port_t timer_port;
port_t timer_reply_port;
port_t xmit_port;
port_t xmit_reply_port;
port_t neg_port;
port_set_name_t port_set;
int secs = 1;
int verbose;
int data_format = MIDI_PROTO_RAW;
int MIDIfd;
int exit_rcv_loop;
u_int queue_max = max(MIDI_COOKED_DATA_MAX, MIDI_RAW_DATA_MAX)*2;

kern_return_t my_timer_event (
	void *arg,
	timeval_t timeval,
	u_int quanta,
	u_int usec_per_quantum,
	u_int real_usec_per_quantum,
	boolean_t timer_expired,
	boolean_t timer_stopped,
	boolean_t timer_forward);

midi_timer_reply_t midi_timer_reply = {
	my_timer_event,
	0,
	0
};

kern_return_t my_queue_notify(
	void *		arg,
	u_int		queue_size);

midi_reply_t midi_reply = {
	0,
	0,
	0,
	my_queue_notify,
	0,
	0
};

main(int argc, char **argv)
{
	int i;
	kern_return_t r;
	boolean_t timer_too = FALSE;
	extern char *optarg;
	extern int optind;
	
	while ((i = getopt(argc, argv, "crvt")) != EOF)
		switch (i) {
		case 'r':
			data_format = MIDI_PROTO_RAW;
			break;
		case 'c':
			data_format = MIDI_PROTO_COOKED;
			break;
		case 'v':
			verbose++;
			break;
		case 't':
			timer_too = TRUE;
			break;
		case '?':
		default:
			usage();
			exit(1);
		}

	/*
	 * Get a connection to the midi driver.
	 */
	r = netname_look_up(name_server_port, "", "midi1", &dev_port);
	if (r != KERN_SUCCESS) {
		mach_error("timer_track: netname_look_up error", r);
		exit(1);
	}

	/*
	 * Become owner of the device.
	 */
	r = port_allocate(task_self(), &owner_port);
	if (r != KERN_SUCCESS) {
		mach_error("allocate owner port", r);
		exit(1);
	}

	neg_port = PORT_NULL;
	r = midi_set_owner(dev_port, owner_port, &neg_port);
	if (r != KERN_SUCCESS) {
		midi_error("become owner", r);
		exit(1);
	}

	/*
	 * Get the timer port for the device.
	 */
	r = midi_get_out_timer_port(dev_port, &timer_port);
	if (r != KERN_SUCCESS) {
		midi_error("output timer port", r);
		exit(1);
	}

	/*
	 * Get the transmit port for the device.
	 */
	r = midi_get_xmit(dev_port, owner_port, &xmit_port);
	if (r != KERN_SUCCESS) {
		midi_error("recv port", r);
		exit(1);
	}

	r = port_allocate(task_self(), &timer_reply_port);
	if (r != KERN_SUCCESS) {
		mach_error("allocate timer reply port", r);
		exit(1);
	}

	/*
	 * Find out what time it is (and other vital information).
	 */
	r = port_allocate(task_self(), &xmit_reply_port);
	if (r != KERN_SUCCESS) {
		mach_error("allocate timer reply port", r);
		exit(1);
	}

	/*
	 * Set the protocol to indicate our preferences.
	 */
	r = midi_set_proto(xmit_port,
		data_format,		// raw, cooked, or packed
		FALSE,			// absolute time codes wanted
		MIDI_PROTO_SYNC_SYS,	// use system clock
		10,			// 10 clocks before data sent
		2,			// 2 clock timeout between input chars
		queue_max);		// maximum output queue size
	if (r != KERN_SUCCESS) {
		mach_error("midi_set_proto", r);
		exit(1);
	}

	/*
	 * Allocate port set.
	 */
	r = port_set_allocate(task_self(), &port_set);
	if (r != KERN_SUCCESS) {
		mach_error("allocate port set", r);
		exit(1);
	}

	/*
	 * Add timer receive port to port set.
	 */
	r = port_set_add(task_self(), port_set, timer_reply_port);
	if (r != KERN_SUCCESS) {
		mach_error("add timer_reply_port to set", r);
		exit(1);
	}

	/*
	 * Add driver reply port to port set.
	 */
	r = port_set_add(task_self(), port_set, xmit_reply_port);
	if (r != KERN_SUCCESS) {
		mach_error("add xmit_reply_port to set", r);
		exit(1);
	}

	if (timer_too) {
		r = timer_quanta_req(timer_port, timer_reply_port,
			0,	// 0 quanta
			TRUE);	// from now
		if (r != KERN_SUCCESS) {
			midi_timer_error("request timer", r);
			exit(1);
		}
	}

	/*
	 * Start the timer up.
	 */
	r = timer_start(timer_port, owner_port);
	if (r != KERN_SUCCESS) {
		midi_error("timer start", r);
		exit(1);
	}

	/*
	 * For each MIDI file (or stdin, if no files) reset ownership
	 * (to clear everything out), call the queue_notify proc to
	 * startup (and continue, without blocking) writing data down.
	 * Afterwards place a request in for notification when the output
	 * queue size is zero, so we don't terminate too soon.
	 */
	if (optind >= argc) {
		MIDIfd = 0;
		goto start_file;
	} else for (; optind < argc; optind++) {
		MIDIfd = open(argv[optind], O_RDONLY, 0);
		if (MIDIfd < 0) {
			perror("open");
			exit(1);
		}

	    start_file:
		/*
		 * Send a message requesting a message when the
		 * queue size is zero (as it should be now).
		 */
		r = midi_output_queue_notify(dev_port, owner_port,
			xmit_reply_port, 0);
		if (r != KERN_SUCCESS) {
			midi_error("await queue length zero", r);
			exit(1);
		}

		/*
		 * Enter the receive loop.  my_queue_notify() will set
		 * exit_rcv_loop to TRUE when all data has been output
		 * from the file (EOF is seen).  This will cause the
		 * receive loop to exit.
		 */	
		exit_rcv_loop = FALSE;
		msg_rcv_loop();

		/*
		 * Do the same thing again, so that we can wait for our
		 * output to drain.
		 */
		r = midi_output_queue_notify(dev_port, owner_port,
			xmit_reply_port, 0);
		if (r != KERN_SUCCESS) {
			midi_error("await queue length zero", r);
			exit(1);
		}
		exit_rcv_loop = FALSE;
		msg_rcv_loop();
	}
	exit(0);
}

#define IN_SIZE max(MIDI_TIMER_REPLY_INMSG_SIZE, MIDI_REPLY_INMSG_SIZE)
#define OUT_SIZE max(MIDI_TIMER_REPLY_OUTMSG_SIZE, MIDI_REPLY_OUTMSG_SIZE)

void msg_rcv_loop(void)
{
	int i;
	kern_return_t r;
	msg_header_t *in_msg, *out_msg;

	/*
	 * Call my_queue_notify directly to get the output started.
	 */
	in_msg = (msg_header_t *)malloc(IN_SIZE);
	out_msg = (msg_header_t *)malloc(OUT_SIZE);
	while (!exit_rcv_loop) {
		in_msg->msg_size = IN_SIZE;
		in_msg->msg_local_port = port_set;
		
		r = msg_receive(in_msg, MSG_OPTION_NONE, 0);
		if (r != KERN_SUCCESS) {
			mach_error("msg_receive", r);
			exit(1);
		}

		if (in_msg->msg_local_port == xmit_reply_port)
			r = midi_reply_handler(in_msg,
				&midi_reply);
		else if (in_msg->msg_local_port == timer_reply_port)
			r = midi_timer_reply_handler(in_msg,
				&midi_timer_reply);
		else {
			fprintf(stderr, "unknown port\n");
			r = KERN_FAILURE;
		}
		if (r != KERN_SUCCESS)
			mach_error("midi_timer_reply_server", r);
	}

	free((char *)in_msg);
	free((char *)out_msg);
}

void usage(void)
{
	fprintf(stderr,
		"usage: midi_write [-c (cooked) | -r (raw)] "
		"[-t (show time)] [file ...]\n");
}

kern_return_t my_timer_event (
	void *arg,
	timeval_t timeval,
	u_int quanta,
	u_int usec_per_quantum,
	u_int real_usec_per_quantum,
	boolean_t timer_expired,
	boolean_t timer_stopped,
	boolean_t timer_forward)
{
	kern_return_t r;
	static int nquanta;
	nquanta += secs * 1000000 / usec_per_quantum;
	printf("time is %d usec/quantum %d\n", quanta, real_usec_per_quantum);

	if (!timer_expired) {
		printf("timer hasn't expired\n");
		return KERN_SUCCESS;
	}

	r = timer_quanta_req(timer_port, timer_reply_port,
		nquanta,		// secs seconds from
		FALSE);			// from timer base
	if (r != KERN_SUCCESS) {
		midi_timer_error("request timer", r);
		exit(1);
	}

	return KERN_SUCCESS;
}

kern_return_t my_queue_notify(
	void *		arg,
	u_int		queue_size)
{
	int n;
	kern_return_t r;
	boolean_t will_block = FALSE;
	union {
		midi_raw_data_t		raw[MIDI_RAW_DATA_MAX];
		midi_cooked_data_t	cooked[MIDI_COOKED_DATA_MAX];
	} data;

	/*
	 * Loop through the MIDI data, until we would block, sending at most
	 * MIDI_{COOKED,RAW}_DATA_MAX messages in a single call.
	 */
	while (!will_block) {
		switch (data_format) {
		case MIDI_PROTO_RAW:
			n = read(MIDIfd, (char *)data.raw, sizeof(data.raw));
			if (n < 0) {
				perror("read");
				exit(1);
			}
			if (n == 0) {
				exit_rcv_loop = TRUE;
				return KERN_SUCCESS;
			}

			/*
			 * Send the data.
			 */
			r = midi_send_raw_data(xmit_port, data.raw,
				n / sizeof(data.raw[0]), TRUE);
			if (r == MIDI_WILL_BLOCK) {
				will_block = TRUE;
			} else if (r != KERN_SUCCESS) {
				midi_error("midi_send_raw_data", r);
				exit(1);
			}
			break;

		case MIDI_PROTO_COOKED:
			n = read(MIDIfd, (char *)data.cooked,
				sizeof(data.cooked));
			if (n < 0) {
				perror("read");
				exit(1);
			}
			if (n == 0) {
				exit_rcv_loop = TRUE;
				return KERN_SUCCESS;
			}

			/*
			 * Send the data.
			 */
			r = midi_send_cooked_data(xmit_port, data.cooked,
				n / sizeof(data.cooked[0]), TRUE);
			if (r == MIDI_WILL_BLOCK) {
				will_block = TRUE;
			} else if (r != KERN_SUCCESS) {
				midi_error("midi_send_cooked_data", r);
				exit(1);
			}
			break;
		}
	}

	/*
	 * Get a message when the queue size is down
	 * to half of it's max.  Exit to await the message
	 * so that we can start up again.
	 */
	r = midi_output_queue_notify(dev_port, owner_port,
		xmit_reply_port, queue_max/2);
	if (r != KERN_SUCCESS) {
		midi_error("midi_output_queue_notify", r);
		exit(1);
	}

	return KERN_SUCCESS;
}

