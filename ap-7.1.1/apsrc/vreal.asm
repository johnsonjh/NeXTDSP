;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/12/87/jms - initial file created from DSPAPSRC/cvtcv.asm
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vreal (AP macro) - vector real part
;;          - form a real vector from the real part of a complex vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vreal   pf,1,ainp0,iinp0,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vreal_\ic is globally unique)
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
;;      The vreal array-processor macro computes C[k]=Real(A[k]), where A[k]
;;      is complex and C[k] is not.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;  
;;      iinp    =   x:(R_X)+;  
;;      aout    =   x:(R_X)+;  
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;  
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout] = sinp:ainp[n*iinp];
;;      }
;;      
;;  MEMORY USE
;;  See vmove
;;      
;;  EXECUTION TIME
;;  See vmove
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vreal (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout, (output)aout, iout, cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvreal(aS,iS,aD,iD,n);    /* Extract real part: D[k] = Real(S[k])
;;          aS  =   S source vector base address (S complex)
;;          iS  =   S address increment (2 for successive elements)
;;          aD  =   D destination vector base address (D real = real part of S)
;;          iD  =   D address increment
;;          n   =   element count (number of complex values for S)  */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvreal'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc a11aa,a12aa,a21aa,a22aa,a31aa,a32aa,a41aa,a42aa,a51aa,a52aa,a61aa,a62aa
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      vreal   ap,1,x,ax_vec,2,x,out1_vec,1,3
;;      end_ap  'tvreal'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vreal.asm
;;      
;;  REGISTER USE  
;;  See vmove
;;      
vreal   macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        vmove pf\_vreal,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        endm

