/* Included by UnitGenerator.m */

/* 
Modification history:

  11/20/89/daj - Minor change to do lazy garbage collection of synth data. 
  04/21/90/daj - Changes to make compiler happy with -W switches on.
*/

#define SynthElement UnitGenerator 
/* It's actually either UnitGenerator or SynthData, but this makes compiler 
   happy */

id _MKSetSynthElementSynthPatchLoc(SynthElement *synthEl,unsigned short loc)
    /* Used for cross-ref into SynthPatch location. */
{
    synthEl->_synthPatchLoc = loc;
    return synthEl;
}

unsigned _MKGetSynthElementSynthPatchLoc(SynthElement *synthEl)
    /* Used for cross-ref into SynthPatch location */
{
    return synthEl->_synthPatchLoc;
}

void _MKProtectSynthElement(SynthElement *synthEl,BOOL protectIt)
{
    synthEl->_protected = protectIt;
}    

static void doDealloc(SynthElement *synthEl,BOOL shouldIdle)
{
    if (shouldIdle)
      [synthEl idle];
    synthEl->synthPatch = nil;
    if (_MK_ORCHTRACE(synthEl->orchestra,MK_TRACEORCHALLOC))
	_MKOrchTrace(synthEl->orchestra,MK_TRACEORCHALLOC,"Deallocating %s",
		     [synthEl name]);
    _MKOrchResetPreviousLosingTemplate(synthEl->orchestra);
    [synthEl _deallocAndAddToList];
}

id _MKDeallocSynthElementSafe(SynthElement *synthEl,BOOL lazy)
  /* Deallocates receiver and frees syntpatch of which it's a member, if any. 
     returns nil */
{
    if ((![synthEl isAllocated]) || (synthEl->_protected))
      return nil;
    if (synthEl->_sharedKey) {
	if (_MKReleaseSharedSynthClaim(synthEl->_sharedKey,lazy))
	  return nil;
	else synthEl->_sharedKey = nil;
    }
    if (synthEl->synthPatch) {
	if (![synthEl->synthPatch isFreeable])
	  return nil;
	else [synthEl->synthPatch _free];
    }
    else {
	doDealloc(synthEl,YES);
    }
    return nil;
}


void _MKDeallocSynthElement(SynthElement *synthEl,BOOL shouldIdle)
  /* Deallocate a SynthElement. The SynthElement is not unloaded
     but is slated for possible garbage collection. */
{
    if (![synthEl isAllocated])
      return;
    doDealloc(synthEl,shouldIdle);
}

