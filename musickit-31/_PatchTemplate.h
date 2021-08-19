/*  Modification history:

    daj/04/23/90 - Created from _musickit.h 

*/
#import "PatchTemplate.h"

/* PatchTemplate functions */
extern BOOL _MKIsClassInTemplate(id templ,id factObj);
extern id _MKDeallocatedSynthPatches(id templ,int orchIndex);
extern void _MKEvalTemplateConnections(id templ,id synthPatchContents);
extern void _MKSetTemplateEMemUsage(id templ,MKOrchMemStruct *reso);
extern unsigned _MKGetTemplateEMemUsage(id templ);
extern id _MKAllocSynthPatch(id templ,id synthPatchFactory,id anOrch,int orchIndex);

