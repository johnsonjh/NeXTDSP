/*
  ScorePerformer.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>

@interface ScorePerformer : Object
/* 
 * 
 * A ScorePerformer performs a Score object by creating a group of
 * PartPerformers, one for each Part in the Score, and controlling the
 * group's performance.  ScorePerformer itself isn't a Performer but it
 * does define a number of methods, such as activate, pause, and resume,
 * that resemble Performer methods.  When a ScorePerformer receives such
 * a message, it simply forwards it to each of its PartPerformer objects,
 * which are true Performers.
 * 
 * ScorePerformer also has a Performer-like status; it can be active,
 * inactive, or paused.  The status of a ScorePerformer is, in general,
 * the same as the status of all of its PartPerformers.  For instance,
 * when you send the activate message to a ScorePerformer, its status
 * becomes MK_active as does the status of all its PartPerformers.
 * However, you can access and control a PartPerformer independent of the
 * ScorePerformer that created it.  Thus, an individual PartPerformer's
 * status can be different from that of the ScorePerformer.
 * 
 * A ScorePerformer's score is set and its PartPerformers are created
 * when it receives the setScore: message.  If you add Parts to or remove
 * Parts from the Score after sending the setScore: message, the changes
 * will not be seen by the ScorePerformer.
 * 
 */ 
{
    MKPerformerStatus status; /* The object's status. */     
    id partPerformers; /* A List of the object's PartPerformer instances. */
    id score; /* The Score with which this object is associated. */     
    double firstTimeTag; /* Smallest timeTag considered for performance. */
    double lastTimeTag; /* Greatest timeTag considered for performance. */
    double timeShift; /* Performance time offset in beats. */
    double duration; /* Maximum performance duration in beats. */
    id conductor; /* The object's Conductor (its PartPerformers' Conductor).*/
    id delegate;  /* The object's delegate. */
    id partPerformerClass; /* The PartPerformer subclass used. */ 
    BOOL _reservedScorePerformer1;     
    MKMsgStruct * _reservedScorePerformer2;     
    void *_reservedScorePerformer3; 
}

 /* METHOD TYPES
  * Creating and freeing a ScorePerformer
  * Modifying the object
  * Querying the object
  * Performing the object
  */
+ new; 
 /* TYPE: Creating; Creates and inits new object.
  * Creates new object from default zone and sends initto the new object  */
   
- init; 
 /* TYPE: Modifying; Inits the receiver. 
  * Inits the receiver; you never invoke this method directly.
  * A subclass implementation
  * should send [super init] before performing its own initialization.
  * The return value is ignored.  
  */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

- freePartPerformers;
 /* TYPE: Modifying; Removes and frees the receiver's PartPerformers. 
  * Removes and frees the receiver's PartPerformers and sets the receiver's Score
  * to nil.  Returns the receiver.
  */
   
- removePartPerformers; 
 /* TYPE: Modifying; Removes the receiver's PartPerformers. 
  * Removes the receiver's PartPerformers (but doesn't free them) and sets the
  * receiver's Score to nil.  Returns the receiver.  
  */

- setScore:aScore; 
 /* TYPE: Modifying; Associates the receiver with the Score aScore. 
  * Sets the receiver's Score to aScore and creates a PartPerformer for
  * each of the Score's Parts.  Subsequent changes to aScore (by adding or
  * removing Parts) won't be seen by the receiver.  The PartPerformers
  * from a previously set Score (if any) are first removed and freed.
  * Returns the receiver.
  */

- score; 
 /* TYPE: Querying; Returns the object's Score. */
   
- activate; 
 /* TYPE:Performing;Activates the receiver's PartPerformers for a performance.
  * Sends activateSelf to the receiver and then sends the activate message
  * to each of the receiver's PartPerformers.  If activateSelf returns
  * nil, the message isn't sent and nil is returned.  Otherwise sends
  * [delegate hasActivated:self] and returns the receiver.
  */

- activateSelf; 
 /* TYPE: Performing; Receiver's activation routine; default does nothing. 
  * You never invoke this method directly; it's invoked as part of the
  * activate method.  A subclass implementation should send [super
  * activateSelf].  If activateSelf returns nil, the receiver isn't
  * activated.  The default implementation does nothing and returns the
  * receiver.
  */

- deactivate; 
 /* TYPE: Performing; Deactivates the receiver's PartPerformers. 
   Also sends [delegate hasDeactivated:self]. */

- deactivateSelf; 
 /* TYPE: Performing; Defines the receiver's deactivation routine. 
  * A subclass can implement this method to perform post-performance
  * activites.  The default does nothing; the return value is ignored.
  * You never invoke this method directly; it's invoked by the deactivate
  * method.
  */

- pause; 
 /* TYPE: Performing; Suspends the receiver's performance.
  * Suspends the receiver's performance by sending the pause message to
  * each of its PartPerformers.  Also sends [delegate hasPaused:self];
  * Returns the receiver.
  */
   
- resume; 
 /* TYPE: Performing; Resumes the receiver's performance. 
  * Resumes a previously paused performance by sending the resume message
  * to each of the receiver's PartPerformers.  Also sends [delegate
  * hasResumed:self]; Returns the receiver.
  */

- setFirstTimeTag:(double )aTimeTag; 
 /* TYPE: Modifying; Sets the smallest timeTag value considered for performance. 
  * Sets the smallest timeTag value considered for performance by sending
  * setFirstTimeTag:aTimeTag to each of the receiver's PartPerformers.
  * Returns the receiver.  If the receiver is active, this does nothing and returns
  * nil.  
  */

- setLastTimeTag:(double )aTimeTag; 
 /* TYPE: M; Sets the greatest timeTag value considered for performance. 
  * Sets the greatest timeTag value considered for performance by sending
  * setLastTimeTag:aTimeTag to each of the receiver's PartPerformers.
  * Returns the receiver.  If the receiver is active, this does nothing
  * and returns nil.
  */

-(double ) firstTimeTag; 
 /* TYPE:Q;Returns the smallest timeTag value considered for performance.*/
   
-(double ) lastTimeTag; 
 /* TYPE:Q;Returns the greatest timeTag value considered for performance.*/
   
- setTimeShift:(double )aTimeShift; 
 /* TYPE: M; Sets the receiver's performance time offset in beats. 
  * Sets the performance time offset by sending setTimeShift:aTimeShift to
  * each of the receiver's PartPerformers.  The offset is measured in
  * beats.  Returns the receiver.  If the receiver is active, this does
  * nothing and returns nil.
  */

- setDuration:(double )aDuration; 
 /* TYPE: M; Sets the receiver's maximum performance duration in beats. 
  * Sets the maximum performance duration by sending setDuration:aDuration
  * to each of the receiver's PartPerformers.  The duration is measured in
  * beats.  Returns the receiver.  If the receiver is active, this does
  * nothing and returns nil.
  */

-(double ) timeShift;
 /* TYPE: Q; Returns the receiver's performance time offset in beats. */
   
-(double ) duration; 
 /* TYPE: Q;Returns the receiver's maximum performance duration in beats.*/

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Returns a new ScorePerformer as a copy of the receiver. 
  * Creates and returns a new, inactive ScorePerformer that's a copy of
  * the receiver.  The new object is associated with the same Score as the
  * receiver, and has the same Conductor and timing window variables
  * (timeShift, duration, fromTimeTag, and toTimeTag).  New PartPerformers
  * are created for the new object.
  */

-copy;
 /* Same as [self copyFromZone:[self zone]]; */

- free; 
 /* TYPE: Creating; Frees the receiver and its PartPerformers. */
   
- setConductor:aConductor; 
 /* TYPE: Modifying; Sets the Conductor for the receiver's PartPerformers. 
  * Sends the message setConductor:aConductor to each of the receiver's
  * PartPerformers. 
  */

- partPerformerForPart:aPart; 
 /* TYPE: Querying; Returns the PartPerformer associated with aPart. 
  * Returns the receiver's PartPerformer that's associated with aPart,
  * where aPart is a Part in the receiver's Score.  Keep in mind that it's
  * possible for a Part to have more than one PartPerformer; this method
  * returns only the PartPerformer that was created by the receiver.
  */

- partPerformers; 
 /* TYPE: Querying; Returns a List of the receiver's PartPerformers.
  * Creates and returns a List containing the receiver's PartPerformers.
  * The sender is responsible for freeing the List.
  */
   
- noteSenders; 
 /* TYPE: Querying; Returns a List of the PartPerformer's NoteSender objects.
  * Creates and returns a List containing the NoteSender objects that
  * belong ot the receiver's PartPerformers.  (A PartPerformer contains at
  * most one NoteSender, created when the PartPerformer is initialized.)
  * The sender is responsible for freeing the List.
  */

-(int) status;

-setPartPerformerClass:aPartPerformerSubclass;
/* Normally, ScorePerformers create instances of the PartPerformer class.
   This method allows you to specify that instances of some PartPerformer
   subclass be created instead. If aPartPerformerSubclass is not 
   a subclass of PartPerformer (or PartPerformer itself), this method has 
   no effect and returns nil. Otherwise, it returns self.
  */
-partPerformerClass;
 /* Returns the class used for PartPerformers, as set by 
   setPartPerformerClass:. The default is PartPerformer itself. */

- setDelegate:object;
 /* Sets the receiver's delegate object. See PerformerDelegate.h */
- delegate;
 /* Returns the receiver's delegate object. See PerformerDelegate.h */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives partPerformers,firstTimeTag,lastTimeTag,timeShift,
     duration, and partPerformerClass. Also optionally archives score
     conductor and delegate using NXWriteObjectReference().
     */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Note that -init is not sent to newly unarchived objects.
     Should be invoked with NXReadObject(). 
     */
-awake;
  /* TYPE: Archiving; Gets unarchived object ready for use. 
     Sets conductor field to defaultConductor if it was nil. */

@end

/* Describes the protocol that may be implemented by the delegate: */
#import "PerformerDelegate.h"



