;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  06/07/89/mm - initial file created from add2.asm; old version is scl1add2.asm.old
;;                Only 1 out-of-loop instruction more than add2.asm!!
;;      
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      scl1add2 (UG macro) - add scaler times first signal to the second
;;
;;  SYNOPSIS
;;      scl1add2 pf,ic,sout,aout0,i1spc,i1adr0,i1gin0,i2spc,i2adr0
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
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                 Initialization
;;      ----------      --------------               --------------
;;      x:(R_X)+        address of input 1 signal    i1adr0
;;      x:(R_X)+        address of input 1 scaler    i1gin0
;;      y:(R_Y)+        address of input 2 signal    i2adr0
;;      y:(R_Y)+        address of output signal     aout0
;;
;;  DESCRIPTION
;;      The scl1add2 unit-generator multiplies the first input by a
;;      scale factor, and adds it to the second input signal to produce a
;;      third.  The output vector can be the same as an input vector.
;;      Faster if space of input1 is not the same as the space of intput2
;;         
;;  DSPWRAP ARGUMENT INFO
;;      scl1add2 (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;      (dspace)i1spc,(input)i1adr,i1gin,(dspace)i2spc,(input)i2adr
;;
;;  MAXIMUM EXECUTION TIME
;;      114 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      80 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/scl1add2.asm
;;
;;  TEST PROGRAM
;;
;;  ALU REGISTER USE
;;       X0  = input 1
;;       Y0  = input 2
;;       Y1  = input 1 scaler
;;       A   = sum
;;
        define scl1add2_pfx "pf\_scl1add2_\ic\_" ; pf = <name>_pfx of invoker
        define scl1add2_pfxm """scl1add2_pfx"""
scl1add2 macro pf,ic,sout,aout0,i1spc,i1adr0,i1gin0,i2spc,i2adr0
        new_xarg scl1add2_pfxm,i1adr,i1adr0     ; allocate x memory argument
        new_xarg scl1add2_pfxm,i1gin,i1gin0     ; allocate x memory argument
        new_yarg scl1add2_pfxm,i2adr,i2adr0     ; allocate y memory argument
        new_yarg scl1add2_pfxm,aout,aout0       ; allocate y memory argument
        move x:(R_X)+,R_I1                      ; input 1 address to R_I1
        move x:(R_X)+,Y1                        ; input 1 address to R_I1
        move y:(R_Y)+,R_I2                      ; input 2 address to R_I2
        move y:(R_Y)+,R_O                       ; output address to R_O

        if "i1spc"=='x'&&"i2spc"=='y'
          move i1spc:(R_I1)+,X0  i2spc:(R_I2)+,A
          endif
        if "i1spc"=='y'&&"i2spc"=='x'
          move i1spc:(R_I1)+,Y0  i2spc:(R_I2)+,A
          endif
        if "i1spc"=="i2spc"
          move i1spc:(R_I1)+,Y0  
          move i2spc:(R_I2)+,A
          endif

        do #I_NTICK,scl1add2_pfx\tickloop
            if "i1spc"=='x'&&"i2spc"=='y'
                macr X0,Y1,A  i1spc:(R_I1)+,X0 i2spc:(R_I2)+,Y0
                tfr Y0,A  A,sout:(R_O)+
                endif
            if "i1spc"=='y'&&"i2spc"=='x'
                macr Y0,Y1,A  i1spc:(R_I1)+,Y0 i2spc:(R_I2)+,X0
                tfr X0,A  A,sout:(R_O)+
                endif
            if "i1spc"=="i2spc"
                macr Y0,Y1,A  i1spc:(R_I1)+,Y0 
                move i2spc:(R_I2)+,X0
                tfr X0,A  A,sout:(R_O)+
                endif   
scl1add2_pfx\tickloop
        endm


