#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	filesound.c
 *	Written by Lee Boynton
 *	Copyright 1988 NeXT, Inc.
 *
 *	Modification History:
 *	04/11/90/mtm	Added #import <stdlib.h> per OS request.
 *	06/04/90/mtm	Allow (ignore) memory space qualifiers (eg. "PI") when
 *			parsing .lod file.  Fix to bug #3023.
 *	07/18/90/mtm	Get rid of static data in get_token().
 *
 */

#import <stdlib.h>
#import <mach.h>
#import <stdlib.h>
#import <string.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/file.h>
#import "utilsound.h"
#import "filesound.h"

extern int close();
extern int map_fd();
extern int open();
extern int read();
extern int write();

static int calc_header_size(s)
    SNDSoundStruct *s;
{
    int size = strlen(s->info) + 1;
    if (size < 4) size = 4;
    else size = (size + 3) & ~3;
    return(sizeof(SNDSoundStruct) - 4 + size);
}

static int calc_data_size(s)
    SNDSoundStruct *s;
{
    int total=0, i, iMax = (s->dataSize / sizeof(SNDSoundStruct **));
    SNDSoundStruct **iBlock, *s2;
    
    iBlock = (SNDSoundStruct **)s->dataLocation;
    for (i=0; i<iMax; i++) {
	if (s2 = iBlock[i])
	    total += s2->dataSize;
	else
	    break;
    }
    return total;
}

static int calc_format(s)
    SNDSoundStruct *s;
{
    SNDSoundStruct **iBlock, *s2;
    
    iBlock = (SNDSoundStruct **)s->dataLocation;
    if (s2 = iBlock[0])
	return s2->dataFormat;
    else
	return 0;
}


#define WHITESPACE(c) ((c == ' ') || (c=='\n') || (c=='\t'))

static char *get_token(char **file_ptr, char *tok)
{
    int i = 0, j = 0;
    char c, *p = *file_ptr;

    while (c = *p) {
	if(!WHITESPACE(c)) break;
	p++;
	j++;
    }
    if (!c)
	return NULL;
    else {
	while (c = *p++) {
	    if(WHITESPACE(c))
		break;
	    tok[i++] = c;
	}
    }
    tok[i] = '\0';
    *file_ptr += (i+j);
    return tok;
}

#define DIGIT(c) (((c>='0')&&(c<='9'))||((c>='A')&&(c<='F')))
#define DIGIT_VALUE(c) ((c < 'A') ? (c - '0') : (c - 'A' + 10))

static int hexNum(char *tok)
{
    char c, *p = tok;
    int val = 0;
    while ((c=*p++) && DIGIT(c))
	val = (val<<4) | DIGIT_VALUE(c);
    return(val);
}


#define PMEM_MAX (512)

static int parse_lod(char *filePtr,int fileSize,SNDSoundStruct **s,char *info)
{
    int i, err, state=-1, count=-1, dp_size = 0;
    int max=-1, adr=-1, dat, codeBlock[PMEM_MAX];
    int *dP, *dCount=(int *)0;
    int *dirBlock = (int *)malloc(4*(8192+512+256+256)*4);
    char *p, *tok, *in_ptr, *in = (char *)malloc(fileSize+1);
    char tokbuf[256];

    if (!in || !dirBlock)
	return SND_ERR_CANNOT_ALLOC;
    dP = dirBlock;
    memmove(in,filePtr,fileSize);
    in[fileSize] = '\0';
    in_ptr = in;
    while (tok = get_token(&in_ptr, tokbuf)) {
	if (!strcmp(tok,"_DATA")) {
	    state = 0;
	} else if (!strcmp(tok,"_BLOCKDATA")) {
	    state = 10;
	} else if (!strcmp(tok,"_END")) {
	    break;
	} else if (state == 0 || state == 10) {
	    if (!strncmp(tok,"P",1)) state += 4;
	    else if (!strncmp(tok,"X",1)) state += 1;
	    else if (!strncmp(tok,"Y",1)) state += 2;
	    else if (!strncmp(tok,"L",1)) state += 3;
	    else return SND_ERR_BAD_SPACE;
	} else if (state > 0 && state < 5) {
	    adr = hexNum(tok);
	    if (state == 4 && adr < PMEM_MAX) {
		state = 5;
	    } else {
		*dP++ = state;
		*dP++ = adr;
		dCount = dP++;
		*dCount = 0;
		dp_size+=3;
		state = 6;
	    }
	} else if (state == 5) {
	    dat = hexNum(tok);
	    if (adr >= PMEM_MAX)
		state = -1;
	    else {
		if (adr > max) max = adr;
		codeBlock[adr++] = dat;
	    }
	} else if (state == 6) {
	    dat = hexNum(tok);
	    *dP++ = dat;
	    dp_size++;
	    (*dCount)++;
	} else if (state > 10 && state < 15) {
	    adr = hexNum(tok);
	    if (state == 14 && adr < PMEM_MAX) {
		state = 15;
	    } else {
		*dP++ = state-10;
		*dP++ = adr;
		dCount = dP++;
		*dCount = 0;
		dp_size+=3;
		state = 16;
	    }
	} else if (state == 15) {
	    count = hexNum(tok);
	    if ((adr+count) > PMEM_MAX)
		count = PMEM_MAX-adr;
	    state = 17;
	} else if (state == 17) {
	    dat = hexNum(tok);
	    for (i=0;i<count;i++)
		codeBlock[adr++] = dat;
	    if ((adr-1) > max) max = adr-1;
	    state = -1;
	} else if (state == 16) {
	    count = hexNum(tok);
	    state = 19;
	} else if (state == 19) {
	    dat = hexNum(tok);
	    for (i=0;i<count;i++)
		*dP++ = dat;
	    *dCount = count;
	    dp_size+=count;
	    state = -1;
	}
    }
    free(in);
    if (max >= 0) {
	int *pI, infoSize, infoSizeMin=128-24, k=11;
	max++;
	if ((int)info == -1) {
	    info = (char *)0;
	    k = 3;
	    infoSize = infoSizeMin;
	} else if (!info)
	    infoSize = infoSizeMin;
	else {
	    infoSize = strlen(info)+1;
	    if (infoSize <= infoSizeMin)
		infoSize = infoSizeMin;
	    else
		infoSize = (((infoSize - infoSizeMin) + 127) >> 7) * 128 +
			infoSizeMin;
	}
	err = SNDAlloc(s,(max+dp_size+k)*4,SND_FORMAT_DSP_CORE,0,0,infoSize);
	if (err) return err;
	if (info) strcpy((*s)->info,info);
	p = (char *)(*s);
	p += (*s)->dataLocation;
	if (k == 11) {
	    pI = (int *)p;
	    *pI++ = 0; //system data block type
	    *pI++ = 0; //system version
	    *pI++ = 5; //block size
	    *pI++ = 0; //sample count high
	    *pI++ = 0; //sample count low
	    *pI++ = 0; //buffer size
	    *pI++ = 0; //unused
	    *pI++ = 0; //unused
	    p = (char *)pI;
	}
	memmove(p,dirBlock,dp_size*4);
	free(dirBlock);
	p += dp_size*4;
	pI = (int *)p;
	*pI++ = 4; /* program space */
	*pI++ = 0; /* beginning address */
	*pI++ = max; /* word count */
	p = (char *)pI;
	memmove(p,codeBlock,max*4);
	return SND_ERR_NONE;
    } else
	return SND_ERR_NOT_SOUND;
}

static int fixHeader(SNDSoundStruct *s, int stat_size)
{
    int dataWidth = SNDSamplesToBytes(1,s->channelCount,s->dataFormat);
    
    if (s->samplingRate > 7090 && s->samplingRate < 8030)
	s->samplingRate = (int)SND_RATE_CODEC;

    if (s->dataLocation < sizeof(SNDSoundStruct)) {
	s->dataLocation = sizeof(SNDSoundStruct);
	s->info[0] = '\0';
    } else {
	char *p = (char *)s;
	p += (s->dataLocation - 1);
	*p = '\0';
    }
    if (dataWidth > 0)
	s->dataSize = (stat_size - s->dataLocation) & ~(dataWidth-1);
    return SND_ERR_NONE;
}


/*
 * Exported routines
 */

int SNDReadSoundfile(char *path, SNDSoundStruct **s)
{
    int fd, err;

    if (!path) return SND_ERR_BAD_FILENAME;
    fd = open(path,O_RDONLY);
    if (fd < 0) return SND_ERR_CANNOT_OPEN;
    err = SNDRead(fd,s);
    close(fd);
    return err;
}

int SNDRead(int fd, SNDSoundStruct **s)
{
    struct stat statbuf;
    int magic;

    if (read(fd,&magic,4) != 4) return SND_ERR_CANNOT_READ;
    if (magic != SND_MAGIC) return SND_ERR_NOT_SOUND;
    if (fstat(fd, &statbuf)) return SND_ERR_CANNOT_READ;
    if (map_fd(fd,0,s,1,statbuf.st_size)) return SND_ERR_CANNOT_READ;
    return fixHeader(*s,statbuf.st_size);
}

int SNDReadHeader(int fd, SNDSoundStruct **s)
{
    struct stat statbuf;
    int magic, offset;

    if (read(fd,&magic,4) != 4) return SND_ERR_CANNOT_READ;
    if (magic != SND_MAGIC) return SND_ERR_NOT_SOUND;
    if (read(fd,&offset,4) != 4) return SND_ERR_CANNOT_READ;
    if (map_fd(fd,0,s,1,offset)) return SND_ERR_CANNOT_READ;
    if (fstat(fd, &statbuf)) return SND_ERR_CANNOT_READ;
    return fixHeader(*s,statbuf.st_size);
}

int SNDWriteSoundfile(char *path, SNDSoundStruct *s)
{
    int fd, err;
    
    if (!path) return SND_ERR_BAD_FILENAME;
    fd = open(path,O_WRONLY|O_CREAT|O_TRUNC,0666);
    if (fd < 0) return SND_ERR_CANNOT_OPEN;
    err = SNDWrite(fd,s);
    close(fd);
    return err;
}

extern int errno;
int SNDWrite(int fd, SNDSoundStruct *s)
{
    int size, i;

    if (!s || s->magic != SND_MAGIC) return SND_ERR_NOT_SOUND;
    if (s->dataFormat == SND_FORMAT_INDIRECT) {
	int savedDataLocation = s->dataLocation;
	int savedDataSize = s->dataSize;
	SNDSoundStruct **iBlock, *s2;
	int i, max;
	char *data;
	s->dataSize = calc_data_size(s);
	s->dataFormat = calc_format(s);
	size = s->dataLocation = calc_header_size(s);
	if (write(fd,s,size) != size) {
	    s->dataLocation = savedDataLocation;
	    s->dataSize = savedDataSize;
	    s->dataFormat = SND_FORMAT_INDIRECT;
	    return SND_ERR_CANNOT_WRITE;
	}
	s->dataLocation = savedDataLocation;
	s->dataSize = savedDataSize;
	s->dataFormat = SND_FORMAT_INDIRECT;
	max = s->dataSize;
	iBlock = (SNDSoundStruct **)savedDataLocation;
	for (i=0; i<max; i++) {
	    if (s2 = iBlock[i]) {
		data = (char *)s2;
		data += s2->dataLocation;
		size = s2->dataSize;
		if (write(fd,data,size) != size) return SND_ERR_CANNOT_WRITE;
	    } else
		break;
	}
    } else {
	size = s->dataLocation + s->dataSize;
	i = write(fd,s,size);
	if (i < 0)
	    return SND_ERR_CANNOT_WRITE;
	else if (i != size)
	    return SND_ERR_CANNOT_WRITE;
    }
    return SND_ERR_NONE;
}

int SNDWriteHeader(int fd, SNDSoundStruct *s)
{
    int size;

    if (!s || s->magic != SND_MAGIC) return SND_ERR_NOT_SOUND;
    if (s->dataFormat == SND_FORMAT_INDIRECT) {
	int savedDataLocation = s->dataLocation;
	int savedDataSize = s->dataSize;
	s->dataSize = calc_data_size(s);
	s->dataFormat = calc_format(s);
	size = s->dataLocation = calc_header_size(s);
	if (write(fd,s,size) != size) {
	    s->dataLocation = savedDataLocation;
	    s->dataSize = savedDataSize;
	    s->dataFormat = SND_FORMAT_INDIRECT;
	    return SND_ERR_CANNOT_WRITE;
	}
	s->dataLocation = savedDataLocation;
	s->dataSize = savedDataSize;
	s->dataFormat = SND_FORMAT_INDIRECT;
    } else {
	size = s->dataLocation;
	if (write(fd,s,size) != size) return SND_ERR_CANNOT_WRITE;
    }
    return SND_ERR_NONE;
}

int SNDReadDSPfile(char *path, SNDSoundStruct **s, char *info)
{
    int fd, fileSize, err;
    struct stat statbuf;
    char *filePtr;
    
    if (!path) return SND_ERR_BAD_FILENAME;
    fd = open(path,O_RDONLY);
    if (fd < 0) return SND_ERR_CANNOT_OPEN;
    if (fstat(fd, &statbuf)) return SND_ERR_CANNOT_READ;
    fileSize = statbuf.st_size;
    if (map_fd(fd,0,&filePtr,1,fileSize)) return SND_ERR_CANNOT_READ;
    err = parse_lod(filePtr, fileSize, s, info);
    close(fd);
    return err;
}



