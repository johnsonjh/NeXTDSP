#ifndef __SYNTHELEMENT_H
#define __SYNTHELEMENT_H
#define _orchIndex _reservedSynthElement1      /* Which DSP. */
#define _synthPatchLoc _reservedSynthElement2  
  /* Index in synthpatch where we are. */
#define _sharedKey _reservedSynthElement3 
  /* In case this is a representation of a shared object. */
#define _protected _reservedSynthElement4
/* Used for some special synthdatas like
   zero and sink that should never go away. */
#define _privStruct _reservedSynthElement5
#endif __SYNTHELEMENT_H



