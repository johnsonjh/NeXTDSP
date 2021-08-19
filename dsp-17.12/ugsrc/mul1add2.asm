;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  06/13/89/mm - initial file created from add3.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      mul1add2 (UG macro) - multiply two signals and add1 to produce a fourth
;;
;;  SYNOPSIS
;;      mul1add2 pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0,i3spc,i3adr0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_mul1add2_\ic\_ is globally unique)
;;      sout      = output memory space ('x' or 'y')
;;      aout0     = initial output address in memory sout
;;      i1spc     = input 1 memory space ('x' or 'y')
;;      i1adr0    = initial input address in memory i1spc
;;      i2spc     = input 2 memory space ('x' or 'y')
;;      i2adr0    = initial input address in memory i2spc
;;      i3spc     = input 3 memory space ('x' or 'y')
;;      i3adr0    = initial input address in memory i3spc
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                 Initialization
;;      ----------      --------------               --------------
;;      x:(R_X)+        address of input 1 signal    i1adr0
;;      x:(R_X)+        address of input 2 signal    i2adr0
;;      y:(R_Y)+        address of input 3 signal    i3adr0
;;      y:(R_Y)+        address of output signal     aout0
;;
;;  DESCRIPTION
;;      The mul1add2 unit-generator outputs one signal vector added to the 
;;      product of two others, i.e.,
;;              out = in1 + (in2 * in3)
;;      The output vector can be the same as an input vector.
;;      The number of instructions is:
;;              spaces:                 # instructions:
;;          out  in1  in2  in3
;;           y    x    y    x           2
;;           *    *    y    x           3
;;           y    x    *    *           3
;;              all others              4
;;
;;  DSPWRAP ARGUMENT INFO
;;      mul1add2 (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;              (dspace)i1spc,(input)i1adr,(dspace)i2spc,(input)i2adr,
;;              (dspace)i3spc,(input)i3adr
;;
;;  MAXIMUM EXECUTION TIME
;;      154 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      88 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/mul1add2.asm
;;
;;  ALU REGISTER USE
;;       A  = input 1 and output
;;       Y0 = input 2
;;       X0 = input 3
;;       X1 = saves register R_X
;;
        define mul1add2_pfx "pf\_mul1add2_\ic\_"        ; pf = <name>_pfx of invoker
        define mul1add2_pfxm """mul1add2_pfx"""
mul1add2 macro pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0,i3spc,i3adr0
        new_xarg mul1add2_pfxm,i1adr,i1adr0     ; allocate x memory argument
        new_xarg mul1add2_pfxm,i2adr,i2adr0     ; allocate x memory argument
        new_yarg mul1add2_pfxm,i3adr,i3adr0     ; allocate y memory argument
        new_yarg mul1add2_pfxm,aout,aout0       ; allocate y memory argument
        move x:(R_X)+,R_I1                      ; input 1 address to R_I1
        move x:(R_X)+,R_I2                      ; input 2 address to R_I2
        move R_X,X1                             ; save R_X
        move y:(R_Y)+,R_X                       ; input 3 address to R_X
        move y:(R_Y)+,R_O                       ; output address to R_O

        move i1spc:(R_I1)+,A                    ; load input 1 to A
        if "i2spc"=='y'&&"i3spc"=='x'
          move i2spc:(R_I2)+,Y0 i3spc:(R_X)+,X0 ; load inputs 2 and 3
        else
          move i2spc:(R_I2)+,Y0
          move i3spc:(R_X)+,X0
        endif

        do #I_NTICK,mul1add2_pfx\tickloop       ; enter do loop
          if "i2spc"=='y'&&"i3spc"=='x'
            macr X0,Y0,A  i2spc:(R_I2)+,Y0 i3spc:(R_X)+,X0
          else
            macr X0,Y0,A  i2spc:(R_I2)+,Y0
            move i3spc:(R_X)+,X0
          endif
          if "sout"=='y'&&"i1spc"=='x'
            move A,sout:(R_O)+  i1spc:(R_I1)+,A
          else
            move A,sout:(R_O)+
            move i1spc:(R_I1)+,A
          endif
mul1add2_pfx\tickloop

        move X1,R_X                              ; restore R_X
        nop
        endm

; Note: There are other three and two instruction cases which we are not supporting


