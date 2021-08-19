/*
	Sound.h
	Sound Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import <objc/hashtable.h>
#import <streams/streams.h>
#import <sound/sound.h>

/* Define this for compatibility */
#define NXSoundPboard NXSoundPboardType

extern NXAtom NXSoundPboardType;
/*
 * This is the sound pasteboard type.
 */

@interface Sound : Object
/*
 * The Sound object encapsulates a SNDSoundStruct, which represents a sound.
 * It supports reading and writing to a soundfile, playback of sound,
 * recording of sampled sound, conversion among various sampled formats, 
 * basic editing of the sound, and name and storage management for sounds.
 */
 {
    SNDSoundStruct *soundStruct; /* the sound data structure */
    int soundStructSize;	 /* the length of the structure in bytes */
    int priority;		 /* the priority of the sound */
    id delegate;		 /* the target of notification messages */
    int status;			 /* what the object is currently doing */
    char *name;			 /* The name of the sound */
    SNDSoundStruct *_scratchSound;
    int _scratchSize;
}

/*
 * Status codes
 */
typedef enum {
    SK_STATUS_STOPPED = 0,
    SK_STATUS_RECORDING,
    SK_STATUS_PLAYING,
    SK_STATUS_INITIALIZED,
    SK_STATUS_RECORDING_PAUSED,
    SK_STATUS_PLAYING_PAUSED,
    SK_STATUS_RECORDING_PENDING,
    SK_STATUS_PLAYING_PENDING,
    SK_STATUS_FREED = -1,
} SKStatus;

/*
 * Macho segment name where sounds may be.
 */
#define SK_SEGMENT_NAME "__SND"


/*
 * --------------- Factory Methods
 */

+ findSoundFor:(char *)aName;

+ addName:(char *)name sound:aSound;
+ addName:(char *)name fromSoundfile:(char *)filename;
+ addName:(char *)name fromMachO:(char *)sectionName;
+ removeSoundForName:(char *)name;

+ getVolume:(float *)left :(float *)right;
+ setVolume:(float)left :(float)right;
+ (BOOL)isMuted;
+ setMute:(BOOL)aFlag;

- initFromSoundfile:(char *)filename;
- initFromMachO:(char *)sectionName;
- initFromPasteboard;

- free;
- write:(NXTypedStream *) stream;
- read:(NXTypedStream *) stream;
- finishUnarchiving;
- (const char *)name;
- setName:(const char *)theName;
- delegate;
- setDelegate:anObject;
- (double)samplingRate;
- (int)sampleCount;
- (int)channelCount;
- (char *)info;
- (int)infoSize;
- play:sender;
- (int)play;
- record:sender;
- (int)record;
- (int)samplesProcessed;
- (int)status;
- (int)waitUntilStopped;
- stop:sender;
- (int)stop;
- pause:sender;
- (int)pause;
- resume:sender;
- (int)resume;
- (int)readSoundfile:(char *)filename;
- (int)writeSoundfile:(char *)filename;
- (int)writeToPasteboard;
- (BOOL)isEmpty;
- (BOOL)isEditable;
- (BOOL)compatibleWith:aSound;
- (int)convertToFormat:(int)aFormat
	   samplingRate:(double)aRate
	   channelCount:(int)aChannelCount;
- (int)deleteSamples;
- (int)deleteSamplesAt:(int)startSample count:(int)sampleCount;
- (int)insertSamples:aSound at:(int)startSample;
- (int)copySound:aSound;
- (int)copySamples:aSound at:(int)startSample count:(int)sampleCount;
- (int)compactSamples;
- (BOOL)needsCompacting;
- (unsigned char *)data;
- (int)dataSize;
- (int)dataFormat;
- (int)setDataSize:(int)newDataSize
     dataFormat:(int)newDataFormat
     samplingRate:(double)newSamplingRate
     channelCount:(int)newChannelCount
     infoSize:(int)newInfoSize;
- (SNDSoundStruct *)soundStruct;
- (int)soundStructSize;
- setSoundStruct:(SNDSoundStruct *)aStruct soundStructSize:(int)aSize;
- (SNDSoundStruct *)soundStructBeingProcessed;
- (int)processingError;
- soundBeingProcessed;
- tellDelegate:(SEL)theMessage;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newFromSoundfile:(char *)filename;
+ newFromMachO:(char *)sectionName;
+ newFromPasteboard;

@end

@interface SoundDelegate : Object
- willRecord:sender;
- didRecord:sender;
- willPlay:sender;
- didPlay:sender;
- hadError:sender;
@end

