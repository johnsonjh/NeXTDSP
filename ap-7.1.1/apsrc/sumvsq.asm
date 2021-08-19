;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn 
;;      
;;  Modification history
;;  --------------------
;;      09/10/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - change "...\ic\_\l..." to "...\ic\_l..."
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;	05/19/89/mtm - fix DSPWRAP C SYNTAX.
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      sumvsq (AP macro) - sum of vector squares
;;          - sum squares of vector elements to a scalar
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      sumvsq   pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0; invoke ap macro sumvsq
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_sumvsq_\ic is globally unique)
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
;;      The sumvsq array-processor macro computes the sum of the squares
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
;;          sout:aout += sinp:ainp[n*iinp] * sinp:ainp[n*iinp];
;;      }
;;      
;;  MEMORY USE
;;      4 x-memory arguments
;;      12 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt*4+14 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      sumvsq (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPsumvsq(A,ia,B,n);  /* sum of vector element squares:
;;          A   =   input vector base address in DSP memory
;;          ia  =   A address increment
;;          B   =   address of output
;;          n   =   element count       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'stdmacro'
;;      beg_ap  'tsumvsq'
;;      symobj  tsumvsq_vec
;;      beg_yeb
;;tsumvsq_vec dc -10000,20000,-30000,40000,-50000,60000,-70000,80000,-90000,100000,-110000,120000
;;      end_yeb
;;      new_xeb sum1,1,$777
;;      sumvsq   ap,1,y,tsumvsq_vec,1,x,sum1,12
;;      end_ap  'tsumvsq'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/sumvsq.asm
;;      
;;  REGISTER USE 
;;      A       accumulator
;;      B       cnt
;;      X0      current element
;;      Y0      current element
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     pointer to ouptut term
;;      
sumvsq   macro pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0

        ; argument declarations
        new_xarg    pf\_sumvsq_\ic\_,ainp,ainp0     ; input address arg
        new_xarg    pf\_sumvsq_\ic\_,iinp,iinp0     ; input increment arg
        new_xarg    pf\_sumvsq_\ic\_,aout,aout0     ; output address arg
        new_xarg    pf\_sumvsq_\ic\_,cnt,cnt0       ; element count arg

        ; get inputs
        move            x:(R_X)+,R_I1
        move            x:(R_X)+,N_I1
        move            x:(R_X)+,R_O    ; output address goes to R_O
        clr     A       x:(R_X)+,B      ; cnt goes to B

        ; set up loop and pipelining
        tst      B
        jeq      pf\_sumvsq_\ic\_l1      ; guard against cnt=0
        move             sinp:(R_I1),Y0 ; move input element to Y0

        ; inner loop 
        do   B1,pf\_sumvsq_\ic\_l1
             move           sinp:(R_I1)+N_I1,X0 ; move same element to X0
             mac    X0,Y0,A sinp:(R_I1),Y0      ; square, accumulate
pf\_sumvsq_\ic\_l1  
        rnd     A
        move            A,sout:(R_O)            ; A=0 when cnt=0
        endm

