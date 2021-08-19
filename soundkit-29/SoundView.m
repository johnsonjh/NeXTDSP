#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	SoundView.m
 *	Written by Lee Boynton
 *	Copyright 1988-89 NeXT, Inc.
 *
 */
#import <stdlib.h>
#import <string.h>
#import <zone.h>
#import <sound/sound.h>
#import <appkit/Application.h>
#import <appkit/Window.h>
#import <appkit/Pasteboard.h>
#import <appkit/timer.h>
#import <dpsclient/wraps.h>
#import "Sound.h"
#import "SoundView.h"
#import	"soundWraps.h"

typedef struct {
    @defs (Sound)
} *soundId;

@implementation SoundView

/*
 * Caret support (only used when the view is the first responder)
 */

#define CURSOR_FLASH_RATE (0.5)
static DPSTimedEntry _timedEntry = 0;
static BOOL cursorOn = 0;
static int cursorState = 0;

static void toggle_cursor(DPSTimedEntry timedEntry, double now, 
							SoundView *self)
{
    [self lockFocus];
    PSsetinstance(1);
    PSnewinstance();
    if (cursorState =! cursorState) {
	PSsetgray(NX_LTGRAY);
	PSsetlinewidth(0.0);
	_SNDMovetoRlinetoStroke(self->selectionRect.origin.x, self->selectionRect.origin.y,
				0.0, self->selectionRect.size.height);
    }
    [self unlockFocus];
}

static void startCaret(SoundView *self)
{
    if (!cursorOn) {
	[self lockFocus];
	PSsetinstance(1);
	PSnewinstance();
	PSsetgray(NX_LTGRAY);
	PSsetlinewidth(0.0);
	_SNDMovetoRlinetoStroke(self->selectionRect.origin.x, self->selectionRect.origin.y,
				0.0, self->selectionRect.size.height);
	[self unlockFocus];
	cursorOn = cursorState = 1;
	_timedEntry = DPSAddTimedEntry(CURSOR_FLASH_RATE, 
			(DPSTimedEntryProc)toggle_cursor, 
			self,NX_BASETHRESHOLD);
    }
}

static void stopCaret(SoundView *self)
{
    if (cursorOn) {
	cursorOn = 0;
	DPSRemoveTimedEntry(_timedEntry);
        [self lockFocus];
	PSsetinstance(1);
	PSnewinstance();
	[self unlockFocus];
    }
}


/*
 * draw a display reduction with user paths.
 * NOTE: this code is currently non-reentrant.
 *
 * The user path uses fractional numbers (31 int, 1 bit fraction), so that
 * in outline mode, the endpoints can be offset by 1/2, leading to better
 * looking jaggies.
 */

#define USERPATHSIZE (1024-2)

static char *userPathOps=0;
static int userPathBBox[4] = {0};
static int *userPath=0;

static void setupUserPath(SoundView *self)
{
    int i;
    char *op;
    op = userPathOps = (char *)malloc(USERPATHSIZE+2);
    *op++ = dps_setbbox;
    *op++ = dps_moveto;
    for (i = 0; i < (USERPATHSIZE-1); i++)
	*op++ = dps_lineto;
    userPath = (int *)malloc(USERPATHSIZE*2*sizeof(int));
}

static void drawSound(SoundView *self, Sound *reduction, int begin, int end)
{
    int cur_chan, cur_x;
    int sampleCount, channelCount, firstSample;
    int i, max_chan;
    SNDSoundStruct *s = [reduction soundStruct];
    int mode = self->displayMode;
    
    if (!s) return;
    if (!userPath) setupUserPath(self);

    sampleCount = SNDSampleCount(s);
    firstSample = 0;
    if (begin >= 0 && end < sampleCount)
	sampleCount = end;
    if (begin > 0 && begin < sampleCount) {
	firstSample += begin;
	sampleCount -= begin;
    }
    channelCount = s->channelCount;
    userPathBBox[0] = firstSample<<1;
    userPathBBox[1] = (-32768)<<1;
    userPathBBox[2] = ((firstSample+sampleCount)<<1)+1;
    userPathBBox[3] = 65535<<1;
    if (mode == SK_DISPLAY_WAVE)
	max_chan = 1;
    else 
	max_chan = channelCount;
    for (cur_chan = 0; cur_chan < max_chan; cur_chan++) {
	cur_x = firstSample;
	if (s->dataFormat == SND_FORMAT_INDIRECT) {
	    int offset = 0;
	    int *p, s2Size, numCoords, numOps, remainingCoords = sampleCount*2;
	    SNDSoundStruct *s2, **iBlock = (SNDSoundStruct **)s->dataLocation;
	    short *src, *srcBreak;
	    int temp = firstSample;

	    s2 = *iBlock++;
	    s2Size = s2->dataSize/(sizeof(short)*channelCount);
	    while (temp > s2Size) {
		temp -= s2Size;
		s2 = *iBlock++;
		s2Size = s2->dataSize/(sizeof(short)*channelCount);
	    }
	    src = (short *)((int)s2+s2->dataLocation);
	    srcBreak = (short *)((int)src + s2->dataSize);
	    src = (short *)((char *)src + 
	    		(temp*channelCount+cur_chan)*sizeof(short));
	    while (remainingCoords > 0) {
		numCoords = (remainingCoords > USERPATHSIZE)?
					     USERPATHSIZE : remainingCoords;
		i = numCoords / 2;
		numOps = i+1;
		p = userPath;
		if (mode == SK_DISPLAY_MINMAX) {
		    while (i--) {
			*p++ = ((cur_x++)<<1) + 1;
			*p++ = ((int)(*src))<<1;
			src += channelCount;
			if (src >= srcBreak) {
			    if (s2 = *iBlock++) {
				src = (short *)((int)s2+s2->dataLocation);
				srcBreak = (short *)((int)src + s2->dataSize);
				src = (short *)((char *)src + 
						(cur_chan * sizeof(short)));
			    } else {
				iBlock--;
				src -= channelCount;
			    }
			}
		    }
		} else if (mode == SK_DISPLAY_WAVE) {
		    while (i--) {
			*p++ = ((cur_x++)<<1);
			*p++ = ((int)(src[offset]))<<1;
			src += channelCount;
			offset = offset? 0 : 1;
			if (src >= srcBreak) {
			    if (s2 = *iBlock++) {
				src = (short *)((int)s2+s2->dataLocation);
				srcBreak = (short *)((int)src + s2->dataSize);
				src = (short *)((char *)src + 
						(cur_chan * sizeof(short)));
			    } else {
				iBlock--;
				src -= channelCount;
			    }
			}
		    }
		}
		DPSDoUserPath(userPath,numCoords,dps_long+1,
			      userPathOps,numOps,
			      userPathBBox,dps_ustroke);
		if (remainingCoords -= numCoords) {
		    cur_x -= 1;
		    remainingCoords += 2;
		}
	    }
	 } else {
	    int offset = 0;
	    short *src = (short *)((unsigned char *)s+s->dataLocation +
	    		(firstSample*s->channelCount+cur_chan)*sizeof(short));
	    int *p, numCoords, numOps, remainingCoords = sampleCount*2;
	    while (remainingCoords > 0) {
		numCoords = (remainingCoords > USERPATHSIZE)?
					     USERPATHSIZE : remainingCoords;
		i = numCoords / 2;
		numOps = i+1;
		p = userPath;
		if (mode == SK_DISPLAY_MINMAX) {
		    while (i--) {
			*p++ = ((cur_x++)<<1) + 1;
			*p++ = ((int)(*src))<<1;
			src += channelCount;
		    }
		} else if (mode == SK_DISPLAY_WAVE) {
		    while (i--) {
			*p++ = ((cur_x++)<<1) + 1;
			*p++ = ((int)(src[offset]))<<1;
			src += channelCount;
			offset = offset? 0 : 1;
		    }
		}
		DPSDoUserPath(userPath,numCoords,dps_long+1,
			      userPathOps,numOps,
			      userPathBBox,dps_ustroke);
		if (remainingCoords -= numCoords) {
		    cur_x -= 1;
		    remainingCoords += 2;
		}
	    }
	 }
     }
}

/*
 * Calculating a display reduction.
 * NOTE: this code is currently non-reentrant.
 */

static unsigned char *dataPointer=0, *dataBreak=0;
static int dataSize=0;
static int dataFormat=0;
static int dataIndirect=0;
static int dataChannels=0;
static SNDSoundStruct **iBlock=0, *currentFrag=0;

static short convertSampToShort(int format, void *theSamp)
{
    switch (format) {
	case SND_FORMAT_LINEAR_16:
	case SND_FORMAT_EMPHASIZED:
	    return((short)(*(short *)theSamp));
	case SND_FORMAT_MULAW_8:
	    return(SNDiMulaw(*(unsigned char *)theSamp));
	case SND_FORMAT_LINEAR_8:
	    return((short)(*(char *)theSamp));
	case SND_FORMAT_LINEAR_32:
	    return((short)(*(int *)theSamp));
	case SND_FORMAT_FLOAT:
	    return((short)(*(float *)theSamp));
	case SND_FORMAT_DOUBLE:
	    return((short)(*(double *)theSamp));
	default:
	    return 0;
    }
}

static int calcSampSize(int format)
{
    switch (format) {
	case SND_FORMAT_MULAW_8:
	case SND_FORMAT_LINEAR_8:
	    return 1;
	case SND_FORMAT_LINEAR_16:
	case SND_FORMAT_EMPHASIZED:
	    return 2;
	case SND_FORMAT_LINEAR_32:
	case SND_FORMAT_FLOAT:
	    return 4;
	case SND_FORMAT_DOUBLE:
	    return 8;
	default:
	    return 0;
    }
}

static void jumpToSamp(SNDSoundStruct *s, int offset, int channel)
{
    dataFormat = s->dataFormat;
    dataChannels = s->channelCount;
    if (dataFormat == SND_FORMAT_INDIRECT) {
	int theOffset = offset;
	dataIndirect = 1;
	iBlock = (SNDSoundStruct **)s->dataLocation;
	currentFrag = *iBlock++;
	dataFormat = currentFrag->dataFormat;
	while (theOffset > currentFrag->dataSize>>2) {
	    theOffset -= currentFrag->dataSize>>2;
	    currentFrag = *iBlock++;
	}
	dataPointer = (unsigned char *)currentFrag;
	dataPointer += currentFrag->dataLocation;
	dataBreak = dataPointer;
	dataSize = calcSampSize(dataFormat);
	dataBreak += currentFrag->dataSize;
	dataPointer += offset*dataChannels*dataSize;
	dataPointer += dataSize*channel;
    } else {
	dataIndirect = 0;
	dataPointer = (unsigned char *)s + s->dataLocation;
	dataSize = calcSampSize(dataFormat);
	dataPointer += dataSize*channel;
    }
}

static short getSamp()
{
    short temp = convertSampToShort(dataFormat,dataPointer);
    dataPointer += dataSize;
    if (dataIndirect && (dataPointer >= dataBreak)) {
	if (currentFrag = *iBlock++) {
	    dataPointer = (unsigned char *)currentFrag;
	    dataPointer += currentFrag->dataLocation;
	    dataBreak = dataPointer;
	    dataBreak += currentFrag->dataSize;
	} else {
	    iBlock--;
	    dataPointer -= dataSize;
	}
    }
    return temp;
}

static id makeReduction(SoundView *self, Sound *theSound)
// Optimize!!!!
{
    int i, err, destWidth, sourceWidth, format, nchan;
    Sound *theReduction = [Sound new];

    short  *data, min, max, temp1, temp2;

    if (![theSound soundStruct]) {
	err = [theReduction  setDataSize:0
				dataFormat:SND_FORMAT_DISPLAY
				samplingRate:0.0
				channelCount:2
				infoSize:0];
	if (err) {
	    [theReduction free];
	    return nil;
	} else
	    return theReduction;
    }
    format = [theSound soundStruct]->dataFormat;
    nchan = [theSound channelCount];
    sourceWidth = [theSound sampleCount];
    destWidth = (int)((float)sourceWidth/self->reductionFactor);
    
    if (sourceWidth < destWidth) {
#ifdef DEBUG
	printf("makeReduction : sourceWidth < destWidth\n");
#endif DEBUG
	return nil; //?
    }
    if (!calcSampSize(format) && (format != SND_FORMAT_INDIRECT)) return nil;
    theReduction = [Sound new];
    jumpToSamp([theSound soundStruct], 0, 0);
    min = 32767; max = -32768;
    err = [theReduction setDataSize:destWidth*2*sizeof(short)
			    dataFormat:SND_FORMAT_DISPLAY
			    samplingRate:0.0
			    channelCount:2
			    infoSize:0];
    if (err) {
	[theReduction free];
	return nil;
    }
    data = (short *)[theReduction data];
    err = sourceWidth >> 1;
    for (i=0; i<sourceWidth; i++) {
	temp1 = getSamp();
	if (nchan == 2)
	    temp2 = getSamp();
	else
	    temp2 = temp1;
	if (temp1 < temp2) {
	    if (temp1 < min) min = temp1;
	    if (temp2 > max) max = temp2;
	} else {
	    if (temp2 < min) min = temp2;
	    if (temp1 > max) max = temp1;
	}
	err -= destWidth;
	if (err < 0) {
	    err += sourceWidth;
	    *data++ = min;
	    *data++ = max;
	    min = 32767; max = -32768;
	}
    }
    return theReduction;
}

/*
 * Common editing functions
 */
static int replaceSelection(SoundView *self, Sound *newSound,
					Sound *newReduction,BOOL selectIt)
{
    int err;
    BOOL oldAutoDisplay=[self isAutodisplay];
    int rFirstSample = (int)self->selectionRect.origin.x;
    int rSampleCount = (int)self->selectionRect.size.width;
    int firstSample = (int)(self->selectionRect.origin.x * 
    					self->reductionFactor);
    int sampleCount= (int)(self->selectionRect.size.width * 
    					self->reductionFactor);
    int deltaSampleCount = [newSound sampleCount];
    [self hideCursor];
    [self setAutodisplay:NO];
    [self->window disableFlushWindow];
    if (self->sound && [self->sound sampleCount]) {
	if (sampleCount) {
	    err = [self->sound deleteSamplesAt:firstSample 
						    count:sampleCount];
	    if (err) goto cannot_delete;
	    err = [self->reduction deleteSamplesAt:rFirstSample 
						    count:rSampleCount];
	}
	if ([newSound sampleCount]) {
	    id s, r;
	    s = [newSound copy];
	    if (![self->sound compatibleWith:s]) {
		err = [s convertToFormat:[self->sound dataFormat]
				  samplingRate:[self->sound samplingRate]
				  channelCount:[self->sound channelCount]];
		if (err) goto cannot_insert;
		deltaSampleCount = [s sampleCount];
		r = makeReduction(self, s);
	    } else {
		s = newSound;
		r = newReduction? newReduction : makeReduction(self, s);
	    }
	    err = [self->sound insertSamples:s at:firstSample];
	    if (!err)
		err = [self->reduction insertSamples:r at:rFirstSample];
	    if (err)
		goto cannot_insert;
	}
    } else {
	if (self->sound) 
	    [self->sound copySound:newSound];
	else
	    self->sound = [newSound copy];
	if (newReduction) {
	    if (!self->reduction) self->reduction = [Sound new];
	    [self->reduction copySound:newReduction];
	} else {
	    self->reduction = makeReduction(self, newSound);
	}
	firstSample = 0;
    }
    self->svFlags.calcDrawInfo = NO;
    if (self->svFlags.autoscale)
	[self scaleToFit];
    else
	[self sizeToFit];
    [self tellDelegate:@selector(soundDidChange:)];
    if (selectIt)
	[self setSelection:firstSample size:deltaSampleCount];
    else
	[self setSelection:firstSample+deltaSampleCount size:0];
    err = 0;
 normal_exit:
    [self->window reenableFlushWindow];
    [self setAutodisplay:oldAutoDisplay];
    if ([self isAutodisplay])
	[self display];
    return err;
 cannot_insert:
    self->svFlags.calcDrawInfo = NO;
    if (self->svFlags.autoscale)
	[self scaleToFit];
    else
	[self sizeToFit];
    [self tellDelegate:@selector(soundDidChange:)];
 cannot_delete:
    goto normal_exit;
}

/*
 * Methods
 */

+ newFrame:(const NXRect *)aRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:aRect];
}

- initFrame:(const NXRect *)aRect
{
    [super initFrame:aRect];
    foregroundGray = NX_BLACK;
    backgroundGray = NX_WHITE;
    reductionFactor = 1.0;
    [self setAutodisplay:NO];
    [self setDrawSize:bounds.size.width : 65536];
    [self setDrawOrigin:0 :-32768.0];
    self->selectionRect.origin.x = 0.0;
    self->selectionRect.origin.y = -32768.0;
    self->selectionRect.size.width = 0.0;
    self->selectionRect.size.height = 65536.0;
    [self setAutodisplay:YES];
    return self;
}

- free
{
    [_scratchSound stop];
    [self stop:nil];
    [self hideCursor];
    [self tellDelegate:@selector(willFree:)];
    if (reduction) [reduction free];
    if (_scratchSound) [_scratchSound free];
    return [super free];
}

- write:(NXTypedStream *)stream
{
    [super write: stream];
    NXWriteTypes(stream, "@@is",&sound,&delegate, &displayMode, &svFlags);
    NXWriteTypes(stream, "ff",&backgroundGray,&foregroundGray);
    NXWriteArray(stream, "c", sizeof(NXRect), &selectionRect);
//    NXWriteTypes(stream, "f",&reductionFactor);
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read: stream];
    NXReadTypes(stream, "@@is", &sound, &delegate, &displayMode, &svFlags);
    NXReadTypes(stream, "ff", &backgroundGray, &foregroundGray);
    NXReadArray(stream, "c", sizeof(NXRect), &selectionRect);
//    if (NXSystemVersion(stream) < NXSYSTEMVERSION0905) {
    if (0) // new version
	NXReadTypes(stream, "f", &reductionFactor);
    else if (sound && frame.size.width)
	reductionFactor = [sound sampleCount]/frame.size.width;
    else
	reductionFactor = 32.0;
    svFlags.calcDrawInfo = YES;
    svFlags.selectionDirty = YES;
    _scratchSound = nil;
    return self;
}

- (BOOL)isEnabled
{
    return (svFlags.disabled ? NO : YES);
}

- setEnabled:(BOOL)aFlag
{
    [self hideCursor];
    svFlags.disabled = (aFlag ? NO : YES);
    [self showCursor];
    return self;
}

- (BOOL)isEditable
{
    return (svFlags.notEditable ? NO : YES);
}

- setEditable:(BOOL)aFlag
{
    if (svFlags.disabled) return self;
    [self hideCursor];
    svFlags.notEditable = (aFlag ? NO : YES);
    [self showCursor];
    return self;
}

- (BOOL)isContinuous
{
    return svFlags.continuous;
}

- setContinuous:(BOOL)aFlag
{
    svFlags.continuous = (aFlag ? YES : NO);
    return self;
}

- (BOOL)isBezeled
{
    return svFlags.bezeled;
}

- setBezeled:(BOOL)aFlag
{
    BOOL oldFlag = svFlags.bezeled;
    svFlags.bezeled = (aFlag ? YES : NO);
    if (!oldFlag && aFlag) {
	NXRect temp = frame;
	NXInsetRect(&temp,2.0,2.0);
	[self setFrame:&temp];
    } else if (oldFlag && !aFlag) {
	NXRect temp = frame;
	NXInsetRect(&temp,-2.0,-2.0);
	[self setFrame:&temp];
    }
    if ([self isAutodisplay])
	[self display];
    return self;
}

- setBackgroundGray:(float)aGray
{
    backgroundGray = aGray;
    if ([self isAutodisplay])
	[self display];
    return self;
}

- (float)backgroundGray
{
    return backgroundGray;
}

- setForegroundGray:(float)aGray
{
    foregroundGray = aGray;
    if ([self isAutodisplay])
	[self display];
    return self;
}

- (float)foregroundGray;
{
    return foregroundGray;
}

- (int)displayMode
{
    return displayMode;
}

- setDisplayMode:(int)aMode
{
    displayMode = aMode;
    if ([self isAutodisplay])
	[self display];
    return self;
}

- (BOOL)isAutoScale
{
    return svFlags.autoscale;
}

- setAutoscale:(BOOL)aFlag
{
    svFlags.autoscale = aFlag? YES : NO;
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

- sound
{
    return sound;
}

- setSound:aSound
{
    BOOL oldAutoDisplay=[self isAutodisplay];
    [self setAutodisplay:NO];
    [window disableFlushWindow];
    sound = aSound;
    svFlags.selectionDirty = YES;
    if (svFlags.autoscale)
	[self scaleToFit];
    else
	[self sizeToFit];
    svFlags.calcDrawInfo = YES;
    [window reenableFlushWindow];
    [self setAutodisplay:oldAutoDisplay];
    if ([self isAutodisplay])
	[self display];
    return self;
}

- reduction
{
    return reduction;
}

- setReduction:aDisplayReduction
{
    float newFactor = [sound sampleCount] / [aDisplayReduction sampleCount];
    BOOL oldAutoDisplay=[self isAutodisplay];
    if (newFactor < 1.0 || newFactor != reductionFactor) return nil;
    [self setAutodisplay:NO];
    if (reduction) [reduction free];
    reduction = aDisplayReduction;
    svFlags.calcDrawInfo = NO;
    [self setAutodisplay:oldAutoDisplay];
    if ([self isAutodisplay])
	[self display];
    return self;
}

- (float)reductionFactor
{
    return reductionFactor;
}

- setReductionFactor:(float)theReductionFactor
{
    if (theReductionFactor == reductionFactor) return self;
    if (!svFlags.autoscale && (theReductionFactor >= 1.0)) {
	BOOL oldAutoDisplay=[self isAutodisplay];
	[self setAutodisplay:NO];
	[window disableFlushWindow];
    	reductionFactor = theReductionFactor;
	if (svFlags.autoscale)
	    [self scaleToFit];
	else
	    [self sizeToFit];
	svFlags.calcDrawInfo = YES;
	[self setAutodisplay:oldAutoDisplay];
	[window reenableFlushWindow];
	if ([self isAutodisplay])
	    [self display];
	return self;
    } else
	return nil;
}

#define MIN_FRAME_WIDTH (32.0)

- sizeToFit
{
    float sampCount, theWidth = MIN_FRAME_WIDTH;
    if (sound && [sound soundStruct]) {
	sampCount = (float)[sound sampleCount];
	if (sampCount && (sampCount > MIN_FRAME_WIDTH))
	    theWidth = sampCount / reductionFactor;
    }
    [super sizeTo:theWidth :frame.size.height];
    selectionRect.size.width = 0.0;
    return self;
}

- scaleToFit
{
    float samples = (float)[sound sampleCount];
    float newFactor;
    if (samples > 0.0)
	newFactor = samples / frame.size.width;
    else
	newFactor = 1.0;
    if (newFactor != reductionFactor) {
	reductionFactor = newFactor;
	svFlags.calcDrawInfo = YES;
    }
    return self; 
}

- sizeTo:(NXCoord)width :(NXCoord)height
{
    BOOL oldAutoDisplay=[self isAutodisplay];
    [self setAutodisplay:NO];
    [super sizeTo:width :height];
    [self setDrawSize:bounds.size.width : 65536];
    [self setDrawOrigin:0 :-32768.0];
    [self setAutodisplay:oldAutoDisplay];
    if ([self isAutodisplay])
	[self display];
    return self;
}

- calcDrawInfo
{
    if (svFlags.calcDrawInfo) {
	if (reduction) [reduction free];
	reduction = sound? makeReduction(self, sound) : nil;
	svFlags.calcDrawInfo = NO;
    }
     return self;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    int begin, end;
    NXRect theRect;
    if (![self getVisibleRect:&theRect]) return self;
    NXIntersectionRect(rects,&theRect);
    NXIntegralRect(&theRect);
    begin = (int)theRect.origin.x;
    end = begin + (int)theRect.size.width + 1;
    if (!selectionRect.size.width)
	[self hideCursor];
    else
	NXHighlightRect(&selectionRect);
    if (svFlags.calcDrawInfo)
	[self calcDrawInfo];
    if (svFlags.bezeled) {
	NXRect temp = frame;
	[[self superview] lockFocus];
	NXInsetRect(&temp,-2.0,-2.0);
	NXDrawGrayBezel(&temp,NULL);
	PSsetgray(backgroundGray);
	NXRectFill(&frame);
	[[self superview] unlockFocus];
    } else {
	PSsetgray(backgroundGray);
	NXRectFill(&theRect);
    }
    if (sound) {
	PSsetgray(foregroundGray);
	PSsetlinewidth(0.0);
	drawSound(self,reduction,begin,end);
    }
    if (!selectionRect.size.width)
	[self showCursor];
    else
	NXHighlightRect(&selectionRect);
    return self;
}

static int extendSelection(SoundView *self, NXPoint *curPoint,
						 NXPoint *basePoint)
{
    static NXPoint lastPoint = {0,0};
    if (lastPoint.x == curPoint->x)
	return 0;
    lastPoint = *curPoint;
    NXHighlightRect(&self->selectionRect);
    if (curPoint->x < basePoint->x) {
	self->selectionRect.origin.x = curPoint->x;
	self->selectionRect.size.width = basePoint->x - curPoint->x;
    } else {
	self->selectionRect.origin.x = basePoint->x;
	self->selectionRect.size.width = curPoint->x - basePoint->x;
    }
    NXHighlightRect(&self->selectionRect);
    [self->window flushWindow];
    return 1;
}

#define SVEVENTMASK (NX_MOUSEUPMASK | NX_MOUSEDRAGGEDMASK | NX_TIMERMASK)

static void fixX(SoundView *self, NXPoint *aPoint)
{
    int temp, max = [self->sound sampleCount];
    if (aPoint->x < 0)
	temp = 0;
    else if (aPoint->x > max)
	temp = max;
    else
	temp = (int)aPoint->x;
    aPoint->x = (NXCoord)temp;
}

- hideCursor
{
    stopCaret(self);
    return self;
}

- showCursor
{
    NXRect temp = selectionRect;
    if (temp.size.width || !sound || svFlags.disabled || svFlags.notEditable)
	stopCaret(self);
    else
	startCaret(self);
    return self;
}

- mouseDown:(NXEvent *)theEvent
{
    NXEvent lastRealEvent, *event=theEvent;
    NXPoint curPoint, basePoint, mouseLocation;
    NXRect vRect;
    int oldMask, notDone = 1;

    NXTrackingTimer timer;	/* timer events used for auto scroll */

    if (svFlags.disabled) return self;
    oldMask = [window addToEventMask:SVEVENTMASK];
    [self lockFocus];
    if (selectionRect.size.width)
	NXHighlightRect(&selectionRect);
    else
	[self hideCursor];
    selectionRect.size.height = bounds.size.height;
    selectionRect.origin.y = bounds.origin.y;
    [self getVisibleRect:&vRect];
    curPoint = event->location;
    [self convertPoint:&curPoint fromView:nil];
    mouseLocation = curPoint;
    fixX(self,&curPoint);
    if (NX_SHIFTMASK & event->flags) {
	basePoint.y = curPoint.y;
	if (curPoint.x < 
		(selectionRect.origin.x + (selectionRect.size.width/2))) {
	    basePoint.x = selectionRect.origin.x + selectionRect.size.width;
	    selectionRect.size.width = basePoint.x - curPoint.x;
	    selectionRect.origin.x = curPoint.x;
	} else {
	    basePoint.x = selectionRect.origin.x;
	    selectionRect.size.width = curPoint.x - selectionRect.origin.x;
	}
    } else {
	basePoint = curPoint;
	selectionRect.origin.x = curPoint.x;
	selectionRect.size.width = 0.0;
    }
    NXHighlightRect(&selectionRect);
    [window flushWindow];
    NXBeginTimer(&timer,0.1,0.1);
    while (notDone) {
	switch (event->type) {
	    case NX_MOUSEUP:
		notDone = 0;
	        break;
	    case NX_MOUSEDRAGGED:
		curPoint = event->location;
		[self convertPoint:&curPoint fromView:nil];
		fixX(self,&curPoint);
		lastRealEvent = *event;
		if (extendSelection(self,&curPoint,&basePoint) &&
							svFlags.continuous)
		    [self tellDelegate:@selector(selectionChanged:)];
		break;
	    case NX_TIMER:
		if (!NXPointInRect(&curPoint, &vRect)) {
		    [self autoscroll:&lastRealEvent];
		    NXPing();
		    [self getVisibleRect:&vRect];
		    curPoint = lastRealEvent.location;
		    [self convertPoint:&curPoint fromView:nil];
		    fixX(self,&curPoint);
		    if (extendSelection(self,&curPoint,&basePoint) &&
							    svFlags.continuous)
			[self tellDelegate:@selector(selectionChanged:)];
		    }
		break;
	}
	event = [NXApp getNextEvent:SVEVENTMASK];
    }
    svFlags.selectionDirty = YES;
    [self tellDelegate:@selector(selectionChanged:)];
    [self unlockFocus];
    NXEndTimer(&timer);
    DPSDiscardEvents(DPSGetCurrentContext(),
			(NX_KEYDOWNMASK | NX_KEYUPMASK | SVEVENTMASK));
    [window setEventMask:oldMask];
    if (!selectionRect.size.width) {
	[self showCursor];
    }
    [window flushWindow];
    NXPing();
    return self;
}

- tellDelegate:(SEL)theMessage
{
    if (theMessage && delegate && [delegate respondsTo:theMessage])
	[delegate perform:theMessage with:self];
    return self;
}

- getSelection:(int *)firstSample size:(int *)sampleCount
{
    *firstSample = (int)(selectionRect.origin.x * reductionFactor);
    *sampleCount = (int)(selectionRect.size.width * reductionFactor);
    return self;
}


- setSelection:(int)firstSample size:(int)sampleCount
{
    int max = [sound sampleCount];
    int offset=firstSample, count=sampleCount;
    if (!sound) return self;
    if ([self isAutodisplay]) {
	[self lockFocus];
	if (!selectionRect.size.width)
	    [self hideCursor];
	else
	    NXHighlightRect(&selectionRect);
    }
    if (offset < 0) offset = 0;
    else if (offset > max) offset = max;
    if ((firstSample+sampleCount) > max) count = max - offset;
    selectionRect.origin.x = offset/reductionFactor;
    selectionRect.size.width = count/reductionFactor;
    if ([self isAutodisplay]) {
	if (!selectionRect.size.width)
	    [self showCursor];
	else
	    NXHighlightRect(&selectionRect);
	[self unlockFocus];
	[window flushWindow];
    }
    svFlags.selectionDirty = YES;
    [window makeFirstResponder:self];
    [self tellDelegate:@selector(selectionChanged:)];
    return self;
}

- play:sender
{
    int err;
    int firstSample = (int)(selectionRect.origin.x * reductionFactor);
    int sampleCount = (int)(selectionRect.size.width * reductionFactor);
    [self stop:sender];
    if (sound) {
	if (!sampleCount) {
	    if (!_scratchSound) {
		_scratchSound = [Sound new];
		[_scratchSound setDelegate:self];
	    }
	    [_scratchSound copySound:sound];
	    svFlags.selectionDirty = YES;
	} else {
	    if (!_scratchSound) {
		_scratchSound = [Sound new];
		[_scratchSound setDelegate:self];
		svFlags.selectionDirty = YES;
	    }
	    if (svFlags.selectionDirty) {
		err = [_scratchSound copySamples:sound at:firstSample 
							    count:sampleCount];
		svFlags.selectionDirty = NO;
	    }
	}
	[_scratchSound play:nil];
    } else {
	[self willPlay:nil];
	[self didPlay:nil];
    }
    return self;
}


- stop:sender
{
    int err = [_scratchSound stop];
    if (err)
	[self tellDelegate:@selector(hadError:)];
    return self;
}

- record:sender
{
    int err;
    [self stop:sender];
    if (!_scratchSound) {
	_scratchSound = [Sound new];
	[_scratchSound setDelegate:self];
	svFlags.selectionDirty = YES;
    }
    err = [_scratchSound record];
    return self;
}

- pause:sender
{
    if (!_scratchSound || ![_scratchSound status]) return nil;
    [_scratchSound pause:self];
    return self;
}

- resume:sender
{
    if (!_scratchSound || ![_scratchSound status]) return nil;
    [_scratchSound resume:self];
    return self;
}


/*
 * Editing methods
 */

static const char *NXSoundDisplayPboard = "NeXT sound display pasteboard type";


- (BOOL)acceptsFirstResponder
{
    return svFlags.disabled? NO : YES;
}

- becomeFirstResponder
{
    [self showCursor];
    return self;
}

- resignFirstResponder
{
    [self hideCursor];
    return self;
}

- selectAll:sender
{
    if (sound)
	[self setSelection:0 size:[sound sampleCount]];
    return self;
}

- delete:sender
{
    if (svFlags.notEditable) return self;
    replaceSelection(self,nil,nil,NO);
    return self;
}

- cut:sender
{
    if (svFlags.notEditable) return self;
    [self copy:sender];
    [self delete:sender];
    return self;
}

static id copiedReduction=nil;
static id copiedSound=nil;
static int copiedRefnum=0;

- provideData:(const char *)type
{
    id thePboard = [Pasteboard new];
    int err;
    if (!strcmp(type,NXSoundPboardType)) {
	err = [copiedSound compactSamples];
	[thePboard writeType:NXSoundPboardType 
			data:(char *)[copiedSound soundStruct]
			length:[copiedSound soundStructSize]];
    } else if (!strcmp(type,NXSoundDisplayPboard)) {
	err = [copiedReduction compactSamples];
	[thePboard writeType:NXSoundDisplayPboard 
			data:(char *)[copiedReduction soundStruct]
			length:[copiedReduction soundStructSize]];
    }
    return self;
}

- copy:sender
{
    const char *typeList[2];
    id thePboard;
    int err;
    int rFirstSample = (int)selectionRect.origin.x;
    int rSampleCount = (int)selectionRect.size.width;
    int firstSample = rFirstSample*reductionFactor;
    int sampleCount = rSampleCount*reductionFactor;
    if (!copiedSound) copiedSound = [Sound new];
    err = [copiedSound copySamples:sound
    			   at:firstSample count:sampleCount];
#ifdef DEBUG
    if (err) printf("Cannot copy sound : %s\n", SNDSoundError(err));//
#endif
    if (reduction) {
	if (!copiedReduction) copiedReduction = [Sound new];
	err = [copiedReduction copySamples:reduction 
				   at:rFirstSample count:rSampleCount];
#ifdef DEBUG
	if (err) printf("Cannot copy reduction : %s\n", SNDSoundError(err));//
#endif
    }
    typeList[0] = NXSoundPboardType;
    typeList[1] = NXSoundDisplayPboard;
    thePboard = [Pasteboard new];
    [thePboard declareTypes:typeList num:2 owner:self];
    copiedRefnum = [thePboard changeCount];
    return self;
}

static id replaceSoundStruct(id theSound, char *s, int slen)
{
    id sound;
    if (!slen) {
	if (theSound) [theSound free];
	return nil;
    }
    sound = theSound? theSound : [Sound new];
    [sound setSoundStruct:(SNDSoundStruct *)s soundStructSize:slen];
    return sound;
}

- paste:sender
{
    int err;
    id thePboard = [Pasteboard new];
    BOOL hasSoundData = NO, hasSoundReduction = NO;
    int changeCount;
    char **types;

    if (svFlags.notEditable) return self;
    if (!thePboard) return self;
    changeCount = [thePboard changeCount];
    types = (char **)[thePboard types];
    while (*types) {
	char *theType = *types++;
	if (!strcmp(theType, NXSoundPboardType))
	     hasSoundData = YES;
	else if (!strcmp(theType, NXSoundDisplayPboard))
	     hasSoundReduction = YES;
    }
    if (!hasSoundData) return self;
    if (!copiedSound || (changeCount != copiedRefnum)) {
	char *s;
	int slen;
	copiedRefnum = changeCount;
	[thePboard readType:NXSoundPboardType data:&s length:&slen];
	copiedSound = replaceSoundStruct(copiedSound,s,slen);
	if (hasSoundReduction) {
	    [thePboard readType:NXSoundDisplayPboard data:&s length:&slen];
	    copiedReduction = replaceSoundStruct(copiedReduction,s,slen);
	} else
	    copiedReduction = nil;
    }
    if (copiedSound) {
	if (![sound compatibleWith:copiedSound]) {
	    id tempReduction, tempSound = [copiedSound copy];
	    err = [tempSound convertToFormat:[sound dataFormat]
	    			samplingRate:[sound samplingRate]
				channelCount:[sound channelCount]];
	    if (!err) {
		tempReduction = makeReduction(self, tempSound);
		replaceSelection(self,tempSound,tempReduction,NO);
	    }	
	} else {
	    if (copiedReduction) {
		int i = [copiedSound sampleCount];
		int j = [copiedReduction sampleCount];
		if (!i || !j || (i/j != reductionFactor))
		    copiedReduction = makeReduction(self, copiedSound);
	    } else
		copiedReduction = makeReduction(self, copiedSound);
	    replaceSelection(self,copiedSound,copiedReduction,NO);
	}
    } else {
	//cannot paste !!! error?
    }
    return self;
}

- didPlay:sender
{
    [self tellDelegate:@selector(didPlay:)];
    return self;
}

- willPlay:sender
{
    [self tellDelegate:@selector(willPlay:)];
    return self;
}

- didRecord:sender
{
    replaceSelection(self,_scratchSound,nil,YES);
    [self tellDelegate:@selector(didRecord:)];
    return self;
}

- willRecord:sender
{
    [self tellDelegate:@selector(willRecord:)];
    return self;
}

- hadError:sender
{
    [self tellDelegate:@selector(hadError:)];
    return self;
}

- soundBeingProcessed
{
    return _scratchSound;
}

- windowChanged:newWindow
{
    [self hideCursor];
    return [super windowChanged:newWindow];
}

@end

/*

Modification History:

soundkit-25
=======================================================================================
11 Sept 90 (wot)	Changed drawing of caret to use zero width lines rather than a
			NXFillRect on a 1 pixel wide rect.
12 Sept 90 (wot)	Implemented "windowChanged:" to hide the caret.

20 Sept 90 (wot)	Added support for SND_FORMAT_EMPHASIZED.  Made it do the same
			things as SND_FORMAT_LINEAR_16.
 8 Oct	90 (wot)	Changed NXSoundPboard to NXSoundPboardType.
*/