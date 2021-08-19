;;  Copyright 1986 by NeXT Inc.
;;  Author - Julius Smith
;;      
;;  Modification history
;;  --------------------
;;      11/25/87/jos - initial file created from DSPAPSRC/vfill.asm
;;      02/15/88/jms - changed "...\ic\_\l..." to "...\ic\_l..."
;;      02/17/88/jms - added protection against count=0
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      06/12/89/mtm - rearrange comments to match dspwrap
;;	06/15/89/mtm - removed update of seed to y memory
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vrand (AP macro) - vector random numbers 
;;          - fill vector with uniform random numbers between -1 and 1.
;;      
;;  SYNOPSIS
;;      include 'ap_macros'
;;      vrand   pf,ic,sseed,aseed0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\vrand_\ic is globally unique)
;;      sseed   =   seed memory space ('x' or 'y')
;;      aseed0  =   address of seed in dsp memory
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector memory address
;;      iout0   =   increment for output vector address
;;      cnt0    =   element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description             Initialization
;;      ------      -----------             --------------
;;      x:(R_X)+    Seed address            aseed0
;;      x:(R_X)+    Output address          aout0
;;      x:(R_X)+    Address increment       iout0
;;      x:(R_X)+    element count           cnt0
;;      
;;  DESCRIPTION
;;      The vrand macro computes uniform pseudo-random numbers
;;      using the linear congruential method for random number generation
;;      (reference: Knuth, volume II of The Art of Computer Programming).
;;      The multiplier used is 5609937 and the offset is 1.
;;
;;  PSEUDO-C NOTATION
;;      aS  =   x:(R_X)+;
;;      aD  =   x:(R_X)+;
;;      iD  =   x:(R_X)+;
;;      ct  =   x:(R_X)+;
;;
;;      seed = sseed:aseed[0];
;;      for (n=0;n<ct;n++) {
;;          seed = (5609937*seed+1) & 0xFFFFFF;
;;          sout:aout[n*iout] = seed;
;;      }
;;      sseed:aseed[0] = seed;
;;      
;;  MEMORY USE 
;;      4 x-memory arguments
;;      20 program memory locations 
;;      
;;  EXECUTION TIME
;;      6*cnt+19 instruction cycles
;;	      
;;  DSPWRAP ARGUMENT INFO
;;      vrand (prefix)pf,(instance)ic,
;;          (dspace)sseed,aseed,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvrand(aS,aD,iD,n);   /* D[k] = RANDOM(S[0]); S[0]=D[k]
;;          aS  =   Address of random number seed
;;          aD  =   Destination vector base address
;;          iD  =   Destination address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvrand'            ; begin ap main program
;;      new_yeb seed,1,0            ; allocate output vector
;;      new_yeb outvec,100,0        ; allocate output vector
;;      vrand ap,1,y,seed,y,outvec,1,100 
;;      end_ap  'tvrand'            ; end of ap main program
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vrand.asm
;;      
;;  REGISTER USE 
;;      A       offset, then random value in A0
;;      B       offset (1)
;;      X0      current seed
;;      X1      multiplier (5609937)
;;      R_I1    seed address
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
vrand   macro pf,ic,sseed,aseed0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vrand_\ic\_,aseed,aseed0    ; seed address arg
        new_xarg    pf\_vrand_\ic\_,aout,aout0      ; output address arg
        new_xarg    pf\_vrand_\ic\_,iout,iout0      ; address increment arg
        new_xarg    pf\_vrand_\ic\_,cnt,cnt0        ; element count arg

        ; get inputs
        move            x:(R_X)+,R_I1       ; seed address
        move            x:(R_X)+,R_O        ; address to R_O
        move            x:(R_X)+,N_O        ; address increment to N_O
        move            x:(R_X)+,B          ; count		

        ; set up loop and pipelining
        tst     B       B,Y1
        jeq     pf\_vrand_\ic\_l1           ; protect against cnt = 0
        clr     B       sseed:(R_I1)+,X0    ; seed to X0
        move            #>1*2,B0            ; offset*2 to B
        ;;  (*2 is due do left-shift which occurs in LSP using MPY)
        tfr     B,A     #5609937,X1         ; offset to A, pick a number = 1 mod 4
        mac     X0,X1,A                     ; make A0 valid
        asr     A                           ; A0 = unsigned X0*X1 LSP

        ; inner loop 
        do      Y1,pf\_vrand_\ic\_aploop
            tfr     B,A     A0,X0           ; A = offset, update seed
            mac     X1,X0,A X0,sout:(R_O)+N_O ; A0 = next random value * 2
            asr     A                       ; A0 = unsigned X0*X1 LSP
pf\_vrand_\ic\_aploop 
;
;	This instruction has been removed.  What did it do?
;       move            X0,y:(R_Y)+         ; update seed
;
	nop     
pf\_vrand_\ic\_l1
        endm

