/*
  Performer.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>
#import "NoteSender.h"
#import "Conductor.h"

typedef enum _MKPerformerStatus { /* Status for Performers. */
    MK_inactive,
    MK_active,
    MK_paused
  } MKPerformerStatus;

@interface Performer : Object
/* 
 * 
 * Performer is an abstract class that defines the general mechanism for
 * performing Notes during a Music Kit performance.  Each subclass of
 * Performer implements the perform method to define how it obtains and
 * performs Notes.
 * 
 * During a performance, a Performer receives a series of perform
 * messages.  In its implementation of perform, a Performer subclass must
 * set the nextPerform variable.  nextPerform indicates the amount of
 * time (in beats) to wait before the next perform message arrives.  The
 * messages are sent by the Performer's Conductor.  Every Performer is
 * managed by a Conductor; unless you set its Conductor explicitly,
 * through the setConductor: method, a Performer is managed by the the
 * defaultConductor.
 * 
 * A Performer contains a List of NoteSenders, objects that send Notes
 * (to NoteReceivers) during a performance.  Performer subclasses should
 * implement the init method to create and add some number of
 * NoteSenders to a newly created instance.  As part of its perform
 * method, a Performer typically creates or othewise obtains a Note (for
 * example, by reading it from a Part or a scorefile) and sends it by
 * invoking NoteSender's sendNote: method.
 * 
 * To use a Performer in a performance, you must first send it the
 * activate message.  activate invokes the activateSelf method and then
 * schedules the first perform message request with the Conductor.
 * activateSelf can be overridden in a subclass to provide further
 * initialization of the Performer.  The performance begins when the
 * Conductor class receives the startPerformance message.  It's legal to
 * activate a Performer after the performance has started.
 * 
 * Sending the deactivate message removes the Performer from the
 * performance and invokes the deactivateSelf method.  This method can be
 * overridden to implement any necessary finalization, such as freeing
 * contained objects.
 * 
 * During a performance, a Performer can be stopped and restarted by
 * sending it the pause and resume messages, respectively.  perform
 * messages destined for a paused Performer are delayed until the object
 * is resumed.
 * 
 * You can shift a Performer's performance timing by setting its
 * timeShift variable.  timeShift, measured in beats, is added to the
 * initial setting of nextPerform.  If the value of timeShift is
 * negative, the Performer's Notes are sent earlier than otherwise
 * expected; this is particularly useful for a Performer that's
 * performing Notes starting from the middle of a Part or Score.  A
 * positive timeShift delays the performance of a Note.
 * 
 * You can also set a Performer's maximum duration.  A Performer is
 * automatically deactivated if its performance extends beyond duration
 * beats.
 * 
 * A Performer has a status, represented as one of the following
 * MKPerformerStatus values:
 *  
 * 
 * * Status       Meaning
 * * MK_inactive  A deactivated or not-yet-activated Performer.
 * * MK_active    An activated, unpaused Performer.
 * * MK_paused    The Performer is activated but currently paused.
 * 
 * Some messages can only be sent to an inactive (MK_inactive) Performer.
 * A Performer's status can be queried with the status message.
 * 
 * CF: Conductor, NoteSender
 */
{
    id conductor;  /* The object's Conductor. */
    MKPerformerStatus status; /* The object's status. */
    int performCount;/* Number of perform messages the 
                    object has received. */
    double timeShift; /* Timing offset. */
    double duration; /* Maximum duration. */
    double time; /* The object's notion of the current time. */
    double nextPerform; /* The next time the object will send a Note. */
    id noteSenders;  /* The object's collection of NoteSenders. */
    id delegate;     /* The object's delegate, if any. */
    double _reservedPerformer1;
    double _reservedPerformer2;
    MKMsgStruct *_reservedPerformer3;
    MKMsgStruct *_reservedPerformer4;
    MKMsgStruct *_reservedPerformer5;
    void *_reservedPerformer6;
}

 /* METHOD TYPES
  * Creating and freeing a Performer
  * Modifying the object
  * Querying the object
  * Performing
  */

+ new; 
 /* TYPE: Creating
  * Creates object from NXDefaultMallocZone() and sends init to the new 
  * instance. 
  */

- noteSenders; 
 /* TYPE: Querying
  * Creates and returns a List containing the receiver's NoteSenders.
  * It's the sender's responsibility to free the List.  */

-(BOOL ) isNoteSenderPresent:aNoteSender; 
 /* TYPE: Querying
  * Returns YES if aNoteSender is a member of the receiver's NoteSender
  * List.  */

- freeNoteSenders; 
 /* TYPE: Modifying
  * Disconnects and frees the receiver's NoteSenders.
  * Returns the receiver.
  */

- removeNoteSenders; 
 /* TYPE: Modifying
  * Removes the receiver's NoteSenders (but doesn't free them).
  * Returns the receiver.
  */

- noteSender; 
 /* TYPE: Querying
  * Returns the first NoteSender in the receiver's List.  This is a convenience
  * method provided for Performers that create and add a single NoteSender.
  */

- removeNoteSender:aNoteSender; 
 /* TYPE: Modifying
  * Removes aNoteSender from the receiver.  The receiver must be inactive.
  * If the receiver is currently in performance, or if aNoteSender wasn't
  * part of its NoteSender List, returns nil.  Otherwise returns the
  * receiver.  */

- addNoteSender:aNoteSender; 
 /* TYPE: Modfifying 
  * Adds aNoteSender to the recevier.  The receiver must be inactive.  If
  * the receiver is currently in performance, or if aNoteSender already
  * belongs to the receiver, returns nil.  Otherwise returns the receiver.
  */

- setConductor:aConductor; 
 /* TYPE: Modifying;
  * Sets the receiver's Conductor to aConductor.
  */

- conductor; 
  /* TYPE: Accessing the Conductor;  Returns the receiver's Conductor.
   * Returns the receiver's Conductor.
   */

- activateSelf; 
 /* TYPE: Performing; Does nothing; subclass may override for special behavior.
  * You never invoke this method directly; it's invoked automatically from
  * the activate method.  A subclass can implement this method to perform
  * pre-performance activities.  In particular, if the subclass needs to
  * alter the initial nextPerform value, it should be done here.  If
  * activateSelf returns nil, the receiver is deactivated.  The default
  * does nothing and returns the receiver.  */

- deactivateSelf; 
 /* TYPE: Performing; Does nothing; subclass may override for special
  * behavior.  You never invoke this method directly; it's invoked
  * automatically from the deactivate method, A subclass can implement
  * this method to perform post-performance activities.  The return value
  * is ignored.  The default does nothing and returns the receiver.  */

- perform; 
 /* TYPE: Performing; Subclass responsibility; sends Notes and sets nextPerform.
  * This is a subclass responsibility expected to send a Note and then set the
  * value of nextPerform.  The return value is ignored.  
  */

- setTimeShift:(double )timeShift;
 /* TYPE: Modifying; Delays performance for shift beats.
  * Shifts the receiver's performance time by timeShift beats.  The
  * receiver must be inactive.  Returns nil if the receiver is currently
  * in performance, otherwise returns the receiver.  */

- setDuration:(double )dur; 
 /* TYPE: Modifying;
  * Sets the receiver's maximum performance duration to dur in beats.  The
  * receiver must be inactive.  Returns nil if the receiver is currently
  * in performance, otherwise returns the receiver.  */

-(double ) timeShift;
 /* TYPE: Querying; Returns the receiver's time shift value.
 */

-(double ) duration; 
 /* TYPE: Querying; Returns the receiver's duration value.
 */

-(int ) status; 
 /* TYPE: Querying; Returns the receiver's status.
 */

-(int ) performCount; 
 /* TYPE: Querying; Returns the number of Notes the receiver has performed.
Returns the number of perform messages the receiver has 
recieved in the current performance.
 */

- activate; 
 /* TYPE: Performing; Prepares the receiver for a performance.
  * If the receiver isn't inactive, immediately returns the receiver; if
  * its duration is less than or equal to 0, immediately returns nil.
  * Otherwise prepares the receiver for a performance by setting
  * nextPerform to 0.0, performCount to 0, invoking activateSelf,
  * scheduling the first perform message request with the Conductor, and
  * setting the receiver's status to MK_active.  If a subclass needs to
  * alter the initial value of nextPerform, it should do so in its
  * implementation of the activateSelf method.  Also sends [delegate
  * hasActivated:self]; Returns the receiver.  */

- deactivate; 
 /* TYPE: Performing; Removes the receiver from the performance.
  * If the receiver's status is inactive, this does nothing and
  * immediately returns the receiver.  Otherwise removes the receiver from
  * the performance, invokes deactivateSelf, and sets the receiver's
  * status to MK_inactive.  Also sends [delegate hasDeactivated:self];
  * Returns the receiver.  */

- init; 
 /* TYPE: Modifying; Initializes the receiver.
  * Initializes the receiver.  You never invoke this method directly.  A
  * subclass implementation should send [super init] before
  * performing its own initialization.  The return value is ignored.  */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

- pause; 
 /* TYPE: Performing; Suspends the the receiver's performance.
  * Suspends the receiver's performance and returns the receiver.  To free
  * a paused Performer during a performance, you should first send it the
  * deactivate message.  Also sends [delegate hasPaused:self]; */

-pauseFor:(double)beats;
 /* TYPE: Performing; Suspends the receiver's performance for beats.
  * Like pause, but also enqueues a resume message to be sent the specified
  * number of beats into the future. */

- resume; 
 /* TYPE: Performing; Resumes the receiver's performance.
  * Resumes the receiver's performance and returns the receiver.  Also
  * sends [delegate hasResumed:self]; */

-copyFromZone:(NXZone *)zone;;
 /* TYPE: Copying: Returns a copy of the receiver.
  * Creates and returns a new, inactive Performer as a copy of the
  * receiver.  The new object has the same time shift and duration as the
  * reciever.  Its time and nextPerform variables are set to 0.0.  The new
  * object's NoteSenders are copied from the receiver.  
  * Note that you shouldn't send init to the new object. */

- copy; 
 /* TYPE: Copying
    Same as [self copyFromZone:[self zone]] 
  */

- free; 
 /* TYPE: Creating
  * Frees the receiver and its NoteSenders. The receiver must be inactive.
  * Does nothing and returns nil if the receiver is currently in
  * performance.  */

-(double ) time; 
 /* TYPE: Accessing time; Returns the receiver's latest performance time.
  * Returns the time, in beats, that the receiver last received the
  * perform message.  If the receiver is inactive, returns MK_ENDOFTIME.
  * The return value is measured from the beginning of the performance and
  * doesn't include any time that the receiver has been paused.  */

- setDelegate:object;
- delegate;

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives noteSender List, timeShift, and duration. Also optionally 
     archives conductor and delegate using NXWriteObjectReference().
     */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     Note that the status of an unarchived Performer is always MK_inactive.
     Note also that -init is not sent to newly unarchived objects.
     See write:. */
-awake;
 /* TYPE: Archiving; 
   Gets newly unarchived object ready for use. 
   */

-(BOOL)inPerformance;
 /* Returns YES if receiver's status is not MK_inactive  */

@end

/* Describes the protocol that may be implemented by the delegate: */
#import "PerformerDelegate.h"



