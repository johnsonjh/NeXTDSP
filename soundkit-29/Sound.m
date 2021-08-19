#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	Sound.m
 *	Written by Lee Boynton
 *	Copyright 1988 NeXT, Inc.
 *
 */
#import <mach.h>
#import <zone.h>
#import <string.h>
#import <stdlib.h>
#import <pwd.h>
#import <sys/types.h>
#import <sys/file.h>
#import <sys/param.h>
#import <sys/time_stamp.h>
#import <objc/typedstream.h>
#import <objc/List.h>
#import <sound/sound.h>
#import <appkit/Application.h>
#import <appkit/Pasteboard.h>
#import <appkit/defaults.h>

#import "Sound.h"

extern int getsectdata();
extern int getuid();

#define HOME_SOUND_DIR "/Library/Sounds/"
#define LOCAL_SOUND_DIR "/LocalLibrary/Sounds/"
#define SYSTEM_SOUND_DIR "/NextLibrary/Sounds/"

@implementation Sound

#define DEFAULT_DURATION_IN_MINUTES (10.0)
#define BIG_BUF_SIZE ((int)(SND_RATE_CODEC*60.0*DEFAULT_DURATION_IN_MINUTES))
#define DEFAULT_INFO_SIZE 128
#define NULL_SOUND ((SNDSoundStruct*)0)

/*
 * Support to provide asynchronous messages to the delegate
 */

typedef struct _sound_message_t {
	msg_header_t	header;
	msg_type_t	type;
	int		theSound;
} sound_message_t;

#define SOUND_MSG_P_BEGIN 0
#define SOUND_MSG_P_END 1
#define SOUND_MSG_R_BEGIN 2
#define SOUND_MSG_R_END 3
#define SOUND_MSG_ERR 4

static port_t listenPort=0;
static int lastError=0;

 static void trimTail(SNDSoundStruct *s, int *nbytes);
 static int calcSoundSize(SNDSoundStruct *s);
 static void clearScratchSound(Sound *self);

static void messageReceived(Sound *self, int id)
{
    int i, err;
    if (!self) {
#ifdef DEBUG
	printf("Message received for null sound\n");
#endif
	return;
    }
    if (self->status == SK_STATUS_FREED)
	[self free];
    else switch (id) {
	case SOUND_MSG_R_BEGIN:
	    [self tellDelegate:@selector(willRecord:)];
	    break;
	case SOUND_MSG_R_END:
	    if (self->status != SK_STATUS_RECORDING_PAUSED) {
		int last_status = self->status;
		if (self->soundStruct) {
		    SNDInsertSamples(self->soundStruct,self->_scratchSound,
						          [self sampleCount]);
		    clearScratchSound(self);
		} else {
		    i = self->_scratchSize;
		    trimTail(self->_scratchSound,&i);
		    self->soundStruct = self->_scratchSound;
		    self->soundStructSize = i;
		}
		self->_scratchSound = 0;
		self->_scratchSize = 0;
		self->status = SK_STATUS_STOPPED;
		[self tellDelegate:@selector(didRecord:)];
		if (last_status == SK_STATUS_PLAYING_PENDING)
		    [self play:self];
	    } else if (self->soundStruct) {
		SNDInsertSamples(self->soundStruct,self->_scratchSound,
						          [self sampleCount]);
		self->_scratchSound->dataSize = self->_scratchSize;
	    } else {
		err = SNDCopySound(&self->soundStruct,self->_scratchSound);
		self->soundStructSize = calcSoundSize(self->_scratchSound);
	    }
	    break;
	case SOUND_MSG_P_BEGIN:
	    [self tellDelegate:@selector(willPlay:)];
	    break;
	case SOUND_MSG_P_END:
	    if (self->status != SK_STATUS_PLAYING_PAUSED) { 
		int last_status = self->status;
		self->status = SK_STATUS_STOPPED;
		[self tellDelegate:@selector(didPlay:)];
		if (last_status == SK_STATUS_RECORDING_PENDING)
		    [self record:self];
	    }
	    break;
	case SOUND_MSG_ERR:
	    [self tellDelegate:@selector(hadError:)];
	    self->status = SK_STATUS_STOPPED;
	    break;
	default:
	    break;
    }
}

static void receive_message(msg_header_t *msg, void *data)
{
    Sound *self = (Sound *)(((sound_message_t *)msg)->theSound);
    messageReceived(self,msg->msg_id);
}

static void init_port(id self)
{
    if (!listenPort) {
	port_allocate(task_self(), &listenPort);
	DPSAddPort((port_t)listenPort, (DPSPortProc)receive_message,
    			(int)sizeof(sound_message_t), (void *)0, 
			(int)NX_MODALRESPTHRESHOLD);
    }
}

extern int kern_timestamp();

static int usec_timestamp()
// wraps around at 40 seconds
{
    struct tsval foo;
    kern_timestamp(&foo);
    return foo.low_val;
}

static int post_message(int object_id, int msg_id)
{
    sound_message_t the_msg;
    static sound_message_t the_msg_template = {
	{
	    /* no name */		0,
	    /* msg_simple */		TRUE,
	    /* msg_size */		sizeof(sound_message_t),
	    /* msg_type */		MSG_TYPE_NORMAL,
	    /* msg_remote_port */	PORT_NULL,
	    /* msg_reply_port */	PORT_NULL,
	    /* msg_id */		0
	},
	{
	    /* msg_type_name = */	MSG_TYPE_INTEGER_32,
	    /* msg_type_size = */	32,
	    /* msg_type_number = */	1,
	    /* msg_type_inline = */	TRUE,
	    /* msg_type_longform = */	FALSE,
	    /* msg_type_deallocate = */	FALSE,
	}
    };
    the_msg = the_msg_template;
    the_msg.header.msg_remote_port = listenPort;
    the_msg.header.msg_id = msg_id;
    the_msg.theSound = object_id;
    return msg_send((msg_header_t *)&the_msg,MSG_OPTION_NONE,0);
}

static int play_begin_time = 0;

static double makeRateDouble(int val)
{
    double temp =(double)val;
    if (temp > 8012.0 && temp < 8013.0) temp = SND_RATE_CODEC;
    return temp;
}

 static int calcFormat(SNDSoundStruct *s);

static int calc_play_begin_time(SNDSoundStruct *s)
{
    int             temp = usec_timestamp(), lag;

    int             format = calcFormat(s);

    if ((format == SND_FORMAT_LINEAR_16 || format == SND_FORMAT_EMPHASIZED)
	&& s->channelCount == 2 &&
	(s->samplingRate == SND_RATE_LOW || s->samplingRate == SND_RATE_HIGH)) {
	lag = (s->samplingRate == SND_RATE_LOW) ? 120000 : 60000;
    } else {
    /* goes through the dsp */
	lag = 120000;
    /* this number is good for mulaw codec sounds */
    }
    return temp - lag;
}
 
static int play_begin(SNDSoundStruct *s, int tag, int err)
{
    play_begin_time = calc_play_begin_time(s);
    if (err) {
	lastError = err;
	post_message(tag, SOUND_MSG_ERR);
    } else
	post_message(tag, SOUND_MSG_P_BEGIN);
    return 0;
}

static int play_resume(SNDSoundStruct *s, int tag, int err)
{
    if (err) {
	lastError = err;
	post_message(tag, SOUND_MSG_ERR);
    } else
	play_begin_time = calc_play_begin_time(s);
    return 0;
}

static int play_end(SNDSoundStruct *s, int tag, int err)
{
    if (err && err != SND_ERR_ABORTED) {
	lastError = err;
	post_message(tag, SOUND_MSG_ERR);
    } else
	post_message(tag, SOUND_MSG_P_END);
    return 0;
}

static int record_begin(SNDSoundStruct *s, int tag, int err)
{
    if (err) {
	lastError = err;
	post_message(tag, SOUND_MSG_ERR);
    } else
	post_message(tag, SOUND_MSG_R_BEGIN);
    return 0;
}

static int record_end(SNDSoundStruct *s, int tag, int err)
{
    if (err && err != SND_ERR_ABORTED) {
	lastError = err;
	post_message(tag, SOUND_MSG_ERR);
    } else
	post_message(tag, SOUND_MSG_R_END);
    return 0;
}

static char *getHomeDirectory()
{
    static char *homeDirectory;
    struct passwd  *pw;
    if (!homeDirectory) {
	pw = getpwuid(getuid());
	if (pw && (pw->pw_dir) && (*pw->pw_dir)) {
	    homeDirectory = (char *)malloc(strlen(pw->pw_dir)+1);
	    strcpy(homeDirectory,pw->pw_dir);
	}
    }
    return homeDirectory;
}

static id findSoundfile(id factory, char *name)
{
    id newSound = nil;
    char filename[1024], *p;

    if (p = getHomeDirectory()) {
	strcpy(filename,p);
	strcat(filename,HOME_SOUND_DIR);
	strcat(filename,name);
	strcat(filename,".snd");
	newSound = [[factory alloc] initFromSoundfile:filename];
	if (newSound) {
	    [newSound setName:name];
	    return newSound;
	}
    }

    strcpy(filename,LOCAL_SOUND_DIR);
    strcat(filename,name);
    strcat(filename,".snd");
    newSound = [[factory alloc] initFromSoundfile:filename];
    if (newSound) {
	[newSound setName:name];
	return newSound;
    }

    strcpy(filename,SYSTEM_SOUND_DIR);
    strcat(filename,name);
    strcat(filename,".snd");
    newSound = [[factory alloc] initFromSoundfile:filename];
    if (newSound)
	[newSound setName:name];
    return newSound;
}


/*
 * Support to maintain a list of named sounds.
 */
static id soundList=nil;

static int namesMatch(const char *s1, const char *s2)
{
    if (!s1 || !s2) return 0;
    return (strcmp(s1,s2) ? 0 : 1);
}

static id findNamedSound(id aList, const char *name)
{
    int i, max;
    id aSound;

    if (aList) {
	max = [aList count];
	for (i=0; i<max; i++) {
	    aSound = [aList objectAt:i];
	    if (namesMatch([aSound name],name)) return aSound;
	}
    }
    return nil;
}

static id addNamedSound(id aList, char *name, id aSound)
{
    [aSound setName:name];
    if (!aList) return nil;
    [aList addObjectIfAbsent:aSound];
    return aSound;
}

static id removeNamedSound(id aList, char *name)
{
    id aSound;
    if (!aList) return nil;
    if (aSound = findNamedSound(aList,name))
	[aList removeObject:aSound];
    return aSound;
}

/*
 * Miscellaneous support functions
 */
static int calcFormat(SNDSoundStruct *s)
{
    if (s->dataFormat != SND_FORMAT_INDIRECT)
	return s->dataFormat;
    else {
	SNDSoundStruct **iBlock = (SNDSoundStruct **)s->dataLocation;
	if (*iBlock)
	    return (*iBlock)->dataFormat;
	else
	    return SND_FORMAT_UNSPECIFIED;
    }
}

static int calcHeaderSize(s)
    SNDSoundStruct *s;
{
    int size = strlen(s->info) + 1;
    if (size < 4) size = 4;
    else size = (size + 3) & 3;
    return(sizeof(SNDSoundStruct) - 4 + size);
}

static int calcSoundSize(SNDSoundStruct *s)
{
    if (!s)
	return 0;
    else if (s->dataFormat != SND_FORMAT_INDIRECT)
	return s->dataLocation + s->dataSize;
    else
	return calcHeaderSize(s);
}

 #define PAGESIZE (8192) //FIX ME!!!
 static int roundUpToPage(int size)
{
    int temp = size % PAGESIZE;
    if (temp)
	return size + PAGESIZE - temp;
    else
	return size;
}

static void trimTail(SNDSoundStruct *s, int *nbytes)
{
    int extraTailPtr = (int)s + s->dataLocation + s->dataSize;
    int extraBytes, extraPtr = (int)s+(*nbytes) + s->dataLocation;
    extraPtr = roundUpToPage(extraPtr);
    extraBytes = extraTailPtr - extraPtr;
    if (extraBytes > 0)
	vm_deallocate(task_self(),(pointer_t)extraPtr,extraBytes);
    *nbytes = s->dataSize;
}


/**************************
 *
 * Methods
 *
 */

+ (BOOL)isSound:(char *)aName
{
    return findNamedSound(soundList,aName)? YES : NO;
}

+ findSoundFor:(char *)aName
{
    id aSound = findNamedSound(soundList,aName);
    if (aSound) return aSound;
    aSound = [[self alloc] initFromMachO:aName];
    if (aSound) return aSound;
    aSound = findSoundfile(self, aName);
    return aSound;
}

static void allocSoundList()
{
    NXZone *zone = [NXApp zone];
    if (!zone) zone = NXDefaultMallocZone();
    soundList = [[List allocFromZone:zone] init];
}

+ addName:(char *)aName sound:aSound
{
    if (!aSound || !aName)
	return nil;
    if (!soundList) allocSoundList();
    else if (findNamedSound(soundList,aName))
	return nil;
    addNamedSound(soundList,aName,aSound);
    return aSound;
}

+ addName:(char *)aName fromSoundfile:(char *)filename
{
    id aSound;
    if (!aName || !filename) return nil;
    if (!soundList) allocSoundList();
    else if (findNamedSound(soundList,aName))
	return nil;
    aSound = [[Sound alloc] initFromSoundfile:filename];
    if (!aSound)
	return nil;
    addNamedSound(soundList,aName,aSound);
    return aSound;
}

+ addName:(char *)aName fromMachO:(char *)sectionName
{
    id aSound;
    if (!aName || !sectionName) return nil;
    if (!soundList) allocSoundList();
    else if (findNamedSound(soundList,aName))
	return nil;
    aSound = [[Sound alloc] initFromMachO:sectionName];
    if (!aSound)
	return nil;
    addNamedSound(soundList,aName,aSound);
    return aSound;
}

+ removeSoundForName:(char *)aName
{
    if (!aName) return nil;
    return removeNamedSound(soundList, aName);
}

+ new
{
    return [[self alloc] init];
}

+ newFromSoundfile:(char *)filename
{
    return [[self alloc] initFromSoundfile:filename];
}

- initFromSoundfile:(char *)filename
{
    SNDSoundStruct *theStruct;

    if (filename && !SNDReadSoundfile(filename,&theStruct)) {
	[super init];
	soundStruct = theStruct;
	soundStructSize = theStruct->dataLocation + theStruct->dataSize;
	_scratchSound = 0;
	return self;
    } else {
	[self free];
	return nil;
    }
}

+ newFromMachO:(char *)sectionName
{
    return [[self alloc] initFromMachO:sectionName];
}

- initFromMachO:(char *)sectionName
{
    SNDSoundStruct *s1, *s2;
    int size, err;
    s1 = (SNDSoundStruct *)getsectdata("__SND", sectionName, &size);
    if (!s1) {
	char filePath[MAXPATHLEN+1];
	
        if (NXArgv[0][0] == '/') {
	    strcpy(filePath, NXArgv[0]);
	    *(rindex(filePath, '/')+1) = 0;
	    strcat(filePath, sectionName);
	} else
	    strcpy(filePath, sectionName);
	if (SND_ERR_NONE != SNDReadSoundfile(filePath, &s2)) {
	    [self free];
	    return nil;
	}
    } else {
        err = SNDCopySound(&s2,s1);
        if (err) {
	    [self free];
	    return nil;
	}
    }
    [super init];
    soundStruct = s2;
    soundStructSize = size;
    _scratchSound = 0;
    return self;
}

+ newFromPasteboard
{
    return [[self alloc] initFromPasteboard];
}

- initFromPasteboard
{
    char *s;
    int slen;
    BOOL hasSoundData = NO;
    char **types;
    id thePboard = [Pasteboard new];
    if (!thePboard) {
	[self free];
	return nil;
    }
    types = (char **)[thePboard types];
    while (*types) {
	char *thetype = *types;
	if (!strcmp(thetype, NXSoundPboardType)) hasSoundData = YES;
	types++;
    }
    if (hasSoundData) {
	[thePboard readType:NXSoundPboardType data:&s length:&slen];
	if (s && slen) {
	    [super init];
	    soundStruct = (SNDSoundStruct *)s;
	    soundStructSize = slen;
	    _scratchSound = 0;
	    return self;
	}
    }
    [self free];
    return nil;
}

+ getVolume:(float *)left :(float *)right
{
    int rawLeft, rawRight;
    int err = SNDGetVolume(&rawLeft,&rawRight);
    if (!err) {
	*left = (float)rawLeft / 43.0;
	*right = (float)rawRight / 43.0;
	return self;
    }
    return nil;
}

+ setVolume:(float)left :(float)right
{
    int rawLeft = (int)(left * 43.0), rawRight = (int)(right * 43.0);
    int err = SNDSetVolume(rawLeft,rawRight);
    return err? nil : self;
}

+ (BOOL)isMuted
{
    int on;
    SNDGetMute(&on);
    return on? NO : YES;
}

+ setMute:(BOOL)aFlag
{
    int err = SNDSetMute(aFlag? 0 : 1);
    return err? nil : self;
}

static void clearScratchSound(Sound *self)
{
    if (self->_scratchSound) {
	SNDFree(self->_scratchSound);
	self->_scratchSound = 0;
	self->_scratchSize = 0;
    }
}

 static int freeSoundStruct(Sound *self)
{
    SNDSoundStruct *s = self->soundStruct;
    int             err;

    if (s && s->magic == SND_MAGIC) {
	if (s->dataFormat != SND_FORMAT_INDIRECT)
	    if (self->soundStructSize > (s->dataLocation + s->dataSize))
		s->dataSize = self->soundStructSize - s->dataLocation;
	err = SNDFree(s);
    } else
	err = SND_ERR_NOT_SOUND;
    self->soundStruct = (SNDSoundStruct *) 0;
    self->soundStructSize = 0;
    return err;
}

- free
{
    if (status != SK_STATUS_STOPPED) {
	status = SK_STATUS_FREED;
	return nil;
    }
    if (name) removeNamedSound (soundList, name);	// patch bs 8/4/89
    if (soundStruct)
	freeSoundStruct(self);
    if (name) free(name);
    return [super free];
}

- write:(NXTypedStream *)stream
{
    [super write: stream];
    NXWriteTypes(stream, "*i@i",&name,&priority,&delegate,&soundStructSize);
    if (soundStructSize) {
	[self compactSamples];
	NXWriteArray(stream, "c", soundStructSize, soundStruct);
    }
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read: stream];
    NXReadTypes(stream, "*i@i",&name,&priority,&delegate,&soundStructSize);
    if (soundStructSize) {
	if (vm_allocate(task_self(),(pointer_t *)&soundStruct,
							soundStructSize,1))
	    soundStructSize = 0;
	else
	    NXReadArray(stream, "c", soundStructSize, soundStruct);
    }
    status = SK_STATUS_STOPPED;
    return self;
}

- finishUnarchiving
{
    if (name) {
	id existingSound = [Sound findSoundFor:name];
	if (existingSound) {
	    [self free];
	    return existingSound;
	} else {
	    [Sound addName:name sound:self];
	    return nil;
	}
    } else
	return nil;
}

- (const char *)name
{
    return name;
}

- setName:(const char *)theName
{
    char	*oldName = name;	// bserlet 8/4/89
    if(theName) {
	if (findNamedSound(soundList,theName))
	    return nil;
    } else
	theName = "";
//    if (name) free(name);
    name = NXCopyStringBufferFromZone(theName, [self zone]);
    if (oldName) free (oldName);	// bserlet 8/4/89
    return self;
}

- delegate
{
    return delegate;
}

- setDelegate:anObject
{
    delegate = anObject;
    return self;
}

- (double)samplingRate
{
    if (soundStruct)
	return makeRateDouble(soundStruct->samplingRate);
    else
	return 0.0;
}

- (int)sampleCount
{
    if (!soundStruct) return 0;
    return SNDSampleCount(soundStruct);
}

- (int)channelCount
{
    return soundStruct ? soundStruct->channelCount : 0;
}

- (char *)info
{
    return soundStruct ? soundStruct->info : (char *)0;
}

- (int)infoSize
{
    return soundStruct ?
	(sizeof(SNDSoundStruct) - soundStruct->dataLocation + 4) : 0;
}

- play:sender
{
    int err = [self play];
    if (err) {
	lastError = err;
	messageReceived((Sound *)self,SOUND_MSG_ERR);
    }
    return self;
}

- (int)play
{
    int err, preempt = 1;
    switch (status) {
	case SK_STATUS_PLAYING:
	    return SND_ERR_NONE;
	case SK_STATUS_PLAYING_PAUSED:
	    return [self resume];
	case SK_STATUS_RECORDING:
	    status = SK_STATUS_PLAYING_PENDING;
	    return [self stop];
	case SK_STATUS_RECORDING_PAUSED:
	    status = SK_STATUS_STOPPED;
	    clearScratchSound(self);
	    [self tellDelegate:@selector(didRecord:)];
	    break;
	case SK_STATUS_PLAYING_PENDING:
	    return SND_ERR_NONE;
	default:
	    break;
    }
    if (!soundStruct || [self isEmpty]) {
	post_message((int)self,SOUND_MSG_P_BEGIN);
	post_message((int)self,SOUND_MSG_P_END);
	return SND_ERR_NONE;
    }
    clearScratchSound(self);
    init_port(self);
    status = SK_STATUS_PLAYING;
    if (err = SNDStartPlaying(soundStruct,(int)self,priority,
    						preempt,play_begin,play_end))
	status = SK_STATUS_STOPPED;
    return err;
}

- record:sender
{
    int err = [self record];
    if (err) {
	lastError = err;
	messageReceived((Sound *)self,SOUND_MSG_ERR);
    }
    return self;
}

- (int)record
{
    int             err, preempt = 0;

    switch (status) {
    case SK_STATUS_RECORDING:
	return SND_ERR_NONE;
    case SK_STATUS_RECORDING_PAUSED:
	return[self resume];
    case SK_STATUS_PLAYING:
	status = SK_STATUS_RECORDING_PENDING;
	return[self stop];
    case SK_STATUS_PLAYING_PAUSED:
	status = SK_STATUS_STOPPED;
	clearScratchSound(self);
	[self tellDelegate:@selector(didPlay:)];
	break;
    case SK_STATUS_RECORDING_PENDING:
	return SND_ERR_NONE;
    default:
	break;
    }
    clearScratchSound(self);
    if (status == SK_STATUS_INITIALIZED) {
	err = SNDCopySound(&_scratchSound, soundStruct);
	_scratchSize = soundStructSize;
    } else {
	err = SNDAlloc(&_scratchSound, BIG_BUF_SIZE, SND_FORMAT_MULAW_8,
		       SND_RATE_CODEC, 1, DEFAULT_INFO_SIZE);
	_scratchSize = BIG_BUF_SIZE;
    }
    if (soundStruct)
	freeSoundStruct(self);
    if (err) {
	_scratchSize = 0;
	_scratchSound = 0;
	return err;
    }
    init_port(self);
    status = SK_STATUS_RECORDING;
    if (err = SNDStartRecording(_scratchSound, (int)self,
				priority, preempt,
				record_begin, record_end))
	status = SK_STATUS_STOPPED;
    return err;
}


- (int)samplesProcessed
{
    int count, temp, now = usec_timestamp();
    double sec;
    switch (status) {
	case SK_STATUS_PLAYING:
	    sec = (double)(now - play_begin_time) / 1000000.0;
	    count = (int)(sec * [self samplingRate]);
	    if (_scratchSound)
		temp = SNDSampleCount(_scratchSound);
	    else
		temp = SNDSampleCount(soundStruct);
	    return (temp < count)? temp : count;
	case SK_STATUS_RECORDING:
	    return SNDSampleCount(_scratchSound);
	case SK_STATUS_PLAYING_PAUSED:
	    return _scratchSize;
	case SK_STATUS_RECORDING_PAUSED:
	    return SNDSampleCount(soundStruct);
	default:
	    return SNDSampleCount(soundStruct);
    }
}

- (int)status
{
    return status;
}

- (int)waitUntilStopped
{
    int err;
    if (status == SK_STATUS_RECORDING || status == SK_STATUS_PLAYING)
	err = SNDWait((int)self);
    else
	err = SND_ERR_NONE;
    return err;
}

- stop:sender
{
    [self stop];
    return self;
}

- (int)stop
{
    int err;
    switch (status) {
	case SK_STATUS_RECORDING:
	case SK_STATUS_PLAYING_PENDING:
	    err = SNDStop((int)self);
	    if (err) {
		status = SK_STATUS_STOPPED;
		break;
	    }
	    break;
	case SK_STATUS_PLAYING:
	case SK_STATUS_RECORDING_PENDING:
	    err = SNDStop((int)self);
	    if (err) {
		status = SK_STATUS_STOPPED;
		clearScratchSound(self);
	    }
	    break;
	default:
	    err = SND_ERR_NONE;
	    status = SK_STATUS_STOPPED;
	    break;
    }
    return err;
}

- pause:sender
{
    int err = [self pause];
    if (err) {
	lastError = err;
	messageReceived((Sound *)self,SOUND_MSG_ERR);
    }
    return self;
}

- (int)pause
{
    int err, old_status = status, count;
    switch (status) {
	case SK_STATUS_RECORDING:
	    status = SK_STATUS_RECORDING_PAUSED;
	    err = SNDStop((int)self);
	    if (err) status = old_status;
	    break;
	case SK_STATUS_PLAYING:
	    count = [self samplesProcessed];
	    if (_scratchSound)
		_scratchSize += count;
	    else
		_scratchSize = count;
	    status = SK_STATUS_PLAYING_PAUSED;
	    err = SNDStop((int)self);
	    if (err) status = old_status;
	    break;
	default:
	    err = SND_ERR_NONE;
	    break;
    }
    return err;
}

- resume:sender
{
    int err = [self resume];
    if (err) {
	lastError = err;
	messageReceived((Sound *)self,SOUND_MSG_ERR);
    }
    return self;
}

- (int)resume
{
    int err, length, preempt = 1;
    switch (status) {
	case SK_STATUS_RECORDING_PAUSED:
	    status = SK_STATUS_RECORDING;
#ifdef DEBUG
	    if (!_scratchSound) printf("Scratchsound is bogus!\n");
#endif
	    _scratchSound->dataSize = _scratchSize;
	    if (err = SNDStartRecording(_scratchSound, (int)self, 
					    priority, preempt,
						0, record_end)) {
		status = SK_STATUS_STOPPED;
    		clearScratchSound(self);
	    }
	    break;
	case SK_STATUS_PLAYING_PAUSED:
	    if (_scratchSize <= 0) {
		clearScratchSound(self);
		status = SK_STATUS_PLAYING;
		if (err = SNDStartPlaying(soundStruct,(int)self,priority,
						 preempt,play_resume,play_end))
		    status = SK_STATUS_STOPPED;
	    } else {
		length = [self sampleCount] - _scratchSize;
#ifdef DEBUG
		printf("resume at %d, size = %d\n",_scratchSize,length);
#endif
		if (_scratchSound) SNDFree(_scratchSound);
		err = SNDCopySamples(&_scratchSound,soundStruct,
						    _scratchSize,length);
		if (err) {
		    status = SK_STATUS_STOPPED;
		    return err;
		}
		status = SK_STATUS_PLAYING;
		if (err = SNDStartPlaying(_scratchSound,(int)self,priority,
						 preempt,play_resume,play_end))
		    status = SK_STATUS_STOPPED;
	    }
	    break;
	default:
	    err = SND_ERR_NONE;
	    break;
    }
    return err;
}


- (int)readSoundfile:(char *)filename;
{
    SNDSoundStruct *theStruct;
    int err;

    if (status) return SND_ERR_UNKNOWN;
    if (!filename) return SND_ERR_BAD_FILENAME;
    err = SNDReadSoundfile(filename,&theStruct);
    if (err == SND_ERR_NONE) {
	if (soundStruct) 
	    freeSoundStruct(self);
	[self setName:NULL];
	soundStruct = theStruct;
	soundStructSize = theStruct->dataLocation + theStruct->dataSize;
    }
    return err;
}

- (int)writeSoundfile:(char *)filename;
{
    if (!soundStruct) return SND_ERR_NOT_SOUND;
    return SNDWriteSoundfile(filename,soundStruct);
}

- (int)writeToPasteboard
{
    const char *typeList[1];
    id thePboard;
    int err = [self compactSamples]; //? is this efficient enough
    if (err) return err;
    typeList[0] = NXSoundPboardType;
    thePboard = [Pasteboard new];
    [thePboard declareTypes:typeList num:1 owner:NXApp];
    thePboard = [Pasteboard new];
    [thePboard writeType:NXSoundPboardType data:(char *)soundStruct 
		  length:soundStructSize];
    return SND_ERR_NONE; //?
}

- (int)convertToFormat:(int)aFormat
	   samplingRate:(double)aRate
	   channelCount:(int)aChannelCount
{
    int err;
    SNDSoundStruct *oldStruct = soundStruct, *s;
    SNDSoundStruct header = {
	SND_MAGIC, sizeof(SNDSoundStruct),0,aFormat,(int)aRate,aChannelCount,0
    };
    if (oldStruct->dataFormat == SND_FORMAT_INDIRECT) {
	s = oldStruct;
	err = SNDCompactSamples(&oldStruct,s);
	SNDFree(s);
    }
    s = &header;
    err = SNDConvertSound(oldStruct,&s);
    if (err)
	return SND_ERR_NOT_IMPLEMENTED;
    else {
	soundStruct = s;
	soundStructSize = calcSoundSize(soundStruct);
	if (oldStruct)
	    SNDFree(oldStruct);
	return SND_ERR_NONE;
    }
    return SND_ERR_NONE;
}

- (BOOL)compatibleWith:aSound
{
    SNDSoundStruct *s = [aSound soundStruct];
    if (!soundStruct || !s) return YES;
    if (calcFormat(soundStruct) == calcFormat(s) &&
    		soundStruct->samplingRate == s->samplingRate &&
			soundStruct->channelCount == s->channelCount)
	return YES;
    else
	return NO;
}

- (BOOL)isEmpty
{
    if (!soundStruct) return YES;
    if ([self isEditable])
	return [self sampleCount]? NO : YES;
    else
	return NO;
}


- (BOOL)isEditable
{
    if (!soundStruct)
	return YES;
    else if (soundStruct->dataFormat == SND_FORMAT_DSP_CORE)
	return NO;
    else
	return YES;
}

- copy
{
    id newSound = [[[self class] alloc] init];
    [newSound copySound:self];
    return newSound;
}

- (int)copySound:aSound
{
    SNDSoundStruct *dst, *src = [aSound soundStruct];
    int err = SND_ERR_NONE;
    if (!src)
	dst = NULL_SOUND;
    else
	err = SNDCopySound(&dst,src);
    if (!err) {
	if (soundStruct)
	    freeSoundStruct(self);
	soundStruct = dst;
	soundStructSize = calcSoundSize(soundStruct);
    }
    return err;
}

- (int)copySamples:aSound at:(int)startSample count:(int)sampleCount
{
    int err;
    SNDSoundStruct *dst, *src = [aSound soundStruct];
    if (![self isEditable])
	return SND_ERR_CANNOT_EDIT;
    if (src) {
	err = SNDCopySamples(&dst,src,startSample,sampleCount);
	if (err)
	    return err;
	if (soundStruct)
	    freeSoundStruct(self);
	soundStruct = dst;
	soundStructSize = calcSoundSize(dst);
	return SND_ERR_NONE;
    } else {
	if (soundStruct)
	    freeSoundStruct(self);
	soundStruct = NULL_SOUND;
	soundStructSize = 0;
	return SND_ERR_NONE;
    }
}

- (int)deleteSamples
{
    int theCount, err;
    if (![self isEditable])
	return SND_ERR_CANNOT_EDIT;
    if (!soundStruct)
	return SND_ERR_NONE;
    theCount = SNDSampleCount(soundStruct);
    err = [self deleteSamplesAt:0 count:theCount];
    return err;
}

- (int)deleteSamplesAt:(int)startSample count:(int)sampleCount
{
    int saveFormat; //!! to fix bug in SNDDeleteSamples
    int err;
    if (![self isEditable])
	return SND_ERR_CANNOT_EDIT;
    if (!soundStruct)
	return SND_ERR_NONE;
    saveFormat = calcFormat(soundStruct);	//!!
    err = SNDDeleteSamples(soundStruct, startSample, sampleCount);
    soundStructSize = calcSoundSize(soundStruct);
    if (!soundStruct->dataSize)			//!
	soundStruct->dataFormat = saveFormat;	//!!
    return err;
}

- (int)insertSamples:aSound at:(int)startSample
{
    int err;
    SNDSoundStruct *s = [aSound soundStruct];
    if (![self isEditable])
	return SND_ERR_CANNOT_EDIT;
    if (!s || ![aSound sampleCount])
	return SND_ERR_NONE;
    if (soundStruct && [self sampleCount]) {
	if ([self compatibleWith:aSound]) {
	    err = SNDInsertSamples(soundStruct,s,startSample);
	    soundStructSize = calcSoundSize(soundStruct);
	} else {
	    SNDSoundStruct *s2, header = {
		    SND_MAGIC, sizeof(SNDSoundStruct),0,
		    calcFormat(soundStruct),
		    soundStruct->samplingRate,
		    soundStruct->channelCount, 0 };
	    s2 = &header;
	    err = SNDConvertSound(s,&s2);
	    if (!err) {
		err = SNDInsertSamples(soundStruct,s2,startSample);
		soundStructSize = calcSoundSize(soundStruct);
	    }
	}
    } else {
	if (soundStruct) SNDFree(soundStruct);
	soundStruct = NULL_SOUND;
	err = SNDCopySound(&soundStruct,s);
	soundStructSize = calcSoundSize(soundStruct);
    }
    return err;
}

- (int)compactSamples
{
    SNDSoundStruct *s;
    int err;
    if (![self isEditable])
	return SND_ERR_CANNOT_EDIT;
    if (soundStruct && [self needsCompacting]) {
	err = SNDCompactSamples(&s,soundStruct);
	if (!err) {
	    freeSoundStruct(self);
	    soundStruct = s;
	    soundStructSize = calcSoundSize(soundStruct);
	}
    } else
	err = SND_ERR_NONE;
    return err;
}

- (BOOL)needsCompacting
{
    if (soundStruct && soundStruct->dataFormat == SND_FORMAT_INDIRECT)
	return YES;
    else
	return NO;
}

- (unsigned char *)data
{
    unsigned char *foo;
    if (!soundStruct)
	foo = (unsigned char *)0;
    else if (soundStruct->dataFormat == SND_FORMAT_INDIRECT)
	foo = (unsigned char *)soundStruct->dataLocation;
    else {
	foo = (unsigned char *)soundStruct;
	foo += soundStruct->dataLocation;
    }
    return foo;
}

- (int)dataSize
{
    return soundStruct? soundStruct->dataSize : 0;
}

- (int)dataFormat
{
    return soundStruct? calcFormat(soundStruct) : 0;
}

- (int)setDataSize:(int)newDataSize
     dataFormat:(int)newDataFormat
     samplingRate:(double)newSamplingRate
     channelCount:(int)newChannelCount
     infoSize:(int)newInfoSize;
{
    SNDSoundStruct *s;
    int err;
    if (status != SK_STATUS_STOPPED && status != SK_STATUS_INITIALIZED)
	return SND_ERR_CANNOT_EDIT;
    err = SNDAlloc(&s,newDataSize,newDataFormat,(int)newSamplingRate,
    			newChannelCount, newInfoSize);
    if (!err) {
	if (soundStruct)
	    freeSoundStruct(self);
	soundStruct = s;
	soundStructSize = s->dataLocation + s->dataSize;
	status = SK_STATUS_INITIALIZED;
    }
    return err;
}


- (SNDSoundStruct *)soundStruct
{
    return soundStruct;
}

- (int)soundStructSize
{
    return soundStructSize;
}

- setSoundStruct:(SNDSoundStruct *)aStruct soundStructSize:(int)aSize
{
    if (status == SK_STATUS_STOPPED || status == SK_STATUS_INITIALIZED) {
	soundStruct = aStruct;
	soundStructSize = aSize;
	status = SK_STATUS_INITIALIZED;
    }
    return self;
}

- tellDelegate:(SEL)theMessage
{
    if (theMessage && delegate && [delegate respondsTo:theMessage])
	[delegate perform:theMessage with:self];
    return self;
}

- soundBeingProcessed
{
    return self;
}

- (SNDSoundStruct *)soundStructBeingProcessed
{
    switch (status) {
	case SK_STATUS_RECORDING:
	    return _scratchSound;
	case SK_STATUS_PLAYING:
	    if (_scratchSound)
		return _scratchSound;
	    else
		return soundStruct;
	default: 
	    return soundStruct;
    }
}

- (int)processingError
{
    return lastError;
}

@end

/*

Created by Lee Boynton.

Modification History:

25
--
 9/11/90 wot 	Changed newFromMachO: to look in the application's directory if
		the sound can't be found in the machO.
 9/20/90 wot	Added support for SND_FORMAT_EMPHASIZED.  Made it do the same
		things as SND_FORMAT_LINEAR_16.
 10/8/90 wot	Changed NXSoundPboard to NXSoundPboardType.

27
--
 10/11/90 aozer	Added initFromSoundfile:, initFromMachO:, and 
		initFromPasteboard. Got rid of init.

*/