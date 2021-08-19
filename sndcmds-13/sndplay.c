/*
 *	sndplay.c -- play soundfiles
 *	Copyright 1988 NeXT, Inc.
 *
 *	Modification History:
 *	08/09/90/mtm	Add playback to DSP serial port.
 *
 */
#import <sound/sound.h>
#import <stdio.h>
#import <libc.h>

void usage(void)
{
    fprintf(stderr, "usage : sndplay\n"
            "\t[-d] (sends 16 bit data to the dsp port)\n"
	    "\t[-c] (dsp receives (rather than generates) bit clock, implies -d)\n"
	    "\t[-s] (dsp receives (rather than generates) frame sync, implies -d)\n"
	    "\tfile ...\n");
    exit(1);
}

main (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    char *optstring = "dcs";
    int opt;
    int size, err, i;
    SNDSoundStruct *s;
    int playDSP = FALSE, playOptions = 0;

    if (argc == 1)
	usage();
    while ((opt = getopt(argc, argv, optstring)) != EOF)
	switch (opt) {
	    case 'd':
		playDSP = TRUE;
		break;
	    case 'c':
		playDSP = TRUE;
		playOptions |= SND_DSP_RECEIVE_CLOCK;
		break;
	    case 's':
		playDSP = TRUE;
		playOptions |= SND_DSP_RECEIVE_FRAME_SYNC;
		break;
	    default:
		usage();
		break;
	}
    if (argc < optind+1) {
	fprintf(stderr, "sndplay : no file specified\n");
	exit(1);
    }
    
    for (i=optind; i<argc; i++) {
	err = SNDReadSoundfile(argv[i],&s);
	if (err)
	    fprintf(stderr,"sndplay : Cannot read soundfile : %s\n",argv[i]);
	else {
	    if (playDSP)
	        err = SNDStartPlayingDSP(s,i,2,0,0,(SNDNotificationFun)SNDFree,
					 playOptions);
	    else
	    	err = SNDStartPlaying(s,i,2,0,0,(SNDNotificationFun)SNDFree);
	    if (err)
		fprintf(stderr,
		  "sndplay : Cannot play soundfile : %s\n",argv[i]);
	}
    }
    SNDWait(0);
    exit(0);
}
