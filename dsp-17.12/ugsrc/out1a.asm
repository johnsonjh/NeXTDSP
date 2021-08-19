;;  Copyright 1989 by NeXT Inc.
;;  Author - J.O. Smith
;;
;;  Modification history
;;  --------------------
;;  04/18/89/mmm - initial file created from out2sum.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      out1a (UG macro) - Sum signal vector into channel 0 of sound output buffer
;;
;;  SYNOPSIS
;;      out1a pf,ic,ispc,iadr0,sclA0  
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_out2sum_\ic\_ is globally unique)
;;      ispc      = input vector memory space ('x' or 'y')
;;      iadr0     = initial input vector memory address
;;      sclA0     = initial channel 0 gain
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     ------------             --------------
;;      x:(R_X)+       Current channel A gain   sclA0
;;      y:(R_Y)+       Current input address    iadr0
;;
;;  DESCRIPTION
;;      The out1a unit-generator sums a signal vector to channel 0 of the outgoing
;;      stereo sound stream, or the mono stream if in mono mode.
;;
;;  RESTRICTIONS
;;
;;  DSPWRAP ARGUMENT INFO
;;      out1a (prefix)pf,(instance)ic,(dspace)ispc,(input)iadr,sclA
;;
;;  MAXIMUM EXECUTION TIME
;;      126 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      92 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;      include 'music_macros'        ; utility macros
;;      beg_orch 'tout2sum'           ; begin orchestra main program
;;           new_yeb outvec,I_NTICK,0 ; Allocate input vector
;;           beg_orcl                 ; begin orchestra loop
;;                oscg orch,1,y,outvec,0.5,8,0,0,0,y,YLI_SIN,$FF    ; sinewave
;;                out1a orch,1,y,outvec,0.999
;;           end_orcl                 ; end of orch loop (update L_TICK,etc.)
;;      end_orch 'tout2sum'           ; end of orchestra main program
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/out1a.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/orchloopbegin.asm - invokes macro beg_orcl
;;      /usr/lib/dsp/smsrc/beginend.asm(beg_orcl) - calls service_write_data
;;      /usr/lib/dsp/smsrc/jsrlib.asm(service_write_data) - clears output tick
;;
out1a macro pf,ic,ispc,iadr0,sclA0
                new_xarg pf\_out1a_\ic\_,sclA,sclA0 ; left-channel gain
                new_yarg pf\_out1a_\ic\_,iadr,iadr0 ; input address arg
                move x:(R_X)+,X1           ; X1 = channel 0 gain
                move p:loc_x_dma_wfp,R_I2  ; hmdispatch.asm
                move  y:(R_Y)+,R_I1        ; input address vector to R_I1
                move x:(R_I2),R_O          ; Current position in dma output buffer
                move #(2*I_NDMA-1),M_O     ; output is modulo ring size

                if I_NCHANS==1
                  move #1,N_O              ; just postincrement by 1
                else
                  move #2,N_O              ; by 2 for interleaved write of channel A
                endif

                if "ispc"=='x'
                  move ispc:(R_I1)+,X0 y:(R_O),A ; read input & sample
                else 
                  move ispc:(R_I1)+,X0     ; read input
                  move y:(R_O),A           ; read sample
                endif

                do #I_NTICK,pf\_out1a_\ic\_loop
                     macr X0,X1,A  y:(R_O+N_O),Y1      ; scale & sum, get next output
                     if "ispc"=='x'
                       tfr  Y1,A  ispc:(R_I1)+,X0  A,y:(R_O)+N_O
                     else
                       tfr  Y1,A  A,y:(R_O)+N_O    ; next buffer samp to A, ship output
                       move ispc:(R_I1)+,X0        ; get next input sample
                     endif
pf\_out1a_\ic\_loop
                move #-1,M_O             ; always assumed
          endm


