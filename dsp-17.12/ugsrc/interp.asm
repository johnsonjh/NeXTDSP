;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  06/13/89/mm - initial file created from add3.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      interp (UG macro) - dynamically interpolate between two signals
;;
;;  SYNOPSIS
;;      interp pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0,i3spc,i3adr0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_interp_\ic\_ is globally unique)
;;      sout      = output memory space ('x' or 'y')
;;      aout0     = initial output address in memory sout
;;      i1spc     = input 1 memory space ('x' or 'y')
;;      i1adr0    = initial input address in memory i1spc
;;      i2spc     = input 2 memory space ('x' or 'y')
;;      i2adr0    = initial input address in memory i2spc
;;      i3spc     = interpolation input memory space ('x' or 'y')
;;      i3adr0    = initial input address in memory i3spc
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                 Initialization
;;      ----------      --------------               --------------
;;      x:(R_X)+        address of input 1 signal    i1adr0
;;      y:(R_Y)+        address of input 2 signal    i2adr0
;;      x:(R_X)+        address of interp signal     i3adr0
;;      y:(R_Y)+        address of output signal     aout0
;;
;;  DESCRIPTION
;;      The interp unit-generator interpolates between two signals.  The
;;      output is the first signal plus the difference signal times the 
;;      interpolation signal.
;;      The output vector can be the same as an input vector.
;;      The inner loop is three instructions if the memory spaces
;;      for in1 and in2 are x and y, respectively, otherwise it is four
;;      instructions.
;;         
;;  DSPWRAP ARGUMENT INFO
;;      interp (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;              (dspace)i1spc,(input)i1adr,(dspace)i2spc,(input)i2adr,
;;              (dspace)i3spc,(input)i3adr
;;
;;  MAXIMUM EXECUTION TIME
;;      156 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      122 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/interp.asm
;;
;;  ALU REGISTER USE
;;       X0  = input 1
;;       A   = input 2 and sum
;;
        define interp_pfx "pf\_interp_\ic\_"    ; pf = <name>_pfx of invoker
        define interp_pfxm """interp_pfx"""
interp macro pf,ic,sout,aout0,i1spc,i1adr0,i2spc,i2adr0,i3spc,i3adr0
        new_xarg interp_pfxm,i1adr,i1adr0       ; allocate x memory argument
        new_yarg interp_pfxm,i2adr,i2adr0       ; allocate y memory argument
        new_xarg interp_pfxm,i3adr,i3adr0       ; allocate x memory argument
        new_yarg interp_pfxm,aout,aout0         ; allocate y memory argument
        move x:(R_X)+,R_I1                      ; input 1 address to R_I1
        move y:(R_Y)+,R_I2                      ; input 2 address to R_I2
        move x:(R_X)+,X0                        ; X0 = control address
        move y:(R_Y)+,R_O                       ; output address to R_O

        if "i1spc"=='x'&&"i2spc"=='y'
          move i1spc:(R_I1)+,X1  i2spc:(R_I2)+,B  ; get the two input signals
        else
          move i1spc:(R_I1)+,X1  
          move i2spc:(R_I2)+,B                  ; get the two input signals
        endif

        if "i3spc"=='y'
          move R_Y,Y1                           ; save R_Y in Y1
          sub  X1,B   X0,R_Y                    ; B = diff, R_Y = control address
        else
          move R_X,Y1                           ; save R_X in Y1
          sub  X1,B   X0,R_X                    ; B = diff, R_X = control address
        endif
        nop                                     ; damn
        do #I_NTICK,interp_pfx\tickloop         ; enter do loop
          if "i3spc"=='y'
            tfr  X1,A  B,X0  i3spc:(R_Y)+,Y0    ; A = 1st sig, X0 = diff, Y0 = control
          else
            tfr  X1,A  B,Y0  i3spc:(R_X)+,X0    ; A = 1st sig, Y0 = diff, X0 = control
          endif
          if "i1spc"=='x'&&"i2spc"=='y'
            macr X0,Y0,A  i1spc:(R_I1)+,X1  i2spc:(R_I2)+,B
          else
            macr X0,Y0,A  i1spc:(R_I1)+,X1
            move i2spc:(R_I2)+,B
          endif
          sub  X1,B  A,sout:(R_O)+
interp_pfx\tickloop

        if "i3spc"=='y'
          move Y1,R_Y                           ; restore R_Y
        else
          move Y1,R_X                           ; restore R_X
        endif
        nop                                     ; for safety
        endm

; Note: There are other three-instruction cases which we are not supporting


