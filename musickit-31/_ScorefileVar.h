#import <objc/Object.h>

/* _ScorefileVar functions */ 
#import "_parameter.h"
#import "_tokens.h"

extern _MKParameter *_MKSFVarGetParameter(id sfVar);
extern id _MKNewScorefileVar(_MKParameter *aPar,char * name,BOOL untyped,
			     BOOL isReadOnly);
extern int _MKSFVarInternalType(id sfVar);
extern _MKParameterUnion *_MKSFVarRaw(id sfVar);
extern int _MKSetDoubleSFVar(id sfVar,double floval);
extern int _MKSetIntSFVar(id sfVar,int  intval);
extern int _MKSetStringSFVar(id sfVar,char * strval);
extern int _MKSetEnvSFVar(id sfVar,id envelope);
extern int _MKSetWaveSFVar(id sfVar,id waveTable);
extern int _MKSetObjSFVar(id sfVar,id anObj);
extern id _MKSetScorefileVarPreDaemon();
extern id _MKSetScorefileVarPostDaemon();
extern id _MKSetReadOnlySFVar(id sfVar,BOOL yesOrNo);
void _MKSFSetPrintfunc();

@interface _ScorefileVar : Object
{
	_MKToken token;
	_MKParameter *myParameter;
	BOOL (*preDaemon)();
	void (*postDaemon)();
	BOOL readOnly;
	char * s;
}
- copy; 
- writeScorefileStream:(NXStream *)aStream; 
-(char *)name;
- free; 

@end



















