#import "Note.h"

#import "_parameter.h"

/* Note functions */
extern void _MKSetNoteType(id aNote,MKNoteType aType);
extern void _MKSetNoteTag(id aNote,int aTag);
extern void _MKSetNoteDur(id aNote,double dur);
extern int _MKGetPar(char *aName,id *aPar);
extern id _MKWriteNote2(id self,id aPart,_MKScoreOutStruct *p);
extern int _MKNoteCompare(const void *el1,const void *el2);
extern void _MKMakePlaceHolder(id aNote);
extern BOOL _MKNoteIsPlaceHolder(Note *aNote);
extern void _MKWriteParameters(id aNote,NXStream *aStream,
			       _MKScoreOutStruct *p);
extern void _MKNoteAddParameter(id aNote,_MKParameter *aPar);
extern void _MKNoteSetMatchTimeTag(id aNote,BOOL yesOrNo);
extern void _MKNoteShiftTimeTag(id aNote, double timeShift);

@interface Note(Private)

-_unionWith:aNote;
-_splitNoteDurNoCopy;
-(void)_setPerformer:anObj;
- _setPartLink:aPart order:(int)theOrder;
-_noteOffForNoteDur;

@end

