#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	editsound.c
 *	Written by Lee Boynton
 *	Copyright 1988 NeXT, Inc.
 *
 *	Modification History:
 *	04/09/90/mtm	#include <mach_init.h>
 */

#import <mach.h>
#import <mach_init.h>
#import <stdlib.h>
#import <string.h>
#import <sys/types.h>
#import "utilsound.h"
#import "editsound.h"

#define PAGESIZE ((int)vm_page_size)
#define NULL_SOUND ((SNDSoundStruct *)0)
#define SMALL_SOUND_SIZE (PAGESIZE*2)

static int checkSound(SNDSoundStruct *s)
{
    if (!s || s->magic != SND_MAGIC) return 0;
    return 1;
}

static int calcSampleCount(SNDSoundStruct *s)
{
    int foo=0;
    SNDSoundStruct **iBlock;
    if (!s) return foo;
    if (!s->channelCount) return foo;
    switch (s->dataFormat) {
	case SND_FORMAT_INDIRECT:
	    iBlock = (SNDSoundStruct **)s->dataLocation;
	    while(*iBlock)
		foo += calcSampleCount(*iBlock++);
	    return foo;
	case SND_FORMAT_DSP_CORE:
	    return 0;
	case SND_FORMAT_LINEAR_16:
	case SND_FORMAT_DSP_DATA_16:
	    foo = s->dataSize>>1;
	    break;
	default:
	    foo = s->dataSize;
	    break;
    }
    return foo/s->channelCount;
}

static int roundUpToPage(int size)
{
    int temp = size % PAGESIZE;
    if (temp)
	return size + PAGESIZE - temp;
    else
	return size;
}

static int vmalloc(pp,size)
    char **pp;
    int size;
{
    if (vm_allocate(task_self(),(pointer_t *)pp,size,1) != KERN_SUCCESS)
	return SND_ERR_CANNOT_ALLOC;
    else
	return SND_ERR_NONE;
}

static int vmfree(p,size)
    char *p;
    int size;
{
    if (vm_deallocate(task_self(),(pointer_t)p,size) != KERN_SUCCESS)
	return SND_ERR_CANNOT_FREE;
    else
	return SND_ERR_NONE;
}

static int vmcopy(pp,src,count)
    char **pp;
    char *src;
    int count;
{
    int size, pageOffset = (int)src % PAGESIZE;
    char *s,*firstPage=src;
    firstPage -= pageOffset;
    size = roundUpToPage(count + pageOffset);
    if (vmalloc(&s,size))
	return SND_ERR_CANNOT_ALLOC;
    if (vm_copy(task_self(),(pointer_t)firstPage,
    				size,(pointer_t)s) != KERN_SUCCESS) {
	vmfree(s,size);
	return SND_ERR_CANNOT_COPY;
    }
    *pp = s + pageOffset;
    return SND_ERR_NONE;
}


static SNDSoundStruct *duplicateSound(SNDSoundStruct *s)
{
    SNDSoundStruct *newSound;
    int size = s->dataLocation + s->dataSize;
    if (vmcopy(&newSound, s, size))
	return NULL_SOUND;
    else
	return newSound;
}

static void freeTail(SNDSoundStruct *s, int leadingSampleCount)
{
    int extraTailPtr = (int)s + s->dataLocation + s->dataSize;
    int extraBytes, extraPtr = (int)s+leadingSampleCount + s->dataLocation;
    extraPtr = roundUpToPage(extraPtr);
    extraBytes = extraTailPtr - extraPtr;
    if (extraBytes > 0)
	vmfree(extraPtr,extraBytes);
    s->dataSize = leadingSampleCount;
}

static SNDSoundStruct *duplicateTail(SNDSoundStruct *s, int tailDataSize)
{
    SNDSoundStruct *newSound;
    int headerSize = sizeof(SNDSoundStruct);
    char *firstPage;
    
    firstPage = (char *)s;
    firstPage += s->dataLocation;
    firstPage += (s->dataSize - tailDataSize);
    firstPage -= headerSize;
    if (vmcopy(&newSound,firstPage,tailDataSize+headerSize))
	return NULL_SOUND;
    memmove(newSound,s,headerSize);
    newSound->info[0] = '\0';
    newSound->dataSize = tailDataSize;
    newSound->dataLocation = headerSize;
    return newSound;
}

static int convertToIndirect(SNDSoundStruct *s)
{
    SNDSoundStruct **iBlock, *s1 = duplicateSound(s);

    if (!s1)
	return SND_ERR_CANNOT_EDIT;
    freeTail(s,0);
    if (vmalloc(&iBlock,PAGESIZE)) {
	SNDFree(s1);
	return SND_ERR_CANNOT_EDIT;
    }
    iBlock[0] = s1;
    iBlock[1] = NULL_SOUND;
    s->dataSize = PAGESIZE;
    s->dataLocation = (int)iBlock;
    s->dataFormat = SND_FORMAT_INDIRECT;
    return SND_ERR_NONE;
}

static int insertFragmented(SNDSoundStruct *s1, SNDSoundStruct *s2, int index)
{
    SNDSoundStruct **iBlock1 = (SNDSoundStruct **)s1->dataLocation;
    SNDSoundStruct **iBlock2 = (SNDSoundStruct **)s2->dataLocation;
    int pointerSize = sizeof(SNDSoundStruct **);
    int i1Max = s1->dataSize / pointerSize;
    int i2Max = s2->dataSize / pointerSize;
    int i, i1Count, i2Count;

    for (i1Count=index; i1Count<i1Max; i1Count++)
	if (!iBlock1[i1Count]) break;
    for (i2Count=0;i2Count<i2Max;i2Count++)
	if (!iBlock2[i2Count]) break;
    if ((i1Count+i2Count) >= i1Max) {
	int newSize = s1->dataSize + roundUpToPage(i2Count*pointerSize);
	SNDSoundStruct **newBlock;
	if (vmalloc(&newBlock,newSize))
	    return SND_ERR_CANNOT_ALLOC;
	memmove((char *)newBlock,(char *)s1->dataLocation,s1->dataSize);
	vmfree(s1->dataLocation,s1->dataSize);
	s1->dataLocation = (int)newBlock;
	s1->dataSize = newSize;
    }
    for (i = i1Count; i > index; i--)
	iBlock1[i+i2Count-1] = iBlock1[i-1];
    for (i = 0; i < i2Count; i++)
	iBlock1[i+index] = iBlock2[i];
    return SND_ERR_NONE;
}

static int insertIndirect(SNDSoundStruct *s1, SNDSoundStruct *s2, int index)
{
    SNDSoundStruct **iBlock = (SNDSoundStruct **)s1->dataLocation;
    int i, iEnd, iMax = s1->dataSize / sizeof(SNDSoundStruct **);
    
    if (s2->dataFormat == SND_FORMAT_INDIRECT) {
	insertFragmented(s1,s2,index);
    } else {
	for (iEnd=index; iEnd<iMax; iEnd++)
	    if (!iBlock[iEnd]) break;
	if ((iEnd+1) == iMax) {
	    int newSize = s1->dataSize + PAGESIZE;
	    SNDSoundStruct **newBlock;
	    if (vmalloc(&newBlock,newSize))
		return SND_ERR_CANNOT_ALLOC;
	    memmove((char *)newBlock,(char *)s1->dataLocation,s1->dataSize);
	    vmfree(s1->dataLocation,s1->dataSize);
	    s1->dataLocation = (int)newBlock;
	    s1->dataSize = newSize;
	}
	for (i = iEnd+1; i > index; i--)
	    iBlock[i] = iBlock[i-1];
	iBlock[index] = s2;
    }
    return SND_ERR_NONE;
}

static int deleteIndirect(SNDSoundStruct *s1, int splitPoint1, int splitPoint2)
{
    SNDSoundStruct *s2, **iBlock = (SNDSoundStruct **)s1->dataLocation;
    int i, j, iEnd, iMax = s1->dataSize / sizeof(SNDSoundStruct **);
    int first, last, format=0;

    for (iEnd=0; iEnd<iMax; iEnd++)
	if (!iBlock[iEnd]) break;  
    if (splitPoint1 > splitPoint2) {
	first = (splitPoint2 < 0) ? 0 : splitPoint2;
	last = (splitPoint1 >= iEnd) ? iEnd : splitPoint1;
    } else {
	first = (splitPoint1 < 0) ? 0 : splitPoint1;
	last = (splitPoint2 >= iEnd) ? iEnd : splitPoint2;
    }
    if (first >= iEnd || last < 0 || first == last) return SND_ERR_NONE;
    for (i=first, j=last; j<iEnd; i++, j++) {
	if (s2 = iBlock[i]) {
	    format = s2->dataFormat;
	    if (i < last)
		SNDFree(s2);
	    iBlock[i] = iBlock[j];
	} else break;
    }
    iEnd -= (last - first);
    iBlock[iEnd] = NULL_SOUND;
    if (!iBlock[0]) {
	s1->dataFormat = format;
	vmfree(s1->dataLocation,s1->dataSize);
	s1->dataLocation = (strlen(s1->info)+3 & ~3) - 4 + 
					sizeof(SNDSoundStruct);
	s1->dataSize = 0;
    }
    return SND_ERR_NONE;
}

static int splitSound(SNDSoundStruct *s, int sampleOffset, int *splitPoint)
{
    SNDSoundStruct **iBlock, *s1, *s2;
    int byteOffset, i, iMax, err, split, curByte=0, nextByte=0, format;
    
    iBlock = (SNDSoundStruct **)s->dataLocation;
    iMax = s->dataSize / sizeof(SNDSoundStruct **);
    if (!iBlock[0] || !iMax) return SND_ERR_CANNOT_EDIT;
    format = iBlock[0]->dataFormat;
    byteOffset=SNDSamplesToBytes(sampleOffset,s->channelCount,format);
    if (byteOffset < 0) return SND_ERR_CANNOT_EDIT;
    for (i = 0; i < iMax; i++) {
	if (!iBlock[i] || byteOffset == curByte) {
	    *splitPoint = i;
	    return SND_ERR_NONE;
	}
	nextByte = curByte + iBlock[i]->dataSize;
	if (curByte < byteOffset && nextByte > byteOffset)
	    break;
	curByte = nextByte;
    }
    if (i >= iMax)
	return SND_ERR_CANNOT_EDIT;
    split = i+1;
    s1 = iBlock[i];
    if (!(s2 = duplicateTail(s1,nextByte - byteOffset)))
	return SND_ERR_CANNOT_EDIT;
    freeTail(s1,byteOffset - curByte);
    if (err = insertIndirect(s,s2,split)) return err;
    *splitPoint = split;
    return SND_ERR_NONE;
}

static int calc_size(SNDSoundStruct *s)
{
    if (s->dataFormat != SND_FORMAT_INDIRECT)
	return s->dataSize;
    else {
	SNDSoundStruct *s2, **iBlock = (SNDSoundStruct **)s->dataLocation;
	int i, size = 0, iMax = s->dataSize / sizeof(SNDSoundStruct **); 
	for (i=0; i<iMax; i++) {
	    if (s2 = iBlock[i])
		size += s2->dataSize;
	    else
		break;
	}
	return size;
    }
}

static int calc_format(SNDSoundStruct *s)
{
    if (s->dataFormat != SND_FORMAT_INDIRECT)
	return s->dataFormat;
    else {
	SNDSoundStruct *s2, **iBlock = (SNDSoundStruct **)s->dataLocation;
	if (s2 = iBlock[0])
	    return s2->dataFormat;
	else
	    return SND_FORMAT_UNSPECIFIED;
    }
}

static int calc_limits(int offset,
		       int count,
		       SNDSoundStruct *s,
		       int *newOffset,
		       int *newCount)
{
    int o=offset, c=count;
    int byteCount, byteOffset;
    int chan = s->channelCount, format = calc_format(s);
    int max = calc_size(s);

    if (c < 0) {
	o += c;
	c = -c;
    }
    if (o < 0) {
	c += o;
	o = 0;
    }
    if (c <= 0) {
	*newOffset = *newCount = 0;
	return SND_ERR_NONE;
    }
    byteOffset = SNDSamplesToBytes(offset,chan,format);
    if (byteOffset < 0)
	return SND_ERR_BAD_FORMAT;
    if (byteOffset > max) {
	byteCount = byteCount = 0;
    } else {
	byteCount = SNDSamplesToBytes(count,chan,format);
	if ((byteOffset + byteCount) > max)
	    byteCount = max - byteOffset;
    }
    *newOffset = byteOffset;
    *newCount = byteCount;
    return SND_ERR_NONE;
}


static int simple_copy_bytes(SNDSoundStruct **s1,
		       SNDSoundStruct *s2,
		       int byteOffset,
		       int byteCount)
{
    SNDSoundStruct *s;
    int err;
    int headerSize = sizeof(SNDSoundStruct);
    char *src, *dst;

    if (byteCount > SMALL_SOUND_SIZE) {
	    src = (char *)s2;
	    src += s2->dataLocation;
	    src += byteOffset;
	    src -= headerSize;
	    if (vmcopy(&s,src,byteCount+headerSize))
		return SND_ERR_CANNOT_EDIT;
	    memmove(s,s2,headerSize);
	    s->info[0] = '\0';
	    s->dataSize = byteCount;
	    s->dataLocation = headerSize;
    } else {
	if (err = SNDAlloc(&s, byteCount, s2->dataFormat, s2->samplingRate,
							   s2->channelCount,0))
	    return err;
	src = (char *)s2;
	src += s2->dataLocation;
	src += byteOffset;
	dst = (char *)s;
	dst += s->dataLocation;
	memmove(dst,src,byteCount);
    }
    *s1 = s;
    return SND_ERR_NONE;
}

static int simple_copy(SNDSoundStruct **s1,
		       SNDSoundStruct *s2,
		       int offset,
		       int count)
{
    int err, byteCount, byteOffset;

    if (err = calc_limits(offset,count,s2,&byteOffset,&byteCount))
	return err;
    return simple_copy_bytes(s1,s2,byteOffset,byteCount);
}

static int fragmented_copy(SNDSoundStruct **s1,
			   SNDSoundStruct *s2,
			   int offset,
			   int count)
{
    int err, byteOffset1, byteOffset2, byteCount;
    SNDSoundStruct *s, **iBlock1, **iBlock2;
    int i, i1, i2, iMax, j, curByte, nextByte;
    
    if (err = calc_limits(offset,count,s2,&byteOffset1,&byteCount))
	return err;
    byteOffset2 = byteOffset1 + byteCount - 1;
    if (byteOffset2 <= byteOffset1) {
	*s1 = NULL_SOUND;
	return SND_ERR_NONE;
    }
    iBlock2 = (SNDSoundStruct **)s2->dataLocation;
    iMax = s2->dataSize / sizeof(SNDSoundStruct **);
    curByte = 0;
    i1 = i2 = -1;
    for (i = 0; i < iMax; i++) {
	if (!iBlock2[i])
	    return SND_ERR_CANNOT_COPY;
	nextByte = curByte + iBlock2[i]->dataSize;
	if (curByte <= byteOffset1 && nextByte > byteOffset1) {
	    byteOffset1 -= curByte;
	    i1 = i;
	}
	if (curByte <= byteOffset2 && nextByte > byteOffset2) {
	    byteOffset2 -= curByte;
	    i2 = i;
	    break;
	}
	curByte = nextByte;
    }
    if (i1 < 0 || i2 < 0)
	return SND_ERR_CANNOT_COPY;
    if (i1 == i2) {
	byteCount = byteOffset2-byteOffset1+1;
	return simple_copy_bytes(s1,iBlock2[i1],byteOffset1,byteCount);
    }
    if (err = SNDAlloc(s1, 0, SND_FORMAT_INDIRECT, 
    				s2->samplingRate, s2->channelCount, 0))
	return err;
    if (err = vmalloc(&iBlock1,s2->dataSize)) {
	SNDFree(*s1);
	*s1 = NULL_SOUND;
	return err;
    }
    (*s1)->dataLocation = (int)iBlock1;
    (*s1)->dataSize = s2->dataSize;
    j = 0;
    byteCount = iBlock2[i1]->dataSize - byteOffset1;
    if (err = simple_copy_bytes(&s,iBlock2[i1],byteOffset1,byteCount)) {
	SNDFree(*s1);
	*s1 = NULL_SOUND;
	return err;
    }
    iBlock1[0] = s;
    j = 1;
    for (i=i1+1;i<i2;i++) {
	if (s = duplicateSound(iBlock2[i]))
	    iBlock1[j++] = s;
	else {
	    iBlock1[j] = NULL_SOUND;
	    SNDFree(*s1);
	    *s1 = NULL_SOUND;
	    return SND_ERR_CANNOT_COPY;
	}
    }
    if (err = simple_copy_bytes(&s,iBlock2[i2],0,byteOffset2+1)) {
	iBlock2[j] = NULL_SOUND;
	SNDFree(*s1);
	*s1 = NULL_SOUND;
	return err;
    }
    iBlock1[j++] = s;
    iBlock1[j] = NULL_SOUND;
    return SND_ERR_NONE;
}


/*
 * Exported functions
 */

int SNDInsertSamples(SNDSoundStruct *s1, SNDSoundStruct *s2, int startSample)
{
    SNDSoundStruct *s;
    int splitPoint, err, size;
    if (!checkSound(s1) || !checkSound(s2))
	return SND_ERR_NOT_SOUND;
    size = calc_size(s2);
    if (err = SNDCopySamples(&s,s2,0,size))
	return SND_ERR_CANNOT_EDIT;
    if (s1->dataFormat != SND_FORMAT_INDIRECT)
	if (err = convertToIndirect(s1)) {
	    SNDFree(s);
	    return SND_ERR_CANNOT_EDIT;
	}
    if (err = splitSound(s1,startSample,&splitPoint)) return err;
    if (err = insertIndirect(s1,s,splitPoint)) return err;
    return SND_ERR_NONE;
}

int SNDDeleteSamples(SNDSoundStruct *s, int startSample, int sampleCount)
{
    int splitPoint1, splitPoint2, err;
    int endSample = startSample + sampleCount;
    if (!checkSound(s))
	return SND_ERR_NOT_SOUND;
    if (s->dataFormat != SND_FORMAT_INDIRECT)
	if (err = convertToIndirect(s)) return err;
    if (err = splitSound(s,startSample,&splitPoint1)) return err;
    if (err = splitSound(s,endSample,&splitPoint2)) return err;
    if (err = deleteIndirect(s,splitPoint1,splitPoint2)) return err;
    return SND_ERR_NONE;
}

int SNDCopySamples(SNDSoundStruct **s1, SNDSoundStruct *s2, 
			int offset, int count)
{
    int err;

    if (!checkSound(s2))
	return SND_ERR_NOT_SOUND;
    if (s2->dataFormat != SND_FORMAT_INDIRECT)
	err = simple_copy(s1,s2,offset,count);
    else
	err = fragmented_copy(s1,s2,offset,count);
    return err;
}

int SNDCopySound(SNDSoundStruct **s1, SNDSoundStruct *s2)
{
    int err, size;
    if (!checkSound(s2))
	return SND_ERR_NOT_SOUND;
    if (s2->dataFormat != SND_FORMAT_INDIRECT) {
	size = s2->dataLocation + s2->dataSize;
	err = vmcopy(s1,s2,size);
    } else {
	size = calc_size(s2);
	err = SNDCopySamples(s1,s2,0,size);
    }
    return err;
}


int SNDCompactSamples(SNDSoundStruct **s1, SNDSoundStruct *s2)
{
    SNDSoundStruct *fragment, *newSound, **iBlock, *oldSound = s2;
    int format, nchan, rate, newSize, infoSize, err;
    char *src, *dst;
    if (!checkSound(oldSound))
	return SND_ERR_NOT_SOUND;
    if (oldSound->dataFormat != SND_FORMAT_INDIRECT)
	return SND_ERR_NONE;
    iBlock = (SNDSoundStruct **)oldSound->dataLocation;
    if (!*iBlock) {
	newSound = (SNDSoundStruct *)0;
    } else {
	format = (*iBlock)->dataFormat;
	nchan = oldSound->channelCount;
	rate = oldSound->samplingRate;
	infoSize = strlen(oldSound->info);
	newSize = SNDSamplesToBytes(calcSampleCount(oldSound),nchan,format);
	err = SNDAlloc(&newSound,newSize,format,rate,nchan,infoSize);
	if (err)
	    return SND_ERR_CANNOT_ALLOC;
	strcpy(newSound->info,oldSound->info);
	dst = (char *)newSound;
	dst += newSound->dataLocation;
	while(fragment = *iBlock++) {
	    src = (char *)fragment;
	    src += fragment->dataLocation;
	    memmove(dst,src,fragment->dataSize);
	    dst += fragment->dataSize;
	}
    }
    *s1 = newSound;
    return SND_ERR_NONE;
}


