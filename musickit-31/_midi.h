/*  Modification history:

    daj/04/23/90 - Created from _musickit.h 

*/

#define _MK_MIDIIN_MULTIPLE_VOICES_ON_SAME_KEYNUM 0

typedef struct __MKMidiInStruct { /* This is the midi input structure. */
    double timeTag;     /* Current time */
    int _noteTags[MIDI_NUMCHANS]; /* Base for each channel */
    unsigned short _on[MIDI_NUMKEYS];   /* Bit vectors, one per key */
    short chan;          /* Channel of note, or _MK_MIDISYS. */ 
    /* The following is the midi parse state. */
    unsigned char _dataByte1,_dataByte2,_statusByte,_runningStatus;
    BOOL _firstDataByteSeen;  
    short _dataBytes;  
    id _note;        /* The note owned by the midi input. */
    int _sysExSize;    /* For collecting system exclusive bytes. */
    char * _sysExBuf;
    char *_endOfSysExBuf;
    char *_sysExP;
#   ifdef _MK_MIDIIN_MULTIPLE_VOICES_ON_SAME_KEYNUM
    unsigned short _defTagOffSent[MIDI_NUMKEYS]; 
    void *_tagLists[MIDI_NUMCHANS];/* One list per chan */ 
#   endif
} _MKMidiInStruct;

typedef struct __MKMidiOutStruct { /* Midi output structure */
    id _owner;         /* Object owning this struct */
    double _timeTag;   /* Current timeTag. */
    void (*_putSysMidi)(struct __MKMidiOutStruct *ptr);
    void (*_putChanMidi)(struct __MKMidiOutStruct *ptr);
    void (*_putSysExcl)(struct __MKMidiOutStruct *ptr,char *sysExclStr);
    void (*_sendBufferedData)(struct __MKMidiOutStruct *ptr);
    unsigned char _bytes[3];
    short _outBytes;
    unsigned char _maxCount[MIDI_NUMCHANS][MIDI_NUMKEYS];
    unsigned char _curCount[MIDI_NUMCHANS][MIDI_NUMKEYS];
    /* Used to stack noteOns against noteOffs. */
    id _map[MIDI_NUMCHANS];
    /* Implements conversion from music kit to MIDI semantics. */
    unsigned char _runningStatus;
    void *_midiFileStruct;
} _MKMidiOutStruct;

#define _MK_MIDIFILEEXT "midi"

/* Midi parts */
#define  _MK_MIDINOTEPORTS MIDI_NUMCHANS +  1
#define  _MK_MIDISYS 0

/* Functions for MIDI->MK semantic conversion. */
extern _MKMidiInStruct *_MKInitMidiIn(void);
extern id _MKMidiToMusicKit(_MKMidiInStruct *ptr,unsigned statusByte);
extern _MKMidiInStruct *_MKFinishMidiIn(_MKMidiInStruct *ptr);

/* Functions for MK->MIDI semantic conversion. */
extern _MKMidiOutStruct *_MKInitMidiOut(void);
extern  _MKMidiOutStruct *_MKFinishMidiOut(_MKMidiOutStruct *ptr);
extern void _MKWriteMidiOut(id aNote,double timeTag,unsigned chan,
			    _MKMidiOutStruct *ptr,id noteReceiver);
extern unsigned char _MKGetSysExByte(char **strP);


