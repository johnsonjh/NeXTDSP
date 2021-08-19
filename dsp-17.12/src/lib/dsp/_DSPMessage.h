int _DSPHostMessageTimed(DSPFix48 *aTimeStampP, int msg);
/* 
 * Issue timed or untimed DSP "host message" (0 args) by issuing "xhm" 
 * host command.
 * 
 * This is private because it is assumed that there is enough room in the TMQ.
 * It is up to the caller to ensure this is true.
 */


int _DSPCallTimedV(DSPFix48 *aTimeStampP,int hm_opcode,int nArgs,...);
/*
 * Usage is int _DSPCallTimedV(aTimeStampP,hm_opcode,nArgs,arg1,...,ArgNargs);
 * Same as _DSPCallTimed() except that a variable number of host message 
 * arguments is specified explicitly in the argument list (using stdarg) 
 * rather than being passed in an array.
 */


int _DSPResetTMQ(void);
/*
 * Reset TMQ buffers to empty state and reset "current time" to 0.
 * Any waiting timed messages in the buffer are lost.
 */


int _DSPForceIdle(void);
/* 
 * Place DSP in the idle without any clean-up in the DSP.
 */

