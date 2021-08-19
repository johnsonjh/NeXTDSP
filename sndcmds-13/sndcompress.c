/*
 * sndcompress.c
 *	Usage: sndcompress [-<n>] <infile> <outfile>
 *	If <infile> sound is in a linear format, it is compressed.
 *	If <infile> sound is in a compressed format, it is decompressed.
 *	<n> controls the compression ratio, specify as an integer 4 <= n <= 8.
 *	Higher numbers give greater compression.  If <n> not specified,
 *	the compression is bit-faithful (decompression can reproduce the
 *	original sound exactly).
 */
#import <stdio.h>
#import <string.h>
#import <sound/sound.h>

static void usage(void)
{
    printf("usage : sndcompress [-<n>] <inputFile> <outputFile>\n"
	   "\tIf <inputFile> is in a 16 bit linear format, it is compressed.\n"
	   "\tIf <inputFile> is in a compressed format, it is decompressed.\n"
	   "\t<n> controls the compression amount, specify as an integer 4 <= n <= 8.\n"
	   "\tHigher numbers give greater compression.  If <n> not specified,\n"
	   "\tthe compression is bit-faithful (decompression reproduces the\n"
	   "\toriginal sound exactly).\n");
    exit(1);
}

main (int argc, char *argv[])
{
    int err;
    SNDSoundStruct *s1, *s2;
    char *infile, *outfile;
    int bitFaithful = 1;
    int dropBits = 4;

    if (argc < 3 || argc > 4)
	usage();
    if (argc == 4) {
	if ((strlen(argv[1]) != 2) || *argv[1] != '-')
	    usage();
	dropBits = atoi(argv[1]+1);
	if (dropBits < 4 || dropBits > 8)
	    usage();
	bitFaithful = 0;
	infile = argv[2];
	outfile = argv[3];
    } else {
	infile = argv[1];
	outfile = argv[2];
    }
    
    err = SNDReadSoundfile(infile, &s1);
    if (err) {
	fprintf(stderr, "%s : %s\n", infile, SNDSoundError(err));
	exit(1);
    }
    err = SNDCompressSound(s1, &s2, bitFaithful, dropBits);
    if (err) {
	fprintf(stderr, "%s\n", SNDSoundError(err));
	exit(1);
    }
    err = SNDWriteSoundfile(outfile, s2);
    if (err) {
	fprintf(stderr, "%s : %s\n", outfile, SNDSoundError(err));
	exit(1);
    }
    exit(0);
}

