;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      06/23/87/jms - initial file created from DSPAPSRC/template
;;      07/27/87/jms - changed underbar usage in loop and variable names
;;      09/09/87/jms - changed around tabs (cosmetic changes)
;;                   - changed "neg a" to "tst a" (after move x:(R_X)+,a)
;;      02/15/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vpv (AP macro) - vector plus vector - add two vectors, creating a third
;;          
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vpv    pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vpv_\ic is globally unique)
;;      sinp_a  =   input vector A memory space ('x' or 'y')
;;      ainp_a0 =   input vector A base address (address of A[1,1])
;;      iinp_a0 =   initial increment for input vector A
;;      sinp_b  =   input vector B memory space ('x' or 'y')
;;      ainp_b0 =   input vector B base address
;;      iinp_b0 =   initial increment for input vector B
;;      sout    =   output vector C memory space ('x' or 'y')
;;      aout0   =   output vector C base address
;;      iout0   =   initial increment for output vector C 
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector A base address     ainp_a0
;;      x:(R_X)+    input vector A increment        iinp_a0
;;      x:(R_X)+    input vector B base address     ainp_b0
;;      x:(R_X)+    input vector B increment        iinp_b0
;;      x:(R_X)+    output vector C base address    aout0
;;      x:(R_X)+    output vector C increment       iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vpv array-processor macro computes C=A+B.
;;
;;  PSEUDO-C NOTATION
;;      ainp_a  =   x:(R_X)+;
;;      iinp_a  =   x:(R_X)+;
;;      ainp_b  =   x:(R_X)+;
;;      iinp_a  =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = sinp_a:ainp_a[n*iinp_a] +
;;              sinp_b:ainp_b[n*iinp_b];
;;      
;;  MEMORY USE 
;;      7 x-memory arguments
;;      16 program memory locations 
;;          +1 program memory location if sinp_a==sinp_b
;;          +1 program memory location if sinp_b==sout
;;      
;;  EXECUTION TIME 
;;      sinp_a==sinp_b: cnt*6+17 instruction cycles
;;      sinp_a!=sinp_b: cnt*6+16 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vpv (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvpv(A, iA, B, iB, C, iC, n);    /* vector add:
;;          A   =   vector A base address in DSP memory
;;          iA  =   A address increment
;;          B   =   vector B base address in DSP memory
;;          iB  =   B address increment
;;          C   =   vector C base address in DSP memory
;;          iC  =   C address increment
;;          n   =   element count                       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvpv'
;;      symobj  ax_vec,bx_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 1,2,3,4,5,6,7,8,9,10,11,12
;;bx_vec    dc 101,102,103,104,105,106,107,108,109,110,111,112
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,12,$777
;;      vpv    ap,1,x,ax_vec,1,x,bx_vec,1,x,out1_vec,1,12
;;      end_ap  'tvpv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vpv.asm
;;      
;;  REGISTER USE 
;;      A       accumulator; temp for testing for cnt==0
;;      B       addend for each sum
;;      X0      cnt
;;      R_I1    running pointer to B (!) vector terms
;;      N_I1    increment for B vector
;;      R_I2    running pointer to A vector terms
;;      N_I2    increment for A vector
;;      R_O     running pointer to C vector terms
;;      N_O     increment for C vector
;;      
vpv    macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vpv_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vpv_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vpv_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vpv_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vpv_\ic\_,aout,aout0
        new_xarg    pf\_vpv_\ic\_,iout,iout0
        new_xarg    pf\_vpv_\ic\_,cnt,cnt0

        ; get inputs
        ;;  the choice of R_I2 for *A* and R_I1 for *B* is dictacted by
        ;;  address register requirements for xy move, below
        move            x:(R_X)+,R_I2   ; input vector A base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector A address increment to N_I2
        move            x:(R_X)+,R_I1   ; input vector B base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector B address increment to N_I1
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,A      ; count

        ; set up loop and pipelining
        tst     A       A,X0            ; squirrel away count for do loop
        jeq     pf\_vpv_\ic\_loop1     ; protect againt count=0
        if "sinp_a"=="sinp_b"
            move            sinp_b:(R_I1)+N_I1,B
            move            sinp_a:(R_I2)+N_I2,A
        else
            if "sinp_a"=='x'
                ;;  The A and B registers are the only ones that will
                ;;  legally work in the following configuration. The
                ;;  reason is that you need a register that can be both
                ;;  D1 and D2 according to the 56000 manual, appendix
                ;;  A, where it discusses XY moves.
                move            sinp_a:(R_I2)+N_I2,B     sinp_b:(R_I1)+N_I1,A
            else
                move            sinp_b:(R_I1)+N_I1,A     sinp_a:(R_I2)+N_I2,B
            endif
        endif

        ; inner loop 
        do      X0,pf\_vpv_\ic\_loop1
            add     B,A     sinp_a:(R_I2)+N_I2,B
            if "sinp_b"=="sout" 
                move        A,sout:(R_O)+N_O
                move        sinp_b:(R_I1)+N_I1,A
            else
                if "sinp_b"=='x'
                    move    sinp_b:(R_I1)+N_I1,A    A,sout:(R_O)+N_O
                else
                    move    A,sout:(R_O)+N_O        sinp_b:(R_I1)+N_I1,A
                endif
            endif
pf\_vpv_\ic\_loop1
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

