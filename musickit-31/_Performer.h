/* 
Modification history:

  03/24/90/daj - Added pauseFor: instance variable.
*/

#ifndef __PERFORMER_H
#define __PERFORMER_H
#define _pauseOffset _reservedPerformer1 
  /* Difference between the beat when a performer
     is paused and its time. */
#define _endTime _reservedPerformer2	
  /* End time for the object.  Used internally.
     Subclass should not set _endTime. */
#define _performMsgPtr _reservedPerformer3
#define _deactivateMsgPtr _reservedPerformer4
#define _pauseForMsgPtr _reservedPerformer5
#define _extraPerformerVars _reservedPerformer6

/* This struct is for instance variables added after the 1.0 instance variable
   freeze. */
typedef struct __extraPerformerInstanceVars {
  double timeOffset;
} _extraPerformerInstanceVars;

#define _timeOffset(_self) \
  ((_extraPerformerInstanceVars *)_self->_extraPerformerVars)->timeOffset

#import "_NoteSender.h"
#import "Performer.h"
@interface Performer(Private)

-_copyFromZone:(NXZone *)zone;

@end

#endif __PERFORMER_H



