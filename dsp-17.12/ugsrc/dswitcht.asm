;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  03/20/89/jos - File created from dswitch.asm
;;  08/11/89/jos - Fixed bug in initialization of input before loop
;;  08/11/89/jos - Switched to cleaner global prefix syntax
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      dswitcht (UG macro) - switch from input 1 to input 2 after nticks delay
;;
;;  SYNOPSIS
;;      dswitcht pf,ic,sout,aout0,sinp,i1adr0,scale10,i2adr0,scale20,tickdelay0
;;
;;  MACRO ARGUMENTS
;;      pf           = global label prefix (any text unique to invoking macro)
;;      ic           = instance count (pf\_dswitcht_\ic\_ globally unique)
;;      sout         = output memory space ('x' or 'y')
;;      aout0        = output address in memory sout
;;      sinp         = input 1 or 2 memory space ('x' or 'y')
;;      i1adr0       = input address in memory sinp
;;      scale10      = scale factor for input 1
;;      i2adr0       = input address in memory sinp
;;      scale20      = scale factor for input 2
;;      tickdelay0   = delay (in ticks) to wait before passing input 2
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                    Initialization
;;      ----------      --------------                  --------------
;;      x:(R_X)+        scale factor for input 1        scale10
;;      x:(R_X)+        scale factor for input 2        scale20
;;      x:(R_X)+        address of input 1 signal       i1adr0
;;
;;      y:(R_Y)+        switch delay in ticks           tickdelay0
;;      y:(R_Y)+        address of output signal        aout0
;;      y:(R_Y)+        address of input 2 signal       i2adr0
;;
;;  DESCRIPTION
;;      The dswitcht unit-generator switches from input 1 to a 
;;      input 2 after a delay specified in ticks.  One tick
;;      equals the number of samples computed on one pass of the orchestra
;;      loop.  Except for having its delay specified in ticks (rather than
;;      samples), and except for having a scale factor for input 2,
;;      dswitcht is equivalent to the dswitch unit generator.  Specifying
;;      switching time in ticks makes dswitcht much more efficient
;;      than dswitch, but less general.
;;
;;  USAGE RESTRICTIONS
;;      Inputs 1 and 2 must reside in the same DSP memory space.
;;      The switching time is quantized to multiples of a tick period.
;;
;;  DSPWRAP ARGUMENT INFO
;;      dswitcht (prefix)pf,(instance)ic,
;;          (dspace)sout,(output)aout,
;;          (dspace)sinp,(input)i1adr,scale1,(input)i2adr,scale2,tickdelay
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/dswitcht.asm
;;
;;  TEST PROGRAM
;;
;;  ALU REGISTER USE (INNER LOOP)
;;       X0  = input signal 1 if input space is X
;;       Y0  = input signal 1 if input space is Y
;;       X1  = input scale factor
;;       A   = output signal
;;
      define dswttp "pf\_dswitcht_\ic\_" ; pf = <name>_pfx of invoker
      define dswttpm """dswttp"""
dswitcht  macro pf,ic,sout,aout0,sinp,i1adr0,scale10,i2adr0,scale20,tickdelay0

        new_xarg dswttp,scale1,scale10
        new_xarg dswttp,scale2,scale20
        new_xarg dswttp,i1adr,i1adr0

        new_yarg dswttp,tickdelay,tickdelay0
        new_yarg dswttp,aout,aout0
        new_yarg dswttp,i2adr,i2adr0

        move x:(R_X)+,A y:(R_Y),B ; scale factor 1, switching delay
        move #>1,Y1             ; decrement for B
        sub Y1,B x:(R_X)+,X0    ; decrement timer, load scale 2
        tlt X0,A                ; select scale
        clr A A,X1              ; install scale
        tst B                   ; clr flushes our condition codes
        tlt A,B                 ; reset timer to 0 if negative
        move B,y:(R_Y)+         ; save timer state
        move y:(R_Y)+,R_O       ; get output address
        move x:(R_X)+,A y:(R_Y)+,Y0 ; i1 adr to A, i2 adr to Y0
        tlt Y0,A                ; select inp addr, get output address
        move A,R_I1             ; install input address
        move sout:-(R_O),A      ; make first output valid
        if "sinp"=='x'
                define dswttp\input 'X0'
        else
                define dswttp\input 'Y0'
        endif
        if "sinp"!="sout"
            move sinp:(R_I1)+,dswttp\input        ; fetch input 1
        endif
        do #I_NTICK+1,dswttp\tickloop
          if "sinp"=="sout"
            move sinp:(R_I1)+,dswttp\input        ; fetch input 1
            mpyr dswttp\input,X1,A  A,sout:(R_O)+ ; scale input 1, ship output
          else
            mpyr dswttp\input,X1,A sinp:(R_I1)+,dswttp\input A,sout:(R_O)+
          endif
dswttp\tickloop
      endm


