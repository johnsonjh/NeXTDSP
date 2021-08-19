#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	utilsound.c
 *	Written by Lee Boynton
 *	Copyright 1988 NeXT, Inc.
 *
 *	Modification History:
 *	04/06/90/mtm	Added support for SND_FORMAT_COMPRESSED*,
 *			SND_FORMAT_DSP_COMMANDS*, and SND_FORMAT_EMPHASIZED.
 *	04/09/90/mtm	#include <mach_init.h>
 *	04/10/90/mtm	Set default compression options in SNDAlloc().
 */

#import <mach.h>
#import <mach_init.h>
#import <string.h>
#import "performsound.h"
#import "filesound.h"
#import "utilsound.h"

static int calcHeaderSize(s)
    SNDSoundStruct *s;
{
    int size = strlen(s->info) + 1;
    if (size < 4) size = 4;
    else size = (size + 3) & 3;
    return(sizeof(SNDSoundStruct) - 4 + size);
}

static int calcFormatSize(int dataFormat)
{
    if (dataFormat == SND_FORMAT_MULAW_8 ||
        dataFormat == SND_FORMAT_LINEAR_8 ||
        dataFormat == SND_FORMAT_MULAW_SQUELCH )
	return 1;
    if (dataFormat == SND_FORMAT_LINEAR_16 ||
	dataFormat == SND_FORMAT_DSP_DATA_16 ||
	dataFormat == SND_FORMAT_DISPLAY ||
	dataFormat == SND_FORMAT_EMPHASIZED)
	return 2;
    if (dataFormat == SND_FORMAT_LINEAR_32 || 
    	dataFormat == SND_FORMAT_DSP_DATA_32 ||
    	dataFormat == SND_FORMAT_FLOAT ||
	dataFormat == SND_FORMAT_INDIRECT)
	return 4;
    if (dataFormat == SND_FORMAT_LINEAR_24 ||
	dataFormat == SND_FORMAT_DSP_DATA_24 )
	return 3;
    if (dataFormat == SND_FORMAT_DSP_CORE)
	return 0;
    if (dataFormat == SND_FORMAT_DOUBLE)
	return 8;
    return -1;
}

int SNDBytesToSamples(int byteCount, int channelCount, int dataFormat)
{
    int formatSize = calcFormatSize(dataFormat);
    if (!channelCount || !formatSize) return 0;
    return byteCount/(channelCount*formatSize);
}

int SNDSamplesToBytes(int sampleCount, int channelCount, int dataFormat)
{
    int formatSize = calcFormatSize(dataFormat);
    return sampleCount*channelCount*formatSize;
}

int SNDSampleCount(SNDSoundStruct *s)
{
    int i;
    if (!s || s->magic != SND_MAGIC)
	return -1;
    if (s->dataFormat == SND_FORMAT_INDIRECT) {
	SNDSoundStruct *s2, **iBlock = (SNDSoundStruct **)s->dataLocation;
	i = 0;
	while (s2 = *iBlock++)
	    i += SNDBytesToSamples(s2->dataSize,
	    				s2->channelCount,s2->dataFormat);
    } else if (s->dataFormat == SND_FORMAT_MULAW_SQUELCH) {
	unsigned char *ip = (unsigned char *)s;
	ip += s->dataLocation;
	ip += 4;  // advance past flag bytes
	i = ( *((int *) ip) );
    } else if (s->dataFormat == SND_FORMAT_DSP_CORE) {
	int temp, *ip = (int *)((int)s + s->dataLocation);
	//NOTE: a load segment is {type,addr,size,[data ... data]}
	// and a system segment is {0, 0, 5, samp_cnt_hi,samp_cnt_hi,...}
	// all words are 24 low bits in an int.
	if (*ip++)
	    i = -1; // not a system segment -- cannot guess sample count
	else {
	    ip+= 2;  // advance past version number and segment length
	    temp = *ip++; // get high order 24 bits of sample count
	    if (temp > 127)
		 i = -1; // too big for return value
	    else
		i = (temp<<24)+(0xffffff & *ip); // get low order 24 bits
	}
    } else if (s->dataFormat == SND_FORMAT_COMPRESSED ||
    	       s->dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED) {
        // Original size is first int in subheader
        i = SNDBytesToSamples(*((int *)((char *)s + s->dataLocation)),
			      s->channelCount,SND_FORMAT_LINEAR_16);
    } else if (s->dataFormat == SND_FORMAT_DSP_COMMANDS ||
    	       s->dataFormat == SND_FORMAT_DSP_COMMANDS_SAMPLES) {
        // Sample count is first int in subheader
        i = *((int *)((char *)s + s->dataLocation));
    } else
	i = SNDBytesToSamples(s->dataSize,s->channelCount,s->dataFormat);
    return (i < 0) ? -1 : i;
}

int SNDGetDataPointer(SNDSoundStruct *s, char **ptr, int *size, int *width)
{
    char *p;
    int i;
    if (!s || s->magic != SND_MAGIC)
	return SND_ERR_NOT_SOUND;
    if (s->dataFormat == SND_FORMAT_INDIRECT) 
	return SND_ERR_BAD_FORMAT;
    i = calcFormatSize(s->dataFormat);
    if (i < 0)
	return SND_ERR_BAD_FORMAT;
    p = (char *)s;
    p += s->dataLocation;
    *ptr = p;
    *size = s->dataSize / i;
    *width = i;
    return SND_ERR_NONE;
}


int SNDAlloc(SNDSoundStruct **s,
	     int dataSize, 
	     int dataFormat,
	     int samplingRate,
	     int channelCount,
	     int infoSize)
{
    SNDSoundStruct *pS;
    int size;
    
    size = sizeof(SNDSoundStruct) + dataSize;
    if (infoSize > 4)
	size += ((infoSize-1) & 0xfffffffc);
    if (vm_allocate(task_self(),(pointer_t *)&pS,size,1) != KERN_SUCCESS) {
	return SND_ERR_CANNOT_ALLOC;
    }
    pS->magic = SND_MAGIC;
    pS->dataLocation = size-dataSize;
    pS->dataSize = dataSize;
    pS->dataFormat = dataFormat;
    pS->samplingRate = samplingRate;
    pS->channelCount = channelCount;
    pS->info[0] = '\0';
    *s = pS;
    if (dataFormat == SND_FORMAT_COMPRESSED ||
        dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED)
	return SNDSetCompressionOptions(pS, 1, 4);	// bitFaithful, 4 dropped bits
    else
        return SND_ERR_NONE;
}

int SNDFree(SNDSoundStruct *s)
{
    int size;
    if (!s || s->magic != SND_MAGIC) return SND_ERR_NOT_SOUND;
    if (s->dataFormat == SND_FORMAT_INDIRECT) {
	if (vm_deallocate(task_self(),s->dataLocation,s->dataSize) != 
								KERN_SUCCESS)
	    return SND_ERR_CANNOT_FREE;
	size = calcHeaderSize(s);
	if (vm_deallocate(task_self(),(pointer_t)s,size) != KERN_SUCCESS)
	    return SND_ERR_CANNOT_FREE;
    } else {
	size = s->dataLocation + s->dataSize;
	if (vm_deallocate(task_self(),(pointer_t)s,size) != KERN_SUCCESS)
	    return SND_ERR_CANNOT_FREE;
    }
    return SND_ERR_NONE;
}

int SNDPlaySoundfile(char *path, int priority)
{
    SNDSoundStruct *s;
    int err = SNDReadSoundfile(path,&s);
    if (err) return err;
    err = SNDStartPlaying(s,(int)s,priority,1,0,(SNDNotificationFun)SNDFree);
    if (err) SNDFree(s);
    return err;
}



