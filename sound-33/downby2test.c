/*
 * test jig for sample rate convertor -- down by 2
 */
#import "sound.h"

check_error(int err)
{
    if (err) {
	printf("Error : %d\n",err);
	exit(1);
    }
    return err;
}

main (int argc, char *argv[])
{
    int err;
    SNDSoundStruct *s1, *s2;
    SNDSoundStruct s3 = {
	SND_MAGIC, 0, 0, 
	SND_FORMAT_LINEAR_16, (int)SND_RATE_LOW, 2, "" };

    check_error(argc != 3);
    
    printf("converting file %s\n",argv[1]);
    err = SNDReadSoundfile(argv[1],&s1);
    check_error(err);
    s2 = &s3;
    err = SNDConvertSound(s1,&s2);
    check_error(err);
    err = SNDWriteSoundfile(argv[2],s2);
    check_error(err);
    exit(0);
}


