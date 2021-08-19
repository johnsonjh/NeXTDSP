/*
 * compresstest.c
 *	Usage: compresstest [-<n>] <infile> <outfile>
 *	If <infile> sound is in a linear format, it is compressed.
 *	If <infile> sound is in a compressed format, it is decompressed.
 *	<n> is the number of bits to drop, 4-8, and implies non-bit-faithful.
 */
#import "sound.h"

check_error(int err)
{
    if (err) {
	printf("Error : %d, %s\n",err,SNDSoundError(err));
	exit(1);
    }
    return err;
}

main (int argc, char *argv[])
{
    int err;
    SNDSoundStruct *s1, *s2;
    char *infile, *outfile;
    int bitFaithful = 1;
    int dropBits = 4;

    if (argc < 3 || argc > 4) {
	printf("Usage: %s [-<n>] <infile> <outfile>\n", argv[0]);
	exit(1);
    }
    if (argc == 4) {
	bitFaithful = 0;
	dropBits = atoi(argv[1]+1);
	if (dropBits < 4)
	    dropBits = 4;
	else if (dropBits > 8)
	    dropBits = 8;
	infile = argv[2];
	outfile = argv[3];
    } else {
	infile = argv[1];
	outfile = argv[2];
    }
    /*printf("bitFaithful = %d, dropBits = %d\n", bitFaithful, dropBits);*/
    
    err = SNDReadSoundfile(infile ,&s1);
    check_error(err);
    err = SNDCompressSound(s1, &s2, bitFaithful, dropBits);
    check_error(err);
    err = SNDWriteSoundfile(outfile,s2);
    check_error(err);
    exit(0);
}
