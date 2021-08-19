

#import "sound.h"

check_error(int err)
{
    if (err) {
	printf("Error : %s\n",SNDSoundError(err));
	exit(1);
    }
    return err;
}

int begin(SNDSoundStruct *s, int tag, int err)
{
    printf("begin : %x %d %d\n",s,tag,err);
}

int end(SNDSoundStruct *s, int tag, int err)
{
    printf("end : %x %d %d\n",s,tag,err);
}

main (int argc, char *argv[])
{
    int size, err, j;
    SNDSoundStruct *s, *s2;

    check_error(argc < 2);
    
    for (j=2; j<argc; j++) {		//check that all hosts are available
	err = SNDSetHost(argv[j]);
	check_error(err);
    }

    err = SNDReadSoundfile(argv[1],&s);
    check_error(err);

    for (j=2; j<argc; j++) {
	err = SNDSetHost(argv[j]);
	check_error(err);
	err = SNDStartPlaying(s,j,2,0,begin,end);
	check_error(err);
    }
    SNDWait(0);
    exit(0);
}


