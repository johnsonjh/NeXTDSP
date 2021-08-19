/*
	SoundView.h
	Sound Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <streams/streams.h>
#import <appkit/View.h>
#import <appkit/graphics.h>

@interface SoundView : View
{
    id sound;
    id reduction;
    id delegate;
    NXRect selectionRect;
    int displayMode;
    float backgroundGray;
    float foregroundGray;
    float reductionFactor;
    struct {
	unsigned int disabled:1;
	unsigned int continuous:1;
	unsigned int calcDrawInfo:1;
	unsigned int selectionDirty:1;
	unsigned int autoscale:1;
	unsigned int bezeled:1;
	unsigned int notEditable:1;
	unsigned int _reservedFlags:9;
    } svFlags;
    id _scratchSound;
    int _currentSample;
}

/*
 * Display modes
 */
#define SK_DISPLAY_MINMAX 0
#define SK_DISPLAY_WAVE 1


- initFrame:(const NXRect *)aRect;

- free;
- write:(NXTypedStream *) stream;
- read:(NXTypedStream *) stream;
- sound;
- setSound:aSound;
- setReductionFactor:(float)reductionFactor;
- (float)reductionFactor;
- reduction;
- setReduction:aDisplayReduction;
- sizeTo:(NXCoord)width :(NXCoord)height;
- delegate;
- setDelegate:anObject;
- tellDelegate:(SEL)theMessage;
- getSelection:(int *)firstSample size:(int *)sampleCount;
- setSelection:(int)firstSample size:(int)sampleCount;
- hideCursor;
- showCursor;
- setBackgroundGray:(float)aGray;
- (float)backgroundGray;
- setForegroundGray:(float)aGray;
- (float)foregroundGray;
- (int)displayMode;
- setDisplayMode:(int)aMode;
- (BOOL)isContinuous;
- setContinuous:(BOOL)aFlag;
- (BOOL)isEnabled;
- setEnabled:(BOOL)aFlag;
- (BOOL)isEditable;
- setEditable:(BOOL)aFlag;
- (BOOL)isBezeled;
- setBezeled:(BOOL)aFlag;
- (BOOL)isAutoScale;
- setAutoscale:(BOOL)aFlag;
- calcDrawInfo;
- scaleToFit;
- sizeToFit;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- mouseDown:(NXEvent *)theEvent;
- (BOOL)acceptsFirstResponder;
- becomeFirstResponder; 
- resignFirstResponder; 
- selectAll:sender;
- delete:sender;
- cut:sender;
- copy:sender;
- paste:sender;
- play:sender;
- record:sender;
- stop:sender;
- pause:sender;
- resume:sender;
- soundBeingProcessed;
- willPlay:sender;
- didPlay:sender;
- willRecord:sender;
- didRecord:sender;
- hadError:sender;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)aRect;

@end

@interface SoundViewDelegate : Object
- soundDidChange:sender;
- selectionDidChange:sender;
- willRecord:sender;
- didRecord:sender;
- willPlay:sender;
- didPlay:sender;
- hadError:sender;
- willFree:sender;
@end

