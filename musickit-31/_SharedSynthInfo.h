#import <objc/Object.h>

/* _SharedSynthKey functions */
extern BOOL _MKCollectSharedDataGarbage(id orch,NXHashTable *garbageTable);
extern NXHashTable *_MKGetSharedSynthGarbage(id self);
extern BOOL _MKInstallSharedObject(id _sharedSet,id aSynthObj,id aKeyObj,
				   MKOrchMemSegment whichSegment,int howLong);
extern id _MKFindSharedSynthObj(id sharedSet,NXHashTable *garbageTable,id aKeyObj,
				MKOrchMemSegment whichSegment,int howLong);
extern void _MKAddSharedSynthClaim(id aKey);
extern id _MKFreeSharedSet(id sharedSet,NXHashTable **garbageTable);
extern id _MKNewSharedSet(NXHashTable **garbageTable);
extern BOOL _MKReleaseSharedSynthClaim(id aKey,BOOL lazy);
extern int _MKGetSharedSynthReferenceCount(id sharedSynthKey);

@interface _SharedSynthInfo : Object
{
    id synthObject;           /* The value we're interested in finding. */
    id theList;               /* List of values that match the keyObj. */
    id theKeyObject;          /* Back pointer to key object. */
    MKOrchMemSegment segment; /* Which segment or MK_noSegment for wildcard. */
    int length;               /* Or 0 for wild card */
    int referenceCount;       
}

@end



















