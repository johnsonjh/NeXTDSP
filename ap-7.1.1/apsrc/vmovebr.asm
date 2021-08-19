;;  Copyright 1986 by NeXT Inc.
;;  Author - Julius Smith
;;      
;;  Modification history
;;  --------------------
;;      06/30/89/jos - initial file created from DSPAPSRC/vmove.asm
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vmovebr (AP macro) - vector move, bit reversed
;;      
;;  SYNOPSIS
;;      include 'ap_macros'      ; load standard dsp macro package
;;      vmovebr    pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vmovebr_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   initial input vector memory address
;;      iinp0   =   initial increment for input vector address - typically cnt0/2
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial output vector memory address
;;      iout0   =   initial increment for output vector address
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description             Initialization
;;      ------      -----------             --------------
;;      x:(R_X)+    Source address          ainp0
;;      x:(R_X)+    Source increment        iinp0
;;      x:(R_X)+    Destination address     aout0
;;      x:(R_X)+    Destination increment   iout0
;;      x:(R_X)+    element count           cnt0
;;      
;;  DESCRIPTION
;;      The vmovebr array-processor macro copies the elements of one vector
;;      to another vector of the same length.  The index used on the input
;;      vector is in bit-reverse mode.  Normally, this move function is used
;;	to unscramble FFT output data.  Typically, iinp equals cnt/2.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = sinp:ainp[bit_reverse(n*iinp)];
;;
;;      where the function bit_reverse() reverses the order of the
;;      bits of its 16-bit integer argument.
;;      
;;  MEMORY USE
;;      5 x-memory arguments
;;      sinp==sout: 13 program memory locations
;;      sinp!=sout: 12 program memory locations
;;      
;;  EXECUTION TIME
;;      cnt*4+13 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vmovebr (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvmovebr(S,iS,D,iD,n);   /* vector move:
;;          S   =   Source vector base address
;;          iS  =   S address increment - typically n/2
;;          D   =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvmovebr'           ; begin ap main program
;;      new_yeb srcvec,10,$400000   ; allocate input vector
;;      new_yeb outvec,10,0         ; allocate output vector
;;      vmovebr ap,1,y,srcvec,1,y,outvec,1,10 
;;      end_ap 'tvmovebr'            ; end of ap main program
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vmovebr.asm
;;      
;;  REGISTER USE
;;      A       temporary storage for transfer
;;      B       count
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      M_I1     cleared for bit-reverse use, then set to -1 on exit
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
vmovebr    macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0
     
        ; argument definitions
        new_xarg    pf\_vmovebr_\ic\_,ainp,ainp0   ; input address 
        new_xarg    pf\_vmovebr_\ic\_,iinp,iinp0   ; input address increment 
        new_xarg    pf\_vmovebr_\ic\_,aout,aout0   ; output address 
        new_xarg    pf\_vmovebr_\ic\_,iout,iout0   ; output address increment 
        new_xarg    pf\_vmovebr_\ic\_,cnt,cnt0     ; element count 
     
        ; get inputs
        move            x:(R_X)+,R_I1           ; input address to R_I1
        move            x:(R_X)+,N_I1           ; address increment to N_I1
        move            x:(R_X)+,R_O            ; output address to R_O
        move            x:(R_X)+,N_O            ; address increment to N_O
        move            #0,M_I1            	; want bit-reverse indexing

        ; set up loop and pipelining
        move            x:(R_X)+,B
        tst     B       sinp:(R_I1)+N_I1,A      ; get first input term
        jeq     pf\_vmovebr_\ic\_aploop          ; protect against count=0

        ; inner loop
        do      B,pf\_vmovebr_\ic\_aploop
            if "sinp"=="sout"
                move    A,sout:(R_O)+N_O        ; vector move
                move    sinp:(R_I1)+N_I1,A
            else
                move    sinp:(R_I1)+N_I1,A  A,sout:(R_O)+N_O 
            endif
pf\_vmovebr_\ic\_aploop    
        move            #-1,M_I1            	; restore default M value
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

