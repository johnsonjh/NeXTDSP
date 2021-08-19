#ifdef SHLIB
#include "shlib.h"
#endif

/*
  MidiIn.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  09/19/89/daj - Change to accomodate new way of doing parameters (structs 
                 rather than objects).
  01/06/90/daj - Added mergeInput option. Flushed flase cond comp.
                 Added some comments. 
  01/31/90/daj - Changed mergeInput to be an "extra var" and changed 
                 midiPorts struct to be a generic "extra var" struct. This
		 all in the interest of 1.0 backward header file compatability.
  02/25/90/daj - Moved putSysExclByte to writeMidi.
                 Changes to accomodate new way of doing midiFiles.
  03/13/90/daj - Added import of _NoteSender.h to accomodate new compiler.
  03/18/90/daj - Fixed bug in _open (needed DPSAddPort)		 
  03/19/90/daj - Changed to use MKGetNoteClass()
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  04/23/90/daj - Changes to make it a shlib and to make header files more
                 modular.
  07/24/90/daj - Changed to use _MKSprintf and _MKVsprintf for thread-safety
                 in a multi-threaded Music Kit performance.
*/

#import <mach_init.h>
#import <midi/midi_server.h>
#import <midi/midi_reply_handler.h>
#import <midi/midi_timer.h>
#import <midi/midi_error.h>
#import <midi/midi_timer_error.h>
#import <dpsclient/dpsclient.h>

/* Music Kit include files */
#import "_musickit.h"
#import "_tokens.h"
#import "_error.h"
#import "_ParName.h"
#import "_NoteSender.h"
#import "_midi.h"
#import "_time.h"
#import "_Conductor.h"
#import "_MKSprintf.h"

#import "Midi.h"
@implementation Midi:Object
/* MidiIn is made to look somewhat like a Performer. It differs from a
   performer, however, in that it responds not to messages sent by a
   conductor, but by midi input which arives through the serial port.

   Note that the Conductor must be clocked to use MidiIn.
   */
{
    id noteSenders;  
    id noteReceivers;
    MKDeviceStatus deviceStatus;
    char * midiDev;              /* Midi device port name. */
    BOOL useInputTimeStamps;
    BOOL outputIsTimed;
    double localDeltaT;
    void *_reservedMidi1;
    unsigned _reservedMidi2;
    void *_reservedMidi3;
    void *_reservedMidi4;
    double _reservedMidi5;
    char _reservedMidi6;
}

#define _extraVars _reservedMidi1
#define _ignoreBits _reservedMidi2
#define _pIn _reservedMidi3     
#define _pOut _reservedMidi4     
#define _timeOffset _reservedMidi5
#define _mode _reservedMidi6

#define MIDIDEV "midi1"

#define MIDIINPTR ((_MKMidiInStruct *)((Midi *)self)->_pIn)
#define MIDIOUTPTR ((_MKMidiOutStruct *)self->_pOut)
#define XVARS ((extraInstanceVars *)self->_extraVars)

#define VARIABLE 3

/* Mach stuff */

#import <objc/HashTable.h>
#import <mach_error.h>
#import <servers/netname.h>

typedef struct _extraInstanceVars {
    BOOL isOwner;
    port_name_t devicePort; /* Device port */
    port_name_t ownerPort;  
    port_name_t recvPort;   /* Port that deals with midiIn */
    port_name_t xmitPort;   /* Port that deals with midiOut */
    port_name_t timingPort; 
    port_name_t replyPort;   /* Port on which we receive midiIn messages
				and 'queue has room' messages from midiOut */
    BOOL mergeInput;
} extraInstanceVars;

static int deallocPort(portP)
    port_name_t *portP;
{
    int ec;
    if (*portP != PORT_NULL) {
	ec = port_deallocate(task_self(), *portP);
	if (ec != KERN_SUCCESS) {
	    _MKErrorf(MK_machErr,"Can't close Midi",mach_error_string(ec));
	    return ec;
	}
	*portP  = PORT_NULL;
    }
    return KERN_SUCCESS;
}

static id portTable = nil;

static int closeMidiDev(Midi *self,extraInstanceVars *ports)
{
    int r;
    if (ports->isOwner)  /* Gk says ok to dealloc here even if not owner */
      deallocPort(&ports->ownerPort);
    r = deallocPort(&ports->replyPort);
    /* Just being paranoic: */
    ports->ownerPort = PORT_NULL;
    ports->devicePort = PORT_NULL;
    ports->ownerPort = PORT_NULL; 
    ports->recvPort = PORT_NULL;
    ports->xmitPort = PORT_NULL;
    ports->timingPort = PORT_NULL;
    return KERN_SUCCESS;
}

#define INPUTENABLED(_x) (_x != 'o')
#define OUTPUTENABLED(_x) (_x != 'i')

static int openMidiDev(Midi *self,
		       extraInstanceVars *ports,
		       port_name_t ownerPort)
    /* "Opens". If the device represented by devicePortName is already 
       accessed by this task, uses the ownerPort currently accessed.
       Otherwise, if ownerPort is PORT_NULL, allocates a new
       port. Otherwise, uses ownerPort as specified. 
       To make the device truly public, you can pass the device port as the
       owner port. As is, we pass the owner port as the negotiation port
       so others can share device. 

       When (if) a MidiManager is written, the devicePortName needs to 
       be the name of the MidiManger (I believe). */

{
    port_name_t tmp;
    int r;
    static const char *const problemMsg = 
      "Problem opening Midi device";
    port_name_t negPort;
    r = netname_look_up(name_server_port, "", self->midiDev,
			&(ports->devicePort));
    if (r != KERN_SUCCESS) {
	ports->devicePort = PORT_NULL;
	_MKErrorf(MK_machErr,"Can't find Midi device",mach_error_string( r));
	return r;
    }
    if (ownerPort == PORT_NULL) {
	r = port_allocate(task_self(), &ports->ownerPort);
	if (r != KERN_SUCCESS) {
	    _MKErrorf(MK_machErr,"Can't become owner of Midi device",
		      mach_error_string( r));
	    return r;
	}
    } 
    else ports->ownerPort = ownerPort;
    tmp = ports->ownerPort;
    negPort = ports->ownerPort; /* Grant rights to others */
    r = midi_set_owner(ports->devicePort, 
		       ports->ownerPort,
		       &negPort);
    
    ports->isOwner = (negPort == ports->ownerPort);
    if (!ports->isOwner) {           /* Someone already owns it */
	if (ownerPort == PORT_NULL)
	  deallocPort(&tmp);         /* Release one we grabbed before */
	ports->ownerPort = negPort;
    }

    r = port_allocate(task_self(), &ports->replyPort);
    if (r != KERN_SUCCESS) {
	_MKErrorf(MK_machErr,problemMsg,mach_error_string( r));
	closeMidiDev(self,ports);
	return r;
    }
    
    /* Input */
    if (INPUTENABLED(self->_mode)) {
	r = midi_get_recv(ports->devicePort,
			  ports->ownerPort,
			  &(ports->recvPort));
	if (r != KERN_SUCCESS) {
	    _MKErrorf(MK_machErr,problemMsg,midi_error_string( r));
	    closeMidiDev(self,ports);
	    return r;
	}
    }

    /* Output */
    if (OUTPUTENABLED(self->_mode)) {
	r = midi_get_xmit(ports->devicePort,
			  ports->ownerPort,
			  &(ports->xmitPort));
	if (r != KERN_SUCCESS) {
	    _MKErrorf(MK_machErr,problemMsg,midi_error_string( r));
	    closeMidiDev(self,ports);
	    return r;
	}
    }

    r = midi_get_in_timer_port(((INPUTENABLED(self->_mode)) ? ports->recvPort :
				ports->xmitPort),
			       &(ports->timingPort));
    if (r != KERN_SUCCESS) {
	_MKErrorf(MK_machErr,problemMsg,midi_error_string( r));
	closeMidiDev(self,ports);
	return r;
    }

#   define INTER_MSG_QUANTA 2 /* Assuming millisecond quanta */
#   define MSG_FRAME_QUANTA 10

    if (INPUTENABLED(self->_mode)) { /* Input prototype */
	r = midi_set_proto(ports->recvPort, 
			   MIDI_PROTO_RAW,
			   (boolean_t)0, /* Use absolute time stamps. 
					    (relative == false) */
			   0,            /* System clock source */
			   INTER_MSG_QUANTA,
			   MSG_FRAME_QUANTA,
			   0             /* Use default queue size. */
			   );
	if (r != KERN_SUCCESS) {
	    _MKErrorf(MK_machErr,problemMsg,midi_error_string( r));
	    closeMidiDev(self,ports);
	    return r;
	}
    }
    if (OUTPUTENABLED(self->_mode)) { /* Output prototype */
	r = midi_set_proto(ports->xmitPort, 
			   MIDI_PROTO_RAW,
			   (boolean_t)0, /* Use absolute time stamps. 
					    (relative == false) */
			   0,            /* System clock source */
			   INTER_MSG_QUANTA,
			   MSG_FRAME_QUANTA,
			   0             /* Use default queue size. */
			   );
	if (r != KERN_SUCCESS) {
	    _MKErrorf(MK_machErr,problemMsg,midi_error_string( r));
	    closeMidiDev(self,ports);
	    return r;
	}
    }
    return KERN_SUCCESS;
}    

static kern_return_t my_queue_notify(void *arg,
				     u_int queue_size)
    /* Function called when queue is empty */
{
    *((BOOL *)arg) = YES;
    return KERN_SUCCESS;
}

static void awaitMidiOutDone(extraInstanceVars *ports)
    /* Wait until Midi is done and then return */
{
    midi_reply_t recvStruct = { /* Tells driver funcs to call */
	NULL,NULL,NULL,my_queue_notify,0,0};
    int r;
    BOOL queueEmpty = NO;
    union midi_reply_request msg;
    r = midi_output_queue_notify(ports->devicePort,ports->ownerPort,
				 ports->replyPort,0);
    if (r != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Problem awaiting Midi queue to finish",
		midi_error_string(r));

    /* Ask for a message when queue gets to 0. */
    recvStruct.arg = (void *)&queueEmpty;
    msg.midi_queue_notify.Head.msg_size = MIDI_REPLY_INMSG_SIZE;
    
    do {
	msg.midi_queue_notify.Head.msg_local_port = ports->replyPort;
	msg_receive((msg_header_t *)&msg,
		    (msg_option_t)MSG_OPTION_NONE,
		    (msg_timeout_t)0); /* Blocks until message */
	midi_reply_handler(&(msg.midi_queue_notify.Head),&recvStruct);
	/* This calls my routine if it's the right kind of msg */
    } while (!queueEmpty);
}

static int stopMidiClock(extraInstanceVars *ports)
{
    int r = timer_stop(ports->timingPort,ports->ownerPort);
    if (r != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't stop Midi clock",midi_timer_error_string(r));
    return r;
}

static int resumeMidiClock(extraInstanceVars *ports)
{
    int r = timer_start(ports->timingPort,ports->ownerPort);
    if (r != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't start Midi clock",
		midi_timer_error_string(r));
    return r;
}

static int resetAndStopMidiClock(extraInstanceVars *ports)
{
    timeval_t zeroTime = {0};
    int r;
    stopMidiClock(ports);
    r = timer_set(ports->timingPort,ports->ownerPort,zeroTime);
    if (r != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't reset Midi clock",
		midi_timer_error_string(r));
    return r;
}

static int emptyMidi(port_name_t aPort)
    /* Get rid of enqueued outgoing midi messages */
{
    int r;
    r = midi_clear_queue(aPort);
    if (r != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't empty Midi queue",midi_error_string(r));
    return r;
}

static int setMidiSysIgnore(extraInstanceVars *ports,unsigned bits)
    /* Tell driver to ignore particular incoming MIDI system messages */
{
    int r = midi_set_sys_ignores(ports->recvPort, bits);
    if (r != KERN_SUCCESS) 
      _MKErrorf(MK_machErr,"Can't ignore requested Midi messages",
		midi_error_string( r));
    return r;
}


/* Low-level output routines */

/* We currently use MIDI "raw" mode. Perhaps cooked mode would be more
   efficient? */

#define MIDIBUFSIZE 128

static struct midi_raw midiBuf[MIDIBUFSIZE];
static struct midi_raw *bufPtr = &(midiBuf[0]);

static void putTimedByte(unsigned curTime,unsigned char aByte)
    /* output a MIDI byte */
{
    bufPtr->quanta = curTime;
    bufPtr->data = aByte;
    bufPtr++;
}

static void sendBufferedData(struct __MKMidiOutStruct *ptr); /* forward decl*/

static void putTimedByteWithCheck(struct __MKMidiOutStruct *ptr,
				  unsigned curTime,unsigned char aByte)
    /* Same as above, but checks for full buffer */
{
    if ((&(midiBuf[MIDIBUFSIZE])) == bufPtr) 
      sendBufferedData(ptr);
    putTimedByte(curTime,aByte);
}

/*** FIXME Should really get this, as well as the absolute vs. relative
  decision from the device itself. ***/

#define QUANTUM 1000 /* 1 ms */
#define QUANTUM_PERIOD ((double)(1.0/((double)QUANTUM)))

static void sendBufferedData(struct __MKMidiOutStruct *ptr)
    /* Send any buffered bytes and reset pointer to start of buffer */
{
    int r;
    int nBytes;
    nBytes = bufPtr - &(midiBuf[0]);
    r = midi_send_raw_data(((extraInstanceVars *)
			    (((Midi *)ptr->_owner)->_extraVars))->xmitPort,
			   &(midiBuf[0]),nBytes,FALSE);
    if (r != KERN_SUCCESS) 
      _MKErrorf(MK_machErr,"Can't send Midi output data",
		midi_error_string(r));
    bufPtr = &(midiBuf[0]);
}

static void putMidi(struct __MKMidiOutStruct *ptr)
    /* Adds a complete MIDI message to the output buffer */
{
    unsigned int curTime = .5 + ptr->_timeTag * QUANTUM;
    if (((&(midiBuf[MIDIBUFSIZE])) - bufPtr) < ptr->_outBytes)
      sendBufferedData(ptr);
    putTimedByte(curTime,ptr->_bytes[0]);
    if (ptr->_outBytes >= 2)
      putTimedByte(curTime,ptr->_bytes[1]);
    if (ptr->_outBytes == 3)
      putTimedByte(curTime,ptr->_bytes[2]);
}


static void putSysExcl(struct __MKMidiOutStruct *ptr,char *sysExclStr)
{
    /* sysExStr is a string. The string consists of system exclusive bytes
	separated by any non-digit delimiter. The musickit uses the 
	delimiter ','. E.g. "f8,13,f7".  This function converts each ASCII
	byte into the corresponding number and sends it to serial port.
       Note that if you want to give each sysex byte a different
       delay, you need to do a separate call to this function.
       On a higher level, this means that you need to put each
       byte in a different Note object. 
	The string may but need not begin with MIDI_SYSEXCL and end with
	MIDI_EOX. 
       */
    unsigned char c;
    unsigned int curTime = .5 + ptr->_timeTag * QUANTUM;
    sendBufferedData(ptr);
    c = _MKGetSysExByte(&sysExclStr);
    if (c == MIDI_EOX)
      return;
    if (c != MIDI_SYSEXCL) 
      putTimedByte(curTime,MIDI_SYSEXCL);
    putTimedByte(curTime,c);
    while (*sysExclStr) {
	c = _MKGetSysExByte(&sysExclStr);
	putTimedByteWithCheck(ptr,curTime,c);
    }
    if (c != MIDI_EOX) 
      putTimedByteWithCheck(ptr,curTime,MIDI_EOX);  /* Terminate it properly */
}

/* Midi parsing. */

/* Currently we use raw input mode. That means we have to parse the MIDI
   ourselves. Perhaps it'd be more efficient to let the driver do the
   parsing (i.e. use the driver's "cooked" mode). But for 1.0, the driver 
   was finished so late, I was afraid to trust its hardly-debugged code. -
   DAJ */

static unsigned char parseMidiStatusByte(statusByte,ptr)
    unsigned char statusByte;
    _MKMidiInStruct *ptr;
    /* This is called when a status byte is found. Returns YES if the status
       byte is a system real time or system exclusive message. */
{
    switch (MIDI_OP(statusByte)) {
      case MIDI_PROGRAM: 
      case MIDI_CHANPRES:
	ptr->_statusByte = ptr->_runningStatus = statusByte;
	ptr->_dataBytes = 1;
	return 0;
      case MIDI_NOTEON:
      case MIDI_NOTEOFF:
      case MIDI_POLYPRES:
      case MIDI_CONTROL:
      case MIDI_PITCH:
	ptr->_statusByte = ptr->_runningStatus = statusByte;
	ptr->_dataBytes = 2;
	ptr->_firstDataByteSeen = NO;
	return 0;
      case MIDI_SYSTEM:
	if (!(statusByte & MIDI_SYSRTBIT)) {
	    ptr->_runningStatus = 0;
	    ptr->_statusByte = statusByte;
	    switch (statusByte) {
	      case MIDI_SONGPOS:
		ptr->_dataBytes = 2;
		ptr->_firstDataByteSeen = NO;
		return 0;
	      case MIDI_TIMECODEQUARTER:
	      case MIDI_SONGSEL:
		ptr->_dataBytes = 1;
		return 0;
	      case MIDI_SYSEXCL:
		ptr->_dataBytes = VARIABLE;
		return MIDI_SYSEXCL;
	      case MIDI_TUNEREQ:         
		ptr->_dataBytes = 0;
		return MIDI_TUNEREQ;
	      case MIDI_EOX: {          
		  BOOL isInSysEx = (ptr->_dataBytes == VARIABLE);
		  ptr->_dataBytes = 0;
		  return (isInSysEx) ? MIDI_SYSEXCL : 0;
	      }
	    }
	}
	else switch (statusByte) {
	  case MIDI_CLOCK:            /* System real time messages. */
	  case MIDI_START:
	  case MIDI_STOP:
	  case MIDI_ACTIVE:
	  case MIDI_RESET:
	  case MIDI_CONTINUE:
	    return statusByte; /* Doesn't affect running status. */
	                       /* Also doesn't affect _dataBytes. This
				  is because real-time messages may occur
				  anywhere, even in a system exclusive 
				  message. */
	  default:             /* Omit unrecognized status. */
	    return 0;         
	}                      
      default:                 /* Garbage */
	ptr->_dataBytes = 0;
	return 0;             
    }   
}

static unsigned char parseMidiByte(aByte,ptr)
    unsigned char aByte;
    _MKMidiInStruct *ptr;
    /* Takes an incoming byte and parses it */
{
    if (MIDI_STATUSBIT & aByte)  
      return parseMidiStatusByte(aByte,ptr);
    switch (ptr->_dataBytes) {
      case 0:                      /* Running status or garbage */
	if (!ptr->_runningStatus)  /* Garbage */
	  return 0;
	parseMidiStatusByte(ptr->_runningStatus,ptr);
	return parseMidiByte(aByte,ptr);
      case 1:                      /* One-argument midi message. */
	ptr->_dataByte1 = aByte;
	ptr->_dataBytes = 0;  /* Reset */
	return ptr->_statusByte;
      case 2:                      /* Two-argument midi message. */
	if (ptr->_firstDataByteSeen) {
	    ptr->_dataByte2 = aByte;
	    ptr->_dataBytes = 0;
	    return ptr->_statusByte;
	}
	ptr->_dataByte1 = aByte;
	ptr->_firstDataByteSeen = YES;
	return 0;
      case VARIABLE:
	return MIDI_SYSEXCL;
      default:
	return 0;
    }
}

static id handleSysExclbyte(_MKMidiInStruct *ptr,unsigned char midiByte)
    /* Paresing routine for incoming system exclusive */
{
    unsigned j;
#   define DEFAULTLEN 200
    if (midiByte == MIDI_SYSEXCL) {  /* It's a new one. */
	if (!ptr->_sysExBuf) {
	    _MK_MALLOC(ptr->_sysExBuf,char,DEFAULTLEN);
	} else _MK_REALLOC(ptr->_sysExBuf,char,DEFAULTLEN);
	ptr->_sysExSize = DEFAULTLEN;
	ptr->_endOfSysExBuf = ptr->_sysExBuf + DEFAULTLEN;
	ptr->_sysExP = ptr->_sysExBuf;
	_MKSprintf(ptr->_sysExP,"%-2x",j = midiByte);
	ptr->_sysExP += 2;
    }
    else {
	if ((ptr->_sysExP + 3) >= ptr->_endOfSysExBuf) { 
	    /* Leave room for \0 */
	    int offset = ptr->_sysExP - ptr->_sysExBuf; 
	    ptr->_sysExSize *= 2;
	    _MK_REALLOC(ptr->_sysExBuf,char,ptr->_sysExSize);
	    ptr->_endOfSysExBuf = ptr->_sysExBuf + ptr->_sysExSize;
	    ptr->_sysExP = ptr->_sysExBuf + offset;
	}
	_MKSprintf(ptr->_sysExP,",%-2x",j = midiByte);
	ptr->_sysExP += 3;
    }
    if (midiByte == MIDI_EOX) {
	*ptr->_sysExP = '\0';
	[ptr->_note free]; /* Free old note. */ 
	ptr->_note = [MKGetNoteClass() newSetTimeTag:ptr->timeTag];
	[ptr->_note setPar:MK_sysExclusive toString:ptr->_sysExBuf];
	ptr->chan = _MK_MIDISYS; 	
	return (id)ptr->_note;
    }
    return nil; /* We're not done yet. */
} 

/* The following functions are invoked by the driver. */
static kern_return_t ret_cooked_data(selfId, midi_cooked_data, 
				   midi_cooked_dataCnt)
    void *selfId;
    midi_cooked_t midi_cooked_data;
    unsigned int midi_cooked_dataCnt;
    /* This must be included, but need not do anything. */
{
    return KERN_SUCCESS;
}

static kern_return_t ret_packed_data(selfId, quanta, midi_packed_data, 
				   midi_packed_dataCnt)
    void *selfId;
    u_int quanta;
    midi_packed_t midi_packed_data;
    unsigned int midi_packed_dataCnt;
    /* This must be included, but need not do anything. */
{
    return KERN_SUCCESS;
}

static kern_return_t ret_raw_data(selfId, midi_raw_data, midi_raw_dataCnt)
    void *selfId;
    midi_raw_t midi_raw_data;
    unsigned int midi_raw_dataCnt;
    /* Since we use raw input, here's where the action is. */
{
#   define self ((Midi *)selfId) 
    _MKMidiInStruct *ptr = MIDIINPTR;
    id aNote;
    unsigned char statusByte;
    for (; midi_raw_dataCnt--; midi_raw_data++) {
	if (statusByte = parseMidiByte(midi_raw_data->data, ptr)) {
	    if (statusByte == MIDI_SYSEXCL) 
	      aNote = handleSysExclbyte(ptr,midi_raw_data->data);
	    else 
	      aNote = _MKMidiToMusicKit(ptr,statusByte);
	    if (aNote) {
		if (self->useInputTimeStamps) {
		    double t;
		    t = (((double) midi_raw_data->quanta) * 
			 QUANTUM_PERIOD + self->_timeOffset);
		    _MKAdjustTime(t); /* Use input time stamp time */
		}
		else [_MKClassConductor() adjustTime]; /* Use our time */
		if (XVARS->mergeInput) { /* Send all on one NoteSender? */
		    MKSetNoteParToInt(aNote,MK_midiChan,ptr->chan);
		    [NX_ADDRESS(self->noteSenders)[0] sendNote:aNote];
		}
		else [NX_ADDRESS(self->noteSenders)[ptr->chan] sendNote:aNote];
		[_MKClassOrchestra() flushTimedMessages]; /* Off to the DSP */
	    }
	}
    }
    return KERN_SUCCESS;
#   undef self
}

static void midiIn(msg_header_t *msg,void *self)
    /* Called by driver when midi input occurs. */
{
    int r;
    static midi_reply_t recvStruct = { /* Tells driver funcs to call */
	ret_raw_data,ret_cooked_data,ret_packed_data,NULL,0,0};
    extraInstanceVars *ports = ((Midi *)self)->_extraVars;
    recvStruct.arg = self;
    r = midi_reply_handler(msg,&recvStruct);        /* This gets data */
    if (r != KERN_SUCCESS) 
      _MKErrorf(MK_machErr,"Problem receiving Midi input data",
		midi_error_string(r));
    r = midi_get_data((midi_rx_t)ports->recvPort,  /* Register for nxt msg */
		      (port_t)ports->replyPort);
    if (r != KERN_SUCCESS) 
      _MKErrorf(MK_machErr,"Problem receiving Midi input data",
		midi_error_string(r));
} 

/* Input configuration */

#import <objc/hashtable.h>

-setUseInputTimeStamps:(BOOL)yesOrNo
{
    if (deviceStatus != MK_devClosed)
      return nil;
    useInputTimeStamps = yesOrNo;
    return self;
}

-(BOOL)useInputTimeStamps
{
    return useInputTimeStamps;
}

  
static unsigned ignoreBit(unsigned param)
{
    switch (param) {
      case MK_sysActiveSensing:
	return MIDI_IGNORE_ACTIVE_SENS;
      case MK_sysClock:
	return MIDI_IGNORE_TIMING_CLCK;
      case MK_sysStart:
	return MIDI_IGNORE_START;
      case MK_sysContinue:
	return MIDI_IGNORE_CONTINUE;
      case MK_sysStop:
	return MIDI_IGNORE_STOP;
      default:
	break;
    }
    return 0;
}

- ignoreSys:(MKMidiParVal)param
{
    _ignoreBits |= ignoreBit(param);
    if (deviceStatus != MK_devClosed)
      setMidiSysIgnore(_extraVars,_ignoreBits);
    return self;
} 

- acceptSys:(MKMidiParVal)param 
{
    _ignoreBits &= ~(ignoreBit(param));
    if (deviceStatus != MK_devClosed)
      setMidiSysIgnore(_extraVars,_ignoreBits);
    return self;
}

/* Performer-like methods. */

-conductor
{
    return [_MKClassConductor() clockConductor];
}


/* Creation */

-_free
  /* Needed below */
{
    return [super free];
}

+allocFromZone:(NXZone *)zone onDevice:(char *)devName
{
    Midi *obj;
    if (!portTable)
      portTable = [HashTable newKeyDesc:"*" valueDesc:"@"];
    devName = (char *)NXUniqueString(devName); /* Uniquify for ptr compare */
    obj = [portTable valueForKey:(const char *)devName];
    if (obj)         /* Already exists */
      return obj;
    obj = [super allocFromZone:zone];
    [portTable insertKey:(const char *)devName value:(char *)obj];
    obj->midiDev = devName;
    return obj;
}

+allocFromZone:(NXZone *)zone
{
    return [self allocFromZone:zone onDevice:MIDIDEV];
}

+alloc
{
    return [self allocFromZone:NXDefaultMallocZone() onDevice:MIDIDEV];
}

+newOnDevice:(char *)devName
{
    self = [self allocFromZone:NXDefaultMallocZone() onDevice:devName];
    [self init];
    return self;
}

-init
  {
    id aNoteSender,aNoteReceiver;
    int i;
    _MKParameter *aParam;
    if (noteSenders) /* Already initialized */
      return nil;
    outputIsTimed = YES;               /* Default is outputIsTimed */
    noteSenders = [List newCount:_MK_MIDINOTEPORTS];
    noteReceivers = [List newCount:_MK_MIDINOTEPORTS];
    for (i = 0; i < _MK_MIDINOTEPORTS; i++) {
	aNoteReceiver = [NoteReceiver new];
	[noteReceivers addObject:aNoteReceiver];
	[aNoteReceiver _setOwner:self];
	[aNoteReceiver _setData:aParam = _MKNewIntPar(0,MK_noPar)];
	_MKSetIntPar(aParam,i);
	aNoteSender = [NoteSender new];
	[noteSenders addObject:aNoteSender];
	[aNoteSender _setPerformer:self];
    }
    useInputTimeStamps = YES;
    _timeOffset = 0;
    _ignoreBits = (MIDI_IGNORE_ACTIVE_SENS |
			   MIDI_IGNORE_TIMING_CLCK |
			   MIDI_IGNORE_START |
			   MIDI_IGNORE_CONTINUE |
			   MIDI_IGNORE_STOP);
    deviceStatus = MK_devClosed;
    _MK_CALLOC(_extraVars,extraInstanceVars,1);
    _MKCheckInit(); /* Maybe we don't want this here, in case we ever want
		       to use Midi without Notes. (?) FIXME */ 
    if (_MKClassOrchestra()) /* Force find-class here */
      ;
    _mode = 'a';
    return self;
}

+new
{
    return [self newOnDevice:MIDIDEV];
}

-free
  /* Aborts and frees the receiver. */
{
    int i,size = [noteReceivers count];
    [self abort];
    for (i=0; i<size; i++) 
      _MKFreeParameter([NX_ADDRESS(noteReceivers)[i] _getData]);
    [noteReceivers makeObjectsPerform:@selector(disconnect)];
    [noteReceivers freeObjects];  
    [noteReceivers free];
    [noteSenders makeObjectsPerform:@selector(disconnect)];
    [noteSenders freeObjects];  
    [noteSenders free];
    [portTable removeKey:midiDev];
    NX_FREE(_extraVars);
    [super free];
    return nil;
}

/* Control of device */

-(MKDeviceStatus)deviceStatus
  /* Returns MKDeviceStatus of receiver. */
{
    return deviceStatus;
}

static id openMidi(Midi *self)
{
    if (openMidiDev(self,XVARS,PORT_NULL) != KERN_SUCCESS)
      return nil;
    if (INPUTENABLED(self->_mode))
      if (!(self->_pIn = (void *)_MKInitMidiIn()))
	return nil;
      else setMidiSysIgnore(XVARS,self->_ignoreBits);
    if (OUTPUTENABLED(self->_mode)) {
	if (!(self->_pOut = (void *)_MKInitMidiOut()))
	  return nil;
	{
	    _MKMidiOutStruct *p = self->_pOut;
	    p->_owner = self;
	    p->_putSysMidi = putMidi;
	    p->_putChanMidi = putMidi;
	    p->_putSysExcl = putSysExcl;
	    p->_sendBufferedData = sendBufferedData;
	}
    }
    resetAndStopMidiClock(XVARS);
    self->deviceStatus = MK_devOpen;
    return self;
}

-_open
{
    switch (deviceStatus) {
      case MK_devClosed: /* Need to open it */
	return openMidi(self);
      case MK_devOpen:
	break;
      case MK_devRunning:
	_MKRemovePort(XVARS->replyPort);
	/* no break here */
      case MK_devStopped:
	if (INPUTENABLED(_mode))
	  emptyMidi(XVARS->recvPort);
	if (OUTPUTENABLED(_mode))
	  emptyMidi(XVARS->xmitPort);
	resetAndStopMidiClock(XVARS);
	deviceStatus = MK_devOpen;
	break;
      default:
	break;
    }
    return self;
}

-openOutputOnly
  /* Same as open but does not enable output. */
{
    if ((deviceStatus != MK_devClosed) && (_mode != 'o'))
      [self close];
    _mode = 'o';
    return [self _open];
}

-openInputOnly
{
    if ((deviceStatus != MK_devClosed) && (_mode != 'i'))
      [self close];
    _mode = 'i';
    return [self _open];
}

-open
  /* Opens device if not already open.
     If already open, flushes output queue. 
     Sets deviceStatus to MK_devOpen. 
     Returns nil if failure.
     */
{
    if ((deviceStatus != MK_devClosed) && (_mode != 'a'))
      [self close];
    _mode = 'a';
    return [self _open];
}

-(double)localDeltaT
{
    return localDeltaT;
}

-setLocalDeltaT:(double)value
{
    localDeltaT = value;
    return self;
}

-run
{
    int r;
    switch (deviceStatus) {
      case MK_devClosed:
	if (!openMidi(self))
	  return nil;
	/* no break here */
      case MK_devOpen:
/*	doDeltaT(self);  Needed if we'd ever use relative time to the driver */
	_timeOffset = MKGetTime();
	/* This is needed by MidiOut. */
      case MK_devStopped:
	if (INPUTENABLED(_mode)) {
	    _MKAddPort(XVARS->replyPort,midiIn,MSG_SIZE_MAX,self,
		       _MK_DPSPRIORITY);
	    r = midi_get_data(XVARS->recvPort,PORT_NULL);
	    if (r != KERN_SUCCESS) 
	      _MKErrorf(MK_machErr,"Problem receiving Midi input data",
			midi_error_string(r));
	    /* This throws away stuff that's come in while we were Stopped. 
	       More precisely, this will cause the driver to format all input 
	       data into a message, and then discard it. */
	    r = midi_get_data(XVARS->recvPort,XVARS->replyPort);
	    if (r != KERN_SUCCESS) 
	      _MKErrorf(MK_machErr,"Problem receiving Midi input data",
			midi_error_string(r));
	}
	/* This says 'send us a message when data comes'. */ 
	resumeMidiClock(XVARS);
	deviceStatus = MK_devRunning;
      default:
	break;
    }
    return self;
}

-stop
{
    switch (deviceStatus) {
      case MK_devClosed:
	return [self open];
      case MK_devOpen:
      case MK_devStopped:
	return self;
      case MK_devRunning:
	stopMidiClock(XVARS);
	_MKRemovePort(XVARS->replyPort);
	deviceStatus = MK_devStopped;
      default:
	break;
    }
    return self;
}

-abort
{
    switch (deviceStatus) {
      case MK_devClosed:
	break;
      case MK_devRunning:
	_MKRemovePort(XVARS->replyPort);
	/* No break here */
      case MK_devStopped:
      case MK_devOpen:
	if (OUTPUTENABLED(_mode))
	  emptyMidi(XVARS->xmitPort);
	if (INPUTENABLED(_mode))
	  emptyMidi(XVARS->recvPort);
	_pIn = (void *)_MKFinishMidiIn(MIDIINPTR);
	_pOut = (void *)_MKFinishMidiOut(MIDIOUTPTR);
	closeMidiDev(self,XVARS);
	deviceStatus = MK_devClosed;
    }
    return self;
}

-close
  /* Need to ask for a message when queue is empty and wait for that message.
     */
{
    switch (deviceStatus) {
      case MK_devClosed:
	break;
      case MK_devRunning:
	if (INPUTENABLED(_mode)) 
	  _MKRemovePort(XVARS->replyPort);
	/* No break here */
      case MK_devStopped:
      case MK_devOpen:
	if (INPUTENABLED(_mode)) {
	    emptyMidi(XVARS->recvPort);
	    _pIn = (void *)_MKFinishMidiIn(MIDIINPTR);
	}
	if (OUTPUTENABLED(_mode)) {
	    awaitMidiOutDone(XVARS);
	    emptyMidi(XVARS->xmitPort); /* Shouldn't be needed */
	    _pOut = (void *)_MKFinishMidiOut(MIDIOUTPTR);
	}
	closeMidiDev(self,XVARS);
	deviceStatus = MK_devClosed;
    }
    return self;
}

/* output configuration */

-setOutputTimed:(BOOL)yesOrNo
/* Controls whether MIDI commands are sent timed or untimed. The default
   is timed. It is permitted to change
   from timed to untimed during a performance. */
{
    outputIsTimed = yesOrNo;
    return self;
}

-(BOOL)outputIsTimed
  /* Returns whether MIDI commands are sent timed. */
{
    return outputIsTimed;
}


/* Receiving noets */

-_realizeNote:aNote fromNoteReceiver:aNoteReceiver
    /* Performs note by converting it to midi and emiting it. 
       Is careful about matching noteOns with noteOffs. For
       notes of type MK_noteDur, schedules up a message to
       self, implementing the noteOff. If the receiver is not in devRunning
       status, aNote is ignored. */
{
    double t;
    int chan;
    if ((!MIDIOUTPTR) || (!aNote) || (deviceStatus != MK_devRunning))
      return nil;
    t = (outputIsTimed) ? (MKGetDeltaTTime() - _timeOffset + localDeltaT):0.0;
    chan = _MKParAsInt([aNoteReceiver _getData]);
    _MKWriteMidiOut(aNote,t,chan,MIDIOUTPTR,aNoteReceiver);
    return self;
}

/* Accessing NoteSenders and NoteReceivers */

-channelNoteSender:(unsigned)n
  /* Returns the NoteSender corresponding to the specified channel or nil
     if none. If n is 0, returns the NoteSender used for Notes fasioned
     from midi channel mode and system messages. */
{ 
    return (n > MIDI_NUMCHANS) ? nil : [noteSenders objectAt:n];
}

-channelNoteReceiver:(unsigned)n
  /* Returns the NoteReceiver corresponding to the specified channel or nil
     if none. If n is 0, returns the NoteReceiver used for Notes fasioned
     from midi channel mode and system messages. */
{ 
    return (n > MIDI_NUMCHANS) ? nil : [noteReceivers objectAt:n];
}

-noteSenders
  /* TYPE: Processing
   * Returns a copy of the receiver's NoteSender List. 
   * It is the sender's responsibility to free the List.
   */
{
    return _MKCopyList(noteSenders);
}


-noteSender
  /* Returns the default NoteSender. This is used when you don't care
     which NoteSender you get. */
{
    return [noteSenders objectAt:0];
}

- noteReceivers	
  /* TYPE: Querying; Returns a copy of the List of NoteReceivers.
   * Returns a copy of the List of NoteReceivers. The NoteReceivers themselves
   * are not copied.	
   */
{
    return _MKCopyList(noteReceivers);
}

-noteReceiver
  /* TYPE: Querying; Returns the receiver's first NoteReceiver.
   * Returns the first NoteReceiver in the receiver's List.
   * This is particularly useful for Instruments that have only
   * one NoteReceiver.
   */
{
    return [noteReceivers objectAt:0];
}

-setMergeInput:(BOOL)yesOrNo
{
    XVARS->mergeInput = yesOrNo;
    return self;
}

@end

