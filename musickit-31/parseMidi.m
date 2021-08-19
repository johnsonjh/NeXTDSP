#ifdef SHLIB
#include "shlib.h"
#endif

/*
  parseMidi.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
  */

/* 
Modification history:

  09/15/89/daj - Changed to use cached Note class. (_MKClassNote())
  01/02/90/daj - Added setting of noteType for noteUpdates (needed because
                 I flushed possiblyUpdateNoteType()). Flushed out-dated comment
		 Changed comments.
  03/01/90/daj - Changed noteOff reader to omit relVelocity when incoming
                 relVelocity is 64.
  03/19/90/daj - Changed to use MKGetNoteClass(). 
*/

#import "_musickit.h"
#import "_midi.h"
#import "_Note.h"   

#define INT(_x) ((int) _x)

_MKMidiInStruct *
  _MKInitMidiIn()
/*
  Makes a new _MKMidiInStruct for specified file. Assumes file is open.
  */
{
    int i;
    _MKMidiInStruct *rtn;
    _MK_CALLOC(rtn,_MKMidiInStruct,1);
    rtn->_noteTags[0] = MKNoteTags(MIDI_NUMKEYS * MIDI_NUMCHANS);
    for (i=1; i<MIDI_NUMCHANS; i++)
      rtn->_noteTags[i] = rtn->_noteTags[i-1] + MIDI_NUMKEYS;
    /* CALLOC above insures all fields are cleared. */
    return rtn;
}

_MKMidiInStruct *
  _MKFinishMidiIn(_MKMidiInStruct *ptr)
{
    /*
      Deletes data structure pointed to by ptr. Returns NULL. 
      Does not close file.
     */
    if (ptr) 
      NX_FREE(ptr);
    return NULL;
}

static int
  cv14(unsigned char byte2, unsigned char byte3)
{
    /* Convert 2 byte quantity to 14 bit int. */
    return (((unsigned int)byte3) << 7) | ((unsigned int) byte2);
}

/* All of the following is needed because... the MIDI spec leaves it up to 
   the receiver what to do with multiple note-ons on the same channel and
   key number. If _MK_MIDIIN_MULTIPLE_VOICES_ON_SAME_KEYNUM is true,
   we do something smart: we allocate a new voice (noteTag). Otherwise,
   (the default compilation) we use the same voice (noteTag). */

#define IS_ON(_ptr,_chan,_keyNum) (_ptr->_on[_keyNum] & (1 << (_chan - 1)))
#define IS_OFF(_ptr,_chan,_keyNum) (!IS_ON(_ptr,_chan,_keyNum))
#define SET_ON(_ptr,_chan,_keyNum) _ptr->_on[_keyNum] |= (1 << (_chan - 1))
#define CLEAR_ON(_ptr,_chan,_keyNum) _ptr->_on[_keyNum] &= \
    ~(((unsigned short)1) << (_chan - 1))

#if _MK_MIDIIN_MULTIPLE_VOICES_ON_SAME_KEYNUM
/* This is normally false */

#define IS_DEFAULT_TAG_OFF_SENT(_ptr,_chan,_keyNum) (_ptr->_defTagOffSent[_keyNum] & (1<<(_chan - 1)))
#define SET_DEFAULT_TAG_OFF_SENT(_ptr,_chan,_keyNum) _ptr->_defTagOffSent[_keyNum] |= (1<<(_chan - 1))
#define CLEAR_DEFAULT_TAG_OFF_SENT(_ptr,_chan,_keyNum) _ptr->_defTagOffSent[_keyNum] &= \
    ~(((unsigned short)1) << (_chan - 1))

/* The duplicates list is an array, accessed by channel, of pointers of 
   type (midiInList *). Each points to a list of midiInLists.  
   Each midiInList contains a list of dupliates. Each entry corresponds to
   duplicate key numbers for a particular channel/keyNum pair. */ 
   
typedef struct _midiInNode {
    struct _midiInNode *next;
    struct _midiInNode *prev;
    int tag;
} midiInNode;

typedef struct _midiInList {
    struct _midiInList *next;
    midiInNode *head,*tail;
    short keyNum;
} midiInList;

static midiInList *findList(short keyNum,_MKMidiInStruct *ptr) 
    /* Find list with given keynNum/chan */
{
    midiInList *l; 
    l = (midiInList *)ptr->_tagLists[ptr->chan];
    while (l && (l->keyNum != keyNum))
      l = l->next;
    return l;
}

/* List of one element is represented by head==tail!=NULL. 
   List of zero elements is represented by head==tail==NULL. */

static void addToTailList(midiInNode *n,midiInList *l,int tag)
    /* Add to tail of list. */
{
    n->next = NULL;          /* Make n the new tail of list */
    if (l->head) {           /* If it has a head, it has a tail. */
	l->tail->next = n;   
	n->prev = l->tail;
    }
    else {
	l->head = n;
	n->prev = NULL;
    }
    l->tail = n;
    n->tag = tag;
}

static midiInList *newList(_MKMidiInStruct *ptr,short keyNum)
    /* Create a new list */
{
    midiInList *l = (midiInList *)ptr->_tagLists[ptr->chan];
    midiInList *nl;
    _MK_CALLOC(nl,midiInList,1);
    /* Add the new list struct as the first element in the chain of 
       list structs. */
    nl->next = (midiInList *)ptr->_tagLists[keyNum];
    ptr->_tagLists[ptr->chan] = (void *)nl;
    nl->keyNum = keyNum; /* KeyNum represented by this list struct */
    return nl;
}

static int removeHeadOfList(midiInList *l,_MKMidiInStruct *ptr,short keyNum)
    /* Remove head of list */
{
    midiInNode *n = l->head;
    int rtnVal;
    l->head = n->next;
    if (!l->head) {
	l->tail = NULL;
	CLEAR_DEFAULT_TAG_OFF_SENT(ptr,ptr->chan,keyNum);
	CLEAR_ON(ptr,ptr->chan,keyNum);
    } else l->head->prev = NULL;
    rtnVal = n->tag;
    NX_FREE(n);
    return rtnVal;
}

#endif

id _MKMidiToMusicKit(_MKMidiInStruct *ptr,unsigned statusByte) 
    /* You call this with the _dataByte fields set appropriately. 
       Returns a note or nil.  The note which is
       returned is owned by the struct. It should be copied on memory.

       ptr->chan is set as follows: If the note represents a
       midi 'channel voice message', ptr->chan is the channel.
       Otherwise, if the note represents a midi 'system
       message' or 'channel mode message', ptr->chan is set to _MK_MIDISYS.
       In the case of 'channel mode messages', the basic channel is represented
       by a parameter in the note.
       
       The conversion from MIDI to musickit semantics is performed as
       follows: Notetags are generated for all notes with key numbers (i.e.
       noteOn, noteOff, and key pressure). Multiple noteOns on the same key 
       number/chan (without intervening noteOffs) are assigned different 
       noteTags. Poly key pressure always goes to the most recently sounding 
       note on a given keyNum/chan. 

       Extraneous noteOffs are filtered out. */
{

    
#   define KITCHAN(_x) ((MIDI_MAXCHAN & _x)+1)
#   define NOTETAG(_chan,_keyNum) (ptr->_noteTags[_chan-1] + _keyNum)
      
    id aNote;                   /* The note. */
    int keyNum;

    if (!ptr)
      return nil;

    /* If we're here, we've got a complete midi message */

    [ptr->_note free]; /* Free old note. */ 
    aNote = ptr->_note = [MKGetNoteClass() newSetTimeTag:ptr->timeTag];
    switch (MIDI_OP(statusByte)) { 
      case MIDI_SYSTEM: 	
	ptr->chan = _MK_MIDISYS; 	
	switch (statusByte) { 	 
	  case MIDI_SONGPOS: 	 
	    [aNote setPar:MK_songPosition toInt: 	 
	     cv14(ptr->_dataByte1, ptr->_dataByte2)];
	    break; 	 
	  case MIDI_TIMECODEQUARTER:
	    [aNote setPar:MK_timeCodeQ toInt:ptr->_dataByte1]; 	 
	    break; 	 
	  case MIDI_SONGSEL: 	 
	    [aNote setPar:MK_songSelect toInt:ptr->_dataByte1]; 	 
	    break; 	 
	  case MIDI_TUNEREQ:
	    [aNote setPar:MK_tuneRequest toInt:1];
	    break; 	 
	  default: 	 /* It's a system real time message */
	    [aNote setPar:MK_sysRealTime toInt:
	     (INT(MK_sysClock) + statusByte -  MIDI_CLOCK)];
	    break; 	
	} 
	break;
      case MIDI_CHANMODE: /* Can be a controller change as well. */ 	
	switch (MIDI_DATA(ptr->_dataByte1)) { 	  
	  case MIDI_LOCALCONTROL: 	    
	    [aNote setPar:MK_chanMode toInt:
 	     ((ptr->_dataByte2) ? MK_localControlModeOn : 
	      MK_localControlModeOff)]; 	    
	    [aNote setPar:MK_basicChan toInt:KITCHAN(statusByte)];
 	    ptr->chan = _MK_MIDISYS; 	    
	    break; 	  
	  case MIDI_RESETCONTROLLERS:
	    ptr->chan = _MK_MIDISYS; 	    
	    [aNote setPar:MK_chanMode toInt:MK_resetControllers];
	    break;
	  case MIDI_MONO: 	  
	    [aNote setPar:MK_monoChans toInt:ptr->_dataByte2]; 
	    /* No break */
	  case MIDI_OMNION: 	  
	  case MIDI_ALLNOTESOFF: 	  
	  case MIDI_OMNIOFF: 	  
	  case MIDI_POLY: 	
	    ptr->chan = _MK_MIDISYS; 	    
	    [aNote setPar:MK_chanMode toInt:
	     (INT(MK_allNotesOff) + MIDI_DATA(ptr->_dataByte1) -
	      MIDI_DATA(MIDI_ALLNOTESOFF))];
 	  default: /* It's a controller change. */
	    ptr->chan = KITCHAN(statusByte);
	    [aNote setPar:MK_controlChange toInt:ptr->_dataByte1];
	    [aNote setPar:MK_controlVal toInt:ptr->_dataByte2];
	    [aNote setNoteType:MK_noteUpdate];
	    break;
	}
	break;
      default:  /* Voice channel messages (except control change) */
	ptr->chan = KITCHAN(statusByte);
	switch (MIDI_OP(statusByte)) {
	  case MIDI_NOTEOFF:
	    keyNum = ptr->_dataByte1;
	    /* Note that we don't set the keynum in the note here. 
	       Noteoffs don't need a keyNum. It's in the noteTag. */
	    if ((ptr->_dataByte2 != 0) && (ptr->_dataByte2 != 64)) 
	      /* Omit relVelocity of 0. This is maybe not the right thing? 
	       * We omit relvelocties of 64, because this is the default you
	       * get anyway. 
	       */
	      [aNote setPar:MK_relVelocity toInt:ptr->_dataByte2];
	    [aNote setNoteType:MK_noteOff];
	    break;
	  case MIDI_NOTEON:
	    keyNum = ptr->_dataByte1;
	    if (ptr->_dataByte2) { /* A true note on? */
		[aNote setPar:MK_velocity toInt:ptr->_dataByte2];
		[aNote setPar:MK_keyNum toInt:keyNum];
		[aNote setNoteType:MK_noteOn];
	    } else [aNote setNoteType:MK_noteOff]; 
	    break;
	  case MIDI_POLYPRES:
	    keyNum = ptr->_dataByte1;
	    [aNote setNoteTag:NOTETAG(ptr->chan,keyNum)];
	    [aNote setPar:MK_keyPressure toInt:ptr->_dataByte2];
	    /* Note type is "upgraded" to noteUpdate by virtue of there
	       being a note tag here. */
	    break;
	  case MIDI_CHANPRES:
	    [aNote setPar:MK_afterTouch toInt:ptr->_dataByte1];
	    [aNote setNoteType:MK_noteUpdate];
	    break;
	  case MIDI_PROGRAM:
	    [aNote setPar:MK_programChange toInt:ptr->_dataByte1];
	    [aNote setNoteType:MK_noteUpdate];
	    break;
	  case MIDI_PITCH:
	    [aNote setPar:MK_pitchBend toInt:
	     cv14(ptr->_dataByte1, ptr->_dataByte2)];
	    [aNote setNoteType:MK_noteUpdate];
	    break;
	}
	break;
    }

#if _MK_MIDIIN_MULTIPLE_VOICES_ON_SAME_KEYNUM
    /* Normally commented out */

    /* Algorithm: We keep a bit vector to see if the note is on. Call it
       the _on bit vector.  If the note is off, we issue a noteOn on the
       default tag and set the on bit.  If the note is on, we create a new
       noteTag, add it to the tail of the duplicates list and set the
       noNoteOffsYet bit in the duplicates book-keeping bit vector.  If a
       noteOff comes, we look at the _on bit.  If it's off, we ignore
       the new note.  If it's on, we look if there's an entry in the
       duplicates list. If not, we send the noteOff with the default tag and
       clear the noteIsOn bit.  If there is an entry in the duplicates list,
       we look at the _defTagOffSent bit. If it's off, we clear it and issue a
       noteOff on the defalt noteTag. If it's on, we issue a noteOff on the
       tag that's the head of the noteTag list, free that record and set the 
       noteTags list to point to the next element, if any. */ 

    switch([aNote noteType]) {
      case MK_noteOn: {
	  int noteTag;
	  short chan = ptr->chan;
	  if (IS_OFF(ptr,chan,keyNum)) { /* Normal case */
	      SET_ON(ptr,chan,keyNum);
	      noteTag = NOTETAG(chan,keyNum);
	  }
	  else { /* Exceptional case: Multiple noteOns on same chan/key */
	      midiInList *l = findList(keyNum,ptr);
	      midiInNode *newN;
	      if (_MKTrace() & MK_TRACEMIDI) 
		fprintf(stderr,"Two noteOns on same keyNum without "
			"intervening noteOff.\n");
	      _MK_MALLOC(newN,midiInNode,1);
	      if (!l)
		l = newList(ptr,keyNum);
	      addToTailList(newN,l,noteTag = MKNoteTag());
	  }
	  [aNote setNoteTag:noteTag];
	  break;
      }
      case MK_noteOff: {
	  int noteTag;
	  midiInList *l;
	  short chan = ptr->chan;
	  if (IS_OFF(ptr,chan,keyNum)) { /* Throw it away. */
	      ptr->_note = nil;
	      [aNote free];            
	      return nil;
	  }
	  if (l = findList(keyNum,ptr)) {
	      /* Multiple noteOns on same chan/key? */
	      if (_MKTrace() & MK_TRACEMIDI) 
		fprintf(stderr,"NoteOff for multiply on keyNum.\n");
	      if (IS_DEFAULT_TAG_OFF_SENT(ptr,chan,keyNum))  
		/* Use element in list. */
		noteTag = removeHeadOfList(l,ptr,keyNum);
	      else { 
		  /* Use default tag */
		  noteTag = NOTETAG(chan,keyNum);
		  SET_DEFAULT_TAG_OFF_SENT(ptr,chan,keyNum);
	      }
	  }
	  else { /* Normal case */
	      noteTag = NOTETAG(chan,keyNum);
	      CLEAR_ON(ptr,chan,keyNum);
	  }
	  [aNote setNoteTag:noteTag];
	  break;
      }
      default:
	break;
    }
#else

    switch([aNote noteType]) {
      case MK_noteOn: {
	  int noteTag;
	  short chan = ptr->chan;
	  SET_ON(ptr,chan,keyNum);
	  noteTag = NOTETAG(chan,keyNum);
	  [aNote setNoteTag:noteTag];
	  break;
      }
      case MK_noteOff: {
	  int noteTag;
	  short chan = ptr->chan;
	  if (IS_OFF(ptr,chan,keyNum)) { /* Throw it away. */
	      ptr->_note = nil;
	      [aNote free];            
	      return nil;
	  }
	  noteTag = NOTETAG(chan,keyNum);
	  CLEAR_ON(ptr,chan,keyNum);
	  [aNote setNoteTag:noteTag];
	  break;
      }
      default:
	break;
    }

#endif

    return (id)aNote;
}





















