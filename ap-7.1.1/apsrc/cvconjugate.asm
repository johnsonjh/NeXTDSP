;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/13/87/jms - initial file created from DSPAPSRC/cvnegate.asm
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvconjugate (AP macro) - complex vector conjugate
;;          - form a complex vector from the elementwise conjugate a complex vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvconjugate pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvconjugate_\ic is globally unique)
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
;;      The cvconjugate array-processor macro computes C[k]=*A[k], where each
;;      term is complex and * denotes the conjugate.
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
;;          sout:aout[n*iout+1] = -sinp:ainp[n*iinp+1];
;;      }
;;      
;;  MEMORY USE
;;      5 x-memory arguments
;;      sinp==sout: 22 program memory locations
;;      sinp!=sout: 20 program memory locations
;;      
;;  EXECUTION TIME 
;;      sinp==sout: cnt*4+20 instruction cycles
;;      sinp!=sout: cnt*4+19 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      cvconjugate (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPcvconjugate(aS,iS,aD,iD,n);    /* Complex conjugate: D[k]=Sr[k]-i*Si[k]
;;          aS  =   S source vector base address (S complex)
;;          iS  =   S address increment (2 for successive elements)
;;          aD  =   D destination vector base address (D complex)
;;          iD  =   D address increment (2 for successive elements)
;;          n   =   element count (number of complex values)    */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tcvconjugate'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc a11aa,a12aa,a21aa,a22aa,a31aa,a32aa,a41aa,a42aa,a51aa,a52aa,a61aa,a62aa
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      cvconjugate   ap,1,x,ax_vec,2,x,out1_vec,2,2
;;      end_ap  'tcvconjugate'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvconjugate.asm
;;      
;;  REGISTER USE  
;;      A       real part of output; count
;;      B       imag part of output
;;      X0      constant (=1)
;;      Y0      temporary storage for count
;;      R_I1    running pointer to vector terms
;;      N_I1    increment for vector
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
cvconjugate   macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_cvconjugate_\ic\_,ainp,ainp0
        new_xarg    pf\_cvconjugate_\ic\_,iinp,iinp0
        new_xarg    pf\_cvconjugate_\ic\_,aout,aout0
        new_xarg    pf\_cvconjugate_\ic\_,iout,iout0
        new_xarg    pf\_cvconjugate_\ic\_,cnt,cnt0

        ; get inputs
        move            #>1,X0
        move            x:(R_X)+,R_I1   ; input vector address
        move            x:(R_X)+,A      ; input vector increment 
        sub     X0,A    x:(R_X)+,R_O    ; output vector address
        move            x:(R_X)+,B      ; output vector increment
        sub     X0,B    A,N_I1          ; decremented input vector increment to N_I1
        move            B,N_O           ; decremented output vector increment to N_O
        move            x:(R_X)+,A      ; count
     
        ; set up loop and pipelining
        tst     A       (R_O)-N_O       ; point R_O to datum before first output
        move            sout:(R_O),B    ; grab datum before first output
        jeq     pf\_cvconjugate_\ic\_loop     ; test A to protect against count=0
        move            A,Y0
        move            sinp:(R_I1)+,A  ; first input real

        ; inner loop 
        do      Y0,pf\_cvconjugate_\ic\_loop
            if "sinp"=="sout"
                move            B,sout:(R_O)+N_O        ; store imag
                move            sinp:(R_I1)+N_I1,B      ; get imag
                neg     B       A,sout:(R_O)+           ; store real
                move            sinp:(R_I1)+,A          ; get real
            else
                if "sinp"=='x'
                    move            sinp:(R_I1)+N_I1,B  B,sout:(R_O)+N_O
                    neg     B       sinp:(R_I1)+,A      A,sout:(R_O)+
                else
                    move            B,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,B  
                    neg     B       A,sout:(R_O)+       sinp:(R_I1)+,A
                endif
            endif
pf\_cvconjugate_\ic\_loop
        move            B,sout:(R_O)        ; Last output imag
        endm

