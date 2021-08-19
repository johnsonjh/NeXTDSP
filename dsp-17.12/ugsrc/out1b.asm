;;  Copyright 1989 by NeXT Inc.
;;  Author - J.O. Smith
;;
;;  Modification history
;;  --------------------
;;  04/19/89/mmm - initial file created from out1a.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      out1b (UG macro) - Sum signal vector into channel 0 of sound output buffer
;;
;;  SYNOPSIS
;;      out1b pf,ic,ispc,iadr0,sclB0  
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_out2sum_\ic\_ is globally unique)
;;      ispc      = input vector memory space ('x' or 'y')
;;      iadr0     = initial input vector memory address
;;      sclB0     = initial channel 1 gain
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     ------------             --------------
;;      x:(R_X)+       Current channel A gain   sclB0
;;      y:(R_Y)+       Current input address    iadr0
;;
;;  DESCRIPTION
;;      The out1b unit-generator sums a signal vector to channel 1 of the outgoing
;;      stereo sound stream, or into the mono stream if in mono mode.
;;
;;  RESTRICTIONS
;;
;;  DSPWRAP ARGUMENT INFO
;;      out1b (prefix)pf,(instance)ic,(dspace)ispc,(input)iadr,sclB
;;
;;  MAXIMUM EXECUTION TIME
;;      128 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      94 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;      include 'music_macros'        ; utility macros
;;      beg_orch 'tout2sum'           ; begin orchestra main program
;;           new_yeb outvec,I_NTICK,0 ; Allocate input vector
;;           beg_orcl                 ; begin orchestra loop
;;                oscg orch,1,y,outvec,0.5,8,0,0,0,y,YLI_SIN,$FF    ; sinewave
;;                out1b orch,1,y,outvec,0.999
;;           end_orcl                 ; end of orch loop (update L_TICK,etc.)
;;      end_orch 'tout2sum'           ; end of orchestra main program
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/out1b.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/orchloopbegin.asm - invokes macro beg_orcl
;;      /usr/lib/dsp/smsrc/beginend.asm(beg_orcl) - calls service_write_data
;;      /usr/lib/dsp/smsrc/jsrlib.asm(service_write_data) - clears output tick
;;
out1b macro pf,ic,ispc,iadr0,sclB0
                new_xarg pf\_out1b_\ic\_,sclB,sclB0 ; right-channel gain
                new_yarg pf\_out1b_\ic\_,iadr,iadr0 ; input address arg
                move x:(R_X)+,X1           ; X1 = channel 0 gain
                move p:loc_x_dma_wfp,R_I2  ; hmdispatch.asm
                move  y:(R_Y)+,R_I1        ; input address vector to R_I1
                move x:(R_I2),R_O          ; Current position in dma output buffer
                move #(2*I_NDMA-1),M_O     ; output is modulo ring size

                if I_NCHANS==1
                  move #1,N_O              ; just postincrement by 1
                else
                  move #2,N_O              ; increment by 2 samples
                  move y:(R_O)+,Y1         ; dummy read of channel A
                endif

                if "ispc"=='x'
                  move ispc:(R_I1)+,X0 y:(R_O),A ; read input & sample
                else 
                  move ispc:(R_I1)+,X0     ; read input
                  move y:(R_O),A           ; read sample
                endif

                do #I_NTICK,pf\_out2sum_\ic\_loop
                     macr X0,X1,A  y:(R_O+N_O),Y1 ; scale & sum, get next output
                     if "ispc"=='x'
                       tfr  Y1,A  ispc:(R_I1)+,X0  A,y:(R_O)+N_O
                     else
                       tfr  Y1,A  A,y:(R_O)+N_O  ; next buffer samp to A, ship output
                       move ispc:(R_I1)+,X0  ; get next input sample
                     endif
pf\_out2sum_\ic\_loop
                move #-1,M_O             ; always assumed
          endm


