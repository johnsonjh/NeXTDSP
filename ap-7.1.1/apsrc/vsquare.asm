;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      07/10/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - changed neg A to tst A in initialization
;;                   - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vsquare (AP macro) - vector square - square the elements of a vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vsquare     pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vsquare_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector base address (address of A[1,1])
;;      iinp0   =   initial increment for input vector
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   initial increment for output vector 
;;      cnt0    =   initial element count
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
;;      The vsquare array-processor macro computes C=A*A.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;    
;;      iinp    =   x:(R_X)+;    
;;      aout    =   x:(R_X)+;  
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;  
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = sinp:ainp[n*iinp]^2;
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
;;      vsquare (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvsquare(aS,iS,aD,iD,n);  /* D[k] = S[k] * S[k]
;;          aS  =   Source vector base address
;;          iS  =   S address increment
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvsquare'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      vsquare     ap,1,x,ax_vec,1,x,out1_vec,1,6
;;      end_ap  'tvsquare'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vsquare.asm
;;      
;;  REGISTER USE  
;;      A       accumulator; temp for testing for cnt==0
;;      X1      cnt
;;      X0 or Y0 input term, depending on sinp
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     running pointer to ouput vector terms
;;      N_O     increment for output vector
;;      
vsquare     macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vsquare_\ic\_,ainp,ainp0
        new_xarg    pf\_vsquare_\ic\_,iinp,iinp0
        new_xarg    pf\_vsquare_\ic\_,aout,aout0
        new_xarg    pf\_vsquare_\ic\_,iout,iout0
        new_xarg    pf\_vsquare_\ic\_,cnt,cnt0
     
        ; get inputs
        move            x:(R_X)+,R_I1   ; input vector base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector address increment to N_I1
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,A      ; count

        ; set up loop and pipelining
        tst     A       A,X1            ; squirrel away count for do loop
        jeq     pf\_vsquare_\ic\_loop2      ; protect againt count=0

        if "sinp"=="sout" 
            move            sinp:(R_I1)+N_I1,X0
            do      X1,pf\_vsquare_\ic\_loop1
                mpyr    X0,X0,A sinp:(R_I1)+N_I1,X0
                move    A,sout:(R_O)+N_O   
pf\_vsquare_\ic\_loop1                
        else
            if "sinp"=='x'
                move            sinp:(R_I1)+N_I1,X0
                mpyr    X0,X0,A sinp:(R_I1)+N_I1,X0
                rep     X1
                    mpyr    X0,X0,A sinp:(R_I1)+N_I1,X0 A,sout:(R_O)+N_O
            else
                move            sinp:(R_I1)+N_I1,Y0
                mpyr    Y0,Y0,A sinp:(R_I1)+N_I1,Y0
                rep     X1
                    mpyr    Y0,Y0,A A,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,Y0 
            endif
        endif
        OPTIONAL_NOP    
            ; from defines.asm, to prevent the macro from ending
            ; at the end of a loop. If you remove the nop, or if
            ; you redefine OPTIONAL_NOP to be just ' ', then the 
            ; macro loaded next might begin with an op code which
            ; is illegal after the end of a loop.

pf\_vsquare_\ic\_loop2

        endm

