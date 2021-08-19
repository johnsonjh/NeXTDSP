;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/08/87/jos - initial file created
;;  04/23/87/jos - changed to 24-bit version
;;  04/23/87/jos - passed test/tunoisehp.asm
;;  04/27/87/jos - changed to new init format involving unoisehp_init.asm
;;  05/06/87/jos - passed test/tunoisehp.asm
;;  02/06/88/jos - current version (with unoisehp_init) moved to grub/*.1
;;  02/06/88/jos - eliminated need for unoisehp_init.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      unoisehp (UG macro) - Highpassed uniform pseudo-random number generator
;;
;;  SYNOPSIS
;;      unoisehp pf,ic,sout,aout0,seed0
;;
;;  MACRO ARGUMENTS
;;      pf    = global label prefix (any text unique to invoking macro)
;;      ic    = instance count (s.t. pf\_unoisehp_\ic\_ is globally unique)
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
;;      The unoisehp unit-generator computes differenced uniform 
;;      pseudo-white noise.  The noise is computed exactly like that in
;;      the unoise unit generator.  The first-order difference approximates
;;      differentiation of the noise.  The spectral energy thus rises as
;;      a function of frequency.
;;      
;;  DSPWRAP ARGUMENT INFO
;;      unoisehp (prefix)pf,(instance)ic,(dspace)sout,(output)aout,seed
;;
;;  CALLING PROGRAM TEMPLATE
;;      include 'music_macros'        ; utility macros
;;      beg_orch 'tunoisehp'          ; begin orchestra main program
;;           new_yeb outvec,I_NTICK,0 ; Allocate output vector
;;           beg_orcl                 ; begin orchestra loop
;;                unoisehp orch,1,y,outvec,0 ; Macro invocation
;;           end_orcl                 ; end of orchestra loop
;;      end_orch 'tunoisehp'          ; end of orchestra main program
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/unoise.asm
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/unoisehp.asm
;;
;;  ALU REGISTER USE
;;      X1 = multiplier
;;      X0 = previous random number
;;       A = offset, then (garbage,new_random_number)
;;       B = 48 bits containing (0,offset)
;;

unoisehp_offset set 1                   ; value of offset (any positive word)

unoisehp    macro pf,ic,sout,aout0,seed0
          new_xarg pf\_unoisehp_\ic\_,aout,aout0    ; output address arg
          new_yarg pf\_unoisehp_\ic\_,seed,seed0    ; random number state
          clr B x:(R_X)+,R_O            ; output address to R_O
          move #>unoisehp_offset*2,B0     ; (48-bit) offset, times 2, to B
          tfr B,A y:(R_Y),X0            ; offset to A, current seed to X0
          move #5609937,X1              ; pick a number = 1 mod 4
          mac X0,X1,A                   ; make A0 valid
          asr A                         ; now A0 contains unsigned X0*X1 LSP
          move A0,Y0
          do #I_NTICK-1,pf\_unoisehp_\ic\_tickloop
                tfr B,A A0,X0           ; restore offset, update seed
                mac X1,X0,A X0,sout:(R_O)+
                asr A                   ; now A0 contains unsigned X0*X1 LSP
                move A0,A1
                sub Y0,A A1,Y0          ; difference the noise
pf\_unoisehp_\ic\_tickloop    
          move A0,sout:(R_O)+           ; could overlap sometimes
          move A0,y:(R_Y)+              ; output last sample, update seed
     endm
