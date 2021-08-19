;; Copyright 1989 by NeXT Inc.
;; Author - David Jaffe
;;
;; Modification history
;; --------------------
;; 05/10/88/daj - initial file created from /usr/lib/dsp/ugsrc/onepole.asm
;; 05/20/88/daj - passed test ttwopole.asm for YX case
;; 05/23/88/daj - added other memory space combos (not tested!)
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      twopole (UG macro) - two-pole digital filter section
;;
;;  SYNOPSIS
;;      twopole pf,ic,sout,aout0,sinp,ainp0,s10,s20,bb00,aa10,aa20
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_twopole_\ic\_ is globally unique)
;;      sout      = output vector memory space ('x' or 'y')
;;      aout0     = initial output vector memory address
;;      sinp      = input vector memory space ('x' or 'y')
;;      ainp0     = initial input vector memory address
;;      bb00      = initial coefficient of undelayed input
;;      aa10      = initial coefficient of negated, once-delayed output
;;      aa20      = initial coefficient of negated, twice-delayed output
;;      s1        = state variable = once-delayed output
;;      s2        = state variable = twice-delayed output
;;
;;  DSP MEMORY ARGUMENTS
;;      The following is for the YX case: (y is output, x is input)
;;      Access         Description              Initialization
;;      ------         -----------              --------------
;;      x:(R_X)+       aa1 coefficient          aa10
;;      x:(R_X)+       aa2 coefficient          aa20
;;      x:(R_X)+       bb0 coefficient          bb00
;;      y:(R_Y)+       Current output address   aout0
;;      y:(R_Y)+       Current input address    ainp0
;;      y:(R_Y)+       s2 state variable         s20
;;      y:(R_Y)+       s1 state variable         s10
;;      The XY case is like the YX case, except all x and y args are swapped.
;;      The YY case is like the YX case, except s1 is an x arg and preceeds 
;;              all other x args.
;;      The XX case is like the YY case, except all x and y args are swapped.
;;
;;  DESCRIPTION
;;      The twopole unit-generator implements a two-pole
;;      filter section in direct form. In pseudo-C notation:
;;      (This is for the YX case. See DSP MEMORY ARGUMENTS above)
;;      ainp = x:(R_X)+;
;;      aout = x:(R_X)+;
;;      s2 = y:(R_Y)+;
;;      s1 = y:(R_Y)+;
;;      aa1 = x:(R_X)+;
;;      aa2 = x:(R_X)+;
;;      bb0 = x:(R_X)+;
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           sout:aout[n] = bb0*sinp:ainp[n] - aa1*s1 - aa2*s2;
;;           s2 = s1            
;;           s1 = sout:aout[n]
;;      }
;;        
;;  DSPWRAP ARGUMENT INFO
;;      twopole (prefix)pf,(instance)ic,
;;         (dspace)sout,(output)aout,
;;         (dspace)sinp,(input)ainp,s1,s2,bb0,aa1
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/twopole.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/biquad.asm  - two-pole, two-zero filter section
;;      /usr/lib/dsp/ugsrc/onezero.asm - one-zero filter section
;;      /usr/lib/dsp/ugsrc/onepole.asm - one-pole filter section
;;      /usr/lib/dsp/ugsrc/twopole.asm - two-pole filter section
;; 
;;  ALU REGISTER USE
;;      When "sout"=='y' and "sinp"=='x'
;;      X0 = coefficients
;;      X1 = input sample
;;      Y0 = s2 = twice delayed output
;;      Y1 = s1 = once delayed output
;;       A = multiply-add accumulator
;;      The XY case is like the YX case, except all X and Y regs are swapped.
;;      The YY case is like the YX case, except X1 and Y1 are swapped.
;;      The XX case is like the YY case, except all X and Y regs are swapped.
;;
twopole macro pf,ic,sout,aout0,sinp,ainp0,s10,s20,bb00,aa10,aa20
;; YX CASE
        if "sout"=='y'&&"sinp"=='x'
             new_xarg pf\_twopole_\ic\_,aa1,aa10   ; once-delayed input coeff
             new_xarg pf\_twopole_\ic\_,aa2,aa20   ; twice-delayed input coeff
             new_xarg pf\_twopole_\ic\_,bb0,bb00   ; undelayed input coeff
             new_yarg pf\_twopole_\ic\_,aout,aout0 ; output address arg
             new_yarg pf\_twopole_\ic\_,ainp,ainp0 ; input address arg
             new_yarg pf\_twopole_\ic\_,s2,s20     ; twice delayed input samp
             new_yarg pf\_twopole_\ic\_,s1,s10     ; once delayed input samp

             move y:(R_Y)+,R_I2                 ; output address to R_I2
             move y:(R_Y)+,R_I1                 ; input address to R_I1
             move #2,N_X                        ; used for arg reset
             move                      y:(R_Y)+,Y0      ; s2 to Y0
             move                      y:(R_Y)-,Y1      ; s1 to Y1
             move x:(R_X+N_X),X0                ; bb0 to X0
             move x:(R_I1)+,X1                  ; first input
                                                ; compute the first sample
             mpy        X1,X0,A        x:(R_X)+,X0     ; bb0 * in, get aa1
             mac        -Y1,X0,A       x:(R_X)+,X0     ; aa1 * s1, get aa2
             macr       -Y0,X0,A       Y1,Y0          ; aa2 * s2, s1 to s2
             tfr        A,Y1           x:(R_X)-N_X,X0               
                ; y(n) to s1. Get bb0 and reset R_X to point to aa1
             move                      x:(R_I1)+,X1    ; next input 
             do #I_NTICK-1,pf\_twopole_\ic\_tickloop
                mpy     X1,X0,A         x:(R_I1)+,X1  A,y:(R_I2)+       
                ; input * bb0, fetch next input and deposit previous output
                move                    x:(R_X)+,X0   ; aa1 to X0
                mac     -Y1,X0,A        x:(R_X)+,X0   ; y(n-1) * aa1, aa2 to X0
                macr    -Y0,X0,A        Y1,Y0         ; y(n-2) * aa2, s2 = s1
                tfr     A,Y1            x:(R_X)-N_X,X0              
                ; y(n) to s1. Get bb0 and reset R_X to point to aa1
pf\_twopole_\ic\_tickloop    
             move       A,y:(R_I2)              ; ship last output
             move Y0,y:(R_Y)+                    ; save s2
             move Y1,y:(R_Y)+    x:(R_X)+,X0     ; save s1, increment R_X
             move x:(R_X)+N_X,X0                 ; adjust R_X
             endif
;; XY CASE
        if "sout"=='x'&&"sinp"=='y'
             new_yarg pf\_twopole_\ic\_,aa1,aa10   ; once-delayed input coeff
             new_yarg pf\_twopole_\ic\_,aa2,aa20   ; twice-delayed input coeff
             new_yarg pf\_twopole_\ic\_,bb0,bb00   ; undelayed input coeff
             new_xarg pf\_twopole_\ic\_,aout,aout0 ; output address arg
             new_xarg pf\_twopole_\ic\_,ainp,ainp0 ; input address arg
             new_xarg pf\_twopole_\ic\_,s2,s20     ; twice delayed input samp
             new_xarg pf\_twopole_\ic\_,s1,s10     ; once delayed input samp

             move x:(R_X)+,R_I2                 ; output address to R_I2
             move x:(R_X)+,R_I1                 ; input address to R_I1
             move #2,N_Y                        ; used for arg reset
             move                      x:(R_X)+,X0      ; s2 to X0
             move                      x:(R_X)-,X1      ; s1 to X1
             move y:(R_Y+N_Y),Y0                ; bb0 to Y0
             move y:(R_I1)+,Y1                  ; first input
                                                ; compute the first sample
             mpy        Y1,Y0,A        y:(R_Y)+,Y0     ; bb0 * in, get aa1
             mac        -X1,Y0,A       y:(R_Y)+,Y0     ; aa1 * s1, get aa2
             macr       -X0,Y0,A        X1,X0          ; aa2 * s2, s1 to s2
             move                      y:(R_Y)-N_Y,Y0  A,X1             
                ; x(n) to s1. Get bb0 and reset R_Y to point to aa1
             move                      y:(R_I1)+,Y1    ; next input 
             do #I_NTICK-1,pf\_twopole_\ic\_tickloop
                mpy     Y1,Y0,A         y:(R_I1)+,Y1  A,x:(R_I2)+       
                ; input * bb0, fetch next input and deposit previous output
                move                    y:(R_Y)+,Y0   ; aa1 to Y0
                mac     -X1,Y0,A        y:(R_Y)+,Y0   ; x(n-1) * aa1, aa2 to Y0
                macr    -X0,Y0,A        X1,X0         ; x(n-2) * aa2, s2 = s1
                move                    y:(R_Y)-N_Y,Y0  A,X1            
                ; x(n) to s1. Get bb0 and reset R_Y to point to aa1
pf\_twopole_\ic\_tickloop    
             move       A,x:(R_I2)              ; ship last output
             move X0,x:(R_X)+                    ; save s2
             move X1,x:(R_X)+    y:(R_Y)+,Y0     ; save s1, increment R_Y
             move y:(R_Y)+N_Y,Y0                 ; adjust R_Y
             endif
;; YY CASE
        if "sout"=='y'&&"sinp"=='y'
             new_xarg pf\_twopole_\ic\_,s1,s10     ; once delayed input samp
             new_xarg pf\_twopole_\ic\_,aa1,aa10   ; once-delayed input coeff
             new_xarg pf\_twopole_\ic\_,aa2,aa20   ; twice-delayed input coeff
             new_xarg pf\_twopole_\ic\_,bb0,bb00   ; undelayed input coeff
             new_yarg pf\_twopole_\ic\_,aout,aout0 ; output address arg
             new_yarg pf\_twopole_\ic\_,ainp,ainp0 ; input address arg
             new_yarg pf\_twopole_\ic\_,s2,s20     ; twice delayed input samp

             move y:(R_Y)+,R_O                 ; output address to R_O
             move R_Y,R_I1                     ; save R_Y
             move y:(R_Y)+,R_I2                ; get input address
             move x:(R_X)+,X1  y:(R_Y)+,Y0     ; s2 to Y0, s1 to X1
             move #2,N_X                        ; used for arg reset below
             move R_I2,R_Y                     ; input address to R_Y
             move x:(R_X+N_X),X0                ; bb0 to X0
             move y:(R_Y)+,Y1                   ; first input
                                                ; compute the first sample
             mpy        Y1,X0,A        x:(R_X)+,X0     ; bb0 * in, get aa1
             mac        -X1,X0,A       x:(R_X)+,X0     ; aa1 * s1, get aa2
             macr       -Y0,X0,A        X1,Y0          ; aa2 * s2, s1 to s2
             move                      x:(R_X)-N_X,X0  A,X1             
                ; y(n) to s1. Get bb0 and reset R_X to point to aa1
             move                      y:(R_I1)+,Y1    ; next input 
             do #I_NTICK-1,pf\_twopole_\ic\_tickloop
                mpy     Y1,X0,A         A,X1         A,y:(R_O)+ 
                ; input * bb0, s1 = y(n) deposit y(n)
                move                    x:(R_X)+,X0  y:(R_Y)+,Y1 
                ; aa1 to X0, get input
                mac     -X1,X0,A        x:(R_X)+,X0   ; y(n-1) * aa1, aa2 to X0
                macr    -Y0,X0,A        X1,Y0         ; y(n-2) * aa2, s2 = s1
                move                    x:(R_X)-N_X,X0 ; reset R_X and get bb0
pf\_twopole_\ic\_tickloop    
             move A,y:(R_O)                      ; ship last output
             move R_I1,R_Y                       ; restore R_Y
             move X1,x:(R_X)+    Y0,y:(R_Y)+     ; save s2, s1
             move x:(R_X)+N_X,X0                 ; adjust R_X
             endif
;; XX CASE
        if "sout"=='x'&&"sinp"=='x'
             new_yarg pf\_twopole_\ic\_,s1,s10     ; once delayed input samp
             new_yarg pf\_twopole_\ic\_,aa1,aa10   ; once-delayed input coeff
             new_yarg pf\_twopole_\ic\_,aa2,aa20   ; twice-delayed input coeff
             new_yarg pf\_twopole_\ic\_,bb0,bb00   ; undelayed input coeff
             new_xarg pf\_twopole_\ic\_,aout,aout0 ; output address arg
             new_xarg pf\_twopole_\ic\_,ainp,ainp0 ; input address arg
             new_xarg pf\_twopole_\ic\_,s2,s20     ; twice delayed input samp

             move x:(R_X)+,R_O                 ; output address to R_O
             move R_X,R_I1                     ; save R_X
             move x:(R_X)+,R_I2                ; get input address
             move y:(R_Y)+,Y1  x:(R_X)+,X0     ; s2 to X0, s1 to Y1
             move #2,N_Y                       ; used for arg reset below
             move R_I2,R_X                     ; input address to R_X
             move y:(R_Y+N_Y),Y0               ; bb0 to Y0
             move x:(R_X)+,X1                  ; first input
                                               ; compute the first sample
             mpy        X1,Y0,A        y:(R_Y)+,Y0     ; bb0 * in, get aa1
             mac        -Y1,Y0,A       y:(R_Y)+,Y0     ; aa1 * s1, get aa2
             macr       -X0,Y0,A        Y1,X0          ; aa2 * s2, s1 to s2
             tfr         A,Y1          y:(R_Y)-N_Y,Y0              
                ; x(n) to s1. Get bb0 and reset R_Y to point to aa1
             move                      x:(R_I1)+,X1    ; next input 
             do #I_NTICK-1,pf\_twopole_\ic\_tickloop
                mpy     X1,Y0,A         A,Y1         A,x:(R_O)+ 
                ; input * bb0, s1 = x(n) deposit x(n)
                move                    y:(R_Y)+,Y0  x:(R_X)+,X1 
                ; aa1 to Y0, get input
                mac     -Y1,Y0,A        y:(R_Y)+,Y0   ; x(n-1) * aa1, aa2 to Y0
                macr    -X0,Y0,A        Y1,X0         ; x(n-2) * aa2, s2 = s1
                move                    y:(R_Y)-N_Y,Y0 ; reset R_Y and get bb0
pf\_twopole_\ic\_tickloop    
             move R_I1,R_X                       ; restore R_X
             move A,x:(R_O)                      ; ship last output
             move Y1,y:(R_Y)+    X0,x:(R_X)+     ; save s2, s1
             move y:(R_Y)+N_Y,Y0                 ; adjust R_Y
             endif
     endm


