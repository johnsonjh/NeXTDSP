#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	sounderror.c
 *	Written by Lee Boynton
 *	Copyright 1988 NeXT, Inc.
 *
 */

#import "sounderror.h"

char *SNDSoundError(int err)
{
    switch (err) {
	case SND_ERR_NONE:			//	0
	    return "No error";
	case SND_ERR_NOT_SOUND:			//	1
	    return "Not a sound";
	case SND_ERR_BAD_FORMAT:		//	2
	    return "Bad data format";
	case SND_ERR_BAD_RATE:			//	3
	    return "Bad sampling rate";
	case SND_ERR_BAD_CHANNEL:		//	4
	    return "Bad channel count";
	case SND_ERR_BAD_SIZE:			//	5
	    return "Bad size";
	case SND_ERR_BAD_FILENAME:		//	6
	    return "Bad file name";
	case SND_ERR_CANNOT_OPEN:		//	7
	    return "Cannot open file";
	case SND_ERR_CANNOT_WRITE:		//	8
	    return "Cannot write file";
	case SND_ERR_CANNOT_READ:		//	9
	    return "Cannot read file";
	case SND_ERR_CANNOT_ALLOC:		//	10
	    return "Cannot allocate memory";
	case SND_ERR_CANNOT_FREE:		//	11
	    return "Cannot free memory";
	case SND_ERR_CANNOT_COPY:		//	12
	    return "Cannot copy";
	case SND_ERR_CANNOT_RESERVE:		//	13
	    return "Cannot reserve access";
	case SND_ERR_NOT_RESERVED:		//	14
	    return "Access not reserved";
	case SND_ERR_CANNOT_RECORD:		//	15
	    return "Cannot record sound";
	case SND_ERR_ALREADY_RECORDING:		//	16
	    return "Already recording sound";
	case SND_ERR_NOT_RECORDING:		//	17
	    return "Not recording sound";
	case SND_ERR_CANNOT_PLAY:		//	18
	    return "Cannot play sound";
	case SND_ERR_ALREADY_PLAYING:		//	19
	    return "Already playing sound";
	case SND_ERR_NOT_IMPLEMENTED:		//	20
	    return "Not implemented";
	case SND_ERR_NOT_PLAYING:		//	21
	    return "Not playing sound";
	case SND_ERR_CANNOT_FIND:		//	22
	    return "Cannot find sound";
	case SND_ERR_CANNOT_EDIT:		//	23
	    return "Cannot edit sound";
	case SND_ERR_BAD_SPACE:			//	24
	    return "Bad memory space in dsp load image";
	case SND_ERR_KERNEL:			//	25
	    return "Mach kernel error";
	case SND_ERR_BAD_CONFIGURATION:		//	26
	    return "Bad configuration";
	case SND_ERR_CANNOT_CONFIGURE:		//	27
	    return "Cannot configure";
	case SND_ERR_UNDERRUN:			//	28
	    return "Data underrun";
	case SND_ERR_ABORTED:			//	29
	    return "Aborted";
	case SND_ERR_BAD_TAG:			//	30
	    return "Bad tag";
	case SND_ERR_CANNOT_ACCESS:		//	31
	    return "Cannot access hardware resources";
	case SND_ERR_TIMEOUT:			//	32
	    return "Timeout";
	case SND_ERR_BUSY:			//	33
	    return "Hardware resources already in use";
	case SND_ERR_CANNOT_ABORT:		//	34
	    return "Cannot abort operation";
	case SND_ERR_INFO_TOO_BIG:		//	35
	    return "Information string too large";
	case SND_ERR_UNKNOWN:
	default:
	    return "Unknown error";
    }
}


