;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/13/87/jms - initial file created from DSPAPSRC/cvfill.asm
;;      02/15/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;	05/18/89/mtm - fixed DSPWRAP C SYNTAX
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvfilli (AP macro) - immediate complex vector fill 
;;          - fill complex vector with immediate constant complex value
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvfilli pf,ic,cnstr0,cnsti0,sout,aout0,iout0,cnt0 ; invoke ap macro cvfilli
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvtcv_\ic is globally unique)
;;      cnstr0  =   real part of fill constant
;;      cnsti0  =   imag part of fill constant
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   initial increment for output vector
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    fill constant, real part        cnstr0
;;      x:(R_X)+    fill constant, imag part        cnsti0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The cvfilli array-processor macro loads the elements of a complex vector
;;      with a complex constant passed as an argument.
;;
;;  PSEUDO-C NOTATION
;;      cnstr   =   x:(R_X)+;       /* real part of fill constant   */
;;      cnsti   =   x:(R_X)+;       /* imag part of fill constant   */
;;      aout    =   x:(R_X)+;       /* output address               */
;;      iout    =   x:(R_X)+;       /* output increment             */
;;      cnt     =   x:(R_X)+;       /* number of complex elements   */     
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout]   = cnstr;    /* real part            */
;;          sout:aout[n*iout+1] = cnsti;    /* imaginary part       */
;;      }
;;      
;;  MEMORY USE
;;      5 x-memory arguments
;;      16 program memory locations 
;;      
;;  EXECUTION TIME
;;      cnt*2+16 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      cvfilli (prefix)pf,(instance)ic,
;;          cnstr,cnsti,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPcvfilli(cr,ci,D,iD,n);   /* complex vector fill immediate: D[k]=c
;;          cr   =  real part of complex fill-constant
;;          ci   =  imaginary part of complex fill-constant
;;          D   =   destination vector base address (points to real part of 1st element)
;;          iD  =   y address increment (2 for successive elements)
;;          n   =   element count (number of complex values)    */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tcvfilli'
;;      new_xeb out1_vec,7,$777
;;      cvfilli ap,1,$a11aa,$a12aa,x,out1_vec,2,3
;;      end_ap  'tcvfilli'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvfilli.asm
;;      
;;  REGISTER USE  
;;      B       count
;;      X0      temporary storage for constant=1
;;      Y0      real part of constant vector
;;      Y1      imag part of constant vector
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
cvfilli macro pf,ic,cnstr0,cnsti0,sout,aout0,iout0,cnt0 

        ; argument definitions
        new_xarg    pf\_cvfilli_\ic\_,cnstr,cnstr0
        new_xarg    pf\_cvfilli_\ic\_,cnsti,cnsti0
        new_xarg    pf\_cvfilli_\ic\_,aout,aout0
        new_xarg    pf\_cvfilli_\ic\_,iout,iout0
        new_xarg    pf\_cvfilli_\ic\_,cnt,cnt0

        ; get inputs
        move            x:(R_X)+,Y0     ; real constant
        move            x:(R_X)+,Y1     ; imag constant
        move            #>1,X0
        move            x:(R_X)+,R_O    ; output vector address
        move            x:(R_X)+,A      ; output vector decrement
        sub     X0,A    x:(R_X)+,B      ; count
        move            A,N_O           ; decremented output vector increment to N_O
     
        ; set up loop 
        tst     B
        jeq     pf\_cvfilli_\ic\_loop   ; test B to protect against count=0

        ; inner loop 
        do      B,pf\_cvfilli_\ic\_loop
            move    Y0,sout:(R_O)+      ; real
            move    Y1,sout:(R_O)+N_O   ; imag
pf\_cvfilli_\ic\_loop
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

