;;  Copyright 1989 by NeXT Inc.
;;  Author - J.O. Smith
;;
;;  Modification history
;;  --------------------
;;  06/27/87/jos - initial file created from outa.asm
;;  08/04/87/jos - Passed test/tout2.asm
;;  10/20/87/jos - Fixed bug in replace-sum case which delayed right chan 1
;;                   sample, repeating first sample in each tick of 1st writer!
;;  02/06/88/jos - Created from out2.asm. Flushed out_new_<chan> variable
;;  02/04/89/jos - added loc_x_dma_wfp indirection (hmdispatch.asm).
;;                 This obviates needing to reassemble this unit-generator
;;                 whenever the DSP system X variables are moved (which happens
;;                 whenever the DSP system code is modified since they float).
;;  02/17/89/jos - changed loc_x_dma_wfp space from x to p so simulation works.
;;  02/17/89/jos - added error checking for plausibility of x:X_DMA_WFP
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      out2sum (UG macro) - Sum signal vector into sound output buffer.
;;
;;  SYNOPSIS
;;      out2sum pf,ic,ispc,iadr0,sclA0,sclB0  
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_out2sum_\ic\_ is globally unique)
;;      ispc      = input vector memory space ('x' or 'y')
;;      iadr0     = initial input vector memory address
;;      sclA0     = initial channel 0 gain
;;      sclB0     = initial channel 1 gain
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     ------------             --------------
;;      x:(R_X)+       Current channel A gain   sclA0
;;      y:(R_Y)+       Current channel B gain   sclB0
;;      y:(R_Y)+       Current input address    iadr0
;;
;;  DESCRIPTION
;;      The out2sum unit-generator sums a signal vector to the outgoing
;;      stereo sound stream. For efficiency, it is desirable to use as few 
;;      instances of out2sum as possible.  For example, several sources
;;      can be summed into a common signal and then passed to out2sum.
;;      Each instance of out2sum can provide a particular stereo placement.
;;
;;  RESTRICTIONS
;;      The out2sum unit-generator must be appear first in each tick
;;      loop pass before out2sum is executed.  This is how the output
;;      buffer gets cleared once per tick prior to out2sum additions.
;;
;;      Two output channels (stereo) assumed.  One channel makes no sense
;;      and more channels means significantly more instructions.  A quad
;;      version (or more) can be easily written from this routine.
;;
;;  DSPWRAP ARGUMENT INFO
;;      out2sum (prefix)pf,(instance)ic,(dspace)ispc,(input)iadr,sclA,sclB
;;
;;  MAXIMUM EXECUTION TIME
;;      192 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      160 DSP clock cycles for one "tick".
;;
;;  CALLING PROGRAM TEMPLATE
;;      include 'music_macros'        ; utility macros
;;      beg_orch 'tout2sum'           ; begin orchestra main program
;;           new_yeb outvec,I_NTICK,0 ; Allocate input vector
;;           beg_orcl                 ; begin orchestra loop
;;                oscg orch,1,y,outvec,0.5,8,0,0,0,y,YLI_SIN,$FF    ; sinewave
;;                out2sum orch,1,y,outvec,0.707,0.707 ; Place it in the middle
;;           end_orcl                 ; end of orch loop (update L_TICK,etc.)
;;      end_orch 'tout2sum'           ; end of orchestra main program
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/out2sum.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/orchloopbegin.asm - invokes macro beg_orcl
;;      /usr/lib/dsp/smsrc/beginend.asm(beg_orcl) - calls service_write_data
;;      /usr/lib/dsp/smsrc/jsrlib.asm(service_write_data) - clears output tick
;;
out2sum macro pf,ic,ispc,iadr0,sclA0,sclB0
                new_xarg pf\_out2sum_\ic\_,sclA,sclA0 ; left-channel gain
                new_yarg pf\_out2sum_\ic\_,sclB,sclB0 ; right-channel gain
                new_yarg pf\_out2sum_\ic\_,iadr,iadr0 ; input address arg
                if I_NCHANS!=2           ; Two output channels enforced
                     fail 'out2sum UG insists on 2-channel output (stereo)'
                endif
                move x:(R_X)+,X1 y:(R_Y)+,Y1 ; (X1,Y1) = channel (0,1) gain
                move p:loc_x_dma_wfp,R_I2 ; hmdispatch.asm
                move  y:(R_Y)+,R_I1      ; input address vector to R_I1
;*old way*      move x:X_DMA_WFP,R_O     ; Current position in dma output buffer
                move x:(R_I2),R_O       ; Current position in dma output buffer
                move #(2*I_NDMA-1),M_O   ; output is modulo ring size
            if UG_DEBUG
                move R_O,X0             ; current write-fill pointer
                move #YB_DMA_W,A        ; lowest legal value
                cmp X0,A                ; lowest - current
                jle pf\_out2sum_\ic\_not_too_low
pf\_out2sum_\ic\_too_high
                  tfr X0,A #DE_WFP_BAD,X0  ; arg and error code
                  jsr stderr               ; add PC, SR, time, etc. and ship it
pf\_out2sum_\ic\_not_too_low
                move #NB_DMA_W,Y0
                add Y0,A                ; largest legal value plus 1
                cmp X0,A                ; largest+1 - current
                jle pf\_out2sum_\ic\_too_high
            endif
                move #2,N_O             ; for interleaved write of channel A
                move N_O,N_I2           ; for interleaved write of channel B
                lua (R_O)+,R_I2         ; R_I2 will point to channel B below
                do #I_NTICK,pf\_out2sum_\ic\_loop
                     if "ispc"=='x'
                          ; Since we must read and write y mem 
                          ; twice, any 8-cycle inner loop is optimal.
                          move x:(R_I1)+,X0 y:(R_O),A ; read input & right
                     else
                          ; Since we must read y mem 3 times and write it
                          ; twice, any 10-cycle inner loop is optimal.
                          move y:(R_I1)+,X0   ; read input
                          move y:(R_O),A      ; read left
                     endif
                     macr X0,X1,A y:(R_I2)+N_I2,B ; comp left, read right
                     macr X0,Y1,B A,y:(R_O)+      ; comp right, out left
                     move B,y:(R_O)+              ; out right
pf\_out2sum_\ic\_loop
                move #-1,M_O             ; always assumed
          endm


