;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      07/23/87/jms - initial file created from DSPAPSRC/template
;;      02/17/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vrampi (AP macro) - vector ramp immediate - fill a vector with a ramp
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vrampi  pf,ic,ar0,ari0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vrampi_\ic is globally unique)
;;      ar0     =   initial ramp value 
;;      ari0    =   initial ramp increment
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   initial increment for output vector 
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS 
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    initial ramp value              ar0
;;      x:(R_X)+    ramp increment                  ari0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vrampi array-processor macro computes D[aD+k*iD] = r+k*i, k=0:n-1.
;;
;;  PSEUDO-C NOTATION
;;      ar      =   x:(R_X)+;
;;      ari     =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      sout:aout = ar;
;;      for (n=1;n<cnt;n++) 
;;          sout:aout[n*iout] = sout:aout[(n-1)*iout]+ari;
;;      
;;  MEMORY USE  
;;      5 x-memory arguments
;;      11 program memory locations 
;;      
;;  EXECUTION TIME
;;      cnt*2+12 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vrampi (prefix)pf,(instance)ic,
;;          ar,ari,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvrampi(r,ri,aD,iD,n);    /* Vector ramp: D = r+k*i, k=0:n-1
;;          r   =   initial ramp value
;;          ri  =   ramp increment
;;          aD  =   Destination vector base address
;;          iD  =   Address increment
;;          n   =   element count               */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvrampi'
;;      new_xeb out1_vec,7,$777
;;      vrampi  ap,1,5,$10,x,out1_vec,1,6
;;      end_ap  'tvrampi'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vrampi.asm
;;      
;;  REGISTER USE  
;;      A       accumulator
;;      B       temp for testing for cnt==0
;;      X0      ari
;;      R_O     running pointer to ouput vector terms
;;      N_O     increment for output vector
;;      
vrampi  macro pf,ic,ar0,ari0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vrampi_\ic\_,ainp,ar0
        new_xarg    pf\_vrampi_\ic\_,ari,ari0
        new_xarg    pf\_vrampi_\ic\_,aout,aout0
        new_xarg    pf\_vrampi_\ic\_,iout,iout0
        new_xarg    pf\_vrampi_\ic\_,cnt,cnt0

        ; get inputs
        move            x:(R_X)+,A      ; initial ramp value to A
        move            x:(R_X)+,X0     ; ramp increment to X0
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,B      ; count

        ; set up loop and pipelining
        tst     B
        jeq     pf\_vrampi_\ic\_l1       ; protect againt count=0

        ; inner loop
        rep     B
        add     X0,A    A,sout:(R_O)+N_O
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; after a rep. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after a rep. 
pf\_vrampi_\ic\_l1
        endm

