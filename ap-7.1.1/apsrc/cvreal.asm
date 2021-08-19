;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/12/87/jms - initial file created from DSPAPSRC/cvtcv.asm  
;;      02/15/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvreal (AP macro) - complex vector from real
;;          - form complex vector from a real vector and 0 imaginary part
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvreal  pf,ic,ainp0,iinp0,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvreal_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector base address
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
;;      The cvreal array-processor macro computes C[k]=A[k] + i*0, 
;;      where A[k] is real and C[k] is complex.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;  
;;      iinp    =   x:(R_X)+;  
;;      aout    =   x:(R_X)+;  
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;  
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout]   = sinp:ainp[n*iinp];
;;          sout:aout[n*iout+1] = 0;
;;      }
;;      
;;  MEMORY USE
;;      5  x-memory arguments
;;      sinp1==sinp2:   17 program memory locations
;;      sinp1!=sinp2:   18 program memory locations
;;      
;;  EXECUTION TIME
;;      sinp1==sinp2:   cnt*3+17 instruction cycles
;;      sinp1!=sinp2:   cnt*3+18 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      cvreal (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPcvreal(aS,iS,aD,iD,n);   /* complex from real: D[k] = S[k] + i*0
;;          aS  =   S source vector base address (S real = real part of D)
;;          iS  =   S address increment
;;          aD  =   D destination vector base address (D complex, zero imaginary part)
;;          iD  =   D address increment (2 for successive elements)
;;          n   =   element count (number of complex values for D)  */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tcvreal'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc a11aa,a12aa,a21aa,a22aa,a31aa,a32aa,a41aa,a42aa,a51aa,a52aa,a61aa,a62aa
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      cvreal  ap,1,x,ax_vec,2,x,out1_vec,2,3
;;      end_ap  'tcvreal'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvreal.asm
;;      
;;  REGISTER USE  
;;      A       real input
;;      B       zero
;;      X0      count; temporary storage of constant 1
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
cvreal  macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_cvreal_\ic\_,ainp,ainp0
        new_xarg    pf\_cvreal_\ic\_,iinp,iinp0
        new_xarg    pf\_cvreal_\ic\_,aout,aout0
        new_xarg    pf\_cvreal_\ic\_,iout,iout0
        new_xarg    pf\_cvreal_\ic\_,cnt,cnt0

        ; get inputs
        move            #>1,X0
        move            x:(R_X)+,R_I1   ; input vector address
        move            x:(R_X)+,N_I1   ; input vector increment 
        move            x:(R_X)+,R_O    ; output vector address
        move            x:(R_X)+,A      ; output vector increment
        sub     X0,A    x:(R_X)+,B      ; count
     
        ; set up loop and pipelining
        tst     B       A,N_O           ; decremented output vector increment to N_O
        jeq     pf\_cvreal_\ic\_loop    ; test A to protect against count=0
        move            B,X0
        clr     A       sinp:(R_I1)+N_I1,B      ; fetch real, set A to 0 for imag

        ; inner loop 
        do      X0,pf\_cvreal_\ic\_loop
            move    B,sout:(R_O)+               ; store real
            if "sinp"=="sout"
                move    A,sout:(R_O)+N_O        ; store 0 imag
                move    sinp:(R_I1)+N_I1,B      ; fetch real
            else
                if "sinp"=='x'
                    move    sinp:(R_I1)+N_I1,B  A,sout:(R_O)+N_O
                else
                    move    A,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,B 
                endif
            endif
pf\_cvreal_\ic\_loop
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

