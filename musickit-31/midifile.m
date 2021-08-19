#ifdef SHLIB
#include "shlib.h"
#endif

/* Based on original version written by Lee Boynton, extensively revised by
   David Jaffe. 
   
   These routines might eventually be in a MIDI library. For now, they're
   private Music Kit functions. 

   The division of labor is as follows: All Music-Kit specifics are kept out
   of this file. The functions in this file read/write raw MIDI and meta-
   events.
*/

/* 
Modification history:

  12/21/89/daj - Fixed bug in writing level-1 files. last_write_time 
                 needs to be set to 0 when the track's incremented.
		 Note: If this stuff is ever made reentrant (like if 
		       I ever support MidifileWriter/Performer), all
		       statics will have to be struct entries.
  02/25-8/90/daj - Changed to make reentrant. Added sysexcl support.
                 Added support for most meta-events. All of the meta-events
		 defined in the July 1988 MIDI file spec are supported, 
		 with the exception of "MIDI Channel Prefix",
		 "Instrument Name" and "Sequencer-Specific Meta-Event".

                 To do:
		     Add support for writing formats 0 and 2.
		     Add support for SMPTE and MIDI time code (see
		     "division").
		     Implement MIDI Channel Prefix

  04/29/90/daj - Flushed unused auto vars
*/



#import "_musickit.h"
#import <midi/midi_types.h>
#import "_midifile.h"

/* Some metaevents */
#define SEQUENCENUMBER 0
#define TRACKCHANGE 0x2f
#define TEMPOCHANGE 0x51
#define SMPTEOFFSET 0x54
#define TIMESIG 0x58
#define KEYSIG 0x59

#define DEFAULTTEMPO (120.0)
#define DEFAULTDIVISION 1024

/*
 * reading
 */

typedef struct _midiFileInStruct {
    double tempo;    /* in quarter notes per minute */
    double timeScale;/* timeScale * currentTime gives time in seconds */
    int currentTrack;/* Current track number */
    int currentTime; /* Current time in quanta. */
    int division;    /* # of delta-time ticks per quarter. (See spec) */
    short format;      /* Level 0, 1 or 2 */
    int quantaSize;  /* In micro-seconds. */
    unsigned char runningStatus; /* Current MIDI running status */
    NXStream *s;     /* Midifile stream */
    int curBufSize;  /* Size of data buffer */
    /* Info for current msg. These are passed up to caller */
    int quanta;	     /* Time in quanta */
    BOOL metaeventFlag;/* YES if this is a meta-event */
    int nData;       /* Number of significant bytes in data */
    unsigned char *data; /* Data bytes */
} midiFileInStruct;

#define REFERENCE **

void *_MKMIDIFileBeginReading(NXStream *s,
			      int REFERENCE quanta,
			      BOOL REFERENCE metaeventFlag,
			      int REFERENCE nData,
			      unsigned char * REFERENCE data)
{
    midiFileInStruct *rtn;
    _MK_MALLOC(rtn,midiFileInStruct,1);
    rtn->tempo = DEFAULTTEMPO; 	    /* in beats per minute */
    rtn->currentTrack = -1; /* We call the first or "tempo track" 
			       "track 0". Therefore, we start counting at -1
			       here. */ 
    rtn->currentTime = 0;
    rtn->division = 0;
    rtn->format = 0;
    rtn->quantaSize = _MKMIDI_DEFAULTQUANTASIZE; /* size in microseconds */
    rtn->s = s;
    /* Malloc enough for SMPTEoffset metaevent. Realloc longer fields later */
    rtn->curBufSize = 6;
    _MK_MALLOC(rtn->data,unsigned char,rtn->curBufSize);
    /* Values are always returned indirectly in these fields. */
    *nData = &rtn->nData;            
    *data = &rtn->data;
    *metaeventFlag = &rtn->metaeventFlag;
    *quanta = &rtn->quanta;
    return rtn;
}

#define IP ((midiFileInStruct *)p)

void *_MKMIDIFileEndReading(void *p)
{
    NX_FREE(IP->data);
    NX_FREE(IP);
    return NULL;
}

enum {unrecognized = -1,endOfStream = 0,ok = 1,undefined,
	/* Multi-packet sys excl: */
	firstISysExcl,middleISysExcl,endISysExcl, 
	/* Single-packet sys excl */
	sysExcl};                                 

static int calcMidiEventSize(int status)
{
    if (MIDI_TYPE_3BYTE(status))
      return 3;
    else if (MIDI_TYPE_2BYTE(status))
      return 2;
    else return 1;
}

static int readChunkType(NXStream *s,char *buf)
{
    int count = NXRead(s,buf,4);
    buf[4] = '\0';
    return (count == 4)? ok : 0;
}

static int readLong(NXStream *s, int *n)
{
    int count = NXRead(s,n,4);
    return (count == 4)? ok : 0;
}

static int readBytes(NXStream *s, unsigned char *bytes,int n)
{
    int count = NXRead(s,bytes,n);
    return (count == n) ? ok : 0;
}

static int readShort(NXStream *s, short *n)
{
    int count = NXRead(s,n,2);
    return (count == 2)? ok : 0;
}

static int readVariableQuantity(NXStream *s, int *n)
{
    int m = 0;
    unsigned char temp;
    while (NXRead(s,&temp,1) > 0) {
	if (128 & temp)
	  m = (m<<7) + (temp & 127);
	else {
	    *n = (m<<7) + (temp & 127);
	    return ok;
	}
    }
    return endOfStream;
}

static int readTrackHeader(midiFileInStruct *p)
{
    char typebuf[8];
    int size;
    if (!readChunkType(p->s,typebuf)) 
      return endOfStream;
    if (strcmp(typebuf,"MTrk")) 
      return endOfStream;
    p->currentTrack++;
    p->currentTime = 0;
    if (!readLong(p->s,&size)) 
      return endOfStream;
    return ok;
}

static void checkRealloc(midiFileInStruct *p,int newSize)
{
    if (p->curBufSize < newSize)
      _MK_REALLOC(p->data,unsigned char,newSize);
    p->curBufSize = newSize;
}

static int readMetaevent(midiFileInStruct *p)
{
    unsigned char theByte;
    int temp;
    int len;
    if (!(NXRead(p->s,&theByte,1) && readVariableQuantity(p->s,&len)))
      return endOfStream;
    if (theByte == SEQUENCENUMBER) {
	short val;
	if (!readShort(p->s,&val))
	  return endOfStream;
	p->data[0] = _MKMIDI_sequenceNumber;
	p->data[1] = val >> 8;
	p->data[2] = val;
	len -= 2;
	p->nData = 3;
    }
    else if (theByte >= 1 && theByte <= 0x0f) { /* Text meta events */
	p->data[0] = theByte;
	p->nData = len + 1; /* nData doesn't include the \0 */
	checkRealloc(p,p->nData + 1);
	if (!readBytes(p->s,&(p->data[1]),p->nData - 1))
	  return endOfStream;
	p->data[p->nData] = '\0';
	return ok;
    }
    else if (theByte == TRACKCHANGE) { 		/* end of track */
	temp = readTrackHeader(p);
	if (temp == endOfStream) 
	  return endOfStream;
	/* trackChange doesn't have any args but we pass up the track number,
	   so no len -= needed.
	 */
	p->nData = 3;
	p->data[0] = _MKMIDI_trackChange;
	p->data[1] = (p->currentTrack >> 8);
	p->data[2] = p->currentTrack;
    } 
    else if (theByte == TEMPOCHANGE) { 	         /* tempo */
	double n;
	int i;
	if (!readBytes(p->s,&(p->data[1]),3))    /* 24 bits */
	  return endOfStream;
	i = (p->data[1] << 16) | (p->data[2] << 8) | p->data[3];
	n = (double)i;
	/* tempo in file is in micro-seconds per quarter note */
	p->tempo = 60000000.0 / n; 
	i = p->tempo;
	/* division is the number of delta time "ticks" that make up a 
	   quarter note. Quanta size is in micro seconds. */
	p->timeScale = n / (double)(p->division * p->quantaSize);
	p->data[0] = _MKMIDI_tempoChange;
	/* It's a 3 byte quantity but we store it in 4 bytes.*/
	p->data[1] = 0;         
	p->data[2] = (i >> 16);
	p->data[3] = (i >> 8);
	p->data[4] = i;
	p->nData = 5;
	len -= 3;
    } 
    else if (theByte == SMPTEOFFSET) {
	p->data[0] = _MKMIDI_smpteOffset;
	if (!readBytes(p->s,&(p->data[1]),5))
	  return endOfStream;
	p->nData = 6;
	len -= 5;
    } 
    else if (theByte == TIMESIG) {
	if (!readBytes(p->s,&p->data[1],4))
	  return endOfStream;
	p->data[0] = _MKMIDI_timeSig;
	p->nData = 5;
	len -= 4;
    } 
    else if (theByte == KEYSIG) {
	if (!readBytes(p->s,&p->data[1],2))
	  return endOfStream;
	p->data[0] = _MKMIDI_keySig;
	p->nData = 3;
	len -= 2;
    } 
    else { /* Skip unrecognized meta events */
	if (!readVariableQuantity(p->s,&temp))
	  return endOfStream;
	NXSeek(p->s,temp,NX_FROMCURRENT);
	return unrecognized;
    }
    NXSeek(p->s,len,NX_FROMCURRENT); /* Skip any extra length in field. */
    return ok;
}

/* We do not support multi-packet system exclusive messages with different
   timings. When such a beast occurs, it is concatenated into a single
   event and the time stamp is that of the last piece of the event. */

static int readSysExclEvent(midiFileInStruct *p,int oldState)
{
    int len;
    unsigned char *ptr;
    if (!readVariableQuantity(p->s,&len))
      return endOfStream;
    if (oldState == undefined) {
	checkRealloc(p,len + 1); /* len doesn't include data[0] */
	p->data[0] = MIDI_SYSEXCL;
	p->nData = len + 1;
	ptr = &(p->data[1]);
    } else {      /* firstISysExcl or middleISysExcl */
	checkRealloc(p,len + p->nData);
	ptr = &(p->data[p->nData]);
	p->nData += len; 
    }
    if (readBytes(p->s,ptr,len) == endOfStream)
      return endOfStream;
    return ((p->data[p->nData - 1] == MIDI_EOX) ? 
	    ((oldState == undefined) ? sysExcl : endISysExcl) : 
	    ((oldState == undefined) ? firstISysExcl : middleISysExcl));
}

static int readEscapeEvent(midiFileInStruct *p)
{
    if (!readVariableQuantity(p->s,&(p->nData)))
      return endOfStream;
    checkRealloc(p,p->nData);
    return readBytes(p->s,p->data, p->nData);
}

#define SCALEQUANTA(_p,quanta) \
  ((int)(0.5 + (((midiFileInStruct *)_p)->timeScale * (double)quanta)))

/*
 * Exported routines
 */

int _MKMIDIFileReadPreamble(void *p,int *level,int *trackCount)
{
    char typebuf[8];
    int size;
    short fmt, tracks, div;
    
    if ((!readChunkType(IP->s,typebuf)) || 
	(strcmp(typebuf,"MThd")) ||  /* not a midifile */ 
	(!readLong(IP->s,&size)) ||
	(size < 6) ||               /* bad header */ 
	(!readShort(IP->s,&fmt)) || 
	(fmt < 0 || fmt > 2)  ||     /* must be level 0, 1 or 2 */
	(!readShort(IP->s,&tracks)) ||  
	(!readShort(IP->s,&div))) 
      return endOfStream;
    size -= 6;
    if (size)
      NXSeek(IP->s,size,NX_FROMCURRENT); /* Skip any extra length in field. */
    *trackCount = fmt ? tracks-1 : 1;
    *level = IP->format = fmt;
    if (div < 0) { /* Time code encoding? */
	/* For now, we undo the effect of the time code. We may want to
	   eventually pass the time code up? */ 
	short SMPTEformat,ticksPerFrame;
	ticksPerFrame = div & 0xff;
	SMPTEformat = -(div >> 8); 
	/* SMPTEformat is one of 24, 25, 29, or 30. It's stored negative */
	div = ticksPerFrame * SMPTEformat;
    }
    IP->division = div;
    IP->currentTrack = -1;
    IP->timeScale = 60000000.0 / (double)(IP->division * IP->quantaSize);
    return ok;
}

int _MKMIDIFileReadEvent(register void *p)
    /* return endOfStream when EOS is reached, return 1 otherwise.
       Data should be an array of length 3. */
{
    int deltaTime,quantaTime,state = undefined;
    unsigned char theByte;
    if (IP->currentTrack < 0 && !readTrackHeader(p)) 
      return endOfStream;
    for (;;) {
	if (!readVariableQuantity(IP->s,&deltaTime)) 
	  return endOfStream;
	IP->currentTime += deltaTime;
	quantaTime = SCALEQUANTA(p,IP->currentTime);
	if (!NXRead(IP->s,&theByte,1)) 
	  return endOfStream;
	if (theByte == 0xff) {
	    state = readMetaevent(p);
	    IP->metaeventFlag = YES;
	    if (state != unrecognized) {
		IP->quanta = quantaTime;
		return state;
	    }
	} else if ((theByte == MIDI_SYSEXCL) || (state != undefined)) {
	    /* System exclusive */
	    state = readSysExclEvent(p,state);
	    IP->metaeventFlag = NO;
	    switch (state) {
	      case firstISysExcl:
		IP->quanta = quantaTime;
		break;
	      case middleISysExcl:
		IP->quanta += quantaTime;
		break;
	      case endISysExcl:
		IP->quanta += quantaTime;
		return ok;
	      case endOfStream:
	      case sysExcl:
		IP->quanta = quantaTime;
		return ok;
	      default:
		break;
	    }
	} else if (theByte == 0xf7) { /* Special "escape" code */
	    IP->quanta = quantaTime;
	    return readEscapeEvent(p);
	} else { /* Normal MIDI */
	    BOOL newRunningStatus = (theByte & MIDI_STATUSBIT);
	    if (newRunningStatus)
	      IP->runningStatus = theByte;
	    IP->metaeventFlag = 0;
	    IP->quanta = quantaTime;
	    IP->nData = calcMidiEventSize(IP->runningStatus);
	    IP->data[0] = IP->runningStatus;
	    if (IP->nData > 1) {
		if (newRunningStatus) {
		    if (!NXRead(IP->s,&(IP->data[1]),1)) 
		      return endOfStream;
		}
		else IP->data[1] = theByte;
		if (IP->nData > 2)
		  if (!NXRead(IP->s,&(IP->data[2]),1)) 
		    return endOfStream;
	    }
	    return ok;
	}
    }
}


/*
 * writing
 */

typedef struct _midiFileOutStruct {
    double tempo;     
    double timeScale; 
    int currentTrack;
    int division;
    int currentCount;
    int lastTime;
    NXStream *s;
    int quantaSize;
} midiFileOutStruct;

#define OP ((midiFileOutStruct *)p)

static int writeBytes(midiFileOutStruct *p, unsigned char *bytes,int count)
{
    int bytesWritten;
    bytesWritten = NXWrite(p->s,bytes,count);
    OP->currentCount += count;
    if (bytesWritten != count)
      return endOfStream;
    else return ok;
}

static int writeByte(midiFileOutStruct *p, unsigned char n)
{
    int bytesWritten = NXWrite(p->s,&n,1);
    p->currentCount += bytesWritten;
    return bytesWritten;
}

static int writeShort(midiFileOutStruct *p, short n)
{
    int bytesWritten = NXWrite(p->s,&n,2);
    p->currentCount += bytesWritten;
    return (bytesWritten == 2) ? ok : endOfStream;
}

static int writeLong(midiFileOutStruct *p, int n)
{
    int bytesWritten = NXWrite(p->s,&n,4);
    p->currentCount += bytesWritten;
    return (bytesWritten == 4) ? ok : endOfStream;
}

static int writeChunkType(midiFileOutStruct *p, char *buf)
{
    int bytesWritten = NXWrite(p->s,buf,4);
    p->currentCount += bytesWritten;
    return (bytesWritten == 4) ? ok : endOfStream;
}

static int writeVariableQuantity(midiFileOutStruct *p, int n)
{
    if ((n >= (1 << 28) && !writeByte(p,(((n>>28)&15)|128) )) ||
	(n >= (1 << 21) && !writeByte(p,(((n>>21)&127)|128) )) ||
	(n >= (1 << 14) && !writeByte(p,(((n>>14)&127)|128) )) ||
	(n >= (1 << 7) && !writeByte(p,(((n>>7)&127)|128) ))) 
      return endOfStream;
    return writeByte(p,(n&127));
}

void *_MKMIDIFileBeginWriting(NXStream *s, int level, char *sequenceName)
{
    short lev = level, div = DEFAULTDIVISION, ntracks = 1;
    midiFileOutStruct *p;
    _MK_MALLOC(p,midiFileOutStruct,1);
    OP->tempo = DEFAULTTEMPO; 	    /* in beats per minute */
    OP->quantaSize = _MKMIDI_DEFAULTQUANTASIZE; /* size in microseconds */
    OP->lastTime = 0;
    OP->s = s;
    if ((!writeChunkType(p,"MThd")) || (!writeLong(p,6)) ||
	!writeShort(p,lev) || !writeShort(p,ntracks) || !writeShort(p,div))
      return endOfStream;
    OP->division = div;
    OP->currentTrack = -1;
    OP->timeScale = 60000000.0 / (double)(OP->division * OP->quantaSize);
    OP->currentCount = 0;
    if (_MKMIDIFileBeginWritingTrack(p,sequenceName))
      return p;
    else {
	NX_FREE(p);
	return NULL;
    }
}

int _MKMIDIFileEndWriting(void *p)
{
    short ntracks = OP->currentTrack+1; /* +1 for "tempo track" */
    if (OP->currentCount) {  /* Did we forget to finish before? */
	int err = _MKMIDIFileEndWritingTrack(p,0);
	if (err == endOfStream) {
	    NX_FREE(p);
	    return endOfStream;
	}
    }
    NXSeek(OP->s,10,NX_FROMSTART);
    if (NXWrite(OP->s,&ntracks,2) != 2) {
	NX_FREE(p);
	return endOfStream;
    }
    NXSeek(OP->s,0,NX_FROMEND);
    NX_FREE(p);
    return ok;
}

int _MKMIDIFileBeginWritingTrack(void *p, char *trackName)
{
    if (OP->currentCount) /* Did we forget to finish before? */
      _MKMIDIFileEndWritingTrack(p,0);
    if (!writeChunkType(p,"MTrk")) 
      return endOfStream;
    if (!writeLong(p,0))  /* This will be the length of the track. */
      return endOfStream;
    OP->lastTime = 0;
    OP->currentTrack++;
    OP->currentCount = 0; /* Set this after the "MTrk" and dummy length are
			     written */
    if (trackName) {
	int i = strlen(trackName);
	if (i) {
	    if (!writeByte(p,0) || !writeByte(p,0xff) || !writeByte(p,0x03) || 
		!writeVariableQuantity(p,i) ||
		!writeBytes(p,(unsigned char *)trackName,i))  
	      return endOfStream;
	}
    }
    return ok;
}

static int writeTime(midiFileOutStruct *p, int quanta)
{
    int thisTime = (int)(0.5 + (OP->timeScale * quanta));
    int deltaTime = thisTime - OP->lastTime;
    OP->lastTime = thisTime;
    if (!writeVariableQuantity(p,deltaTime)) 
      return endOfStream;
    return ok;
}

int _MKMIDIFileEndWritingTrack(void *p,int quanta)
{
    if (!writeLong(p,0x00ff2f00)) 
      return endOfStream;
    /* Seek back to the track length field for this track. */
    NXSeek(OP->s,-(OP->currentCount+4),NX_FROMCURRENT);
    /* +4 because we don't include the "MTrk" specification and length
       in the count */
    if (NXWrite(OP->s,&OP->currentCount,4) != 4) 
      return endOfStream;
    /* Seek to end again. */
    NXSeek(OP->s,OP->currentCount,NX_FROMCURRENT);
    OP->currentCount = 0; /* Signals other functions that we've just finished
			     a track. */
    return ok;
}


#define METAEVENT(_x) (0xff00 | _x)

int _MKMIDIFileWriteSig(void *p,int quanta,short metaevent,unsigned data)
{
    BOOL keySig = (metaevent == _MKMIDI_keySig);
    unsigned char byteCount = (keySig) ? 2 : 4;
    metaevent = METAEVENT(((keySig) ? KEYSIG : TIMESIG));
    if (!writeTime(p,quanta) || !writeShort(p,metaevent) ||
	!writeByte(p,byteCount))
      return endOfStream;
    return (keySig) ? writeShort(p,data) : writeLong(p,data);
}

int _MKMIDIFileWriteText(void *p,int quanta,short metaevent,char *text)
{
    int i;
    metaevent = METAEVENT(metaevent);
    if (!text)
      return ok;
    i = strlen(text);
    if (!writeTime(p,quanta) || !writeShort(p,metaevent) ||
	!writeVariableQuantity(p,i) || !writeBytes(p,(unsigned char *)text,i))
      return endOfStream;
    return ok;
}

int _MKMIDIFileWriteSMPTEoffset(void *p,unsigned char hr,unsigned char min,
				unsigned char sec,unsigned char ff,
				unsigned char fr)
{
    if (!writeByte(p,0) ||  /* Delta-time is always 0 for SMPTE offset */
	!writeShort(p,METAEVENT(SMPTEOFFSET)) ||
	!writeByte(p,5) || !writeByte(p,hr) || !writeByte(p,min) ||
	!writeByte(p,sec) || !writeByte(p,fr) || !writeByte(p,ff))
      return endOfStream;
    return ok;
}

int _MKMIDIFileWriteSequenceNumber(void *p,int data)
{
    if (!writeByte(p,0) || /* Delta time is 0 */
	!writeShort(p,METAEVENT(SEQUENCENUMBER)) ||
	!writeByte(p,2) || !writeShort(p,data))
      return endOfStream;
    return ok;
}

int _MKMIDIFileWriteTempo(void *p,int quanta, int beatsPerMinute)
{
    int n;
    OP->tempo = beatsPerMinute;
    n = (int)(0.5 + (60000000.0 / OP->tempo));
    OP->timeScale = (double)(OP->division * OP->quantaSize) / (double)n;
    n &= 0x00ffffff;
    n |= 0x03000000;
    if (!writeTime(p,quanta) || !writeShort(p,METAEVENT(TEMPOCHANGE)) || 
	!writeLong(p,n)) 
      return endOfStream;
    return ok;
}

int _MKMIDIFileWriteEvent(register void *p,int quanta,int nData,
			  unsigned char *bytes)
{
    if (!writeTime(p,quanta))
      return endOfStream;
    if (nData && MIDI_TYPE_SYSTEM(bytes[0])) 
      if (!writeByte(p,MIDI_EOX) ||  /* Escape byte */
	  !writeVariableQuantity(p,nData)) /* Length of message */
	return endOfStream;
    if (!writeBytes(p,bytes,nData)) 
      return endOfStream;
    return ok;
}

int _MKMIDIFileWriteSysExcl(void *p,int quanta,unsigned char *bytes,int nData)
    /* Assumes there's no MIDI_SYSEXCL at start and there is a
     * MIDI_EOX at end. */
{
    if (!writeTime(p,quanta) || !writeByte(p,MIDI_SYSEXCL) ||
	!writeVariableQuantity(p,nData) || !writeBytes(p,bytes,nData))
      return endOfStream;
    return ok;
}

#if 0

int _MKMIDISetReadQuantasize(void *p,int usec)
{
    IP->quantaSize = usec;
    if (IP->division) {
	double n = 60000000.0 / IP->tempo;
	IP->timeScale = n / (double)(IP->division * IP->quantaSize);
    }
    return usec;
}

#endif

