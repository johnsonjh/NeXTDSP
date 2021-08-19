#ifdef SHLIB
#include "shlib.h"
#endif

/*
  NoteFilter.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* Modification history:

  03/21/90/daj - Added archiving.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  08/23/90/daj - Changed to zone API.

*/

#import "_musickit.h"

#import "_Instrument.h"
#import "_NoteSender.h"
#import "_NoteReceiver.h"

#import "NoteFilter.h"
@implementation NoteFilter:Instrument
  /* NoteFilter is an abstract class. 
     NoteFilter adds some of the functionality of Performer to that of 
     Instrument. In particular, it adds the ability to send to elements
     in a collection of NoteSenders.
     You subclass NoteFilter and override
     realizeNote:fromNoteSender: to do multiplexing of the input and output
     paths of the NoteFilter. NoteFilters may modify Notes.
     The only requirement is that any modification
     you make before sending a Note is undone afterwards. I.e. the 
     'copy on write or memory' principle is used.
     */
{
    id noteSenders;    /* Collection of NoteSenders. */
}

#define VERSION2 2

+initialize
{
    if (self != [NoteFilter class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

-init
{
    [super init]; /* Creates noteReceivers */
    noteSenders = [List new];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: and archives noteSender List. */
{
    [super write:aTypedStream];
    NXWriteObject(aTypedStream,noteSenders);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"NoteFilter") == VERSION2) 
      noteSenders = NXReadObject(aTypedStream);
    return self;
}

#import "noteDispatcherMethods.m"

-copyFromZone:(NXZone *)zone
  /* Copies object, copying NoteSenders and NoteReceivers. */
{
    NoteFilter *newObj = [super copyFromZone:zone];
    id *el,newEl;
    unsigned n;
    newObj->noteSenders = [List newCount:n = [noteSenders count]];
    for (el = NX_ADDRESS(noteSenders); n--; el++) 
      [newObj addNoteSender:newEl = [*el copy]];
    return newObj;
}

-addNoteSender:(id)aNoteSender
  /* If aNoteSender is already owned by the receiver, returns nil.
     Otherwise, aNoteSender is removed from its owner, the owner
     of aNoteSender is set to self, aNoteSender is added to 
     noteSenders (as the last element) and aNoteSender is returned. 
     For some subclasses, it is inappropriate for anyone
     other than the subclass instance itself to send this message. 
     If you override this method, first forward it to super.
     If the receiver is in performance, this message is ignored and nil
     is returned.
     */
{
    id owner = [aNoteSender owner];
    if (owner == self)
      return nil;
    if (_noteSeen)
      return nil;
    [owner removeNoteSender:aNoteSender];
    [noteSenders addObjectIfAbsent:aNoteSender];
    [aNoteSender _setPerformer:nil]; /* Tell it we're not a performer */
    return aNoteSender;
}

-removeNoteSender:(id)aNoteSender
  /* If aNoteSender is not owned by the receiver, returns nil.
     Otherwise, removes aNoteSender from the receiver's NoteSender List
     and returns aNoteSender. 
     For some subclasses, it is inappropriate for anyone
     other than the subclass instance itself to send this message. 
     If the receiver is in a performance, this message is ignored and nil is
     returned. */
{
    if ([aNoteSender owner] != self)
      return nil;
    if (_noteSeen)
      return nil;
    [noteSenders removeObject:aNoteSender];
    [aNoteSender _setOwner:nil];
    return aNoteSender;
}


-free
  /* TYPE: Creating
   * This invokes \fBfreenoteSenders\fR and \fBfreenoteReceivers\fR.
   * Then it frees itself.
   */
{
    [self freeNoteSenders];
    [noteSenders free];
    return [super free];
}

@end

