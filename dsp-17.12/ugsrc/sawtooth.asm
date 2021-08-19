;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  08/03/87/jos - initial file created from oscs.asm
;;  08/04/87/jos - passed test/tsawtooth.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      sawtooth (UG macro) - Sawtooth oscillator
;;
;;  SYNOPSIS
;;      sawtooth pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,roundit
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (so that pf\_sawtooth_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = initial output vector address
;;      amp0      = initial maximum amplitude in [-1.0,1.0)
;;      inc0h     = initial amplitude increment, high-order word
;;      inc0l     = initial amplitude increment, low-order word
;;      phs0h     = initial amplitude, high-order word
;;      phs0l     = initial amplitude, low-order word
;;      roundit   = 0 means don't round and save an instruction
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     --------------           --------------
;;      y:(R_Y)+       current output address   aout0
;;      y:(R_Y)+       current max amplitude    amp0
;;      l:(R_L)P_L     current increment        inc0h,inc0l
;;      l:(R_L)P_L     current wave amplitude   phs0h,phs0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of a sawtooth. In pseudo-C:
;;
;;      amp  = y:(R_Y)+;         /* current peak amplitude */
;;      aout = y:(R_Y)+;         /* output address */
;;      inc  = l:(R_L)P_L;       /* amplitude increment */
;;      phs  = l:(R_L)P_L;       /* instantaneous amplitude */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           if (roundit) phs1=round(phs);
;;           if (phs1>amp) phs1=0;
;;           sout:aout[n] = phs1;
;;           phs += inc;
;;      }
;;
;;      phs1 denotes the high-order word of phs.
;;      round(phs) is approximately phs1 if the MSB of phs0 is 0, 
;;      and phs1+1 otherwise.  See The DSP56000 manual for precise
;;      rounding description.
;;
;;  USAGE RESTRICTIONS
;;      The amplitude increment cannot be negative unless the maximum
;;      amplitude is also negative.
;;  
;;  DSPWRAP ARGUMENT INFO
;;      sawtooth (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         amp,inch,incl,phsh,phsl,roundit
;;
;;  MINIMUM EXECUTION TIME
;;       (3*I_NTICK+9)/I_NTICK instruction cycles, no rounding
;;       (4*I_NTICK+9)/I_NTICK instruction cycles, with rounding
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/sawtooth.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscs.asm - simple sinusoidal oscillator
;;
;;  ALU REGISTER USE
;;      X1 = Amplitude
;;      X0 = Zero
;;      Y  = increment
;;      A  = current phase
;;
sawtooth  macro pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,roundit
          new_yarg pf\_sawtooth_\ic\_,aout,aout0
          new_yarg pf\_sawtooth_\ic\_,amp,amp0
          new_larg pf\_sawtooth_\ic\_,inc,inc0h,inc0l
          new_larg pf\_sawtooth_\ic\_,phs,phs0h,phs0l

; Set up data alu regs from state held in memory arguments
          move y:(R_Y)+,R_O        ; Current output pointer
          move y:(R_Y)+,X1         ; X1 = amplitude
          move #0,X0               ; reset value
          move l:(R_L)P_L,Y        ; Y = frequency (phase increment)
          move l:(R_L),A           ; A = current phase = ramp value
          do #I_NTICK,pf\_sawtooth_\ic\_tickloop
               if roundit
                    rnd A          ; this improves accuracy slightly
               endif
               cmp X1,A            ; current value minus max value
               tgt X0,A            ; reset
               add Y,A A,sout:(R_O)+    ; add in phase increment, ship
pf\_sawtooth_\ic\_tickloop
          move A,l:(R_L)P_L        ; A = next phase = next ramp value
          endm


