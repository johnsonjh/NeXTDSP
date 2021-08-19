/* 
Modification history:

  02/25/90/daj - Changed to make instancable. Added sysexcl support.
*/

#import <streams/streams.h>

/*
 * Only the two following metaevents are supported; data[0] contains one
 * of the following codes if the metaevent flag is set. The metaevents are
 * provided for reading. Separate functions exist to write metaevents.
 */

#define _MKMIDI_DEFAULTQUANTASIZE (1000)

typedef enum __MKMIDIMetaEvent {
    /* In all of the metaevents, data[0] is the metaevent itself. */
    _MKMIDI_sequenceNumber = 0,
    /*
     * data[1] and data[2] contain high and low order bits of number. 
     */
    _MKMIDI_text = 1,
    _MKMIDI_copyright = 2,
    _MKMIDI_sequenceOrTrackName = 3,
    /* _MKMIDI_instrumentName not supported */  
    _MKMIDI_lyric = 5,
    _MKMIDI_marker = 6,
    _MKMIDI_cuePoint = 7,
    /* data[1]* specifies null-terminated text. 
     */
    /*
     * _MKMIDI_channelprefix, should be implemented by midifile.m and 
     * should not be passed up to user. 
     */
    _MKMIDI_trackChange,
    /*
     * Track change metaevent: data[1] and data[2] contain high/low order bits,
     * respectively, containing the track number. These events can only be 
     * encountered when reading a level-1 file.
     */
    _MKMIDI_tempoChange,
    /*
     * Tempo change metaevent: data[1:4] contain 4 bytes of data.
     */
    _MKMIDI_smpteOffset,
    /*
      data[1:5] are the 5 numbers hr mn sec fr ff
      */
    _MKMIDI_timeSig,
    /* data is a single int, where 1-byte fields are nn dd cc bb */
    _MKMIDI_keySig
    /*  data is a single short, where 1-byte fields are sf mi  */
  } _MKMIDIMetaevent;

extern void *_MKMIDIFileBeginReading(NXStream *s,
				     int **quanta,
				     BOOL **metaevent,
				     int **ndata,
				     unsigned char ***data);
/* Ref args are set to pointers to where data is returned */

extern void *_MKMIDIFileEndReading(void *p);

extern int _MKMIDIFileReadPreamble(void *p,int *level,int *track_count);
/*
 * Read the header of the specified file, and return the midifile level 
 * (format) of the file, and the total number of tracks, in the respective 
 * parameters. The return value will be non-zero if all is well; any error
 * causes zero to be returned.
 */

extern int _MKMIDIFileReadEvent(void *p);
/*
 * Read the next event in the current track. Return nonzero if successful;
 * zero if an error or end-of-stream occurred.
 */

void *_MKMIDIFileBeginWriting(NXStream *s, int level, char *sequenceName);
/*
 * Writes the preamble and opens track zero for writing. In level 1 files,
 * track zero is used by convention for timing information (tempo,time
 * signature, click track). To begin the first track in this case, first
 * call _MKMIDIFileBeginWritingTrack.
 * _MKMIDIFileBeginWriting must be balanced by a call to _MKMIDIFileEndWriting.
 */


extern int _MKMIDIFileEndWriting(void *p);
/*
 * Terminates writing to the stream. After this call, the stream may
 * be closed.
 */

extern int _MKMIDIFileBeginWritingTrack(void *p, char *trackName);
extern int _MKMIDIFileEndWritingTrack(void *p,int quanta);
/*
 * These two functions must be called in a level 1 file to bracket each
 * chunk of track data (except track 0, which is special).
 */

extern int _MKMIDIFileWriteTempo(void *p,int quanta, int beatsPerMinute);

extern int _MKMIDIFileWriteEvent(void *p,int quanta,int ndata,
				 unsigned char *bytes);

extern int _MKMIDIFileWriteSysExcl(void *p,int quanta,unsigned char *bytes,
				   int ndata);

extern int _MKMIDIFileWriteSig(void *p,int quanta,short metaevent,
			       unsigned data);
/* Write time sig or key sig. Specified in midifile format. */

extern int _MKMIDIFileWriteText(void *p,int quanta,short metaevent,char *data);

extern int _MKMIDIFileWriteSMPTEoffset(void *p,unsigned char hr,
				       unsigned char min,
				       unsigned char sec,
				       unsigned char ff,
				       unsigned char  fr);

int _MKMIDIFileWriteSequenceNumber(void *p,int data);

