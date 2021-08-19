;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      09/11/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - remove "andi #$fe,ccr"; replace ror with lsr, both in initialization 
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;	05/30/89/mtm - fixed a comment
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vpvnolim (AP macro) - vector add no limiting
;;          - add two vectors, creating a third, suppressing 56000 limiting on output
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vpvnolim pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vpvnolim_\ic is globally unique)
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
;;      The vpvnolim array-processor macro computes C=A+B, with on-chip
;;      limiting suppressed.
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
;;      sinp_a==sinp_b: 34 program memory locations 
;;      sinp_a!=sinp_b: 32 program memory locations 
;;      
;;  EXECUTION TIME
;;      cnt even: cnt*3+30 instruction cycles
;;      cnt odd:  cnt*3+32 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vpvnolim (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvpvnolim(A, iA, B, iB, C, iC, n);   /* vector add with no limiting:
;;          A   =   vector A base address in DSP memory
;;          iA  =   A address increment
;;          B   =   vector B base address in DSP memory
;;          iB  =   B address increment
;;          C   =   vector C base address in DSP memory
;;          iC  =   C address increment
;;          n   =   element count                           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvpvnolim'
;;      symobj  ax_vec,bx_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 800001,800002,800003,800004,800005,800006,800007,800008,800009,800010,800011,800012
;;bx_vec    dc 800101,800102,800103,800104,800105,800106,800107,800108,800109,800110,800111,800112
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,12,$777
;;      vpvnolim ap,1,x,ax_vec,1,x,bx_vec,1,x,out1_vec,1,12
;;      end_ap  'tvpvnolim'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vpvnolim.asm
;;      
;;  REGISTER USE 
;;      A,B     accumulator
;;      X0,X1,Y0,Y1 vector elements
;;      R_I1    running pointer to A vector terms
;;      N_I1    increment for A vector
;;      R_I2    running pointer to B vector terms
;;      N_I2    increment for B vector
;;      R_O     running pointer to C vector terms
;;      N_O     increment for C vector
;;      N_L     flag for cnt odd or even
;;      N_X     copy of cnt/2
;;      
vpvnolim macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vpvnolim_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vpvnolim_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vpvnolim_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vpvnolim_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vpvnolim_\ic\_,aout,aout0
        new_xarg    pf\_vpvnolim_\ic\_,iout,iout0
        new_xarg    pf\_vpvnolim_\ic\_,cnt,cnt0

        ; get inputs
        ;;  the choice of R_I2 for *A* and R_I1 for *B* is dictacted by
        ;;  address register requirements for xy move, below
        move            x:(R_X)+,R_I1   ; input vector A base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector A address increment to N_I1
        move            x:(R_X)+,R_I2   ; input vector B base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector B address increment to N_I2
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        clr     A       x:(R_X)+,B      ; count to B

        ; set up loop and pipelining
        lsr     B                       ; B gets cnt/2
        jcc     pf\_vpvnolim_\ic\_l1
        move            #1,A1           ; A gets "cnt odd" flag
pf\_vpvnolim_\ic\_l1   move B1,N_X     ; copy cnt0/2 into N_X for loop
        move            A1,N_L          ; N_L gets "cnt even/odd" flag
        move            sinp_a:(R_I1)+N_I1,X0
        move            sinp_b:(R_I2)+N_I2,A
        add     X0,A
        tst     B                       ; guard against cnt=0 or cnt=1
        jeq     pf\_vpvnolim_\ic\_l3   ; protect againt count=0
        if "sinp_a"=="sinp_b"||"sinp_a"=='x'
            move            sinp_a:(R_I1)+N_I1,X1
        else
            move            sinp_a:(R_I1)+N_I1,Y1
        endif
        move            sinp_b:(R_I2)+N_I2,B

        ; inner loop
        do      N_X,pf\_vpvnolim_\ic\_l3
            if "sinp_a"=="sinp_b"
                move            A1,sout:(R_O)+N_O
                add     X1,B    sinp_a:(R_I1)+N_I1,X0
                move            sinp_b:(R_I2)+N_I2,A
                move            B1,sout:(R_O)+N_O 
                add     X0,A    sinp_a:(R_I1)+N_I1,X1
                move            sinp_b:(R_I2)+N_I2,B
            else
                if "sinp_a"=='x'
                    move            A1,sout:(R_O)+N_O
                    add     X1,B    sinp_a:(R_I1)+N_I1,X0    sinp_b:(R_I2)+N_I2,A
                    move            B1,sout:(R_O)+N_O 
                    add     X0,A    sinp_a:(R_I1)+N_I1,X1    sinp_b:(R_I2)+N_I2,B
                else
                    move            A1,sout:(R_O)+N_O
                    add     Y1,B    sinp_b:(R_I2)+N_I2,A     sinp_a:(R_I1)+N_I1,Y0
                    move            B1,sout:(R_O)+N_O 
                    add     Y0,A    sinp_b:(R_I2)+N_I2,B     sinp_a:(R_I1)+N_I1,Y1
                endif
            endif
pf\_vpvnolim_\ic\_l3
        move            N_L,B
        tst     B    
        jeq     pf\_vpvnolim_\ic\_l2       ; if cnt odd (including cnt=1),
        move            A1,sout:(R_O)+N_O   ; store final element
pf\_vpvnolim_\ic\_l2
        endm

