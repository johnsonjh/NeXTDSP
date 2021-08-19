;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/12/87/jms - initial file created from DSPAPSRC/cvfill.asm
;;      02/15/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvfill (AP macro) - complex vector fill 
;;          - fill complex vector with a constant complex value
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvfill  pf,ic,scnstr,acnst,sout,aout0,iout0,cnt0 ; invoke ap macro cvfill
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvfill_\ic is globally unique)
;;      scnstr  =   constant vector memory space ('x' or 'y')
;;      acnst   =   constant vector base address
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   increment for output vector
;;      cnt0    =   element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    constant vector base address    acnst
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The cvfill array-processor macro loads the elements of a complex vector
;;      with a complex constant from DSP memory.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;   /* input address        */
;;      aout    =   x:(R_X)+;   /* output address       */
;;      iout    =   x:(R_X)+;   /* output increment     */
;;      cnt     =   x:(R_X)+;   /* number of elements   */
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout]   = scnstr:ainp[0];       /* real part        */
;;          sout:aout[n*iout+1] = scnstr:ainp[1];       /* imaginary part   */
;;      }
;;      
;;  MEMORY USE
;;      4 x-memory arguments
;;      17 program memory locations 
;;      
;;  EXECUTION TIME 
;;      cnt*2+17 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      cvfill (prefix)pf,(instance)ic,
;;          (dspace)scnstr,acnst,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPcvfill(C,D,iD,n);    /* complex vector fill: D[k]=C[0]
;;          C   =   address of real part of complex fill-constant in DSP memory
;;          D   =   destination vector base address (points to real part of 1st element)
;;          iD  =   y address increment (2 for successive elements)
;;          n   =   element count (number of complex values)    */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tcvfill'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc a11aa,a12aa
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      cvfill  ap,1,x,ax_vec,x,out1_vec,2,3
;;      end_ap  'tcvfill'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvfill.asm
;;      
;;  REGISTER USE  
;;      A       accumulator
;;      B       count
;;      Y0      real part of constant vector; temporary storage for constant=1
;;      Y1      imag part of constant vector
;;      R_I1    real, imaginary terms of input
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
cvfill  macro pf,ic,scnstr,acnst0,sout,aout0,iout0,cnt0 

        ; argument definitions
        new_xarg    pf\_cvfill_\ic\_,acnst,acnst0
        new_xarg    pf\_cvfill_\ic\_,aout,aout0
        new_xarg    pf\_cvfill_\ic\_,iout,iout0
        new_xarg    pf\_cvfill_\ic\_,cnt,cnt0

        ; get inputs
        move            #>1,Y1
        move            x:(R_X)+,R_I1       ; constant vector address
        move            x:(R_X)+,R_O        ; output vector address
        move            x:(R_X)+,A          ; output vector decrement
        sub     Y1,A    scnstr:(R_I1)+,Y0   ; get real part of constant
        move            A,N_O               ; decremented output vector increment to N_O
        move            scnstr:(R_I1)+,Y1   ; get imag part of constant
        move            x:(R_X)+,B          ; count
     
        ; set up loop 
        tst     B
        jeq     pf\_cvfill_\ic\_loop        ; test B to protect against count=0

        ; inner loop 
        do      B,pf\_cvfill_\ic\_loop
            move    Y0,sout:(R_O)+          ; real
            move    Y1,sout:(R_O)+N_O       ; imag
pf\_cvfill_\ic\_loop
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

