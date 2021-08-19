#ifdef SHLIB
#include "shlib.h"
#endif

/*
  SynthInstrument.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
  */
/* 
Modification history:

  09/19/89/daj - Changed to use Note C functions for efficiency.
  12/15/89/daj - Changed _noteOffAndScheduleEnd: to noteOff:.
  12/18/89/daj - Added flushTimedMessages to setSynthPatchCount: (to fix 
                 bug (suggestion) 3132)
  12/22/89/daj - Added method forgetUpdates.
  12/22/89/daj - Added more allocation failure recovery logic. In particular,
                 it now tries to use a different template when it's losing.
		 (fix for bug (suggestion) 1617)
  01/09/90/daj - Fixed bug in findAltListRunningPatch
  01/24/90/daj - Changed _MKReplaceSynthPatch to _MKReplaceFinishingPatch.
  03/13/90/daj - Minor changes for new categories for private methods.
  03/17/90/daj - Moved _MKInheritsFrom to _musickit.h.
  03/19/90/daj - Added method to return note update and controllers state.
                 Changed to use MKGetNoteClass().
		 Added retainUpdates methods and instance var. Changed
		 forgetUpdates to clearUpdates.
  03/21/90/daj - Added archiving.
  03/22/90/daj - Changed setSynthPatchClass: to be a little more forgiving.
                 (It'll let you set the synthPatchClass if it's nil.)
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  07/23/90/daj - Added orchestra instance var for hand-allocated multi-DSP
                 allocation.
  08/13/90/daj - Added orchestra instance var init in awake method.
  08/23/90/daj - Changed to zone API.
*/

#import <objc/Storage.h>
#import <objc/HashTable.h>

#import "_musickit.h"
#import "_error.h"
#import "SynthPatch.h" // @requires
#import "Conductor.h" // @requires
#import "_Note.h" // @requires
#import "_Orchestra.h" // @requires
#import "UnitGenerator.h" // @requires

#import "_SynthInstrument.h"
@implementation SynthInstrument:Instrument
/* Instances of the SynthInstrument class manage a collection of SynthPatches.
   The principal job of the SynthInstrument is to assign SynthPatches to 
   incoming Notes.

   A SynthInstrument can be in one of two modes, 'manual mode' or 'automatic
   mode'. 
   If the SynthInstrument is in automatic mode, Patches are allocated directly
   from the Orchestra as needed, then returned to the available pool, to be
   shared among all SynthInstruments. This is the default.
   If more Notes arrive than there are synthesizer resources,
   the oldest running Patch of this SynthInstrument is preempted 
   (the SynthPatch is sent the preemptFor:aNote message) 
   and used for the new Note. This behavior can be over-ridden for 
   an alternative "grab" strategy.

   If it is in manual mode,
   a fixed number of Patches are used over and over and you set the
   number of Patches managed. If more Notes arrive than there are Patches
   set aside, the oldest running Patch is preempted (the SynthPatch is 
   sent the preemptFor:aNote message) and used for the new Note. As above,
   this behavior can be over-ridden for an alternative "grab" strategy.   
   You can
   set the number of patches for each template.
   
   Each SynthInstrument instance supports patches of a particular SynthPatch 
   subclass. 

   Mutes Notes are ignored by SynthInstruments.
   */
{
    id synthPatchClass; /* class used to create patches. */
    unsigned short allocMode;  /* One of MK_MANUALALLOC or MK_AUTOALLOC */
    id taggedPatches;  
    id controllerTable;
    id updates;
    BOOL retainUpdates;
    id orchestra;
    id _reservedSynthInstrument1; 
}

#import "_Instrument.h"
/* Set of active SynthPatches but not including those 
   which had no noteTag. Hash from noteTag to patch. */
#define _patchLists _reservedSynthInstrument1
/* Storage object. */

typedef struct {
	@defs (SynthPatch)
} synthPatchStruct;
#import "_SynthPatch.h"
#define NEXT(_y) (((synthPatchStruct *)(_y))->_next)
// #define NEXTSP(_x) ((NEXT(_x) != _x) ? _x : nil)
#define NEXTSP(_x) (NEXT(_x))
/* Beware when using this macro! The list may be changed by your operation */

#define WILD -1

typedef struct _patchList {
    id idleNewest,idleOldest,activeNewest,activeOldest;
    int idleCount,totalCount;
    id template;
} patchList; /* One of these for each template. */

#define PLISTSIZE	(sizeof(patchList))
#define PLISTDESCR	"{@@@@ii@}" 

static patchList *getList(SynthInstrument *self,id template)
    /* Returns list matching template, else NULL. */
{
    register patchList *tmp = (patchList *)[self->_patchLists elementAt:0];
    register patchList *last = tmp + [self->_patchLists count];
    while (tmp < last)
      if (tmp->template == template)
	return tmp;
      else tmp++;
    return NULL;
}

static patchList *addList(SynthInstrument *self,id template)
    /* adds list. Assumes it's not there. */
{
    patchList p = {nil,nil,nil,nil,0,0};
    p.template = template;
    [self->_patchLists insert:(char *)&p at:[self->_patchLists count]];
    return (patchList *)[self->_patchLists elementAt:
			 [self->_patchLists count]-1];
}

static patchList *findListWithIdlePatch(SynthInstrument *self)
{
    register patchList *tmp = (patchList *)[self->_patchLists elementAt:0];
    register patchList *last = tmp + [self->_patchLists count];
    while (tmp < last)
      if (tmp->idleOldest) 
	return tmp;
      else tmp++;
    return NULL;
}


#define VERSION2 2

static NXZone *zone; /* Cache zone for copying notes. */

+initialize
{
    if (self != [SynthInstrument class])
      return self;
    [self setVersion:VERSION2];
    zone = NXDefaultMallocZone();
    return self;
}

-init
  /* Does instance initialization. Sent by superclass on creation. 
     If subclass overrides this method, it must send [super initialize]
     before setting its own defaults. */
{
    _MKLinkUnreferencedClasses([SynthPatch class]);
    [super init];
    _patchLists = 
      [Storage newCount:0 elementSize:PLISTSIZE description:PLISTDESCR];
    taggedPatches = [HashTable newKeyDesc:"i"];
    controllerTable = [HashTable newKeyDesc:"i" valueDesc:"i"];
    [self addNoteReceiver:[NoteReceiver new]];
    updates = [MKGetNoteClass() new];  /* For remembering partUpdate 
				parameter values on this channel. */
    [updates setNoteType:MK_noteUpdate];
    _MKOrchAddSynthIns(self);
    orchestra = _MKClassOrchestra();
    return self;
}

-copyFromZone:(NXZone *)zone
  /* Returns a copy of the receiver. The copy has the same connections but
     has no synth patches allocated. */
{
    SynthInstrument *newObj = [super copyFromZone:zone];
    newObj->_patchLists = 
      [Storage newCount:0 elementSize:PLISTSIZE description:PLISTDESCR];
    newObj->taggedPatches = [HashTable newKeyDesc:"i"];
    newObj->controllerTable = [HashTable newKeyDesc:"i" valueDesc:"i"];
    newObj->updates = [MKGetNoteClass() new];  
    /* For remembering no-tag noteUpdate parameter values on this channel. */
    [newObj->updates setNoteType:MK_noteUpdate];
    _MKOrchAddSynthIns(newObj);
    return newObj;
}

-setRetainUpdates:(BOOL)yesOrNo
{
    retainUpdates = yesOrNo;
    return self;
}

-(BOOL)doesRetainUpdates
{
    return retainUpdates;
}

-preemptSynthPatchFor:aNote patches:firstPatch
  /* You never send this message. Rather, 
     this method is invoked when we are in manual allocation mode and 
     all SynthPatches are in use or we are in auto allocation mode
     and no more DSP resources are available. The default implementation
     returns the Patch with the appropriate template whose phrase began the 
     earliest  (This is the same value returned by the method -activeSynthPatches:.)

     You may override this method to provide a different scheme for 
     determining which Patch to grab. For example, you might want to
     grab the quietest Patch. To do this, you need to examine the currently
     running patches and choose one. (see -activeSynthPatches: below).
     The subclass may return nil to signal that the new Note should be
     omitted. It is illegal to return a patch which is not a member of the
     active patch list.
     
     The subclass should not send the 
     preemptFor: message to the patch. This is done automatically for you.
     */	
{
    return firstPatch;
}

-activeSynthPatches:aTemplate
  /* Returns the first in the list of patches currently sounding with the
     specified template. If aTemplate is nil, 
     [synthPatchClass defaultPatchTemplate] is used.
     The list is ordered by when the phrase began, from the earliest to
     the latest. In addition, all finishing SynthPatches are before all
     running ones. You step down the list by sending the -next message to
     each patch. Returns nil if there are no patches sounding with that
     template. */
{
    patchList *p;
    if (!aTemplate)
      aTemplate = [synthPatchClass defaultPatchTemplate];
    p = getList(self,aTemplate);
    if (p)
      return p->activeOldest;
    return nil;
}

-mute:aMute
  /* This method is invoked when a Note of type mute is received.
     Notes of type mute are not sent to SynthPatches because they do not deal 
     directly with sound production. The default implementation does
     nothing. A subclass may implement this 
     method to look at the parameters of aMute and perform some appropriate
     action. 

     */
{
    return self;
}

static char *tagStr(int noteTag)
{
    if (noteTag == MAXINT)
      return "<none>";
    return _MKIntToStringNoCopy(noteTag);
}

static void adjustRunningPatch(SynthInstrument *self,int noteTag,id aNote,
			       id currentPatch,patchList *aPatchList,
			       MKPhraseStatus *phraseStatus)
{
    _MKRemoveSynthPatch(currentPatch, 
			&aPatchList->activeNewest,
			&aPatchList->activeOldest,
			_MK_ACTIVELIST);
    _MKSynthPatchPreempt(currentPatch,aNote,self->controllerTable); 
    if (noteTag != MAXINT) 
      [self->taggedPatches removeKey:(const void *)
       [currentPatch noteTag]];
    if (MKIsTraced(MK_TRACESYNTHINS) ||
	MKIsTraced(MK_TRACEPREEMPT))
      fprintf(stderr,
	      "SynthInstrument preempts patch %d at time %f "
	      "for tag %s.\n",
	      currentPatch,MKGetTime(),tagStr(noteTag));
    *phraseStatus = MK_phraseOnPreempt;
}

static id findAltListRunningPatch(SynthInstrument *self,id aNote,patchList **aPatchList)
{
    register patchList *tmp = (patchList *)[self->_patchLists elementAt:0];
    register patchList *last = tmp + [self->_patchLists count];
    id currentPatch;
    while (tmp < last) {
	if (tmp != *aPatchList)
	  if (tmp->activeOldest) { /* don't bother if there's no patches */
	      currentPatch = [self preemptSynthPatchFor:aNote patches:
			      tmp->activeOldest];
	      if (currentPatch) {
		  *aPatchList = tmp;
		  return currentPatch;
	      } 
	  }
	tmp++;
    }
    return nil;
}

static id adjustIdlePatch(patchList *aPatchList,int noteTag)
{
    id currentPatch = aPatchList->idleOldest;
    if (!currentPatch) 
      return nil;
    _MKRemoveSynthPatch(currentPatch,
			&aPatchList->idleNewest,
			&aPatchList->idleOldest,
			_MK_IDLELIST);
    aPatchList->idleCount--;
    if (MKIsTraced(MK_TRACESYNTHINS))
      fprintf(stderr,
	      "SynthInstrument uses patch %d at time %f "
	      "for tag %s.\n",
	      currentPatch,MKGetTime(),tagStr(noteTag));
    return currentPatch;
}

static void alternatePatchMsg(void)
{
    fprintf(stderr,"(No patch of requested template"
	    "was available. Using alternative template.");
}

-realizeNote:aNote fromNoteReceiver:aNoteReceiver
  /* Does SynthPatch allocation. 
     
     The entire algorithm is given below. The new steps are so-indicated:
     
     MANUAL:
     1m	Look for idle patch of correct template.
     2m	Else try and preempt patch of correct template.
     3m	Else look for idle patch of incorrect template.
     4m	Else try and preempt patch of incorrect template.
     5m	Else give up.
     
     AUTO
     1a	Try to alloc a new patch of correct template.
     2a	Else try and preempt patch of correct template.
     3a	Else try and preempt patch of incorrect template.
     4a	Else give up.
     */
{
    int noteTag;
    id currentPatch;
    MKNoteType curNoteType;
    if (!aNote)
      return nil;
    noteTag = [aNote noteTag];
    curNoteType = [aNote noteType];

    if (noteTag != MAXINT)       
      currentPatch =  (id)[taggedPatches valueForKey:(const void *)noteTag];
    else switch(curNoteType) {
      case MK_noteDur:
      case MK_noteUpdate:
 	currentPatch = nil;
	break;
      case MK_mute:
	if (MKIsTraced(MK_TRACESYNTHINS))
	  fprintf(stderr,
		  "SynthInstrument receives mute Note at time %f.\n",
		  MKGetTime());
 	[self mute:aNote];
      default:       /* NoteOn or noteOff with no tag or a mute (ignored) */
	return nil;
    }

    switch (curNoteType) {
      case MK_noteDur:
      case MK_noteOn: {
	  MKPhraseStatus phraseStatus;
	  if (currentPatch) {/* We have an active patch already for this tag */
	      phraseStatus = MK_phraseRearticulate;
	      if (MKIsTraced(MK_TRACESYNTHINS))
		fprintf(stderr,
			"SynthInstrument receives note for active notetag "
			"stream %s at time %f.\n",tagStr(noteTag),MKGetTime());
	  }
	  else {  /* It is a new phrase. */
	      id aTemplate;
	      patchList *aPatchList;
	      phraseStatus = MK_phraseOn;
	      aNote = [aNote copyFromZone:zone];
	      /* Copy common updates into aNote. */
	      aNote = [aNote _unionWith:updates];
	      aTemplate = [synthPatchClass patchTemplateFor:aNote];
	      aPatchList = getList(self,aTemplate);
	      if (!aPatchList)
		aPatchList = addList(self,aTemplate);
              if (MKIsTraced(MK_TRACESYNTHINS))
		fprintf(stderr,
			"SynthInstrument receives note for new notetag stream "
			"%s at time %f ",tagStr(noteTag),MKGetTime());
	      if (allocMode == MK_AUTOALLOC) { 
		  currentPatch = 
		    [orchestra allocSynthPatch:
		     synthPatchClass patchTemplate:aTemplate];
		  if ((currentPatch) && (MKIsTraced(MK_TRACESYNTHINS)))
		    fprintf(stderr,
			    "SynthInstrument creates patch %d at time %f "
			    "for tag %s.\n",
			    currentPatch,MKGetTime(),tagStr(noteTag));
	      } 
	      else 
		currentPatch = adjustIdlePatch(aPatchList,noteTag);
	      if (!currentPatch) { /* Allocation failure */
		  currentPatch = [self preemptSynthPatchFor:aNote patches:
				  aPatchList->activeOldest];
		  if (currentPatch)
		    adjustRunningPatch(self,noteTag,aNote,currentPatch,
				       aPatchList,&phraseStatus);
		  else { /* Try and use a patch of a different template */
		      if (allocMode == MK_MANUALALLOC) {
			  aPatchList = findListWithIdlePatch(self);
			  if (aPatchList)
			    currentPatch = adjustIdlePatch(aPatchList,noteTag);
		      }
		      if (currentPatch) {
			  if (MKIsTraced(MK_TRACESYNTHINS))
			    alternatePatchMsg();
		      }
		      else { /* Keep trying */
			  currentPatch = 
			    findAltListRunningPatch(self,aNote,&aPatchList);
			  if (currentPatch) {
			      adjustRunningPatch(self,noteTag,aNote,
						 currentPatch,
						 aPatchList,&phraseStatus);
			      if (MKIsTraced(MK_TRACESYNTHINS))
				alternatePatchMsg();
			  }
		      }
		      if (!currentPatch) { /* Now we give up. */
			  if (MKIsTraced(MK_TRACESYNTHINS) ||
			      MKIsTraced(MK_TRACEPREEMPT)) 
			    fprintf(stderr,
				    "SynthInstrument omits note at time %f "
				    "for tag %s.\n",
				    MKGetTime(),tagStr(noteTag));
			  _MKErrorf(MK_synthInsOmitNoteErr,MKGetTime());
			  [aNote free];
			  return nil;
		      }
		  } 
	      }
	      /* We're ok if we made it to here. */
	      _MKAddPatchToList(currentPatch,&aPatchList->activeNewest,
				&aPatchList->activeOldest,_MK_ACTIVELIST);
	      _MKSynthPatchSetInfo(currentPatch,noteTag,self);
	      /* First set noteTag, then add to taggedPatchSet. */
	      if (noteTag != MAXINT) 
		[taggedPatches insertKey:(const void *)noteTag value:
		 (void *)currentPatch];
	      if (phraseStatus == MK_phraseOnPreempt)
		return self;
	      [currentPatch controllerValues:controllerTable];
	  } 
	  if (![currentPatch noteOn:aNote]) { /* Synthpatch abort? */
	      if (phraseStatus == MK_phraseOn)
		[aNote free];
	      return nil;
	  }
	  if (curNoteType == MK_noteDur) 
	    _MKSynthPatchNoteDur(currentPatch,aNote,
				 (noteTag == MAXINT) ? YES : NO);
	      /* If noteTag is MAXINT, the patch is not addressable. 
		 Therefore, the SynthPatch need not go through the
		 SynthInstrument for it's auto-generated noteOff and 
		 can handle the noteOff: message itself. */
	  if (phraseStatus == MK_phraseOn)
	    [aNote free];
	  return self;
      }
      case MK_noteUpdate:
	if (noteTag == MAXINT) { /* It's a broadcast */ 
	    register patchList *tmp;
	    register patchList *last;
	    for (tmp  = (patchList *)[_patchLists elementAt:0],
		 last = tmp + [_patchLists count];
		 (tmp < last);
		 tmp++) {
		currentPatch = tmp->activeOldest;
		if (currentPatch) 
		  do {
		      [currentPatch noteUpdate:aNote];
		  } while(currentPatch = NEXTSP(currentPatch)); 
	    }
	    /* Now save the parameters in a note in updates so 
	       that new  notes on this channel can be inited properly. */
	    [updates copyParsFrom:aNote];
	    {
		/* Control change has to be handled separately, since there
		   can be several values that all need to be maintained. Sigh.
		   */
		int controller = MKGetNoteParAsInt(updates,MK_controlChange);
		int controlVal;
		if (controller != MAXINT) {
		    controlVal = MKGetNoteParAsInt(updates,MK_controlVal);
		    [updates removePar:MK_controlChange];
		    if (controlVal != MAXINT)  {
			[controllerTable insertKey:(const void *)controller 
		         value:(void *)controlVal];
			[updates removePar:MK_controlVal];
		    }
		}
	    }
	    return self;
	}
	else { /* It's an ordinary note update. */
	    if (!currentPatch)
	      return nil;
	    [currentPatch noteUpdate:aNote];
	    return self;
	}
      case MK_noteOff:
	if (!currentPatch) 
	  return nil;
        [currentPatch noteOff:aNote];
	return self;
    }
#if _MK_MAKECOMPILERHAPPY
    return self; /* This can never happen */
#endif
}

-orchestra
{
    return orchestra ? orchestra : _MKClassOrchestra();
}

-setSynthPatchClass:aSynthPatchClass orchestra:anOrch
  /* Set synthPatchClass as specified. If the receiver is in a performance
     or if aSynthPatchClass doesn't inherit from SynthPatch, 
     does nothing and returns nil. Otherwise, returns self. */
{
    if ((_noteSeen && (aSynthPatchClass != nil))
	|| (!_MKInheritsFrom(aSynthPatchClass,[SynthPatch class]))) 
      return nil;
    synthPatchClass = aSynthPatchClass;
    [synthPatchClass initialize]; /* Make sure PartialsDatabase is inited */
    if (!anOrch)
      orchestra = [synthPatchClass orchestraClass];
    return self;
}

-setSynthPatchClass:aSynthPatchClass
{
    return [self setSynthPatchClass:aSynthPatchClass
	  orchestra:nil];
}

-synthPatchClass
  /* Returns synthPatchClass */
{
    return synthPatchClass;
}

-free
  /* Frees the receiver. It is illegal to send this message to a 
     SynthInstrument which is in performance. Returns self in this case,
     otherwise nil. */
{     
    if (_noteSeen)
      return self;
    [self abort];
    [updates free]; 
    [controllerTable free];
    [taggedPatches free];
    _MKOrchRemoveSynthIns(self);
    return [super free];
}

static void deallocRunningVoices(SynthInstrument *self,id orch)
    /* Broadcasts 'noteEnd' to all running voices on given orch. (nil orch
       is wild card) */
{
    register id aPatch,nextPatch;
    register patchList *aPatchList;
    register patchList *last;
    for (aPatchList  = (patchList *)[self->_patchLists elementAt:0],
	 last = aPatchList + [self->_patchLists count];
	 (aPatchList < last);
	 aPatchList++) {
	for (aPatch = aPatchList->activeOldest; aPatch; aPatch = nextPatch) {
	    nextPatch = NEXTSP(aPatch);  
	    if ((!orch) || ([aPatch orchestra] == orch)) 
	      [aPatch noteEnd];
	}
    }
}

static void releaseIdlePatch(id aPatch,patchList *aPatchList)
{
    _MKRemoveSynthPatch(aPatch, &(aPatchList->idleNewest),
			&(aPatchList->idleOldest),_MK_IDLELIST);
    [aPatch _deallocate];
    aPatchList->totalCount--;
    aPatchList->idleCount--;
}

static void deallocIdleVoices(SynthInstrument *self,id orch)
    /* Deallocates all idle voices using orch. If orch is nil, it's a wild
       card. */
{
    register id aPatch,nextPatch;
    register patchList *aPatchList;
    register patchList *last;
    for (aPatchList  = (patchList *)[self->_patchLists elementAt:0],
	 last = aPatchList + [self->_patchLists count];
	 (aPatchList < last);
	 aPatchList++) {
	for (aPatch = aPatchList->idleOldest; aPatch; aPatch = nextPatch) {
	    nextPatch = NEXTSP(aPatch);  
	    if ((!orch) || ([aPatch orchestra] == orch)) 
	      releaseIdlePatch(aPatch,aPatchList);
	}
    }
}

-abort
  /* Sends the noteEnd message to all running (or finishing) synthPatches 
     managed by this SynthInstrument. This is used only for aborting in 
     emergencies. */
{
    deallocRunningVoices(self,nil); /* Must be first */
    return self;
}

-clearUpdates
/* Causes the SynthInstrument to forget any noteUpdate state it has accumulated
   as a result of receiving noteUpdates without noteTags.
   The effect is not felt by the SynthPatches until the next phrase. Also
   clears controller info.
 */
{
    [controllerTable empty];
    [updates free];
    updates = [MKGetNoteClass() new];  
    [updates setNoteType:MK_noteUpdate];
    return self;
}

-_disconnectOnOrch:anOrch
  /* Same as disconnect but only affects voices running on the given orch. */
{
    deallocRunningVoices(self,anOrch); /* Must be first. Makes them all on
					  idle list. */
    deallocIdleVoices(self,anOrch);    /* Releases idle lists. */
    [_patchLists free];  
    _patchLists = 
      [Storage newCount:0 elementSize:PLISTSIZE description:PLISTDESCR];
    if (!retainUpdates)
      [self clearUpdates];
    return self;
}

-autoAlloc
  /* Sets allocation mode to MK_AUTOALLOC and releases any manually 
     allocated patches. If the receiver is in performance and the
     receiver is not already in MK_AUTOALLOC mode, this message is
     ignored and nil is returned. Otherwise, returns self. */
{
    if ([self inPerformance] && (allocMode != MK_AUTOALLOC))
      return nil;
    deallocIdleVoices(self,nil); /* Frees up all idle voices. */
    allocMode = MK_AUTOALLOC;    
    return self;
}

-(unsigned short)allocMode
{
    return allocMode;
}

-(int)synthPatchCountForPatchTemplate:aTemplate
  /* Returns number of manually-allocated voices for the specified template. 
     If the receiver is in automatic allocation mode, returns 0. */
{
    patchList *aPatchList;
    if (allocMode == MK_AUTOALLOC) 
      return 0;
    if (!aTemplate)
      aTemplate = [synthPatchClass defaultPatchTemplate];
    aPatchList = getList(self,aTemplate);
    if (!aPatchList)
      return 0;
    return aPatchList->totalCount;
}

-(int)synthPatchCount
  /* Returns number of manually-allocated voices for the default template. */
{
    return [self synthPatchCountForPatchTemplate:nil];
}

-(int)setSynthPatchCount:(int)voices
  /* Same as setSynthPatchCount:voices template:nil. */
{
    return [self setSynthPatchCount:voices patchTemplate:nil];
}

-(int)setSynthPatchCount:(int)voices patchTemplate:aTemplate
  /* Sets the synthPatchCount for the given template.
     This message may only be sent when the Orchestra is open.
     If in performance and the receiver is not in manual allocation mode,
     this message is ignored. If aTemplate is nil, the value returned by
     [synthPatchClass defaultPatchTemplate] is used. Returns the number of 
     voices for the given template. If the number of voices is decreased,
     the extra voices are allowed to finish in the normal manner. */
{
    id aPatch,nextPatch;
    int i,j;
    patchList *aPatchList;
    if ((!synthPatchClass) || 
	(([self inPerformance] && (allocMode != MK_MANUALALLOC))) ||
	(voices < 0))
      return 0;
    allocMode = MK_MANUALALLOC;
    if (!aTemplate)
      aTemplate = [synthPatchClass defaultPatchTemplate];
    aPatchList = getList(self,aTemplate);
    if (!aPatchList)
      aPatchList = addList(self,aTemplate);
    if (voices == aPatchList->totalCount)
      return voices;
    if (voices < aPatchList->totalCount) { /* Releasing some */
	/* First release the idle ones. */
	j = aPatchList->totalCount - voices;
	for (i = 0, aPatch = aPatchList->idleOldest; 
	     (aPatch && (i < j));
	     aPatch = nextPatch, i++)  {
	    nextPatch = NEXTSP(aPatch);
	    releaseIdlePatch(aPatch,aPatchList);
	}
	/* Now we explicitly set totalCount to be the number we want (so that
	   the _dealloc method will release the active ones for us). */
	aPatchList->totalCount = voices;
	return voices;
    }
    while (aPatchList->totalCount < voices) {
	aPatch = [orchestra
		allocSynthPatch:synthPatchClass patchTemplate:aTemplate];
	if (!aPatch)
	  break;
	aPatchList->totalCount++;  
	_MKSynthPatchSetInfo(aPatch, MAXINT, self);
	aPatchList->idleCount++;
	_MKAddPatchToList(aPatch,&aPatchList->idleNewest,
			  &aPatchList->idleOldest,_MK_IDLELIST);
    }
    [_MKClassOrchestra() flushTimedMessages];
    return aPatchList->totalCount;
}

-getUpdates:(Note **)aNoteUpdate controllerValues:(HashTable **)controllers
/* Returns by reference the NoteUpdate used to store the accumulated 
   noteUpdate state. Also returns by reference the HashTable used to 
   store the state of the controllers. Any alterations to the returned
   objects will effect future phrases. The returned objects must not
   be freed. */
{
    *aNoteUpdate = updates;
    *controllers = controllerTable;
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: method. Also archives allocMode, retainUpdates 
     and, if retainUpdates is YES, the controllerTable and updates. */
{
    /* FIXME Is this ok? */
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"#sc",&synthPatchClass,&allocMode,
		 &retainUpdates);
    if (retainUpdates) 
      NXWriteTypes(aTypedStream,"@@",&controllerTable,&updates);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"SynthInstrument") == VERSION2){
	NXReadTypes(aTypedStream,"#sc",&synthPatchClass,&allocMode,
		    &retainUpdates);
	if (retainUpdates) 
	  NXReadTypes(aTypedStream,"@@",&controllerTable,&updates);
    }
    return self;
}

-awake
  /* Makes unarchived object ready for use. */
{
    /* See initialize above. */
    [super awake];
    _patchLists = 
      [Storage newCount:0 elementSize:PLISTSIZE description:PLISTDESCR];
    taggedPatches = [HashTable newKeyDesc:"i"];
    if (!controllerTable)
      controllerTable = [HashTable newKeyDesc:"i" valueDesc:"i"];
    if (!updates) {
	updates = [MKGetNoteClass() new];
	[updates setNoteType:MK_noteUpdate];
    }
    orchestra = _MKClassOrchestra();
    _MKOrchAddSynthIns(self);
    return self;
}

@end


@implementation SynthInstrument(Private)

-_repositionInActiveList:synthPatch template:patchTemplate
  /* -activeSynthPatches used to list patches in the order of their noteOns.
     Now it lists all patches that are finishing first, in the order of their 
     noteOffs, then all other active patches, in the order of their noteOns.
     */
{
    patchList *aPatchList = getList(self,patchTemplate);
    if (!aPatchList) /* Should never happen. */
      return nil;
    _MKReplaceFinishingPatch(synthPatch,
			     &aPatchList->activeNewest,
			     &aPatchList->activeOldest,
			     _MK_ACTIVELIST);
    return self;
}

-_deallocSynthPatch:aSynthPatch template:aTemplate tag:(int)noteTag
    /* Removes SynthPatch from active list, possibly adding it to idle list. 
       Returns nil if the SynthPatch is being deallocated, else self. */
{
    patchList *aPatchList = getList(self,aTemplate);
    if (!aPatchList) {            /* Should never happen. */
	return nil;
    }
    _MKRemoveSynthPatch(aSynthPatch, &aPatchList->activeNewest, 
			&aPatchList->activeOldest,_MK_ACTIVELIST);
    if (noteTag != MAXINT)
      [taggedPatches removeKey:(void *)noteTag];
    if ((allocMode == MK_AUTOALLOC) || 
	(aPatchList->totalCount == aPatchList->idleCount)){/* A released one */
	[aSynthPatch _deallocate];
	return nil;
    }
    aPatchList->idleCount++;
    _MKAddPatchToList(aSynthPatch,&aPatchList->idleNewest,
		      &aPatchList->idleOldest,_MK_IDLELIST);
    return self;
}

@end

