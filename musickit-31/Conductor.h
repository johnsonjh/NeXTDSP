/*
  Conductor.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>
#import <cthreads.h>

 /* The Conductor message structure.  All fields are private and
  * shouldn't be altered directly from an application. 
  */
typedef struct _MKMsgStruct { 
    double _timeOfMsg;     
    SEL _aSelector;       
    id _toObject;	       
    int _argCount;             
    id _arg1;
    id _arg2;
    struct _MKMsgStruct *_next;	
    IMP _methodImp;        
    id *_otherArgs;
    BOOL _conductorFrees;  
    BOOL _onQueue;      
    struct _MKMsgStruct *_prev;
    id _conductor;
} MKMsgStruct;

 /* C functions that create and manipulate MKMsgStructs. 
  * The functionality they provide is largely subsumed by the methods 
  * defined by Conductor.  However, you can use these functions to 
  * directly schedule message requests. 
  * See the C functions chapter for details.
  */

 /* Create new MKMsgStruct, but doesn't schedule it. */ 
extern MKMsgStruct 
  *MKNewMsgRequest(double timeOfMsg,SEL whichSelector,id destinationObject,
		   int argCount,...);

 /* Schedule MKMsgStructPtr based on info specified to MKNewMsgStruct() */ 
extern void 
  MKScheduleMsgRequest(MKMsgStruct *aMsgStructPtr, id conductor);

 /* Cancels and frees MKMsgStructPtr. */
extern MKMsgStruct *
  MKCancelMsgRequest(MKMsgStruct *aMsgStructPtr);

 /* Cancels the previous request and schedules a new one. The returned value
  * is not necessarily the same as the argument. */
extern MKMsgStruct *
  MKRescheduleMsgRequest(MKMsgStruct *aMsgStructPtr,id conductor,
			 double timeOfNewMsg,SEL whichSelector,
			 id destinationObject,int argCount,...);

 /* Same as MKRescheduleMsgRequest(), but uses all the old arguments. */
extern MKMsgStruct *
  MKRepositionMsgRequest(MKMsgStruct *aMsgStructPtr,double newTimeOfMsg);

@interface Conductor : Object
/* The Conductor class controls a Music Kit performance.  Each instance of
 * Conductor contains a message queue, a list of messages that are to be sent 
 * to particular objects at specific times.  The messages in the queue are 
 * sorted according to the times that they are to be sent.  When a Music Kit 
 * performance starts, the Conductor instances begin sending the messages in 
 * their queues. While methods are provided for adding messages directly to a 
 * Conductor's queue, this is usually done automatically; for example, each 
 * time a Performer receives the perform message, it automatically enqueues a 
 * request for another invocation of perform (see the Performer class).
 * 
 * The rate or tempo at which a Conductor processes its queue can be set 
 * through either of two methods:
 * 
 * * setTempo: sets the rate as beats per minute.
 * * setBeatSize: sets the size of an individual beat, in seconds.
 *
 * You can change a Conductor's tempo anytime, even during a performance.
 * However, each Conductor can have only one tempo at a time.  If your 
 * application uses multiple simultaneous tempi, you need to create more than 
 * one Conductor, one for each tempo.  A Conductor's tempo is initialized to 
 * 60 beats per minute.
 *
 * Every Performer object is controlled by an instance of Conductor, as set
 * through Performer's setConductor: method.  The Performer's perform
 * message requests are enqueued with this Conductor.  As a convenience, the 
 * Music Kit automatically creates an instance of Conductor called the 
 * defaultConductor; if you don't set a Performer's Conductor directly, 
 * it's controlled by the defaultConductor (most applications only need one 
 * tempo at a time, so the defaultConductor is usually sufficient.)  
 * You can retrieve the defaultConductor
 * by sending the defaultConductor message to the Conductor class.  All
 * performances also have a clockConductor, which you can retrieve through the
 * clockConductor class method.  The clockConductor has an unchangeable
 * tempo of 60 beats per minute and can be used to enqueue messages that must 
 * be sent out at times that aren't affected by tempo.
 *
 * The Conductor class responds to messages that control an entire
 * performance:
 *
 * * startPerformance starts a performance.
 * * pausePerformance temporarily suspends the performance.
 * resumePerformance resumes a paused performance.
 * finishPerformance stops the performance completely.
 * 
 * Normally, a performance ends when all the Conductors' message queues are 
 * empty. However, if you send setFinishWhenEmpty:NO to the Conductor class, 
 * the performance keeps going until the Conductor receives the
 * finishPerformance message.  This is useful if your Performers create
 * Notes based on mouse or keyboard events, where it's not unusual for the 
 * message queues to be empty as they wait for the generation of another Note. 
 *
 * The begin time of an individual Conductor can be offset from the beginning 
 * of the entire performance by sending it the setTimeOffset: message.  The
 * argument to this method is the number of seconds the instance waits before 
 * it starts to process its queue.  You can't offset the clockConductor's 
 * begin time.
 *
 * Each Conductor instance, except the clockConductor, can be paused and 
 * resumed and each has a notion of the current time in beats, as returned by 
 * sending it the time message.  This value is the amount of time the receiver
 * has spent in performance and doesn't include the receiver's time offset 
 * nor any time it spends in a paused state.  You can get the absolute time 
 * in the performance by sending time to the clockConductor or by calling the C
 * function MKGetTime().  Both return the current time in seconds, not
 * including the performance's delta time.
 *
 * The setClocked: class method determines whether a performance is clocked
 * or unclocked.  In clocked mode, the default, the Conductor operates in real
 * time, allowing you to interactively control the performance with the mouse 
 * or from the keyboard.  A clocked performance expects a running Application 
 * object to be present (this is provided for you by Interface Builder).  You 
 * should send adjustTime to the Conductor class when responding to an 
 * asychronous event; adjustTime synchronizes all the Conductors, bringing 
 * them into step with the current time. (See also +lockPerformance).
 *
 * In an unclocked performance, messages are sent as quickly as possible, one
 * after another.  This is useful, for example, for writing files.  While you
 * can't interactively control an unclocked performance, it can still be used 
 * to synthesize music on the DSP.  Timing is handled by the DSP rather
 * than by the Music Kit.  An unclocked performance always ends when the 
 * queues are empty. 
 *
 * Most clocked performances actually run slightly ahead of real time; there's
 * a lag between the time that a message is sent and the time that the 
 * associated musical note is realized.  As this time lag, or delta time 
 * becomes larger, DSP synthesis (or MIDI) becomes more dependable, but at 
 * the cost of decreased responsiveness.  
 * You can set a performance's delta time by 
 * passing an argument, in seconds, to the C function MKSetDeltaT().  
 * You should also set the delta time for an unclocked performance.  Since an 
 * unclocked performance doesn't support interaction, its delta time can 
 * usually be arbitrarily large (a delta time of a second is considered to be 
 * large).  In the case of an unclocked performance, the delta time is 
 * merely an offset on the performance times that is added in before the
 * commands are sent to the DSP or MIDI.
 * Delta time isn't added into the value returned by the time 
 * methods or by the MKGetTime() function. 
 *
 * CF:  Performer
 */
{
    double time;        /* Current time in beats. */
    double nextMsgTime; /* Time, in seconds, when the object is scheduled 
                           to send its next message. */
    double beatSize;    /* The duration of a single beat, in seconds. */ 
    double timeOffset;  /* Performance time offset, in seconds. */
    BOOL isPaused;      /* YES if this object is currently paused. */
    id delegate;        /* Conductor's delegate. */
    MKMsgStruct *_reservedConductor1; 
    id _reservedConductor2;
    id _reservedConductor3;    
    double _reservedConductor4;
    void *_reservedConductor5;
}
 
 /* METHOD TYPES
  * Creating and freeing a Conductor
  * Querying the object
  * Modifying the object
  * Controlling a performance
  * Manipulating time
  * Requesting messages
  */
 
+ new; 
 /* TYPE: Creating; Creates a new Conductor.
  * Creates a new Conductor in the default zone and sends init to the new
  * object.
  * If a performance is currently in progress, this does nothing and returns
  * nil. */

+allocFromZone:(NXZone *)zone;
 /* TYPE: Creating; Creates a new Conductor
  * Creates a new Conductor in the specified zone and sends.
  * If a performance is currently in progress, this does nothing and returns
  * nil. */

+alloc;
 /* Same as allocFromZone:, but uses the default zone. */

- init;
 /* Inits a new Conductor object with a tempo of 60 beats per min.
  */

+ adjustTime; 
 /* TYPE: Man; Updates every Conductor's notion of time.
  * Updates every Conductor's notion of time.  This method should be invoked
  * whenever an asychronous action -- such as a mouse or keyboard event -- that
  * affects the performance takes place.  Returns the receiver.  
  * See also +lockPerformance.
  */

+ startPerformance; 
 /* TYPE: Control; Starts a performance.
  * Starts a performance.  All Conductor objects begin at the same time.  If the
  * performance is clocked and you don't have a running Application object 
  * (NXApp),
  * this does nothing and returns nil; otherwise it returns the receiver.  
  */

+ defaultConductor; 
 /* TYPE: Querying; Returns the defaultConductor.  */

+(BOOL) inPerformance; 
 /* TYPE: Querying; Returns YES if a performance is in session.
  * Returns YES if a performance is currently taking place, otherwise returns
  * NO.  
  */

+ finishPerformance; 
 /* TYPE: Control; Ends the performance.
  * Ends the performance.  All enqueued messages are removed and the
  * afterPerformance messages are sent by the Conductor class.  If
  * finishWhenEmpty is YES, this message is automatically sent when all
  * message queues are exhausted.  Returns nil.  
  */

+ pausePerformance; 
 /* TYPE: Controlling; Pauses the performance.
  * Pauses the performance.  The performance is suspended until Conductor class
  * receives the resumePerformance message.  The receiver's notion of time is
  * suspended until the performance is resumed.  You can't pause an unclocked
  * performance; returns nil if the performance is unclocked.  Otherwise
  * returns the receiver.  
  */

+(BOOL) isPaused; 
 /* TYPE: Querying; YES if the performance is paused.
  * Returns YES if the receiver is paused, otherwise returns NO.  
  */

+ resumePerformance; 
 /* TYPE: Controlling; Resumes the performance.
  * Resumes the receiver, allowing the performance to continue from where
  * it was paused.  Returns the receiver.
  */

+ currentConductor; 
 /* TYPE: Querying; Returns the Conductor that's sending a message, if any.
  * Returns the Conductor instance that's currently sending a message, or
  * nil if no message is being sent.
  */

+ clockConductor;
 /* TYPE: Querying; Returns the clockConductor.  */

+ setClocked:(BOOL)yesOrNo; 
 /* TYPE: Modifying; Establishes clocked or unclocked performance.  
  * If yesOrNo is YES (the default), messages are dispatched at
  * specific times, using the Application object's event loop to process
  * user-initiated events.  If NO, messages are sent as quickly as possible
  * and the user interface is effectively disabled until the performance is over.
  * Does nothing and returns nil if a performance is in progress, otherwise
  * returns the receiver.  
  */

+(BOOL) isClocked; 
 /* TYPE: Querying; YES if performance is clocked, NO if not.
  * Returns YES if the performance is clocked, NO if it isn't.
  * By default, a performance is unclocked.
  */

+ setFinishWhenEmpty:(BOOL)yesOrNo; 
 /* TYPE: Modifying; If arg is YES, the performance continues despite empty 
  * queues.
  * If yesOrNo is YES (the default), the performance is terminated when
  * all the Conductors' message queues are empty.  If NO, the performance
  * continues until the finishPerformance message is sent to the Conductor 
  * class.  
  */

+(BOOL) isEmpty;
 /* TYPE: Querying; YES if in performance and all queues are empty.
  */

+(BOOL) finishWhenEmpty;
 /* TYPE: Querying; YES if the performance will finish when the queues are empty.
  * Returns YES if the performance will finish when all Conductors' message
  * queues are empty, NO if it won't.  
  */

- copy;
 /* TYPE: Creating; Same as [self copyFromZone:[self zone]] */

- copyFromZone:(NXZone *)zone;
 /* Returns a new Conductor created through 
    [Conductor allocFromZone:zone] and initializes it. */

-(BOOL) isPaused; 
 /* TYPE: Querying; Returns YES if the receiver is paused.  */

- pause; 
 /* TYPE: Controlling; Pauses the receiver's performance.
  * Pauses the performance of the receiver.  The effect is restricted to the
  * present performance.  Invoke resume to unpause a Conductor.  Returns the
  * receiver.  
  */

-pauseFor:(double)seconds;
 /* TYPE: Controlling; Pauses the receiver's performance.
  * Like pause, but also enques a resume message to itself at a time seconds
  * into the future (i.e. seconds is relative to the value of [Conductor time]).
  * If there is already a pauseFor: request, the new one supercedes the old one.
  * If the performance is not yet in progress, the resume is enqueued for 
  * seconds into the performance. 
  */

- resume; 
 /* TYPE: Controlling; Resumes a paused receiver.
  * Resumes the receiver's performance and returns the receiver.  If the receiver
  * isn't currently paused (it hadn't previously received the pause message),
  * this has no effect.  
  */

-(double) setBeatSize:(double)newBeatSize; 
 /* TYPE: Man; Sets the tempo by resizing the beat.
  * Sets the tempo by changing the size of a beat to newBeatSize, measured in
  * seconds.  The default beat size is 1.0 (one second).  Attempts to set
  * the tempo of the clockConductor are ignored.  Returns the previous beat size.
  */

-(double) beatSize; 
 /* TYPE: Querying; Returns the size of the receiver's beat in seconds.  */

-(double) setTempo:(double)newTempo; 
 /* TYPE: Man; Sets the tempo in beats per minute.
  * Sets the tempo to newTempo, measured in beats per minute.  Implemented as
  * [self setBeatSize:(60.0/newTempo)].  Attempts to set the
  * tempo of the clockConductor are ignored.  Returns the previous tempo.  
  */

-(double) tempo; 
 /* TYPE: Querying; Returns the receiver's tempo in beats per minute.  */

-(double) setTimeOffset:(double)newTimeOffset; 
 /* TYPE: Man; Sets the receiver's performance time offset in seconds.
  * Sets the receiver's performance time offset to newTimeOffset seconds.
  * Keep in mind that since the offset is measured in seconds,
  * it's not affected by the receiver's tempo.  Attempts to set the offset of the
  * clockConductor are ignored.  Returns the previous offset.  
  */

-(double) timeOffset; 
 /* TYPE: Querying; Returns the receiver's performance time offset in seconds.  */

- sel:(SEL)aSelector to:toObject withDelay:(double)beats argCount:(int)argCount,...;
 /* TYPE: Requesting; Sends aSelector to toObject after beats beats.
  * Places, in the receiver's queue, a request for aSelector to be sent to
  * toObject at time beats beats from the receiver's notion of the
  * current time.  To ensure that the receiver's notion of time is up-to-date, you
  * should send adjustTime before invoking this method.  argCount
  * specifies the number of four-byte arguments to aSelector
  * followed by the arguments themselves, seperated by commas (two arguments,
  * maximum).  
  */

- sel:(SEL)aSelector to:toObject atTime:(double)time argCount:(int)argCount,...;
 /* TYPE: Requesting; Sends aSelector to toObject at time time.
  * Places, in the receiver's queue, a request for aSelector to be sent to
  * toObject at time time beats from the beginning of the performance.
  * argCount specifies the number of four-byte arguments to
  * aSelector followed by the arguments themselves, seperated by commas (two
  * arguments, maximum).  
 */

-(double) predictTime:(double)beatTime; 
 /* TYPE: Man; Returns predicted time in seconds corresponding to beatTime.
  * Returns the time, in seconds, when beat beatTime should occur.  The
  * default implementation simply returns beatTime multiplied by the
  * receiver's beat size.  You can subclass this method to provide a more
  * sophisticated time mapping.  
  */

+(double) time; 
 /* Returns the time in seconds as viewed by the clock conductor.
  *   Same as [[Conductor clockConductor] time]. Returns MK_NODVAL if not in
  *   performance. Use MKIsNoDVal() to check for this return value.  */

-(double) time; 
 /* TYPE: Querying; Returns the receiver's notion of the current time in beats.
  * Returns the receiver's notion of the current time in beats. 
  */

- emptyQueue; 
 /* TYPE: Modifying; Empties the receiver's message queue.
  * Empties the receiver's message queue and returns the receiver.  Doesn't
  * send any of the messages.
  */

-(BOOL) isCurrentConductor;
 /* TYPE: Querying; YES if the receiver is sending a message.
  * Returns YES if the receiver is currently sending a message.  
  */

+(MKMsgStruct *) afterPerformanceSel:(SEL)aSelector to:toObject argCount:(int)argCount,...; 
 /* TYPE: Requesting; Sends aSelector to toObject after the performance.
  * Places, in a special queue, a request for aSelector to be sent to
  * toObject immediately after the performance ends.  argCount
  * specifies the number of four-byte arguments to aSelector followed by the
  * arguments themselves, seperated by commas (two arguments, maximum).  You can
  * enqueue as many of these requests as you want; they're sent in the order that
  * they were enqueued.  Returns a pointer to an MKMsgStruct that can be
  * passed to cancelMsgRequest:.  
  */

+(MKMsgStruct *) beforePerformanceSel:(SEL)aSelector to:toObject argCount:(int)argCount,...; 
 /* TYPE: Requesting; Sends aSelector to toObject after the performance.
  * Places, in a special queue, a request for aSelector to be sent to
  * toObject immediately just as the performance begins.  argCount
  * specifies the number of four-byte arguments to aSelector followed by the
  * arguments themselves, seperated by commas (two arguments, maximum).  You can
  * enqueue as many of these requests as you want; they're sent in the order that
  * they were enqueued.  Returns a pointer to an MKMsgStruct that can be
  * passed to cancelMsgRequest:.  
  */

- setDelegate:object;
  /* Set instance delegate. See ConductorDelegate.h */
- delegate;
  /* Return instance delegate. See ConductorDelegate.h */

+ setDelegate:object;
  /* Set class delegate. See ConductorDelegate.h */
+ delegate;
  /* Return class delegate. See ConductorDelegate.h */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object.
   * You never send this message directly.  
   * Archives beatSize and timeOffset. Also archives whether this
   * was the clockConductor or defaultConductor.
   */
- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads object.
  *   You never send this message directly.  
  *   Should be invoked with NXReadObject(). 
  *   Attempts to read the defaultConductor or clockConductor are handled
  *   specially, as described in finishUnarchiving. 
  */
-finishUnarchiving;
 /* TYPE: Archiving; 
  * If the unarchived object was the clockConductor, frees the new object
  * and returns the clockConductor. Otherwise, if there is a performance
  * going on, frees the new object and returns the defaultConductor. 
  * Otherwise, if the unarchived object was the defaultConductor, sets the 
  * defaultConductor's beatSize and timeOffset from the unarchived object,
  * frees the new object and returns the defaultConductor.
  * Otherwise, the new unarchived Conductor is added to the Conductor
  * list and nil is returned. In any case, NXReadObject() returns a valid
  * Conductor. */

+ useSeparateThread:(BOOL)yesOrNo;
 /* TYPE: Modifying;
  * If invoked with an argument of YES, all following performances will
  * be run in a separate Mach thread.  Some restrictions apply to 
  * separate-threaded performances.  See the Technical Documentation for 
  * details.  Default is NO.
  */

+ lockPerformance;
 /* In a separate-threaded performance, this method gets the Music Kit
  * lock, then sends [Conductor adjustTime].  
  * See the Technical Documentation for details. 
  * LockPerformance may be called multiple times -- e.g. if you lock
  * twice you must unlock twice to give up the lock. 
  * In a performance that is not separate-threaded, this method is the same as 
  * +adjustTime. 
  */
 
+ unlockPerformance;
 /* Undoes lockPerformance. 
  * In a separate-threaded performace, sends [Orchestra flushTimedMessages]
  * and then gives up the Music Kit lock. 
  * In a performance that is not separate-threaded, this method is the same as
  * Orchestra's flushTimedMessages.  (See Orchestra.h.)
  */

+ (BOOL)lockPerformanceNoBlock;
 /* Same as lockPerformance but does not wait and returns NO if the lock is 
  * unavailable.  If the lock is successful, sends [Conductor adjustTime] 
  * and returns YES. You rarely use this method.  It is provided for cases 
  * where you would prefer to give up than to wait (e.g. when simultaneously 
  * doing graphic animation.)
  */

+ setThreadPriority:(float)priorityFactor;
 /*
  * This method sets the thread priority of the following and all
  * subsequent performances.  The priority change takes effect when the
  * startPerformance method is invoked and is set back to its original
  * value in the finishPerformance method.  In a separate-threaded
  * performance, the thread that is affected is the performance thread.
  * In the case of a performance that is not separate-threaded, the thread
  * affected is the one that invoked the startPerformance method.
  * Priority is specified as a "priorityFactor" between 0.0 and 1.0.  1.0 
  * corresponds to the maximum priority of a user process, 0.0 corresponds 
  * to the base priority. The default value is 0.0.
  * 
  * In addition, if priorityFactor is greater than 0, and the application
  * is running as root, the Music Kit uses Mach's "fixed priority thread
  * scheduling policy".  (See the Mach documentation for details on thread
  * scheduling policies. ) This scheduling policy is more advantageous for
  * real-time processes than the ordinary time sharing policy. To run your
  * program as root:
  *    1) su to root
  *    2) chown root <yourApp>
  *    3) chmod u+s <yourApp>
  *
  * Note that when a program running as root writes files, these files are owned
  * by root. If you would like to write files that are owned by the user who
  * ran the program, but would also like to use fixed priority scheduling, first
  * set the thread priority and then set the user ID. Example:
  *
  *    [Conductor setThreadPriority:1.0];
  *    setuid(getuid());
  *
  * Keep in mind that you should only run your program as root once you are sure
  * it is fully debugged.
  */

+ (cthread_t) performanceThread;
 /* In a separate-threaded Music Kit performance, returns the c-thread
  * used in that performance.  When the thread has exited, returns
  * NO_CTHREAD. */

@end

 /* Describes the protocol that may be implemented by the delegate: */
#import "ConductorDelegate.h"



