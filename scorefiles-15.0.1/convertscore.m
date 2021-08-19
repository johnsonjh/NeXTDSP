#import <musickit/musickit.h>

#import <string.h>
#import <stdlib.h>
#import <pwd.h>
#import <sys/types.h>
#import <sys/file.h>
#import <musickit/musickit.h>

static char *getHomeDirectory()
{
    static char *homeDirectory;
    struct passwd  *pw;
    if (!homeDirectory) {
	pw = getpwuid(getuid());
	if (pw && (pw->pw_dir) && (*pw->pw_dir)) {
	    homeDirectory = (char *)malloc(strlen(pw->pw_dir)+1);
	    strcpy(homeDirectory,pw->pw_dir);
	}
    }
    return homeDirectory;
}

#define PERMS 0660 /* RW for owner and group. */ 
#define HOME_SCORE_DIR "/Library/Music/Scores/"
#define LOCAL_SCORE_DIR "/LocalLibrary/Music/Scores/"
#define SYSTEM_SCORE_DIR "/NextLibrary/Music/Scores/"

#import <stdio.h>

static int tryIt(char *filename,char *extension,char *name,int addExt,
		 char *dir1,char *dir2)
{
    if (dir1) {
	strcpy(filename,dir1);
	if (dir2)
	  strcat(filename,dir2);
	strcat(filename,name);
    }
    else strcpy(filename,name);
    if (addExt)
      strcat(filename,extension);
    return open(filename,O_RDONLY,PERMS); 
}

static BOOL extensionPresent(char *filename,char *extension)
{
    char *ext = strrchr(filename,'.');
    if (!ext)
      return NO;
    return (strcmp(ext,extension) == 0);
}

static int findFile(char *name)
{
    int fd;
    char filename[1024], *p;
    int addScoreExt,addMidiExt, addPlayscoreExt;
    if (!name) {
	fprintf(stderr,"No file name specified.\n");
	exit(1);
    }
    addScoreExt = (!extensionPresent(name,".score"));
    addPlayscoreExt = (!extensionPresent(name,".playscore"));
    addMidiExt = (!extensionPresent(name,".midi"));
    fd = tryIt(filename,".score",name,addScoreExt,NULL,NULL);
    if (fd != -1) 
      return fd;
    fd = tryIt(filename,".playscore",name,addPlayscoreExt,NULL,NULL);
    if (fd != -1) 
      return fd;
    fd = tryIt(filename,".midi",name,addMidiExt,NULL,NULL);
    if (fd != -1) 
      return fd;
    if (name[0] != '/') { /* There's hope */
	if (p = getHomeDirectory()) {
	    fd = tryIt(filename,".score",name,addScoreExt,p,HOME_SCORE_DIR);
	    if (fd != -1) 
	      return fd;
	    fd = tryIt(filename,".playscore",name,addPlayscoreExt,p,HOME_SCORE_DIR);
	    if (fd != -1) 
	      return fd;
	    fd = tryIt(filename,".midi",name,addMidiExt,p,HOME_SCORE_DIR);
	    if (fd != -1) 
	      return fd;
	}
	
	fd = tryIt(filename,".score",name,addScoreExt,LOCAL_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
	fd = tryIt(filename,".playscore",name,addPlayscoreExt,LOCAL_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
	fd = tryIt(filename,".midi",name,addMidiExt,LOCAL_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
	
	fd = tryIt(filename,".score",name,addScoreExt,SYSTEM_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
	fd = tryIt(filename,".playscore",name,addPlayscoreExt,SYSTEM_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
	fd = tryIt(filename,".midi",name,addMidiExt,SYSTEM_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
    }
    if (fd == -1) {
	fprintf(stderr,"Can't find %s.\n",name);
	exit(1);
    }
    return fd;
}

static char *makeStr(char *str)
{
    char *newStr;
    if (!str)
      return NULL;
    newStr = malloc(strlen(str)+1);
    strcpy(newStr,str);
    return newStr;
}

static char *stripExt(char *str)
{
    char *ext = strrchr(str,'.');
    char *newStr;
    if (!ext)
      return makeStr(str);
    newStr = malloc(ext-str+1);
    memmove(newStr ,str,ext-str);
    newStr[ext-str] = '\0';
    return newStr;
}


const char * const help = "\n"
"usage : convertscore \n"
"        [-m] write midifile (.midi) format \n"
"        [-p] write optimized scorefile (.playscore) format \n"
"        [-s] write scorefile (.score) format \n"
"        [-o <output file>] \n"
"        file\n"
"\n"
"Default output file name is 'convertscore'.\n";

typedef enum _whichOutputFormat {none,midi,score,playscore};

static char *formatStr(int aFormat)
{
    return (aFormat == score) ? ".score" : (aFormat == playscore) ? 
      ".playscore" : ".midi";
}

main(ac, av)
    int ac;
    char * av[];
{
    NXStream *aStream;
    char *inputFile,*outputFile = NULL;
    int errorFlag = 0;
    int outFormat = none,i,inFormat = none,fd,firstWord;
    id aScore;
    if (ac == 1) {
	fprintf(stderr,help);
	exit(1);
    }
    for (i=1; i<(ac-1); i++) {
	if ((strcmp(av[i],"-m") == 0))  /* midi */
	  outFormat = midi;
	else if ((strcmp(av[i],"-p") == 0)) /* optimized scorefile */
	  outFormat = playscore;
	else if (strcmp(av[i],"-s") == 0) 
	  outFormat = score;
	else if (strcmp(av[i],"-o") == 0)  {
	    i++;
	    if (i < ac) 
	      outputFile = makeStr(av[i]);
	}
    }
    inputFile = makeStr(av[ac-1]);
    if (!outputFile) 
      outputFile = stripExt(inputFile); /* Extension added by write routine */
    fd = findFile(inputFile);
    aStream = NXOpenFile(fd,NX_READONLY);
    NXRead(aStream,&firstWord,4);
#   define MIDIMAGIC 1297377380
    aScore = [Score new];                  
    if (firstWord == MK_SCOREMAGIC) 
      inFormat = playscore;
    else if (firstWord == MIDIMAGIC) 
      inFormat = midi;
    else
      inFormat = score;
    NXSeek(aStream, 0, NX_FROMSTART);
    if (outFormat == none) 
      outFormat = (inFormat == playscore) ? score : playscore;
    fprintf(stderr,"Converting from %s to %s format.\n",
	    formatStr(inFormat),
	    formatStr(outFormat));
    switch (inFormat) {
      case score:
      case playscore:
	if (![aScore readScorefileStream:aStream])  {
	    fprintf(stderr,"Fix scorefile errors and try again.\n");
	    exit(1);
	}
	break;
      case midi:
	if (![aScore readMidifileStream:aStream])  {
	    fprintf(stderr,"This doesn't look like a midi file.\n");
	    exit(1);
	}
    }
    switch (outFormat) {
      case score:
	if (![aScore writeScorefile:outputFile])  
	  errorFlag = 1;
	break;
      case playscore:
	if (![aScore writeOptimizedScorefile:outputFile])  
	  errorFlag = 1;
	break;
      case midi:
	if (![aScore writeMidifile:outputFile])  
	  errorFlag = 1;
	break;
    }
    if (errorFlag) {
	fprintf(stderr,"Can't write %s.\n",outputFile);
	exit(1);
    }
    exit(0);
}


