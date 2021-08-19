;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      06/24/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - changed neg A to tst A in initialization
;;                   - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      05/30/89/mtm - fix bug in comment
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vswap (AP macro) - vector swap 
;;          - swap elements of two vectors
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vswap   pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vswap_\ic is globally unique)
;;      sinp_a  =   input vector A memory space ('x' or 'y')
;;      ainp_a0 =   input vector A base address (address of A[1,1])
;;      iinp_a0 =   initial increment for input vector A
;;      sinp_b  =   input vector B memory space ('x' or 'y')
;;      ainp_b0 =   input vector B base address
;;      iinp_b0 =   initial increment for input vector B
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector A base address     ainp_a0
;;      x:(R_X)+    input vector A increment        iinp_a0
;;      x:(R_X)+    input vector B base address     ainp_b0
;;      x:(R_X)+    input vector B increment        iinp_b0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vswap array-processor macro swaps elements of two vectors.
;;
;;  PSEUDO-C NOTATION
;;      ainp_a  =   x:(R_X)+;    
;;      iinp_a  =   x:(R_X)+;    
;;      ainp_b  =   x:(R_X)+;    
;;      iinp_a  =   x:(R_X)+;    
;;      cnt     =   x:(R_X)+;    
;;
;;      for (n=0;n<cnt;n++) {
;;          tmp = sinp_a:ainp_a[n*iinp_a]; 
;;          sinp_a:ainp_a[n*iinp_a] = sinp_b:ainp_b[n*iinp_b];
;;          sinp_b:ainp_b[n*iinp_b] = tmp;
;;      }
;;      
;;  MEMORY USE 
;;      5 x-memory arguments
;;      sinp_a==sinp_b: 15 program memory locations 
;;      sinp_a!=sinp_b: 13 program memory locations 
;;      
;;  EXECUTION TIME
;;      cnt*4+13 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vswap (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvswap(aA,iA,aB,iB,n);    /* swap vectors A and B:
;;          aA  =   A vector base address
;;          iA  =   A address increment
;;          aB  =   B vector base address
;;          iB  =   B address increment
;;          n   =   element count               */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvswap'
;;      symobj  aX1_vec,bX1_vec
;;      beg_xeb
;;          radix   16
;;aX1_vec   dc a11,a12,a13,a14,a15,a16
;;bX1_vec   dc b11,b12,b13,b14,b15,b16
;;          radix   A  
;;      end_xeb
;;      vswap   ap,1,x,aX1_vec,1,x,bX1_vec,1,6
;;      end_ap  'tvswap'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vswap.asm
;;      
;;  REGISTER USE 
;;      A       term from A vector; temp for testing for cnt==0
;;      B       term from B vector
;;      X0      cnt
;;      R_I1    running pointer to A vector terms
;;      N_I1    increment for A vector
;;      R_I2    running pointer to B vector terms
;;      N_I2    increment for B vector
;;      
vswap   macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,cnt0

        ; argument definitions
        new_xarg    pf\_vswap_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vswap_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vswap_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vswap_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vswap_\ic\_,cnt,cnt0
     
        ; get inputs
        move            x:(R_X)+,R_I1   ; input vector A base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector A address increment to N_I1
        move            x:(R_X)+,R_I2   ; input vector B base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector B address increment to N_I2
        move            x:(R_X)+,A      ; count

        ; set up loop and pipelining
        tst     A       A,X0            ; squirrel away count for do loop
        jeq     pf\_vswap_\ic\_loop1    ; protect againt count=0

        ; inner loop
        do      X0,pf\_vswap_\ic\_loop1
            if "sinp_a"=="sinp_b"
                move    sinp_a:(R_I1),A
                move    sinp_b:(R_I2),B
                move    A,sinp_b:(R_I2)+N_I2
                move    B,sinp_a:(R_I1)+N_I1
            else
                if "sinp_a"=='x'
                    move    sinp_a:(R_I1),A         sinp_b:(R_I2),B
                    move    B,sinp_a:(R_I1)+N_I1    A,sinp_b:(R_I2)+N_I2     
                else
                    move    sinp_b:(R_I2),B         sinp_a:(R_I1),A     
                    move    B,sinp_a:(R_I1)+N_I1    A,sinp_b:(R_I2)+N_I2     
                endif
            endif
pf\_vswap_\ic\_loop1
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

