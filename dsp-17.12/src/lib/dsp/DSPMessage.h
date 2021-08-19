/* DSPMessage.h - procedure prototypes for functions in DSPMessage.c 
   These functions have been moved to DSPObject.c, but they are still declared here.
   Copyright 1989 by NeXT Inc.
*/


extern int DSPMessagesOff(void);
/* 
 * Turn off DSP messages at the source.
 */


extern int DSPMessagesOn(void);
/* 
 * Enable DSP messages.
 */


extern int DSPMessageGet(int *msgp);
/*
 * Return a single DSP message in *msgp, if one is waiting,
 * otherwise it returns the DSP error code DSP_ENOMSG.
 * The DSP message returned in *msgp is 24 bits, right justified.
 */


extern int DSPAwaitAnyMessage(
    int *dspackp,		/* returned DSP message */
    int msTimeLimit);		/* time-out in milliseconds */
/*
 * Await any message from the DSP.
 */


extern int DSPAwaitUnsignedReply(
    DSPAddress opcode,	       /* opcode of expected DSP message */
    DSPFix24 *datum,	       /* datum of  expected DSP message (returned) */
    int msTimeLimit);	       /* time-out in milliseconds */
/* 
 * Wait for specific DSP message containing an unsigned datum.
 */


extern int DSPAwaitSignedReply(
    DSPAddress opcode,	    /* opcode of expected DSP message */
    int *datum,		    /* datum of	 expected DSP message (returned) */
    int msTimeLimit);	    /* time-out in milliseconds */
/* 
 * Wait for specific DSP message containing a signed datum.
 */


extern int DSPAwaitMessage(
    DSPAddress opcode,		/* opcode of expected DSP message */
    int msTimeLimit);		/* time-out in milliseconds */
/* 
 * Return specific DSP message, declaring any others as errors.
 */


extern int DSPHostMessage(int msg);
/* 
 * Issue untimed DSP "host message" (minus args) by issuing "xhm" 
 * host command.  Example: DSPHostMessage(DSP_HM_ABORT).
 */


extern int DSPMKHostMessageTimed(DSPFix48 *aTimeStampP, int msg);

/* 
 * Issue timed or untimed DSP "host message" (0 args) by issuing "xhm" 
 * host command.  Example: DSPMKHostMessageTimed(aTimeStampP,DSP_HM_ABORT).
 */


extern int DSPMKHostMessageTimedFromInts(
    int msg,	      /* Host message opcode. */
    int hiwd,	      /* High word of time stamp. */
    int lowd);	      /* Lo   word of time stamp. */
/* 
 * Same as DSPMKHostMessageTimed(), but taking time stamp from ints
 * instead of a DSPFix48 struct.
 */


extern int DSPCall(
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray);
/*
 * Send an untimed host message to the DSP.
 */


extern int DSPCallB(
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray);
/*
 * Same as DSPCall() except that the argArray is sent in reverse
 * order to the DSP.
 */


extern int DSPCallV(DSPAddress hm_opcode,int nArgs,...);
/*
 * Usage is int DSPCallV(hm_opcode,nArgs,arg1,...,ArgNargs);
 * Same as DSPCall() except that a variable number of host message arguments 
 * is specified explicitly (using varargs) rather than being passed in an
 * array.
 */


extern int DSPMKFlushTimedMessages(void);
/* 
 * Flush all combined timed messages for the current time. 
 * You must send this if you are sending updates to the DSP 
 * asynchronously (e.g. in response to MIDI or mouse events 
 * as opposed to via the Music Kit Conductor). 
 */


extern int DSPMKCallTimed(
    DSPFix48 *aTimeStampP,
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray);
/*
 * Enqueue a timed host message for the DSP.  If the time stamp of the
 * host message is greater than that of the host messages currently
 * in the timed message buffer, the buffer is flushed before the
 * new message is enqueued.  If the timed stamp is equal to those
 * currently in the buffer, it is appended to the buffer.  It is an
 * error for the time stamp to be less than that of the current
 * timed message buffer, unless it is zero; a zero time stamp means
 * execute the message at the end of the current "tick" in the DSP.
 * If aTimeStamp is NULL, the host message is executed untimed.  
 */


extern int DSPMKCallTimedV(DSPFix48 *aTimeStampP,int hm_opcode,int nArgs,...);
/*
 * Usage is int DSPMKCallTimedV(aTimeStampP,hm_opcode,nArgs,arg1,...,ArgNargs);
 * Same as DSPMKCallTimed() except that a variable number of host message 
 * arguments is specified explicitly in the argument list (using stdarg) 
 * rather than being passed in an array.
 */

