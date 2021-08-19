/* 
Modification history:

  1/24/90/daj - Flushed _preempted and _noteDurOff instance variables. 
  03/13/90/daj - Added private category.
*/

#ifndef __SYNTHPATCH_H
#define __SYNTHPATCH_H

#import "SynthPatch.h"

/* Private instance variables */
#define _whichList _reservedSynthPatch1 /* Which list am I on? */
#define _orchIndex _reservedSynthPatch2 /* Which DSP. */
#define _next      _reservedSynthPatch3 /* Used internally for linked list of 
					   active SynthPatches. */
#define _noteEndMsgPtr _reservedSynthPatch4 
/* Used to unqueue noteEnd request.  If non-null, we have seen a noteOff
   but are not yet noteEnd. */
#define _noteDurMsgPtr _reservedSynthPatch5 
/* Used to unqueue noteOff:aNote request. Non-null if we have seen a 
   noteDur recently. */
/* Used to remember tagged NoteOffs auto-generated from NoteDurs. */
#define _sharedKey _reservedSynthPatch6
#define _notePreemptMsgPtr _reservedSynthPatch8
#define _phraseStatus   _reservedSynthPatch10

/* The following were used in 1.0 but not 2.0. Retained for backward
   header file compatability. */
#define _dummy1 _reservedSynthPatch7
#define _dummy2 _reservedSynthPatch9

#define _extraSynthPatchVars _reservedSynthPatch11 /* Currently unused */

/* SynthPatch functions and defines */
extern void _MKSynthPatchPreempt(id aPatch,id aNote,id controllers);
extern id _MKAddPatchToList(id self,id *headP,id *tailP,unsigned short listFlag);
extern id _MKSynthPatchSetInfo(id synthP, int aNoteTag, id synthIns);
extern id _MKSynthPatchNoteDur(id synthP,id aNoteDur,BOOL noTag);
extern void _MKSynthPatchScheduleNoteEnd(id synthP,double releaseDur);
extern id _MKRemoveSynthPatch(id synthP,id *headP,id *tailP,
			      unsigned short listFlag);
extern void _MKReplaceFinishingPatch(id synthP,id *headP,id *tailP,
				     unsigned short listFlag);
extern id _MKSynthPatchCmp();


#define _MK_IDLELIST 1
#define _MK_ACTIVELIST 2
#define _MK_ORCHTMPLIST 3

@interface SynthPatch(Private)

+_newWithTemplate:(id)aTemplate
 inOrch:(id)anOrch index:(int)whichDSP;
-_free;
-_preemptNoteOn:aNote controllers:controllers;
-_remove:aUG;
-_add:aUG;
-_prepareToFree:(id *)headP :(id *)tailP;
-_freeList:head;
-(void)_setShared:aSharedKey;
-(void)_addSharedSynthClaim;
-_connectContents;
-(void)_allocate;
-_deallocate;
-(BOOL)_usesEMem:(MKOrchMemSegment) segment;

@end

#endif

