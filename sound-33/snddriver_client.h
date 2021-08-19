/*
 *	snddriver_client.h
 *	Copyright 1990 NeXT, Inc.
 *
 *	This file gets #import'ed by sounddriver.h.
 */
 
kern_return_t snddriver_get_device_parms (
	port_t		device_port,		// valid device port
	boolean_t	*speaker,		// returned speaker enable 
	boolean_t	*lowpass,		// returned lowpass filter en.
	boolean_t	*zerofill);

kern_return_t snddriver_set_device_parms (
	port_t		device_port,		// valid device port
	boolean_t	speaker,		// enable speaker	
	boolean_t	lowpass,		// enable lowpass filter
	boolean_t	zerofill);

/* new in 2.0 */
kern_return_t snddriver_set_ramp (
	port_t		device_port,		// valid device port
	int		rampflags);		// Flags for setting ramp

kern_return_t snddriver_get_volume (
	port_t		device_port,		// valid device port
	int		*left_chan,		// returned volume
	int		*right_chan);

kern_return_t snddriver_set_volume (
	port_t		device_port,		// valid device port
	int		left_chan,		// volume on left channel
	int		right_chan);

kern_return_t snddriver_set_dsp_owner_port (
	port_t		device_port,		// valid device port
	port_t		owner_port,		// dsp owner port
	port_t		*neg_port);

kern_return_t snddriver_set_sndin_owner_port (
	port_t		device_port,		// valid device port
	port_t		owner_port,		// sound in owner port
	port_t		*neg_port);

kern_return_t snddriver_set_sndout_owner_port (
	port_t		device_port,		// valid device port
	port_t		owner_port,		// sound out owner port
	port_t		*neg_port);

kern_return_t snddriver_get_dsp_cmd_port (
	port_t		device_port,		// valid device port
	port_t		owner_port,		// valid owner port
	port_t		*cmd_port);

kern_return_t snddriver_dspcmd_req_msg (
	port_t		cmd_port,		// valid dsp command port
	port_t		reply_port);

kern_return_t snddriver_dspcmd_req_err (
	port_t		cmd_port,		// valid dsp command port
	port_t		reply_port);

kern_return_t snddriver_dspcmd_req_condition (	//?
	port_t		cmd_port,		// valid dsp command port
	u_int		mask,			// mask of flags in condition
	u_int		flags,			// value of flags in condition
	int		priority,		// priority of this transaction
	port_t		reply_port);

kern_return_t snddriver_dsp_set_flags (
	port_t		cmd_port,		// valid dsp command port
	u_int		mask,			// mask of flags to affect
	u_int		flags,			// values of affected flags
	int		priority);

kern_return_t snddriver_dsp_host_cmd (
	port_t		cmd_port,		// valid dsp command port
	u_int		host_command,		// host command to execute
	int		priority);

/* New for 2.0 */
kern_return_t snddriver_dsp_read_data (
	port_t		cmd_port,		// valid command port
	void		**data,			// buffer pointer or NULL
	int		count,			// count of data elements
	int		data_size,		// bytes per data element
	int		priority);		// priority of this transaction

/* New for 2.0 */
kern_return_t snddriver_dsp_read_messages (
	port_t		cmd_port,		// valid command port
	void		*data,			// buffer pointer or NULL
	int		*count,			// count of data elements
	int		data_size,		// bytes per data element
	int		priority);		// priority of this transaction

kern_return_t snddriver_dsp_read (
	port_t		cmd_port,		// valid command port
	void		*data,			// pointer to buffer
	int		*count,			// count of data elements
	int		data_size,		// bytes per data element
	int		priority);
/* Calls read_messages if DSPMSG, DSPERR, or C_DMA protocol, else read_data */

kern_return_t snddriver_dsp_write (
	port_t		cmd_port,		// valid command port
	void		*data,			// pointer to buffer
	int		count,			// count of data elements
	int		data_size,		// bytes per data element
	int		priority);

kern_return_t snddriver_dsp_boot (
	port_t		cmd_port,		// valid command port
	int		*bootImage,		// on-chip instructions
	int		bootImageSize,		// instruction count
	int		priority);

/*
 * Set number of dma descriptors in stream.
 * New for 2.0.
 */
kern_return_t snddriver_stream_ndma (
	port_t		stream_port,		// valid stream port
	int		tag,			// tag to identify stream
	int		ndma);			// number of dma descs in stream

/* 
 * Reset dsp (new in 2.0). 
 */
kern_return_t snddriver_dsp_reset (
	port_t		cmd_port,		// valid command port
	int		priority);		// priority of this transaction

/*
 * Re-allocate the device port.  This causes all current connections to be
 * terminated. (New in 2.0)
 * This routine causes the current device port (and all other driver ports) to be
 * deallocated.  A new device port is allocated and returned in the new_dev_port
 * argument.  The purpose of this call is to provide security by allowing a privileged
 * entity to terminate all connections to the device and re-register the device in the
 * proper name space.  The host_priv_port argument is the port returned by the
 * host_priv_port() trap (see <mach_host.h>).
 */
kern_return_t snddriver_new_device_port (
	port_t		device_port,	// current valid device port
	port_t		host_priv_port,	// from host_priv_self()
	port_t		*new_dev_port);	// new device port

/*
 * Exchange the current dsp owner port with a new one.  New for 2.0.
 */
kern_return_t snddriver_reset_dsp_owner (
	port_t		device_port,	// valid device port
	port_t		old_owner_port,	// old dsp owner port
	port_t		new_owner_port,	// new dsp owner port
	port_t		new_negotiation);// new dsp negotiation port

/*
 * Exchange the current sndin owner port with a new one.  New for 2.0.
 */
kern_return_t snddriver_reset_sndin_owner (
	port_t		device_port,	// valid device port
	port_t		old_owner_port,	// old sndin owner port
	port_t		new_owner_port,	// new sndin owner port
	port_t		new_negotiation);// new sndin negotiation port

/*
 * Exchange the current sndout owner port with a new one.  New for 2.0.
 */
kern_return_t snddriver_reset_sndout_owner (
	port_t		device_port,	// valid device port
	port_t		old_owner_port,	// old sndout owner port
	port_t		new_owner_port,	// new sndout owner port
	port_t		new_negotiation);// new sndout negotiation port

/*
 * User-initiated dma transfer to dsp. See programming examples. (New in 2.0.)
 */
kern_return_t snddriver_dsp_dma_write (
	port_t		cmd_port,	// valid dsp command port
	int		size,		// # dsp words to transfer
	int		mode,		// mode of dma [1..5]
	pointer_t	data);		// data to output

/*
 * User-initiated dma from dsp. See programming examples. (New in 2.0.)
 */
kern_return_t snddriver_dsp_dma_read (
	port_t		cmd_port,	// valid dsp command port
	int		size,		// .. of dsp buffer in words
	int		mode,		// mode of dma [1..5]
	pointer_t	*data);		// where data is put

/*
 * Set the size of the sound out buffers used by snddriver_stream_setup() when
 * it configures the stream.  The default is vm_page_size.
 * New in 2.0.
 */
kern_return_t snddriver_set_sndout_bufsize (
	port_t		dev_port,		// valid device port
	port_t		owner_port,		// valid owner port
	int		sobsize);		// so buf size

/*
 * Set the number of sound out buffers used by snddriver_stream_setup() when
 * it configures the stream.  The default is 4.
 * New in 2.0.
 */
kern_return_t snddriver_set_sndout_bufcount (
	port_t		dev_port,		// valid device port
	port_t		owner_port,		// valid owner port
	int		sobcount);		// so buf count

kern_return_t snddriver_stream_setup (
	port_t		dev_port,		// valid device port
	port_t		owner_port,		// valid owner port
	int		config,			// stream configuration
	int		buf_size,		// samples per buffer
	int		sample_size,		// bytes per sample
	int		low_water,		// low water mark
	int		high_water,		// high water mark
	int		*protocol,		// modified dsp protocol
	port_t		*stream_port);

kern_return_t snddriver_dsp_protocol (
	port_t		device_port,		// valid device port
	port_t		owner_port,		// port registered as owner
	int		protocol);

/*
 * Stream interaction functions
 */
kern_return_t snddriver_stream_start_reading (
	port_t		stream_port,		// valid stream port
	char		*filename,		//? backing store (or null) NYI
	int		data_size,		// count of samples to read
	int		tag,			// user data
	boolean_t	started_msg,		// send message when started
	boolean_t	completed_msg,		// send message when completed
	boolean_t	aborted_msg,		// send message when aborted
	boolean_t	paused_msg,		// send message when paused
	boolean_t	resumed_msg,		// send message when resumed
	boolean_t	overflow_msg,		// send message when overflowed
	port_t		reply_port);		// port for above messages

kern_return_t snddriver_stream_start_writing (
	port_t		stream_port,		// valid stream port
	void		*data,			// pointer to samples 
	int		data_size,		// count of samples to write
	int		tag,			// user data
	boolean_t	preempt,		// play preemptively
	boolean_t	deallocate,		// deallocate data when sent
	boolean_t	started_msg,		// send message when started
	boolean_t	completed_msg,		// send message when completed
	boolean_t	aborted_msg,		// send message when aborted
	boolean_t	paused_msg,		// send message when paused
	boolean_t	resumed_msg,		// send message when resumed
	boolean_t	overflow_msg,		// send message when overflowed
	port_t		reply_port);

kern_return_t snddriver_stream_control (
	port_t		stream_port,		// valid stream port
	int		tag,			// tag to identify stream
	int		snd_control);

kern_return_t snddriver_stream_nsamples (
	port_t		stream_port,		// valid stream port
	int		*nsamples);


