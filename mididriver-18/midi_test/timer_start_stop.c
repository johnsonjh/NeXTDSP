/*
 * Print a message out every n seconds (default 1) based on the midi timer.
 */
#import <mach.h>
#import <stdio.h>
#import <stdlib.h>
#import <mach_error.h>
#import <servers/netname.h>

#import <midi/midi_server.h>
#import <midi/midi_timer.h>
#import <midi/midi_timer_reply_handler.h>
#import <midi/midi_error.h>
#import <midi/midi_timer_error.h>

/*
 * These routines should be prototyped someplace in /usr/include!
 */
int getopt(int argc, char **argv, char *optstring);

void usage(void);

port_t dev_port;
port_t owner_port;
port_t timer_port;
port_t timer_reply_port;
port_t neg_port;
int secs = 1;

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

main(int argc, char **argv)
{
	int i;
	kern_return_t r;
	msg_header_t *in_msg, *out_msg;
	extern char *optarg;
	extern int optind;
	
	while ((i = getopt(argc, argv, "d:")) != EOF)
		switch (i) {
		case 'd':
			secs = atoi(optarg);
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
	 * Find out what time it is (and other vital information).
	 */
	r = port_allocate(task_self(), &timer_reply_port);
	if (r != KERN_SUCCESS) {
		mach_error("allocate timer reply port", r);
		exit(1);
	}

	r = timer_quanta_req(timer_port, timer_reply_port,
		0,	// 0 quanta
		TRUE);	// from now
	if (r != KERN_SUCCESS) {
		midi_timer_error("request timer", r);
		exit(1);
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
	 * Enter the timer loop.
	 */
	in_msg = (msg_header_t *)malloc(MSG_SIZE_MAX);
	out_msg = (msg_header_t *)malloc(MSG_SIZE_MAX);
	while (1) {
		in_msg->msg_size = MSG_SIZE_MAX;
		in_msg->msg_local_port = timer_reply_port;
		
		r = msg_receive(in_msg, RCV_TIMEOUT, 2000);
		if (r == RCV_TIMED_OUT) {
			printf("timed out, start timer\n");
			r = timer_start(timer_port, owner_port);
			if (r != KERN_SUCCESS) {
				midi_error("timer start", r);
				exit(1);
			}
		} else if (r != KERN_SUCCESS) {
			mach_error("msg_receive", r);
			exit(1);
		}

		r = midi_timer_reply_handler(in_msg, &midi_timer_reply);
		if (r != KERN_SUCCESS)
			mach_error("midi_timer_reply_server", r);
	}
}

void usage(void)
{
	fprintf(stderr, "usage: timer_track [-d delay]\n");
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
	printf("time is %d usec/quantum %d\n", quanta, real_usec_per_quantum);

	if (!timer_expired) {
		printf("timer hasn't expired\n");
		return KERN_SUCCESS;
	}

	nquanta += secs * 1000000 / usec_per_quantum;

	r = timer_quanta_req(timer_port, timer_reply_port,
		nquanta,		// secs seconds from
		FALSE);			// from timer base
	if (r != KERN_SUCCESS) {
		midi_timer_error("request timer", r);
		exit(1);
	}

	printf("timeout, stop timer\n");
	r = timer_stop(timer_port, owner_port);
	if (r != KERN_SUCCESS) {
		midi_error("timer stop", r);
		exit(1);
	}

	return KERN_SUCCESS;
}


