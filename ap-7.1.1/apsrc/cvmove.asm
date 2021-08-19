;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/12/87/jms - initial file created from cvtcv.asm
;;      02/15/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      06/12/89/mtm - rearrange comments to match dspwrap
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvmove (AP macro)    - complex vector move 
;;          - copy complex vector from one location to another
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvmove   pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0 ; invoke cvmove
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvmove_\ic is globally unique)
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
;;      The cvmove array-processor macro copies the elements of one complex 
;;      vector to another vector of the same length.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;   /* input address        */
;;      iinp    =   x:(R_X)+;   /* input increment      */
;;      aout    =   x:(R_X)+;   /* output address       */
;;      iout    =   x:(R_X)+;   /* output increment     */
;;      cnt     =   x:(R_X)+;   /* number of elements   */
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout]   = sinp:ainp[n*iinp];     /* real part       */
;;          sout:aout[n*iout+1] = sinp:ainp[n*iinp+1];   /* imaginary part  */
;;      }
;;      
;;  MEMORY USE
;;      5 x-memory arguments
;;      sinp==sout: 19 program memory locations
;;      sinp!=sout: 17 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt*4+17 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      cvmove (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout, (output)aout, iout, cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPcvmove(S,iS,D,iD,n);  /* complex vector move: D[k]=S[k]
;;          S   =   Source vector base address
;;          iS  =   S address increment (2 for successive elements)
;;          D   =   Destination vector base address
;;          iD  =   D address increment (2 for successive elements)
;;          n   =   element count (number of complex values)    */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tcvmove'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc a11aa,a12aa,a21aa,a22aa,a31aa,a32aa,a41aa,a42aa,a51aa,a52aa,a61aa,a62aa
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      cvmove   ap,1,x,ax_vec,2,x,out1_vec,2,2
;;      end_ap  'tcvmove'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvmove.asm
;;      
;;  REGISTER USE  
;;      A       real part of input/output
;;      B       imag part of input/output
;;      Y1      temporary storage for constant=1
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector 
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
cvmove   macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_cvmove_\ic\_,ainp,ainp0
        new_xarg    pf\_cvmove_\ic\_,iinp1,iinp0
        new_xarg    pf\_cvmove_\ic\_,aout,aout0
        new_xarg    pf\_cvmove_\ic\_,iout,iout0
        new_xarg    pf\_cvmove_\ic\_,cnt,cnt0

        ; get inputs
        move            #>1,Y1
        move            x:(R_X)+,R_I1   ; input vector 1 address
        move            x:(R_X)+,A      ; input vector 1 increment 
        sub     Y1,A    x:(R_X)+,R_O    ; output vector address to R_O
        move            x:(R_X)+,B      ; output vector increment
        sub     Y1,B    A,N_I1          ; decremented input vector 1 increment to N_I1
        move            B,N_O           ; decremented output vector increment to N_O
        move            x:(R_X)+,B      ; count
     
        ; set up loop and pipelining
        tst      B      sinp:(R_I1)+,A  ; fetch real 
        jeq      pf\_cvmove_\ic\_loop    ; test B to protect against count=0

        ; inner loop 
        do      B,pf\_cvmove_\ic\_loop
            if "sinp"=="sout"
                move    A,sout:(R_O)+           ; store real
                move    sinp:(R_I1)+N_I1,B      ; fetch imag
                move    B,sout:(R_O)+N_O        ; store imag
                move    sinp:(R_I1)+,A          ; fetch real
            else
                if "sinp"=='x'
                    move    sinp:(R_I1)+N_I1,B  A,sout:(R_O)+       ; fetch imag, store real
                    move    sinp:(R_I1)+,A      B,sout:(R_O)+N_O    ; fetch real, store imag
                else
                    move    A,sout:(R_O)+       sinp:(R_I1)+N_I1,B  ; fetch imag, store real
                    move    B,sout:(R_O)+N_O    sinp:(R_I1)+,A      ; fetch real, store imag
                endif
            endif
pf\_cvmove_\ic\_loop
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

