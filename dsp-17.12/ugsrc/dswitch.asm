;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  07/05/88/jos - initial file created from mul2.asm
;;  08/22/88/jos - added dspace spec for arg 2 in DSPWRAP ARG INFO field
;;  09/12/88/jos - switch delay was not being saved in arg on exit - fixed.
;;  02/15/89/jos - removed (dspace) before i2adr0 in DSPWRAP ARGUMENT INFO line
;;  03/18/89/jos - changed name from delayedswitch to dswitch
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      dswitch (UG macro) - switch from input 1 to input 2 after delay
;;
;;  SYNOPSIS
;;      dswitch pf,ic,sout,aout0,sinp,i1adr0,scale10,i2adr0,switchdelay0
;;
;;  MACRO ARGUMENTS
;;      pf           = global label prefix (any text unique to invoking macro)
;;      ic           = instance count (pf\_dswitch_\ic\_ globally unique)
;;      sout         = output memory space ('x' or 'y')
;;      aout0        = output address in memory sout
;;      sinp         = input 1 or 2 memory space ('x' or 'y')
;;      i1adr0       = input address in memory sinp
;;      i2adr0       = input address in memory sinp
;;      scale10      = scale factor for input 1
;;      switchdelay0 = delay (in samples) to wait before passing input 2
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                    Initialization
;;      ----------      --------------                  --------------
;;      x:(R_X)+        address of input 1 signal       i1adr0
;;      y:(R_Y)+        address of input 2 signal       i2adr0
;;      y:(R_Y)+        address of output signal        aout0
;;      x:(R_X)+        scale factor for input 1        scale10
;;      y:(R_Y)+        switch delay in samples         switchdelay0
;;
;;  DESCRIPTION
;;      The dswitch unit-generator switches from input 1 (scaled) to a 
;;      input 2 (unscaled) after a delay specified in samples.  The delay
;;      can be interpreted as the number of samples input 1 is passed to 
;;      the output.  On each output sample, the delay is decremented by 1.
;;      Input 1 times the scale factor scale1 is passed to the output as long 
;;      as delay remains nonnegative. Afterwards, input 2 is passed 
;;      to the output with no scaling.
;;
;;      Note that input 2 can be the same as input 1 to provide a switching
;;      scale factor.
;;
;;  USAGE RESTRICTIONS
;;      Inputs 1 and 2 must reside in the same DSP memory space.
;;
;;  DSPWRAP ARGUMENT INFO
;;      dswitch (prefix)pf,(instance)ic,
;;          (dspace)sout,(output)aout,
;;          (dspace)sinp,(input)i1adr,scale1,(input)i2adr,switchdelay
;;
;;  MAXIMUM EXECUTION TIME
;;      158 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      126 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/dswitch.asm
;;
;;  ALU REGISTER USE
;;       X0  = input signal 1 if input space is X
;;       Y0  = input signal 1 if input space is Y
;;       X1  = amplitude scale factor for input 1
;;       Y0  = input signal 2 (not scaled)
;;       Y1  = constant 1 for decrementing switch delay in B
;;       A   = output signal
;;       B   = remaining time delay for switch-over (in samples)
;;
dswitch  macro pf,ic,sout,aout0,sinp,i1adr0,scale10,i2adr0,switchdelay0

        new_xarg pf\_dswitch_\ic\_,i1adr,i1adr0
        new_yarg pf\_dswitch_\ic\_,i2adr,i2adr0
        new_yarg pf\_dswitch_\ic\_,aout,aout0
        new_xarg pf\_dswitch_\ic\_,scale1,scale10
        new_yarg pf\_dswitch_\ic\_,switchdelay,switchdelay0

        move x:(R_X)+,R_I1      ; input 1 address to R_I1
        move y:(R_Y)+,R_I2      ; input 2 address to R_I2
        move y:(R_Y)+,R_O       ; output address to R_O (R_O needed first)
        move x:(R_X)+,X1 y:(R_Y),B ; scale factor, switching delay
        move #>1,Y1             ; decrement for B
        move sout:-(R_O),A      ; make first output valid
        if "sinp"=='x'
                define pf\_dswitch_\ic\_input 'X0'
        else
                define pf\_dswitch_\ic\_input 'Y0'
        endif
        move sinp:(R_I1)+,pf\_dswitch_\ic\_input            ; fetch input 1
        do #I_NTICK,pf\_dswitch_\ic\_tickloop
          if "sinp"=="sout"
            mpyr pf\_dswitch_\ic\_input,X1,A  A,sout:(R_O)+ ; scale in 1, ship
            move sinp:(R_I1)+,pf\_dswitch_\ic\_input        ; fetch input 1
          else
            mpyr pf\_dswitch_\ic\_input,X1,A sinp:(R_I1)+,pf\_dswitch_\ic\_input A,sout:(R_O)+
          endif
          sub Y1,B  sinp:(R_I2)+,Y0     ; decrement timer, fetch input 2
          tlt Y0,A 
pf\_dswitch_\ic\_tickloop
        move A,sout:(R_O)+      ; last output
        tst B #0,X0             ; remaining count
        tle X0,B                ; don't let it reach -infinity
        move B,y:(R_Y)+         ; store for next time
      endm


