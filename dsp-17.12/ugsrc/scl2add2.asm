;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  06/16/89/mm - initial file created from scl1add2.asm; 

;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      scl2add2 (UG macro) - add two signals each scaled by a constant
;;
;;  SYNOPSIS
;;      scl2add2 pf,ic,sout,aout0,i1spc,i1adr0,i1gin0,i2spc,i2adr0,i2gin0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_add2_\ic\_ is globally unique)
;;      sout      = output memory space ('x' or 'y')
;;      aout0     = initial output address in memory sout
;;      i1spc     = input 1 memory space ('x' or 'y')
;;      i1adr0    = initial input address in memory i1spc
;;      i1gin0    = gain factor for input 1 [-1.0 to 1.0]
;;      i2spc     = input 2 memory space ('x' or 'y')
;;      i2adr0    = initial input address in memory i2spc
;;      i2gin0    = gain factor for input 2 [-1.0 to 1.0]
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                 Initialization
;;      ----------      --------------               --------------
;;      x:(R_X)+        address of input 1 signal    i1adr0
;;      x:(R_X)+        address of input 1 scaler    i1gin0
;;      y:(R_Y)+        address of input 2 signal    i2adr0
;;      x:(R_Y)+        address of input 2 scaler    i2gin0
;;      y:(R_Y)+        address of output signal     aout0
;;
;;  DESCRIPTION
;;      The scl2add2 unit-generator multiplies two input signals
;;      times constant scalers then adds them together to produce a
;;      third.  The output vector can be the same as an input vector.
;;      Inner loop is two instructions if space of input1 is "x" and
;;      space of input2 is "y", otherwise three instructions.
;;         
;;  DSPWRAP ARGUMENT INFO
;;      scl2add2 (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;      (dspace)i1spc,(input)i1adr,i1gin,(dspace)i2spc,(input)i2adr,i2gin
;;
;;  MAXIMUM EXECUTION TIME
;;      114 DSP clock cycles for one "tick" which equals sixteen audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;       82 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/scl2add2.asm
;;
;;  TEST PROGRAM
;;
;;  ALU REGISTER USE
;;       X0  = input 1
;;       Y0  = input 2
;;       X1  = input 1 scaler
;;       Y1  = input 2 scaler
;;       A   = sum
;;
        define scl2add2_pfx "pf\_scl2add2_\ic\_"        ; pf = <name>_pfx of invoker
        define scl2add2_pfxm """scl2add2_pfx"""
scl2add2 macro pf,ic,sout,aout0,i1spc,i1adr0,i1gin0,i2spc,i2adr0,i2gin0
        new_xarg scl2add2_pfxm,i1adr,i1adr0     ; allocate x memory argument
        new_xarg scl2add2_pfxm,i1gin,i1gin0     ; allocate x memory argument
        new_yarg scl2add2_pfxm,i2adr,i2adr0     ; allocate y memory argument
        new_yarg scl2add2_pfxm,i2gin,i2gin0     ; allocate x memory argument
        new_yarg scl2add2_pfxm,aout,aout0       ; allocate y memory argument
        move x:(R_X)+,R_I1                      ; input 1 address to R_I1
        move y:(R_Y)+,R_I2                      ; input 2 address to R_I2
        move x:(R_X)+,X1  y:(R_Y)+,Y1           ; scalers
        move y:(R_Y)+,R_O                       ; output address to R_O

        move i1spc:(R_I1)+,X0
        mpy  X0,X1,A  i2spc:(R_I2)+,Y0

        do #I_NTICK,scl2add2_pfx\tickloop
          if "i1spc"=='x'&&"i2spc"=='y'
            macr Y0,Y1,A  i1spc:(R_I1)+,X0 i2spc:(R_I2)+,Y0
          else
            macr Y0,Y1,A  i1spc:(R_I1)+,X0 
            move i2spc:(R_I2)+,Y0
          endif
          mpy  X0,X1,A  A,sout:(R_O)+
scl2add2_pfx\tickloop
        endm


