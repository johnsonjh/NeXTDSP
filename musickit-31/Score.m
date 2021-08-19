#ifdef SHLIB
#include "shlib.h"
#endif

/*
  Score.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  12/8/89/daj  - Fixed bug in midi-file reading -- first part was being
                 initialized to a bogus info object. 
  12/15/89/daj - Changed how Midi channel is encoded and written so that the 
                 information is not lost when reading/writing a format 1
		 file.
  12/20/89/daj - Added writeOptimizedScorefile: and 
                 writeOptimizedScorefileStream:
  01/08/90/daj - Added clipping of firstTimeTag in readScorefile().
  02/26-28/90/daj - Changes to accomodate new way of doing midiFiles. 
                 Added midifile sys excl and meta-event support.
  03/13/90/daj - Changes for new categories for private methods.
  03/19/90/daj - Changed to use MKGetNoteClass(), MKGetPartClass().
  03/21/90/daj - Added archiving. 
  03/27/90/daj - Added 10 new scorefile methods to make the scorefile and
                 midifile cases look the same. *SIGH*
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  04/23/90/daj - Changes to make it a shlib and to make header files more
                 modular.
  05/16/90/daj - Got rid of the "fudgeTime" kludge in the MIDI file reader,
                 now that the Part object's been fixed to insure correct
		 ordering. Added check for clas of Part class in setPart:.
  06/10/90/daj - Fixed bug in writing of scorefiles.
  07/24/90/daj - Removed unneeded copy of Note from readScorefile. Then
                 added it back because it actually sped things up.
  08/31/90/daj - Added import of stdlib.h and define of NOT_YET
  09/26/90/daj - Fixed minor bug in freeNotes.
*/

#define NOT_YET 1 /* Force override of alloca with __builtin_alloca */
#import <stdlib.h>
#import "_musickit.h"  
#import "_Part.h"
#import "_Note.h"
#import "_midi.h"
#import "_midifile.h"
#import "_tokens.h"
#import "_error.h"

#import "_Score.h"
@implementation Score:Object
/*  A score contains a collection of Parts and has methods for manipulating
    those Parts. Scores and Parts work closely together. 
    Scores can be performed. 
    The score can read or write itself from a scorefile or midifile.
    */
{
    id parts;
    NXStream *scorefilePrintStream; /* NXStream used for scorefile print 
				       statements output */
    id info;
    void *_reservedScore1;
}

/* METHOD TYPES
 * x
 */

#define READIT 0
#define WRITEIT 1

/* Creation and freeing ------------------------------------------------ */

#define VERSION2 2

+initialize
{
    if (self != [Score class])
      return self;
    [self setVersion:VERSION2];
    _MKCheckInit();
    return self;
}

+new
  /* Create a new instance and sends [self init]. */
{
    self = [self allocFromZone:NXDefaultMallocZone()];
    [self init];
    [self initialize]; /* Avoid breaking pre-2.0 apps. */
    return self;
}

-initialize 
  /* For backwards compatibility */
{ 
    return self;
} 

-init
  /* TYPE: Creating and freeing a Part
   * Initializes the receiver:
   *
   *  * Creates a new \fBnotes\fR collection.
   * 
   * Sent by the superclass upon creation;
   * you never invoke this method directly.
   * An overriding subclass method should send \fB[super initialize]\fR
   * before setting its own defaults. 
   */
{
    parts = [List new];
    return self;
}

-freeNotes
    /* Frees the notes contained in the parts. Does not freeParts
       nor does it free the receiver. Implemented as 
       [parts makeObjectsPerform:@selector(freeNotes)];
       Also frees the receivers info.
       */
{
    [parts makeObjectsPerform:@selector(freeNotes)];
    info = [info free];
    return self;
}

-freeParts
    /* frees contained parts and their notes. Does not free the
       receiver. Use -free to free, all at once,
       parts, receiver and notes. Does not free Score's info. */
{
    [parts makeObjectsPerform:@selector(_unsetScore)];
    [parts freeObjects];          /* Frees Parts. */
    [parts empty];
    return self;
}

-freePartsOnly
    /* Frees contained Parts, but not their notes. It is illegal
       to free a part which is performing or which has a PartSegment which
       is performing. Implemented as 
       [parts makeObjectsPerform:@selector(freeSelfOnly)];
       Returns self. */
{
    [parts makeObjectsPerform:@selector(_unsetScore)];
    [parts makeObjectsPerform:@selector(freeSelfOnly)];
    [parts empty];
    return self;
}

-freeSelfOnly
    /* Frees receiver. Does not free contained Parts nor their notes.  
       Does not free info.
    */
{
    [parts makeObjectsPerform:@selector(_unsetScore)];
    [parts free];
    return [super free];
}

-free
  /* Frees receiver, parts and Notes, including info. */
{
    [self freeParts];
    [info free];
    return [super free];
}

-empty
  /* TYPE: Modifying; Removes the receiver's Parts.
   * Removes the receiver's Parts but doesn't free them.
   * Returns the receiver.
   */
{
    [parts makeObjectsPerform:@selector(_unsetScore)];
    [parts empty];
    return self;
}

/* Reading Scorefiles ------------------------------------------------ */

static id readScorefile(); /* forward ref */

#if 0
-readScorefileIncrementalStream:(NXStream *)aStream
  /* Read and execute the next statement from the specified scorefile stream.
     You may repeatedly send this message until the stream reaches EOF or 
     an END statement is parsed.
     */
{
    /* Need to keep a table mapping streams to _MKScorefileInStructs or could
       have this method return a 'handle' */
    /* FIXME */
    [self notImplemented:_cmd];
}
#endif

-readScorefile:(char *)fileName 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
 timeShift:(double)timeShift
  /* Read from scoreFile to receiver, creating new Parts as needed
       and including only those notes between times firstTimeTag to
       time lastTimeTag, inclusive. Note that the TimeTags of the
       notes are not altered from those in the file. I.e.
       the first note's TimeTag will be greater than or equal to
       firstTimeTag.
       Merges contents of file with current Parts when the Part
       name found in the file is the same as one of those in the
       receiver. 
       Returns self or nil if file not found or the parse was aborted
       due to errors. */
{
    NXStream *stream;
    int fd;
    id rtnVal;
    stream = _MKOpenFileStream(fileName,&fd,NX_READONLY,
			       _MK_BINARYSCOREFILEEXT,NO);
    if (!stream)
      stream = _MKOpenFileStream(fileName,&fd,NX_READONLY,_MK_SCOREFILEEXT,
				 YES);
    if (!stream)
    	return nil;
    rtnVal = readScorefile(self,stream,firstTimeTag,lastTimeTag,timeShift,
			   fileName);
    NXClose(stream);
    close(fd);
    return rtnVal;
 }
 
-readScorefileStream:(NXStream *)stream 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
 timeShift:(double)timeShift
    /* Read from scoreFile to receiver, creating new Parts as needed
       and including only those notes between times firstTimeTag to
       time lastTimeTag, inclusive. Note that the TimeTags of the
       notes are not altered from those in the file. I.e.
       the first note's TimeTag will be greater than or equal to
       firstTimeTag.
       Merges contents of file with current Parts when the Part
       name found in the file is the same as one of those in the
       receiver. 
       Returns self or nil if the parse was aborted due to errors. 
       It is the application's responsibility to close the stream after calling
       this method.
       */
{
    return readScorefile(self,stream,firstTimeTag,lastTimeTag,timeShift,NULL);
}

/* Scorefile reading "convenience" methods  --------------------------- */

-readScorefile:(char *)fileName     
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
{
    return [self readScorefile:fileName firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag timeShift:0.0];
}

-readScorefileStream:(NXStream *)stream
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
{
    return [self readScorefileStream:stream firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag timeShift:0.0];
}

-readScorefile:(char *)fileName     
{
    return [self readScorefile:fileName firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}

-readScorefileStream:(NXStream *)stream     
{
    return [self readScorefileStream:stream firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}

/* Writing Scorefiles --------------------------------------------------- */

-_noteTagRangeLowP:(int *)lowTag highP:(int *)highTag
    /* Returns by reference the lowest and highest noteTags in receiver. */
{
    int noteTag,ht,lt;
    id *el,notes,*notePtr;
    unsigned n,m;
    ht = 0;
    lt = MAXINT;
    for (el = NX_ADDRESS(parts), n = [parts count]; n--; el++) {
	notes = [*el notesNoCopy];
	for (notePtr = NX_ADDRESS(notes), m = [notes count]; m--; notePtr++) {
	    noteTag = [*notePtr noteTag];
	    if (noteTag != MAXINT) {
		ht = MAX(ht,noteTag);
		lt = MIN(lt,noteTag);
	    }
	}
    }
    *highTag = ht;
    *lowTag = lt;
    return self;
}

static void writeNotes();

-_writeScorefileStream:(NXStream *)aStream firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag timeShift:(double)timeShift
 binary:(BOOL)isBinary
  /* Same as writeScorefileStream: but only writes notes within specified
     time bounds. */
{
    _MKScoreOutStruct * p;
    int lowTag, highTag;
    if (!aStream) 
      return nil;
    p = _MKInitScoreOut(aStream,self,info,timeShift,NO,isBinary);
    [self _noteTagRangeLowP:&lowTag highP:&highTag];
    if (lowTag <= highTag) {
	if (isBinary) {
	    _MKWriteShort(aStream,_MK_noteTagRange);
	    _MKWriteInt(aStream,lowTag);
	    _MKWriteInt(aStream,highTag);
	}
	else NXPrintf(aStream,"%s = %d %s %d;\n",
		      _MKTokNameNoCheck(_MK_noteTagRange),lowTag,
		      _MKTokNameNoCheck(_MK_to),highTag);
    }
    writeNotes(aStream,self,p,firstTimeTag,lastTimeTag,timeShift);
    _MKFinishScoreOut(p,YES);            /* Doesn't close aStream. */
    return self;
}

-_writeScorefile:(char *)aFileName firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag timeShift:(double)timeShift 
 binary:(BOOL)isBinary
{
    NXStream *stream;
    int fd;
    stream = _MKOpenFileStream(aFileName,&fd,NX_WRITEONLY,
			       (isBinary) ? _MK_BINARYSCOREFILEEXT :
			       _MK_SCOREFILEEXT,YES);
    if (!stream)
       return nil;
    [self _writeScorefileStream:stream firstTimeTag:firstTimeTag 
   lastTimeTag:lastTimeTag timeShift:timeShift binary:isBinary];
    NXFlush(stream);
    NXClose(stream);
    if (close(fd) == -1)
      _MKErrorf(MK_cantCloseFileErr,aFileName);
    return self;
}

-writeScorefile:(char *)aFileName 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
 timeShift:(double)timeShift
  /* Write scorefile to file with specified name within specified
     bounds. */
{
    return [self _writeScorefile:aFileName 
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:timeShift
	  binary:NO];
}

-writeScorefileStream:(NXStream *)aStream 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
 timeShift:(double)timeShift
  /* Same as writeScorefileStream: but only writes notes within specified
     time bounds. */
{
    return [self _writeScorefileStream:aStream
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:timeShift binary:NO];
}

-writeOptimizedScorefile:(char *)aFileName 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
 timeShift:(double)timeShift
  /* Write scorefile to file with specified name within specified
     bounds. */
{
    return [self _writeScorefile:aFileName 
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:timeShift
	  binary:YES];
}

-writeOptimizedScorefileStream:(NXStream *)aStream 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
 timeShift:(double)timeShift
  /* Same as writeScorefileStream: but only writes notes within specified
     time bounds. */
{
    return [self _writeScorefileStream:aStream
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:timeShift binary:YES];
}

/* Scorefile writing "convenience methods" ------------------------ */

-writeScorefile:(char *)aFileName 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
{
    return [self _writeScorefile:aFileName 
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:0.0
	  binary:NO];
}

-writeScorefileStream:(NXStream *)aStream 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
{
    return [self _writeScorefileStream:aStream
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:0.0 binary:NO];
}

-writeScorefile:(char *)aFileName 
{
    return [self writeScorefile:aFileName firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}

-writeScorefileStream:(NXStream *)aStream
{
    return [self writeScorefileStream:aStream firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}

-writeOptimizedScorefile:(char *)aFileName 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
{
    return [self _writeScorefile:aFileName 
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:0.0
	  binary:YES];
}

-writeOptimizedScorefileStream:(NXStream *)aStream 
 firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag 
{
    return [self _writeScorefileStream:aStream
	  firstTimeTag:firstTimeTag
	  lastTimeTag:lastTimeTag
	  timeShift:0.0 binary:YES];
}

-writeOptimizedScorefile:(char *)aFileName 
{
    return [self writeOptimizedScorefile:aFileName firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}

-writeOptimizedScorefileStream:(NXStream *)aStream
{
    return [self writeOptimizedScorefileStream:aStream firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}


/* Writing MIDI files ------------------------------------------------ */

static int timeInQuanta(void *p,double timeTag)
{
    return _MKMIDI_DEFAULTQUANTASIZE * timeTag + .5; /* .5 for rounding */
}

static void putMidi(struct __MKMidiOutStruct *ptr)
{
    _MKMIDIFileWriteEvent(ptr->_midiFileStruct,
			  timeInQuanta(ptr->_midiFileStruct,ptr->_timeTag),
			  ptr->_outBytes,
			  &(ptr->_bytes[0]));
}

static void putSysExcl(struct __MKMidiOutStruct *ptr,char *sysExclStr)
{
    unsigned char *buffer = alloca(strlen(sysExclStr)); /* More than enough */
    unsigned char *bufptr = buffer;
    int len;
    unsigned char c;
    c = _MKGetSysExByte(&sysExclStr);
    if (c == MIDI_SYSEXCL) 
      c = _MKGetSysExByte(&sysExclStr);
    while (*sysExclStr && c != MIDI_EOX) 
      *bufptr++ = c = _MKGetSysExByte(&sysExclStr);
    if (c != MIDI_EOX) 
      *bufptr++ = MIDI_EOX;
    len = bufptr - buffer;
    _MKMIDIFileWriteSysExcl(ptr->_midiFileStruct,
			    timeInQuanta(ptr->_midiFileStruct,ptr->_timeTag),
			    buffer,
			    len);
}

static void sendBufferedData(struct __MKMidiOutStruct *ptr)
    /* Dummy function. (Since we don't need an extra level of buffering
       here */
{
}

-writeMidifileStream:(NXStream *)aStream firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag timeShift:(double)timeShift
    /* Write midi on aStream. */
{
    _MKMidiOutStruct *p;
    void *fileStructP;
#   if _MK_MAKECOMPILERHAPPY 
    double t = 0;
    id anInfo = nil;
#   else
    double t;
    id anInfo;
#   endif
    unsigned n,noteCount;
    char *title;
    int defaultChan,chan;
    unsigned parBits;
    int tempo;
    id aPart,notes,*curPart;
    register id *curNote;
    if (!aStream) 
      return nil;
    title = NULL;
    tempo = 60;
    if (info) {
	if ([info isParPresent:MK_title])
	  title = [info parAsStringNoCopy:MK_title];
	if ([info isParPresent:MK_tempo])
	  tempo = [info parAsInt:MK_tempo];
    }
    p = _MKInitMidiOut();
    if (!(fileStructP = _MKMIDIFileBeginWriting(aStream,1,title))) {
	_MKFinishMidiOut(p);
	return nil;
    }
    else p->_midiFileStruct = fileStructP; /* Needed so functions called from
					      _MKWriteMidiOut can find 
					      struct */
    n = [parts count];
    if (n == 0) {
	_MKMIDIFileEndWriting(fileStructP);
	_MKFinishMidiOut(p);
	return self;
    }
    p->_owner = self;
    p->_putSysMidi = putMidi;
    p->_putChanMidi = putMidi;
    p->_putSysExcl = putSysExcl;
    p->_sendBufferedData = sendBufferedData;
    _MKMIDIFileWriteTempo(fileStructP,0,tempo);
#   define STRPAR MKGetNoteParAsStringNoCopy
#   define INTPAR MKGetNoteParAsInt
    if (info) {
	if ([info isParPresent:MK_copyright])
	  _MKMIDIFileWriteText(fileStructP,0,_MKMIDI_copyright,
			       STRPAR(info,MK_copyright));
	if ([info isParPresent:MK_sequence])
	  _MKMIDIFileWriteSequenceNumber(fileStructP,
					 INTPAR(anInfo,MK_sequence));
	if ([info isParPresent:MK_smpteOffset]) {
	    char *str = STRPAR(info,MK_smpteOffset);
	    unsigned char hr,mn,sec,fr,ff;
	    hr = (str) ? (unsigned char)strtol(str,&str,0) : 0;
	    mn = (str) ? (unsigned char)strtol(str,&str,0) : 0;
	    sec = (str) ? (unsigned char)strtol(str,&str,0) : 0;
	    fr = (str) ? (unsigned char)strtol(str,&str,0) : 0;
	    ff = (str) ? (unsigned char)strtol(str,&str,0) : 0;
	    _MKMIDIFileWriteSMPTEoffset(fileStructP,hr,mn,sec,fr,ff);
	}
    }
    for (curPart = NX_ADDRESS(parts); n--; curPart++) {
	if ([*curPart noteCount] == 0)
	  continue;
	/* Don't start a new track for the first track -- we've already
	   started it above with the _MKMIDIFileBeginWriting(). */
	if (curPart != NX_ADDRESS(parts)) 
	    _MKMIDIFileBeginWritingTrack(fileStructP,
					 (char *)MKGetObjectName(*curPart));
	aPart = [*curPart copy]; /* Need to copy to split notes. */
	[aPart splitNotes]; 
	[aPart sort];
	notes = [aPart notesNoCopy];
	anInfo = [aPart info];
	defaultChan = 1;
	if (anInfo) 
	  if ([anInfo isParPresent:MK_midiChan])
	    defaultChan = [anInfo parAsInt:MK_midiChan];
#       define T timeInQuanta(fileStructP,(t+timeShift))
	for (curNote = NX_ADDRESS(notes), noteCount = [notes count]; 
	     noteCount--;
	     curNote++)
	  if ((t = [*curNote timeTag]) >= firstTimeTag)
	    if (t > lastTimeTag)
	      break;
	    else { 
		/* First handle normal midi */
		chan = INTPAR(*curNote,MK_midiChan);
		_MKWriteMidiOut(*curNote,t+timeShift,
				((chan == MAXINT) ? defaultChan : chan),
				p,nil);
		/* Now check for meta-events. */
		parBits = [*curNote parVector:4];
#               define INRANGE(_par) (_par >= 128 && _par <= 159)	
#               define PRESENT(_par) (parBits & (1<<(_par - 128)))
#               define WRITETEXT(_meta,_par) \
		_MKMIDIFileWriteText(fileStructP,T,_meta,STRPAR(*curNote,_par))
		NX_ASSERT((INRANGE(MK_tempo) && INRANGE(MK_lyric) &&
			   INRANGE(MK_cuePoint) && INRANGE(MK_marker) &&
			   INRANGE(MK_timeSignature) && 
			   INRANGE(MK_keySignature)),
			  "Illegal use of parVector.");
		if (parBits) { 
		    if (PRESENT(MK_text))
		      WRITETEXT(_MKMIDI_text,MK_text);
		    if (PRESENT(MK_lyric))
		      WRITETEXT(_MKMIDI_lyric,MK_lyric);
		    if (PRESENT(MK_cuePoint))
		      WRITETEXT(_MKMIDI_cuePoint,MK_cuePoint);
		    if (PRESENT(MK_marker))
		      WRITETEXT(_MKMIDI_marker,MK_marker);
		    if (PRESENT(MK_timeSignature)) {
			char *str = STRPAR(*curNote,MK_timeSignature);
			unsigned char nn,dd,cc,bb;
			unsigned int allData;
			nn = (str) ? (unsigned char)strtol(str,&str,0) : 0;
			dd = (str) ? (unsigned char)strtol(str,&str,0) : 0;
			cc = (str) ? (unsigned char)strtol(str,&str,0) : 0;
			bb = (str) ? (unsigned char)strtol(str,&str,0) : 0;
			allData = (nn << 24) | (dd << 16) | (cc << 8) | bb;
			_MKMIDIFileWriteSig(fileStructP,T,_MKMIDI_timeSig,
					    allData);
		    }
		    if (PRESENT(MK_keySignature)) {
			char *str = STRPAR(*curNote,MK_keySignature);
			unsigned char sf,mi;
			unsigned int allData;
			sf = (str) ? (unsigned char)strtol(str,&str,0) : 0;
			mi = (str) ? (unsigned char)strtol(str,&str,0) : 0;
			allData = (sf << 8) | mi;
			_MKMIDIFileWriteSig(fileStructP,T,_MKMIDI_keySig,
					    allData);
		    }
		    if (PRESENT(MK_tempo))
		      _MKMIDIFileWriteTempo(fileStructP,T,
					    INTPAR(*curNote,MK_tempo));
		}
	    }
	_MKMIDIFileEndWritingTrack(fileStructP,T);
	[aPart free];
    }
    _MKMIDIFileEndWriting(fileStructP);
    _MKFinishMidiOut(p);
    return self;
}
     
-writeMidifile:(char *)aFileName firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag timeShift:(double)timeShift
   	/* Write midi to file with specified name. */
{
    NXStream *stream;
    int fd;
    stream = _MKOpenFileStream(aFileName,&fd,NX_WRITEONLY,_MK_MIDIFILEEXT,YES);
    if (!stream)
       return nil;
    [self writeMidifileStream:stream firstTimeTag:firstTimeTag 
      lastTimeTag:lastTimeTag timeShift:timeShift];
    NXFlush(stream);
    NXClose(stream);
    if (close(fd) == -1)
      _MKErrorf(MK_cantCloseFileErr,aFileName);
    return self;
}

/* Midi file writing "convenience methods" --------------------------- */

-writeMidifileStream:(NXStream *)aStream 
 firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag
{
    return [self writeMidifileStream:aStream firstTimeTag:firstTimeTag 
	  lastTimeTag:lastTimeTag timeShift:0.0];
}

-writeMidifile:(char *)aFileName 
 firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag
{
    return [self writeMidifile:aFileName firstTimeTag:firstTimeTag 
	  lastTimeTag:lastTimeTag timeShift:0.0];
}

-writeMidifileStream:(NXStream *)aStream 
{
    return [self writeMidifileStream:aStream firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME];
}

-writeMidifile:(char *)aFileName
{
    return [self writeMidifile:aFileName firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME];
}
  

/* Reading MIDI files ---------------------------------------------- */

- readMidifile:(char *)aFileName firstTimeTag:(double) firstTimeTag
    lastTimeTag:(double) lastTimeTag timeShift:(double)timeShift
/*
  * Read from Midifile and add the notes to the parts as follows: MIDI Channel
  * Voice Messages are added to the parts "midiChan1","midiChan2",...
  * "midiChan16". MIDI Channel Mode Messages are added to
  * the part "chanMode". MIDI System Messages are added to the part "midiSys". 
  * Returns self. Includes only those notes between times firstTimeTag to
  * time lastTimeTag, inclusive. Note that the TimeTags of the notes are not
  * altered from those in the file. I.e. the first note's TimeTag will be
  * greater than or equal to firstTimeTag. 
  */
{
    id rtnVal;
    NXStream *stream;
    int fd;
    stream = _MKOpenFileStream(aFileName,&fd,NX_READONLY,_MK_MIDIFILEEXT,YES);
    if (!stream)
       return nil;
    rtnVal = [self readMidifileStream:stream firstTimeTag:firstTimeTag 
	    lastTimeTag:lastTimeTag timeShift:timeShift];
    NXClose(stream);
    close(fd);
    return rtnVal;
}

static void writeDataAsNumString(id aNote,int par,unsigned char *data,
				 int nBytes)
{
#   define ROOM 4 /* Up to 3 digits per number followed by space */
    int size = nBytes * ROOM;
    char *str = alloca(size);
    int i,j;
    for (i=0; i<nBytes; i++) 
      sprintf(&(str[i * ROOM]),"%-3d ",j = data[i+1]);
    str[size - 1] = '\0'; /* Write over last space. */
    MKSetNoteParToString(aNote,par,str);
}

- readMidifileStream:(NXStream *) aStream firstTimeTag:(double) firstTimeTag
    lastTimeTag:(double) lastTimeTag timeShift:(double)timeShift
 /*
  * Read from midiFile. Each note which represents a channel voice message
  * is added to the Part found using [self midiPart:channel] 
  * Channel Mode Messages and system messages are added to
  * the part found using [self findMidiSysPart]. 
  * If an appropriate Part does not exist, one is created using a name
  * of the form "midiSys", "midiChan1", "midiChan2", ..., "midiChan16".
  * For any Parts it creates, also creates a info Note and sets 
  * MK_midiChan parameter in that info Note (the exception is the first part
  * which is either the "midiSys" or the "tempo" part -- no midiChan parameter
  * is given.)
  * Returns self. Includes only those notes between times firstTimeTag to
  * time lastTimeTag, inclusive. Note that the TimeTags of the notes are not
  * altered from those in the file. I.e. the first note's TimeTag will be
  * greater than or equal to firstTimeTag. File is assumed to be open. It is 
  * the application's responsibility to close the file.
  */
{
    int fileFormatLevel;
    int trackCount;
    double timeFactor,t,prevT,lastTempoTime = -1;
    id              aPart;
    int             i;
    register id     aNote;
#   define MIDIPARTS (16 + 1)
    _MKMidiInStruct *midiInPtr;
    id *midiParts,*curPart;
    BOOL trackInfoMidiChanWritten = NO;
    void *fileStructP;
    int *quanta;
    BOOL *metaevent;
    int *nData;
    unsigned char **data;
    if (!(fileStructP = 
	_MKMIDIFileBeginReading(aStream,&quanta,&metaevent,&nData,&data)))
      return nil;
#   define DATA (*data)
    if (!_MKMIDIFileReadPreamble(fileStructP,&fileFormatLevel,&trackCount)) 
      return nil;
    if (fileFormatLevel == 0)
      trackCount = MIDIPARTS;
    else trackCount++; 	/* trackCount doesn't include the 'tempo' track so
			   we increment here */
    if (!(midiInPtr = _MKInitMidiIn())) 
      return nil;
    _MK_MALLOC(midiParts,id,trackCount);
    curPart = midiParts;
    for (i=0; i<trackCount; i++) { 
	aPart = [MKGetPartClass() new];
	aNote = [MKGetNoteClass() new];
	if ((fileFormatLevel == 0) && (i != 0))
	  [aNote setPar:MK_midiChan toInt:i];
	[aPart setInfo:aNote];
	[self addPart:aPart];
	*curPart++ = aPart;
    }
    lastTimeTag = MIN(lastTimeTag, MK_ENDOFTIME);
    timeFactor = 1.0/(double)_MKMIDI_DEFAULTQUANTASIZE;
    /* In format 0 files, *curPart will be the _MK_MIDISYS part. */
    curPart = midiParts;
#   define FIRSTTRACK *midiParts
#   define CURPART *curPart
    prevT = -1;
    if (!info) 
      info = [MKGetNoteClass() new];
#   define SHORTDATA ((int)(*((short *)&(DATA[1]))))
#   define LONGDATA ((int)(*((int *)&(DATA[1]))))
#   define STRINGDATA ((char *)&(DATA[1]))
#   define LEVEL0 (fileFormatLevel == 0)
#   define LEVEL1 (fileFormatLevel == 1)
#   define LEVEL2 (fileFormatLevel == 2)
    if (LEVEL2) /* Sequences numbered consecutively from 0 by default. */
      MKSetNoteParToInt([FIRSTTRACK info],MK_sequence,0);
    while (_MKMIDIFileReadEvent(fileStructP)) {
	if (*metaevent) {
	    /* First handle meta-events that are Part or Score info
	       parameters. We never want to skip these. */
	    switch (DATA[0]) { 
	      case _MKMIDI_sequenceNumber:
		MKSetNoteParToInt(LEVEL2 ? [CURPART info] : info,
				  MK_sequence,SHORTDATA);
		break;
	      case _MKMIDI_smpteOffset: 
		writeDataAsNumString(LEVEL2 ? [CURPART info] : info,
				     MK_smpteOffset,DATA,5);
		break;
	      case _MKMIDI_sequenceOrTrackName:
		if ((curPart == midiParts) /* First part */
		    && !LEVEL2)  /* No MK_title in level 2 files, since
				    the title is merely the name of the first
				    sequence. */
		  MKSetNoteParToString(info,MK_title,STRINGDATA);
		/* In level 1 files, we name the current part with the
		   title. Note that we do this even if the name is a 
		   sequence name rather than a track name. In level 0 
		   files, we do not name the part. */
		if (fileFormatLevel != 0)
		  MKNameObject(STRINGDATA,*curPart);
		break;
	      case _MKMIDI_copyright:
		MKSetNoteParToString(info,MK_copyright,STRINGDATA);
		break;
	      default:
		break;
	    }
	}
	t = *quanta * timeFactor;
	/* FIXME Should do something better here. (need to change
	   Part to allow ordering spec for simultaneous notes.) */
	if (t < firstTimeTag) 
	  continue;
	if (t > lastTimeTag)
	  if (LEVEL0)
	    break; /* We know we're done */
	  else continue;
	if (*metaevent) {
	    /* Now handle meta-events that can be in regular notes. These
	       are skipped when out of the time window, as are regular 
	       MIDI messages. */
	    aNote = [MKGetNoteClass() newSetTimeTag:t+timeShift];
	    switch (DATA[0]) { 
	      case _MKMIDI_trackChange: 
		/* Sent at the end of every track. May be missing from the
		   end of the file. */
		if (t > (prevT + _MK_TINYTIME)) {
		    /* We've got a significant trackChange time. */
		    MKSetNoteParToString(aNote,LEVEL1 ? MK_track : MK_sequence,
					 "end");
		    [CURPART addNote:aNote];    /* Put an end-of-track mark */
		}
		else [aNote free];
		curPart++;
		if (curPart >= midiParts + trackCount)
		  goto outOfLoop;
		trackInfoMidiChanWritten = NO;
		if (LEVEL1) /* Other files have no "tracks" */
		  MKSetNoteParToInt([CURPART info],MK_track,SHORTDATA);
		else if (LEVEL2) {
		    /* Assign ascending sequence number parameters */
#		    define OLDNUM \
		       MKGetNoteParAsInt([(*(curPart-1)) info],MK_sequence)
		    MKSetNoteParToInt([CURPART info],MK_sequence,OLDNUM + 1);
		}
		lastTempoTime = -1;
		prevT = -1;
		continue; /* Don't clobber prevT below */
	      case _MKMIDI_tempoChange: 
		/* For MK-compatibility, tempo is duplicated in info
		   Notes, but only if it's at time 0 in file.    */
		if (t == 0) 
		  if (lastTempoTime == 0) {
		      /* Supress duplicate tempi, which can arise because of 
			 the way we duplicate tempo in info */
		      [aNote free];
		      break;
		  }
		  else { /* First setting of tempo for current track. */
		      id theInfo = LEVEL2 ? [CURPART info] : info;
		      if (MKGetNoteParAsInt(theInfo,MK_tempo) == MAXINT) 
			MKSetNoteParToInt(theInfo,MK_tempo,LONGDATA);
		  }
		lastTempoTime = t;
		MKSetNoteParToInt(aNote,MK_tempo,LONGDATA);
		[(LEVEL2 ? FIRSTTRACK : CURPART) addNote:aNote];
		break;
	      case _MKMIDI_text:
	      case _MKMIDI_cuePoint:
	      case _MKMIDI_lyric: 
		MKSetNoteParToString(aNote,
				     ((DATA[0] == _MKMIDI_text) ? MK_text :
				      (DATA[0] == _MKMIDI_lyric) ? MK_lyric :
				      MK_cuePoint),
				     STRINGDATA);
		[CURPART addNote:aNote];
		break;
	      case _MKMIDI_marker:
		MKSetNoteParToString(aNote,MK_marker,STRINGDATA);
		[(LEVEL2 ? FIRSTTRACK : CURPART) addNote:aNote];
		break;
	      case _MKMIDI_timeSig:
		writeDataAsNumString(aNote,MK_timeSignature,DATA,4);
		[(LEVEL2 ? FIRSTTRACK : CURPART) addNote:aNote];
		break;
	      case _MKMIDI_keySig: {
		  char str[5];
		  /* Want sf signed, hence (char) cast  */
		  int x = (int)((char)DATA[2]); /* sf */
		  sprintf(&(str[0]),"%-2d ",x);
		  x = (int)DATA[3]; /* mi */
		  sprintf(&(str[3]),"%d",x);
		  str[4] = '\0';
		  MKSetNoteParToString(aNote,MK_keySignature,str);
		  [(LEVEL2 ? FIRSTTRACK : CURPART) addNote:aNote];
		  break;
	      }
	      default:
		[aNote free];
		break;
	    }
	} 
	else { /* Standard MIDI, not sys excl */
	    switch (*nData) {
	      case 3:
		midiInPtr->_dataByte2 = DATA[2];
		/* No break */
	      case 2:
		midiInPtr->_dataByte1 = DATA[1];
		/* No break */
	      case 1:
		/* Status passed directly below. */
		break;
	      default: { /* Sys exclusive */
		  unsigned j;
		  char *str = alloca(*nData * 3); /* 3 chars per byte */
		  char *ptr = str;
		  unsigned char *endP = *data + *nData;
		  sprintf(ptr,"%-2x",j = **data++);
		  ptr += 2;
		  while (*data < endP) {
		      sprintf(ptr,",%-2x",j = **data++);
		      ptr += 3;
		  }
		  *ptr = '\0';
		  aNote = [MKGetNoteClass() newSetTimeTag:t+timeShift];
		  MKSetNoteParToString(aNote,MK_sysExclusive,str); /* copy */
		  [CURPART addNote:aNote];
		  continue;
	      }
	    }
	    aNote = _MKMidiToMusicKit(midiInPtr,DATA[0]);
	    if (aNote) { /* _MKMidiToMusicKit can omit Notes sometimes. */
		[aNote setTimeTag:t+timeShift];
		/* Need to copy Note because it's "owned" by midiInPtr. */
		if (LEVEL0) 
		  [midiParts[midiInPtr->chan] addNoteCopy:aNote];
		else {
		    if (!trackInfoMidiChanWritten && 
			midiInPtr->chan != _MK_MIDISYS) {
			trackInfoMidiChanWritten = YES;
			MKSetNoteParToInt([CURPART info],MK_midiChan,
					  midiInPtr->chan);
			/* Set Part's chan to chan of first note in track. */
		    }
		    aNote = [CURPART addNoteCopy:aNote];
		    /* aNote is new one */
		    if (midiInPtr->chan != _MK_MIDISYS)
		      MKSetNoteParToInt(aNote,MK_midiChan,midiInPtr->chan);
		}
	    }
	} /* End of standard MIDI block */
	prevT = t;	 
    } /* End of while loop */
  outOfLoop:
    NX_FREE(midiParts);
    _MKFinishMidiIn(midiInPtr);
    return self;
}

/* Midifile reading "convenience methods"------------------------ */

-readMidifile:(char *)fileName
 firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag
{
    return [self readMidifile:fileName firstTimeTag:firstTimeTag 
	  lastTimeTag:lastTimeTag timeShift:0.0];
}

-readMidifileStream:(NXStream *)aStream
 firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag
{
    return [self readMidifileStream:aStream firstTimeTag:firstTimeTag 
	  lastTimeTag:lastTimeTag timeShift:0.0];
}

-readMidifile:(char *)fileName
  /* Like readMidifile:firstTimeTag:lastTimeTag:, 
     but always reads the whole file. */
{
    return [self readMidifile:fileName firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}

-readMidifileStream:(NXStream *)aStream
    /* Like readMidifileStream:firstTimeTag:lastTimeTag:, 
       but always reads the whole file. */
{
    return [self readMidifileStream:aStream firstTimeTag:0.0 
	  lastTimeTag:MK_ENDOFTIME timeShift:0.0];
}

/* Number of notes and parts ------------------------------------------ */

-(unsigned)partCount
{
    return [parts count];
}

-(unsigned)noteCount
    /* Returns the total number of notes in all the contained Parts. */
{
    id *el;
    unsigned n;
    unsigned numNotes = 0;
    for (el = NX_ADDRESS(parts), n = [parts count]; n--; el++) 
      numNotes += [*el noteCount];
    return numNotes;
}

/* Modifying the set of Parts. ------------------------------- */

-addPart:(id)aPart
    /* If aPart is already a member of the Score, returns nil. Otherwise,
       adds aPart to the receiver and returns aPart,
       first removing aPart from any other score of which it is a member. */
{
    if ((!aPart) || ([aPart score] == self) || ![aPart isKindOf:[Part class]])
      return nil;
    [aPart _setScore:self];
    return [parts addObjectIfAbsent:aPart];
}

-removePart:(id)aPart
    /* Removes aPart from self and returns aPart. 
       If aPart is not a member of this score, returns nil. */
{
    [aPart _unsetScore];
    return [parts removeObject:aPart];
}

-shiftTime:(double)shift
  /* TYPE: Editing
   * Shift is added to the timeTags of all notes in the Part. 
   */
{
    id *el;
    unsigned n;
    for (el = NX_ADDRESS(parts), n = [parts count]; n--; el++)
      [*el shiftTime:shift];
    return self;
}

/* Finding a Part ----------------------------------------------- */

-(BOOL)isPartPresent:aPart
    /* Returns whether Part is a member of the receiver. */
{
    return ([parts indexOf:aPart] == -1) ? NO : YES;
}

-midiPart:(int)aChan
  /* Returns the first Part with a MK_midiChan info parameter equal to
     aChan, if any. aChan equal to 0 corresponds to the Part representing
     MIDI system and channel mode messages. */
{
    id *el, aInfo;
    unsigned n;
    if (aChan == MAXINT)
      return nil;
    for (el = NX_ADDRESS(parts), n = [parts count]; n--; el++)
      if (aInfo = [*el info])
	if ([aInfo parAsInt:MK_midiChan] == aChan) 
	  return *el;
    return nil;
}


/* Manipulating notes. ------------------------------- */

static void addContentsOfList(id toList, id fromList)
{
    id *el;
    unsigned n;
    IMP addMethod;
    if (!(fromList && toList))
      return;
    addMethod = [toList methodFor:@selector(addObject:)];
    for (el = NX_ADDRESS(fromList), n = [fromList count]; n--; el++) 
      (*addMethod)(toList,@selector(addObject:),*el);
}

#ifndef ABS
#define ABS(_x) ((_x < 0) ? -_x : _x)
#endif

static int noteIDCompare(const void *el1,const void *el2)
{
    /* Like _MKNoteCompare, but uses id comparison for collissions (i.e.
       Notes with the same timeTag.) We can't use Note's _MKNoteCompare 
       because the Notes are not all in the same Part so the ordering tags 
       are bogus. */ 
    id id1 = *((id *)el1);
    id id2 = *((id *)el2);
    double t1,t2;
    if (id1 == id2) 
      return 0;
    if (!id1)      /* Push nils to the end. */
      return 1;
    if (!id2)    
      return -1;
    t1 = [id1 timeTag];
    t2 = [id2 timeTag];
    if (t2 == t1)   /* Same time */
      return (id1 < id2) ? -1 : 1; /* Use id to disambiguate */
    return (t1 < t2) ? -1 : 1;
}

static void writeNotes(aStream,aScore,p,firstTimeTag,lastTimeTag,timeShift)
    NXStream * aStream;
    Score *aScore;
    _MKScoreOutStruct * p;
    double firstTimeTag,lastTimeTag,timeShift;
{
    /* Write score body on aStream. Assumes p is a valid _MKScoreOutStruct. */
    id *el;
    unsigned n;
    BOOL timeBounds = ((firstTimeTag != 0) || (lastTimeTag != MK_ENDOFTIME));
    id allNotes = [List newCount:[aScore noteCount]];
    id aList,aPart;
    for (el = NX_ADDRESS(aScore->parts), n = [aScore->parts count]; n--; el++){
	if (timeBounds) {
	    aList = [*el firstTimeTag:firstTimeTag lastTimeTag:
		     lastTimeTag];
	    addContentsOfList(allNotes,aList);
	    [aList free];
	}
	else addContentsOfList(allNotes,[*el notesNoCopy]);
	_MKWritePartDecl(*el,p,[*el info]);
    }
    n = [allNotes count];
    qsort((void *)NX_ADDRESS(allNotes),(size_t)n,(size_t)sizeof(id),
	  noteIDCompare);
    p->_timeShift = timeShift;
    for (el = NX_ADDRESS(allNotes); n--; el++) 
      _MKWriteNote(*el,aPart = [*el part],p);
    [allNotes free];
}

static id
readScorefile(self,stream,firstTimeTag,lastTimeTag,timeShift,fileName)
    Score *self;
    NXStream *stream;
    double firstTimeTag,lastTimeTag,timeShift;
    char * fileName;
 {
     /* Read from scoreFile to receiver, creating new Parts as needed
       and including only those notes between times firstTimeTag to
       time lastTimeTag, inclusive. Note that the TimeTags of the
       notes are not altered from those in the file. I.e.
       the first note's TimeTag will be greater than or equal to
       firstTimeTag.
       Merges contents of file with current Parts when the Part
       name found in the file is the same as one of those in the
       receiver. 
       Returns self or nil if error abort.  */
    register _MKScoreInStruct *p;
    register id aNote;
    IMP noteCopy,partAddNote;
    id rtnVal;
    partAddNote = [MKGetPartClass() instanceMethodFor:@selector(addNote:)];
    noteCopy = [MKGetNoteClass() instanceMethodFor:@selector(copy)];
    p = _MKNewScoreInStruct(stream,self,self->scorefilePrintStream,NO,
			    fileName);
    if (!p)
      return nil;
    _MKParseScoreHeader(p);
    lastTimeTag = MIN(lastTimeTag, MK_ENDOFTIME);
    firstTimeTag = MIN(firstTimeTag, MK_ENDOFTIME);
    do 
      aNote = _MKParseScoreNote(p);
    while (p->timeTag < firstTimeTag);
    /* Believe it or not, is actually better to copy the note here!
       I'm not sure why.  Maybe the hashtable has some hysteresis and
       it gets reallocated each time. */
    while (p->timeTag <= lastTimeTag) {
	if (aNote) {
	    aNote = (id)(*noteCopy)(aNote,@selector(copy));
	    _MKNoteShiftTimeTag(aNote,timeShift);
	    (*partAddNote)(p->part,@selector(addNote:),aNote);
	}
	aNote = _MKParseScoreNote(p);
	if ((!aNote) && (p->timeTag > (MK_ENDOFTIME-1)))
	  break;
    }
    rtnVal = (p->_errCount == MAXINT) ? nil : self;
    _MKFinishScoreIn(p);
    return rtnVal;
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

-_setInfo:aInfo
  /* Needed by scorefile parser  */
{
    if (!info)
      info = [aInfo copy];
    else [info copyParsFrom:aInfo];
    return self;
}

-setInfo:aNote
  /* Sets info, overwriting any previous info. aNote is copied. The old info,
     if any, is freed. */
{
    [info free];
    info = [aNote copy];
    return self;
}

-info
{
    return info;
}


-parts
  /* Returns a copy of the List of Parts in the receiver. The Parts themselves
     are not copied. It is the sender's repsonsibility to free the List. */
{
    return _MKCopyList(parts);
}

-copyFromZone:(NXZone *)zone
  /* Copies receiver, including its Parts, Notes and info. */ 
{
    id *el;
    unsigned n;
    Score *newScore = [Score allocFromZone:zone];
    [newScore init];
    for (el = NX_ADDRESS(parts), n = [parts count]; n--; el++)
      [newScore addPart:[*el copyFromZone:zone]];
    newScore->info = [info copyFromZone:zone];
    return newScore;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives Parts, Notes and info. */
{
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"@@",&parts,&info);
    return self;
}

static BOOL isUnarchiving = NO;

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    isUnarchiving = YES; /* Inhibit Parts' mapping of noteTags. */
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"Score") == VERSION2) 
      NXReadTypes(aTypedStream,"@@",&parts,&info);
    return self;
}

- awake
  /* Maps noteTags as represented in the archive file onto a set that is
     unused in the current application. This insures that the integrity
     of the noteTag is maintained. */
{
    id tagTable;
    [super awake];
    tagTable = [HashTable newKeyDesc:"i" valueDesc:"i"];
    [parts makeObjectsPerform:@selector(_mapTags:) with:tagTable];
    [tagTable free];
    isUnarchiving = NO;
    return self;
}

@end

@implementation Score(Private)

+(BOOL)_isUnarchiving
{
    return isUnarchiving;
}

-_newFilePartWithName:(char *)name
 /* You never send this message. It is used only by the Scorefile parser
     to add a Part to the receiver when a part is declared in the
     scorefile. 
     It is a method rather than a C function to hide from the parser
     the differences between Score and ScorefilePerformer.
     */
{
    id aPart = [MKGetPartClass() new];
    MKNameObject(name,aPart);
    [self addPart:aPart];
    return aPart;
}

#if 0
-_elements
  /* Same as parts. (needed by Scorefile parser)
     It is a method rather than a C function to hide from the parser
     the differences between Score and ScorefilePerformer.
     */
{
    return [self parts];
}
#endif

@end

