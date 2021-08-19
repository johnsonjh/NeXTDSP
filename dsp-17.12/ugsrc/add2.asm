;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  10/19/87/jos - initial file created from scale.asm
;;  03/16/88/jos - optimized inner loop (still not tested)
;;  03/30/89/jos - changed to cleaner pfx syntax and changed tabs for troff
;;  04/17/89/jos - saved one instruction in pre-loop setup in optimized case
;;  04/25/89/mmm - changed optimized case to "i1spc"=='x'&&"i2spc"=='y'
;;              (better to have flexibility of output space)
;;  03/16/90/jos - added (input) and (output) qualifiers to ARGUMENT INFO
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      add2 (UG macro) - add two signals to produce a third
;;
;;  SYNOPSIS
;;      add2 pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_add2_\ic\_ is globally unique)
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
;;      The add2 unit-generator simply sums two signal vectors into a
;;      third.  The output vector can be the same as an input vector.
;;      The inner loop is two instructions if the memory spaces
;;      for in1 and in2 are x and y, respectively. In all other cases the
;;      inner loop is three instructions.
;;         
;;  DSPWRAP ARGUMENT INFO
;;      add2 (prefix)pf,(instance)ic,
;;           (dspace)sout,(output)aout,
;;           (dspace)i1spc,(input)i1adr,
;;           (dspace)i2spc,(input)i2adr
;;
;;  MAXIMUM EXECUTION TIME
;;      112 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      78 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/add2.asm
;;
;;  ALU REGISTER USE
;;       X0  = input 1
;;       A   = input 2 and sum
;;
        define add2_pfx "pf\_add2_\ic\_"        ; pf = <name>_pfx of invoker
        define add2_pfxm """add2_pfx"""
add2    macro pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0
        new_xarg add2_pfxm,i1adr,i1adr0         ; allocate x memory argument
        new_yarg add2_pfxm,i2adr,i2adr0         ; allocate y memory argument
        new_yarg add2_pfxm,aout,aout0           ; allocate y memory argument
        move x:(R_X)+,R_I1                      ; input 1 address to R_I1
        move y:(R_Y)+,R_I2                      ; input 2 address to R_I2
        move y:(R_Y)+,R_O                       ; output address to R_O

        if "i1spc"=='x'&&"i2spc"=='y'
          move i1spc:(R_I1)+,X0  i2spc:(R_I2)+,A
        else
          move i1spc:(R_I1)+,X0                 ; load input 1 to X0
          move i2spc:(R_I2)+,A                  ; load input 2 to A
        endif
        do #I_NTICK,add2_pfx\tickloop           ; enter do loop
                if "i1spc"=='x'&&"i2spc"=='y'
                  add X0,A  i1spc:(R_I1)+,X0 i2spc:(R_I2)+,Y0
                else
                  add X0,A  i1spc:(R_I1)+,X0
                  move i2spc:(R_I2)+,Y0
                endif             
                tfr Y0,A  A,sout:(R_O)+
add2_pfx\tickloop
        endm

; Note: There are other two-instruction cases which we are not supporting



