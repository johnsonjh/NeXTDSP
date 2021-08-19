
/*
 *	sndinfo.c -- display information in the header of the soundfile
 *	Written by Lee Boynton
 *	Copyright 1988 NeXT, Inc.
 *
 *	Modification History:
 *	03/21/90/mtm	Added SND_FORMAT_COMPRESSED and SND_FORMAT_DSP_COMMANDS.
 *	04/09/90/mtm	Added SND_FORMAT_EMPHASIZED, SND_FORMAT_COMPRESSED_EMPHASIZED,
 *			and SND_FORMAT_DSP_COMMANDS_SAMPLES.
 *	08/02/90/mtm	Map in whole soundfile to get compression info.
 *
 */

#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/file.h>
#import <sound/sound.h>

extern int open();

crash(s1,s2)
    char *s1,*s2;
{
    fprintf(stderr,"sndinfo : %s %s \n",s1,s2);
    exit(1);
}

main (argc,argv)
    int argc;
    char *argv[];
{
    SNDSoundStruct *s;
    int i,j, fd;
    int sample_count;
    char filename[1024], format[128];
    float rate, duration;
    
    if (argc < 2) {
	fprintf(stderr, "usage : sndinfo sndfile1 sndfile2 ...\n");
	exit(1);
    }
    for (i=1; i<argc; i++) {
	
	if (!argv[i])
	    crash("Bad filename","");
	strcpy(filename,argv[i]);
	if ((fd = open(argv[i],O_RDONLY)) < 0) {
	    j = strlen(filename);
	    if ((j<5) || strcmp(&filename[j-4],".snd")) {
		strcat(filename,".snd");
		if ((fd = open(filename,O_RDONLY)) < 0)
		    crash("Cannot open",argv[i]);
	    } else
		crash("Cannot open",argv[i]);
	}
	if (SNDRead(fd,&s))
	    crash("Cannot read",filename);

	sample_count = 0;
	switch (s->dataFormat) {
	    case SND_FORMAT_UNSPECIFIED:
		strcpy(format,"Unspecified");
		break;
	    case SND_FORMAT_DSP_CORE:
		strcpy(format,"DSP Code");
		break;
	    case SND_FORMAT_LINEAR_16:
		strcpy(format,"16 bit linear");
		break;
	    case SND_FORMAT_EMPHASIZED:
		strcpy(format,"16 bit linear, emphasized");
		break;
	    case SND_FORMAT_DSP_DATA_16:
		strcpy(format,"16 bit dsp data");
		break;
	    case SND_FORMAT_LINEAR_32:
		strcpy(format,"32 bit linear");
		break;
	    case SND_FORMAT_LINEAR_8:
		strcpy(format,"8 bit linear");
		break;
	    case SND_FORMAT_MULAW_8:
		strcpy(format,"8 bit muLaw");
		break;
	    case SND_FORMAT_FLOAT:
		strcpy(format,"Single precision floating point");
		break;
	    case SND_FORMAT_DOUBLE:
		strcpy(format,"Double precision floating point");
		break;
	    case SND_FORMAT_MULAW_SQUELCH:
		strcpy(format,"8 bit muLaw, squelch run-length encoded");
		break;
	    case SND_FORMAT_COMPRESSED:
		strcpy(format,"Compressed 16 bit linear");
		break;
	    case SND_FORMAT_COMPRESSED_EMPHASIZED:
		strcpy(format,"Compressed 16 bit linear, emphasized");
		break;
	    case SND_FORMAT_DSP_COMMANDS:
		strcpy(format,"DSP Commands");
		break;
	    case SND_FORMAT_DSP_COMMANDS_SAMPLES:
		strcpy(format,"DSP Commands with samples");
		break;
	    default:
		sprintf(format,"%d (undefined format)", s->dataFormat);
		break;
	}
	sample_count = SNDSampleCount( s );
	if (s->samplingRate == (int)SND_RATE_CODEC)
	    rate = (float)SND_RATE_CODEC;
	else
	    rate = (float)s->samplingRate;

	printf("\nFilename: %s\n",filename);
	printf("Size: %d bytes",s->dataSize);
	if (sample_count) {
	    printf(", %d samples",sample_count);
	    if (sample_count && rate)
		printf(", %5.3f seconds",
	    			(float)sample_count/(float)s->samplingRate);
	}
	printf("\n");
	printf("Format: %s\n", format);
	printf("SamplingRate: %5.3f Hz\n",rate);
	printf("Channels: %d\n",s->channelCount);
	printf("%s\n",s->info);
    }
    exit(0);
}



