/*
 *	sounddriver.c - functional interface to the mach sound/dsp driver.
 *	1.0 version written by Lee Boynton
 *	Copyright 1988-90 NeXT, Inc.
 *
 * Please copy any changes to julius so he can maintain the local version
 * used for libdsp.
 *
 *	Modification History:
 *	04/11/90/mtm	Added #import <stdlib.h> per OS request.
 *
 */

#import <stdlib.h>
#import <mach.h>
#import <cthreads.h>
#import <stdlib.h>
#import <string.h>
#import <sys/types.h>

extern int thread_reply();

#include "foodriver_client.c"

/*
 * These are the callout functions that the foodriver_reply_server depends on.
 * They should be local, though eventually MIG may argue about that...
 */

static kern_return_t sndreply_started(port_t port, int tag)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->started) {
	(*handlers->started)(handlers->arg,tag);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_completed(port_t port, int tag)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->completed) {
	(*handlers->completed)(handlers->arg,tag);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_aborted(port_t port, int tag)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->aborted) {
	(*handlers->aborted)(handlers->arg,tag);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_paused(port_t port, int tag)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->paused) {
	(*handlers->paused)(handlers->arg,tag);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_resumed(port_t port, int tag)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->resumed) {
	(*handlers->resumed)(handlers->arg,tag);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_overflow(port_t port, int tag)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->overflow) {
	(*handlers->overflow)(handlers->arg,tag);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_recorded_data(port_t port, int tag, 
						pointer_t ptr, int size)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->recorded_data) {
	(*handlers->recorded_data)(handlers->arg,tag, (void *)ptr, size);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_dsp_cond_true(port_t port, u_int mask,
						 u_int flags, u_int value)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->condition_true) {
	(*handlers->condition_true)(handlers->arg,mask,flags,value);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_dsp_msg(port_t port, pointer_t ptr, int size)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->dsp_message) {
	(*handlers->dsp_message)(handlers->arg,(void *)ptr,size);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

static kern_return_t sndreply_dsp_err(port_t port, pointer_t ptr, int size)
{
    foodriver_handlers_t *handlers = (foodriver_handlers_t *)port;
    if (handlers->dsp_error) {
	(*handlers->dsp_error)(handlers->arg,(void *)ptr,size);
	return KERN_SUCCESS;
    } else
	return KERN_FAILURE;
}

kern_return_t foodriver_reply_handler(
	msg_header_t *msg,
	foodriver_handlers_t *handlers)
{
    int err;
    port_t old_local_port = msg->msg_local_port;
    msg->msg_local_port = (port_t)handlers;
    err = foodriver_reply_server(msg,0); // this must change for MIG!!!
    msg->msg_local_port = old_local_port;
    return err;
}

/*** FIXME: chan_data not fully supported by stream_setup. 
  Can't set DSP space,skip,addr */

