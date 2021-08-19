/* DSPError.h - Functions in libdsp_s.a having to do with error handling
 * Copyright 1989,1990, by NeXT, Inc. 
 * J.O. Smith
 */

/***************************** Error Enabling ********************************/

extern int DSPEnableErrorLog(void);
/* 
 * Turn on DSP error logging into the current error log file
 * set by DSPSetErrorFile().
 */


extern int DSPDisableErrorLog(void);
/* 
 * Turn off DSP error message logging.
 */


extern int DSPErrorLogIsEnabled(void);
/* 
 * Returns nonzero if DSP error logging is enabled.
 */

/******************************* ERROR FILE CONTROL **************************/

extern int DSPEnableErrorFile(char *fn);
/* 
 * Turn on DSP error message logging (equivalent to DSPEnableErrorLog())
 * and set the error file to fn (equivalent to DSPSetErrorFile(fn)).
 */


extern int DSPEnableErrorFP(FILE *fp);
/* 
 * Turn on DSP error message logging (equivalent to DSPEnableErrorLog())
 * and set the error file pointer to fp (equivalent to DSPSetErrorFP(fp)).
 */


extern int DSPSetErrorFile(char *fn);
/* 
 * Set the file-name for DSP error messages to fn.
 * The default file used if this is not called is
 * DSP_ERRORS_FILE defined in dsp.h.
 * This will clear any file-pointer specified using
 * DSPSetErrorFP().
 */


extern char *DSPGetErrorFile(void);
/* 
 * Get the file-name being used for DSP error messages, if known.
 * If unknown, such as when only a file-pointer was passed to
 * specify the output destination, 0 is returned.
 */


extern int DSPSetErrorFP(FILE *fp);
/* 
 * Set the file-pointer for DSP error messages to fp.
 * The file-pointer will clear and override any prior specification
 * of the error messages filename using DSPSetErrorFile().
 */


extern FILE *DSPGetErrorFP(void);
/* 
 * Get the file-pointer being used for DSP error messages.
 */


extern int DSPCloseErrorFP(void);
/*
 * Close DSP error log file.
 */


/******************************* Error File Usage ****************************/


extern int DSPUserError(
    int errorcode,
    char *msg);
/* 
 * Print message string 'msg' and integer error code 'errorcode' to the
 * DSP error log (if any), and return errorcode.  The message string 
 * normally identifies the calling function and the nature of the error.
 * The error code is arbitrary, but positive integers are generally in
 * use by other packages.
 */


extern int DSPUserError1(
    int errorcode,
    char *msg,
    char *str);
/* 
 * Print message string 'msg' and integer error code 'errorcode' to the
 * DSP error log (if any), and return errorcode.  The message string is
 * assumed to contain a single occurrence of '%s' which corresponds to the
 * desired placement of 'str' in the style of printf().
 */


extern char *DSPMessageExpand(int msg);
/*
 * Convert 24-bit DSP message into a string containing mnemonic message 
 * opcode and argument datum in hex. Example: The DSP message 0x800040
 * expands into "BREAK(0x0040)" which means a breakpoint occurred in the DSP
 * at location 0x40.  (DSP messages are messages from the DSP to the host,
 * as read by DSPReadMessages().  They are not error codes returned by 
 * libdsp functions.)
 */
