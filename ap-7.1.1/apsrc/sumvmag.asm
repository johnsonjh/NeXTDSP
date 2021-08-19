;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn 
;;      
;;  Modification history
;;  --------------------
;;      09/09/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - change "...\ic\_\l..." to "...\ic\_l..."
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;	05/19/89/mtm - fix DSPWRAP C SYNTAX.
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      sumvmag (AP macro) - vector magnitude sum 
;;          - sum vector element magnitudes to a scalar
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      sumvmag   pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0; invoke ap macro sumvmag
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_sumvmag_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   initial input vector memory address
;;      iinp0   =   initial increment for input vector address
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial address for output scalar
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    Input Vector base address       ainp0
;;      x:(R_X)+    Input Address increment         iinp0
;;      x:(R_X)+    Output                          aout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The sumvmag array-processor macro computes the sum of magnitudes 
;;      of the elements of vector ainp.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;      
;;      aout = 0;
;;      for (n=0;n<cnt;n++) {
;;          sout:aout += abs(sinp:ainp[n*iinp]);
;;      }
;;      
;;  MEMORY USE
;;      4 x-memory arguments
;;      14 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt*4+14 instruction cycles 
;;      
;;  DSPWRAP ARGUMENT INFO
;;      sumvmag (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPsumvmag(A,ia,B,n);  /* vector element magnitude sum:
;;          A   =   input vector base address in DSP memory
;;          ia  =   A address increment
;;          B   =   address of output
;;          n   =   element count       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'stdmacro'
;;      beg_ap  'tsumvmag'
;;      symobj  tsumvmag_vec
;;      beg_yeb
;;tsumvmag_vec dc -1,2,-3,4,-5,6,-7,8,-9,10,-11,12
;;      end_yeb
;;      new_xeb sum1,1,$777
;;      sumvmag   ap,1,y,tsumvmag_vec,1,x,sum1,12
;;      end_ap  'tsumvmag'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/sumvmag.asm
;;      
;;  REGISTER USE 
;;      A       test for cnt == 0; holds current addend 
;;      B       sum
;;      X1      cnt
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     pointer to ouptut term
;;      
sumvmag   macro pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0

        ; argument declarations
        new_xarg    pf\_sumvmag_\ic\_,ainp,ainp0     ; input address arg
        new_xarg    pf\_sumvmag_\ic\_,iinp,iinp0     ; input increment arg
        new_xarg    pf\_sumvmag_\ic\_,aout,aout0     ; output address arg
        new_xarg    pf\_sumvmag_\ic\_,cnt,cnt0       ; element count arg

        ; get inputs
        move            x:(R_X)+,R_I1
        move            x:(R_X)+,N_I1
        move            x:(R_X)+,R_O    ; output address goes to R_O
        move            x:(R_X)+,A      ; cnt goes to A

        ; set up loop and pipelining
        clr     B        A,X1           ; copy cnt into X1
        tst     A
        jeq     pf\_sumvmag_\ic\_l1       ; guard against cnt=0
        move    sinp:(R_I1)+N_I1,A

        ; inner loop 
        do      X1,pf\_sumvmag_\ic\_l1
            abs     A
            add     A,B     sinp:(R_I1)+N_I1,A
pf\_sumvmag_\ic\_l1  
        move            B,sout:(R_O)    ; B=0 when cnt=0
        endm

