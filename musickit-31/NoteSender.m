#ifdef SHLIB
#include "shlib.h"
#endif

/*
  NoteSender.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  09/19/89/daj - Changed _myData to type void *.
  03/13/90/daj - Moved private methods to category.
  03/21/90/daj - Added archiving.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  08/23/90/daj - Zone API changes
*/
#import "_musickit.h"

#import "_Note.h"
#import "_NoteSender.h"
@implementation NoteSender:Object
/* NoteSender provides a mechanism for broadcasting
 * Note objects during a Music Kit performance.
 *
 * .ib
 * [aNoteSender connect:aNoteReceiver]
 * .iq
 *
 * A connection can be any object that implements
 * the method \fBreceiveNote:\fR.
 * In a typical
 * Music Kit performance, the connection set consists
 * exclusively of NoteReceivers which are owned by NoteFilters and Instruments.
 *
 * NoteSender's \fBsendNote:\fR method defines the Note-sending
 * mechanism:  when a NoteSender receives the message \fBsendNote:\fIaNote\fR,
 * it forwards the argument (a Note object)
 * by sending the message \fBreceiveNote:\fIaNote\fR
 * to each of its connections.  
 * Similarly,
 * an arbitrary message with two optional arguments
 * can be sent to the connections through one of the \fBelementsPerform:\fR
 * methods.
 *
 * Performers usually
 * send \fBsendNote:\fR to \fBself\fR as part of their
 * \fBperform:\fR method and NoteFilter sends the message 
 * from its \fBreceiveNote:\fR method.
 *
 * You can squelch a NoteSender by sending it the \fBsquelch\fR message.
 * A squelched NoteSender suppresses 
 * messages that it would otherwise send to its connections.
 *
 */
{
    id *noteReceivers;    /* Array of NoteReceivers */
    int connectionCount;  /* Number of connections */
    BOOL isSquelched;     /* \fBYES\fR if the object is currently squelched. */
    id owner;             /* NoteFilter or Performer owning this PerfLink. */
    void *_reservedNoteSender1;
    int _reservedNoteSender2;
    BOOL _reservedNoteSender3;
    short _reservedNoteSender4;
    void *_reservedNoteSender5;
}			
#define _myData _reservedNoteSender1
#define _capacity _reservedNoteSender2 /* Size of array. */
#define _ownerIsPerformer _reservedNoteSender3	/* YES if owner is a Performer. */
#define _isSending _reservedNoteSender4 /* Count of recursive sendNote:s */

/* METHOD TYPES
 * Class initialization
 * Creating and freeing a NoteSender
 * Manipulating the connection set
 * Sending messages to the noteReceivers
 * Querying a NoteSender
 * Squelching and unsquelching
 * Printing the object
 */


-owner
  /* Gets the owner (an Instrument or NoteFilter). */
{
    return owner;
}

static unsigned indexOf(NoteSender *self,id aNoteReceiver)
{
    register id *this = self->noteReceivers;
    register id *last = this + self->_capacity;
    while (this < last) {
        if (*this == aNoteReceiver)
	  return (unsigned)(this - self->noteReceivers);
	this++;
    }
    return (unsigned) -1;
}

-(BOOL)isConnected:aNoteReceiver 
  /* TYPE: Querying; \fBYES\fR if \fIaNoteReceiver\fR is a connection.
   * Returns \fBYES\fR if \fIaNoteReceiver\fR is connected to the receiver.
   */
{
    return (indexOf(self,aNoteReceiver) == (unsigned)-1) ? NO : YES; 
}

-squelch
  /* TYPE: Squelch; Turns off message-sending capability.
   * Squelches the receiver.  While a receiver is squelched it can't send
   * messages to its noteReceivers.
   *
   * Note:  You can schedule a \fBsendNote:\fR message through
   * \fBsendNote:atTime:\fR or \fBsendNote:withDelay\fR even if the
   * receiver is squelched.
   * However, if the receiver is still squelched when the
   * \fBsendNote:\fR message is received, the Note isn't sent.
   *
   * Returns the receiver.
   */
{
    isSquelched = YES;
    return self;
}

-unsquelch
  /* TYPE: Squelch; Turns on message-sending capability.
   * Unsquelches and returns the receiver.
   */
{
    isSquelched = NO;
    return self;
}

-(BOOL)isSquelched
  /* TYPE: Querying; \fBYES\fR if the receiver is squelched.
   * Returns \fBYES\fR if the receiver is squelched.
   */
{
    return isSquelched;
}

-(unsigned)connectionCount
  /* TYPE: Querying; Returns the number of noteReceivers.
   * Returns the number of noteReceivers in the
   * receiver's connection set.
   */
{
    return connectionCount;
}

-connections
  /* TYPE: Manipulating; Returns a List of the connections.
   * Returns a List of the receiver's noteReceivers.
   * The noteReceivers themselves are not
   * copied. It is the sender's responsibility to free the List.
   */
{
    register id *this = noteReceivers;
    register id *last = this + _capacity;
    id aList = [List newCount:connectionCount];
    while (this < last) {
        if (*this)
	  [aList addObject:*this];
	this++;
    }
    return aList;
}

-free 
  /* TYPE: Creating; Frees the receiver.
   * Frees the receiver. Illegal while the receiver is sending. Returns nil
   * Also removes the name, if any, from the name table.
   * if the receiver is freed.
   */
{
    if (_isSending)
      return self;
    [self disconnect];
    NX_FREE(noteReceivers);
    MKRemoveObjectName(self);
    return [super free];
}			

#define EXPANDAMT 1

+new
  /* Create a new instance and sends [self init]. */
{
    self = [self allocFromZone:NXDefaultMallocZone()];
    [self init];
    return self;
}

- init
{
    [super init];
   _MK_CALLOC(noteReceivers,id,EXPANDAMT);
   _capacity = EXPANDAMT;
    return self;
}

-_disconnect:aNoteReceiver
{
    unsigned i;
    if ((i = indexOf(self,aNoteReceiver)) != (unsigned)-1) {
	connectionCount--;
	/* May want to shrink if not sending here. */
	noteReceivers[i] = nil;
	return self;
    }
    return nil;
}

-_connect:aNoteReceiver
{
    unsigned i;
    register id *this = noteReceivers;
    register id *last = this + _capacity;
    if ((i = indexOf(self,aNoteReceiver)) != (unsigned)-1) 
      return nil; /* Already there. */
    connectionCount++;
    while (this < last) {
        if (!*this)  {
	    *this = aNoteReceiver;
	    return self;
	}
	this++;
    }
    _MK_REALLOC(noteReceivers,id,_capacity + EXPANDAMT);
    noteReceivers[_capacity] = aNoteReceiver; /* This is first free slot */
    bzero((char *)(&noteReceivers[_capacity+1]),(EXPANDAMT-1) * sizeof(id));
    /* Make sure there are nils in the remaining slots. */
    _capacity += EXPANDAMT;
    return self;
}

- disconnect:aNoteReceiver
  /* TYPE: Manipulating; Disconnects \fIaNoteReceiver\fR from the receiver.
   * Disconnects \fIaNoteReceiver\fR from the receiver.
   * Returns self. 
   * If the receiver is currently sending to its noteReceivers, returns nil.
   */
{
    if (!aNoteReceiver) 
      return self;
    if ([aNoteReceiver _disconnect:self])
      [self _disconnect:aNoteReceiver];
    return self;
}	

-connect:(id)aNoteReceiver 
  /* TYPE: Manipulating; Connects \fIaNoteReceiver\fR to the receiver.
   * Connects \fIaNoteReceiver\fR to the receiver 
   * and returns \fIself\fR.  
   */
{
    if (![aNoteReceiver isKindOf:[NoteReceiver class]])
      return self;
    if ([self _connect:aNoteReceiver])  
      [aNoteReceiver _connect:self];    
    return self;
}

- disconnect
  /* TYPE: Manipulating; Disconnects all the receiver's noteReceivers.
   * Disconnects all the objects connected to the receiver.
   * Returns the receiver, unless the receiver is currently sending to its
   * noteReceivers, in which case does nothing and returns nil.
   */
{
    register id *this = noteReceivers;
    register id *last = this + _capacity;
    while (this < last) {
        if (*this) { 
	    [*this _disconnect:self];
	    *this = nil;
	}
	this++;
    }
    return self;
}

-sendNote:aNote atTime:(double)time
  /* TYPE: Sending; Sends \fIaNote\fR at beat \fItime\fR of the performance.
   * Schedules a request (with \fIaNote\fR's Conductor) for 
   * \fBsendNote:\fIaNote\fR to be sent to the receiver at time
   * \fItime\fR, measured in beats from the beginning of the
   * performance.
   * Returns the receiver.
   * 
   * Keep in mind that the connection set may change between the time that
   * this message is received and the time that the \fBsendNote:\fR
   * message is sent.
   */
{	
    [[aNote conductor] sel:@selector(sendNote:) to:self atTime:time 
   argCount:1,aNote];
    return self;
}

-sendNote:aNote withDelay:(double)deltaT
  /* TYPE: Sending; Sends \fIaNote\fR after \fIdeltaT\fR beats.
   * Schedules a request (with \fIaNote\fR's Conductor) for 
   * \fBsendNote:\fIaNote\fR to be sent to the receiver at time
   * \fIdeltaT\fR measured in beats
   * from the time this message is received.
   * Returns the receiver.
   *
   * Keep in mind that the connection set may change between the time that
   * this message is received and the time that the \fBsendNote:\fR
   * message is sent.
   */
{
    [[aNote conductor] sel:@selector(sendNote:) to:self withDelay:deltaT 
   argCount:1,aNote];
    return self;
}

-sendAndFreeNote:aNote withDelay:(double)delayTime
    /* Sends the specifed note, delayed by delayTime from the
       current time, as far as the note's conductor is concerned. Then
       frees the note. */
{
    [[aNote conductor] sel:@selector(sendAndFreeNote:) to:self 
   withDelay:delayTime argCount:1,aNote];
    return self;
}

-sendAndFreeNote:aNote
    /* Send note and then free it. */
{
    [self sendNote:aNote];
    [aNote free];
    return self;
}

-sendAndFreeNote:aNote atTime:(double)time
    /* Send the specifed note at the specified time using
       the note's Conductor for time coordination. Then free the note. */
{
    [[aNote conductor] sel:@selector(sendAndFreeNote:) 
   to:self atTime:(double)time argCount:1,aNote];
    return self;
}

-sendNote:aNote
  /* TYPE: Sending; Immediately sends \fIaNote\fR.
   * If the receiver isn't squelched, the \fBreceiveNote:\fIaNote\fR
   * message is sent to its noteReceivers and the receiver is returned.
   * If the receiver is squelched, the message isn't sent 
   * and \fBnil\fR is returned.
   */
{	
    register id *this = noteReceivers;
    register id *last = this + _capacity;
    if (!connectionCount)
      return self;
    if (_ownerIsPerformer)
      [aNote _setPerformer:owner];
    _isSending++;
    while ((this < last) && (!isSquelched)) 
      [*this++ receiveNote:aNote];
    if (_ownerIsPerformer)
      [aNote _setPerformer:nil];
    _isSending--;
    return (isSquelched) ? nil : self;
}

#define VERSION2 2

+initialize
{
    if (self != [NoteSender class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

- copyFromZone:(NXZone *)zone
  /* TYPE: Creating; Creates a new NoteSender as a copy of the receiver.
   * Creates, initializes, and returns a new NoteSender with the same
   * noteReceivers as the 
   * receiver.
   */
{
    register id *this,*last;
    NoteSender *newObj = [super copyFromZone:zone];
    newObj->_capacity = _capacity;
    _MK_MALLOC(newObj->noteReceivers,id,_capacity);
    this = noteReceivers; 
    last = noteReceivers + _capacity;
    while (this < last) 
      [newObj connect:*this++];
    /* Data object is not copied. */
    newObj->_myData = nil;
    newObj->owner = nil;
    newObj->_ownerIsPerformer = NO;
    newObj->_isSending = 0;
    return newObj;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives isSquelched. Also archives NoteReceiver List and owner using 
     NXWriteObjectReference(). */
{
    register id *this;
    register id *last;
    char *str;
    [super write:aTypedStream];
    str = (char *)MKGetObjectName(self);
    NXWriteTypes(aTypedStream,"*icc",&str,&_capacity,&isSquelched,
		 &_ownerIsPerformer);
    NXWriteObjectReference(aTypedStream,owner);
    for (this = noteReceivers, last = this + _capacity; this < last; this++)
      NXWriteObjectReference(aTypedStream,*this);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    register id *this;
    register id *last;
    char *str;
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"NoteSender") == VERSION2) {
	NXReadTypes(aTypedStream,"*icc",&str,&_capacity,&isSquelched,
		     &_ownerIsPerformer);
	if (str) {
	    MKNameObject(str,self);
	    NX_FREE(str);
	}
	owner = NXReadObject(aTypedStream);
	_MK_MALLOC(noteReceivers,id,_capacity);
	for (this = noteReceivers, last = this + _capacity; (this < last); 
	     this++)
	  if (*this = NXReadObject(aTypedStream))
	    connectionCount++;
    }
    return self;
}

@end


@implementation NoteSender(Private)

-_setOwner:obj
  /* Sets the owner (an Instrument or NoteFilter). In most cases,
     only the owner itself sends this message. 
     */
{
    owner = obj;
    return self;
}


-(void)_setData:(void *)anObj 
  /* Facility for associating arbitrary data with a NoteReceiver */
{
    _myData = anObj;
}

-(void *)_getData
  /* */
{
    return _myData;
}

-(void)_setPerformer:aPerformer
  /* TYPE: Modifying; Sets the receiver's Performer.
   * Associates a \fIaPerformer\fR
   * with the receiver, such that \fIaPerformer\fR
   * owns the receiver.
   * Normally, you only invoke this method if you are 
   * implementing a subclass of Performer that creates NoteSender instances.
   * Returns the receiver.
   */
{
    if (!aPerformer) 
      _ownerIsPerformer = NO;
    else {
	owner = aPerformer;
	_ownerIsPerformer = YES;
    }
}

@end














