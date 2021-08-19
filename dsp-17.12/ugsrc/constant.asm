;;  Copyright 1989 by NeXT Inc.
;;  Author - J. O. Smith
;;
;;  Modification history
;;  --------------------
;;  5/24/87/jos - initial file created from /usr/lib/dsp/ugsrc/template.doc
;;  5/24/87/jos - passed test/tonezero.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      constant (UG macro) - generate a constant signal
;;
;;  SYNOPSIS
;;      constant pf,ic,sout,aout0,cnst0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_constant_\ic\_ is globally unique)
;;      sout      = output vector memory space ('x' or 'y')
;;      aout0     = initial output vector memory address
;;      cnst0     = initial constant value
;;
;;  DSP MEMORY ARGUMENTS
;;      Access         Description              Initialization
;;      ------         -----------              --------------
;;      x:(R_X)+       Current output address   aout0
;;      y:(R_Y)+       Current constant value   cnst0
;;
;;  DESCRIPTION
;;      The constant unit-generator write a constant into a signal vector.
;;      In pseudo-C notation:
;;
;;      aout = x:(R_X)+;
;;      cnst = y:(R_Y)+;
;;      for (n=0;n<I_NTICK;n++) sout:aout[n] = cnst;
;;        
;;  DSPWRAP ARGUMENT INFO
;;      constant (prefix)pf,(instance)ic,(dspace)sout,(output)aout,cnst
;;
;;  MAXIMUM EXECUTION TIME
;;      42 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      42 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/constant.asm
;;
;;  ALU REGISTER USE
;;      X0 = Current constant
;;
constant  macro pf,ic,sout,aout0,cnst0
               new_xarg pf\_constant_\ic\_,aout,aout0  ; output address arg
               new_yarg pf\_constant_\ic\_,cnst,cnst0
               move x:(R_X)+,R_O                  ; output address to R_O
               move y:(R_Y)+,X0                   ; constant to X0
               do #I_NTICK,pf\_constant_\ic\_tickloop
                    move X0,sout:(R_O)+
pf\_constant_\ic\_tickloop    
     endm


