/*
 *	sounddriver.h - functional sound/dsp driver interface.
 *	Copyright 1988-90 NeXT, Inc.
 *
 */

#import <mach.h>
#import <mach_init.h>
#import <sys/message.h>

/*
 * Valid sources and destination codes for stream setup
 */

#define FOODRIVER_STREAM_FROM_SNDIN			(1)
#define FOODRIVER_STREAM_TO_SNDOUT_22			(2)
#define FOODRIVER_STREAM_TO_SNDOUT_44			(3)
#define FOODRIVER_STREAM_FROM_DSP			(4)
#define FOODRIVER_STREAM_TO_DSP				(5)
#define FOODRIVER_STREAM_SNDIN_TO_DSP			(6)
#define FOODRIVER_STREAM_DSP_TO_SNDOUT_22		(7)
#define FOODRIVER_STREAM_DSP_TO_SNDOUT_44		(8)
#define FOODRIVER_STREAM_FROM_SNDIN_THROUGH_DSP		(9)
#define FOODRIVER_STREAM_THROUGH_DSP_TO_SNDOUT_22	(10)
#define FOODRIVER_STREAM_THROUGH_DSP_TO_SNDOUT_44	(11)
/* New for 2.0 */
#define FOODRIVER_DMA_STREAM_TO_DSP			(12)
#define FOODRIVER_DMA_STREAM_FROM_DSP			(13)
#define FOODRIVER_DMA_STREAM_THROUGH_DSP_TO_SNDOUT_22	(14)
#define FOODRIVER_DMA_STREAM_THROUGH_DSP_TO_SNDOUT_44	(15)

/*
 * Protocol options for the dsp (subset of <nextdev/snd_msgs.h> protocols)
 */
#define FOODRIVER_DSP_PROTO_DSPERR	0x1	// DSP error messages enabled
#define FOODRIVER_DSP_PROTO_C_DMA	0x2	// Complex DMA mode
#define FOODRIVER_DSP_PROTO_S_DMA	0x4	// Simple DMA mode
/* New for 2.0 */
#define FOODRIVER_DSP_PROTO_HFABORT	0x80	// Enable DSP abort on HF2&HF3
#define FOODRIVER_DSP_PROTO_DSPMSG	0x100	// DSP messages enabled
#define FOODRIVER_DSP_PROTO_RAW 	0x200	// Enable raw DSP mode

/*
 * DSP Host Commands used with protocol FOODRIVER_DSP_PROTO_C_DMA.
 * See on-line programming examples for usage.
 */
#define FOODRIVER_DSP_HC_HOST_RD	(0x24>>1) 	// Host Read Done
#define FOODRIVER_DSP_HC_HOST_WD	(0x28>>1)	// Host Write Done
#define FOODRIVER_DSP_HC_SYS_CALL	(0x2C>>1)	// System Call

/*
 * Control codes for streams
 */
#define FOODRIVER_AWAIT_STREAM		(0x1)
#define FOODRIVER_ABORT_STREAM		(0x2)
#define FOODRIVER_PAUSE_STREAM		(0x4)
#define FOODRIVER_RESUME_STREAM		(0x8)

/*
 * Driver message priorities
 */
#define FOODRIVER_LOW_PRIORITY		(2)
#define FOODRIVER_MED_PRIORITY		(1)
#define FOODRIVER_HIGH_PRIORITY		(0)

/*
 * Host flag bits
 */
#define FOODRIVER_ICR_HF0		(0x08000000)	// settable host flag
#define FOODRIVER_ICR_HF1		(0x10000000)	// settable host flag
#define FOODRIVER_ISR_HF2		(0x0800)	// readable host flag
#define FOODRIVER_ISR_HF3		(0x1000)	// readable host flag

#include "foodriver_client.h"

/*
 * Asynchronous return message parsing and callout (the reply server).
 *
 * The foodriver_reply_server implementation takes a pointer to a message 
 * and a pointer to a structure containing the dispatch functions for
 * each type of message. It parses the message and calls the
 * appropriate procedure, and handles the reply message (if any).
 */

typedef void (*sndreply_tagged_t)(void *arg, int tag);
typedef void (*sndreply_recorded_data_t)(void *arg, int tag, 
					 void *data, int size);
typedef void (*sndreply_dsp_cond_true_t)(void *arg, u_int mask, 
					 u_int flags, u_int regs);
typedef void (*sndreply_dsp_msg_t)(void *arg, int *data, int size);

typedef struct foodriver_handlers {
    void *			arg;
    int				timeout;
    sndreply_tagged_t		started;
    sndreply_tagged_t		completed;
    sndreply_tagged_t		aborted;
    sndreply_tagged_t		paused;
    sndreply_tagged_t		resumed;
    sndreply_tagged_t		overflow;
    sndreply_recorded_data_t	recorded_data;
    sndreply_dsp_cond_true_t	condition_true;
    sndreply_dsp_msg_t		dsp_message;
    sndreply_dsp_msg_t		dsp_error;
 } foodriver_handlers_t;

kern_return_t foodriver_reply_handler(
	msg_header_t		*msg,		// message to parse
	foodriver_handlers_t	*handlers);	// table of callout procs


