/* This is dsp-18's DSPError.c as of 8/23/90 */

#ifdef SHLIB
#include "shlib.h"
#endif

#ifdef SHLIB
#include <sound/sounddriver.h>	/* This will fully replace the following */
#else
#include "snddriver.h"		/*** FIXME ***/
#endif

#include "dsp/_dsp.h"
#include <stdarg.h>

/* 
 * Error handler for libdsp:
 *
 *   _DSPError(errorcode, "message");
 *
 * errorcode is assumed to be a globally unique error code defined in
 *		dsperrno.h or <sys/errno.h>.  
 *
 * If errocode is equal to -1 (DSP_EUNIX), perror() is called to print
 * the error code defined in <sys/errno.h>.  If errorcode is 0,
 * the error message is printed and not associated with a numeric code.
 *
 */

/*
 *   _DSPErrorV - Error reporting routine with variable arguments
 *
 *   Usage:
 *
 *	  _DSPErrorV(error_code, printf_style_format, arg1, arg2...);
 *
 */


/* error log */
static int s_error_log_disabled = 1;	 /* Stifle error print-out if nonzero */
static FILE *s_err_fp = 0;
static char *s_err_fn = DSP_ERRORS_FILE;

#include <pwd.h>		/* for getpwuid */
static struct passwd *pw;

#include <time.h>
/* #include <sys/time.h> */
/* extern char *ctime(); */
/* extern long time(); */
/* extern int getpid(); */

/* unprototyped procedures which trigger -Wimplicit warnings */
extern int umask();
extern int getpid();

/******************************* error string ***************************/

#import <sound/sounderror.h>	/* for SNDSoundError() */

#ifndef SND_NO_ERROR
#define SND_NO_ERROR	100	// non-error ack.
#define SND_BAD_PORT	101	// message sent to wrong port
#define SND_BAD_MSG	102	// unknown message id
#define SND_BAD_PARM	103	// bad parameter list in message
#define SND_NO_MEMORY	104	// can't allocate memory (record)
#define SND_PORT_BUSY	105	// access req'd to existing excl access port
#define SND_NOT_OWNER	106	// must be owner to do this
#define SND_BAD_CHAN	107	// dsp channel hasn't been inited
#define SND_SEARCH	108	// couldn't find requested resource
#define SND_NODATA	109	// can't send data commands to dsp in this mode
#define SND_NOPAGER	110	// can't allocate from external pager (record).
#define SND_NOTALIGNED	111	// bad data alignment.
#define SND_BAD_HOST_PRIV 112	// bad host privilege port passed.
#endif

#ifndef SND_BAD_PROTO
#define SND_BAD_PROTO 	113	// can't do requested operation in cur protocol
#endif

#define SND_MAX_ERROR 	113

static char *snddriver_error_list[] = {
	"sound success",
	"sound message sent to wrong port",
	"unknown sound message id",
	"bad parameter list in sound message",
	"can't allocate memory for recording",
	"sound service in use",
	"sound service requires ownership",
	"DSP channel not initialized",
	"can't find requested sound resource",
	"bad DSP mode for sending data commands",
	"external pager support not implemented",
	"sound data not properly aligned",
	"bad host provilege port passed",
	"can't do requested operation in extant DSP protocol"
};

/*
 * Error code to string conversion (new in ???) */
extern char *mach_error_string();
static char *my_snddriver_error_string(int error)
{
    if (error <= 0)
      return mach_error_string(error);
    else if (error >= SND_NO_ERROR && error <= SND_MAX_ERROR)
      return snddriver_error_list[error-SND_NO_ERROR];
    else
      return SNDSoundError(error);
}

/***************************** Error Enabling ********************************/

/* Preferred API */

int DSPEnableErrorLog(void)
{
    s_error_log_disabled = 0;
    return 0;
}


int DSPDisableErrorLog(void)
{
    s_error_log_disabled = 1;
    return 0;
}


int DSPErrorLogIsEnabled(void)
{
    return(!s_error_log_disabled);
}


/* Old API */

int DSPDisableErrorLogging(int *olds_error_log_disabledP)
{
    if (olds_error_log_disabledP)
      *olds_error_log_disabledP = s_error_log_disabled; /* DSPGlobals.c */
    s_error_log_disabled = 1;
    return(0);
}

int DSPEnableErrorLogging(int *olds_error_log_disabledP)
{
    if (olds_error_log_disabledP)
      *olds_error_log_disabledP = s_error_log_disabled; /* DSPGlobals.c */
    s_error_log_disabled = 0;
    return(0);
}

int DSPRestoreErrorLogging(int olds_error_log_disabled)
{
    s_error_log_disabled = olds_error_log_disabled;
    return(0);
}

/******************************* ERROR FILE CONTROL **************************/

int DSPSetErrorFP(FILE *fp)
{
    s_err_fp = fp;
    s_err_fn = 0;
    return(0);
}

FILE *DSPGetErrorFP(void)
{
    return s_err_fp;
}

int DSPSetErrorFile(
    char *fn)
{
    s_err_fn = fn;
    s_err_fp = 0;
    return(0);
}


char *DSPGetErrorFile(void)
{
    return s_err_fn;
}


int DSPEnableErrorFile(
    char *fn)
{
    DSP_UNTIL_ERROR(DSPEnableErrorLog());
    return DSPSetErrorFile(fn);
}


int DSPEnableErrorFP(
    FILE *fp)
{
    DSP_UNTIL_ERROR(DSPEnableErrorLog());
    return DSPSetErrorFP(fp);
}


int _DSPCheckErrorFP(void)
{
    char *lname;
    time_t tloc;

    if (s_error_log_disabled)
      return 0;

    if (s_err_fp == 0) { /* open error log */
	umask(0); /* Don't CLEAR any filemode bits (arg is backwards!) */
	if ((s_err_fp=fopen(s_err_fn,"w"))==NULL) {
	    fprintf(stderr,
		    "*** _DSPCheckErrorFP: Could not open DSP error file %s\n"
		    "Setting error output to stderr.\n",
		    s_err_fn);
	    s_err_fp = stderr;
	    return(-1);
	} else
	  if (_DSPVerbose)
	    fprintf(stderr,"Opened DSP error log file %s\n",s_err_fn);

	tloc = time(0);
	    
#ifdef GETPWUID_BUG_FIXED
	lname = getlogin();
	if (!lname) {
	    pw = getpwuid(getuid());
	    if (pw)
	      lname = pw->pw_name;
	    else
	      lname = "<user not in /etc/passwd>";
	}
	fprintf(s_err_fp,"DSP error log for PID %d started by %s on %s\n",
		getpid(),lname,ctime(&tloc));
#else	   
	fprintf(s_err_fp,"DSP error log for PID %d started on %s\n",
		getpid(),ctime(&tloc));
#endif

	fflush(s_err_fp);
	return(0);
    }
}      
	

int DSPCloseErrorFP(void)
{
    if (_DSPVerbose && !s_error_log_disabled) {
	_DSPCheckErrorFP();
	fprintf(s_err_fp,"End of DSP errors.\n");
	if (s_err_fp != stderr && s_err_fp != stdout)
	  fprintf(stderr,"Closed DSP error log file %s\n",
		  (s_err_fn? s_err_fn : ""));
    }
    if (s_err_fp)
      fclose(s_err_fp);
    return(0);
}

/******************************* Error File Usage ****************************/

int _DSPErrorV(int errorcode,char *fmt,...)
{
    va_list args;

    extern int errno;
    extern int sys_nerr;
    extern char *sys_errlist[];

    if (s_error_log_disabled)
      return 0;

    va_start(args,fmt);

    _DSPCheckErrorFP();

    fflush(stdout);
    if ( errorcode != -1 ) 
      DSPErrorNo = errorcode;	/* DSPGlobals.c */
    if ( errorcode < DSP_ERRORS && errorcode != -1 ) 
      errno = errorcode;

    if (!s_error_log_disabled) {
	fprintf(s_err_fp,";;*** libdsp:");    
	/* man says: vfprintf(fmt, args); */
	/* I suppose: vfprintf(s_err_fp,fmt, args); */
	fprintf(s_err_fp,
	    "\n_DSPErrorV called --- vprintf is missing in this UNIX\n");
	fprintf(s_err_fp,"%s",fmt);
	fprintf(s_err_fp," ***\n");
	if ( errorcode == -1 ) 
	  fprintf(s_err_fp,
		  "\t\tLast UNIX error: %s\n",
		  (errorcode<sys_nerr? 
		   sys_errlist[errorcode] 
		   : "<invalid error code>"));
//	else {
//	    fprintf(s_err_fp,";;(error code 0x%X)\n",errorcode);
//	}
	fprintf(s_err_fp,"\n");
    }
    /*	   if (DSPErrorHalt) kill(getpid(),SIGINT); (or) abort() */
    va_end(args);
    return errorcode;
}	

/* *** When _DSPErrorV works, use it below *** */

int _DSPError1(
    int errorcode,
    char *msg,
    char *arg)
{   
    extern int errno;
    extern int sys_nerr;
    extern char *sys_errlist[];

#if 0
    /* This unfortunately introduces a cycle in libdsp.
     * It should only be used when needed badly.
     */
    if (DSPIsSimulated()) {
	fprintf(DSPGetSimulatorFP(),";;*** libdsp:");
	fprintf(DSPGetSimulatorFP(),msg,arg);
	fprintf(DSPGetSimulatorFP()," ***\n");
	fflush(DSPGetSimulatorFP());
    }
#endif

    if (s_error_log_disabled)
      return errorcode;

    _DSPCheckErrorFP();

    fflush(stdout);
    if ( errorcode != -1 ) 
      DSPErrorNo = errorcode;	/* DSPGlobals.c */
    if ( errorcode < DSP_ERRORS && errorcode != -1 ) 
      errno = errorcode;

    fprintf(s_err_fp,";;*** libdsp:");
    fprintf(s_err_fp,msg,arg);
    fprintf(s_err_fp," ***\n");
    if ( errorcode == -1 ) {
	perror(";;\t\tLast UNIX error:");
	fprintf(s_err_fp,"\n");
    }

    fflush(s_err_fp); /* unnecessary? */
    return errorcode;
} 


int _DSPError(
    int errorcode,
    char *msg)
{   
    return(_DSPError1(errorcode,DSPCat(msg,"%s"),""));
} 

int DSPUserError(
    int errorcode,
    char *msg)
{   
    return _DSPError(DSP_EUSER,DSPCat(DSPCat(msg,"\n;; DSP User Error "),
				       _DSPCVS(errorcode)));
}

int DSPUserError1(
    int errorcode,
    char *msg,
    char *str)
{   
    _DSPError1(DSP_EUSER,DSPCat(DSPCat(msg,"\n;; DSP User Error "),
				       _DSPCVS(errorcode)),str);
    return errorcode;
}

void _DSPFatalError(
    int errorcode,
    char *msg)
{   
    _DSPCheckErrorFP();
    if (s_err_fp)
      fprintf(s_err_fp,
	    "\n\t*** Fatal error detected by DSP library routine ***\n\n"
	    "\t%s\n\n\t*** Aborting ***\n",msg);
    if (s_err_fp != stderr)
      fprintf(stderr,
	      "\n\t*** Fatal error detected by DSP library routine ***\n\n"
	      "\t%s\n\n\t*** Aborting ***\n",msg);
    /* Cannot do because of cycle introduced into libdsp */
    /* DSPClose(); */
    exit(1);
} 


int _DSPMachError(
    int error,
    char *msg)
{   
    char *s = my_snddriver_error_string(error); /* FIXME: Delete "my_" */
    return _DSPError1(DSP_EMACH,
		      DSPCat(msg,"\n;; Mach/snddriver/sound error: %s"),s);
} 


/*########################### err.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* ERR */

void _DSPErr(msg)
char *msg;
{   
    fprintf(stderr,"\n\n\t\t\toops!\n\n");
    fprintf(stderr,msg);
    fprintf(stderr,"\n*** Aborting\n^C\n");
    exit(1);
} 


/*########################### DSPMessageExpand #############################*/

#include "dsp/_dsp_message_names.h" /* DSP message and error mnemonic arrays */

char *DSPMessageExpand(int msg)
/*
 * Convert 24-bit DSP message into a string containing mnemonic message 
 * opcode and argument datum in hex. Example: The DSP message 0x800040
 * expands into "BREAK(0x0040)" which means a breakpoint in the DSP at 
 * location 0x40.
 */
{
    int op,opndx,dat;
    char *opname;

    op = DSP_MESSAGE_OPCODE(msg);
    dat = DSP_MESSAGE_ADDRESS(msg);

    if (DSP_IS_ERROR_OPCODE(op)) {
	opndx = DSP_ERROR_OPCODE_INDEX(op);
	if (opndx < DSPNErrorNames)
	  opname = DSPErrorNames[DSP_ERROR_OPCODE_INDEX(op)];
	else
	  opname = DSPCat("?0x",_DSPCVHS(op)); 
    } else
      opname = (op < DSPNMessageNames ?
		DSPMessageNames[op] :
		DSPCat("?0x",_DSPCVHS(op)));	

    return(DSPCat(opname,DSPCat("(0x",DSPCat(_DSPCVHS(dat),")"))));

}
