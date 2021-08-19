;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/08/87/jos - initial file created
;;  04/23/87/jos - changed to 24-bit version
;;  04/23/87/jos - passed test/tunoise.asm
;;  04/27/87/jos - changed to new init format involving unoise_init.asm
;;  05/06/87/jos - passed test/tunoise.asm
;;  02/06/88/jos - current version (with unoise_init) moved to grub/*.1
;;  02/06/88/jos - eliminated need for unoise_init.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      unoise (UG macro) - Uniform pseudo-random number generator
;;
;;  SYNOPSIS
;;      unoise pf,ic,sout,aout0,seed0 
;;
;;  MACRO ARGUMENTS
;;      pf    = global label prefix (any text unique to invoking macro)
;;      ic    = instance count (such that pf\_unoise_\ic\_ is globally unique)
;;      sout  = output vector memory space ('x' or 'y')
;;      aout0 = initial output vector memory address
;;      seed0 = initial state of random number generator (becomes last number)
;;
;;  DSP MEMORY ARGUMENTS
;;      Access         Description              Initialization
;;      ------         -----------              --------------
;;      x:(R_X)+       Current output address   aout0
;;      y:(R_Y)+       Current state            seed0
;;
;;  DESCRIPTION
;;      The unoise unit-generator computes uniform pseudo-white noise
;;      using the linear congruential method for random number generation
;;      (reference: Knuth, volume II of The Art of Computer Programming).
;;      The multiplier used has not been tested for quality.
;;      
;;  DSPWRAP ARGUMENT INFO
;;      unoise (prefix)pf,(instance)ic,(dspace)sout,(output)aout,seed
;;
;;  MAXIMUM EXECUTION TIME
;;      116 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      Same as maximum.
;;
;;  CALLING PROGRAM TEMPLATE
;;      include 'music_macros'        ; utility macros
;;      beg_orch 'tunoise'            ; begin orchestra main program
;;           new_yeb outvec,I_NTICK,0 ; Allocate output vector
;;           beg_orcl                 ; begin orchestra loop
;;                unoise orch,1,y,outvec,0 ; Macro invocation
;;           end_orcl                 ; end of orchestra loop
;;      end_orch 'tunoise'            ; end of orchestra main program
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/unoise.asm
;;
;;  ALU REGISTER USE
;;      X1 = multiplier
;;      X0 = previous random number
;;       A = offset, then (garbage,new_random_number)
;;       B = 48 bits containing (0,offset)
;;
unoise_offset set 1                     ; value of offset (any positive word)

unoise    macro pf,ic,sout,aout0,seed0
          new_xarg pf\_unoise_\ic\_,aout,aout0    ; output address arg
          new_yarg pf\_unoise_\ic\_,seed,seed0    ; random number state
          clr B x:(R_X)+,R_O            ; output address to R_O
          move #>unoise_offset*2,B0     ; (48-bit) offset, times 2, to B
          tfr B,A y:(R_Y),X0            ; offset to A, current seed to X0
          move #5609937,X1              ; pick a number = 1 mod 4
          mac X0,X1,A                   ; make A0 valid
          asr A                         ; now A0 contains unsigned X0*X1 LSP
          do #I_NTICK-1,pf\_unoise_\ic\_tickloop
               tfr B,A A0,X0            ; restore offset, update seed
               mac X1,X0,A X0,sout:(R_O)+
               asr A                    ; now A0 contains unsigned X0*X1 LSP
pf\_unoise_\ic\_tickloop    
          move A0,sout:(R_O)+           ; could overlap sometimes
          move A0,y:(R_Y)+              ; output last sample, update seed
     endm


