#ifndef __INSTRUMENT_H
#define __INSTRUMENT_H
#define _noteSeen _reservedInstrument1         

#import "_NoteReceiver.h"
#import "Instrument.h"

@interface Instrument(Private)

-_realizeNote:aNote fromNoteReceiver:aNoteReceiver;
-_afterPerformance;

@end

#endif __INSTRUMENT_H



