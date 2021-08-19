;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/28/87/jos - initial file created from /usr/lib/dsp/ugsrc/biquad.asm
;;  05/24/87/jos - passed test/tonezero.asm
;;  10/06/88/jos - added fast case for y input
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      onezero (UG macro) - one-zero digital filter section
;;
;;  SYNOPSIS
;;      onezero pf,ic,sout,aout0,sinp,ainp0,s0,bb00,bb10
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_onezero_\ic\_ is globally unique)
;;      sout      = output vector memory space ('x' or 'y')
;;      aout0     = initial output vector memory address
;;      sinp      = input vector memory space ('x' or 'y')
;;      ainp0     = initial input vector memory address
;;      bb00      = initial coefficient of undelayed input
;;      bb10      = initial coefficient of once delayed input
;;      s0        = initial once delayed state variable
;;
;;  DSP MEMORY ARGUMENTS
;;      Access         Description              Initialization
;;      ------         -----------              --------------
;;      x:(R_X)+       Current output address   aout0
;;      x:(R_X)+       Current input address    ainp0
;;      x:(R_X)+       s state variable         s0
;;      y:(R_Y)+       bb0 coefficient          bb00
;;      y:(R_Y)+       bb1 coefficient          bb10
;;
;;  DESCRIPTION
;;      The onezero unit-generator implements a one-zero
;;      filter section in direct form.  For best performance,
;;      the input and output signals should be in separate
;;      memory spaces x or y.
;;
;;      In pseudo-C notation:
;;
;;      ainp = x:(R_X)+;
;;      aout = x:(R_X)+;
;;      s = x:(R_X)+;
;;      bb0 = y:(R_Y)+;
;;      bb1 = y:(R_Y)+;
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           in = sinp:ainp[n];
;;           sout:aout[n] = bb0*in + bb1*s;
;;           s = in;
;;      }
;;        
;;  DSPWRAP ARGUMENT INFO
;;      onezero (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         (dspace)sinp,(input)ainp,s,bb0,bb1
;;
;;  MAXIMUM EXECUTION TIME
;;      116 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;       82 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/onezero.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/biquad.asm  - two-pole, two-zero filter section
;;      /usr/lib/dsp/ugsrc/onepole.asm - one-pole filter section
;;      /usr/lib/dsp/ugsrc/twopole.asm - two-pole filter section
;;
;;  ALU REGISTER USE
;;      X0 = x(n) = in = current input signal
;;      X1 = x(n-1) = s = once delayed input
;;      Y0 = bb0 = undelayed signal coefficient
;;      Y1 = bb1 = once-delayed signal coefficient
;;       A = multiply-add accumulator
;;       B = input signal copy for transfer to X1
;;
onezero   macro pf,ic,sout,aout0,sinp,ainp0,s0,bb00,bb10
               new_xarg pf\_onezero_\ic\_,aout,aout0   ; output address arg
               new_xarg pf\_onezero_\ic\_,ainp,ainp0   ; input address arg
               new_xarg pf\_onezero_\ic\_,s,s0    ; once delayed input sample
               new_yarg pf\_onezero_\ic\_,bb0,bb00     ; undelayed input coeff
               new_yarg pf\_onezero_\ic\_,bb1,bb10     ; once-delayed input coeff

               move x:(R_X)+,R_O                  ; output address to R_O
               move x:(R_X)+,R_I1                 ; input address to R_I1
             if "sinp"=='x'
               move x:(R_X),X1 y:(R_Y)+,Y0        ; s to X1, bb0 to Y0
               move y:(R_Y)+,Y1                   ; bb1 to Y1
               move sinp:(R_I1),X0                ; First input sample to X0
               mpy  X1,Y1,A   sinp:(R_I1)+,X1     ; s*b1, update s=x(n-1)
               macr X0,Y0,A   sinp:(R_I1),X0      ; x(n)*b0,   update x(n)
               do #I_NTICK-1,pf\_onezero_\ic\_tickloop
                    if "sout"!="sinp"
;                        A=bb1*x(n-1)   X1=x(n)             out=y(n-1)
                         mpy  X1,Y1,A   sinp:(R_I1)+,X1     A,sout:(R_O)+
                    else
                         move           A,sout:(R_O)+
                         mpy  X1,Y1,A   sinp:(R_I1)+,X1
                    endif
;                   A=A+bb0*x(n)   X0=x(n+1)
                    macr X0,Y0,A   sinp:(R_I1),X0
pf\_onezero_\ic\_tickloop    
               if "sout"!='x'
                    move X1,x:(R_X)+    A,sout:(R_O)+
               else
                    move A,sout:(R_O)+       ; ship last output
                    move X1,x:(R_X)+         ; save filter state in mem arg
               endif
             else ; "sinp"=='y' => same switching X1 and Y1. Also 'x' and 'y'.
               move x:(R_X),Y1                    ; s to Y1
               move y:(R_Y)+,Y0                   ; bb0 to Y0
               move y:(R_Y)+,X1                   ; bb1 to X1
               move sinp:(R_I1),X0                ; First input sample to X0
               mpy  Y1,X1,A   sinp:(R_I1)+,Y1     ; s*bb1, update s=x(n-1)
               macr X0,Y0,A   sinp:(R_I1),X0      ; x(n)*b0,   update x(n)
               do #I_NTICK-1,pf\_onezero_\ic\_tickloop
                    if "sout"!="sinp"
;                        A=bb1*x(n-1)   Y1=x(n)             out=y(n-1)
                         mpy  Y1,X1,A   sinp:(R_I1)+,Y1     A,sout:(R_O)+
                    else
                         move           A,sout:(R_O)+
                         mpy  Y1,X1,A   sinp:(R_I1)+,Y1
                    endif
;                   A=A+bb0*x(n)   X0=x(n+1)
                    macr X0,Y0,A   sinp:(R_I1),X0
pf\_onezero_\ic\_tickloop    
               move A,sout:(R_O)+       ; ship last output
               move Y1,x:(R_X)+         ; save filter state in mem arg
             endif
     endm


