

#import "sound.h"

#define BUF_SIZE 8192
#define BUF_MAX 8

static char *target_host;
static int buf_ptr, buf_max;
static SNDSoundStruct *buffers[BUF_MAX+1];

int begin(SNDSoundStruct *s, int tag, int err)
{
    printf("started recording buffer %d\n",tag);
}

int end(SNDSoundStruct *s, int tag, int err)
{
    if (err) printf("error while recording %d\n",tag);
    printf("completed recording buffer %d\n",tag);
    if (buf_ptr < buf_max) {
	err = SNDStartRecording(buffers[buf_ptr], buf_ptr, 5,0,begin,end);
	if (err) printf("cannot start recording %d\n",buf_ptr);
	buf_ptr++;
    }
}

main (int argc, char *argv[])
{
    int size, err, i, j;
    int x = 0;

    if (argc < 2) {
	printf("usage: chaintest file ...\n");
	exit(0);
    }
    for (j=1; j<argc; j++) {
	err = SNDAlloc(&buffers[j],BUF_SIZE,SND_FORMAT_MULAW_8,
							SND_RATE_CODEC,1,0);
	check_error(err);
    }
    buf_ptr = 3;
    buf_max = argc;
    printf("recording...\n");
    err = SNDStartRecording(buffers[1],1,2,0,begin,end);
    check_error(err);
    err = SNDStartRecording(buffers[2],2,2,0,begin,end);
    check_error(err);
    SNDWait(0);
    printf("writing...\n");
    for (j=1; j<argc; j++) {
	err = SNDWriteSoundfile(argv[j],buffers[j]);
	if (err) printf("cannot write file %s...\n", argv[j]);
    }
    exit(0);
}

check_error(int err)
{
    if (err) {
	printf("Error : %s\n",SNDSoundError(err));
	exit(1);
    }
    return err;
}



