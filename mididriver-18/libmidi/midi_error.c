/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 07-Jun-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#import <stdio.h>
#import <mach_error.h>
#import <midi/midi_error.h>
#import <midi/midi_types.h>

void midi_error(const char *s, kern_return_t r)
{
	if (r < MIDI_BAD_PARM || r > MIDI_WILL_BLOCK)
		mach_error(s, r);
	else
		fprintf(stderr, "%s : %s (%d)\n", s, midi_error_string(r), r);
}

static const char *midi_error_list[] = {
	"bad parameter list in message",
	"access requested to existing exclusive access port",
	"ownership rights required",
	"data not properly aligned",
	"no device owner",
	"improper operation in current mode",
	"message received on wrong port",
	"next message will block",
	"operation not supported"
};

const char *midi_error_string(kern_return_t r)
{
	if (r < MIDI_BAD_PARM || r > MIDI_WILL_BLOCK)
		return mach_error_string(r);
	else
		return midi_error_list[r - MIDI_BAD_PARM];
}

