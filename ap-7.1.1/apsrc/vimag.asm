;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/25/87/jms - initial file created from DSPAPSRC/vreal.asm and DSPAPSRC/vmove.asm
;;      02/15/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vimag (AP macro) - vector imaginary part
;;          - form a real vector from the imaginary part of a complex vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vimag   pf,ic,ainp0,iinp0,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vimag_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector base address (address of first real in vector)
;;      iinp0   =   increment for input vector
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   increment for output vector
;;      cnt0    =   element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector base address       ainp0
;;      x:(R_X)+    input vector increment          iinp0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vimag array-processor macro computes C[k]=Imag(A[k]), where A[k]
;;      is complex and C[k] is not.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;  
;;      ainp++;
;;      iinp    =   x:(R_X)+;  
;;      aout    =   x:(R_X)+;  
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;  
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout] = sinp:ainp[1+n*iinp];
;;      }
;;      
;;  MEMORY USE 
;;      5 x-memory arguments
;;      sinp==sout: 14 program memory locations
;;      sinp!=sout: 13 program memory locations
;;      
;;  EXECUTION TIME
;;      cnt*2+14 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vimag (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout, (output)aout, iout, cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvimag(aS,iS,aD,iD,n);    /* Extract imaginary part: D[k] = Imag(S[k])
;;          aS  =   S source vector base address (S complex; aS points to first REAL in vector)
;;          iS  =   S address increment (2 for successive elements)
;;          aD  =   D destination vector base address (D real = imaginary part of S)
;;          iD  =   D address increment
;;          n   =   element count (number of complex values for S)      */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvimag'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc a11aa,a12aa,a21aa,a22aa,a31aa,a32aa,a41aa,a42aa,a51aa,a52aa,a61aa,a62aa
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      vimag   ap,1,x,ax_vec,2,x,out1_vec,1,3
;;      end_ap  'tvimag'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vimag.asm
;;      
;;  REGISTER USE
;;      A       temporary storage for transfer
;;      B       count
;;      R_I1    running pointer to input vector
;;      N_I1    increment for input vector
;;      R_O     running pointer to output terms
;;      N_O     increment for output vector
;;      
vimag   macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vimag_\ic\_,ainp,ainp0  ; input address 
        new_xarg    pf\_vimag_\ic\_,iinp,iinp0  ; input address increment 
        new_xarg    pf\_vimag_\ic\_,aout,aout0  ; output address 
        new_xarg    pf\_vimag_\ic\_,iout,iout0  ; output address increment 
        new_xarg    pf\_vimag_\ic\_,cnt,cnt0    ; element count 
     
        ; get inputs
        ; The first argument is the base address for the input
        ; vector. That base address points to the first real in the
        ; vector.  Bump up that base address by 1, so that it points
        ; to the imaginary part of the first point in the vector.
        move            x:(R_X)+,R_I1   ; input base address to R_I1
        move            x:(R_X)+,N_I1   ; address increment to N_I1
        move            (R_I1)+         ; R_I1 now has address of imaginary part of first element 
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; address increment to N_O

        ; set up loop and pipelining
        move            x:(R_X)+,B
        tst     B       sinp:(R_I1)+N_I1,A  ; get first input term
        jeq     pf\_vimag_\ic\_aploop       ; protect against count=0

        ; inner loop
        do      B,pf\_vimag_\ic\_aploop
            if "sinp"=="sout"
                move    A,sout:(R_O)+N_O    ; vector move
                move    sinp:(R_I1)+N_I1,A
            else
                move    sinp:(R_I1)+N_I1,A  A,sout:(R_O)+N_O 
            endif
pf\_vimag_\ic\_aploop    
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

