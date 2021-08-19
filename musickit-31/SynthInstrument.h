/*
  SynthInstrument.h
  Copyright 1988, NeXT, Inc.
  
  DEFINED IN: The Music Kit
  */
#define MK_AUTOALLOC 0
#define MK_MANUALALLOC 1

#import "Instrument.h"
#import "Note.h"
#import <objc/HashTable.h>

@interface SynthInstrument : Instrument
/* 
 * 
 * A SynthInstrument realizes Notes by synthesizing them on the DSP.  It
 * does this by forwarding each Note it receives to a SynthPatch object,
 * which translates the parameter information in the Note into DSP
 * instructions.  A SynthInstrument can manage any number of SynthPatch
 * objects (limited by the speed and size of the DSP).  However, all of
 * its SynthPatches are instances of the same SynthPatch subclass.  You
 * assign a particular SynthPatch subclass to a SynthInstrument through
 * the latter's setSynthPatchClass: method.  A SynthInstrument can't
 * change its SynthPatch class during a performance.
 * 
 * Each SynthPatch managed by the SynthInstrument corresponds to a
 * particular noteTag.  As the SynthInstrument receives Notes, it
 * compares the Note's noteTag to the noteTags of the SynthPatches that
 * it's managing.  If a SynthPatch already exists for the noteTag, the
 * Note is forwarded to that object; otherwise, the SynthInstrument
 * either asks the Orchestra to allocate another SynthPatch, or it
 * preempts an allocated SynthPatch to accommodate the Note.  Which
 * action it takes depends on the SynthInstrument's allocation mode and
 * the available DSP resources.
 * 
 * A SynthInstrument can either be in automatic allocation mode
 * (MK_AUTOALLOC) or manual mode (MK_MANUALALLOC).  In automatic mode,
 * SynthPatches are allocated directly from the Orchestra as Notes are
 * received by the SynthInstrument and released when it's no longer
 * needed.  Automatic allocation is the default.
 * 
 * In manual mode, the SynthInstrument pre-allocates a fixed number of
 * SynthPatch objects through the setSynthPatchCount: method.  If it
 * receives more simultaneously sounding Notes than it has SynthPatches,
 * the SynthInstrument preempt its oldest running SynthPatch (by sending
 * it the preemptFor: message).
 * 
 */
{
    id synthPatchClass;        /* class used to create patches. */
    unsigned short allocMode;  /* One of MK_MANUALALLOC or MK_AUTOALLOC */
    id taggedPatches;   /* HashTable mapping noteTags to SynthPatches */
    id controllerTable; /* HashTable mapping MIDI controllers to values */
    id updates;         /* Note for storing common (no noteTag) updates. */
    BOOL retainUpdates; /* NO if updates and controllerTable are cleared after
                           each performance. */
    id orchestra;
    id _reservedSynthInstrument1; 
}
 
 /* METHOD TYPES
  * Freeing a SynthInstrument
  * Modifying the object
  * Querying the object
  * Allocating SynthPatch objects
  * Performing the object
  */
- init;
 /* TYPE: Modifying; Initializes the receiver.
 * Initializes the receiver.  You never invoke this method directly.
 * An overriding subclass method 
 * should send [super init] before setting its own defaults. 
 */

-(int)setSynthPatchCount:(int)voices patchTemplate:aTemplate;
 /* TYPE: Allocating; Allocates voices SynthPatches with the given template.
 * Immediately allocates voices SynthPatch objects using the 
 * patch template aTemplate (the
 * Orchestra must be open) and puts the receiver in manual mode.  
 * If aTemplate is nil, the value returned by the message
 * 
 * [synthPatchClass defaultPatchTemplate] 
 *
 * is used.  Returns the number of objects that were allocated (it may be less
 * than the number requested).
 * If the receiver is in performance and it isn't already in manual
 * mode, this message is ignored and 0 is returned.
 *
 * If you decrease the number of manually allocated
 * SynthPatches during a performance, the extra SynthPatches aren't 
 * deallocated until they become inactive.  In other words, reallocating
 * downward won't interrupt notes that are already sounding.
 */

-(int)setSynthPatchCount:(int)voices;
 /* TYPE: Allocating; Allocates voices SynthPatch objects.
 * Immediately allocates voices SynthPatch objects.
 * Implemented as
 *
 * [self setSynthPatchCount:voices template:nil];
 *
 * Returns the number of objects that were allocated.
 */
-(int)synthPatchCountForPatchTemplate:aTemplate;
-(int)synthPatchCount;

- realizeNote:aNote fromNoteReceiver:aNoteReceiver;
 /* TYPE: Performing; Realizes aNote.
 * Synthesizes aNote.
 */
   
- synthPatchClass;
 /* TYPE: Querying; Returns the receiver's SynthPatch class.
 * Returns the receiver's SynthPatch class.
 */

- setSynthPatchClass:aSynthPatchClass; 
 /* TYPE: Modifying; Sets the receiver's SynthPatch class.
 * Sets the receiver's SynthPatch class to aSynthPatchClass.
 * Returns nil if the argument isn't a subclass of SynthPatch or 
 * the receiver is in a performance (the class isn't set in this case).  
 * Otherwise returns the receiver.
 */ 
   
- setSynthPatchClass:aSynthPatchClass orchestra:anOrch; 
 /* TYPE: Modifying; This is a rarely-used method.
   It is like setSynthPatchClass: but also specifies that 
   SynthPatch instances are to be allocated using the object anOrch. This is 
   only used when you want a particular orchestra instance to be used rather
   than allocating from the Orchestra class. If anOrch is nil, the orchestra 
   used is the value returned by [aSynthPatchClass orchestraClass]. */

- orchestra;
/* Returns the value set with setSynthPatchClass:orchestra:, if any.
   Otherwise returns [Orchestra class].
  */

- free; 
 /* TYPE: Creating; Frees the receiver.
 * If the receiver isn't in performance, this frees the receiver 
 * (returns nil).  Otherwise does nothing and returns
 * the receiver.
 */

-preemptSynthPatchFor:aNote patches:firstPatch;
 /* TYPE: Allocating; You never invoke this method.
You never invoke this method.  It's invoked automatically when the receiver is
in manual mode and all SynthPatches are in use, or when it's in auto mode and
the DSP resources to build another SynthPatch aren't available.  The return
value is taken as the SynthPatch to preempt in order to accommodate the latest
request.  firstPatch is the first in a sequence of ordered
active SynthPatches, as returned by the activeSynthPatches: method.  The
default implementation simply returns firstPatch, the SynthPatch with the
oldest phrase.  A subclass can reimplement this method to provide a different
scheme for determining which SynthPatch to preempt.  
  */

-activeSynthPatches:aTemplate;
 /* TYPE: Querying; Returns the SynthPatch with the oldest sounding phrase.
Returns the first in the sequence of aTemplate SynthPatches that are
currently sounding.  The sequence is ordered by the begin times of the
SynthPatches' current phrases, from the earliest to the latest. In addition,
all finishing SynthPatches are before all running SynthPatches. You step down
the sequence by sending next to the objects returned by this method.  If
aTemplate is nil, returns the default PatchTemplate.  If there
aren't any active SynthPatches with the specified template, returns nil.
  */

-mute:aMute;
 /* TYPE: Modifying; You never invoke this method.
You never invoke this method; it's invoked automatically when the receiver
receives a mute Note.  Mutes aren't normally forwarded to SynthPatches since
they usually don't produce sound.  The default implementation does nothing.
A subclass can implement this method to examine aMute and act
accordingly.

  */

-autoAlloc;
 /* TYPE: Modifying; Puts the receiver in auto allocation mode.
Sets the receiver's allocation mode to MK_AUTOALLOC and releases any manually
allocated SynthPatch objects.  If the receiver is in performance and isn't
already in MK_AUTOALLOC mode, this does nothing and returns nil.
Otherwise returns the receiver.  
  */

-(unsigned short)allocMode;
 /* TYPE: Querying; Returns the receiver's allocation mode.
Returns the recevier's allocation mode, one of MK_AUTOALLOC or MK_MANUALALLOC.
  */

-abort;
  /* Sends the noteEnd message to all running (or finishing) synthPatches 
     managed by this SynthInstrument. This is used only for aborting in 
     emergencies. */

-copyFromZone:(NXZone *)zone;
  /* Returns a copy of the receiver. The copy has the same connections but
     has no synth patches allocated. */

-clearUpdates;
/* Causes the SynthInstrument to clear any noteUpdate state it has accumulated
   as a result of receiving noteUpdates without noteTags.
   The effect is not felt by the SynthPatches until the next phrase. Also
   clears controller info.
 */

-setRetainUpdates:(BOOL)yesOrNo;
/* Controls whether the noteUpdate and controller state is retained from
   performance to performance. Default is NO.
  */
-(BOOL)doesRetainUpdates;
/* Returns whether the noteUpdate and controller state is retained from
   performance to performance. */

-getUpdates:(Note **)aNoteUpdate controllerValues:(HashTable **)controllers;
/* Returns by reference the Note used to store the accumulated 
   noteUpdate state. Also returns by reference the HashTable used to 
   store the state of the controllers. Any alterations to the returned
   objects will effect future phrases. The returned objects should be 
   used only immediately after they are returned. If clearUpdates is
   sent or the performance ends, the objects may be emptied or freed by the 
   SynthInstrument.  */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes out the object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: method. Also archives allocMode, retainUpdates 
     and, if retainUpdates is YES, the controllerTable and updates. */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads the object. 
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     Note that -init is not sent to newly unarchived objects.
     See write:. */
-awake;
  /* TYPE: Archiving; Makes unarchived object ready for use. 
   */

@end




