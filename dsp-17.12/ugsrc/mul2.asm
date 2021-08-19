;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  03/16/88/jos - initial file created from add2.asm
;;  03/30/89/jos - changed to cleaner pfx syntax and changed tabs for troff
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      mul2 (UG macro) - multiply two signals to produce a third
;;
;;  SYNOPSIS
;;      mul2 pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_mul2_\ic\_ is globally unique)
;;      sout      = output memory space ('x' or 'y')
;;      aout0     = initial output address in memory sout
;;      i1spc     = input 1 memory space ('x' or 'y')
;;      i1adr0    = initial input address in memory i1spc
;;      i2spc     = input 2 memory space ('x' or 'y')
;;      i2adr0    = initial input address in memory i2spc
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                 Initialization
;;      ----------      --------------               --------------
;;      x:(R_X)+        address of input 1 signal    i1adr0
;;      y:(R_Y)+        address of input 2 signal    i2adr0
;;      y:(R_Y)+        address of output signal     aout0
;;
;;  DESCRIPTION
;;      The mul2 unit-generator simply multiplies two signal vectors into a
;;      third.  The output vector can be the same as an input vector.
;;         
;;  DSPWRAP ARGUMENT INFO
;;      mul2 (prefix)pf,(instance)ic,
;;           (dspace)sout,(output)aout,(dspace)i1spc,(input)i1adr,
;;           (dspace)i2spc,(input)i2adr
;;
;;  MAXIMUM EXECUTION TIME
;;      112 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      80 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/mul2.asm
;;
;;  ALU REGISTER USE
;;       X0  = input 1
;;       Y0  = input 2
;;       A   = product
;;
      define mul2_pfx "pf\_mul2_\ic\_" ; pf = <name>_pfx of invoker
      define mul2_pfxm """mul2_pfx"""
mul2  macro pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0
        new_xarg mul2_pfxm,i1adr,i1adr0
        new_yarg mul2_pfxm,i2adr,i2adr0
        new_yarg mul2_pfxm,aout,aout0
        move y:(R_Y)+,R_I2      ; input 2 address to R_I2
        move y:(R_Y)+,R_O       ; output address to R_O (R_O needed now)
        move x:(R_X)+,R_I1      ; input 1 address to R_I1
        move sout:-(R_O),A      ; make first output valid
        do #I_NTICK,mul2_pfx\tickloop
          if "i1spc"=="i2spc"
                  move i1spc:(R_I1)+,X0
                  move i2spc:(R_I2)+,Y0
          else
                if "i1spc"=='x'
                  move i1spc:(R_I1)+,X0 i2spc:(R_I2)+,Y0
                else
                  move i2spc:(R_I2)+,X0 i1spc:(R_I1)+,Y0
                endif
          endif
          mpy X0,Y0,A    A,sout:(R_O)+

; Note: There is also the case "i1spc"=="i2spc"!="sout" which could
; be overlapped in a way we are not catching.

mul2_pfx\tickloop
        move A,sout:(R_O)+      ; last output
      endm


