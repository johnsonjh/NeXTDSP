/*
 * Simple command line utility to convert soundfiles between different
 * useful formats, including from ascii test files.
 * Written by Lee Boynton
 * dana c. massie added code to convert ascii to mulaw
 * also added scaling commandline argument to normalize
 * float input values.
 *
 * Modification History:
 *	04/24/90/mtm	Initialize err in convert_from_ascii().
 *	08/08/90/mtm	Enable arbitrary sample rate conversion.
 *	10/15/90/mtm	Add stereo to mono conversion (bug #10777).
 */
#import <stdlib.h>
#import <string.h>
#import <stdio.h>
#import <fcntl.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sound/sound.h>

extern int getopt(int argc, char **argv, char *optstring);
extern int open();
extern int close();
extern int read();
extern int write();
extern int lseek();

#define BUFSIZE 65536

usage()
{
    fprintf(stderr, "usage : sndconvert\n"
				"\t[-o <output file>]\n"
				"\t[-f <data format code>]\n"
				"\t  format codes:  MULAW_8   = 1\n"
				"\t\t    LINEAR_8  = 2\n"
				"\t\t    LINEAR_16 = 3\n"
				"\t\t    FLOAT     = 6\n"
				"\t\t    DOUBLE    = 7\n"
				"\t[-g <2 ** scale factor (def=15)>]\n"
				"\t[-s <sampling rate>]\n"
				"\t[-c <channel count>]\n"
				"\t[-i comment]\n"
				"\t[-r conversion from raw binary data]\n"
				"\t[-a conversion from raw ascii data]\n"
				"\t[-d convert Motorola dsp .lod file]\n"
				"\tfile\n");
    exit(1);
}

crash(char *s)
{
    fprintf(stderr,"sndconvert : %s\n",s);
    exit(1);
}

crasherr (int err)
{
    crash(SNDSoundError(err));
}

force_extension(char *buf, char *path, char *extension)
{
    char *p;
    int plen = strlen(path), elen = strlen(extension);
    strcpy(buf,path);
    p = rindex(buf,extension[0]);
    if ((plen < elen) || !p)
	strcat(buf,extension);
    else if (p)
	strcpy(p,extension);
}

int ensure_extension(char *buf, char *path, char *extension)
{
    char *p;
    int plen = strlen(path), elen = strlen(extension);
    strcpy(buf,path);
    p = rindex(buf,extension[0]);
    if ((plen < elen) || !p)
	strcat(buf,extension);
    else if (p) {
	if (strcmp(p,extension)) return 0;
	else return 1;
    } else
	return 1;
}

parse_lod(char *infile, char *outfile, int format, int rate,
					 int nchan, char *info)
{
    SNDSoundStruct *s;
    int err;
    char buf[1024];
    
    if (err = SNDReadDSPfile(infile, &s, info))
	return err;
    s->channelCount = 2;
    s->dataFormat = format;
    if (rate > 30000)
	s->samplingRate = (int)SND_RATE_HIGH;
    else if (rate > 10000)
	s->samplingRate = (int)SND_RATE_LOW;
    else
	s->samplingRate = s->channelCount = 0;
    if (!outfile) outfile = infile;
    force_extension(buf,outfile,".snd");
    return SNDWriteSoundfile(buf, s);
}

int infosize (char * info)
{
    int err, infoSize, infoSizeMin=128-24;
    if (!info)
	infoSize = infoSizeMin;
    else {
	infoSize = strlen(info)+1;
	if (infoSize <= infoSizeMin)
	    infoSize = infoSizeMin;
	else
	    infoSize = (((infoSize - infoSizeMin) + 127) >> 7) * 128 +
		    infoSizeMin;
    }
    return infoSize;
}

int write_header(char *outfile, int size, int format, int rate,
			int nchan, char *info)
{
    int err;
    SNDSoundStruct *s;
    int fd = open(outfile,O_WRONLY|O_TRUNC|O_CREAT, 0666);
    if (fd == -1) return -1;
    err = SNDAlloc(&s, size, format, rate, nchan, infosize(info));
    if (err) return -1;
    if (info)
	strcpy(s->info,info);
    err = SNDWriteHeader(fd,s);
    if (err) return -1;
    return fd;
}

int build_from_raw(char *infile, char *outfile, int format, 
	   int rate, int nchan, char *info)
{
    struct stat statb;
    SNDSoundStruct *s;
    int infd, outfd, count;
    int remainingBytes;
    char *buf, filebuf[1024];
    int err;

    if (rate < 0) rate = SND_RATE_LOW;
    if (format < 0) format = SND_FORMAT_LINEAR_16;
    if (nchan < 0) nchan = 2;

    infd = open(infile, O_RDONLY, 0);
    if (infd == -1) return SND_ERR_CANNOT_OPEN;
    fstat(infd, &statb);
    force_extension(filebuf,outfile,".snd");
    outfd = write_header(filebuf, statb.st_size, format, 
    				rate, nchan, info);
    if (outfd == -1) return SND_ERR_CANNOT_WRITE;

    remainingBytes = statb.st_size;
    buf = (char *)malloc(BUFSIZE);
    while (remainingBytes > 0) {
	count = (remainingBytes > BUFSIZE)? BUFSIZE : remainingBytes;
	if (read(infd, buf, count) != count)
	    return SND_ERR_CANNOT_READ;
	if (write(outfd, buf, count) != count)
	    return SND_ERR_CANNOT_WRITE;
	remainingBytes -= BUFSIZE;
    }
    close(outfd);
    close(infd);
    free(buf);
    return SND_ERR_NONE;
}

int convert_from_ascii(char *infile, char *outfile, int format, 
	   	int rate, int nchan, char *info, double RealScaleFactor)
{
    struct stat statb;
    SNDSoundStruct *s;
    int outfd, count;
    FILE *infp;
    char *buf, filebuf[1024];
    int err = SND_ERR_NONE;
    double insamp;
    unsigned char chartemp;

    infp = fopen(infile, "r");
    if (!infp) return SND_ERR_CANNOT_OPEN;
    if (!outfile) outfile = infile;
    force_extension(filebuf,outfile,".snd");
    outfd = write_header(filebuf, 0, format, 
    				rate, nchan, info);
    if (outfd == -1) {
	fclose(infp);
	return SND_ERR_CANNOT_WRITE;
    }
    if (format == SND_FORMAT_DOUBLE) {
	double temp;
	count = 0;
	while (fscanf(infp,"%lf",&insamp) != EOF) {
	    temp = insamp;
	    if (write(outfd,&temp,sizeof(double)) != sizeof(double)) {
		err = SND_ERR_CANNOT_WRITE;
		break;
	    }
	    count++;
	}
	if (!err) { /* update the header! */
	    lseek(outfd,8,0);
	    count *= sizeof(double);
	    if (write(outfd,&count,4) != 4)
		err = SND_ERR_CANNOT_WRITE;
	    else
		err = SND_ERR_NONE;
	}
    } else if (format == SND_FORMAT_FLOAT) {
	float temp;
	count = 0;
	while (fscanf(infp,"%lf",&insamp) != EOF) {
	    temp = (float)insamp;
	    if (write(outfd,&temp,sizeof(float)) != sizeof(float)) {
		err = SND_ERR_CANNOT_WRITE;
		break;
	    }
	    count++;
	}
	if (!err) {/* update the header! */
	    lseek(outfd,8,0);
	    count *= sizeof(float);
	    if (write(outfd,&count,4) != 4)
		err = SND_ERR_CANNOT_WRITE;
	    else
		err = SND_ERR_NONE;
	}
    } else if (format == SND_FORMAT_LINEAR_16) {
	short temp;
	count = 0;
	while (fscanf(infp,"%lf",&insamp) != EOF) {
	    temp = (short)(insamp*RealScaleFactor);
	    if (write(outfd,&temp,sizeof(short)) != sizeof(short)) {
		err = SND_ERR_CANNOT_WRITE;
		break;
	    }
	    count++;
	}
	if (!err) {/* update the header! */
	    lseek(outfd,8,0);
	    count *= sizeof(short);
	    if (write(outfd,&count,4) != 4)
		err = SND_ERR_CANNOT_WRITE;
	    else
		err = SND_ERR_NONE;
	}
    } else if (format == SND_FORMAT_MULAW_8) {
	int	inword;
	count = 0;
	while (fscanf(infp,"%ld",&inword) != EOF) {
	    chartemp = (unsigned char)(inword);
	    if (write(outfd,&chartemp,sizeof(unsigned char))
	        != sizeof(unsigned char)) {
		err = SND_ERR_CANNOT_WRITE;
		break;
	    }
	    count++;
	}
	if (!err) {/* update the header! */
	    lseek(outfd,8,0);
	    count *= sizeof(unsigned char);
	    if (write(outfd,&count,4) != 4)
		err = SND_ERR_CANNOT_WRITE;
	    else
		err = SND_ERR_NONE;
	}
    } else

    close(outfd);
    fclose(infp);
    return err;
}
int mulaw_to_linear(char *infile, char *outfile, 
				SNDSoundStruct *oldHeader,  char *info)
{
    int infd, outfd, err=0, remainingBytes, bytes, i;
    char filename[1024];
    unsigned char *inbuf;
    short *outbuf;
    
    if (!outfile) outfile = infile;
    force_extension(filename,outfile,".snd");
    if (!strcmp(infile,filename)) {
	strcat(filename,"~");
	rename(infile,filename);
	outfile = infile;
	infile = filename;
    } else
	outfile = filename;
    infd = open(infile,O_RDONLY,0);
    if (infd == -1) return SND_ERR_CANNOT_READ;
    outfd = write_header(outfile, oldHeader->dataSize*2, 
    				SND_FORMAT_LINEAR_16, 
    				oldHeader->samplingRate, 
				oldHeader->channelCount, info);
    if (outfd == -1) {
	close(infd);
	return SND_ERR_CANNOT_WRITE;
    }
    lseek(infd,oldHeader->dataLocation,0);
    inbuf = (unsigned char *)malloc(BUFSIZE);
    outbuf = (short *)malloc(BUFSIZE*sizeof(short));
    remainingBytes = oldHeader->dataSize;
    while (remainingBytes > 0) {
	bytes = (remainingBytes > BUFSIZE)? BUFSIZE : remainingBytes;
	if (read(infd,inbuf,bytes) != bytes) {
	    err = SND_ERR_CANNOT_READ;
	    break;
	}
	for (i=0; i<bytes; i++)
	    outbuf[i] = SNDiMulaw(inbuf[i]);
	if (write(outfd,outbuf,bytes*2) != bytes*2) {
	    err = SND_ERR_CANNOT_WRITE;
	    break;
	}
	remainingBytes -= bytes;
    }
    close(infd);
    close(outfd);
    free(inbuf);
    free(outbuf);
    return err;
}

int mono_to_stereo(char *infile, char *outfile, 
			SNDSoundStruct *oldHeader,  char *info, int dataFormat)
{
    int infd, outfd, err=0, remainingBytes, bytes, i;
    char filename[1024];
    unsigned char *inbuf;
    unsigned char *outbuf;
    char *junkptr;	// not used
    int width;
    int size;
    
    if (!outfile) outfile = infile;
    force_extension(filename,outfile,".snd");
    if (!strcmp(infile,filename)) {
	strcat(filename,"~");
	rename(infile,filename);
	outfile = infile;
	infile = filename;
    } else
	outfile = filename;
    infd = open(infile,O_RDONLY,0);
    if (infd == -1) return SND_ERR_CANNOT_READ;
    err = SNDGetDataPointer(oldHeader, &junkptr, &size, &width);

    outfd = write_header(outfile, oldHeader->dataSize*2, 
    				dataFormat,     // same data format!
    				oldHeader->samplingRate, 
				2,		// channel count
				info);
    if (outfd == -1) {
	close(infd);
	return SND_ERR_CANNOT_WRITE;
    }
    lseek(infd,oldHeader->dataLocation,0);
    inbuf = (unsigned char *)malloc(BUFSIZE);
    
    outbuf = malloc(BUFSIZE*2);
    remainingBytes = oldHeader->dataSize;
    while (remainingBytes > 0) {
	bytes = (remainingBytes > BUFSIZE)? BUFSIZE : remainingBytes;
	if (read(infd,inbuf,bytes) != bytes) {
	    err = SND_ERR_CANNOT_READ;
	    break;
	}
	switch (width)
	{
	    case 2: {
		short *src, *dst;
		src = (short *)inbuf;
		dst = (short *)outbuf;
		for (i=bytes/2; i>0; i--) {
		    *dst++ = *src;
		    *dst++ = *src++;
		}
		break;
	    }
	    case 4: {
		int *src, *dst;
		src = (int *)inbuf;
		dst = (int *)outbuf;
		for (i=bytes/4; i>0; i--) {
		    *dst++ = *src;
		    *dst++ = *src++;
		}
		break;
	    }
	    default:
		err = SND_ERR_BAD_CHANNEL;
		break;
	}
	if (write(outfd,outbuf,bytes*2) != bytes*2) {
	    err = SND_ERR_CANNOT_WRITE;
	    break;
	}
	remainingBytes -= bytes;
    }
    close(infd);
    close(outfd);
    free(inbuf);
    free(outbuf);
    return err;
}

int stereo_to_mono(char *infile, char *outfile, 
		   SNDSoundStruct *oldHeader,  char *info, int dataFormat)
{
    int infd, outfd, err=0, remainingBytes, bytes, i;
    char filename[1024];
    unsigned char *inbuf;
    unsigned char *outbuf;
    char *junkptr;	// not used
    int width;
    int size;
    
    if (!outfile) outfile = infile;
    force_extension(filename,outfile,".snd");
    if (!strcmp(infile,filename)) {
	strcat(filename,"~");
	rename(infile,filename);
	outfile = infile;
	infile = filename;
    } else
	outfile = filename;
    infd = open(infile,O_RDONLY,0);
    if (infd == -1) return SND_ERR_CANNOT_READ;
    err = SNDGetDataPointer(oldHeader, &junkptr, &size, &width);

    outfd = write_header(outfile, oldHeader->dataSize/2, 
			 dataFormat,     // same data format!
			 oldHeader->samplingRate, 
			 1,		// channel count
			 info);
    if (outfd == -1) {
	close(infd);
	return SND_ERR_CANNOT_WRITE;
    }
    lseek(infd,oldHeader->dataLocation,0);
    inbuf = (unsigned char *)malloc(BUFSIZE);
    
    outbuf = malloc(BUFSIZE/2);
    remainingBytes = oldHeader->dataSize;
    while (remainingBytes > 0) {
	bytes = (remainingBytes > BUFSIZE)? BUFSIZE : remainingBytes;
	if (read(infd,inbuf,bytes) != bytes) {
	    err = SND_ERR_CANNOT_READ;
	    break;
	}
	switch (width)
	{
	    case 2: {
		short *src, *dst;
		int aSample;
		src = (short *)inbuf;
		dst = (short *)outbuf;
		for (i=bytes/2; i>0; i--) {
		    aSample = *src++;
		    aSample += *src++;
		    aSample += aSample & 1;	/* Round up if odd */
		    *dst++ = aSample / 2;
		}
		break;
	    }
	    case 4: {
		int *src, *dst, aSample1, aSample2;
		src = (int *)inbuf;
		dst = (int *)outbuf;
		for (i=bytes/4; i>0; i--) {
		    aSample1 = *src++;
		    aSample1 += aSample1 & 1;
		    aSample2 = *src++;
		    aSample2 += aSample2 & 1;
		    *dst = (aSample1 / 2) + (aSample2 /2);
		    if (*dst > 32767)
			*dst = 32767;
		    dst++;
		}
		break;
	    }
	    default:
		err = SND_ERR_BAD_CHANNEL;
		break;
	}
	if (write(outfd,outbuf,bytes/2) != bytes/2) {
	    err = SND_ERR_CANNOT_WRITE;
	    break;
	}
	remainingBytes -= bytes;
    }
    close(infd);
    close(outfd);
    free(inbuf);
    free(outbuf);
    return err;
}

int linear_to_mulaw(char *infile, char *outfile, 
				SNDSoundStruct *oldHeader,  char *info)
{
    int infd, outfd, err=0, remainingBytes, bytes, i;
    char filename[1024];
    unsigned char *outbuf;
    short *inbuf;
    
    if (!outfile) outfile = infile;
    force_extension(filename,outfile,".snd");
    if (!strcmp(infile,filename)) {
	strcat(filename,"~");
	rename(infile,filename);
	outfile = infile;
	infile = filename;
    } else
	outfile = filename;
    infd = open(infile,O_RDONLY,0);
    if (infd == -1) return SND_ERR_CANNOT_READ;
    outfd = write_header(outfile, oldHeader->dataSize*2, 
    				SND_FORMAT_MULAW_8, 
    				oldHeader->samplingRate, 
				oldHeader->channelCount, info);
    if (outfd == -1) {
	close(infd);
	return SND_ERR_CANNOT_WRITE;
    }
    lseek(infd,oldHeader->dataLocation,0);
    outbuf = (unsigned char *)malloc(BUFSIZE);
    inbuf = (short *)malloc(BUFSIZE*sizeof(short));
    remainingBytes = oldHeader->dataSize;
    while (remainingBytes > 0) {
	bytes = (remainingBytes > BUFSIZE)? BUFSIZE : remainingBytes;
	if (read(infd,inbuf,bytes) != bytes) {
	    err = SND_ERR_CANNOT_READ;
	    break;
	}
	for (i=0; i<bytes/2; i++)
	    outbuf[i] = SNDMulaw(inbuf[i]);
	if (write(outfd,outbuf,bytes/2) != bytes/2) {
	    err = SND_ERR_CANNOT_WRITE;
	    break;
	}
	remainingBytes -= bytes;
    }
    close(infd);
    close(outfd);
    free(outbuf);
    free(inbuf);
    return err;
}

int convert_from_real(char *infile, char *outfile, 
		SNDSoundStruct *oldHeader, char *info, double RealScaleFactor)
{
    int infd, outfd, err=0, remainingBytes, bytes, i;
    char filename[1024];
    short *outbuf;
    char *inbuf;
    float *float_inbuf;
    double *double_inbuf;
    int sizefactor = 2;
    
    if (oldHeader->dataFormat == SND_FORMAT_DOUBLE)
	sizefactor *= 2;
    
    if (!outfile) outfile = infile;
    force_extension(filename,outfile,".snd");
    if (!strcmp(infile,filename)) {
	strcat(filename,"~");
	rename(infile,filename);
	outfile = infile;
	infile = filename;
    } else
	outfile = filename;
    infd = open(infile,O_RDONLY,0);
    if (infd == -1) return SND_ERR_CANNOT_READ;
    outfd = write_header(outfile, oldHeader->dataSize/sizefactor, 
    				SND_FORMAT_LINEAR_16, 
    				oldHeader->samplingRate, 
				oldHeader->channelCount, info);
    if (outfd == -1) {
	close(infd);
	return SND_ERR_CANNOT_WRITE;
    }
    lseek(infd,oldHeader->dataLocation,0);
    outbuf = (short *)malloc(BUFSIZE);
    inbuf = (char *)malloc(BUFSIZE*sizefactor);
    remainingBytes = oldHeader->dataSize;
    while (remainingBytes > 0) {
	bytes = (remainingBytes > BUFSIZE)? BUFSIZE : remainingBytes;
	if (read(infd,inbuf,bytes) != bytes) {
	    err = SND_ERR_CANNOT_READ;
	    break;
	}
	if (sizefactor == 2) {
	    float_inbuf = (float *)inbuf;
	    for (i=0; i<(bytes/(2*sizefactor)); i++)
		outbuf[i] = (short)(RealScaleFactor*float_inbuf[i]);
	} else {
	    double_inbuf = (double *)inbuf;
	    for (i=0; i<(bytes/(2*sizefactor)); i++)
		outbuf[i] = (short)(RealScaleFactor*double_inbuf[i]);
	}
	if (write(outfd,outbuf,(bytes/sizefactor)) != (bytes/sizefactor)) {
	    err = SND_ERR_CANNOT_WRITE;
	    break;
	}
	remainingBytes -= bytes;
    }
    close(infd);
    close(outfd);
    free(outbuf);
    free(inbuf);
    return err;
}


int convert_to_real(char *infile, char *outfile, 
		SNDSoundStruct *oldHeader, char *info, double RealScaleFactor)
{
    int infd, outfd, err=0, remainingBytes, bytes, i;
    char filename[1024];
    char *outbuf;
    short *inbuf;
    float *float_outbuf;
    double *double_outbuf;
    int sizefactor = 2;
    
    if (oldHeader->dataFormat == SND_FORMAT_DOUBLE)
	sizefactor *= 2;
    
    if (!outfile) outfile = infile;
    force_extension(filename,outfile,".snd");
    if (!strcmp(infile,filename)) {
	strcat(filename,"~");
	rename(infile,filename);
	outfile = infile;
	infile = filename;
    } else
	outfile = filename;
    infd = open(infile,O_RDONLY,0);
    if (infd == -1) return SND_ERR_CANNOT_READ;
    outfd = write_header(outfile, oldHeader->dataSize*sizefactor, 
    				oldHeader->dataFormat, 
    				oldHeader->samplingRate, 
				oldHeader->channelCount, info);
    if (outfd == -1) {
	close(infd);
	return SND_ERR_CANNOT_WRITE;
    }
    lseek(infd,oldHeader->dataLocation,0);
    inbuf = (short *)malloc(BUFSIZE);
    outbuf = (char *)malloc(BUFSIZE*sizefactor);
    remainingBytes = oldHeader->dataSize;
    while (remainingBytes > 0) {
	bytes = (remainingBytes > BUFSIZE)? BUFSIZE : remainingBytes;
	if (read(infd,inbuf,bytes) != bytes) {
	    err = SND_ERR_CANNOT_READ;
	    break;
	}
	if (sizefactor == 2) {
	    float_outbuf = (float *)outbuf;
	    for (i=0; i<(bytes/sizeof(short)); i++)
		float_outbuf[i] = ((float)inbuf[i])/RealScaleFactor;
	} else {
	    double_outbuf = (double *)outbuf;
	    for (i=0; i<(bytes/sizeof(short)); i++)
		double_outbuf[i] = ((double)inbuf[i])/RealScaleFactor;
	}
	if (write(outfd,outbuf,(bytes*sizefactor)) != (bytes*sizefactor)) {
	    err = SND_ERR_CANNOT_WRITE;
	    break;
	}
	remainingBytes -= bytes;
    }
    close(infd);
    close(outfd);
    free(outbuf);
    free(inbuf);
    return err;
}

main(argc, argv)
    int argc;
    char **argv;
{
    extern char *optarg;
    extern int optind;
    char *optstring = "o:i:f:g:s:c:radm";
    int opt, err;
    int format = -1;
    float rate = -1.0;
    int nchan = -1;
    char *infile;
    char *outfile = NULL;
    char *info = NULL;
    int raw = 0, dsp = 0, mk = 0, ascii = 0;
    double RealScaleFactor = 32767.0;
    int scaleExponent= 15; // specify the scale factor for translating ascii
    
    if (argc == 1)
	usage();
    while ((opt = getopt(argc, argv, optstring)) != EOF)
	switch (opt) {
	    case 'o':
		outfile = optarg;
		break;
	    case 'f':
		format = atoi(optarg);
		break;
	    case 'g':
	        scaleExponent = atoi(optarg);
		RealScaleFactor = ((double) ((1 << scaleExponent)-1));
		fprintf(stderr, "RealScaleFactor= %f\n",RealScaleFactor);
		break;
	    case 's':
		rate = (float)atof(optarg);
		break;
	    case 'c':
		nchan = atoi(optarg);
		break;
	    case 'r':
	    	raw = 1;
		ascii = mk = dsp = 0;
		break;
	    case 'a':
	    	ascii = 1;
		raw = mk = dsp = 0;
		break;
	    case 'd':
	    	dsp = 1;
		raw = mk = ascii = 0;
		break;
	    case 'i':
		info = optarg;
		break;
	    case '?':
	    default:
		    usage();
	}
    if (argc != optind+1) {
	fprintf(stderr, "sndconvert : no file specified\n");
	exit(1);
    }
    infile = argv[optind];
    
/*  we should figure out how to display the destination format for the user ...
    fprintf(stderr,"output file info:\n");
    fprintf(stderr,"format = %d\n",format);
    fprintf(stderr,"nchan = %d\n",nchan);
    fprintf(stderr,"rate = %f\n",rate);
    fprintf(stderr,"infile = %s\n",infile);
*/

    if (dsp) {
	char buf[1024];
	format = SND_FORMAT_DSP_CORE;
	if (ensure_extension(buf,infile,".lod"))
	    err = parse_lod(buf,outfile,format,rate,nchan,info);
	else
	    crash("input not a Motorola DSP absolute load image"); 
    } else if (raw)
	err = build_from_raw(infile,outfile,format,(int)rate,nchan,info);
    else if (ascii)
	err = convert_from_ascii(
	  infile,outfile,format,(int)rate,nchan,info,RealScaleFactor);
    else {
	char buf[1024];
	SNDSoundStruct *oldHeader;
	int fd;
	int sRate = (int)rate;
	if (!ensure_extension(buf,infile,".snd"))
	    crash("input not a soundfile");
	infile = buf;
	fd = open(buf,O_RDONLY);
	if (fd < 0) crasherr(SND_ERR_CANNOT_OPEN);
	err = SNDReadHeader(fd,&oldHeader);
	close(fd);
	if (err) crasherr(err);

	if (sRate < 0) sRate = oldHeader->samplingRate;
	if (format < 0) format = oldHeader->dataFormat;
	if (nchan < 0) nchan = oldHeader->channelCount;

	if (sRate != oldHeader->samplingRate) {
	    if ((oldHeader->samplingRate == (int)SND_RATE_CODEC) &&
		(sRate == SND_RATE_LOW) &&
		(oldHeader->dataFormat == SND_FORMAT_MULAW_8) &&
		(nchan == 2)) {
		    
		// upsample codec data here!
		
		SNDSoundStruct *s1, *s2;
		SNDSoundStruct s3 = {
		    SND_MAGIC, 0, 0, 
		    SND_FORMAT_LINEAR_16, (int)SND_RATE_LOW, 
		    2, "" };
		
		err = SNDReadSoundfile(infile,&s1);
		s2 = &s3;
		err = SNDConvertSound(s1,&s2);
		err = SNDWriteSoundfile(outfile,s2);
	
		if (err)
		    crash("conversion not implemented");
	    } else {
		// try arbitrary sample rate conversion here
		
	        SNDSoundStruct *s1, *s2;
	        SNDSoundStruct s3 = {
		    SND_MAGIC, 0, 0, 
		    oldHeader->dataFormat, sRate, 
		    oldHeader->channelCount, "" };
	    
	        err = SNDReadSoundfile(infile,&s1);
	        s2 = &s3;
	        err = SNDConvertSound(s1,&s2);
	        err = SNDWriteSoundfile(outfile,s2);
	
	        if (err)
	            crash("conversion not implemented");
	    }
	} else if (nchan != oldHeader->channelCount) {
	    if ((nchan == 2) && (oldHeader->channelCount == 1)) {
	    // then we can perform mono to stereo
		err = mono_to_stereo( infile,  outfile,  
			    oldHeader,  info,  format);
	    } else if ((nchan == 1) && (oldHeader->channelCount == 2)) {
		err = stereo_to_mono(infile, outfile,
				     oldHeader, info, format);
	    } else
		crash("conversion not implemented");
	} else if (format != oldHeader->dataFormat) {
	    if (oldHeader->dataFormat == SND_FORMAT_MULAW_8) {
		if (format == SND_FORMAT_LINEAR_16) {
		    err = mulaw_to_linear(infile,outfile,oldHeader,info);
		} else 
		    crash("conversion not implemented");
	    } else if (oldHeader->dataFormat == SND_FORMAT_LINEAR_16) {
		if (format == SND_FORMAT_MULAW_8)
		    err = linear_to_mulaw(infile,outfile,oldHeader,info);
		else if (format == SND_FORMAT_FLOAT) {
		    oldHeader->dataFormat = format;
		    err = convert_to_real(
		       infile, outfile, oldHeader, info,RealScaleFactor);
		} else if (format == SND_FORMAT_DOUBLE) {
		    oldHeader->dataFormat = format;
		    err = convert_to_real(
		    infile, outfile, oldHeader, info ,RealScaleFactor);
		} else
		    crash("conversion not implemented");
	    } else if (oldHeader->dataFormat == SND_FORMAT_FLOAT) {
	    	if (format == SND_FORMAT_LINEAR_16) {
		    oldHeader->dataFormat = SND_FORMAT_FLOAT;
		    err = convert_from_real(
		      infile, outfile, oldHeader, info ,RealScaleFactor);
		} else
		    crash("conversion not implemented");
	    } else if (oldHeader->dataFormat == SND_FORMAT_DOUBLE) {
	    	if (format == SND_FORMAT_LINEAR_16) {
		    oldHeader->dataFormat = SND_FORMAT_DOUBLE;
		    err = convert_from_real(
		      infile, outfile, oldHeader, info ,RealScaleFactor);
		} else
		    crash("conversion not implemented");
	    } else
		crash("conversion not implemented");
	} else {
	    crash("No conversion performed");
	}
    }
    if (err) crasherr(err);
    exit (0);
}


