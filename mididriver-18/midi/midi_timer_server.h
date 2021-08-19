/* 
 * Mach Operating System
 * Copyright (c) 1988 NeXT, Inc.
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */
/*
 * HISTORY
 * 08-Jun-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#ifndef _MIDI_TIMER_SERVER_
#define _MIDI_TIMER_SERVER_
#include <midi/midi_timer.h>

/*
 * Server routine for messages to midi driver timer server.
 */
boolean_t midi_timer_server(msg_header_t *in_msg, msg_header_t *out_msg);

#endif	_MIDI_TIMER_SERVER_

