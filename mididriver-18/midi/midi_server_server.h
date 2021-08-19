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

#ifndef _MIDI_SERVER_SERVER_
#define _MIDI_SERVER_SERVER_
#include <midi/midi_server.h>

/*
 * Server routine for messages to midi driver.
 */
boolean_t midi_server_server(msg_header_t *in_msg, msg_header_t *out_msg);

#endif	_MIDI_SERVER_SERVER_

