/*
	SoundMeter.h
	Sound Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <appkit/View.h>

@interface SoundMeter:View
{
    id sound;
    int currentSample;
    float currentValue;
    float currentPeak;
    float minValue;
    float maxValue;
    float holdTime;
    float backgroundGray;
    float foregroundGray;
    float peakGray;
    struct {
	unsigned int running:1;
	unsigned int bezeled:1;
	unsigned int shouldStop:1;
	unsigned int _reservedFlags:13;
    } smFlags;
    DPSTimedEntry _timedEntry;
    int _valTime;
    int _peakTime;
    float _valOneAgo;
    float _valTwoAgo;
}

- initFrame:(const NXRect *)frameRect;

- read:(NXTypedStream *)aStream;
- write:(NXTypedStream *)aStream;
- (float)holdTime;
- setHoldTime:(float)seconds;
- (float)backgroundGray;
- setBackgroundGray:(float)aValue;
- (float)foregroundGray;
- setForegroundGray:(float)aValue;
- (float)peakGray;
- setPeakGray:(float)aValue;
- sound;
- setSound:aSound;
- run:sender;
- stop:sender;
- (BOOL)isRunning;
- (BOOL)isBezeled;
- setBezeled:(BOOL)aFlag;
- setFloatValue:(float)aValue;
- (float)floatValue;
- (float)peakValue;
- (float)minValue;
- (float)maxValue;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- drawCurrentValue;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end
