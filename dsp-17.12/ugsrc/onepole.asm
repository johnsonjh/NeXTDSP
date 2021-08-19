;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  10/16/87/jos - initial file created from /usr/lib/dsp/ugsrc/onezero.asm
;;  10/16/87/jos - passed test/tonezero.asm and tonezero0.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      onepole (UG macro) - one-pole digital filter section
;;
;;  SYNOPSIS
;;      onepole pf,ic,sout,aout0,sinp,ainp0,s0,bb00,aa10
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_onepole_\ic\_ is globally unique)
;;      sout      = output vector memory space ('x' or 'y')
;;      aout0     = initial output vector memory address
;;      sinp      = input vector memory space ('x' or 'y')
;;      ainp0     = initial input vector memory address
;;      bb00      = initial coefficient of undelayed input
;;      aa10      = initial coefficient of negated, once-delayed output
;;      s0        = state variable = once-delayed output
;;
;;  DSP MEMORY ARGUMENTS
;;      Access         Description              Initialization
;;      ------         -----------              --------------
;;      x:(R_X)+       Current output address   aout0
;;      x:(R_X)+       Current input address    ainp0
;;      x:(R_X)+       s state variable         s0
;;      y:(R_Y)+       bb0 coefficient          bb00
;;      y:(R_Y)+       aa1 coefficient          aa10
;;
;;  DESCRIPTION
;;      The onepole unit-generator implements a one-pole
;;      filter section in direct form. In pseudo-C notation:
;;
;;      ainp = x:(R_X)+;
;;      aout = x:(R_X)+;
;;      s = x:(R_X)+;
;;      bb0 = y:(R_Y)+;
;;      aa1 = y:(R_Y)+;
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           s = bb0*sinp:ainp[n] - aa1*s;
;;           sout:aout[n] = s;
;;      }
;;        
;;  DSPWRAP ARGUMENT INFO
;;      onepole (prefix)pf,(instance)ic,
;;         (dspace)sout,(output)aout,
;;         (dspace)sinp,(input)ainp,s,bb0,aa1
;;
;;  MAXIMUM EXECUTION TIME
;;      84 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      80 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/onepole.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/onezero.asm - one-zero filter section
;;      /usr/lib/dsp/ugsrc/twopole.asm - two-pole filter section
;;      /usr/lib/dsp/ugsrc/biquad.asm  - two-pole, two-zero filter section
;;      /usr/lib/dsp/ugsrc/onezero.asm - one-zero filter section
;;  
;;  ALU REGISTER USE
;;      When "sout"=='y',
;;      X0 = x(n) = in = current input signal
;;      X1 = y(n-1) = s = once delayed output
;;      Y0 = bb0 = undelayed input coefficient
;;      Y1 = aa1 = once-delayed signal coefficient
;;       A = multiply-add accumulator
;;      When "sout"=='x', X1 and Y1 are interchanged
;;
onepole macro pf,ic,sout,aout0,sinp,ainp0,s0,bb00,aa10
             new_xarg pf\_onepole_\ic\_,aout,aout0   ; output address arg
             new_xarg pf\_onepole_\ic\_,ainp,ainp0   ; input address arg
             new_xarg pf\_onepole_\ic\_,s,s0         ; once delayed input sample
             new_yarg pf\_onepole_\ic\_,bb0,bb00     ; undelayed input coeff
             new_yarg pf\_onepole_\ic\_,aa1,aa10     ; once-delayed input coeff

             move x:(R_X)+,R_O                  ; output address to R_O
             move x:(R_X)+,R_I1                 ; input address to R_I1

         if "sout"=='x'
             move x:(R_X),Y1                    ; s to Y1
           if "sinp"=='x'
             move sinp:(R_I1)+,X0 y:(R_Y)+,Y0      ; input to X0, bb0 to Y0
           else
             move sinp:(R_I1)+,X0  ; input to X0
             move y:(R_Y)+,Y0   ; bb0 to Y0
           endif             
             mpy  X0,Y0,A       y:(R_Y)+,X1     ; in*bb0, aa1 to X1
             macr -Y1,X1,A   sinp:(R_I1)+,X0            ; first output
             do #I_NTICK-1,pf\_onepole_\ic\_tickloop
               mpy  X0,Y0,A     A,sout:(R_O)+      A,Y1    ; in*bb0
               macr -Y1,X1,A    sinp:(R_I1)+,X0         ; y(n)
pf\_onepole_\ic\_tickloop    
             move A,x:(R_O)+          ; ship last output
             move A,x:(R_X)+          ; save filter state in mem arg
         else
             move x:(R_X),X1    y:(R_Y)+,Y0     ; s to X1, bb0 to Y0:
             move            sinp:(R_I1)+,X0    ; input to X0
             mpy  X0,Y0,A       y:(R_Y)+,Y1     ; in*bb0, aa1 to Y1
             macr -X1,Y1,A      sinp:(R_I1)+,X0 ; first output
             do #I_NTICK-1,pf\_onepole_\ic\_tickloop
               mpy  X0,Y0,A     A,X1            A,sout:(R_O)+
               macr -X1,Y1,A    sinp:(R_I1)+,X0
pf\_onepole_\ic\_tickloop    
             move A,y:(R_O)+ A,x:(R_X)+ ; ship last out, save filter state
          endif
     endm


