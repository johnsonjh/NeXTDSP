/* Modification history:

   4/26/90/daj - For a bit of efficiency, changed _MKTrace() to direct ref 
                 to _MKTraceFlag, since _MKTraceFlag is, indeed, a 
		 "private extern". Might want to do the same for
		 _MKGetOrchSimulator (using @defs)

*/

#import "_DSPMK.h"

#import "Orchestra.h"

#define _MK_ORCHTRACE(_orch,_debugFlag) \
  ((_MKTraceFlag & _debugFlag) || (_MKGetOrchSimulator(_orch)))

/* Orchestra functions */
extern id MKOrchestraClasses(void);
extern void _MKOrchResetPreviousLosingTemplate(id self);
extern id _MKFreeMem(id self,MKOrchAddrStruct *mem);
extern _MKAddTemplate(id aNewTemplate);
extern FILE *_MKGetOrchSimulator();
extern DSPFix48 *_MKCurSample(id orch);
extern void _MKOrchAddSynthIns(id anIns);
extern void _MKOrchRemoveSynthIns(id anIns);
extern BOOL _MKOrchLateDeltaTMode(id theOrch); /* See Orchestra.m ***SIGH*** */

@interface Orchestra(Private)

+(id *)_addTemplate:aNewTemplate ;

@end

