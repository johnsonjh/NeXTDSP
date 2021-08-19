#ifdef SHLIB
#include "shlib.h"
#endif

/*
  ScorefileWriter.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  09/18/89/daj - Added casts to (id) in _getData/_setData: to accomodate new
                 void * type.
  10/25/89/daj - Added instance variable isOptimized.		 
  01/31/90/daj - Changed setOptimzedFile:, setFile:, etc. to not set 
                 isOptimized if first note has been seen. Changed isOptimized
		 to share _reservedScorefileWriter3 (to remain backward
		 compatible with 1.0 header file). Added check for 
		 inPerformance in -free.
  03/23/90/daj - Added archiving. Flushed hack described above. Added 
                 instance var.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
                 Fixed bug in read: method.
  08/23/90/daj - Changed to zone API.
*/

#import "_musickit.h"
#import "_Instrument.h"
#import "_scorefile.h"

#import "ScorefileWriter.h"

@implementation ScorefileWriter:FileWriter
/*  A score writer writes notes to a scorefile. Like any other Instrument,
    it maintains a list of NoteReceivers. Each NoteReceiver corresponds to
    a part to appear in the scorefile. Methods are provided for
    manipulating the set of NoteReceivers.

    It is illegal to remove parts during performance. 

    If a performance session is repeated, the file must be specified again
    using setFileStream: (see FileWriter class).

    PartNames in the score correspond to the names of the NoteReceivers. 
    You add the NoteReceivers with addNoteReceiver:. You name the
    NoteReceivers with the MKNameObject() C function.

    It's illegal to change the name of a data object (such as an Envelope,
    WaveTable or NoteReceiver) during a performance
    involving a ScorefileWriter. (Because an object'll get written to the
    file with the wrong name.)
    */
{
    id info;
    int _reservedScorefileWriter1,_reservedScorefileWriter2;
    BOOL _reservedScorefileWriter3;
    void *_reservedScorefileWriter4;
}
#define SCOREPTR ((_MKScoreOutStruct *)_p)
#import "_Instrument.h"
#define _highTag _reservedScorefileWriter1 /* Tag range of notes written. */
#define _lowTag _reservedScorefileWriter2
#define _isOptimized _reservedScorefileWriter3
#define _p _reservedScorefileWriter4 /* Pointer to scoreFile writer struct . */

#define VERSION2 2

+initialize
{
    if (self != [ScorefileWriter class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

#define PARTINFO(_aNR) ((id)[_aNR _getData])

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write:, which archives NoteReceivers.
     Then archives info, isOptimized, and Part info Notes.  */
{
    id *el;
    id partInfo;
    int n;
    [super write:aTypedStream];
    n = [noteReceivers count]; 
    NXWriteTypes(aTypedStream,"@ci",&info,&_isOptimized,&n);
    for (el = NX_ADDRESS(noteReceivers);  n--; el++) {
	partInfo = PARTINFO(*el);
	NXWriteObject(aTypedStream,partInfo);
    }
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    id *el;
    int noteReceiverCount;
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"ScorefileWriter") 
	== VERSION2) {
	NXReadTypes(aTypedStream,"@ci",&info,&_isOptimized,&noteReceiverCount);
	/* Because we can't install the Part infos now in the NoteReceivers,
	   we have to use them temporarily in an available pointer, _p. 
	   See awake below. */
	_MK_MALLOC(el,id,noteReceiverCount);
	_p = el;
	while (noteReceiverCount--) 
	  *el++ = NXReadObject(aTypedStream);
    }
    return self;
}

- awake
  /* Installs Part infos */
{
    unsigned n;
    id *el1,*el2;
    [super awake];
    for (el1 = NX_ADDRESS(noteReceivers), 
	 el2 = (id *)_p, n = [noteReceivers count]; 
	 n--;)                 
      [*el1++ _setData:*el2++]; 
    NX_FREE(_p);
    _p = NULL;
    return self;
}

+(char *)fileExtension
  /* Returns "score", the default file extension for score files.
     This method is used by the FileWriter class. The string is not
     copied. */
{
    return _MK_SCOREFILEEXT;
}

-(char *)fileExtension
  /* Returns "score", the default file extension for score files if the
     file was set with setFile: or setStream:. Returns "scoreB", the
     default file extension for optimized format score files if the file
     was set with setOptimizedFile: or setOptimizedStream:. 
     The string is not copied. */
{
    return (_isOptimized) ? _MK_BINARYSCOREFILEEXT : _MK_SCOREFILEEXT;
}

-initializeFile
    /* Initializes file specified by the name of the FileWriter. You never
       send this message directly. */
{
    id *el;
    unsigned n;
    _highTag = -1;
    _lowTag = MAXINT;
    _p = (void *)_MKInitScoreOut(stream,self,info,timeShift,YES,_isOptimized);
    /* Write declarations in header. */
    for (el = NX_ADDRESS(noteReceivers), n = [noteReceivers count]; n--; el++)
      _MKWritePartDecl(*el,SCOREPTR,PARTINFO(*el));
    SCOREPTR->_tagRangePos = NXTell(SCOREPTR->_stream);
    NXPrintf(SCOREPTR->_stream,"                                        \n");
    /* We'll fill this in later. */
    return self;
}

-finishFile
    /* Does not close file. You never send this message directly. */
{
    long curPos;
    curPos = NXTell(SCOREPTR->_stream);
    NXSeek(SCOREPTR->_stream,SCOREPTR->_tagRangePos,NX_FROMSTART);
    if (_lowTag < _highTag)
      NXPrintf(SCOREPTR->_stream,
	       "noteTagRange = %d to %d;\n",_lowTag,_highTag);
    NXSeek(SCOREPTR->_stream,curPos,NX_FROMSTART);
    (void)_MKFinishScoreOut(SCOREPTR,YES);
    return self;
}

-setInfo:aNote
  /* Sets info, overwriting any previous info. aNote is copied. The info is 
     written out in the initializeFile method. The old info, if any, is freed. 
     */
{
    [info free];
    info = [aNote copy];
    return self;
}

-info
{
    return info;
}

-setInfo:aNote forNoteReceiver:aNR
  /* Sets Info for partName corresponding to specified NoteReceiver.
     If in performance or if aNR is not a NoteReceiver of the 
     receiver, generates an error and returns nil. 
     If the receiver is in performance, does nothing and returns nil. 
     aNote is copied. The old info, if any, is freed. */
{
    if (_noteSeen || (![self isNoteReceiverPresent:aNR]))
      return nil;
    [PARTINFO(aNR) free]; 
    [aNR _setData:(void *)[aNote copy]];
    return self;
}

- infoForNoteReceiver:aNoteReceiver
{
    return PARTINFO(aNoteReceiver);
} 

-copyFromZone:(NXZone *)zone 
    /* Copies object and set of parts. The copy has a copy of 
       the noteReceivers and info notes. */
{
    id *el1,*el2;
    unsigned n;
    ScorefileWriter *newObj =  [super copyFromZone:zone];
    newObj->_highTag = -1;
    newObj->_lowTag = MAXINT;
    newObj->_p = NULL;
    newObj->info = [info copy];
    for (el1 = NX_ADDRESS(noteReceivers), 
	 el2 = NX_ADDRESS(newObj->noteReceivers), n = [noteReceivers count]; 
	 n--;)                 
      [*el2++ _setData:[PARTINFO(*el1++) copy]]; /* Copy part info notes. */ 
    return newObj;
}

-free
  /* Frees receiver, NoteReceivers and info notes. */ 
{
    id *el;
    unsigned n;
    if ([self inPerformance])
      return self;
    [info free];
    for (el = NX_ADDRESS(noteReceivers), n = [noteReceivers count]; 
	 n--; el++) 
      [PARTINFO(*el) free]; /* Free part info notes. */
    return [super free];
}

-realizeNote:aNote fromNoteReceiver:aNoteReceiver
  /* Realizes note by writing it to the file, assigned to the part 
     corresponding to aNoteReceiver. */
{
    int noteTag = [aNote noteTag];
    if (noteTag != MAXINT) {
	_highTag = MAX(noteTag,_highTag);
	_lowTag = MIN(noteTag,_lowTag);
    }
    _MKWriteNote(aNote,aNoteReceiver,SCOREPTR);
    return self;
}

-setOptimizedStream:(NXStream *)aStream
  /* Same as setStream: but specifies that the data be written in optimized 
     scorefile format. */
{
    id rtn;
    rtn = [super setStream:aStream];
    _isOptimized = YES;
    return rtn;
}

-setOptimizedFile:(char *)aName
  /* Same as setFile: but specifies that the file be in optimized format. */
{
    id rtn;
    rtn = [super setFile:aName];
    _isOptimized = YES;
    return rtn;
}

-setFile:(char *)aName
  /* See superclass documentation */
{
    id rtn;
    rtn = [super setFile:aName];
    _isOptimized = NO;
    return rtn;
}

-setStream:(NXStream *)aStream
  /* See superclass documentation */
{
    id rtn;
    rtn = [super setStream:aStream];
    _isOptimized = NO;
    return rtn;
}

@end

