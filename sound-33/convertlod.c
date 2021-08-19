/*
 * convertlod.c - convert a .lod 56k assembler file into soundfile format.
 * This command is used solely to cache an image for the sound library's
 * dsp routines; for general-purpose conversion of real sounds, use the
 * sndconvert command (in the sndcmds project).
 */

#import "sound.h"

static void set_dsp_core_sizes(SNDSoundStruct *s, int samp_count, int buf_size)
{
    int *p = (int *)((int)s + s->dataLocation);
    p[3] = (samp_count>>24)&255;
    p[4] = (samp_count&0xffffff);
    p[5] = buf_size;
}

main (argc,argv)
    int argc;
    char *argv[];
{
    SNDSoundStruct *s;
    int *p, i,j;
    char name[256];
    
    if (argc != 2 && argc != 3)
	crash("usage: sndfromlod <lodfile> [suppress_sys_header]");
    strcpy(name,argv[1]);
    i = strlen(name)-4;
    if ((i<=0) || strcmp(".lod",&name[i]))
	crash("Bad file name");
    if (SNDReadDSPfile(name,&s,(argc == 2)? (char *)0 : (char *)-1))
	crash("Cannot read lodfile");

    s->samplingRate = SND_RATE_HIGH;
    s->channelCount = 2;
//    set_dsp_core_sizes(s,220500,2048); //debug

    name[i] = '\0';
    strcat(name,".snd");
    if (SNDWriteSoundfile(name,s))
	crash("Cannot write soundfile");
    SNDFree(s);
}

crash(s)
    char *s;
{
    printf("*** %s \n",s);
    exit(0);
}

