;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  09/30/88/jos - created from slpdur.asm
;;-----------------------------------------------------------------------------
;;  NAME
;;      ramp (UG macro) - Generate an output signal having constant slope
;;
;;  SYNOPSIS
;;      ramp pf,ic,sout,aout0,val0h,val0l,slp0h,slp0l
;;
;;  MACRO ARGUMENTS
;;      pf    = global label prefix (any text unique to invoking macro)
;;      ic    = instance count (such that pf\_ramp_\ic\_ is globally unique)
;;      sout  = output envelope memory space ('x' or 'y')
;;      aout0 = initial output address in memory sout (0 to $FFFF)
;;      val0h = Initial value, high-order word ($800000 to $7FFFFF)
;;      val0l = Initial value, low-order word ($800000 to $7FFFFF)
;;      slp0h  = Slope value, high-order word
;;      slp0l  = Slope value, low-order word
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     ------------             --------------
;;      x:(R_X)+       Current output address   aout0
;;      l:(R_L)P_L     Current slope            (slp0h<<24)|slp0l
;;      l:(R_L)P_L     Current value            (val0h<<24)|val0l
;;
;;  DESCRIPTION
;;      The ramp unit-generator computes a ramp signal in DSP memory.
;;      It can be used as one segment of a piecewise-linear envelope.
;;      The ramp is computed in double precision (48 bits), but the 
;;      output is the high-order 24 bits of the 48-bit current value.
;;      The ramp is computed by simply adding the slope argument each sample.
;;
;;  DSPWRAP ARGUMENT INFO
;;      ramp (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;           valh,vall,slph,slpl
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/ramp.asm
;;
;;  TEST PROGRAM
;;
;;  ALU REGISTER USE
;;          X = current slope 
;;          A = current ramp value
;;
ramp    macro pf,ic,sout,aout0,val0h,val0l,slp0h,slp0l
        new_xarg pf\_ramp_\ic\_,aout,aout0       ; output address
        new_larg pf\_ramp_\ic\_,slp,slp0h,slp0l  ; ramp slope
        new_larg pf\_ramp_\ic\_,val,val0h,val0l  ; ramp state
        
        move x:(R_X)+,R_O               ; output pointer
        move l:(R_L)P_L,X               ; current slope
        move l:(R_L),A                  ; current ramp state, increment below
        rep #I_NTICK                    ; compute ramp
          add X,A A,sout:(R_O)+         ; inner loop
        move A,l:(R_L)P_L               ; save ramp state for next tick
        endm   ; ramp


