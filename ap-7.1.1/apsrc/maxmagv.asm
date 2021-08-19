;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn 
;;      
;;  Modification history
;;  --------------------
;;      09/11/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - changed pf\_maxmagv_\ic\_\l2 to pf\_maxmagv_\ic\_l1, and
;;                   - changed pf\_maxmagv_\ic\_\l3 to pf\_maxmagv_\ic\_l2
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;	05/19/89/mtm - fix DSPWRAP C SYNTAX
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      maxmagv (AP macro) - vector magnitude maximum 
;;          - write maximum of absolute value of vector elements to a scalar
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      maxmagv  pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0; invoke ap macro maxmagv
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_maxmagv_\ic is globally unique)
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
;;      The maxmagv array-processor macro finds the maximum of 
;;      the absolute value of the elements of vector ainp.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;      
;;      aout = abs(sinp:ainp[n*iinp]);
;;      for (n=0;n<cnt;n++) {
;;          if (abs(sinp:ainp[n*iinp]) > aout)
;;              aout = abs(sinp:ainp[n*iinp]);
;;      }
;;      
;;  MEMORY USE 
;;      4 x-memory arguments
;;      16 program memory locations
;;      
;;  EXECUTION TIME
;;      cnt*4+14 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      maxmagv (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,cnt
;;          
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPmaxmagv(A,ia,B,n);     /* vector maximum of magnitudes:
;;          A   =   input vector base address in DSP memory
;;          ia  =   A address increment
;;          B   =   address of output
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'stdmacro'
;;      beg_ap  'tmaxmagv'
;;      symobj  tmaxmagv_vec
;;      beg_yeb
;;tmaxmagv_vec dc -10000,20000,-30000,40000,-50000,60000,-70000,80000,-90000,100000,-110000,120000
;;      end_yeb
;;      new_xeb sum1,1,$777
;;      maxmagv  ap,1,y,tmaxmagv_vec,1,x,sum1,12
;;      end_ap  'tmaxmagv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/maxmagv.asm
;;      
;;  REGISTER USE 
;;      A       accumulator
;;      B       absolute value of current element
;;      X0      current element
;;      X1      cnt
;;      R_I1    running pointer input vector terms
;;      N_I1    increment for input vector
;;      R_O     pointer to output term
;;      
maxmagv  macro pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0

        ; argument declarations
        new_xarg    pf\_maxmagv_\ic\_,ainp,ainp0     ; input address arg
        new_xarg    pf\_maxmagv_\ic\_,iinp,iinp0     ; input increment arg
        new_xarg    pf\_maxmagv_\ic\_,aout,aout0     ; output address arg
        new_xarg    pf\_maxmagv_\ic\_,cnt,cnt0       ; element count arg

        ; get inputs	
        move        x:(R_X)+,R_I1
        move        x:(R_X)+,N_I1
        move        x:(R_X)+,R_O        ; output address goes to R_O
        move        x:(R_X)+,B          ; cnt goes to B

        ; set up loop and pipelining
        tst     B       sinp:(R_I1)+N_I1,A  ; get first element of vector
        jeq     pf\_maxmagv_\ic\_l2          ; guard against cnt=0 
        abs     A       B,X1                ; squirrel away cnt for loop
        move            A,B                 ; A and B both have absolute value
                                            ;    of first vector element

        ;;  During the first past through the loop, the abs, move, and
        ;;  cmp instructions are superfluous.  Doing things this way
        ;;  guarantees results for any cnt>=1.  The alternative would be
        ;;  to put an explicit test for cnt==1 into the loop setup.  

        ; inner loop 
        do      X1,pf\_maxmagv_\ic\_l1
            abs     B
            move            B,X0
            cmp     B,A     sinp:(R_I1)+N_I1,B 
            tlt     X0,A      
pf\_maxmagv_\ic\_l1  
        move            A,sout:(R_O)
pf\_maxmagv_\ic\_l2  
        endm

