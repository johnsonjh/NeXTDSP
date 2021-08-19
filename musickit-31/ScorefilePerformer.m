#ifdef SHLIB
#include "shlib.h"
#endif

/*
  ScorefilePerformer.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  09/18/89/daj - Added casts to (id) in _getData/_setData: to accomodate new
                 void * type.
  10/26/89/daj - Added +fileExtensions method for binary scorefile
                 support.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  08/23/90/daj - Changed to zone API.
*/

#import "_musickit.h"
#import "_scorefile.h"
#import "_Instrument.h"
#import "_Part.h"

#import "_ScorefilePerformer.h"
@implementation ScorefilePerformer:FilePerformer  
/* ScorefilePerformers are used to access and perform scorefiles.
 * Instances of this class are used directly in an application;
 * you don't have to design your own subclass.
 *
 * A ScorefilePerformer creates
 * a separate NoteSender object for each part name in the file
 * (as given in the file's part statements).  The NoteSender objects
 * are maintained as an List in the inherited variable \fBnoteSenders\fR.
 * The NoteSenders are named with the names of the Parts in the file. 
 * Thus, you can find out the names of the Parts in the file by getting 
 * a List of the noteSenders (using -noteSenders) and using the function
 * MKGetObjectName(noteSender).
 *
 * Much of ScorefilePeformer's functionality is 
 * documented under FilePerformer, and Performer. 
 */
{
    NXStream *scorefilePrintStream; /* NXStream used for scorefile print 
				       statements output */
    id info;
    void *_reservedScorefilePerformer1;
    id _reservedScorefilePerformer2;
}
#define _p _reservedScorefilePerformer1    /* Information retrieved from the scorefile. */
#define _parts _reservedScorefilePerformer2 /* Set of part stubs. Used internally. */

/* METHOD TYPES
 * Initializing a ScorefilePerformer
 * Accessing files
 * Accessing Notes
 */

#define VERSION2 2

+initialize
{
    if (self != [ScorefilePerformer class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write:, which archives NoteSenders.
     Then archives info and part infos gleaned from the Scorefile. */
{
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"@@",&info,&_parts);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"ScorefilePerformer") 
	== VERSION2) 
      NXReadTypes(aTypedStream,"@@",&info,&_parts);
    return self;
}

-init
  /* TYPE: Initializing
   * Sent by the superclass upoon creation.
   * You never invoke this method directly.
   */
{
    [super init];
    _parts = [List new];
    return self;
}

+(char *)fileExtension
  /* Returns "score", the default file extension for score files.
     This method is used by the FilePerformer class. The string is not
     copied. */
{
    return _MK_SCOREFILEEXT;
}

+(char **)fileExtensions
  /* Returns a null-terminated array of the default file extensions 
     recognized by ScorefilePerformer instances. This array consists of
     "score" and "scoreO".
     This method is used by the FilePerformer class. The string is not
     copied. */
{
    static char *extensions[3] = {NULL};
    extensions[0] = _MK_BINARYSCOREFILEEXT;
    extensions[1] = _MK_SCOREFILEEXT;
    return extensions;
}

-info
{
    return info;
}

#define SCOREPTR ((_MKScoreInStruct *)_p)

-initializeFile
  /* TYPE: Accessing f
   * Initializes the information obtained from the scorefile header.
   * Notice that the parts representing the scorefile do not appear
   * in the ScorefilePerformer until activation.
   * Returns the receiver, or \fBnil\fR if the file can't be read, there
   * are too many parse errors, or there is no body. 
   * You never send the \fBinitializeFile\fR message 
   * directly to a ScorefilePerformer; it's invoked by the
   * \fBselfActivate\fR method.
   */
{
    _p = (void *)_MKNewScoreInStruct(stream,self,scorefilePrintStream,YES,
				     filename);
    if (!_p)
      return nil;
    _MKParseScoreHeader((_MKScoreInStruct *)_p);
    if (SCOREPTR->timeTag > (MK_ENDOFTIME-1)) {
	[self deactivateSelf];
	return nil;
    }
    return self;
}

-finishFile
  /* TYPE: Accessing f
   * Performs file finalization and returns the receiver.
   * You never send the \fBfinishFile\fR message 
   * directly to a ScorefilePerformer; it's invoked by the
   * \fBdeactivate\fR method.
   */
{
    _p = (void *)_MKFinishScoreIn(SCOREPTR);
    return self;
}

-setScorefilePrintStream:(NXStream *)aStream
  /* Sets the stream to be used for Scorefile 'print' statement output. */
{
    scorefilePrintStream = aStream;
    return self;
}

-(NXStream *)scorefilePrintStream
  /* Returns the stream used for Scorefile 'print' statement output. */
{
    return scorefilePrintStream;
}

-nextNote
  /* TYPE: Accessing N
   * Grabs the next entry in the body of the scorefile.
   * If the entry is a note statement, this fashions a Note
   * object and returns it.  If it's a time statement, updates
   * the \fBfileTime\fR variable
   * and returns \fBnil\fR.
   *
   * You never send \fBnextNote\fR directly to a 
   * ScorefilePerformer; it's invoked by the \fBperform\fR method.
   * You may override this method, e.g. to modify the note before it is 
   * performed but you must send [super nextNote].
   */
{
    id aNote = _MKParseScoreNote(SCOREPTR);
    fileTime = SCOREPTR->timeTag;
    return aNote;

}

-infoForNoteSender:(id)aNoteSender
  /* If aNoteSender is a member of the receiver, returns the info Note
     corresponding to the partName represented by that NoteSender. Otherwise, 
     returns nil. */
{
    
    return ([aNoteSender owner] == self) ? 
      [((id)[aNoteSender _getData]) info] : nil;
}

-midiNoteSender:(int)aChan
  /* Returns the first NoteSender whose corresponding Part has 
     a MK_midiChan info parameter equal to
     aChan, if any. aChan equal to 0 corresponds to the Part representing
     MIDI system and channel mode messages. */
{
    id *el,aInfo;
    unsigned n;
    if (aChan == MAXINT)
      return nil;
    for (el = NX_ADDRESS(noteSenders), n =  [noteSenders count]; n--; el++)
      if (aInfo = [((id)[*el _getData]) info])
	if ([aInfo parAsInt:MK_midiChan] == aChan) 
	  return *el;
    return nil;
}

-performNote:aNote
  /* TYPE: Accessing N
   * Sends \fIaNote\fR to the appropriate NoteSender
   * You never send \fBperformNote:\fR directly to a ScorefilePerformer;
   * it's invoked by the \fBperform\fR method.
   */
{
    [[SCOREPTR->part _noteSender] sendNote:aNote];    
    return self;
}

-free
  /* Frees receiver and its NoteSenders.  Also frees the info.
     If the receiver is active, does nothing and returns self. Otherwise,
     returns nil. */
{
    if (status != MK_inactive)
      return self;
    [_parts freeObjects];
    [_parts free];
    [info free];
    return [super free];
}

-copyFromZone:(NXZone *)zone
  /* Copies self and info. */
{
    ScorefilePerformer *newObj = [super copyFromZone:zone];
    newObj->info = [info copy];
    return newObj;
}


@end


@implementation ScorefilePerformer(Private)

-_newFilePartWithName:(char *)name
  /* You never send this message. It is used only by the Scorefile parser
     to add a NoteSender to the receiver when a part is declared in the
     scorefile. 
     It is a method rather than a C function to hide from the parser
     the differences between Score and ScorefilePerformer.
     */
{
    id aNoteSender = [NoteSender new];
    id aPart = [Part new];
    [self addNoteSender:aNoteSender];
    MKNameObject(name,aNoteSender);
    [aPart _setNoteSender:aNoteSender];/* Forward ptr for performNote */
    [aNoteSender _setData:aPart];  /* Back ptr */
    [_parts addObject:aPart];
    return aPart;
}

-_elements
  /* Same as noteSenders. (Needed by Scorefile parser.) 
   It is a method rather than a C function to hide from the parser
   the differences between Score and ScorefilePerformer. */
{
//    return [self noteSenders];
    return _MKCopyList(_parts); 
}

-_setInfo:aInfo
  /* Needed by scorefile parser  */
{
    if (!info)
      info = [aInfo copy];
    else [info copyParsFrom:aInfo];
    return self;
}

@end

