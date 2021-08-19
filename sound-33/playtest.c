/*
 * playtest.c
 *
 *	Modification History:
 *	04/16/90/mtm	Added -d switch.
 */

#import "sound.h"

main (int argc, char *argv[])
{
    int size, err, i;
    SNDSoundStruct *s;
    int playDSP = FALSE;
    int startArg = 1;

    if (argc < 2) {
	printf("usage : playtest [-d] file ...\n");
	exit(0);
    }
    if (strcmp(argv[1],"-d") == 0) {
	if (argc < 3) {
	    printf("usage : playtest [-d] file ...\n");
	    exit(0);
	}
        playDSP = TRUE;
	startArg = 2;
    }
    for (i=startArg; i<argc; i++) {
	err = SNDReadSoundfile(argv[i],&s);
	if (err)
	    printf("playtest : Cannot read soundfile : %s\n",argv[i]);
	else {
	    if (playDSP)
	        err = SNDStartPlayingDSP(s,i,2,0,0,(SNDNotificationFun)SNDFree,
		SND_DSP_RECEIVE_CLOCK/*| SND_DSP_RECEIVE_FRAME_SYNC*/);
	    else
	        err = SNDStartPlaying(s,i,2,0,0,(SNDNotificationFun)SNDFree);
	    if (err)
		printf("playtest : Cannot play soundfile : %s\n",argv[i]);
	}
    }
    SNDWait(0);
    exit(0);
}


