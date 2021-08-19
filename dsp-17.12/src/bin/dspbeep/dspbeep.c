/* dspbeep.c - Get a sine beep from the DSP at 172Hz.

Modification history
	08/12/88/jos - file created from dsplo.c
	08/30/88/jos - upgrade to new DSP DMA protocol
	04/17/90/jos - removed override_wd_reader option and code
	04/17/90/jos - removed "-countBufs n" option
*/

#if 0
int snddriver_set_dsp_buffers_per_soundout_buffer(
	int		dev_port,		// valid device port
	int		owner_port,		// valid owner port
	int		dbpsob			// so buf size / dsp buf size
)
{
	return 0;
}

int snddriver_dsp_reset(
	int		cmd_port,		// valid command port
	int		priority)		// priority of this transaction
{
	return 0;
}
#endif

#if 1
#define USAGE "dspbeep [-verbose_toggle] [-sim] [-trace n] [-test] [-nbufs n] [-lowsrate] [-f file] [-bufsmall] [-hang] [-quiet] [-ramp] [-wait ms] [-absorb_wd_thread] [-ssi]"
#else
#define USAGE "dspbeep [-verbose_toggle] [-trace n] [-nbufs n] [-lowsrate] [-hang]"
#endif
#include <dsp/_dsp.h>

/* 
extern FILE *_DSPMyFopen();
#define DSP_TRACE_HOST_INTERFACE 1024
Also, defaultTimeOut
and something else re. time stamps
*/

#include <sound/sound.h>	/* For write-data output file */
#include <sys/file.h>
#include <stdio.h>

/* #include "beepsubs.c" */
#include <sys/resource.h>
#include <signal.h>
#include <dsp/_DSPMachSupport.h>

extern int sleep();
extern int read_sound_out_mapped(); /* defined at end of this file */

FILE *ofp=0;

void error_exit(msg) 
char *msg;
{
    fprintf(stderr,"*** %s\n",msg);
    DSPMKStopSoundOut();
    DSPRawClose();
    if (ofp)
      fclose(ofp);
    exit(1);
}

static int stop = 0;		/* set by ^C interrupt */

void cchandler(anint)
    int anint;
{
    stop += 1;
    if (stop>1) 
      error_exit("Aborted");
}

static char* s_wd_fn=0;
static FILE* s_wd_fp=0;	  
static SNDSoundStruct *s_wd_header=0;
static int s_low_srate = 0;
static port_t s_wd_stream_port = 0;
static port_t s_wd_reply_port = 0;
static int s_wd_timeout=1000;
static int s_open = 0;
static int s_wd_sample_count=0;
static int my_wd_reader();
static int s_sound_out=1;
static int s_write_data_running=0;
static int s_stop_write_data=0;
static int s_dsp_record_buf_size = 4*DSPMK_NB_DMA_W; /* in bytes */
static port_t s_so_stream_port = 0;	/* Sound-out stream (host to DAC) */

static int ramp_state = 0;

void main(argc,argv) 
    int argc; char *argv[]; 
{
    int iarg,i,j,nbad,count,dspack,ec,started=0;
    int nsinebuffers = 100;
    char *simfn;
    long timeout_us;
    int do_sim = DSP_FALSE;	/* if TRUE, write out host-interface file */
    int mapped = DSP_FALSE;	/* TRUE for memory mapped DSP interface */
    int ssi = DSP_FALSE;	/* TRUE to turn on SSI SoundOut in DSP */
    int dotest = DSP_FALSE;	/* TRUE to make test measurements */
    int hang = DSP_FALSE;	/* TRUE to hang at end instead of closing DSP*/
    int no_wd_thread=DSP_FALSE;	/* TRUE to call write data reader in line */
    struct rlimit rlim;
    long time0,time1;
    double rtime;
    DSPFix48 dspTime;

    /* Set handler for control-C */
    /*     signal(SIGINT,cchandler); */

    while (--argc && **(++argv)=='-') {
	_DSPToLowerStr(++(argv[0])); /* lower casify and skip over '-' */
	switch (*argv[0]) {
	case '?':
	    fprintf(stderr,"\nUsage:\t%s\n\n",USAGE);
	    exit(0);
	    break;
	case 'w':			       /* -wait ms */
	    s_wd_timeout = (--argc)? strtol(*(++argv),NULL,0) : s_wd_timeout;
	    DSPMKSetWriteDataTimeOut(s_wd_timeout);
	    break;
	case 'r':			       /* -ramp */
	    ramp_state = 1;
	    break;
	case 'q':			       /* -quiet */
	    s_sound_out = 0;
	    break;
	case 'a':			       /* -absorb_wd_thread */
	    no_wd_thread = DSP_TRUE;
	    break;
	case 'h':			       /* -hang */
	    hang = DSP_TRUE;
	    break;
	case 'v':			       /* -verbose */
	    _DSPVerbose = ! _DSPVerbose;
	    break;
	case 'l':			       /* -lowsrate */
	    s_low_srate = DSP_TRUE;
	    break;
	case 'b':			       /* -bufsmall */
	    DSPMKEnableSmallBuffers();
	    break;
	case 'f':			       /* -file */
	    s_wd_fn = *(++argv);
	    argc--;
	    break;
	case 'm':			       /* -mappedio */
	    mapped = DSP_TRUE;
	    break;
	case 'u':			       /* -unmappedio */
	    mapped = DSP_FALSE;
	    break;
	case 't':			
	    if (argv[0][1]=='r') {	       /* -trace n */
		_DSPTrace = (--argc)? strtol(*(++argv),NULL,0) : -1;
		fprintf(stderr,"_DSPTrace set to 0x%X\n",_DSPTrace);
		break;
	    } else if (argv[0][1]=='e')			/* -test */
	      dotest=DSP_TRUE;
	    else
	      fprintf(stderr,"Unknown switch -%s\n",argv[0]);
	    break;
	case 's':
	    if (argv[0][1]=='s')		/* -ssi */
	      ssi = DSP_TRUE;
	    else if (argv[0][2]=='m' || argv[0][1]=='\0') {
		/* -simulator or -simfile */
		do_sim=DSP_TRUE;
		if (argv[0][3]=='f')  {		/* -simfile */
		    simfn = *++argv;		/* simulator input file */
		    argc--;
		}
	    }
	    else 
	      fprintf(stderr,"Unknown switch -%s\n",argv[0]);
	    break;
	case 'n':				/* no{m,s,h} */
	    if (argv[0][1]=='b' ||		/* -nbuffers */
		argv[0][1]=='\0') {		/* -n */
		 nsinebuffers = strtol(*(++argv),NULL,0);
		 argc--;
	    }
	    else if (argv[0][2]=='m') {
		mapped = DSP_FALSE;
	    }
	    else if (argv[0][2]=='s')
	      ssi = DSP_FALSE;
	    else if (argv[0][2]=='h')
	      hang = DSP_FALSE;
	    else 
	      fprintf(stderr,"Unknown switch -%s\n",argv[0]);
	    break;
	default:
	    fprintf(stderr,"Unknown switch -%s\n",argv[0]);
	    error_exit("unknown switch");
	}
    }
	    
    DSPEnableErrorLog();
    if (_DSPVerbose)
      DSPSetErrorFile("/dev/tty");

    if (mapped)
      _DSPEnableMappedOnly();
    else
      _DSPDisableMappedOnly();
    
    if (s_sound_out)
      DSPMKEnableSoundOut();
    else
      DSPMKDisableSoundOut();

    if (ssi) 
      DSPMKEnableSSISoundOut();
    else
      DSPMKDisableSSISoundOut(); /* default */
    
    if (s_low_srate)
      DSPMKSetSamplingRate(DSPMK_LOW_SAMPLING_RATE);
    else
      DSPMKSetSamplingRate(DSPMK_HIGH_SAMPLING_RATE);

    if (s_wd_fn) {
	DSPMKEnableWriteData();
	DSPMKSetWriteDataFile(s_wd_fn);
    } else
      DSPMKDisableWriteData();

    ec = DSPMKInit();
    if (ec)
      _DSPErr("dspbeep: could not init dsp");
    
    if (!simfn) 
      simfn = "dspbeep.io";
    if (do_sim)
      if(ec=DSPOpenSimulatorFile(simfn))
	_DSPError1(ec,
		   "dspbeep: could not open simulator output file: %s",
		   simfn);
    

    if (ssi) 
      DSPMKStartSSISoundOut();

    _DSPSineTest(nsinebuffers);
    
    if (s_wd_fn) {
	if(no_wd_thread)
	  _DSPMKStartWriteDataNoThread();
	else
	  DSPMKStartWriteData();
    }

#   define BUFSIZE (DSPMK_NB_DMA_W>>1)

    if (mapped && s_sound_out) 
      read_sound_out_mapped(nsinebuffers);

    if (do_sim)
      DSPCloseSimulatorFile();

#define REPCOUNT 1000

    if (dotest) {
	fprintf(stderr,"Doing %d maximally fast 'Are you alive' transactions\n",
	       REPCOUNT);
	for (i=0,time0=time(0); i<REPCOUNT; i++)
	  if (!DSPMKIsAlive()) {
	      fprintf(stderr,"DSP seems to have crashed\n");
	      break;
	  }
	time1 = time(0);
	rtime = ((double)(time1 - time0))/((double)REPCOUNT);

	if(i==REPCOUNT)
	  fprintf(stderr,"DSP passed the %d requests test.\n"
		 "Number of milliseconds per round-trip transaction = %f7.3\n",
		 REPCOUNT,rtime*1000.0);

	for (i=0; i < 1+nsinebuffers*BUFSIZE; i+=44100, sleep(1))
	  if (DSPMKIsAlive())
	    fprintf(stderr,"DSP seems to be running ok\n");
	  else {
	      fprintf(stderr,"DSP seems to have crashed\n");
	      break;
	  }
    }

    if (hang) {
	fprintf(stderr,"\n\tType RETURN to stop:");
	getchar();
	fprintf(stderr,"\n");
    } else { 
	if (s_wd_fn) {
	    for(i=0; i<5; i++) {
		if (!DSPMKWriteDataIsRunning())	/* halted self. Timeout? */
		  break;
		if (DSPMKGetWriteDataSampleCount() >= nsinebuffers*BUFSIZE)
		  DSPMKStopWriteData();
		sleep(1);
	    }
	    DSPMKStopWriteData();
	    if (DSPMKGetWriteDataSampleCount() < nsinebuffers*BUFSIZE)
	      fprintf(stderr,"*** Only got %d instead %d "
		      "words of write-data\n",DSPMKGetWriteDataSampleCount(),
		      nsinebuffers*BUFSIZE);
	    if (DSPMKGetWriteDataSampleCount() > nsinebuffers*BUFSIZE)
	      fprintf(stderr,"Got %d instead %d "
		      "words of write-data\n",DSPMKGetWriteDataSampleCount(),
		      nsinebuffers*BUFSIZE);
	} else
	  sleep(1+(int)(0.5*(double)nsinebuffers*BUFSIZE
			/(s_low_srate?22050.0:44100.0)));
    }

 egress:
    s_open = 0;

    DSPClose();

    exit(ec?1:0);
}

int read_sound_out_mapped(nsinebuffers)
    int nsinebuffers;
{
    int i,j,k,dspack,parity=0,channel,startAddress,lastAddress,ec;
    int data[BUFSIZE];
    fprintf(stderr,"\tSound-out to host started in mapped mode!\n");
    fprintf(stderr,"\tDSP is spewing data which we can read polled.\n");
    fprintf(stderr,"\tPolling for write-data request.\n");
    
    /* Read buffers and quit */
    for (i=0;i<nsinebuffers;i++) { 
	if (DSPIsSimulated())
	  fprintf(DSPGetSimulatorFP(),";; Await DSP_DM_HOST_R_REQ\n");
	ec = DSPAwaitUnsignedReply(DSP_DM_HOST_R_REQ,&dspack,5000);
	channel = dspack & 0x2F;
	startAddress = parity? DSPMK_YB_DMA_W2 : DSPMK_YB_DMA_W;
	if (ec) 
	  _DSPErr("Failed to see DSP DMA request after 5 seconds");
	else
	  fprintf(stderr,"\t! Got DMA request for channel 0x%X, "
		  "parity %d, address 0x%X\n",
		  channel,parity,startAddress);	
	if (DSPIsSimulated())
	  fprintf(DSPGetSimulatorFP(),";; Set HF0 and read DSP sound-out\n");
	DSPSetHF0();
	ec = DSPReadRXArray(data,BUFSIZE);
	if (ec) 
	  _DSPErr("Failed to read DSP data");
	else
	  fprintf(stderr,"\t! DSP buffer read successfully\n");
	fprintf(stderr,"Buffer %d = ",i);
	for (j=0;j<(BUFSIZE>>1);j++) {
	    for (k=0;k<2;k++)
	      fprintf(stderr,"0x%X = %s ",
		      data[(j<<1)+k],
		      DSPFix24ToStr(data[(j<<1)+k]));
	    fprintf(stderr,"\n");
	}
	if (DSPIsSimulated())
	  fprintf(DSPGetSimulatorFP(),
		  ";; Exit DSP-to-Host DMA with an HM_HOST_R_DONE\n");
	if(DSPHostMessage(DSP_HM_HOST_R_DONE))
	  _DSPErr("Could not send HM_HOST_R_DONE");
	
	/* Read two extra words in host interface pipe (HTX & RX) and cnt */
	if (DSPIsSimulated())
	  fprintf(DSPGetSimulatorFP(),";; Read garbage word in HTX,RX + ack,,count\n");
	if(ec=DSPReadRX(&dspack))
	  _DSPErr("Could not read back garbage word after sending HOST_R_DONE");
	
	if (_DSPTrace & DSP_TRACE_HOST_INTERFACE)
	  fprintf(stderr,"After HOST_R_DONE, garbage word = 0x%X\n",dspack);
	if (DSPIsSimulated())
	  fprintf(DSPGetSimulatorFP(),"After HOST_R_DONE, garbage word = 0x%X\n",
		  dspack);
	
	ec = DSPAwaitUnsignedReply(DSP_DM_HOST_R_DONE,&dspack,
				   DSPDefaultTimeLimit);
	if(ec)
	  _DSPErr("Could not read back next-read-address "
		  "after sending HOST_R_DONE");
	
	lastAddress = parity? DSPMK_YB_DMA_W + 2 : DSPMK_YB_DMA_W2 + 2;
	
	if (dspack != lastAddress )
	{
	    fprintf(stderr,"? Got wrong last-address = 0x%X ?\n",dspack);
	    if (DSPIsSimulated())
	      fprintf(DSPGetSimulatorFP(),
		      "Last address = 0x%X is incorrect. "
		      "*** ""DMA"" READ FAILED ***\n",dspack);
	}
	if (DSPIsSimulated()) {
	    fprintf(DSPGetSimulatorFP(),"Last-read-address looks good = 0x%X\n",
		    dspack);
	}
	parity = 1-parity;
    }			/* for (i=0;i<nsinebuffers;i++) { */
    
}
