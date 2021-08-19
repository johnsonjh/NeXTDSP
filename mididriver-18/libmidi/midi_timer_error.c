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
#import <midi/midi_timer_error.h>
#import <midi/midi_types.h>

void midi_timer_error(const char *s, kern_return_t r)
{
	if (r < TIMER_BAD_PARM || r > TIMER_MODE_WRONG)
		mach_error(s, r);
	else
		fprintf(stderr, "%s : %s (%d)\n", s,
			midi_timer_error_string(r), r);
}

static const char *midi_timer_error_list[] = {
	"bad parameter list in message",
	"bad control port",
	"timer in wrong mode for operation",
	"operation not supported"
};

const char *midi_timer_error_string(kern_return_t r)
{
	if (r < TIMER_BAD_PARM || r > TIMER_MODE_WRONG)
		return mach_error_string(r);
	else
		return midi_timer_error_list[r - MIDI_BAD_PARM];
}

