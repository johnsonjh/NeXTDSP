/*
 * recordtest.c
 * 	Usage:  recordtest [options...] seconds file ...
 *      Options:
 *	none	record from codec
 * 	-d	record from dsp
 * 	-2	record from dsp at 22K mono
 * 	-c	record from dsp compressed
 *	-2c	record from dsp compressed at 22K mono
 * 	-b	record from dsp compressed, bit faithful
 * 	-e	use the _EMPHASIZED format
 *
 *	Modification History:
 * 	03/30/90/mtm	Added support for recording from DSP, normal and compressed
 * 	04/10/90/mtm	Added -e for creating emphasized sound.
 * 	04/16/90/mtm	Changed dataData to dspData.
 * 	04/19/90/mtm	Changed codec buf size.
 * 	04/21/90/mtm    Added setting of compression options.
 * 	05/11/90/mtm	Added 22K mono dsp recording.
 *	05/14/90/mtm	Added 22K mono compressed.  Cleaned up header and USAGE.
 *	05/21/90/mtm	Added seconds option.
 */

#import "sound.h"

#define USAGE "Usage: recordtest [-d | -2 | -c | -2c | -b [-e]] seconds file ...\n"

main (int argc, char *argv[])
{
    int size, err, j;
    SNDSoundStruct *s[10], *s2;
    int startArg = 1;
    int dspData = FALSE;
    int compress = FALSE;
    int emphasize = FALSE;
    int bitfaithful = FALSE;
    int mono22 = FALSE;
    float seconds;

    if (argc < 3) {
	printf(USAGE);
	exit(1);
    }
    if ((strcmp(argv[1],"-d") == 0) || (strcmp(argv[1],"-2") == 0)) {
	if (argc < 4) {
	    printf(USAGE);
	    exit(0);
	}
        dspData = TRUE;
	if (strcmp(argv[1],"-2") == 0)
	    mono22 = TRUE;
	startArg = 2;
    } else if ((strcmp(argv[1],"-c") == 0) || (strcmp(argv[1],"-b") == 0) ||
	       (strcmp(argv[1],"-2c") == 0)) {
	if (argc < 4) {
	    printf(USAGE);
	    exit(0);
	}
        compress = TRUE;
	if (strcmp(argv[1],"-b") == 0)
	    bitfaithful = TRUE;
	else if (strcmp(argv[1],"-2c") == 0)
	    mono22 = TRUE;
	startArg = 2;
    }
    if ((dspData || compress) && strcmp(argv[2],"-e") == 0) {
        if (argc < 5) {
	    printf(USAGE);
	    exit(0);
	}
        emphasize = TRUE;
	startArg = 3;
    }
    seconds = atof(argv[startArg++]);

    for (j=startArg; j<argc; j++) {
        if (dspData) {
	    if (mono22)
		err = SNDAlloc(&s[j], seconds*SND_RATE_LOW*2,
			       SND_FORMAT_DSP_DATA_16,SND_RATE_LOW,1,0);
	    else
		err = SNDAlloc(&s[j], seconds*SND_RATE_HIGH*2*2,
			       SND_FORMAT_DSP_DATA_16,SND_RATE_HIGH,2,0);
	} else if (compress) {
	    int method,dropBits;
	    if (mono22) {
		err = SNDAlloc(&s[j], seconds*SND_RATE_LOW*2,
			       SND_FORMAT_COMPRESSED,SND_RATE_LOW,1,0);
		err = SNDSetCompressionOptions(s[j],0,4);
		err = SNDGetCompressionOptions(s[j],&method,&dropBits);
	    } else {
		err = SNDAlloc(&s[j], seconds*SND_RATE_HIGH*2*2,
			       SND_FORMAT_COMPRESSED,SND_RATE_HIGH,2,0);
		err = SNDSetCompressionOptions(s[j],bitfaithful,4);
		err = SNDGetCompressionOptions(s[j],&method,&dropBits);
	    }
	    printf("Compression options: bitFaithful=%d, dropBits=%d\n",
		   method,dropBits);
	} else
	    err = SNDAlloc(&s[j], seconds*SND_RATE_CODEC,
			   SND_FORMAT_MULAW_8,SND_RATE_CODEC,1,0);
	if (err) printf("cannot allocate space...\n");
    }
        
    printf("recording...\n");
    for (j=startArg; j<argc; j++) {
	err = SNDStartRecording(s[j],j,1,0,0,0);
	if (err) printf("cannot start recording...\n");
    }
    SNDWait(0);
    printf("writing files...\n");
    for (j=startArg; j<argc; j++) {
        if (dspData) {
	    if (emphasize)
                s[j]->dataFormat = SND_FORMAT_EMPHASIZED;
	    else
                s[j]->dataFormat = SND_FORMAT_LINEAR_16;
	} else if (compress && emphasize)
		s[j]->dataFormat = SND_FORMAT_COMPRESSED_EMPHASIZED;
	err = SNDWriteSoundfile(argv[j],s[j]);
	if (err) printf("cannot write file %s...\n", argv[j]);
    }
    exit(0);
}
