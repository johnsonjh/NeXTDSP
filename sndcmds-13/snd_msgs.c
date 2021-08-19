/*
 * snd_msgs.c
 *
 * Interface routines for composing messages to send to the sound
 * facilities.
 *
 * Modification History:
 *	06/05/90/mtm	Recoginize "system segment" header when booting.
 *			Fixes bug #3571.
 *	08/13/90/mtm	Cast msg pointer to msg_send().
 *	09/26/90/gk	Fix for new message protocols (bug #9796).
 */
#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <mach_error.h>
#import <kern/ipc_ptraps.h>
#import <sound/sound.h>
#import <nextdev/snd_dspreg.h>
#include <nextdev/snd_msgs.h>

static kern_return_t error_ret(snd_illegal_msg_t *msg, int from_msgid)
{
	switch (msg->header.msg_id) {
	case SND_MSG_ILLEGAL_MSG: {
		if (msg->ill_msgid != from_msgid) {
			printf("Ill reply (%d) to wrong msgid %d,"
				" msgid should be %d\n", msg->ill_error,
				msg->ill_msgid,
				from_msgid);
			return KERN_FAILURE;
		}
		if (msg->ill_error == SND_NO_ERROR) {
			printf("Got ack when expecting return, msgid %d\n",
				msg->ill_msgid);
			return KERN_FAILURE;
		}
		return msg->ill_error;
	}
	default:
		printf("Unexpected msg recieved, msgid %d\n",
			msg->header.msg_id);
		return KERN_FAILURE;
	}
}

static kern_return_t normal_ret(snd_illegal_msg_t *msg, int from_msgid)
{
	switch (msg->header.msg_id) {
	case SND_MSG_ILLEGAL_MSG: {
		if (msg->ill_msgid != from_msgid) {
			printf("Ill reply (%d) to wrong msgid %d,"
				" msgid should be %d\n", msg->ill_error,
				msg->ill_msgid,
				from_msgid);
			return KERN_FAILURE;
		}
		if (msg->ill_error == SND_NO_ERROR)
			return KERN_SUCCESS;
		return msg->ill_error;
	}
	default:
		printf("Unexpected msg recieved, msgid %d\n",
			msg->header.msg_id);
		return KERN_FAILURE;
	}
}

static kern_return_t snd_stream_control (
	port_name_t	stream_port,
	int		snd_control)
{
	kern_return_t r;
	struct str_ctl {
		snd_stream_msg_t	str_msg;
		snd_stream_control_t	control;
	} m;
	static struct str_ctl M = {
	    {
		{
			/* no name */		0,
			/* msg_simple */	TRUE,
			/* msg_size */		sizeof(M),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		SND_MSG_STREAM_MSG
		},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0
	    },
	    {
		{{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		SND_MT_CONTROL},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0,
	    }
	};

	m = M;
	m.control.snd_control = snd_control;
	m.str_msg.header.msg_remote_port = stream_port;
	m.str_msg.header.msg_local_port = thread_reply();
	
	r = msg_rpc(&m.str_msg.header,
		MSG_OPTION_NONE,
		sizeof(snd_illegal_msg_t),
		0,
		0);
	if (r != KERN_SUCCESS)
		return r;

	return normal_ret((snd_illegal_msg_t *)&m, SND_MSG_STREAM_MSG);
}

kern_return_t snd_abort_stream (
	port_name_t	stream_port)
{
	return snd_stream_control(stream_port, SND_DC_ABORT);
}

kern_return_t snd_pause_stream (
	port_name_t	stream_port)
{
	return snd_stream_control(stream_port, SND_DC_PAUSE);
}

kern_return_t snd_await_stream (
	port_name_t	stream_port)
{
	return snd_stream_control(stream_port, SND_DC_AWAIT);
}

kern_return_t snd_resume_stream (
	port_name_t	stream_port)
{
	return snd_stream_control(stream_port, SND_DC_RESUME);
}

kern_return_t snd_record (
	port_name_t	stream_port,
	port_name_t	reg_port,
	int		high_water,
	int		low_water,
	int		dma_size,
	int		region_size)
{
	kern_return_t r;
	struct str_rec {
		snd_stream_msg_t		str_msg;
		snd_stream_record_data_t	record;
		snd_stream_options_t		options;
	} m;
	static struct str_rec M = {
	    {
		{
			/* no name */		0,
			/* msg_simple */	FALSE,
			/* msg_size */		sizeof(M),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		SND_MSG_STREAM_MSG
		},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0
	    },		// message header
	    {
		{{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		SND_MT_RECORD_DATA},	// record message
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		2,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0,	// options
		0,	// nbytes
		{
			/* msg_type_name = */		MSG_TYPE_PORT,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0,	// region_port
		{
			/* msg_type_name = */		MSG_TYPE_CHAR,
			/* msg_type_size = */		8,
			/* msg_type_number = */		0,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		}	// no file name
	    },
	    {
	    	{{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		SND_MT_OPTIONS},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		3,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0,	// high_water
		0,	// low_water
		0	// dma_size
	    }
	};

	m = M;
	m.str_msg.header.msg_remote_port = stream_port;
	m.str_msg.header.msg_local_port = thread_reply();
	m.record.options =   SND_DM_COMPLETED_MSG
		     | SND_DM_ABORTED_MSG
		     | SND_DM_OVERFLOW_MSG;
	m.record.nbytes = region_size;
	m.record.reg_port = reg_port;
	m.record.filenameType.msg_type_number = 0;
	m.options.high_water = high_water;
	m.options.low_water = low_water;
	m.options.dma_size = dma_size;
	
	r = msg_rpc(&m.str_msg.header,
		MSG_OPTION_NONE,
		sizeof(snd_illegal_msg_t),	// recieve size
		0,		// send_timeout
		0);		// recieve_timeout
	if (r != KERN_SUCCESS)
		return r;

	return normal_ret((snd_illegal_msg_t *)&m, SND_MSG_STREAM_MSG);
}

/*
 * Message to request a stream port with the requested charactaristics.
 */
kern_return_t snd_get_stream (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// valid soundout/in/dsp owner port
	port_t		*stream_port,	// returned stream_port
	u_int		stream)		// stream to/from what?
{
	int r;

	union {
		snd_get_stream_t	g_stream;
		snd_ret_port_t		r_stream;
		snd_illegal_msg_t	r_ill;
		msg_header_t		header;
	} msg;

	static snd_get_stream_t M = {
		{
			/* no name */		0,
			/* msg_simple */	FALSE,
			/* msg_size */		sizeof(snd_get_stream_t),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		SND_MSG_GET_STREAM
		},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0,
		{
			/* msg_type_name = */		MSG_TYPE_PORT,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0
	};

	msg.g_stream = M;
	msg.header.msg_remote_port = device_port;
	msg.header.msg_local_port = thread_reply();
	msg.g_stream.owner = owner_port;
	msg.header.msg_local_port = owner_port;
	msg.g_stream.stream = stream;
	
	r = msg_rpc(&msg.header, MSG_OPTION_NONE, sizeof(msg), 0, 0);
	if (r != KERN_SUCCESS)
		return r;
	if (msg.header.msg_id != SND_MSG_RET_STREAM)
		return error_ret(&msg.r_ill, SND_MSG_GET_STREAM);
	*stream_port = msg.r_stream.header.msg_remote_port;
	return r;
}

/*
 * Tells the driver who the owner of the DSP device is,
 * for returning unsolicited messages and allocating resources.
 */
static kern_return_t snd_set_owner (
	port_t		device_port,
	port_t		owner_port,
	port_t		*neg_port,
	int		msg_id)
{
	register int r;
	union {
		snd_illegal_msg_t	r_ill;
		snd_set_owner_t		s_own;
		msg_header_t		header;
	} msg;
	static snd_set_owner_t M = {
		{
			/* no name */		0,
			/* msg_simple */	FALSE,
			/* msg_size */		sizeof(snd_set_owner_t),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		0
		},
		{
			/* msg_type_name = */		MSG_TYPE_PORT,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0,
		{
			/* msg_type_name = */		MSG_TYPE_PORT,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0
	};

	msg.s_own = M;
	msg.header.msg_id = msg_id;
	msg.header.msg_remote_port = device_port;
	msg.header.msg_local_port = thread_reply();
	msg.s_own.owner = owner_port;
	msg.s_own.negotiation = *neg_port;

	r = msg_rpc(&msg.header, MSG_OPTION_NONE, sizeof(msg), 0, 0);
	if (r == KERN_SUCCESS)
		r = normal_ret(&msg.r_ill, msg_id);
	if (r != KERN_SUCCESS)
		*neg_port = msg.r_ill.header.msg_remote_port;
	return r;
}

/*
 * Tells the driver who the owner of the DSP device is,
 * for returning unsolicited messages and allocating resources.
 */
kern_return_t snd_set_dspowner (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// dsp owner port
	port_t		*neg_port)	// dsp negotiation port
{
	return snd_set_owner(device_port, owner_port, neg_port,
		SND_MSG_SET_DSPOWNER);
}

/*
 * Tells the driver who the owner of the sound in device is.
 */
kern_return_t snd_set_sndinowner (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// sound in owner port
	port_t		*neg_port)	// sound in negotiation port
{
	return snd_set_owner(device_port, owner_port, neg_port,
		SND_MSG_SET_SNDINOWNER);
}

/*
 * Tells the driver who the owner of sound out device is.
 */
kern_return_t snd_set_sndoutowner (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// sound out owner port
	port_t		*neg_port)	// sound out negotiation port
{
	return snd_set_owner(device_port, owner_port, neg_port,
		SND_MSG_SET_SNDOUTOWNER);
}

/*
 * Set DSP protocol.
 */
kern_return_t snd_dsp_proto (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// port registered as owner
	int		proto)		// volume on right channel	
{
	register int r;
	union {
		snd_dsp_proto_t		 s_proto;
		snd_illegal_msg_t	r_ill;
		msg_header_t		header;
	} msg;
	static snd_dsp_proto_t M = {
		{
			/* no name */		0,
			/* msg_simple */	FALSE,
			/* msg_size */		sizeof(snd_dsp_proto_t),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		SND_MSG_DSP_PROTO
		},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0,
		{
			/* msg_type_name = */		MSG_TYPE_PORT,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0
	};

	msg.s_proto = M;
	msg.header.msg_remote_port = device_port;
	msg.header.msg_local_port = thread_reply();
	msg.s_proto.owner = owner_port;
	msg.s_proto.proto = proto;

	r = msg_rpc(&msg.header, MSG_OPTION_NONE, sizeof(msg), 0, 0);
	if (r != KERN_SUCCESS)
		return r;
	return normal_ret(&msg.r_ill, SND_MSG_DSP_PROTO);
}
/*
 * Message to request the dsp command port.
 */
kern_return_t snd_get_dsp_cmd_port (
	port_t		device_port,	// valid device port
	port_t		owner_port,	// valid owner port
	port_t		*cmd_port)	// returned cmd_port
{
	int r;

	union {
		snd_get_dsp_cmd_port_t	g_cmd;
		snd_ret_port_t		r_cmd;
		snd_illegal_msg_t	r_ill;
		msg_header_t		header;
	} msg;

	static snd_get_dsp_cmd_port_t M = {
		{
			/* no name */		0,
			/* msg_simple */	FALSE,
			/* msg_size */		sizeof(snd_get_dsp_cmd_port_t),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		SND_MSG_GET_DSP_CMD_PORT
		},
		{
			/* msg_type_name = */		MSG_TYPE_PORT,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		0
	};

	msg.g_cmd = M;
	msg.header.msg_remote_port = device_port;
	msg.header.msg_local_port = thread_reply();
	msg.g_cmd.owner = owner_port;

	r = msg_rpc(&msg.header, MSG_OPTION_NONE, sizeof(msg), 0, 0);
	if (r != KERN_SUCCESS)
		return r;
	if (msg.r_cmd.header.msg_id != SND_MSG_RET_CMD)
		return error_ret(&msg.r_ill, SND_MSG_GET_DSP_CMD_PORT);
	*cmd_port = msg.header.msg_remote_port;
	return r;
}

/*
 * Set interface for the specified channel.
 */
kern_return_t snd_dspcmd_chandata (
	port_t		cmd_port,	// valid dsp command port
	int		addr,		// .. of dsp buffer
	int		size,		// .. of dsp buffer
	int		skip,		// dma skip factor
	int		space,		// dsp space of buffer
	int		mode,		// mode of dma [1..5]
	int		chan)		// channel for dma
{
	register int r;
	snd_dspcmd_chandata_t s_chandata;
	static snd_dspcmd_chandata_t M = {
		{
			/* no name */		0,
			/* msg_simple */	TRUE,
			/* msg_size */		sizeof(snd_dspcmd_chandata_t),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		SND_MSG_DSP_CHANDATA
		},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		6,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		}
	};

	s_chandata = M;
	s_chandata.header.msg_remote_port = cmd_port;
	s_chandata.header.msg_local_port = thread_reply();

	s_chandata.addr = addr;
	s_chandata.size = size;
	s_chandata.skip = skip;
	s_chandata.space = space;
	s_chandata.mode = mode;
	s_chandata.chan = chan;

	r = msg_rpc(&s_chandata.header,
		MSG_OPTION_NONE,
		sizeof(snd_illegal_msg_t),
		0,
		0);
	if (r != KERN_SUCCESS)
		return r;
	return normal_ret((snd_illegal_msg_t *)&s_chandata,
			  SND_MSG_DSP_CHANDATA);
}

#define DSP_SPACE_P 4

static int simple_boot_image_offset(int *dspImage, int imageSize)
/*
 * If the first thing in the image is program memory at address zero,
 * return the offset to it.
 * Otherwise return -1 (not a simple image).
 * Ignores the "system segment" (header added by sound library) if there is one.
 */
{
    int i=0, *image = dspImage;
    while (i<imageSize) {
	if ((image[i] == DSP_SPACE_P) && (image[i+1] == 0))
	    return i+3;
	if ((image[i] > 0) && (image[i] < 5))
	    return -1; // not simple
	i += 2;
	i += (image[i] + 1);
     }
     return -1;
}

kern_return_t snd_dsp_boot (
	port_name_t	cmd_port,
	int		*boot_code,
	int		boot_code_size)
{
	register int r, msg_size;
	boolean_t simple_boot, inline_data;
	SNDSoundStruct *booterCore;
	int imageOffset;

	msg_header_t		*m;
	snd_dspcmd_msg_t	*msg;
	snd_dsp_data_t		*code;
	snd_dsp_host_flag_t	*f1;
	snd_dsp_host_flag_t	*f2;
	snd_dsp_data_t		*booter;

	static snd_dspcmd_msg_t M = {
		{
			/* no name */		0,
			/* msg_simple */	TRUE,
			/* msg_size */		sizeof(snd_dspcmd_msg_t),
			/* msg_type */		MSG_TYPE_NORMAL,
			/* msg_remote_port */	0,
			/* msg_reply_port */	0,
			/* msg_id */		SND_MSG_DSP_MSG
		}, // header
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		2,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		DSP_MSG_MED,	// priority
		0		// atomic
	};
	static snd_dsp_data_t D = {
		{{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		SND_DSP_MT_DATA},
		{
			{
				/* msg_type_name = */		0,
				/* msg_type_size = */		0,
				/* msg_type_number = */		0,
				/* msg_type_inline = */		TRUE,
				/* msg_type_longform = */	TRUE,
				/* msg_type_deallocate = */	FALSE,
			},
			/* msg_type_long_name = */	MSG_TYPE_INTEGER_32,
			/* msg_type_long_size = */	32,
			/* msg_type_long_number = */	0,
		},
	};
	static snd_dsp_host_flag_t F = {
		{{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		1,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		SND_DSP_MT_HOST_FLAG},
		{
			/* msg_type_name = */		MSG_TYPE_INTEGER_32,
			/* msg_type_size = */		32,
			/* msg_type_number = */		2,
			/* msg_type_inline = */		TRUE,
			/* msg_type_longform = */	FALSE,
			/* msg_type_deallocate = */	FALSE,
		},
		ICR_HF0<<24,
		ICR_HF0<<24
	};

	/*
	 * Figure out if this is a simple or complex dsp boot.
	 */
	imageOffset = simple_boot_image_offset(boot_code, boot_code_size);
	if (imageOffset > 0) {
		boot_code += imageOffset;
		boot_code_size -= imageOffset;
		simple_boot = TRUE;
		msg_size =    sizeof(M)		// msg header
			    + sizeof(D)		// code
			    - sizeof(D.data)
			    + boot_code_size*sizeof(int)
			    + 2*sizeof(F);	// toggle hf1
	} else {
		simple_boot = FALSE;
		SNDReadSoundfile("/usr/lib/sound/booter.snd", &booterCore);
		msg_size =  sizeof(M);		// msg header
		msg_size += sizeof(D);		// booter
		msg_size -= sizeof(D.data);
		msg_size += booterCore->dataSize;
		msg_size += 2*sizeof(F);	// toggle hf1
		msg_size += sizeof(D);		// code
		msg_size -= sizeof(D.data);
		msg_size += boot_code_size*sizeof(int);
	}

	if (msg_size > MSG_SIZE_MAX) {
		inline_data = FALSE;
		msg_size -= (boot_code_size*sizeof(int) - sizeof(D.data));
	} else
		inline_data = TRUE;

	m = (msg_header_t *)malloc(msg_size);
	msg = (snd_dspcmd_msg_t *)m;
   	if (simple_boot) {
		code = (snd_dsp_data_t *)(msg+1);
		if (inline_data)
			f1 = (snd_dsp_host_flag_t *)((int)code +
				(  sizeof(D)
				 + (  boot_code_size*sizeof(int)
				    - sizeof(D.data))));
		else
			f1 = (snd_dsp_host_flag_t *)(code+1);
		f2 = f1+1;
	} else {
		booter = (snd_dsp_data_t *)(msg+1);
		f1 = (snd_dsp_host_flag_t *)((int)booter +
			(  sizeof(D)
			 - (booterCore->dataSize - sizeof(D.data))));
		f2 = f1+1;
		code = (snd_dsp_data_t *)(f2+1);
		booter->dataType.msg_type_long_number =
			booterCore->dataSize/sizeof(int);
		bcopy((char *)((int)booterCore + booterCore->dataLocation),
			(char *)&booter->data, booterCore->dataSize);
	}

	*msg = M;
	m->msg_size = msg_size;
	m->msg_local_port = thread_reply();
	m->msg_remote_port = cmd_port;
	*code = D;
	*f1 = F;
	*f2 = F;
	f2->flags = 0;
	code->dataType.msg_type_long_number = boot_code_size;
	if (inline_data) {
		bcopy((char *)boot_code, (char *)&code->data,
			boot_code_size*sizeof(int));
	} else {
		code->dataType.msg_type_header.msg_type_inline = FALSE;
		m->msg_simple = FALSE;
		code->data = (int)boot_code;
	}

	r = msg_rpc(m,
		MSG_OPTION_NONE,
		sizeof(snd_illegal_msg_t),
		0,
		0);
	if (r != KERN_SUCCESS)
		return r;
	return normal_ret((snd_illegal_msg_t *)m, SND_MSG_DSP_MSG);
}

static char *snd_error_list[] = {
	"sound success",
	"sound message sent to wrong port",
	"unknown sound message id",
	"bad parameter list in sound message",
	"can't allocate memory for recording",
	"sound service in use",
	"sound service requires ownership",
	"DSP channel not initialized",
	"can't find requested sound resource",
	"bad DSP mode for sending data commands",
	"external pager support not implemented",
	"sound data not properly aligned"
};

char *snd_error_string(int error)
{
	if (error >= SND_NO_ERROR)
		return snd_error_list[error-SND_NO_ERROR];
	return "unrecognized sound error message";
}
void snd_error(char *string, int error)
{
	fprintf(stderr, "%s (%s)\n", string,
		error <= 0 ?
			  mach_error_string(error)
			: snd_error_string(error));
}



