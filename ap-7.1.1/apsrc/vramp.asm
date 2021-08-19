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
;;      vramp (AP macro) - vector ramp - fill a vector with a ramp function
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vramp   pf,ic,sar,ar0,sari,ari0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vramp_\ic is globally unique)
;;      sar     =   initial ramp value memory space ('x' or 'y')
;;      ar0     =   initial ramp value address 
;;      sari    =   ramp increment memory space ('x' or 'y')
;;      ari0    =   initial ramp increment
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   initial increment for output vector 
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS 
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    address of initial ramp value   ar0
;;      x:(R_X)+    address of ramp increment       ari0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vramp array-processor macro computes D[aD+k*iD] = r+k*i, k=0:n-1.
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
;;      13 program memory locations 
;;      
;;  EXECUTION TIME 
;;      cnt*2+14 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vramp (prefix)pf,(instance)ic,
;;          (dspace)sar,ar,(dspace)sari,ari,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvramp(ar,ari,aD,iD,n);   /* Vector ramp: D = r+k*i, k=0:n-1
;;          ar  =   address of initial ramp value
;;          ari =   address of ramp increment
;;          aD  =   Destination vector base address
;;          iD  =   Address increment
;;          n   =   element count               */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvramp'
;;      new_xeb xar,1,5
;;      new_xeb xari,1,$10
;;      new_xeb out1_vec,7,$777
;;      vramp   ap,1,x,xar,x,xari,x,out1_vec,1,6
;;      end_ap  'tvramp'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vramp.asm
;;      
;;  REGISTER USE  
;;      A       accumulator
;;      B       temp for testing for cnt==0
;;      X0      ari
;;      R_I1    point to ar
;;      R_I2    point to ari
;;      R_O     running pointer to ouput vector terms
;;      N_O     increment for output vector
;;      
vramp   macro pf,ic,sar,ar0,sari,ari0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vramp_\ic\_,ainp,ar0
        new_xarg    pf\_vramp_\ic\_,ari,ari0
        new_xarg    pf\_vramp_\ic\_,aout,aout0
        new_xarg    pf\_vramp_\ic\_,iout,iout0
        new_xarg    pf\_vramp_\ic\_,cnt,cnt0

        ; get inputs
        move            x:(R_X)+,R_I1   ; address of initial ramp value to R_I1
        move            x:(R_X)+,R_I2   ; address of ramp increment to X0
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,B      ; count

        ; set up loop and pipelining
        tst     B
        jeq     pf\_vramp_\ic\_l1       ; protect againt count=0
        move    sar:(R_I1),A
        move    sari:(R_I2),X0

        ; inner loop
        rep     B
            add     X0,A    A,sout:(R_O)+N_O
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; after a rep. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after a rep. 
pf\_vramp_\ic\_l1
        endm

