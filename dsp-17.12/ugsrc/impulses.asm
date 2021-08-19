;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  05/06/87/jos - initial file created from unoise.asm
;;  05/15/87/jos - passed test/timpulses.asm
;;  10/06/87/jos - failed test/timpulses.asm. It never could have worked
;;                 because Y0 was being subtracted from B which was doing
;;                 a single-precision subtract from B1.  B was always
;;                 negative, and we got pulses on every sample. I don't 
;;                 know how this could have happened. I do remember it
;;                 working in the past.  Perhaps there was a compensating
;;                 simulator bug back then!
;;  10/06/88/jos - passed after chaging "sub Y0,B" to "sub Y,B" and
;;                 reworking amplitude argument usage.
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      impulses (UG macro) - Periodic impulse-train generator
;;
;;  SYNOPSIS
;;      impulses pf,ic,sout,aout0,amp0,per0h,per0l,phs0h,phs0l
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_impulses_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = initial output vector address
;;      amp0      = initial amplitude in [-1.0,1.0)
;;      per0h     = initial period in samples, high-order word
;;      per0l     = initial period in samples, low-order word
;;      phs0h     = initial phase in samples, high-order word
;;      phs0l     = initial phase in samples, low-order word
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     --------------           --------------
;;      x:(R_X)+       current amplitude        amp0
;;      y:(R_Y)+       current output address   aout0
;;      l:(R_L)P_L     current period           per0h,per0l
;;      l:(R_L)P_L     current phase            phs0h,phs0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of an impulse train by counting down to zero.
;;      The phase is the number of initial zero samples.
;;      While inexpensive, this type of impulse train suffers from aliasing
;;      in the spectrum except when the desired period is an integer.
;;      See /usr/lib/dsp/ugsrc/test/toscg5.asm for an example of generating
;;      bandlimited pulse trains with less aliasing.
;;
;;      In pseudo-C:
;;
;;      amp  = x:(R_X)+;           /* current amplitude */
;;      aout = y:(R_Y)+;           /* output address */
;;      per  = l:(R_L)P_L;         /* waveform period */
;;      phs  = l:(R_L)P_L;         /* current count-down value */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           phs1 -= 1;
;;           if (phs1<0) {
;;                sout:aout[n] = amp; /* Fire off a unit-sample pulse */
;;                phs += per;         /* Reset count-down phase */
;;           } else sout:aout[n] = 0; /* No unit-sample pulse */
;;      }
;;
;;      phs1 denotes the high-order word of phs, interpreted as an integer.
;;
;;  USAGE RESTRICTIONS
;;      For perfectly periodic impulse trains, require per=1.0 (perh=1 and perl=0).
;;
;;  DSPWRAP ARGUMENT INFO
;;      impulses (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         amp,perh,perl,phsh,phsl
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/impulses.asm
;;
;;  ALU REGISTER USE
;;      Y1 = Amplitude or 0
;;       Y = 1 (double-precision decrement)
;;       X = Period (double precision)
;;       B = Phase (double-precision count-down count)
;;       A = amplitude or 0 = output signal
;;
impulses  macro pf,ic,sout,aout0,amp0,per0h,per0l,phs0h,phs0l
          new_xarg pf\_impulses_\ic\_,amp,amp0
          new_yarg pf\_impulses_\ic\_,aout,aout0
          new_larg pf\_impulses_\ic\_,per,per0h,per0l
          new_larg pf\_impulses_\ic\_,phs,phs0h,phs0l

; Set up data alu regs from state held in memory arguments

          move #0,Y1               ; decrement 
          move #>1,Y0              ; decrement 
          move y:(R_Y)+,R_O        ; R_O Current output pointer
          move l:(R_L)P_L,X        ; X = current period
          clr A l:(R_L),B          ; B = current phase
          do #I_NTICK,pf\_impulses_\ic\_tickloop
pf\_impulses_\ic\_continue
               sub Y,B A,sout:(R_O)+         ; decrement counter, ship 0
               jlt pf\_impulses_\ic\_dopulse ; pulse on -1 (phase 0 = 1st)
pf\_impulses_\ic\_tickloop    
          jmp _done
pf\_impulses_\ic\_dopulse
          move x:(R_X),Y1               ; Y1 = amplitude
          add X,B Y1,sout:-(R_O)        ; redo previous sample
          movec LC,A                    ; check loop counter
          tst A     (R_O)+              ; if zero, don't go back to loop
          jeq _done
          clr A #0,Y1                   ; back to loop
          jmp pf\_impulses_\ic\_continue
_done
          move x:(R_X)+,Y1              ; Increment R_X
          move B,l:(R_L)P_L             ; save phase in memory argument 
          endm


