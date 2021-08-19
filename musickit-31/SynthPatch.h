/*
    SynthPatch.h     
    Copyright, 1989, NeXT, Inc. 
    DEFINED IN:The Music Kit
  */
#import <objc/Object.h>
#import <objc/List.h>    /* Needed, by subclass, to access synth elements. */
#import "Conductor.h"
#import "orch.h"

typedef enum _MKPhraseStatus {
    MK_phraseOn,
    MK_phraseOnPreempt,
    MK_phraseRearticulate,
    MK_phraseUpdate,
    MK_phraseOff,
    MK_phraseOffUpdate,
    MK_phraseEnd,
        MK_noPhraseActivity}
MKPhraseStatus;

@interface SynthPatch : Object
/* A SynthPatch contains a List of unit generators which behave as
   a functional unit. 
   SynthPatches are not created by the application. Rather, they
   are created by the Orchestra. The Orchestra is also
   responsible for filling the SynthPatch instance with UnitGenerator and
   SynthData instances. It does this on the basis of a template provided by the
   SynthPatch class method +patchTemplate. The subclass designer implements
   this method to provide a PatchTemplate which specifies what the mix of 
   UnitGenerators and SynthData objects should
   be, in what order it should be allocated, and how to connect them up.
   (See PatchTemplate.)
   The SynthPatch instance, thus, is List 
   containing the UnitGenerators and SynthData objects in the order they were
   specified in the template and connected as specified in the temlate. 

   SynthPatches can be in one of three states:
   MK_running
   MK_finishing
   MK_idle

   The subclass may supply methods to be invoked at the initialiation 
   (creation), noteOn, noteOff, noteUpdate and end-of-note (noteEnd) times, as
   described below.

   SynthPatches are ordinarilly used in conjunction with a Conducted 
   performance
   by using a SynthInstrument. The SynthInstrument manages the allocation
   of SynthPatches in response to incoming Notes. Alternatively, SynthPatches
   may be used in a stand-alone fashion. In this case, you must allocate the
   SynthPatch by sending the Orchestra the -allocSynthPatch: or
   allocSynthPatch:patchTemplate: method.

   */
{
    id synthElements;    /* List of UnitGenerator and SynthData objects. */
    id synthInstrument;   /* The SynthInstrument object that owns the object,
                        if any. */
    int noteTag;           /* The object's current noteTag. */
    MKSynthStatus status;  /* The object's status. */
    id patchTemplate;      /* The object's PatchTemplate. */     
    BOOL isAllocated;      /* YES if the object is allocated. */  
    id orchestra;          /* Orchestra on which the object is allocated. */
    unsigned short _reservedSynthPatch1;
    int _reservedSynthPatch2;
    id _reservedSynthPatch3;
    MKMsgStruct *_reservedSynthPatch4;
    MKMsgStruct *_reservedSynthPatch5;
    id _reservedSynthPatch6;
    id _reservedSynthPatch7;
    MKMsgStruct *_reservedSynthPatch8;
    BOOL _reservedSynthPatch9;
    short _reservedSynthPatch10;
    void *_reservedSynthPatch11;
}
 
+new;
+allocFromZone:(NXZone *)zone;
+alloc;
-copy;
-copyFromZone:(NXZone *)zone;
 /* These methods are overridden to return [self doesNotRecognize]. 
    You never create, free or copy SynthPatches directly. These operations
    are always done via an Orchestra object. */

+ patchTemplateFor:currentNote; 
 /* TYPE: Querying; Returns PatchTemplate for specified note.
   patchTemplateFor: determines
   an appropriate patchTemplate and returns it. 
   In some cases, it is necessary to look at the current note to 
   determine which patch to use. 
   patchTemplateFor: is sent by the SynthInstrument 
   when a new SynthPatch is to be created. It may also be sent by
   an application to obtain the template to be used as an argument to 
   SynthInstrument's -setSynthPatchCount:patchTemplate:.
   Implementation of this method is a subclass responsibility. 
   The subclass should implement this method such that when
   currentNote is nil, a default template is returned. */

+ orchestraClass;
 /* TYPE: Querying; Returns Orchestra class.
   This method always returns the Orchestra factory. It is provided for
   applications that extend the Music Kit to use other hardware. Each 
   SynthPatch subclass is associated with a particular kind of hardware.
   The default hardware is that represented by Orchestra, the DSP56001.
   */

+ defaultPatchTemplate; 
  /* TYPE: Querying; Returns default PatchTemplate.
     You never implement this method. It is the same as 
     return [self patchTemplateFor:nil]. */

- synthInstrument;
  /* TYPE: Querying; Returns synthInstrument owning the receiver, if any. */

- init; 
 /* TYPE: Creating; Sent when an instance is created.
   Init is sent by the orchestra 
   only when a new SynthPatch has just been created and before its
   connections have been made, as defined by the PatchTemplate.
   Subclass may override the init method to provide additional 
   initialization. The subclass method may return nil to 
   abort the creation. In this case, the new SynthPatch is freed.
   The PatchTemplate is available in the instance variable patchTemplate.
   Default implementation just returns self.
   */

- initialize;
 /* Obsolete */

- dealloc;
 /* TYPE: Allocating; Deallocate the receiver.
   This is used to explicitly deallocate a SynthPatch you previously
   allocated manually from the Orchestra with allocSynthPatch: or 
   allocSynthPatch:patchTemplate:.
   It sends noteEnd to the receiver, then causes the receiver to become 
   deallocated and returns nil. This message is ignored (and self is returned)
   if the receiver is owned by a SynthInstrument. 
   */

- synthElementAt:(unsigned)anIndex;
  /* TYPE: Querying; Returns the UnitGenerator or SynthData at anIndex.
     Returns the UnitGenerator or SynthData at the specified index or nil if 
     anIndex is out of bounds. anIndex is zero-based. */

- preemptFor:aNote;
  /* TYPE: Modifying; Sent when the receiver is preempted.
     The preemptFor: message is sent when the receiver is running or 
     finishing and it is preempted by its SynthInstrument.
     This happens, for example, when a SynthInstrument with 
     3 voices receives a fourth note. It preempts one of the voices by 
     sending it preemptFor:newNote followed by noteOn:newNote. The default
     implementation does nothing and returns self. Normally, a time equal to
     the value returned by MKGetPreemptDuration() is allowed before the new note
     begins. A SynthPatch can specify that the new note happen immediately 
     by returning nil. */

- noteOnSelf:aNote; 
  /* TYPE: Modifying; Specifies behavior in response to noteOns.
     Subclass may override this method to do any initialization needed for 
     each noteOn. noteOnSelf: is sent whenever a new note commences, even if
     the SynthPatch is already running. (The subclass can determine whether or not
     the SynthPatch is already running by examing the status instance
     variable. It should also check for preemption with -phraseStatus.) 

     Returns self or nil if the SynthPatch should immediately
     become idle. The message -noteEnd is sent to the SynthPatch
     if noteOnSelf: returns nil.

     You never invoke this method directly. 
     The default implementation just returns self. */

- noteUpdateSelf:aNote; 
  /* TYPE: Modifying; Specifies behavior in response to noteUpdates. 
     You override but never send this message. It is sent by the noteUpdate:
     method. noteUpdateSelf: should send whatever messages are necessary
     to update the state of the DSP as reflected by the parameters in 
     aNote. */

-(double ) noteOffSelf:aNote; 
  /* TYPE: Modifying; Specifies behavior in response to noteOffs.
     You may override but never call this method. It is sent when a noteOff
     or end-of-noteDur is received. The subclass may override it to do whatever
     it needs to do to begin the final segment of the note or phrase.
     The return value is a duration to wait before releasing the 
     SynthPatch.
     For example, a SynthPatch that has 2 envelope handlers should implement
     this method to send finish to each envelope handler and return
     the maximum of the two. The default implementation returns 0. */

- noteEndSelf; 
  /* TYPE: Modifying; Specifies behavior at end of note or phrase.
     This method is invoked automatically when
     the note or phrase is completed. You never invoke this method directly.
     Subclass may override this to do what it needs to do to insure that
     the SynthPatch produces no output. Usually, the subclass implementation
     sends the -idle message to the Out2sumUGx or Out2sumUGy UnitGenerator. 
     The default implementation just returns self. */

- noteOn:aNote; 
  /* TYPE: Modifying; Start or rearticulate a note or phrase.
     Start or rearticulate a note or phrase by sending 
     [self noteOnSelf:aNote]. If noteOnSelf:aNote returns self, 
     sets status to MK_running, returns self. Otherwise,
     if noteOnSelf returns nil, sends [self noteEnd] and returns nil.
     Ordinarily sent only by SynthInstrument.
     */

- noteUpdate:aNote;
  /* TYPE: Modifying; Update running SynthPatch.
     Sent ordinarily only by the SynthInstrument when a noteUpdate is
     received. Implemented simply as [self noteUpdateSelf:aNote]. */

-(double ) noteOff:aNote; 
  /* TYPE: Modifying; Begin final release of note or phrase.
     Conclude a note or phrase by sending
     [self noteOffSelf:aNote]. Sets status to MK_finishing.
     Returns the release duration as returned by noteOffSelf:.
     Ordinarily sent only by SynthInstrument.
     */

- noteEnd; 
    /* TYPE: Modifying; Conclude note or phrase.
       Causes the receiver to become idle.
       The message noteEndSelf is sent to self, the status
       is set to MK_idle and returns self.
       Ordinarily sent automatically only, but may be sent by anyone to
       immediately stop a patch. 
       */

- moved:aUG; 
  /* TYPE: Modifying; Sent by the Orchestra during memory compaction.
     The moved: message is sent when the Orchestra moves a SynthPatch's
     UnitGenerator during DSP memory compaction. aUG is the unit generator that
     was moved. Subclass occasionally overrides this method.
     The default method does nothing. See also phraseStatus.
     */

-(int ) status; 
 /* TYPE: Querying; Returns status of receiver.
   Returns status of the receiver. This is not necessarily the status
   of all contained UnitGenerators. For example, it is not unusual
   for a SynthPatch to be idle but most of its UnitGenerators, with the
   exception of the Out2sum, to be running. */

-(BOOL ) isEqual:anObject; 
  /* Obsolete. */

-(unsigned ) hash;  
  /* Obsolete. */

- patchTemplate; 
    /* TYPE: Querying; Returns receiver's PatchTemplate.
       Returns PatchTemplate associated with the receiver. */

-(int ) noteTag; 
    /* TYPE: Querying; Returns noteTag of current note or phrase.
       Returns the noteTag associated with the note or phrase the 
       receiver is currently playing. */

- orchestra; 
    /* TYPE: Querying; Returns the receiver's Orchestra. 
       Returns the Orchestra instance to which the receiver belongs. All
       UnitGenerators and SynthData in an instance of SynthPatch are on
       the same Orchestra instance. In the standard NeXT configuration, there
       is one DSP and, thus, one Orchestra instance. */

-(BOOL ) isFreeable; 
  /* TYPE: Querying; Returns whether the receiver is freeable.
     Returns whether or not the receiver may be freed. A SynthPatch may only
     be freed if it is idle and not owned by any SynthInstrument in 
     MK_MANUALALLOC mode. */

- free; 
 /* Same as dealloc */

-controllerValues:controllers;
  /* This message is sent by the SynthInstrument 
     to a SynthPatch when a new tag stream begins, before the noteOn:
     message is sent. The argument, 'controllers' describing the state of
     the MIDI controllers. It is a HashTable object 
     (see /usr/include/objc/HashTable.h), mapping integer controller
     number to integer controller value. The default implementation of this 
     method does nothing. You may override it in a subclass as desired.

     Note that pitchbend is not a controller in MIDI. Thus the current
     pitchbend is included in the Note passed to noteOn:, not in the
     HashTable. See the HashTable spec sheet for details on how to 
     access the values in controllers. The table should not be altered
     by the receiver. 

     */

- next;
  /* TYPE: Querying; Return next active SynthPatch. 
     This method is used in conjunction with a SynthInstrument's
     -preemptSynthPatchFor:patches: method. If you send -next to a SynthPatch
     which is active (not idle) and which is managed by a 
     SynthInstrument,
     the value returned is the next in the list of active SynthPatches (for
     a given PatchTemplate) managed 
     by that SynthInstrument. The list is in the order of the onset times
     of the phrases played by the SynthPatches. */

- freeSelf;
  /* You can optionally implement this method. FreeSelf is sent to the object
     before it is freed. */

-(MKPhraseStatus)phraseStatus;
/* This is a convenience method for SynthPatch subclass implementors.
   The value returned takes into account whether the phrase is preempted, 
   the type of the current note and the status of the synthPatch. 
   If not called by a SynthPatch subclass, returns MK_noPhraseActivity */

 /* -read: and -write: 
  * Note that archiving is not supported in the SynthPatch object, since,
  * by definition the SynthPatch instance only exists when it is resident on
  * a DSP.
  */

@end




