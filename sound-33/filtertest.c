/*
 * filtertest.c - Lowpass filter test
 *	Try playing emphasized and non-emphasized sounds.
 */

#import "sound.h"

main (int argc, char *argv[])
{
    int err, i;
    SNDSoundStruct *s;
    int filterStatus;

    if (argc < 2) {
	printf("usage : filtertest file ...\n");
	exit(0);
    }
    
    err = SNDGetFilter(&filterStatus);
    if (err)
        printf("SNDGetFilter() returned %d\n", err);
    printf("Filter status: %d\n", filterStatus);

    printf("Turning filter on...\n");
    err = SNDSetFilter(1);
    if (err)
        printf("SNDSetFilter(1) returned %d\n", err);
    err = SNDGetFilter(&filterStatus);
    if (err)
        printf("SNDGetFilter() returned %d\n", err);
    printf("Filter status: %d\n", filterStatus);

    printf("Turning filter off...\n");
    err = SNDSetFilter(0);
    if (err)
        printf("SNDSetFilter(0) returned %d\n", err);
    err = SNDGetFilter(&filterStatus);
    if (err)
        printf("SNDGetFilter() returned %d\n", err);
    printf("Filter status: %d\n", filterStatus);

    for (i=1; i<argc; i++) {
	err = SNDReadSoundfile(argv[i],&s);
	if (err)
	    printf("filtertest : Cannot read soundfile : %s\n",argv[i]);
	else {
	    err = SNDStartPlaying(s,i,2,0,0,(SNDNotificationFun)SNDFree);
	    if (err)
		printf("filtertest : Cannot play soundfile : %s\n",argv[i]);
	}
    }
    SNDWait(0);
    
    printf("Done playing sound\n");
    err = SNDGetFilter(&filterStatus);
    if (err)
        printf("SNDGetFilter() returned %d\n", err);
    printf("Filter status: %d\n", filterStatus);
    exit(0);
}


