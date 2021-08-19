;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/06/87/jos - initial file created from Andy Moorer's asymp
;;  05/06/87/jos - passed test/tasymp.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      asymp (UG macro) - One segment of an exponential (ADSR type) envelope
;;
;;  SYNOPSIS
;;      asymp orch,1,sout,aout0,rate0,trg0,amp0h,amp0l    ; exponential segment
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_asymp_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = output vector address
;;      rate0     = rate for this envelope (0 to 1.0-EPSILON)
;;      trg0      = target value for this envelope (0 to 0.5-EPSILON)
;;      amp0h     = amplitude, high-order word
;;      amp0l     = amplitude,  low-order word
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use        Initialization
;;      ----------     --------------      --------------
;;      x:(R_X+)       output address      aout0
;;      x:(R_X+)       target value        trg0
;;      y:(R_Y+)       exponential ratio   rate0
;;      l:(R_L-)       envelope state      amp0h,amp0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of an exponential segment. In pseudo-C:
;;
;;      aout = x:(R_X)+; /* Output address */
;;      trg  = x:(R_X)+; /* Final value approached by exponential */
;;      rate = y:(R_Y)+; /* 0 means no change, 1 = reach target in 1 sample */
;;      amp  = l:(R_L)P_L; /* Load initial envelope amplitude */
;;
;;      for (n=0;n<I_NTICK;n++) { /* Compute a tick's worth of exponential */
;;           amp = (1.0-rate)*amp + rate*trg;
;;           sout:aout[n] = amp;
;;      }
;;
;;  DSPWRAP ARGUMENT INFO
;;      asymp (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;              rate,trg,amph,ampl
;;
;;  MAXIMUM EXECUTION TIME
;;      80 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      80 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/asymp.asm
;;
;;  ALU REGISTER USE
;;      A  = amplitude envelope value amp
;;      X1 = amplitude copied from a
;;      X0 = target trg
;;      Y1 = rate rate
;;
asymp     macro pf,ic,sout,aout0,rate0,trg0,amp0h,amp0l
; Allocate arguments for this instance
          new_xarg pf\_asymp_\ic\_,aout,aout0
          new_xarg pf\_asymp_\ic\_,trg,trg0
          new_yarg pf\_asymp_\ic\_,rate,rate0
          new_larg pf\_asymp_\ic\_,amp,amp0h,amp0l
; Set up data alu regs:
          move x:(R_X)+,R_O             ; Current output pointer
          move x:(R_X)+,X0 y:(R_Y)+,y1  ; X0 = t, y1 = r
          move l:(R_L),A                ; load current (long) amplitude to A
; First envelope sample
          mac  X0,y1,A  A,X1            ; A=A+t*r = amp+r*trg
          mac  -X1,y1,A                 ; A=A-r*amp = amp*(1-r)+r*trg
; Remaining I_NTICK-1 envelope samples:
          do #I_NTICK-1,pf\_asymp_\ic\_l1
               mac X0,y1,A A,X1         ; X1 = sample1 amp, A=A+f*r
               mac -X1,y1,A X1,sout:(R_O)+ ; X1 out, A = sample2 amp
pf\_asymp_\ic\_l1
          move A,sout:(R_O)+            ; write out last result
          move A,l:(R_L)P_L             ; save last amplitude for next (A10?)
          endm


