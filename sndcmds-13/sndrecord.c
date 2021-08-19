 
/*
 * sndrecord -- record a sound file from either the DSP or the 8khz
 *		codec monitor input.
 *
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY:
 * 11-Jan-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 * modified by dana massie 7-89 to support 24 bit word input
 * from the dsp.
 *	04/09/90/mtm	Cast unix_thread to cthread_fn_t in cthread_fork() call.
 *	04/09/90/mtm	#include <mach_init.h>
 *	08/13/90/mtm	Add -m (emphasized) switch.
 *	09/24/90/mtm	Call SNDAcquire() (bug #8846).
 *
 * to create the dsp file needed for this routine, 
 * asm56000
 * 	asm56000 -a -b -l dsprecsim16.asm
 *	convertlod dsprecsim16.lod     
 *
 *  NOTE!!  FIXME!!  the dsp boot image parsing needs to be fixed.
 *  The released version of the program "convertlod" apparently creates
 * a dsp image that is not correctly read by the dsp boot function here.
 * I have worked around this bug by using an old version of
 * convertlod.  dcm 7-11-89
 */ 

#import <stdlib.h>
#import <cthreads.h>
#import <mach.h>
#import <mach_init.h>
#import <stdio.h>
#import <fcntl.h>
#import <signal.h>
#import <string.h>
#import <mach_error.h>
#import <kern/ipc_ptraps.h>
#import <sys/stat.h>
#import <servers/netname.h>
#import <sound/sound.h>
#import <nextdev/snd_msgs.h>

/*
 * These prototypes are nowhere... lrb
 */
kern_return_t snd_abort_stream (
	port_name_t	stream_port);
kern_return_t snd_pause_stream (
	port_name_t	stream_port);
kern_return_t snd_await_stream (
	port_name_t	stream_port);
kern_return_t snd_resume_stream (
	port_name_t	stream_port);
kern_return_t snd_record (
	port_name_t	stream_port,
	port_name_t	reg_port,
	int		high_water,
	int		low_water,
	int		dma_size,
	int		region_size);
kern_return_t snd_get_stream (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// valid soundout/in/dsp owner port
	port_t		*stream_port,	// returned stream_port
	u_int		stream);		// stream to/from what?
kern_return_t snd_set_dspowner (
	port_t		device_port,		// valid device port
	port_t		owner_port,		// dsp owner port
	port_t		*neg_port);		// dsp negotiation port
kern_return_t snd_set_sndinowner (
	port_t		device_port,		// valid device port
	port_t		owner_port,		// sound in owner port
	port_t		*neg_port);		// sound in negotiation port
kern_return_t snd_dsp_proto (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// port registered as owner
	int		proto);		// volume on right channel	
kern_return_t snd_get_dsp_cmd_port (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// valid owner port
	port_t		*cmd_port);	// returned cmd_port
kern_return_t snd_dspcmd_chandata (
	port_t		cmd_port,	// valid dsp command port
	int		addr,		// .. of dsp buffer
	int		size,		// .. of dsp buffer
	int		skip,		// dma skip factor
	int		space,		// dsp space of buffer
	int		mode,		// mode of dma [1..5]
	int		chan);		// channel for dma
kern_return_t snd_dsp_boot (
	port_name_t	cmd_port,
	int		*boot_code,
	int		boot_code_size);
void snd_error(char *string, int error);


/*
 * These routines should be prototyped someplace in /usr/include!
 */
int getopt(int argc, char **argv, char *optstring);
int open(char *path, int flags, int mode);
int close(int fd);
int read(int fd, char *buf, int nbytes);
int write(int fd, char *buf, int nbytes);
int ftruncate(int fd, off_t length);
off_t lseek(int fd, off_t offset, int whence);
int kill(int pid, int sig);
int getsectdata();

/*
 * Routines declaired locally.
 */
void usage(void);
void sound_init(void);
void unix_thread(port_name_t port);
void update_stream(void);
void sigint(void);
void sigstop(void);
void sigcont(void);

/*
 * Globals:
 */
int		max_filesize = -1;
int		high_water, low_water;
int		region_size, dma_size;
int		nbytes_read;
int		nbytes_enqueued;
int		soundFileFD, commentFileFD;
boolean_t 	wants_overflow = FALSE;
boolean_t 	auto_record = FALSE;
port_name_t	dev_port;
port_name_t	owner_port;
port_name_t	stream_port;
port_name_t	reg_port;
port_name_t	cmd_port;
char		*dspprog;
SNDSoundStruct	*dspSNDStruct;
SNDSoundStruct	*recordedSNDStruct;

int		default_dsp;

/*
 * Constants
 */
#define DEF_HIGH_WATER(is_codec)	(  (is_codec) \
					 ? 8*8192 \
					 : (512+256)*1024)
#define DEF_LOW_WATER(is_codec)		(  (is_codec) \
					 ? 6*8192 \
					 : (512)*1024)
#define DEF_REGION_SIZE(is_codec)	(DEF_HIGH_WATER(is_codec))
#define DEF_DMA_SIZE(is_codec)		((is_codec) ? 256 : vm_page_size)

#define STD16_DSPRECORD "dsprecsim16.snd"
#define STD24_DSPRECORD "dsprecsim24.snd"

#define START_RECORDING	1000
#define STOP_RECORDING	1001

int ensure_extension(char *buf, char *path, char *extension)
{
    char *p;
    int plen = strlen(path), elen = strlen(extension);
    strcpy(buf,path);
    p = rindex(buf,extension[0]);
    if ((plen < elen) || !p)
	strcat(buf,extension);
    else if (p) {
	if (strcmp(p,extension)) return 0;
	else return 1;
    } else
	return 1;
}

/*
 * main
 * Parse input arguments, open and initialize connection to sound device,
 * create output sound file, setup to receive suspend and interrupt signals,
 * start the recording, wait for completion.
 */
main(ac, av)
	int ac;
	char **av;
{
	register int r, size, verbose;
	msg_header_t *msg = (msg_header_t *)malloc(MSG_SIZE_MAX);
	port_set_name_t port_set;
	port_name_t unix_port;
	char *comment = "";
	struct stat statb;
	extern char *optarg;
	extern int optind;
	char buf[1024];
	int WaitingForAbort = FALSE;
	int format, emphasized = FALSE;
	port_t dummy_port;

	if (ac < 2) {
	    usage();
	    exit(1);
	}
	
	/*
	 * Collect input arguments.
	 */
	while ((r = getopt(ac, av, "edmf:s:wh:l:D:c:C:v")) != EOF)
		switch (r) {
		case 'd':
			/*
			 * Record from the dsp.  STD16_DSPRECORD is the
			 * name of the file which is run in the DSP.
			 * It is assumed that the output from the
			 * DSP is 16bit 44KHZ stereo.
			 */
			dspprog = STD16_DSPRECORD;
			default_dsp = 1;
			break;
		case 'e':
			/*
			 * Record from the dsp.  STD24_DSPRECORD is the
			 * name of the file which is run in the DSP.
			 * This dsp file reads 24 bit data from the dsp.
			 * It also echos the dsp data out the dsp port.
			 * It is assumed that the output from the
			 * DSP is 16bit 44KHZ stereo.
			 */
			dspprog = STD24_DSPRECORD;
			default_dsp = 1;
			break;
		case 'm':
			emphasized = 1;
			break;
		case 'f':
			/*
			 * Record from the dsp.  Optarg is the
			 * name of the file which is run in the DSP.
			 * It is assumed that the output from the
			 * DSP is 16bit 44KHZ stereo.
			 */
			dspprog = optarg;
			default_dsp = 0;
			break;
		case 's':
			/*
			 * Record at most optarg bytes into the file
			 * (default MAXINT).
			 */
			max_filesize = atoi(optarg);

			/*
			 * Don't ask the user to start and stop recording
			 * through stdio, go until the size is reached
			 * and then terminate.
			 */
			auto_record = TRUE;
			break;
		case 'h':
			/*
			 * Set the high water mark used in maintaining
			 * locked-down physical memory within the driver.
			 */
			high_water = atoi(optarg);
			break;
		case 'l':
			/*
			 * Set the low water mark used in maintaining
			 * locked-down physical memory within the driver.
			 */
			low_water = atoi(optarg);
			break;
		case 'D':
			/*
			 * Set the low water mark used in maintaining
			 * locked-down physical memory within the driver.
			 */
			dma_size = atoi(optarg);
			break;
		case 'w':
			/*
			 * Report when an overflow condition is seen.
			 */
			wants_overflow = TRUE;
			break;
		case 'c':
			/*
			 * Get a comment from the argument list.
			 */
			comment = optarg;
			break;
		case 'C':
			/*
			 * Get a comment from the specified file.
			 */
			commentFileFD = open(optarg, O_RDONLY, 0);
			if (commentFileFD < 0) {
				fprintf(stderr, "sndrecord: can't open "
					"comment file %s\n", optind);
//				usage();
				exit(1);
			}
			fstat(commentFileFD, &statb);
			comment = (char *)malloc(statb.st_size+1);
			read(commentFileFD, comment, statb.st_size);
			comment[statb.st_size] = '\0';
			close(commentFileFD);
			break;
		case 'v':
			verbose++;
			break;
		case '?':
		default:
			usage();
			exit(1);
		}

	/*
	 * Look at the rest of the arguments to satisfy any comment information
	 * and make sure that we can record into the sound file.
	 */
	if (ac != optind+1) {
		fprintf(stderr, "sndrecord: no output sound file\n");
//		usage();
		exit(1);
	}
	if (!ensure_extension(buf,av[optind],".snd")) {
		fprintf(stderr, "sndrecord: "
				"output file of wrong type\n");
		exit(1);
	}
	soundFileFD = open(buf, O_WRONLY|O_TRUNC|O_CREAT, 0666);
	if (soundFileFD == -1) {
		fprintf(stderr, "sndrecord: can't open output file %s\n", buf);
//		usage();
		exit(1);
	}
	optind++;

	/*
	 * Find out how much storage space we need for the sound header.
	 * We try to keep it a multiple of 128 bytes for proper alignment
	 * of data.
	 */
	size = sizeof(*recordedSNDStruct) - sizeof(recordedSNDStruct->info);
	size += strlen(comment)+1;

	if (size % 128)
		size += (128-(size%128));

	if (dspprog) {
	    if (emphasized)
		format = SND_FORMAT_EMPHASIZED;
	    else
		format = SND_FORMAT_LINEAR_16;
	} else
	    format = SND_FORMAT_MULAW_8;

	r = SNDAlloc(&recordedSNDStruct,
		     0,			// dataSize unknown at this point
		     format,
		     dspprog?SND_RATE_HIGH:SND_RATE_CODEC,
		     dspprog?2:1,	// stereo or mono
		     size - (  sizeof(*recordedSNDStruct)
			     - sizeof(recordedSNDStruct->info)));
	if (r) {
		fprintf(stderr,
			"sndrecord: couldn't allocate sound struct (%s)",
			SNDSoundError(r));
		exit(1);
	}

	/*
	 * Add comments to header.
	 */
	strcpy(recordedSNDStruct->info, comment);

	/*
	 * Save room for the sound structure at the beginning of the file.
	 */
	r = ftruncate(soundFileFD, size);
	if (r) {
		perror("sndrecord: couldn't save space for header");
		exit(1);
	}

	/*
	 * Set default sizes if not specified.
	 */
	if (!high_water) high_water = DEF_HIGH_WATER(!dspprog);
	if (!low_water) low_water = DEF_LOW_WATER(!dspprog);
	if (!dma_size) dma_size = DEF_DMA_SIZE(!dspprog);
	if (!region_size) region_size = DEF_REGION_SIZE(!dspprog);

	/*
	 * Get any info we need for recording from the DSP.
	 */
	if (dspprog) {
		register char *s;

		/*
		 * Read the dsp sound file (lod file) if we're going to
		 * record through the DSP.
		 */
		s = strrchr(dspprog, '.');
		if (default_dsp) {
		    int foo;
		    dspSNDStruct = (SNDSoundStruct *)getsectdata("__SND", 
		    						dspprog,
								 &foo);
		    if (!dspSNDStruct) {
			fprintf(stderr, "sndrecord: unknown error\n");
			exit(1);
		    }
		} else {
		    if (s && !strcmp(s, ".lod"))
			r = SNDReadDSPfile(dspprog, &dspSNDStruct, 0);
		    else if (s && !strcmp(s, ".snd"))
		        r = SNDReadSoundfile(dspprog, &dspSNDStruct);
		    else
			r = SND_ERR_BAD_FILENAME;
    
		    if (r) {
			fprintf(stderr, "sndrecord: bad dsp file %s (%s)\n",
				    dspprog, SNDSoundError(r));
			    exit(1);
		    }
		}
	}

	/*
	 * Get a connection to the sound server.  Obtain whatever ownership
	 * is necessary to do the recording.
	 */
	r = SNDAcquire(0, 0, 0, -1, NULL_NEGOTIATION_FUN, 0, &dev_port, &dummy_port);
	if (r != SND_ERR_NONE) {
	    fprintf(stderr, "sndrecord: %s\n", SNDSoundError(r));
	    exit(1);
	}

	/*
	 * Initialize the driver
	 */
	sound_init();

	/*
	 * Receive on the following ports:
	 *	reg_port	-- completion messages/overflow from stream
	 *	owner_port	-- start/stop indication
	 */
	r = port_set_allocate(task_self(), &port_set);
	if (r != KERN_SUCCESS) {
		mach_error("sndrecord: can't allocate port set", r);
		exit(1);
	}
	r = port_set_add(task_self(), port_set, reg_port);
	if (r != KERN_SUCCESS) {
		mach_error("sndrecord: can't add port to port set", r);
		exit(1);
	}
	r = port_set_add(task_self(), port_set, owner_port);
	if (r != KERN_SUCCESS) {
		mach_error("sndrecord: can't add port to port set", r);
		exit(1);
	}
	
	/*
	 * Catch SIGTSTP, and SIGSTOP to pause the recording.
	 * Catch SIGCONT to continue the recording.
	 */
	signal(SIGTSTP, (void (*)())sigstop);
	signal(SIGSTOP, (void (*)())sigstop);
	signal(SIGCONT, (void (*)())sigcont);

	/*
	 * If we're using stdin to start/stop recording, launch the thread
	 * used to read stdin and send us messages to terminate.  Also, arm
	 * an interrupt handler for SIGINT to terminate the recording.
	 * Wait for the message from the thread before continuing.
	 */
	if (!auto_record) {
		port_allocate(task_self(), &unix_port);
		cthread_init();
		cthread_set_name(cthread_self(), "mach thread");
		cthread_detach(cthread_fork((cthread_fn_t)unix_thread,
			      (any_t)unix_port));
		msg->msg_size = MSG_SIZE_MAX;
		msg->msg_local_port = unix_port;
		r = msg_receive(msg, MSG_OPTION_NONE, 0);
		if (r != KERN_SUCCESS) {
			mach_error("sndrecord: bad start msg from thread", r);
			exit(1);
		}
		port_set_add(task_self(), port_set, unix_port);
	}
	signal(SIGINT, (void (*)())sigint);

	/*
	 * Start the recording process by unpausing the stream.
	 */
	r = snd_resume_stream(stream_port);
	if (r != KERN_SUCCESS) {
		snd_error("sndrecord: can't get resume stream", r);
		exit(1);
	}

	/*
	 * DSP recording needs to overlap DSP setup with record setup.
	 * Sending down the region before the DSP code starts up helps
	 * ensure that buffers are available to the DSP by the time the
	 * DSP has data to send to the CPU.
	 */
	if (dspprog) {
		/*
		 * Get a boot image and send it down.
		 */
		r = snd_dsp_boot(cmd_port,
			(int *)((int)dspSNDStruct+dspSNDStruct->dataLocation),
				 dspSNDStruct->dataSize/sizeof(int));
		if (r != KERN_SUCCESS) {
			snd_error("sndrecord: can't boot DSP", r);
			exit(1);
		}
	}

	/*
	 * Loop recieving messages:
	 *	If Received data, write it out and send another await msg.
	 *	If Received stop-recording msg (from thread or hdlr),
	 *		abort the stream and exit after collecting final
	 *		data from stream.
	 *	If we've completed a region in the stream, add another
	 *		to the queue.
	 *	If we Receive an overflow message, note that, if requested.
	 *	If we've collected all the data we need exit.
	 */
	while (TRUE) {
		snd_recorded_data_t *rd;

		msg->msg_local_port = port_set;
		msg->msg_size = MSG_SIZE_MAX;
		r = msg_receive(msg, RCV_TIMEOUT, 500);

		if (r == RCV_TIMED_OUT) {
			if (WaitingForAbort == TRUE)
				goto AbortTimedOut;
			snd_await_stream(stream_port);	// missed complete?
			continue;
		} else  if (r != KERN_SUCCESS)
		    break;
		/*
		 * Process the message.
		 */
		switch (msg->msg_id) {
		case SND_MSG_OVERFLOW:
			if (verbose) {
				printf("<ovf>");
				fflush(stdout);
			}
			if (wants_overflow == TRUE)
				printf("overflow at byte %d\n", nbytes_read);
			break;
		case SND_MSG_RECORDED_DATA:
			rd = (snd_recorded_data_t *)msg;
			if (verbose) {
				printf("<%d>",
					rd->dataType.msg_type_long_number);
				fflush(stdout);
			}
			r = write(soundFileFD, (char *)rd->recorded_data,
				rd->dataType.msg_type_long_number);
			if (r < 0) {
				perror("sndrecord: can't write data");
				exit(1);
			}
			r = vm_deallocate(task_self(),
				(vm_address_t)rd->recorded_data,
				(vm_size_t)rd->dataType.msg_type_long_number);
			if (r != KERN_SUCCESS) {
				mach_error("sndrecord: deallocate failed", r);
				exit(1);
			}
			nbytes_read += rd->dataType.msg_type_long_number;
			if (max_filesize > 0 && nbytes_read >= max_filesize) {
				snd_abort_stream(stream_port);
				goto done;
			}
			update_stream();
			break;
		case STOP_RECORDING:
			if (verbose) {
				printf("<stop>");
				fflush(stdout);
			}
			WaitingForAbort = TRUE;
			snd_abort_stream(stream_port);
			break;
		case SND_MSG_ABORTED:
		    AbortTimedOut:
			if (verbose) {
				printf("<abrt>");
				fflush(stdout);
			}
			max_filesize = nbytes_read;
			WaitingForAbort = FALSE;
			goto done;
		default:
			fprintf(stderr, "sndrecord: Received unknown message "
				"id (%d)\n", msg->msg_id);
			exit(1);
		}
	}
	if (r != KERN_SUCCESS) {
		mach_error("sndrecord: msg Receive failure", r);
		exit(1);
	}

    done:
	/*
	 * Update and write out the soundfile header.
	 */
	printf("%d bytes read\n", max_filesize);
	recordedSNDStruct->dataSize = max_filesize;
	r = lseek(soundFileFD, 0, SEEK_SET);
	if (r) {
		perror("sndrecord: can't seek to beginning of file");
		exit(1);
	}
	r = write(soundFileFD, (char *)recordedSNDStruct, size);
	if (r < 0) {
		perror("sndrecord: can't write file header");
		exit(1);
	}
	exit(0);
}

void usage(void)
{
	fprintf(stderr, "usage : sndrecord\n"
			"\t[-d] (reads 16 bit dsp data to sndfile)\n"
			"\t[-e] (reads 24 bit dsp data to 16 bit sndfile)\n"
		        "\t[-m] (dsp data is emphasized)\n"
			"\t[-w] (report overflow condition)\n"
			"\t[-f <DSP program file>]\n"
			"\t[-h <high water mark>]\n"
			"\t[-l <low water mark>]\n"
			"\t[-D <dma size>]\n"
			"\t[-c comment]\n"
			"\t[-C <comment file>]\n"
			"\tfile\n");
}

void sound_init(void)
{
	int r;

	/*
	 * Allocate the port we'll use to register as owner.
	 */
	r = port_allocate(task_self(), &owner_port);
	if (r != KERN_SUCCESS) {
		mach_error("sndrecord: port_allocate error", r);
		exit(1);
	}

	/*
	 * Aquire the device.
	 */
	if (dspprog) {
		port_t temp = owner_port;
		r = snd_set_dspowner(dev_port, owner_port, &temp);
	} else {
		port_t temp = owner_port;
		r = snd_set_sndinowner(dev_port, owner_port, &temp);
	}
	if (r != KERN_SUCCESS) {
		snd_error("sndrecord: set owner error", r);
		exit(1);
	}

	/*
	 * Use a special port for recieving messages from the record stream.
	 */
	r = port_allocate(task_self(), &reg_port);
	if (r != KERN_SUCCESS) {
		mach_error("port_allocate", r);
		exit(1);
	}

	/*
	 * Set up the DSP (if necessary).
	 */
	if (dspprog) {
		/*
		 * Get the DSP command port.
		 */
		r = snd_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
		if (r != KERN_SUCCESS) {
			snd_error("sndrecord: snd_get_dsp_cmd_port", r);
			exit(1);
		}
	
		r = snd_dspcmd_chandata(cmd_port,
			0,
			dma_size/sizeof(short),
			0,
			0,
			sizeof(short),
			DSP_SO_CHAN);
		if (r != KERN_SUCCESS) {
			snd_error("sndrecord: snd_dspcmd_chan_data", r);
			exit(1);
		}
	
		/*
		 * Initialize driver state.
		 */
		r = snd_dsp_proto(dev_port, owner_port, SND_DSP_PROTO_S_DMA);
		if (r != KERN_SUCCESS) {
			snd_error("sndrecord: snd_dspcmd_chan_data", r);
			exit(1);
		}
	}

	/*
	 * Get the port for talking to the stream.  Pause it so that
	 * we'll only start when we're ready.
	 */
	r = snd_get_stream(dev_port, owner_port, &stream_port,
		dspprog?SND_GD_DSP_OUT:SND_GD_SIN);
	if (r != KERN_SUCCESS) {
		snd_error("sndrecord: can't get record stream port", r);
		exit(1);
	}
	r = snd_pause_stream(stream_port);
	if (r != KERN_SUCCESS) {
		snd_error("sndrecord: can't get pause stream", r);
		exit(1);
	}
	
	/*
	 * Start up the sound stream.
	 */
	update_stream();
}

/*
 * Send as many regions down to the stream as we need to have an
 * adaquate backlog of memory to record into.
 */
void update_stream(void)
{
	int deficit = nbytes_enqueued-nbytes_read-region_size;
	int r;

	while (deficit <= 0) {
		r = snd_record(stream_port, reg_port,
				high_water,
				low_water,
				dma_size,
				region_size);
		if (r != KERN_SUCCESS) {
			snd_error("sndrecord: snd_record failed", r);
			exit(1);
		}
		deficit += region_size;
		nbytes_enqueued += region_size;
	}

	/* FIXME:
	 * If we just crossed over a region boundary we have an extra
	 * await hanging out there.
	 */ 
	snd_await_stream(stream_port);
}

/*
 * Wait for carriage returns (bleh)
 */
void unix_thread(port_name_t port)
{
	msg_header_t rmsg;
	char buf[20];

	cthread_set_name(cthread_self(), "unix thread");

	rmsg.msg_id = START_RECORDING;
	rmsg.msg_size = sizeof(rmsg);
	rmsg.msg_local_port = PORT_NULL;
	rmsg.msg_remote_port = port;
	rmsg.msg_simple = TRUE;

	/*
	 * After the first carraige return, send the START_RECORDING msg.
	 */
	printf("type carriage return to start recording\n");
	read(0, buf, sizeof(buf));
	msg_send(&rmsg, MSG_OPTION_NONE, 0);

	rmsg.msg_id = STOP_RECORDING;

	/*
	 * After the second carriage return, send the STOP_RECORDING msg.
	 */
	printf("type carriage return or cntl-C to stop recording\n");
	read(0, buf, sizeof(buf));
	msg_send(&rmsg, MSG_OPTION_NONE, 0);
	
	cthread_exit(0);
}

void sigint(void)
{
	msg_header_t rmsg;

	signal(SIGINT, SIG_DFL);
	rmsg.msg_id = STOP_RECORDING;
	rmsg.msg_size = sizeof(rmsg);
	rmsg.msg_local_port = PORT_NULL;
	rmsg.msg_remote_port = owner_port;
	rmsg.msg_simple = TRUE;
	msg_send(&rmsg, MSG_OPTION_NONE, 0);
}

void sigstop(void)
{
	int r;
	
	signal(SIGTSTP, SIG_DFL);
	signal(SIGSTOP, SIG_DFL);
	r = snd_pause_stream(stream_port);
	if (r != KERN_SUCCESS) {
		snd_error("sndrecord: can't get pause stream", r);
		exit(1);
	}
	kill(0, SIGSTOP);
}

void sigcont(void)
{
	int r;
	
	signal(SIGTSTP, (void (*)())sigstop);
	signal(SIGSTOP, (void (*)())sigstop);
	r = snd_resume_stream(stream_port);
	if (r != KERN_SUCCESS) {
		snd_error("sndrecord: can't get resume stream", r);
		exit(1);
	}
}
