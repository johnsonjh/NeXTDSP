/*
  Orchestra.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>
#import "orch.h"
#import "devstatus.h"

@interface Orchestra : Object
/* 
 *  The Orchestra class manages DSP resources during a Music Kit performance.
 *  Each instance of Orchestra represents a single
 *  DSP, identified by orchIndex, a zero-based integer index.
 *  In the basic NeXT configuration, there's only one DSP so there's only one
 *  Orchestra instance (orchIndex 0).  
 *  
 *  There are two levels of allocation:  SynthPatch allocation and 
 *  UnitGenerator allocation. SynthPatches are higher-level entities, 
 *  collections of UnitGenerators. Both levels may be used at the same time.
 */
{
    double computeTime;   /* Time in seconds to compute one sample. */
    double samplingRate;  /* Sampling rate. */
    id stack;             /* Stack of UnitGenerators in order as they
                             appear in DSP memory. (A List object.) */
    char *outputSoundfile; /* For output sound samples. */
    char *inputSoundfile;  /* For processing sound samples with the DSP. */
    char *outputCommandsFile;/* For output DSP commands. */
    id xZero;         /* Special patchpoint in x memory that always holds 0. */
    id yZero;         /* Special patchpoint in y memory that always holds 0. */
    id xSink;         /* Special patchpoint in x memory that you never read. */
    id ySink;         /* Special patchpoint in y memory that you never read. */
    id sineROM;       /* Special read-only SynthData object used to represent
                         the sine ROM. */
    id muLawROM;      /* Special read-only SythData object used to represent
                         the mu-law ROM. */
    MKDeviceStatus deviceStatus; /* Status of this instance. */
    unsigned short orchIndex;    /* Index to the DSP that's managed by this 
                                    instance. */
    char isTimed;    /* YES if DSP commands are timed. */
    BOOL useDSP;     /* YES if running on a DSP (always YES in 2.0) */
    BOOL soundOut;   /* YES if sound is being sent to the DAC. */
    BOOL SSISoundOut; /* YES if sound is being sent to the DSP port. */
    BOOL isLoopOffChip; /* YES if the DSP code is running partially off-chip. */
    BOOL fastResponse;  /* YES if response latency should be minimized */
    double localDeltaT; /* positive offset in seconds added to out-going 
                           time-stamps */
    short onChipPatchPoints; /* Size of on-chip patch point partition. */
    id _reservedOrchestra1;
    int _reservedOrchestra2;
    void *_reservedOrchestra3;
    void  *_reservedOrchestra4;
    id _reservedOrchestra5;
    DSPAddress _reservedOrchestra6;
    DSPAddress _reservedOrchestra7;
    DSPAddress _reservedOrchestra8;
    DSPAddress _reservedOrchestra9;
    DSPAddress *_reservedOrchestra10;
    DSPAddress *_reservedOrchestra11;
    unsigned long _reservedOrchestra12;
    unsigned long _reservedOrchestra13;
    double _reservedOrchestra14;
    double _reservedOrchestra15;
    id _reservedOrchestra16;
    id _reservedOrchestra17;
    DSPFix48 _reservedOrchestra18;
    int _reservedOrchestra19;
    int _reservedOrchestra20;
    int _reservedOrchestra21;
    int _reservedOrchestra22;
    int _reservedOrchestra23;
    int _reservedOrchestra24;
    int _reservedOrchestra25;
    float _reservedOrchestra26;
    float _reservedOrchestra27;
    void *_reservedOrchestra28;
    void *_reservedOrchestra29;
}

 /* METHOD TYPES
  * Creating and freeing an Orchestra
  * Modifying the object
  * Querying the object
  */

+ initialize; 

-(MKDeviceStatus)deviceStatus;
 /* TYPE: Querying; Returns MKDeviceStatus of receiver. 
 * Returns the MKDeviceStatus of the receiver. 
 */

- setHeadroom:(double)headroom;
  /* TYPE: Modifying; Sets DSP computation headroom. 
   Sets the DSP's computation headroom,  
   adjusting the tradeoff between
   processing power and reliability.
   A value of -.5 runs a large risk of falling out of real time.
   A value of .5 runs a small risk of falling out of real time but 
   allows for a substantially less powerful Orchestra. 
   The default is .1. Since the timing estimates for UnitGenerators are worst-
   case estimates, it is often possible to set headroom negative without 
   falling out of real time.
   
   The effective sampling period is computed as 
   (1.0/samplingRate) * (1 - headroom).
   */

+ setHeadroom:(double)headroom;
  /* TYPE: Modifying; Sends setHeadroom: to all Orchestras. 
   Sends setHeadroom: to all Orchestras that have been created.
   */ 

-(double)headroom;
  /* TYPE: Querying; Returns the receiver's headroom. */

- beginAtomicSection;
  /* TYPE: Modifying; Marks the beginning of an atomic section.
   Marks the beginning of a section of DSP commands that are sent as 
   a unit. */

- endAtomicSection;
  /* TYPE: Modifying; Marks the end of an atomic section.
   Marks the end of a section of DSP commands that are sent as 
   a unit. */

+ new; 
 /* Returns and inits a new Orchestra for the default DSP, if one doesn't
    already exist. Otherwise returns the existing one. */
+ newOnDSP:(unsigned short)index; 
 /* Creates, inits and returns an Orchestra instance for the index'th
  * DSP, if one doesn't already exist. 
  * If one already exists, it returns the existing object.
  * Returns nil if index is out of bounds.
  */ 

+ alloc;
 /* TYPE: Creating; Allocates on default zone and default DSP */
+ allocFromZone:(NXZone *)zone;
 /* TYPE: Creating; Allocates on zone and default DSP */
+ allocFromZone:(NXZone *)zone onDSP:(unsigned short)dspNum;
 /* TYPE: Creating; Allocates on zone and specified dsp. Returns nil
    if index is out of bounds. */
- init;
 /* Initializes new object as created with any of the alloc methods above */

- copyFromZone:(NXZone *)zone;
- copy;
 /* These methods are not implemented. Copying of Orchestras is not 
    supported. */

+ flushTimedMessages; 
  /* TYPE: Modifying; Sends all buffered DSP commands.
  * Sends all buffered DSP commands for each instance.  Returns the receiver.
  *
  * See the instance method by the same name, as well as 
  * Conductor's +unlockPerformance.
  */

+(unsigned short)DSPCount;
  /* TYPE: Querying; Returns the number of DSPs configured on your machine.
 * Returns the number of DSPs configured on your machine.
 */

+ nthOrchestra:(unsigned short)index; 
  /* TYPE: Querying; Returns the index'th Orchestra.
 * Returns the Orchestra instance that corresponds to the index'th
 * DSP, or nil if index is out of bounds or if no Orchestra
 * for it has been created.
 */

+ free; 
  /* TYPE: Creating; Frees all the Orchestras instances.
 * Frees all the Orchestra instances by sending free to each
 * of them.  Returns the receiver.
 */

+ open; 
  /* TYPE: Modifying; Opens all existing Orchestras.
 * Opens all existing Orchestra instances by sending open to each of 
 * them. Returns nil if an Orchestra returns nil, otherwise returns the 
 * receiver. Note that if you send +open and then
 * create a new instance, the new instance is not opened.
 */

+ run; 
  /* TYPE: Modifying; Starts all existing Orchestra running.
 * Starts all existing Orchestra instances running by sending run to 
 * each of them.  
 * Returns nil if an Orchestra returns nil, otherwise returns the receiver.
 */

+ stop; 
  /* TYPE: Modifying; Stops all existing Orchestras.
 * Stops all existing Orchestra instances by sending stop to each of 
 * them. 
 * Returns nil if an Orchestra returns nil, otherwise returns the receiver.
 */

+ close; 
  /* TYPE: Modifying; Closes all existing Orchestras.
 * Closes all existing Orchestra instances by sending close to each of 
 * them. 
 * Returns nil if an Orchestra returns nil, otherwise returns the receiver.
 */

+ abort;
  /* TYPE: Modifying; Aborts all existing Orchestras.
 * Aborts all existing Orchestra instances by sending abort to each of
 * them. 
 * Returns nil if an Orchestra returns nil, otherwise returns the receiver.
 */

-(double ) samplingRate; 
  /* TYPE: Querying; Returns the receiver's sampling rate.
 * Returns the receiver's sampling rate (in samples per second).
 */

+ setSamplingRate:(double )newSRate; 
  /* TYPE: Modifying; Sets the sampling rate for all Orchestras.
 * Sets the sampling rate of all the Orchestra instances
 * by sending setSamplingRate:newSRate to all closed Orchestras.
 * This method also changes the default
 * sampling rate so that when a new instance is created, it gets newSRate.
 * Returns the receiver.  
 */

- setSamplingRate:(double )newSRate; 
  /* TYPE: Modifying; Sets the receiver's sampling rate to newSRate.
 * Sets the receiver's sampling rate to
 * newSRate, taken as samples per second (the default rate is 22050).
 * The receiver must be closed.
 * Returns the receiver.  
 */

#define MK_UNTIMED ((char)0)
#define MK_TIMED   ((char)1)
#define MK_SOFTTIMED ((char)2) /* Obsolete. */

+ setTimed:(char )areOrchsTimed; 
  /* TYPE: Modifying; For all Orchestras, determines how DSP commands are timed.
 * For all Orchestras, determines how DSP commands are processed with 
 * respect to time by sending setTimed:areOrchsTimed to each
 * instance.  
 * This method also changes the default value for whether a new orchestra
 * is timed or untimed. Returns the receiver. CF. setTimed:
 *
 */

- setTimed:(char )isOrchTimed; 
  /* TYPE: Modifying; For all Orchestras, determines how DSP commands are timed.
 * If isOrchTimed is YES, DSP commands
 * are processed (on the DSP) according to their timestamps.  If it's NO,
 * DSP commands are processed as soon as they're received by the DSP.
 * Note that untimed mode is intended primarily as a 
 * means of inserting "out of band" messages into a timed stream. It is 
 * not intended for high-bandwidth transfers such as are normally associated
 * with a Music Kit performance and it is not deterministic with respect
 * to precise timing. It is permissable to change time timing mode 
 * during a Music Kit performance.
 *
 * MK_TIMED is a synonym for YES, while MK_UNTIMED is a synonym for NO.
 */

-(char ) isTimed; 
  /* TYPE: Querying; Returns MK_TIMED (YES) or MK_UNTIMED (NO).
  */

-setSynchToConductor:(BOOL)yesOrNo;
  /* The DSP's clock and the Conductor's clock (in clocked mode) are intended 
    to keep the same time (except for fluctuations due to the internal 
    buffering of the DSP and sound drivers). Over a long-term 
    performance, however, the two clocks may begin to drift apart slightly,
    on the order of a few milliseconds per minutes. If you are 
    running with an extremely small "delta time" (cf. MKSetDeltaT()), you
    may want to synchronize the two clocks periodically. By sending 
    setSynchToConductor:YES, you specify that the Orchestra is to 
    synchronizes the DSP to the Conductor's time every 10 seconds to account 
    for slight differences between the rate of the two clocks. 

    Note: this method assumes an Application object exists and is running and
    that the Conductor is in clocked mode.
    */

-setFastResponse:(char)yesOrNo;
  /* TYPE: Modifying; Sets whether fastResponse is important. 
   If you pass YES as an argument, the receiver uses small sound-out buffers
   in an effort to minimize response latency. The default is NO. 
   Note that other values (for faster response) may be added in future 
   releases. 
   */

+setFastResponse:(char)yesOrNo;
  /* TYPE: Modifying; Sets whether fastResponse is important for all orchestras. 
   Broadcasts setFastResponse:yesOrNo to all existing Orchestras and
   returns self. Also sets default (as used by Orchestras to be created)
   to the argument's vaue.
   */

-(char)fastResponse;
  /* TYPE: Querying; YES if fastResponse is enabled.
 * YES if the receiver is using small sound-out buffers 
 * to minimize response latency. 
 *
 * Note that other values (for faster response) may be added in future 
 * releases. 
 */

- setLocalDeltaT:(double)val;
 /* Sets an offset, in seconds, to be added to time stamps sent to DSP.
    Has no effect if the receiver is not in timed mode. This is in addition
    to the deltaT set with MKSetDeltaT(). */
- (double)localDeltaT;

+ setLocalDeltaT:(double)val;
  /* Sends setLocalDeltaT: to all Orchestras and changes the default (which is,
     normally, 0). */

- setSoundOut:(BOOL)yesOrNo;
  /* TYPE: Sending; Sets whether the receiver sends to sound out (the DAC).
 * Sets whether the receiver, which must be closed,
 * sends its sound signal to 
 * sound out (the DAC), as yesOrNo is YES or NO.  
 * Returns the receiver, or nil if it isn't closed. 
 * All Orchestras send to sound out by default.
 *
 * It is not currently permissable to have an output soundfile and do 
 * sound-out at the same time. Thus, sending setSoundOut:YES also sends 
 * setOutputSoundfile:NULL. 
 *
 * Sending setSoundOut:YES also automatically sends useDSP:YES. 
 */

-(BOOL)soundOut;
  /* Returns whether or not sound-out is being used. */

-setSSISoundOut:(BOOL)yesOrNo;
  /* Controls whether sound is sent to the SSI port. The default is NO. 
   If the receiver is not closed, this message has no effect.
   
   Sending setSSISoundOut:YES also automatically sends useDSP:YES. 
   */

-(BOOL)SSISoundOut;
  /* Returns whether or not sound is being sent to the SSI port. */

-setOutputSoundfile:(char *)fileName;
  /* Sets a file name to which output samples are to be written as a 
   soundfile. A copy of the fileName is stored in the instance variable 
   outputSoundfile. 
   
   This message is currently ignored if the receiver is not closed. 
   
   If you re-run the Orchestra, the file is rewritten. To specify that
   you no longer want to write a file when the Orchestra is re-run, close the 
   Orchestra, then send setOutputSoundfile:NULL. 
   
   Note that in release 2.0, it is not permissable to have an output 
   soundfile and do sound-out at the same time. 
   Also note that sending setOutputSoundfile:NULL does not automatically 
   send setSoundOut:YES. You must do this yourself. 
   
   Finally, note that in the 
   current release, some small number of extra zero samples (less than 256) 
   may appear at the start of the output sound file. 
   
   */

-(char *)outputSoundfile;
  /* Returns the output soundfile or NULL if none. */

-setOutputCommandsFile:(char *)fileName;
  /* Sets a file name to which DSP commands are to be written as a DSP Commands
   format soundfile.  A copy of the fileName is stored in the instance variable
   outputCommandsFile.
   This message is currently ignored if the receiver is not closed.
   */

-(char *)outputCommandsFile;
  /* Returns the output DSP Commands soundfile name or NULL if none. */

-setSimulatorFile:(char *)filename;
  /* Sets a file name to which logfile output suitable for the DSP simulator
   is to be written. In the current release a complete log is only available
   when doing sound-out or writing a soundfile.
   This message is currently ignored if the receiver is not closed. 
   If you re-run the Orchestra, the file is rewritten. To specify that
   you no longer want a file when the Orchestra is re-run, close the Orchestra,
   then send setSimulatorFile:NULL.  */

-(char *)simulatorFile;
  /* Gets text file being used for DSP log file output, if any. */

+ allocUnitGenerator:classObj; 
  /* TYPE: Allocating; Allocates a UnitGenerator of class classObj.
 * Allocates memory for a UnitGenerator of class classObj.
 * The memory is allocated on the first Orchestra that can accomodate it.  
 * Returns the UnitGenerator, or nil if the object couldn't be allocated.
 */ 

+ allocSynthData:(MKOrchMemSegment )segment length:(unsigned )size; 
  /* TYPE: Allocating; Allocates a SynthData object (private data memory).
 * Allocates memory for a SynthData object.  The allocationn
 * is on the first Orchestra that will accommodate size words in 
 * segment segment.
 * Returns the SynthData, or nil if the object couldn't be allocated.
 */

+ allocPatchpoint:(MKOrchMemSegment )segment; 
  /* TYPE: Allocating; Allocates a patchpoint.
 */

+ allocSynthPatch:aSynthPatchClass;
  /* TYPE: Allocating; Allocates a SynthPatch with default PatchTemplate.
 * Same as allocSynthPatch:patchTemplate: but uses the default 
 * template. 
 */

+ allocSynthPatch:aSynthPatchClass patchTemplate:p;
  /* TYPE: Allocating; Allocates a SynthPatch with the specified PatchTemplate.
 * Get a SynthPatch with the specified template on the first Orchestra 
 * that has sufficient resources. 
 */

+ dealloc:aSynthResource;
  /* TYPE: Allocating; Deallocates a SynthPatch, UnitGenerator or SynthData.
 * Deallocates a  SynthPatch, UnitGenerator or SynthData by sending it 
 * the dealloc message. 
 * This method is provided for symmetry with the alloc family
 * of methods. */

- flushTimedMessages;
  /* TYPE: Modifying; Send buffered DSP commands to the DSP. 
   Sends buffered DSP commands to the DSP. This is ordinarily done by the
   Conductor. However, if you generate DSP commands in response to an
   asynchronous event such as a mouse click or keyboard stroke, you must
   send flushTimedMessages to the Orchestra after generating your DSP
   commands. Note that you must send flushTimedMessages even if the Orchestra
   is set to MK_UNTIMED mode ("flushTimedMessages" is somewhat of a 
   misnomer; a better name would have been "sendBufferedDSPCommands"). 

   See Conductor's +unlockPerformance.
   */

- installSharedObject:aSynthObj for:aKeyObj;
  /* TYPE: Modifying; Install a shared object.
   Places aSynthObj on the shared object
   table and sets its reference count to 1.
   Does nothing and returns nil if the 
   aSynthObj is already present in the table.
   Also returns nil if the orchestra is not open. 
   Otherwise, returns the receiver.

   aKeyObj is any object associated with the abstract notion of the data.
   This method differs from installSharedObjectWithSegmentAndLength:for: in 
   that the length and segment are wild-cards.
   */

-installSharedSynthDataWithSegment:aSynthDataObj for:aKeyObj;
  /* TYPE: Modifying; Install a shared SynthData object.
   This method is usually used to associate a WaveTable (aKeyObj) with a 
   SynthData (aSynthDataObj).
   Places aSynthDataObj on the shared object
   table in the segment of aSynthDataObj and sets its reference count to 1.
   Does nothing and returns nil if the 
   aSynthObj is already present in the table.
   Also returns nil if the orchestra is not open. 
   Otherwise, returns the receiver.

   aKeyObj is any object associated with the abstract notion of the data.
   This method differs from installSharedObjectWithSegmentAndLength:for: in 
   that the length is a wild-card.
   */

-installSharedSynthDataWithSegmentAndLength:aSynthDataObj
 for:aKeyObj;
  /* TYPE: Modifying; Install a shared SynthData object.
   This method is usually used to associate a WaveTable (aKeyObj) with a 
   SynthData (aSynthDataObj).
   Places aSynthDataObj on the shared object
   table in the segment of aSynthDataObj with the specified length 
   and sets its reference count to 1.
   Does nothing and returns nil if the 
   aSynthDataObj is already present in the table.
   Also returns nil if the orchestra is not open. 
   Otherwise, returns the receiver.

   aKeyObj is any object associated with the abstract notion of the data.
   */

- sharedObjectFor:aKeyObj;
  /* TYPE: Modifying; Returns the shared object specified.
   Returns the SynthData, UnitGenerator, or SynthPatch object
   that's the manifestation of aKeyObj
   on the receiver.  If the object is found,  aKeyObj's reference 
   count is incremented. If it isn't found, 
   or if the orchestra isn't open, returns nil. 

   aKeyObj is any object associated with the abstract notion of the data.
 */ 

- sharedObjectFor:aKeyObj segment:(MKOrchMemSegment)whichSegment;
  /* TYPE: Modifying; Returns the shared object specified.
   Returns the SynthData, UnitGenerator, or SynthPatch object
   that's the manifestation of aKeyObj on the receiver 
   in the specified segment.  If the object is found,
   aKeyObj's reference count is incremented.  If it isn't found, 
   or if the orchestra isn't open, returns nil. 

   aKeyObj is any object associated with the abstract notion of the data.
   For example, aKeyObj may be a WaveTable object.
 */ 

- sharedObjectFor:aKeyObj segment:(MKOrchMemSegment)whichSegment 
 length:(int)length;
  /* TYPE: Modifying; Returns the shared object specified.
   Returns the SynthData, UnitGenerator, or SynthPatch object
   that's the manifestation of aKeyObj on the receiver 
   in the specified segment and with the specified length.  
   If the object is found,  aKeyObj's reference count is incremented.  
   If it isn't found, or if the orchestra isn't open, returns nil. 

   aKeyObj is any object associated with the abstract notion of the data.
   For example, aKeyObj may be a WaveTable object.
 */ 

- sineROM; 
  /* TYPE: Querying; Returns the SineROM SynthData.
   Returns a SynthData object representing the SineROM.  You should never
   deallocate this object. */

- muLawROM; 
  /* TYPE: Querying; Returns the MuLawROM SynthData.
   Returns a SynthData object representing the MuLawROM.  You should never
   deallocate this object. */

- segmentZero:(MKOrchMemSegment )segment; 
  /* TYPE: Querying; Returns the zero Patchpoint on the specified segment.
   Returns special pre-allocated Patchpoint which always holds 0 and which,
   by convention, nobody ever writes to.  This patch-point may not be
   deallocated.  Segment should be MK_xPatch or MK_yPatch. */

- segmentSink:(MKOrchMemSegment )segment; 
  /* TYPE: Querying; Returns the sink Patchpoint on the specified segment.
   Returns special pre-allocated Patchpoint from which nobody reads, by
   convention.  It is commonly used as a place to send the output of idle 
   UnitGenerators.  This patch-point may not be deallocated.
   Segment should be MK_xPatch or MK_yPatch. 
 */

- open; 
  /* TYPE: Modifying; Opens device if not already open. 
   Opens the receiver.
   Resets orchestra loop (if not already reset), freeing all Unit Generators 
   and SynthPatches.  
   Sets deviceStatus to MK_devOpen.  Returns nil if an error
   occurs, otherwise returns the receiver. 

  */

- run; 
  /* TYPE: Modifying; Starts DSP clock if not already running.
   Starts the receiver's clock and sets its deviceStatus to
   MK_devRunning.
   If the receiver isn't open, this first sends [self open].
  Returns nil if an error occurs, otherwise returns the receiver.
 */

- stop; 
  /* TYPE: Modifying; Stops DSP clock if not already stopped.
  Stops the receiver's clock and sets its deviceStatus to MK_devStopped.
   If the receiver isn't open, this first sends [self open].
  Returns nil if an error occurs, otherwise returns the receiver.

   */

- close; 
  /* TYPE: Modifying; Closes DSP after waiting.
   This waits for all enqueued DSP command to be exectued
   then frees the
   UnitGenerators in the UnitGenerator stack, deallocates all 
   SynthPatches that are controlled by SynthInstruments and clears all 
   SynthInstrument allocation lists, and releases the DSP.  It's an error
   to free an Orchestra that has non-idle SynthPatches or allocated 
   UnitGenerators that aren't members of a SynthPatch.  An attempt to
   do so generates an error. 
   Returns nil if an error occurs, otherwise returns the receiver.

   */

- abort;
  /* TYPE: Modifying; Closes DSP without waiting.
   Same as close, but does not wait for enqueued DSP commands to be 
   executed. 
   Returns nil if an error occurs, otherwise returns the receiver.
 */   

- free; 
  /* TYPE: Modifying; Frees the orchestra instance.
   Frees the 
   UnitGenerators in the receiver's UnitGenerator stack, clears all
   SynthPatch allocation lists, and releases the DSP.  It's an error
   to free an Orchestra that has non-idle SynthPatches or allocated 
   UnitGenerators that aren't members of a SynthPatch.  An attempt to
   do so generates an error. 
   Returns nil if an error occurs, otherwise returns the receiver.
   */

- useDSP:(BOOL )useIt; 
  /* TYPE: Modifying; Sets whether DSP is used.
   This method is not supported in release 2.0. 
   Controls whether or not the DSP commands actually go to the DSP. 
   Has no effect if the Orchestra isn't closed. The default is YES.
 */

-(BOOL ) isDSPUsed; 
  /* TYPE: Querying; Returns whether DSP is used.
   If the receiver isn't closed, returns YES if the DSP is used.
   If the receiver is closed, returns YES if the receiver is set to use
   the DSP when it's opened. 
   In release 2.0, always returns YES.
 */

- trace:(int )typeOfInfo msg:(char * )fmt,...; 
  /* This method is used for printing debugging information. 
   Arguments are like those to printf().  
   If the typeOfInfo trace is set, prints to stderr. */

-(char * )segmentName:(int )whichSegment; 
  /* TYPE: Querying; Returns the specified segment name.
   Returns the name of the specified MKOrchMemSegment. The string isn't
   copied. */

-(MKOrchMemStruct *)peekMemoryResources:(MKOrchMemStruct *)peek;
  /* TYPE: Querying; Returns the available resources.
   Returns the available resources in peek. 
   The returned value
   is the maximum available of each kind, assuming the memories
   aren't overlaid.  However, xData, yData and pSubr compete for the
   same memory. The caller should interpret what is returned with appropriate
   caution.  peek must point to a valid MKOrchMemStruct.
 */
-(unsigned short) index; 
  /*  TYPE: Querying; Returns the index of the DSP. 
    Returns the index of the DSP associated with this instance. 
 */

-(double ) computeTime; 
  /* TYPE: Querying; Returns estimated time to compute a sample.
   Returns the compute time currently used by the orchestra system in 
   seconds per sample. 
 */

- allocSynthPatch:aSynthPatchClass; 
  /* TYPE: Allocating; Allocates a SynthPatch with the default template.
   Same as allocSynthPatch:patchTemplate: but uses the
   default template obtained by sending -defaultPatchTemplate to 
   aSynthPatchClass. 
 */

- allocSynthPatch:aSynthPatchClass patchTemplate:p; 
  /* TYPE: Allocating; Allocates a SynthPatch with the specified template.
   Allocates and returns a SynthPatch for PatchTemplate p.
   The receiver first tries to find an idle SynthPatch; failing that,
   it creates and returns a new one.
   If a new one can't be built, this method returns nil.
   The List of objects in the 
   SynthPatch is in the same order as specified in the template. 
 */

- allocUnitGenerator:class; 
  /* TYPE: Allocating; Allocate unit generator of the specified class. 
   Allocates and returns a UnitGenerator of the specified class, creating
   a new one, if necessary. 
 */

- allocUnitGenerator:class before:aUnitGeneratorInstance; 
  /* TYPE: Allocating; Allocate unit generator before specified instance.
   Allocates and returns a UnitGenerator of the specified class that runs 
   before the aUnitGeneratorInstance. 
 */

- allocUnitGenerator:class after:aUnitGeneratorInstance; 
  /* TYPE: Allocating; Allocate unit generator after specified instance.
   Allocates and returns a UnitGenerator of the specified class that runs 
   after the aUnitGeneratorInstance. 
 */

- allocUnitGenerator:class between:aUnitGeneratorInstance :anotherUnitGeneratorInstance; 
  /* TYPE: Allocating; Allocate unit generator between specified instance.
   Allocates and returns UnitGenerator of the specified class that runs 
   after aUnitGeneratorInstance and before
   anotherUnitGenerator.
 */

- allocSynthData:(MKOrchMemSegment )segment length:(unsigned )size; 
  /* TYPE: Allocating; Allocates a SynthData. 
   Allocates and returns a new SynthData object 
   with the specified length, or nil if 
   there's no more memory (or if size is 0) or if an illegal segment
   is requested. Segment should be MK_xData or MK_yData. 
 */

- allocPatchpoint:(MKOrchMemSegment )segment; 
  /* TYPE: Allocating; Returns a new patchpoint.
   Allocates and returns a SynthData to be used as a patchpoint.
   Segment must be MK_xPatch or MK_yPatch.
   Returns nil if an illegal segment is requested. 
   */

- dealloc:aSynthResource;
  /* TYPE: Allocating; Deallocates resource.
   Deallocates the argument by sending it the dealloc
   message. 
   aSynthResource may be a UnitGenerator, a SynthData or a SynthPatch.
   (This method is provided for symmetry with the alloc family
   of methods.) 
 */

-setOnChipMemoryConfigDebug:(BOOL)debugIt patchPoints:(short)count;
  /* Sets configuration of onchip memory.
   If debugIt is YES, a partition is reserved for the DSP debugger.
   Also sets the number of on-chip patch points. This also implicitly sets
   the maximum possible number of L unit generator arguments; the more
   patch points, the fewer L unit generator arguments. 
   The default is no debug and 11 on-chip patchpoints. 
   You may not reconfigure when open. Setting count to 0 uses the defaults.
   Attempts to set patchpoint count so high as to allow no L arguments are
   ignored. Returns self if success, else nil. */

-setOffChipMemoryConfigXArg:(float)xPercentage yArg:(float)yPercentage;
 /* Sets configuration of offchip memory.
   xPercentage is the percentage of off-chip memory devoted to x arguments.
   yPercentage is the percentage of off-chip memory devoted to y arguments.
   Passing a value of 0 for either uses the default.
   These percentages are expressed as numbers between 0 and 1. If xPercentage +
   yPercentage is greater than 1.0, the settings are ignored and the method 
   returns nil. You may not reconfigure when open. Returns self if 
   success, else nil. */

-segmentInputSoundfile:(MKOrchMemSegment)segment;
   /* Returns special pre-allocated Patchpoint which always holds 0 and which,
     by convention, nobody ever writes to. This patch-point may not be
     freed. You should not deallocate
     the returned value. Segment should be MK_xPatch or MK_yPatch. */

-setInputSoundfile:(char *)file;
  /* Sets a soundfile name from which samples are to be read and processed on 
   the DSP. A copy of the fileName is stored in the instance variable 
   inputSoundfile. 

   This message is currently ignored if the receiver is not closed. 

   If you re-run the Orchestra, the file is reread. To specify that
   you no longer want to read the file when the Orchestra is re-run, close the 
   Orchestra, then send setInputSoundfile:NULL. 

   NOTE: At the time of this writing, it is unclear whether support for input 
         soundfiles will make it into the 2.0 release. Please check the
	 release notes of the DSP library and the Music Kit. 
 */
-(char *)inputSoundfile;
  /* Returns name of inputSoundfile or NULL if none.  */

-pauseInputSoundfile;
-resumeInputSoundfile;
  /* These methods pause and resume, respectively, sending of the 
   inputSoundfile
   to the DSP. They may be sent at any time after the Orchestra is open. 
   These methods can be used to send a soundfile to the DSP based on user
   input. They can also be used to pause and resume the soundfile under
   scheduler control (see the Conductor class). To start a soundfile in the
   paused state, first send pauseInputSoundfile and then send run. 

   CF setInputSoundfile: */

 /* -read: and -write: 
  * Note that archiving the Orchestra is not supported.
  */

@end



