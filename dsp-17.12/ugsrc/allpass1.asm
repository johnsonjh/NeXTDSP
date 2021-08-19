;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  10/06/88/jos - initial file created from /usr/lib/dsp/ugsrc/onepole.asm
;;  10/06/88/jos - passed tallpass1.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      allpass1 (UG macro) - one-pole digital allpass filter section
;;
;;  SYNOPSIS
;;      allpass1 pf,ic,sout,aout0,sinp,ainp0,s0,bb00
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_allpass1_\ic\_ is globally unique)
;;      sout      = output vector memory space ('x' or 'y')
;;      aout0     = initial output vector memory address
;;      sinp      = input vector memory space ('x' or 'y')
;;      ainp0     = initial input vector memory address
;;      bb00      = initial coefficient of undelayed input and delayed output
;;      s0        = state variable
;;
;;  DSP MEMORY ARGUMENTS
;;      Access         Description              Initialization
;;      ------         -----------              --------------
;;      x:(R_X)+       Current input address    ainp0
;;      y:(R_Y)+       Current output address   aout0
;;      x:(R_X)+       bb0 coefficient          bb00
;;      y:(R_Y)+       s state variable         s0
;;
;;  DESCRIPTION
;;      The allpass1 unit-generator implements a one-pole, one-zero
;;      allpass filter section in direct form. 
;;
;;      The transfer function implemented is
;;
;;              bb0 + 1/z
;;      H(z) =  ---------
;;              1 + bb0/z
;;
;;      In pseudo-C notation:
;;
;;      ainp = x:(R_X)+;
;;      aout = y:(R_Y)+;
;;      bb0 = x:(R_X)+;
;;      s = y:(R_Y);
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           t = sinp:ainp[n] - bb0*s;
;;           sout:aout[n] = bb0*t + s;
;;           s = t;
;;      }
;;
;;      y:(R_Y)+ = s;
;;        
;;  DSPWRAP ARGUMENT INFO
;;      allpass1 (prefix)pf,(instance)ic,
;;         (dspace)sout,(output)aout,
;;         (dspace)sinp,(input)ainp,s,bb0
;;
;;  MAXIMUM EXECUTION TIME
;;      116 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      116 DSP clock cycles for one "tick".
;;
;;  TEST PROGRAM
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/allpass1.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/onezero.asm     - one-zero filter section
;;      /usr/lib/dsp/ugsrc/onepole.asm     - one-pole filter section
;;      /usr/lib/dsp/ugsrc/twopole.asm     - two-pole filter section
;;      /usr/lib/dsp/ugsrc/allpass.asm     - like allpass1 but with large delay
;;  
;;  ALU REGISTER USE
;;      X0 = bb0 = undelayed input coefficient = once-delayed-output coeff.
;;      Y1 = v(n-1) = s = once delayed auxiliary signal
;;      A  = (1) x(n) (current input signal), then (2) v(n)
;;      B  = v(n) (auxiliary signal), then (2) y(n) (current output)
;;
allpass1 macro pf,ic,sout,aout0,sinp,ainp0,s0,bb00
             new_xarg pf\_allpass1_\ic\_,ainp,ainp0   ; input address arg
             new_yarg pf\_allpass1_\ic\_,aout,aout0   ; output address arg
             new_xarg pf\_allpass1_\ic\_,bb0,bb00     ; undelayed input coeff
             new_yarg pf\_allpass1_\ic\_,s,s0         ; once delayed output

             move x:(R_X)+,R_I1                 ; input address to R_I1
             move y:(R_Y)+,R_O                  ; output address to R_O
             move sinp:(R_I1)+,A                ; x(n)
             move sout:-(R_O),B                 ; y(n+1)
             move x:(R_X)+,X0 y:(R_Y),Y1        ; bb0 to X0, s = v(n-1) to Y1
             do #I_NTICK,pf\_allpass1_\ic\_tickloop

;               v(n)=x(n)-bb0*v(n-1),   ship B = y(n)
                mac -X0,Y1,A            B,sout:(R_O)+   

;               v(n-1) -> B             v(n) -> Y1
                tfr Y1,B                A,Y1

;               y(n)=v(n)+bb0*v(n-1)    x(n) -> A
                macr X0,Y1,B            sinp:(R_I1)+,A

pf\_allpass1_\ic\_tickloop    
                move B,sout:(R_O)+      ; last output
                move Y1,y:(R_Y)+        ; save filter state in mem arg
     endm


