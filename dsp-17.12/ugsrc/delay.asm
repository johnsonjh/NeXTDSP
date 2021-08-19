;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  08/01/87/jos - initial file created from /usr/lib/dsp/ugsrc/delay.asm
;;  08/01/87/jos - changed delay.asm to delaym.asm so this one could be delay.
;;  08/03/87/jos - passed test/tdelay.asm
;;  10/05/87/jos - passed test/tdelay.asm, absolute and relative cases.
;;  10/11/87/jos - changed pointer-wrapping test from "teq" to "tge"
;;                   so that end-pointer "edel" can be off slightly.
;;  10/15/87/jos - changed current version to delayticks.asm and rewrote
;;                   to support arbitrary numbers of samples delay.
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      delay (UG macro) - sample-based delay line using non-modulo indexing
;;
;;  SYNOPSIS
;;      delay pf,ic,sout,aout0,sinp,ainp0,sdel,adel0,pdel0,edel0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_delay_\ic\_ is globally unique)
;;      sout      = output vector memory space ('x' or 'y')
;;      aout0     = initial output vector memory address
;;      sinp      = input vector memory space ('x' or 'y')
;;      ainp0     = initial input vector memory address
;;      sdel      = delay-line memory space ('x' or 'y')
;;      adel0     = delay-line start address
;;      pdel0     = delay-line pointer
;;      edel0     = address of first sample beyond delay line
;;
;;  DSP MEMORY ARGUMENTS
;;      Access         Description              Initialization
;;      ------         -----------              --------------
;;      x:(R_X)+       Output address           aout0
;;      x:(R_X)+       Input address            ainp0
;;      x:(R_X)+       Delay pointer            pdel0
;;      y:(R_Y)+       Last address + 1         edel0
;;      y:(R_Y)+       Start address            adel0   
;;
;;  DESCRIPTION
;;
;;      The delay unit-generator implements a simple delay line using a circular
;;      buffer (not modulo storage).  It is less efficient than the delaym unit
;;      generator but is easier to use because the initial address of the delay
;;      line memory is unconstrained.
;;
;;      For best performance, the input and output signals should be in the same
;;      memory space, which should be different from the delay-line memory space.
;;      I.e., if the delay table is in x memory, both input and output should be 
;;      in y memory, or vice versa.
;;      
;;      In pseudo-C notation:
;;
;;      aout = x:(R_X)+;
;;      ainp = x:(R_X)+;
;;      pdel = x:(R_X)+;
;;      edel = y:(R_Y)+;
;;      adel = y:(R_Y)+;
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           sout:aout[n] = sdel:pdel[n];
;;           sdel:pdel[n] = sinp:ainp[n];
;;           if (++pdel>=edel) pdel=adel;
;;      }
;;
;;  DSPWRAP ARGUMENT INFO
;;      delay (prefix)pf,(instance)ic,
;;         (dspace)sout,(output)aout,
;;         (dspace)sinp,(input)ainp,
;;         (dspace)sdel,(address)adel,(address)pdel,(address)edel
;;
;;  MAXIMUM EXECUTION TIME
;;      280 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      248 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/delay.asm
;;
;;  ALU REGISTER USE
;;      Y1 = start address of delay line
;;      Y0 = last address of delay line
;;       A = temporary register for input signal
;;       B = temporary register for output signal
;;
delay     macro pf,ic,sout,aout0,sinp,ainp0,sdel,adel0,pdel0,edel0
               new_xarg pf\_delay_\ic\_,aout,aout0   ; output address arg
               new_xarg pf\_delay_\ic\_,ainp,ainp0   ; input address arg
               new_xarg pf\_delay_\ic\_,pdel,pdel0   ; current pointer arg
               new_yarg pf\_delay_\ic\_,edel,edel0   ; last-address+1 arg
               new_yarg pf\_delay_\ic\_,adel,adel0   ; start-address arg

               move x:(R_X)+,R_O        ; output address to R_O
               move x:(R_X)+,R_I1       ; input address to R_I1
               move x:(R_X),R_I2        ; delay pointer to R_I2, update on exit
               remember 'move one of following to x mem arg for parallel load'
               move y:(R_Y)+,Y0         ; delay pointer when out of bounds
               move y:(R_Y)+,Y1         ; start adr to Y1 
;;
;;  The conditional compilation below falls into four categories depending
;;  on the sequence sinp,sdel,sout:
;;
;;         (1) xyx | yxy   (fastest case)
;;         (2) xxx | yyy   (slowest case)
;;         (3) xxy | yyx
;;         (4) xyy | yxx
;;
;;  The above numbers appear in the line comments below to aid in thinking 
;;  through each case.
;;
               if "sout"!="sdel"        ; use R_L instead of R_I2 for dly ptr
                    move R_L,N_I2       ; (1) (3) save R_L in an unused place
               endif

               move R_I2,A              ; current delay pointer expected in A
               do #I_NTICK,pf\_delay_\ic\_tickloop
                    cmp Y0,A            ; check against last address + 1
                    tge Y1,A            ; wrap around if necessary
                    move A,R_I2         ; delay pointer for next entry
                    if "sout"!="sdel"
                            move A,R_L
                    endif
                    ; load current input to A, current delay value to B
                    if "sinp"=="sdel"
                         move sinp:(R_I1)+,A      ; (2) (3)
                         if "sout"=="sdel"
                              move sdel:(R_I2),B  ; (2) no post-increment!
                         else
                              move sdel:(R_I2)+,B ; (3) post-inc since R_L used
                         endif
                    else
                         if "sout"=="sdel"
                              nop                                  ; (4)
                              move sinp:(R_I1)+,A sdel:(R_I2),B
                         else
                              move sinp:(R_I1)+,A sdel:(R_I2)+,B   ; (1)
                         endif
                    endif

                    ; output current delay-line value, overwrite with input
                    if "sout"=="sdel"
                         move A,sdel:(R_I2)+                    ; (2) (4)
                         move B,sout:(R_O)+
                    else
                         move A,sdel:(R_L)+ B,sout:(R_O)+       ; (1) (3)
                    endif
                    move R_I2,A         ; delay pointer for next entry
pf\_delay_\ic\_tickloop    
               move A,x:(R_X)+          ; save delay pointer for next entry
               if "sout"!="sdel"
                    move N_I2,R_L       ; restore R_L
               endif
     endm


