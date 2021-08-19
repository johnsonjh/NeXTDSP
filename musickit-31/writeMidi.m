#ifdef SHLIB
#include "shlib.h"
#endif

/*
  writeMidi.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
  */
/* 
Modification history:

  11/15/89/daj - Fixed bug involving noteDurs without tags (see bug 4031)
  12/20/89/daj - Added feature: If the chan parameter passed to 
                 _MKWriteMidiOut is 0, a channel is fashioned from the 
		 MK_midiChan parameter, if any.
  01/07/89/daj - Added comments.
  02/25/90/daj - Moved putSysExclByte to writeMidi. 
                 Changes to accomodate new way of doing midiFiles.
  04/21/90/daj - Flushed unused auto var.
  04/27/90/daj - Removed check if _MKClassConductor exists. It always exists
                 now that we're a shlib.
*/

/* This is fairly complicated, due to the differences between Music Kit and
   MIDI semantics. See the discussion in the manual (not in 1.0); also 
   available as ~david/doc/Midi.doc. */

#import "_musickit.h"
#import "_midi.h"
#import "_Note.h"   
#import "_Conductor.h"
#import "_tokens.h"
#import <objc/Object.h>
#import <ctype.h>

#define INT(_x) ((int) _x)
#define UCHAR(_x) ((unsigned char) _x)

/* Tag map nodes and init/finish. ----------------------------------------- */

/* midiOutNode is a struct that maps a noteTag onto a keyNumber (key). It
   also contains a noteDurOff and noteDurMsgPtr. These are use for 
   enqueing a noteOff that was derived from a noteDur. */
typedef struct _midiOutNode {
    unsigned key;
    MKMsgStruct *noteDurMsgPtr;
    id noteDurOff;
} midiOutNode;    

#define CACHESIZE 20
static midiOutNode *cache[CACHESIZE]; 
static int cachePtr = 0;

#import <objc/HashTable.h>

_MKMidiOutStruct *_MKInitMidiOut()
    /* Creates new _MKMidiOutStruct data structure */
{
    static BOOL beenHere = NO;
    int i;
    _MKMidiOutStruct *rtn;
    if (!beenHere) {
	beenHere = YES;
	for ( ; cachePtr < CACHESIZE; cachePtr++) 
	  _MK_CALLOC(cache[cachePtr],midiOutNode,1);
    }
    _MK_MALLOC(rtn, _MKMidiOutStruct, 1);
    for (i=0; i<MIDI_NUMCHANS; i++) /* Map from tag to midiOutNode. */
      rtn->_map[i] = [HashTable newKeyDesc:"i" valueDesc:"$"];
    rtn->_timeTag = 0;
    return rtn;
}

unsigned char _MKGetSysExByte(char **strP)
    /* Helper function for putSysExcl below */
{
#   import <ctype.h>
    char *str = *strP;
    BOOL gotOne = NO;
    unsigned char rtn = 0;
    unsigned char digit;
    while (*str && !isxdigit(*str)) 
      str++;
    while (isxdigit(*str)) {
	rtn = rtn << 4; 
	digit = ((isdigit(*str)) ? (*str++ - '0') : 
		 (((isupper(*str)) ? tolower(*str++) : *str++) 
		  - 'a' + 10));
	rtn += digit;
	gotOne = YES;
    }
    *strP = str;
    return ((gotOne) ? rtn : MIDI_EOX);
}

static midiOutNode *newMidiOutNode(unsigned akey)
  /* Make a new ordered triple with x1 and x2 values. */
{
    midiOutNode *rtn;
    if (cachePtr > 0) 
      rtn = cache[--cachePtr];
    else _MK_CALLOC(rtn,midiOutNode,1);
    rtn->key = akey;
    return rtn;
}

static void cancelMidiOutNoteDurMsg(midiOutNode *node);

static void freeMidiOutNode(midiOutNode *aNode)
    /* free struct, caching if possible */
{
    cancelMidiOutNoteDurMsg(aNode);
    if (cachePtr < CACHESIZE)
      cache[cachePtr++] = aNode;
    else NX_FREE(aNode);
}

static void midiOutNodeNoteDur(midiOutNode *node,id aNoteDur,id msgReceiver)
    /* Enqueues a noteOff corresponding to a noteDur's dur. */
{
    id cond;
    double time;
    cond = [aNoteDur conductor];
    if (node->noteDurOff) 
      [node->noteDurOff free];
    node->noteDurOff = [aNoteDur _noteOffForNoteDur];      
    time = [cond time] + [aNoteDur dur]; 
    /* Always do beat time here. When the note eventually happens, it will
       be written in beats or seconds depending on how the caller of 
       _MKMidiOut passes the time. */
    /* See comment in SynthPatch. */
    node->noteDurMsgPtr = 
      [cond _rescheduleMsgRequest:node->noteDurMsgPtr atTime:
       (time - _MK_TINYTIME) sel:@selector(receiveNote:) to:
       msgReceiver argCount:1,node->noteDurOff];
}

static void cancelMidiOutNoteDurMsg(midiOutNode *node)
    /* Cancel request for noteOff that was derived from a noteDur Note. */
{
    if (!node->noteDurOff)
      return;
    node->noteDurMsgPtr = [_MKClassConductor()
			   _cancelMsgRequest:node->noteDurMsgPtr]; 
    [node->noteDurOff free];
    node->noteDurOff = nil;
}

_MKMidiOutStruct *
_MKFinishMidiOut(ptr)
    _MKMidiOutStruct *ptr;
    /* Delete _MKMidiOutStruct data structure */
{
    id map;
    int i,key;
    midiOutNode *value;
    NXHashState state;
    if (!ptr) 
      return NULL;
    for (i=0; i<MIDI_NUMCHANS; i++) {
	map = ptr->_map[i]; 
	state = [map initState];
	while ([map nextState:&state key:(const void **)&key 
	      value:(void **)&value]) 
	  freeMidiOutNode(value);
	[map free];
    }
    NX_FREE(ptr);
    return NULL;
}

#define MCHAN(_x) ((_x-1) & MIDI_MAXCHAN)

static unsigned char mChan(unsigned int i)
    /* Converts 1-based channel to 0-based channel */
{
    if (i == MAXINT)
	return 0;
    return MCHAN(i);
}

#define SET3BYTES(_ptr,_x,_y,_z) \
  ptr->_bytes[0] = _x; ptr->_bytes[1] = _y; ptr->_bytes[2] = _z; \
  ptr->_outBytes = 3
#define SET2BYTES(_ptr,_x,_y) \
  ptr->_bytes[0] = _x; ptr->_bytes[1] = _y;  ptr->_outBytes = 2
#define SET1BYTE(_ptr,_x) \
  ptr->_bytes[0] = _x; ptr->_outBytes = 1 

static void noteOn(unsigned char chan,unsigned char keyNum,
		   unsigned char velocity,_MKMidiOutStruct *ptr)
    /* Write a noteOn */
{
    velocity = MAX(velocity,1); /* Velocity is clipped in getMidiVelocity */
    SET3BYTES(ptr,MIDI_NOTEON | chan,keyNum,velocity);
    (*ptr->_putChanMidi)(ptr);
    /* keyNum is clipped before we get here. */
}

static void noteOff(unsigned char chan,
		    unsigned char keyNum,
		    unsigned int velocity, /* int to catch MAXINT */
		    _MKMidiOutStruct *ptr)
    /* Write a noteOff */
{
    if (velocity == MAXINT) {
	SET3BYTES(ptr,MIDI_NOTEON | chan,keyNum,0);
    }
    else {
	SET3BYTES(ptr,MIDI_NOTEOFF | chan,keyNum,UCHAR(velocity));
    }
    (*ptr->_putChanMidi)(ptr);
}

#define MIDICLIP(_x) ((unsigned char) MIDI_DATA(MAX(0,_x)))

void  _MKWriteMidiOut(id aNote,double timeTag,unsigned chan, /* 1 based */
		      _MKMidiOutStruct *ptr,id noteReceiver)
    /*
     * Convert the note into midi messages and write them as specified 
     * in ptr. Also inserts timeTag. 
     * A timeTag with no note may be inserted by setting aNote
     * to nil. The timeTag is in seconds. 

     See the Midi documentation (or ~david/doc/Midi.doc) for details about
     Music Kit-to-MIDI semantic conversion. 

     chan is the channel to write the note on, for channel
     voice messages. Note that for channel mode messages,
     the MK_basicChan parameter is used. Chan is 1-based.

     */
{
    int             midiPar = 0;
    int _ival;
    int keyNum = MAXINT;
    unsigned        pars;
    MKNoteType type;

#   define ADDCHAN(_x)  (_x | chan)
#   define ADDBCHAN(_x) (_x | mChan(MKGetNoteParAsInt(aNote,MK_basicChan)))
      
    /* This is to lookup parameter and fill in a default. */

#   define GETPAR(_par,_default) \
    (((_ival=MKGetNoteParAsInt(aNote,_par))==MAXINT) ? \
     _default : MIDICLIP(_ival))

#   define BIT(_x) ((unsigned int)1 << _x)
#   define MIDIOPMASK ((unsigned int)(\
				      BIT(MK_afterTouch) | \
				      BIT(MK_controlChange) | \
				      BIT(MK_pitchBend) | \
				      BIT(MK_programChange) | \
				      BIT(MK_timeCodeQ) | \
				      BIT(MK_songPosition) | \
				      BIT(MK_songSelect) | \
				      BIT(MK_sysExclusive) | \
				      BIT(MK_chanMode) | \
				      BIT(MK_sysRealTime) | \
				      BIT(MK_tuneRequest)))

#   define FIRSTMIDIOP (int)MK_afterTouch
#   define LASTMIDIOP  (int)MK_sysRealTime
    if (!ptr)
      return;
    ptr->_timeTag = timeTag;     
    pars = [aNote parVector:0];
    if (chan == 0) { /* If channel is 0, use midiChan parameter in Note */
	chan = MKGetNoteParAsInt(aNote,MK_midiChan);
	if (chan == MAXINT) /* No parameter? Default to 1. */
	  chan = 1;
    }
    chan = (chan - 1) & ((unsigned)MIDI_MAXCHAN);
    if (pars & MIDIOPMASK) {
	for (midiPar = FIRSTMIDIOP; midiPar <= LASTMIDIOP; midiPar++)
	  if (pars & BIT(midiPar))
	    switch (midiPar) {
	      case MK_controlChange:
		SET3BYTES(ptr,ADDCHAN(MIDI_CONTROL),
			  GETPAR(MK_controlChange, 121),
			  GETPAR(MK_controlVal, 0));
		(*ptr->_putChanMidi)(ptr);
		break;
	      case MK_programChange:
		SET2BYTES(ptr,ADDCHAN(MIDI_PROGRAM),
			  GETPAR(MK_programChange, 0));
		(*ptr->_putChanMidi)(ptr);
		break;
	      case MK_afterTouch:
		SET2BYTES(ptr,ADDCHAN(MIDI_CHANPRES),
			  GETPAR(MK_afterTouch, 0));
		(*ptr->_putChanMidi)(ptr);
		break;
	      case MK_pitchBend:{
		  int val;
		  val = MKGetNoteParAsInt(aNote,MK_pitchBend);
		  if (val == MAXINT) 
		    val = 0;
		  SET3BYTES(ptr,ADDCHAN(MIDI_PITCH),
			    UCHAR(MIDI_DATA(val)),    /* LSB */
			    UCHAR((MIDI_DATA((val >> 7)))));    /* MSB */
		  (*ptr->_putChanMidi)(ptr);
		  break;
	      }
	      case MK_chanMode:{    /* Channel mode messages. */
		  int modeVal = MKGetNoteParAsInt(aNote,MK_chanMode);
		  switch (modeVal) {
		    case MK_localControlModeOn:
		      SET3BYTES(ptr,ADDBCHAN(MIDI_CHANMODE),
				MIDI_LOCALCONTROL,UCHAR(127));
		      break;
		    case MK_resetControllers:
		      SET3BYTES(ptr,ADDBCHAN(MIDI_CHANMODE),
				MIDI_RESETCONTROLLERS,0);
		      break;
		    case MK_monoMode:
		      SET3BYTES(ptr,ADDBCHAN(MIDI_CHANMODE),MIDI_MONO,
				GETPAR(MK_monoChans, 0));
		      break;
		    default:          /* All others take a 0 arg. */
		      SET3BYTES(ptr,ADDBCHAN(MIDI_CHANMODE),
				MIDICLIP(modeVal - INT(MK_allNotesOff) + 
					 MIDI_ALLNOTESOFF),
				0);
		      break;
		  }
		  (*ptr->_putChanMidi)(ptr);
		  break;
	      }
	      case MK_timeCodeQ:         /* System common parameters. */
	      case MK_tuneRequest:  
	      case MK_songSelect:
	      case MK_songPosition:
		ptr->_runningStatus = 0; 
		switch (midiPar) {
		  case MK_tuneRequest:
		    SET1BYTE(ptr,MIDI_TUNEREQ);
		    break;
		  case MK_songPosition:{
		      int             val;
		      val = MKGetNoteParAsInt(aNote,MK_songPosition);
		      if (val == MAXINT) val = 0;
		      SET3BYTES(ptr,MIDI_SONGPOS,
				MIDI_DATA(val),         /* LSB */
				MIDI_DATA((val >> 7))); /* MSB */	
		      break;
		  }
		  case MK_songSelect:
		    SET2BYTES(ptr,MIDI_SONGSEL,GETPAR(MK_songSelect, 0));
		    break;
		  case MK_timeCodeQ:
		    SET2BYTES(ptr,MIDI_TIMECODEQUARTER,
			      GETPAR(MK_timeCodeQ, 0));
		    break;
		}
		(*ptr->_putSysMidi)(ptr);
		break;
	      case MK_sysRealTime:
		SET1BYTE(ptr,(MIDI_CLOCK + 
			      MKGetNoteParAsInt(aNote,MK_sysRealTime) -
			      INT(MK_sysClock)));
		(*ptr->_putSysMidi)(ptr);
		break;
	      case MK_sysExclusive:{
		  ptr->_runningStatus = 0;
		  (*ptr->_putSysExcl)
		    (ptr,MKGetNoteParAsStringNoCopy(aNote,MK_sysExclusive));
		  break;
	      }
	      default:
		continue;
	    }
    }
    
#   define KEYPRESSURE() (pars & BIT(MK_keyPressure))
    /* Key pressure is special because it requires a keyNum so it can only
       occur in the context of a noteOn, noteOff or update with a noteTag.
       */
    
    switch (type = [aNote noteType]) {
      case MK_noteDur:
      case MK_noteOn:
	{
	    int             velocity;
	    int             noteTag = [aNote noteTag];
	    BOOL newTag = NO;
	    midiOutNode *mapNode;
	    /* Every noteOn must have a keyNumber or it gets the default
	       keyNumber, not(!) the keyNumber of the previous note with
	       this note tag. */
	    if (noteTag == MAXINT) 
	      if (type == MK_noteOn) {
		  if (_MKTrace() & MK_TRACEMIDI) {
		      fprintf(stderr,"NoteOn missing a noteTag at time %f",
			      ptr->_timeTag);
		  }
		  (*ptr->_sendBufferedData)(ptr);
		  return;
	      }
	      else {
		  [aNote setNoteTag:noteTag = MKNoteTag()];
		  newTag = YES;
	      }
	    velocity = MKGetNoteParAsInt(aNote,MK_velocity);
	    if (velocity == MAXINT)
	      velocity = 64;
	    mapNode = (midiOutNode *)[ptr->_map[chan] valueForKey:
				  (const void *)noteTag];
	    keyNum = [aNote keyNum];
	    if (keyNum == MAXINT) 
	      if (mapNode)
		keyNum = mapNode->key;
	      else keyNum = 64;
	    if (mapNode) {
		BOOL sameKey = (mapNode->key == keyNum);
		/* If the key is the same, we do the noteOff first. The point
		   here is that we know there are two noteOns on the same
		   noteTag so this has! to be the same MIDI voice. The only
		   way to get a rekey in MIDI is to do a noteOff followed
		   by a noteOn. Reversing the order here is incorrect because
		   the MIDI spec says the synthesizer may fire up a new voice
		   */
		if (sameKey)
		  noteOff(chan,keyNum,MAXINT,ptr);
		noteOn(chan, UCHAR(keyNum), UCHAR(velocity),ptr);
		/* If the key is not the same, do the noteOn before noteOff.
		   This is because we want Mono mode to work. 
		   A penalty may be felt if the receiver is in Poly
		   mode because a voice may be spuriously cut off. 
		   But, we argue (MMM), if someone puts several noteOns in 
		   the same tag stream, he wants them to be mono or 
		   pseudo-mono and thus we are doing the right thing. 
		   If he really wants poly, he should use different notetags. 
		   */
		if (!sameKey) {
		    noteOff(chan,UCHAR(mapNode->key),MAXINT,ptr);
		    mapNode->key = keyNum;
		}
		cancelMidiOutNoteDurMsg(mapNode);
	    }
	    else {
		noteOn(chan, UCHAR(keyNum), UCHAR(velocity), ptr);
		[ptr->_map[chan] insertKey:(const void *)noteTag
	       value:(void *)(mapNode = newMidiOutNode(keyNum))];
	    }
	    if (KEYPRESSURE()) {
		SET3BYTES(ptr,ADDCHAN(MIDI_POLYPRES),keyNum,
			  GETPAR(MK_keyPressure, 0));
		(*ptr->_putChanMidi)(ptr);
	    }
	    if ((type == MK_noteDur) && (noteReceiver))
	      midiOutNodeNoteDur(mapNode,aNote,noteReceiver);
	    if (newTag) /* Restore it to its original state. */
	      [aNote setNoteTag:MAXINT];
	    break;
	}
      case MK_noteOff:
	{
	    int noteTag = [aNote noteTag];
	    int velocity = MKGetNoteParAsInt(aNote,MK_relVelocity);
	    midiOutNode *mapNode;
	    if (noteTag == MAXINT) {
		if (_MKTrace() & MK_TRACEMIDI) {
		    fprintf(stderr,"NoteOff missing a note tag at time %f",
			    ptr->_timeTag);
		}
		break;
	    }
	    mapNode = (midiOutNode *)[ptr->_map[chan] removeKey:
				      (const void *)noteTag];
	    if (!mapNode) {
		if (_MKTrace() & MK_TRACEMIDI) {
		    fprintf(stderr,
			    "NoteOff for noteTag which is already off at time %f",
			    ptr->_timeTag);
		}
		break;
	    }
	    keyNum = mapNode->key;
	    noteOff(chan,UCHAR(keyNum),velocity,ptr);
	    freeMidiOutNode(mapNode);
	    if (KEYPRESSURE()) {
		SET3BYTES(ptr,ADDCHAN(MIDI_POLYPRES),keyNum,
			  GETPAR(MK_keyPressure, 0));
		(*ptr->_putChanMidi)(ptr);
	    }
	}
	break;
      case MK_noteUpdate:
	if (KEYPRESSURE()) {
	    int noteTag = [aNote noteTag];
	    midiOutNode *mapNode = 
	      (midiOutNode *)[ptr->_map[chan] 
			    valueForKey:(const void *)noteTag];
	    if (!mapNode) {
		(*ptr->_sendBufferedData)(ptr);
		if (_MKTrace() & MK_TRACEMIDI) {
		    fprintf(stderr,
			    "PolyKeyPressure with invalid noteTag or missing" 
			    " keyNum: %s %f;",
			    _MKTokNameNoCheck(_MK_time),ptr->_timeTag);
		}
		break; /* Gets us out of entire case statement */
	    }
	    keyNum = mapNode->key;
	    SET3BYTES(ptr,ADDCHAN(MIDI_POLYPRES),keyNum,
		      GETPAR(MK_keyPressure, 0));
	    (*ptr->_putChanMidi)(ptr);
	}
	break;
      default:       /* Nothing special for mutes. */
	break;
    }
    (*ptr->_sendBufferedData)(ptr);
}

/* Musickit-to-MIDI:
   
   On a given tag stream: 
   * If we get two noteOns on the same key, do noteOff followed by noteOn.
   * If we get a noteOn on a new key, do noteOn followed by noteOff on old key.
   
   As an added feature, we map identical tags on different channels to
   different pseudo-tag streams. Sending the same tag to two channels is 
   really an application bug. So this is just a way of being forgiving.

*/


