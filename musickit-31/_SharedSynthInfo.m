#ifdef SHLIB
#include "shlib.h"
#endif

/*
  _SharedDataInfo.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
  */
/* 
Modification history:

  11/20/89/daj - Changed to do lazy garbage collection of synth data. 
*/

/* need to document that you must never free an object that was installed
   in the shared data table during a performance, even if its reference count 
   has gone to 0. You may free it after the performance.

   That's because I don't remove the mapping from the object (e.g partials
   object) to the List of sharedDataInfos.

*/
   
 
#import "_musickit.h"
#import "_UnitGenerator.h"
#import "_SharedSynthInfo.h"
#import "_Orchestra.h"
#import <objc/HashTable.h>

@implementation _SharedSynthInfo:Object
  /* Private class. */
{
    id synthObject;           /* The value we're interested in finding. */
    id theList;               /* Back pointer to List. */
    id theKeyObject;          /* Back pointer to key object. */
    MKOrchMemSegment segment; /* Which segment or MK_noSegment for wildcard. */
    int length;               /* Or 0 for wild card */
    int referenceCount;       
}

/* The shared object table is a HashTable that hashes from id (e.g. Partials
   object) to a List object. The List is a List of _SharedSynthInfos.
   Each _SharedSynthInfo contains a back-pointer to the List. */

enum {obj, objSegment, objSegmentLength};

/* Functions for freeing just the values of the table. */

static void freeObject (void *aList) 
{
    _SharedSynthInfo **el;
    id myList = (id)aList;
    unsigned n;
    for (el = NX_ADDRESS(myList), n = [myList count]; n--; el++) 
      [(*el)->synthObject _setShared:nil];
    [myList freeObjects];
    [myList free];
};
    
static void noFree (void *item) {};
    
id _MKFreeSharedSet(id sharedSet,NXHashTable **garbageTable)
{
    [sharedSet freeKeys:noFree values:freeObject];
    NXFreeHashTable(*garbageTable);
    *garbageTable = NULL;
    return [sharedSet free];
}

id _MKNewSharedSet(NXHashTable **garbageTable)
{
    static NXHashTablePrototype proto = {0};
    proto.free = NXNoEffectFree;
    *garbageTable = NXCreateHashTable(proto,0,NULL);
    return [HashTable new];
}

static void reallyRelease(_SharedSynthInfo *aSharedSynthInfo)
{
    [aSharedSynthInfo->theList removeObject:aSharedSynthInfo];
    [aSharedSynthInfo free];
}

BOOL _MKReleaseSharedSynthClaim(_SharedSynthInfo *aSharedSynthInfo,BOOL lazy)
    /* Returns YES if still in use or slated for garbage collection.
       Returns NO if it can be deallocated now. */
{
    if (--aSharedSynthInfo->referenceCount > 0)
      return YES; /* Still in use */
#   define ORCH [aSharedSynthInfo->synthObject orchestra]
    if (lazy) {
	NXHashInsert(_MKGetSharedSynthGarbage(ORCH),
		     (const void *)aSharedSynthInfo); 
	return YES;
    }
    else reallyRelease(aSharedSynthInfo);
    return NO;
}

void _MKAddSharedSynthClaim(_SharedSynthInfo *aSharedSynthInfo)
{
    aSharedSynthInfo->referenceCount++;
}

int _MKGetSharedSynthReferenceCount(_SharedSynthInfo *aSharedSynthInfo)
{
    return aSharedSynthInfo->referenceCount;
}

static id findSharedSynthInfo(id aList,MKOrchMemSegment whichSegment,int howLong)
{
    register _SharedSynthInfo **el;
    unsigned n;
    BOOL test;
    BOOL isEqualFlag = ((whichSegment == MK_noSegment) ? obj : 
			(howLong == 0) ? objSegment : 
			objSegmentLength);
    for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++) {
	switch (isEqualFlag) {
	  case obj:
	    return (*el);
	  case objSegment:
	    test = (whichSegment == (*el)->segment);
	    break;
	  case objSegmentLength:
	    test = ((whichSegment == (*el)->segment) && 
		    (howLong == (*el)->length));
	    break;
	}
	if (test) 
	  return (*el);
    }
    return nil;
}

id _MKFindSharedSynthObj(id sharedSet,NXHashTable *garbageTable,id aKeyObj,
			 MKOrchMemSegment whichSegment,int howLong)
{
    id aList = [sharedSet valueForKey:(const void *)aKeyObj];
    id rtnVal;
    _SharedSynthInfo *aSharedSynthInfo;
    if (!aList)
      return nil;
    aSharedSynthInfo = findSharedSynthInfo(aList,whichSegment,howLong);
    if (!aSharedSynthInfo)
      return nil;
    rtnVal = aSharedSynthInfo->synthObject;
    if (aSharedSynthInfo->referenceCount == 0)   /* Was lazily deallocated */
      NXHashRemove(garbageTable,(const void *)aSharedSynthInfo);
    [rtnVal _addSharedSynthClaim];
    return rtnVal;
}	

BOOL _MKCollectSharedDataGarbage(id orch,NXHashTable *garbageTable)
    /* Deallocates all garbage and empties the table. */
{
    id dataObj;
    BOOL gotOne = NO;
    _SharedSynthInfo *infoObj;
    NXHashState	state = NXInitHashState(garbageTable);
    if (_MK_ORCHTRACE(orch,MK_TRACEORCHALLOC))
      _MKOrchTrace(orch,MK_TRACEORCHALLOC,
		   "Garbage collecting unreferenced shared data.");
    while (NXNextHashState (garbageTable, &state, (void **)&infoObj)) {
	gotOne = YES;
	dataObj = infoObj->synthObject;
	[dataObj _setShared:nil];
	reallyRelease(infoObj);
	_MKDeallocSynthElement(dataObj,NO);
    }
    if (gotOne)
      NXEmptyHashTable(garbageTable);
    else if (_MK_ORCHTRACE(orch,MK_TRACEORCHALLOC))
      _MKOrchTrace(orch,MK_TRACEORCHALLOC,"No unreferenced shared data found.");
    return gotOne;
}	

BOOL _MKInstallSharedObject(id sharedSet,id aSynthObj,
			    id aKeyObj,MKOrchMemSegment whichSegment,
			    int howLong)
    /* Returns nil if object is already in Set */
{
    _SharedSynthInfo *aSharedSynthInfo;
    id aList = [sharedSet valueForKey:(const void *)aKeyObj];
    if (!aList) {
	aList = [List new];
	[sharedSet insertKey:(const void *)aKeyObj value:(void *)aList];
    }
    else if (findSharedSynthInfo(aList,whichSegment,howLong))
      return NO;
    aSharedSynthInfo = [_SharedSynthInfo new];
    aSharedSynthInfo->synthObject = aSynthObj;
    aSharedSynthInfo->theList = aList;
    aSharedSynthInfo->theKeyObject = aKeyObj;
    aSharedSynthInfo->segment = whichSegment;
    aSharedSynthInfo->length = howLong;
    aSharedSynthInfo->referenceCount = 0;
    [aList addObject:aSharedSynthInfo];
    [aSynthObj _setShared:aSharedSynthInfo];
    _MKAddSharedSynthClaim(aSharedSynthInfo);
    return YES;
}

@end

