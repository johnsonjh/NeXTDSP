/* Written by David A. Jaffe. */

/* 
Modification history:

  12/20/89/daj - Added support for optimized scorefiles.
  03/05/90/daj - Removed warning message for no synthPatch in -f mode.
  04/11/90/mtm - Added support for generating DSP Commands soundfile.
  07/1/90/daj -  Added Midi support and repeat count.
  07/26/90/daj - Removed nice() (on conditional compilation).  New MK
                 runs performance at a high-priority (equivalent to nice(-18)).
		 Added setThreadPriority:
  08/06/90/daj - Merged in midi and repeat features from CCRMA summer class
  		 version.
  08/14/90/daj - Added setuid(getuid()) 
  	         There's a definite bug in coordination between DSP and Midi.
  	         The DSP generally starts too soon, which means (probably) that the 
		 sound-out buffers are not getting properly filled.
*/ 

#import <string.h>
#import <stdlib.h>
#import <pwd.h>
#import <sys/types.h>
#import <sys/file.h>
#import <musickit/musickit.h>

static BOOL quiet = NO;

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

#define SOUND_OUT_PAUSE_BUG 1

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
    int addScoreExt = 0, addPlayscoreExt = 0;
    if (!name) {
	fprintf(stderr,"No file name specified.\n");
	exit(1);
    }
    if (!extensionPresent(name,".playscore"))  /* Ext not there */
      addPlayscoreExt = 1;
    if (!extensionPresent(name,".score"))  /* Ext not there */
      addScoreExt = 1;
    fd = tryIt(filename,".playscore",name,addPlayscoreExt,NULL,NULL);
    if (fd != -1) 
      return fd;
    fd = tryIt(filename,".score",name,addScoreExt,NULL,NULL);
    if (fd != -1) 
      return fd;
    if (name[0] != '/') { /* There's hope */
	if (p = getHomeDirectory()) {
	    fd = tryIt(filename,".playscore",name,addPlayscoreExt,p,HOME_SCORE_DIR);
	    if (fd != -1) 
	      return fd;
	    fd = tryIt(filename,".score",name,addScoreExt,p,HOME_SCORE_DIR);
	    if (fd != -1) 
	      return fd;
	}
	
	fd = tryIt(filename,".playscore",name,addPlayscoreExt,LOCAL_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
	
	fd = tryIt(filename,".score",name,addScoreExt,LOCAL_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;

	fd = tryIt(filename,".playscore",name,addPlayscoreExt,SYSTEM_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
	fd = tryIt(filename,".score",name,addScoreExt,SYSTEM_SCORE_DIR,NULL);
	if (fd != -1) 
	  return fd;
    }
    if (fd == -1) {
	fprintf(stderr,"Can't find %s.\n",name);
	exit(1);
    }
    return fd;
}

const char * const help = 
"\n\n\
   The playscore program performs a scorefile by playing it on the DSP\n\
   and/or MIDI.\n\
   It's invoked as\n\n\
        playscore [options] <scorefile>\n\n\
   If the specified file isn't an absolute path, the program searches for\n\
   it in the following directories, in order:\n\n\
   The current working directory\n\
   ~/Library/Music/Scores\n\
   /LocalLibrary/Music/Scores\n\
   /NextLibrary/Music/Scores.\n\n\
   The '.playscore' or '.score' extension, which identify a file as a \n\
   scorefile, can be omitted -- the program will append it for you. If you\n\
   leave off the extension and both a '.playscore' and a '.score' version are\n\
   present, the '.playscore' version is choosen.\n\n\
   The following options are recognized: \n\
       -q               Quiet mode. All print-out is surpressed.\n\
       -d               Debug mode. This leaves room for the Ariel Bug56\n\
                        debugger on the DSP.\n\
       -f               Fast mode -- uses a ScorefilePerformer.\n\
                        Usually, playscore reads the entire scorefile before\n\
                        playing it. By using a ScorefilePerformer, the\n\
                        program can read the file on the fly. The\n\
                        performance begins sooner but puts a heavier demand\n\
                        on the main CPU than does the default.\n\
       -r  <count>      Repeat performance the specified number of times.\n\
       -w  <soundfile>  Write the DSP sample output as the named soundfile.\n\
                        The '.snd' extension is automatically appended to\n\
                        the file name.\n\
       -c  <soundfile>  Write the DSP commands output as the named soundfile.\n\
                        The '.snd' extension is automatically appended to\n\
                        the file name.\n\
    For futher details, see the playscore MAN page.\n\
       \n";


double setUpFromScoreInfo(id scoreInfo,double *midiOffset)
    /* returns srate */
{
    double samplingRate = 22050;
    int midiOffsetPar;
    *midiOffset = 0;
    if (scoreInfo) {  
	if ([scoreInfo isParPresent:MK_headroom])
	  [Orchestra setHeadroom:[scoreInfo parAsDouble:MK_headroom]];
	if ([scoreInfo isParPresent:MK_samplingRate]) {
	    samplingRate = [scoreInfo parAsDouble:MK_samplingRate];
	    if ((samplingRate == 44100.0) || (samplingRate == 22050.0)) 
	      [Orchestra setSamplingRate:samplingRate];
	    else if (!quiet)
	      fprintf(stderr,
		      "Sampling rate must be 44100 or 22050.\n");
	}
	if ([scoreInfo isParPresent:MK_tempo]) {
	    double tempo = [scoreInfo parAsDouble:MK_tempo];
	    [[Conductor defaultConductor] setTempo:tempo];
	}
	midiOffsetPar = [Note parName:"midiOffset"];
	if ([scoreInfo isParPresent:midiOffsetPar])
	    *midiOffset = [scoreInfo parAsDouble:midiOffsetPar];
    }
    else {
	[Orchestra setSamplingRate:22050.0];
//	[Orchestra setHeadroom:-.5];
    }
#if SOUND_OUT_PAUSE_BUG
    if (samplingRate == 22050)
    	*midiOffset +=  .36363636363636/8.0;
    else *midiOffset += .181818181818181/8.0;
#else
    if (samplingRate == 22050)
    	*midiOffset +=  .36363636363636;
    else *midiOffset += .181818181818181;
#endif
/* Note: there is a .1 second indeterminacy (in the 22khz case) due to not knowing where
   we are in soundout buffering. Using more, but smaller buffers would solve this. */
    return samplingRate;
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

static char *makeSoundfileStr(char *name)
{
    int addExt = 0;
    char *newStr;
    if (!extensionPresent(name,".snd"))  /* Ext not there */
      addExt = 1;
    newStr = malloc(strlen(name)+((addExt) ? 5 : 1));
    strcpy(newStr,name);
    if (addExt)
      strcat(newStr,".snd");
    return newStr;
}

static BOOL isMidiClassName(char *className)
{
    return (className && ((strcmp(className,"midi") == 0)  ||
			  (strcmp(className,"midi1") == 0) ||
			  (strcmp(className,"midi0") == 0)));
}

#if SOUND_OUT_PAUSE_BUG

static BOOL checkForMidi(id obj)
{
    BOOL weGotScore;
    id subobjs;
    int i,cnt;
    id info;
    char *sp;
    if (weGotScore = [obj isKindOf:[Score class]])
      subobjs = [obj parts];
    else subobjs = [obj noteSenders];
    if (!subobjs)
      return NO;
    cnt = [subobjs count];
    for (i=0; i<cnt; i++) {
	if (weGotScore)
	  info = [[subobjs objectAt:i] info];
	else info = [obj infoForNoteSender:[subobjs objectAt:i]];
	if ([info isParPresent:MK_synthPatch] &&
	    (isMidiClassName([info parAsStringNoCopy:MK_synthPatch]))) {
	    [subobjs free];
	    return YES;
	}
    }
    [subobjs free];
    return NO;
}

#endif

static void nullErrorHandler(char *msg)
{
}

main(ac, av)
    int ac;
    char * av[];
{
    int i,repeatCount,repeat;
    char *inputFile;
    id *instruments;
    NXStream *aStream;
    double midiOffset;
    BOOL onTheFly = NO;
    BOOL weGotMidi = NO;
    BOOL debugIt = NO;
    char *outputFile = NULL;
    char *commandsFile = NULL;
    id aPerformer,anOrch,midis[2];
    double samplingRate;
    int fd;
    if (ac == 1) {
	fprintf(stderr,help);
	exit(1);
    }
    [Conductor setThreadPriority:1.0];
    setuid(getuid());   
    repeatCount = 1;
    for (i=1; i<(ac-1); i++) {
      if ((strcmp(av[i],"-f") == 0)) 
	onTheFly = YES;
      else if ((strcmp(av[i],"-d") == 0)) 
	debugIt = YES;
      else if ((strcmp(av[i],"-r") == 0)) {
	  ++i;
	  if (i + 1 == ac) {
	    if (!quiet) 
	      fprintf(stderr,"-r requires a repeat count.\n");
	    exit(1);
	  }
	  repeatCount = atoi(av[i]);
        }
	else if (strcmp(av[i],"-w") == 0) {
	    ++i;
	    if (i + 1 == ac) {
		if (!quiet)
		  fprintf(stderr,"-w requires a file to be specified.\n"); 
		exit(1);
	    }
	    outputFile = makeSoundfileStr(av[i]);
	}
	else if (strcmp(av[i],"-c") == 0) {
	    ++i;
	    if (i + 1 == ac) {
		if (!quiet)
		  fprintf(stderr,"-c requires a file to be specified.\n"); 
		exit(1);
	    }
	    commandsFile = makeSoundfileStr(av[i]);
	}
	else if ((strcmp(av[i],"-q") == 0)) {
	    MKSetErrorProc(nullErrorHandler);
	    quiet = YES;
	}
    }
    for (repeat = 0; repeat < repeatCount; repeat++) {
      if (repeat == 0) {
	inputFile = makeStr(av[ac-1]);
      }
      if (!onTheFly) {
	if (repeat == 0) {
	  id aScore = [Score new];                  
	  fd = findFile(inputFile);
	  aStream = NXOpenFile(fd,NX_READONLY);
	  if (!quiet)
	    if (aStream)
	      fprintf(stderr,"playscore reading %s...\n",inputFile);
	  if (![aScore readScorefileStream:aStream]) { 
	    if (!quiet)
	      fprintf(stderr,"Fix scorefile errors and try again.\n");
	    exit(1);
	  }
#if SOUND_OUT_PAUSE_BUG
	  weGotMidi = checkForMidi(aScore);
#endif
	  aPerformer = [ScorePerformer new];
	  [aPerformer setScore:aScore];
	  samplingRate = setUpFromScoreInfo([aScore info],&midiOffset);
	  if (!quiet)
	    fprintf(stderr,"...done\n");
	}
	[aPerformer activate]; 
      }
      else { /* on the fly */
	fd = findFile(inputFile);
	aStream = NXOpenFile(fd,NX_READONLY);
	if (repeat == 0) {
	  aPerformer = [ScorefilePerformer new];                  
	}
	else [aPerformer freeNoteSenders];
	[aPerformer setStream:aStream];
	[aPerformer activate]; 
#if SOUND_OUT_PAUSE_BUG
	weGotMidi = checkForMidi(aPerformer);
#endif
	setUpFromScoreInfo([aPerformer info],&midiOffset);
      }
      if (repeat == 0) {
	anOrch = [Orchestra new];        
	if (debugIt)
	  [anOrch setOnChipMemoryConfigDebug:YES patchPoints:0];
	if (outputFile)
	  [anOrch setOutputSoundfile:outputFile];
	if (commandsFile)
	  [anOrch setOutputCommandsFile:commandsFile];
#if SOUND_OUT_PAUSE_BUG
	if (weGotMidi)
	  [anOrch setFastResponse:YES];
#endif
	midis[0] = nil;
	midis[1] = nil;
      }
      if (!onTheFly) {  
	int partCount,synthPatchCount,voices,midiChan,whichMidi;
	char *className;
	id partPerformers,synthPatchClass,partPerformer,partInfo,aPart,anIns;
	partPerformers = [aPerformer partPerformers];
	partCount = [partPerformers count];
	if (repeat == 0) {
	  instruments = (id *)malloc(sizeof(id) * partCount);
	}
	for (i = 0; i < partCount; i++) {
	  partPerformer = [partPerformers objectAt:i];
	  aPart = [partPerformer part]; 
	  partInfo = [aPart info];      
	  if (!partInfo) {               
	    continue;
	  }		
	  if (![partInfo isParPresent:MK_synthPatch]) {
	    continue;
	  }
	  className = [partInfo parAsStringNoCopy:MK_synthPatch];
	  if (isMidiClassName(className)) {
	    if (repeat == 0) {
	      midiChan = [partInfo parAsInt:MK_midiChan];
	      if ((midiChan == MAXINT) || (midiChan > 16))
		midiChan = 1;
	      if (strcmp(className,"midi") == 0)
		className = "midi1";
	      if (strcmp(className,"midi1") == 0) 
		whichMidi = 1;
	      else whichMidi = 0;
	      if (midis[whichMidi] == nil)
		midis[whichMidi] = [Midi newOnDevice:className];
	      [[partPerformer noteSender] connect:
	       [midis[whichMidi] channelNoteReceiver:midiChan]];
	    }
	  }
	  else { /* It's a DSP part */
	    synthPatchClass = [partInfo findClass:className];
	    if (!synthPatchClass) {          
	      if (!quiet)
		fprintf(stderr,"SynthPatch %s not loaded into program.\n",
			className);
	      continue;
	    }
	    if ([anOrch deviceStatus] != MK_devOpen) {
	      if (![anOrch open]) {
		if (!quiet)
		  fprintf(stderr,"Can't open DSP.\n");
		exit(1);
	      }
	    }
	    if (repeat == 0) {
	      anIns = instruments[i] = [SynthInstrument new];      
	      [[partPerformer noteSender] connect:[anIns noteReceiver]];
	    }
	    [(anIns = instruments[i]) setSynthPatchClass:synthPatchClass];
	    if (![partInfo isParPresent:MK_synthPatchCount])
	      continue;         
	    voices = [partInfo parAsInt:MK_synthPatchCount];
	    synthPatchCount = 
	      [anIns setSynthPatchCount:voices patchTemplate:
	       [synthPatchClass patchTemplateFor:partInfo]];
	    if (repeat == 0 && synthPatchCount < voices) 
	      if (!quiet)
		fprintf(stderr,
			"Could only allocate %d instead of %d %ss for %s\n",
			synthPatchCount,voices, 
			[synthPatchClass name],MKGetObjectName(aPart));
	  }
	}
      }
      else {  /* on the fly */
	int partCount,synthPatchCount,voices,midiChan,whichMidi;
	char *className;
	id noteSenders,synthPatchClass,partInfo,anIns,aNoteSender;
	noteSenders = [aPerformer noteSenders];
	partCount = [noteSenders count];
	for (i = 0; i < partCount; i++) {
	  aNoteSender = [noteSenders objectAt:i];
	  partInfo = [aPerformer infoForNoteSender:aNoteSender];
	  if (!partInfo) {               
	    continue;
	  }		
	  if (![partInfo isParPresent:MK_synthPatch]) 
	    continue;         
	  className = [partInfo parAsStringNoCopy:MK_synthPatch];
	  if (isMidiClassName(className)) {
	    if (repeat == 0) {
	      midiChan = [partInfo parAsInt:MK_midiChan];
	      if ((midiChan == MAXINT) || (midiChan > 16))
		midiChan = 1;
	      if (strcmp(className,"midi") == 0)
		className = "midi1";
	      if (strcmp(className,"midi1") == 0) 
		whichMidi = 1;
	      else whichMidi = 0;
	      if (midis[whichMidi] == nil)
		midis[whichMidi] = [Midi newOnDevice:className];
	      [aNoteSender connect:
	       [midis[whichMidi] channelNoteReceiver:midiChan]];
	    }
	  }
	  else { /* dsp */
	    synthPatchClass = [aNoteSender findClass:className];
	    if (!synthPatchClass) {          
	      if (!quiet)
		fprintf(stderr,"Class %s not loaded into program.\n",
			className);
	      continue;
	    }
	    if ([anOrch deviceStatus] != MK_devOpen)
	      if (![anOrch open]) {
		if (!quiet)
		  fprintf(stderr,"Can't open DSP.\n");
		exit(1);
	      }
	    if (repeat == 0) {
	      instruments = (id *)malloc(sizeof(id) * partCount);
	      instruments[i] = [SynthInstrument new];      
	    }
	    anIns = instruments[i];
	    [aNoteSender connect:[anIns noteReceiver]];
	    [anIns setSynthPatchClass:synthPatchClass];
	    if (![partInfo isParPresent:MK_synthPatchCount])
	      continue;         
	    voices = [partInfo parAsInt:MK_synthPatchCount];
	    synthPatchCount = 
	      [anIns setSynthPatchCount:voices patchTemplate:
	       [synthPatchClass patchTemplateFor:partInfo]];
	    if (repeat == 0 && synthPatchCount < voices) 
	      if (!quiet)
		fprintf(stderr,
			"Could only allocate %d instead of %d %ss for %s\n",
			synthPatchCount,voices,[synthPatchClass name],
			MKGetObjectName(aNoteSender));
	  }
	}
	[noteSenders free];
      }
      MKSetDeltaT(1.0);              
      [Conductor setClocked:NO];     
      for (i=0; i<2; i++) 
	[midis[i] openOutputOnly];
#if 1
      for (i=0; i<2; i++) 
        if (midiOffset > 0) 
		[midis[i] setLocalDeltaT:midiOffset];
	else if (midiOffset < 0)
		[anOrch setLocalDeltaT:-midiOffset];
#endif
      if (!quiet) {
	  if (commandsFile)
	    fprintf(stderr,"writing to %s...\n",commandsFile);
	  if (outputFile)
	    fprintf(stderr,"writing to %s...\n",outputFile);
	  else fprintf(stderr,"playing...\n");
      }
      for (i=0; i<2; i++)
	[midis[i] run];
      if ([anOrch deviceStatus] == MK_devOpen)
	[anOrch run];                  
      [Conductor startPerformance];  
      if ([anOrch deviceStatus] == MK_devRunning)
	[anOrch close];                
      for (i=0; i<2; i++)
	[midis[i] close];
      if (!quiet)
	fprintf(stderr,"...done\n");
      if (onTheFly || (repeat == (repeatCount - 1))) {
	NXClose(aStream);
	close(fd);
      }
    }
    exit(0);
}

