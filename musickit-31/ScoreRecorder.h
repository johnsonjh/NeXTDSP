/*
  ScoreRecorder.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
  */

#import <objc/Object.h>

@interface ScoreRecorder : Object
/*
 * 
 * A ScoreRecorder is a pseudo-Instrument that adds Notes to the Parts in
 * a given Score.  It does this by creating a PartRecorder, a true
 * Instrument, for each of the Score's Part objects.  A ScoreRecorder's
 * Score is set through the setScore: method.  If you add Parts to or
 * remove Parts from the Score after sending the setScore: message, the
 * changes will not be seen by the ScoreRecorder.  For example, if you
 * add a Part to the Score, the ScoreRecorder won't create an additional
 * PartRecorder for that Part.
 * 
 * A ScoreRecorder can access a PartRecorder by the name of the Part with
 * which it's associated.  It can also set the time unit of all its
 * PartRecorders through a single message, setTimeUnit:.
 * 
 * A ScoreRecorder is said to be in performance from the time any of its
 * PartRecorders receives a Note until the performance is finished.
 * 
 * CF: PartRecorder
 */
{
    id partRecorders; /* The object's Set of PartRecorders. */
    id score; /* The object's Score. */
    MKTimeUnit timeUnit; /* Unit the object's PartRecorders use to 
                            measure time; one of MK_second or MK_beat. */
    id partRecorderClass; /* The PartRecorder subclass used. */
    BOOL _reservedScoreRecorder1;
    BOOL _reservedScoreRecorder2;
    void *_reservedScoreRecorder3;
}
 /* METHOD TYPES
  * Creating and freeing a ScoreRecorder
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

- setScore:aScore; 
 /* TYPE: Modifying; Sets the receiver's Score to aScore.
  * Removes and frees the receiver's current PartRecorders, sets its Score
  * to aScore, and then creates and adds a PartRecorder for each Part in
  * the Score.  Subsequent changes to aScore (adding or removing Parts)
  * aren't seen by the receiver.  If the receiver is in performance, this
  * does nothing and returns nil, otherwise it returns the receiver.
  * 
  * If you want to set the Score without freeing the current PartRecorders you
  * should send removePartRecorders before invoking this method; the
  * PartRecorders are then removed but not freed.  
  */

- score; 
 /* TYPE: Querying; Returns the receiver's Score. */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Creates and returns a ScoreRecorder as a copy of the receiver.
  * Creates and returns a ScoreRecorder as a copy of the receiver.  The
  * new object has the same Score as the receiver, but contains its own
  * set of PartRecorders.
  */

-copy;
 /* Same as [self copyFromZone:[self zone]]; */

- freePartRecorders;
 /* TYPE: Modifying; Frees the receiver's PartRecorders and sets its Score to nil.
  * Frees the receiver's PartRecorders and sets its Score to nil.
  * Returns the receiver.  
  */

- removePartRecorders; 
 /* TYPE: Modifying; Removes the receiver's PartRecorders and sets its Score to nil.
  * Removes the receiver's PartRecorders and sets its Score to nil.  (The
  * PartRecorder objects aren't freed.)  Returns the receiver.  
  */

- free; 
 /* TYPE: Creating; Frees the receiver and its PartRecorders. 
  * Frees the receiver and its PartRecorders.  If you want to free the
  * receiver only, send removePartRecorders to the receiver before
  * invoking this method.
  */

- (MKTimeUnit)timeUnit;
 /* TYPE: Querying; Returns the receiver's time unit, either MK_second or MK_beat. */

- setTimeUnit:(MKTimeUnit)aTimeUnit;
 /* TYPE: Modifying; Forwards setTimeUnit:aTimeUnit to the receiver's PartRecorders.
  * Sets the receiver's time unit to aTimeUnit, one of MK_beat and
  * MK_second, and forwards the setTimeUnit:aTimeUnit message to the
  * receiver's PartRecorders.  If the receiver is in performance, this
  * does nothing and returns nil.  Otherwise returns the receiver.
  */

- partRecorders; 
 /* TYPE: Modifying; Returns the receiver's PartRecorders.
  * Returns a List object that contains the receiver's PartRecorders.
  * It's the sender's responsibility to free the List.
  */

- (BOOL )inPerformance;
 /* TYPE: Performing; YES if the receiver is in performance.  
  * Returns YES if the receiver is in performance, otherwise returns NO.
  */

- firstNote:aNote; 
 /* TYPE: Performing; You never invoke this method.
  * You never invoke this method; it's invoked automatically when the
  * first Note is received by any of the receiver's PartRecorders.  The
  * default does nothing; a subclass can implement this method for
  * performance initialization.  The returns value is ignored.
  */

- afterPerformance; 
 /* TYPE: Performing; You never invoke this method.
  * You never invoke this method; it's invoked automatically at the end of
  * the performance.  The default does nothing; a subclass can implement
  * this method for post-performance cleanup.  A subclass version should
  * always invoke [super afterPerformance].  The return value is ignored.
  */

- noteReceivers; 
 /* TYPE: Querying; Returns a List of the receiver's NoteReceivers.
  * Returns a List object that contains the receiver's NoteReceivers.  You
  * must free the List yourself when you're done with it.
  */

- partRecorderForPart:aPart; 
 /* TYPE: Querying; Returns the receiver's PartRecorder for aPart.
  * Returns the receiver's PartRecorder for aPart, or nil if not found.
  */ 

-setPartRecorderClass:aPartRecorderSubclass;
 /* Normally, ScoreRecorders create instances of the PartRecorder class.
   This method allows you to specify that instances of some PartRecorder
   subclass be created instead. If aPartRecorderSubclass is not 
   a subclass of PartRecorder (or PartRecorder itself), this method has 
   no effect and returns nil. Otherwise, it returns self.
  */
-partRecorderClass;
 /* Returns the class used for PartRecorders, as set by 
   setPartRecorderClass:. The default is PartRecorder itself. */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives partRecorders, timeUnit and partRecorderClass.
     Also optionally archives score using NXWriteObjectReference().
     */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked with NXReadObject(). 
     */

@end



