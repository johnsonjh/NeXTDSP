#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	SoundMeter.h
 *	Written by Lee Boynton
 *	Copyright 1988-89 NeXT, Inc.
 *
 *	Modification History:
 *	10/12/90/mtm	Give up CPU more often when playing (bug #10591).
 *	10/12/90/mtm	Adjust BREAK_DELAY and timed entry period
 *			(bug #6312).
 */

#import <appkit/Application.h>
#import <appkit/Window.h>
#import <dpsclient/wraps.h>
#import <dpsclient/dpsNeXT.h>
#import <sys/time_stamp.h>
#import <zone.h>

#import "SoundMeter.h"
#import "Sound.h"

extern int kern_timestamp();

@implementation SoundMeter

static void calcAverage(SNDSoundStruct *s, int begin, int end,
						int *average, int *max)
{
    unsigned char *p;
    int i, m = 0, nchan;
    if (!s || (end <= begin)) {
	*average = *max = 0.0;
	return;
    }
    nchan = s->channelCount;
    p = (unsigned char *)s;
    p += s->dataLocation;
    if (s->dataFormat == SND_FORMAT_MULAW_8) {
	int a = 0, temp;
	p += (begin*nchan);
	for (i=begin; i<end; i++) {
	    temp = SNDiMulaw(*p);
	    p += nchan;
	    temp = ((temp > 0)? temp : -temp);
	    a += temp;
	    if (temp > m) m = temp;
	}
	a /= (end-begin);
	*average = a;
	*max = m;
    } else if (s->dataFormat == SND_FORMAT_LINEAR_16 || 
    	       s->dataFormat == SND_FORMAT_EMPHASIZED) {
	int a = 0, temp;
	short *sp = (short *)p;
	sp += (begin*nchan);
	for (i=begin; i<end; i++) {
	    temp = *sp;
	    sp += nchan;
	    temp = ((temp > 0)? temp : -temp);
	    a += temp;
	    if (temp > m) m = temp;
	}
	a /= (end-begin);
	*average = a;
	*max = m;
    } else if (s->dataFormat == SND_FORMAT_INDIRECT) {
	SNDSoundStruct *ss, **iBlock = (SNDSoundStruct **)s->dataLocation;
	int p = 0, temp, ave_count = 0;
	int a=0, e, b = 0, temp_ave, temp_max;
	ss = *iBlock++;
	if (!ss) return;
	temp = SNDSampleCount(ss);
	while ((p+temp-1) <= begin) {
	    p += temp;
	    ss = *iBlock++;
	    if (!ss) return;
	    temp = SNDSampleCount(ss);
	}
	b = begin - p;
	while ((p+temp-1) <= end) {
	    e = temp-1;
	    p += temp;
	    if (e > b) {
		calcAverage(ss,b,e,&temp_ave, &temp_max);
		a += (temp_ave*(e-b));
		if (temp_max > m) m = temp_max;
		ave_count += (e-b);
	    }
	    ss = *iBlock++;
	    if (!ss) goto abnormal_exit;
	    temp = SNDSampleCount(ss);
	    b = 0;
	}
	e = end - p;
	if (e>b) {
	    calcAverage(ss,b,e,&temp_ave, &temp_max);
	    a += temp_ave*(e-b);
	    if (temp_max > m) m = temp_max;
	    ave_count += (e-b);
	}
 abnormal_exit:
 	if (ave_count) a /= ave_count;
	*average = a;
	*max = m;
    } else
	*max = *average = 0;
}

static float smoothValue(SoundMeter *self, float aValue)
{
    float newValue;
    
    if (aValue >= self->currentPeak)
	newValue = aValue;
    else
	newValue = (2*aValue+2*self->_valOneAgo+self->_valTwoAgo)/5.0;
    self->_valTwoAgo = self->_valOneAgo;
    self->_valOneAgo = aValue;
    return (aValue > 0)? newValue : aValue;
}


static float prepareValueForDisplay(id self, float m)
{
    float result;
    int val = (m > 0)? 32767.0 * m  :  0;
    int temp = (int)SNDMulaw(val);
    temp = ~temp & 127;
    result = ((float)(temp))/128.0;
    return result;
}

#define MAXWINDOWSIZE 50

static void calcValues(SoundMeter *self, float *aveVal, float *peakVal)
{
    int samp, ave, max=0;
    SNDSoundStruct *s = [self->sound soundStructBeingProcessed];
    samp = [self->sound samplesProcessed];
    if (self->currentSample >= samp || ![self->sound status] ||
    	 [self->sound status] == SK_STATUS_RECORDING_PAUSED ||
	 [self->sound status] == SK_STATUS_PLAYING_PAUSED) {
	*peakVal = self->currentValue * 0.7;
	self->currentSample = samp;
    } else {
	int i, j, temp_ave=-1, temp_max=-1, win_count = 0, win_ave = 0;
	for (i=self->currentSample; i<samp; i+= MAXWINDOWSIZE) {
	    j = ((i+MAXWINDOWSIZE) > samp)? samp : (i+MAXWINDOWSIZE);
	    calcAverage(s,i,j,&temp_ave,&temp_max);
	    if (!win_count) {
		win_ave = temp_ave*(j-i);
		max = temp_max;
	    } else {
		if (temp_max > max) max = temp_max;
		win_ave += temp_ave*(j-i);
	    }
	    win_count += (j-i);
	}
	if (win_count) {
	    ave = win_ave / win_count;
	    *aveVal = (float)ave / 32767.0;
	}
	*peakVal = (float)max / 32767.0;
	self->currentSample = samp;
    }
}

static int shouldBreak(SoundMeter *self)
{
   NXEvent event;
   int result;
   int status = [self->sound status];

   /* Always give up the CPU when playing. */
   if (status == SK_STATUS_PLAYING)
       return 1;
   result = [NXApp peekNextEvent:NX_ALLEVENTS into:&event 
	     waitFor:0.0 threshold:NX_BASETHRESHOLD]? 1 : 0;
   return result || !status || self->smFlags.shouldStop;
}


#define DONE_DELAY (10)
#define BREAK_DELAY (0)

static void animate_self(DPSTimedEntry timedEntry, double now,
							 SoundMeter *self)
{
    static int stopDelay = DONE_DELAY;
    BOOL oldAutoDisplay = [self isAutodisplay];
    int breakDelay = BREAK_DELAY;
    float aveVal, peakVal;

    [self setAutodisplay:NO];
    [self lockFocus];
    if ([self->sound status] && !self->smFlags.shouldStop)
	stopDelay = DONE_DELAY;
    else
	stopDelay--;
    if (!stopDelay) {
	[self setFloatValue:-1.0];
	[self drawCurrentValue];
	[self->window flushWindow];
	DPSRemoveTimedEntry(self->_timedEntry);
	self->_timedEntry = 0;
	self->smFlags.running = NO;
	stopDelay = DONE_DELAY;
    } else {
	while(1) {
	    if (self->sound) {
		calcValues(self, &aveVal, &peakVal);
		if (aveVal < self->minValue) self->minValue = aveVal;
		if (aveVal > self->maxValue) self->maxValue = aveVal;
	    } else
		self->minValue = self->maxValue = aveVal = peakVal = 0.0;
	    [self setFloatValue:peakVal];
	    [self drawCurrentValue];
	    [self->window flushWindow];
	    NXPing();
	    if (!breakDelay) break;
	    else if (shouldBreak(self)) breakDelay--;
	}
    }
    [self unlockFocus];
    [self setAutodisplay:oldAutoDisplay];
}

/**********************************************************************
 *
 * Exports
 *
 */

+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(const NXRect *)frameRect
{
    [super initFrame:frameRect];
    holdTime = 0.7; // in seconds
    backgroundGray = NX_DKGRAY;
    foregroundGray = NX_LTGRAY;
    peakGray = NX_WHITE;
    smFlags.bezeled = YES;
    return self;
}

- (float)floatValue { return currentValue; }
- (float)peakValue { return currentPeak; }
- (float)minValue { return minValue; }
- (float)maxValue { return maxValue; }

- setHoldTime:(float)seconds { holdTime = seconds; return self;}
- (float)holdTime { return holdTime; }

- (float)backgroundGray { return backgroundGray; }
- setBackgroundGray:(float)aValue
{
    backgroundGray = aValue;
    if ([self isAutodisplay]) [self display];
    return self;
}

- (float)foregroundGray { return foregroundGray; }
- setForegroundGray:(float)aValue
{
    foregroundGray = aValue;
    if ([self isAutodisplay]) [self display];
    return self;
}

- (float)peakGray { return peakGray; }
- setPeakGray:(float)aValue
{
    peakGray = aValue;
    if ([self isAutodisplay]) [self display];
    return self;
}

- sound { return sound; }
- setSound:aSound { sound = aSound; return self;}

- run:sender
{
    if (!smFlags.running && !_timedEntry && sound) {
	float aveVal, peakVal;
	smFlags.running = YES;
	minValue = 1.0;
	maxValue = 0.0;
	currentSample = 0;
	if (sound) {
	    calcValues(self, &aveVal, &peakVal);
	    if (aveVal < minValue) minValue = aveVal;
	    if (aveVal > maxValue) maxValue = aveVal;
	} else
	    minValue = maxValue = aveVal = peakVal = 0.0;
	[self setFloatValue:peakVal];
	[self drawCurrentValue];
	[window flushWindow];
	NXPing();
	_timedEntry = DPSAddTimedEntry(0.05, 
		    (DPSTimedEntryProc)animate_self, self,NX_BASETHRESHOLD);
    }
    smFlags.shouldStop = NO;
    return self;
}

- stop:sender
{
    if (smFlags.running) {
	smFlags.shouldStop = YES;
    }
    return self;
}

- (BOOL)isRunning
{
    return smFlags.running;
}

- (BOOL)isBezeled
{
    return smFlags.bezeled;
}

- setBezeled:(BOOL)aFlag
{
    smFlags.bezeled = aFlag? YES : NO;
    if ([self isAutodisplay]) [self display];
    return self;
}

- setFloatValue:(float)aValue
{
    struct tsval foo;
    double peakDelay;

    if (aValue < 0.0)
	currentValue = currentPeak = aValue;
    else if (aValue > 1.0)
	currentValue = 1.0;
    else
	currentValue = aValue;
    kern_timestamp(&foo);
    _valTime = foo.low_val;
    peakDelay = ((float)(foo.low_val - _peakTime))/1000000.0;
    if (currentValue > currentPeak || peakDelay > holdTime) {
	currentPeak = currentValue;
	_peakTime = foo.low_val;
    }
    if ([self isAutodisplay]) {
	[self lockFocus];
	[self drawCurrentValue];
	[self unlockFocus];
	[window flushWindow];
    }
    return self;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    NXRect temp = bounds;
    if (smFlags.bezeled) {
	NXDrawGrayBezel(&bounds,NULL);
	NXInsetRect(&temp,2.0,2.0);
    }
    PSsetgray(backgroundGray);
    NXRectFill(&temp);
    [self drawCurrentValue];
    return self;
}

- drawCurrentValue
{
    #define PEAK_WIDTH (3.0)
    NXCoord x, y, w, h;
    NXCoord valueOffset, peakOffset;
    float displayValue = prepareValueForDisplay(self,smoothValue(self,
    								currentValue));
    float displayPeak = prepareValueForDisplay(self,currentPeak);
    x = bounds.origin.x + 5.0;
    y = bounds.origin.y + 5.0;
    w = bounds.size.width - 9.0;
    h = bounds.size.height - 9.0;
    valueOffset = (w - PEAK_WIDTH) * displayValue;
    peakOffset = (w - PEAK_WIDTH) * displayPeak;
    if (peakOffset > 0.0) {
	if (valueOffset > 0.0) {
	    PSsetgray(foregroundGray);
	    PSrectfill(x,y,valueOffset,h);
	    PSsetgray(backgroundGray);
	    PSrectfill(x+valueOffset,y,w-valueOffset,h);
	} else {
	    PSsetgray(backgroundGray);
	    PSrectfill(x,y,w,h);
	}
	PSsetgray(peakGray);
	PSrectfill(x+peakOffset,y,PEAK_WIDTH,h);
    } else {
	PSsetgray(backgroundGray);
	PSrectfill(x,y,w,h);
    }
    return self;
}

- write:(NXTypedStream *)stream
{
    [super write: stream];
    NXWriteTypes(stream, "@ffffffffs",&sound,&currentValue,
    			&currentPeak, &minValue, &maxValue,
			&holdTime, &backgroundGray, &foregroundGray,&peakGray,
			&smFlags);
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read: stream];
    NXReadTypes(stream, "@ffffffffs",&sound,&currentValue,
    			&currentPeak, &minValue, &maxValue,
			&holdTime, &backgroundGray, &foregroundGray,&peakGray,
			&smFlags);
    smFlags.running = NO;
    _valTime = _peakTime = currentSample = 0;
    _valOneAgo = _valTwoAgo = 0.0;
    return self;
}

@end

/*

Modification History:

soundkit-25
=======================================================================================
20 Sept 90 (wot)	Added support for SND_FORMAT_EMPHASIZED.  Made it do the same
			things as SND_FORMAT_LINEAR_16.
*/

