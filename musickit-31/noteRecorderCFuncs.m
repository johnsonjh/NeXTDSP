#ifdef SHLIB
#include "shlib.h"
#endif

/* 
Modification history:

  01/08/90/daj - Added comments.

*/
#import "_musickit.h"
#import "timeunits.h"

/* This file implements the "NoteRecorder" functionality. Originally this
   was a class (subclass of Instrument) but since it does so little, it's
   now just a couple of functions. 

   The idea of this is to support both tempo-evaluated file writing and
   tempo-unevaluated file writing. Which is used depends on the value of
   an MKTimeUnit variable. See timeunits.h. */

double _MKTimeTagForTimeUnit(id aNote,MKTimeUnit timeUnit)
  /* Return value depends on the time unit. If time unit is MK_second, 
     returns [Conductor time]. Otherwise, returns [[aNote conductor] time].
     */
{
    id cond; 
    return (timeUnit == MK_second) ? MKGetTime() : 
      (cond = [aNote conductor]) ? [cond time] : MKGetTime();
}

double _MKDurForTimeUnit(id aNoteDur,MKTimeUnit timeUnit)
  /* Return value depends on the time unit. If time unit is MK_beat,
     this is the same as [aNoteDur dur]. Otherwise, returns 
     the dur predicted by aNoteDur's conductor. If aNoteDur is not of 
     type MK_noteDur, returns 0. */
{
    id aCond;
    double dur = [aNoteDur dur];
    if ([aNoteDur noteType] != MK_noteDur)
      return 0;
    if (timeUnit == MK_beat)
      return dur;
    aCond = [aNoteDur conductor];
    if (!aCond)  
      return dur;
    return [aCond predictTime:[aCond time] + dur] - MKGetTime();
}


