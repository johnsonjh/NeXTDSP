;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/12/87/jms - initial file created from DSPAPSRC/cvtcv.asm 
;;      02/17/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/22/88/jms - update memory use and execution time in documentation 
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvcombine (AP macro) - complex vector combine
;;          - form complex vector by combining two real ones
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvcombine  pf,ic,sinp1,ainp10,iinp10,sinp2,ainp20,iinp20,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvcombine_\ic is globally unique)
;;      sinp1   =   input vector 1 memory space ('x' or 'y')
;;      ainp10  =   input vector 1 base address
;;      iinp10  =   increment for input vector 1
;;      sinp2   =   input vector 2 memory space ('x' or 'y')
;;      ainp20  =   input vector 2 base address
;;      iinp20  =   increment for input vector 2
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   increment for output vector
;;      cnt0    =   element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector 1 base address     ainp10
;;      x:(R_X)+    input vector 1 increment        iinp10
;;      x:(R_X)+    input vector 2 base address     ainp20
;;      x:(R_X)+    input vector 2 increment        iinp20
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The cvcombine array-processor macro loads the elements of a complex vector
;;      from two real vectors.
;;
;;  PSEUDO-C NOTATION
;;      ainp1   =   x:(R_X)+;   /* input address 1      */
;;      iinp1   =   x:(R_X)+;   /* input increment 1    */
;;      ainp2   =   x:(R_X)+;   /* input address 2      */
;;      iinp2   =   x:(R_X)+;   /* input increment 2    */
;;      aout    =   x:(R_X)+;   /* output address       */
;;      iout    =   x:(R_X)+;   /* output increment     */
;;      cnt     =   x:(R_X)+;   /* number of elements   */
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout]   = sinp1:ainp1[n*iinp1]; /* real part        */
;;          sout:aout[n*iout+1] = sinp2:ainp2[n*iinp2]; /* imaginary part   */
;;      }
;;      
;;  MEMORY USE
;;      7 x-memory arguments
;;      sinp2==sout:    23 program memory locations
;;      sinp2!=sout:    21 program memory locations
;;      
;;  EXECUTION TIME
;;      cnt*4+21 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      cvcombine (prefix)pf,(instance)ic,
;;          (dspace)sinp1,(input)ainp1,iinp1,
;;          (dspace)sinp2,(input)ainp2,iinp2,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPcvcombine(aA,iA,aB,iB,aC,iC,n); /* complex combine:C[k]=A[k]+i*B[k]
;;          aA  =   A source vector base address (A real, real part of C)
;;          iA  =   A address increment
;;          aB  =   B source vector base address (B real, imaginary part of C)
;;          iB  =   B address increment
;;          aC  =   C destination vector base address (C complex)
;;          iC  =   C address increment (2 for successive elements)
;;          n   =   element count (number of complex values)    */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tcvcombine'
;;      symobj  ax_vec,bx_vec
;;      beg_xeb
;;          radix 16
;;ax_vec    dc a11aa,a12aa,a21aa,a22aa,a31aa,a32aa,a41aa,a42aa,a51aa,a52aa,a61aa,a62aa
;;bx_vec    dc b11bb,b12bb,b21bb,b22bb,b31bb,b32bb,b41bb,b42bb,b51bb,b52bb,b61bb,b62bb
;;          radix A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      cvcombine ap,1,x,ax_vec,2,x,bx_vec,2,x,out1_vec,2,3
;;      end_ap  'tcvcombine'
;;      end
;;
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvcombine.asm
;;      
;;  REGISTER USE  
;;      A       accumulator
;;      B       count
;;      X0      vector 1
;;      Y0      vector 2
;;      Y1      storage for constant=1
;;      R_I1    running pointer to vector 1 terms
;;      N_I1    increment for vector 1
;;      R_L     running pointer to vector 2 terms
;;      N_L     increment for vector 2
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      N_X     temporary storage for R_L
;;      
cvcombine  macro pf,ic,sinp1,ainp10,iinp10,sinp2,ainp20,iinp20,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_cvcombine_\ic\_,ainp1,ainp10
        new_xarg    pf\_cvcombine_\ic\_,iinp1,iinp10
        new_xarg    pf\_cvcombine_\ic\_,ainp2,ainp20
        new_xarg    pf\_cvcombine_\ic\_,iinp2,iinp20
        new_xarg    pf\_cvcombine_\ic\_,aout,aout0
        new_xarg    pf\_cvcombine_\ic\_,iout,iout0
        new_xarg    pf\_cvcombine_\ic\_,cnt,cnt0

        ; get inputs
        move            #>1,Y1
        move            R_L,N_X
        move            x:(R_X)+,R_I1   ; input vector 1 address
        move            x:(R_X)+,N_I1   ; input vector 1 increment 
        move            X:(R_X)+,R_L    ; input vector 2 address 
        move            x:(R_X)+,N_L    ; input vector 2 increment
        move            x:(R_X)+,R_O    ; output vector address
        move            x:(R_X)+,A      ; output vector decrement
        sub     Y1,A    x:(R_X)+,B      ; count
        move            A,N_O           ; decremented output vector increment to N_O
     
        ; set up loop and pipelining
        tst     B   
        jeq     pf\_cvcombine_\ic\_loop                ; test B to protect against count=0
        move            sinp1:(R_I1)+N_I1,A         ; fetch real

        ; inner loop 
        do  B,pf\_cvcombine_\ic\_loop
            if "sinp2"=="sout"                 
                move    A,sout:(R_O)+           ; store real
                move    sinp2:(R_L)+N_L,B       ; fetch imag
            else
                if "sinp2"=='x'
                    move    sinp2:(R_L)+N_L,B   A,sout:(R_O)+ 
                else
                    move    A,sout:(R_O)+       sinp2:(R_L)+N_L,B
                endif
            endif
            if "sinp1"=="sout"
                move    B,sout:(R_O)+N_O        ; store imag
                move    sinp1:(R_I1)+N_I1,A     ; fetch real
            else
                if "sinp1"=='x'
                    move    sinp1:(R_I1)+N_I1,A B,sout:(R_O)+N_O    
                else
                    move    B,sout:(R_O)+N_O    sinp1:(R_I1)+N_I1,A
                endif
            endif
pf\_cvcombine_\ic\_loop
        move    N_X,R_L
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.
        endm

