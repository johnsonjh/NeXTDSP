#import <objc/Object.h>
#import "devstatus.h"
#import "params.h"

@interface Midi:Object
/* The Midi object provides Midi input/output access. It emulates some of the
 * behavior of a Performer: It contains a List of NoteSenders, one per MIDI
 * channel (as well as an extra one for MIDI system and channel mode messages).
 * You can receive Notes derived from MIDI input by connecting an Instrument's
 * NoteReceivers to the NoteSenders of a Midi instance.
 *
 * Similarly, Midi emulates some of the behavior of an Instrument: It contains
 * a List of NoteReceivers, one per MIDI channel (as well as the extra one).
 * You can send Notes to MIDI output by connecting a Performer's NoteSenders
 * to the NoteReceivers of a Midi instance.
 *
 * However, the Midi object is unlike a Performer in that it represents a 
 * real-time  device. In this manner, Midi is somewhat like Orchestra, 
 * which represents the DSP. The protocol for controlling Midi is the same 
 * as that for the Orchestra. This protocol is described in the file 
 * <musickit/devstatus.h>.
 * 
 * The conversion between Music Kit and MIDI semantics is described in the
 * NeXT technical documentation. 
 */
{
    id noteSenders;        /* The object's collection of NoteSenders. */
    id noteReceivers;      /* The object's collection of NoteReceivers */
    MKDeviceStatus deviceStatus; /* See devstatus.h */
    char * midiDev;              /* Midi device port name. */
    BOOL useInputTimeStamps;     /* YES if Conductor's time updated from 
                                    driver's time stamps.*/
    BOOL outputIsTimed;   /* YES if the driver's clock is used for output */
    double localDeltaT;   /* Offset added to MIDI-out time stamps.(see below)*/
    void *_reservedMidi1; 
    unsigned _reservedMidi2;
    void *_reservedMidi3;
    void *_reservedMidi4;
    double _reservedMidi5;
    char _reservedMidi6;
}

-conductor;
 /* Always returns the clockConductor, if the Conductor class is loaded. 
  * Provided for Performer compatability */

+allocFromZone:(NXZone *)zone onDevice:(char *)devName;
 /* Creates, but doesn't initialize new Midi object, if one doesn't already
    exist for the specified devName ("midi1" or "midi0"). 
    Otherwise, returns existing one.
    You must send init to the new object (sending init to an already 
    initialized object will do no harm.) */
+allocFromZone:(NXZone *)zone;
 /* Like allocFromZone:onDevice:, but uses default device ("midi1", the
    top serial port). */
+alloc;
 /* Like allocFromZone:, but uses default zone. */

+newOnDevice:(char *)devName;
 /* Allocates and sends init to new object, if one doesn't exist already, for
    specified device. */
+new;
 /* Allocates and sends init to new object, if one doesn't exist already, for
    default device. */

-init;
 /* Initializes new object. */

-free;
  /* free object, closing device if it is not already closed. */

-(MKDeviceStatus)deviceStatus;
  /* Returns MKDeviceStatus of receiver. */

-open;
  /* Opens device if not already open.
   * If already open, flushes input and output queues. 
   * Sets deviceStatus to MK_devOpen. 
   * Returns nil if failure.
   */

-openInputOnly;
-openOutputOnly;
 /* These are like -open but enable only input or output, respectively. */ 

-run;
 /* If not open, does a [self open].
  * If not already running, starts clock. 
  * Sets deviceStatus to MK_devRunning. Returns nil if failure. */

-stop;
 /* If not open, does a [self open].
  * Otherwise, stops MidiIn clock and sets deviceStatus to MK_devPaused.
  */

-close;
 /* Closes the device, after waiting for the output queue to empty.
  * Releases the device port. This method blocks until the output queue
  * is empty.
  */

-abort;
 /* Closes the device, without waiting for the output queue to empty. */

-setOutputTimed:(BOOL)yesOrNo;
-(BOOL)outputIsTimed;
 /* If setOutputTimed:YES is sent, events are sent to the driver with time
  * stamps equal to the Conductor's time plus "deltaT". (See MKSetDeltaT()).
  * If setOutputTimed:NO is sent, events are sent to the driver with a time
  * stamp of 0, indicating they are to be played as soon as they are received.
  * Default is YES.
  */

-channelNoteSender:(unsigned)n;
 /* Returns the NoteSender corresponding to the specified channel or nil
  * if none. If n is 0, returns the NoteSender used for Notes fasioned
  * from MIDI channel mode and system messages. */

-channelNoteReceiver:(unsigned)n;
 /* Returns the NoteReceiver corresponding to the specified channel or nil
  * if none. The NoteReceiver corresponding to 0 is special. It uses
  * the MK_midiChan parameter of the note, if any, to determine which
  * midi channel to send the note on. If no MK_midiChan parameter is present,
  * the default is channel 1. This NoteReceiver is also commonly used
  * for MIDI channel mode and system messages. */

-noteReceiver;
 /* Returns the defualt NoteReceiver. */
-noteReceivers;
 /* Returns a List containing the receiver's NoteReceivers. The List may be
  * freed by the caller, although its contents should not. */
-noteSender;
 /* Returns the defualt NoteSender. */
-noteSenders;
 /* Returns a List containing the receiver's NoteSenders. The List may be
  * freed by the caller, although its contents should not. */

-setUseInputTimeStamps:(BOOL)yesOrNo;
-(BOOL)useInputTimeStamps;
 /* By default, Conductor's time is adjusted to that of the Midi input 
  * clock (useInputTimeStamps == YES). 
  * This is desirable when recording the Notes (e.g. with a 
  * PartRecorder). However, for real-time MIDI processing, it is preferable
  * to use the Conductor's time (useInputTimeStamps == NO). 
  * setUseInputTimeStamps: and useInputTimeStamps set and get, respectively, 
  * this flag. 
  *
  * Note that even with setUseInputTimeStamps:YES, the Conductor's clock is 
  * not slave to the MIDI input clock. Rather, fine adjustment of 
  * the Conductor's clock is made to match that of MIDI input. (Future releases
  * of the Music Kit may provide the ability to use the MIDI clock as the
  * primary source of time.)
  *
  * It is the application's responsibility to insure that Midi is stopped
  * when the performance is paused and that Midi is run when the performance
  * is resumed.
  */

-ignoreSys:(MKMidiParVal)param;
-acceptSys:(MKMidiParVal)param;
 /* By default, Midi input ignores MIDI messages that set the MK_sysRealTime
  * parameter to the following MKMidiParVals:
  * MK_sysClock, MK_sysStart, MK_sysContinue, MK_sysStop, 
  * MK_sysActiveSensing. These are currently the only MIDI messges that
  * can be ignored. You enable or disable this feature using the 
  * ignoreSys: and acceptSys: methods. For example, to receive 
  * MK_sysActiveSensing, send [aMidiObj acceptSys:MK_activeSensing]. */

-(double)localDeltaT;
-setLocalDeltaT:(double)value;
 /* Sets and retrieves the value of localDeltaT. LocalDeltaT is added to time 
  * stamps sent to MIDI-out. 
  * This is in addition to the deltaT set with MKSetDeltaT(). 
  * Has no effect if the receiver is not in outputIsTimed mode. Default is 
  * 0. 
  */

-setMergeInput:(BOOL)yesOrNo;
 /* If set to YES, the Notes fashioned from the incoming MIDI stream are all
  * sent to NoteSender 0 (the one that normally gets only system messages).
  * In addition, a MK_midiChan is added so that the stream can be split up
  * again later.
  */

 /* -read: and -write: 
  * Note that archiving is not yet supported in the Midi object. To archive
  * the connections of a Midi object, archive the individual NoteSenders and
  * NoteReceivers. 
  */

@end




