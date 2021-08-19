#import "SynthData.h"
@interface SynthData(Private)

-(MKOrchMemStruct *) _resources;
-_deallocAndAddToList;
+(id)_newInOrch:(id)anOrch index:(unsigned short)whichDSP
 length:(int)size segment:(MKOrchMemSegment)whichSegment 
 baseAddr:(DSPAddress)addr;
-(MKOrchMemStruct *)_setSynthPatch:aSynthPatch;
-(void)_setShared:aSharedKey;
-(void)_addSharedSynthClaim;

@end



