;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      09/20/87/jms - initial file created from DSPAPSRC/template
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vmax (AP macro) - vector max - max two vectors, creating a third
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vmax    pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vmax_\ic is globally unique)
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
;;      The vmax array-processor macro computes C=A MAX B.
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
;;          sout:aout[n*iout] = 
;;              sinp_a:ainp_a[n*iinp_a] MAX sinp_b:ainp_b[n*iinp_b];
;;      
;;  MEMORY USE 
;;      7 x-memory arguments
;;      program memory locations:
;;                              sout==x     sout==y
;;          sinp_a   sinp_b 
;;          x           x           36          34 
;;          x           y           32          34 
;;          y           x           34          34 
;;          y           y           34          36 
;;      
;;  EXECUTION TIME 
;;      cnt even: cnt*4+30 instruction cycles
;;      cnt odd:  cnt*4+32 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vmax (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvmax(A, iA, B, iB, C, iC, n);    /* vector max:
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
;;      beg_ap  'tvmax'
;;      symobj  ax_vec,bx_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 1,2,3,4,5,6,7,8,9,10,11,12
;;bx_vec    dc 101,102,103,104,105,106,107,108,109,110,111,112
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,12,$777
;;      vmax    ap,1,x,ax_vec,1,x,bx_vec,1,x,out1_vec,1,12
;;      end_ap  'tvmax'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vmax.asm
;;      
;;  REGISTER USE 
;;      A       accumulator; temp for testing for cnt==0
;;      B       accumulator
;;      X0,X1   A[n]
;;      Y0      cnt/2
;;      Y1      flag for cnt odd/even
;;      R_I1    running pointer to B (!) vector terms
;;      N_I1    increment for B vector
;;      R_I2    running pointer to A vector terms
;;      N_I2    increment for A vector
;;      R_O     running pointer to C vector terms
;;      N_O     increment for C vector
;;      
vmax    macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vmax_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vmax_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vmax_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vmax_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vmax_\ic\_,aout,aout0
        new_xarg    pf\_vmax_\ic\_,iout,iout0
        new_xarg    pf\_vmax_\ic\_,cnt,cnt0

        ; get inputs
        ;;  the choice of R_I2 for *A* and R_I1 for *B* is dictacted by
        ;;  address register requirements for xy move, below
        move            x:(R_X)+,R_I2   ; input vector A base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector A address increment to N_I2
        move            x:(R_X)+,R_I1   ; input vector B base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector B address increment to N_I1
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        clr      A      x:(R_X)+,B      ; count to B

        ; set up loop and pipelining
        lsr     B                       ; B gets cnt/2 
        jcc     pf\_vmax_\ic\_l1
        move    #1,A1                   ; A gets "cnt odd" flag
pf\_vmax_\ic\_l1     
        move            A1,Y1           ; Y1 gets "cnt even/odd" flag
        if "sinp_a"=="sinp_b"||"sinp_a"=='y'
            move        sinp_a:(R_I2)+N_I2,X0
            move        sinp_b:(R_I1)+N_I1,A
        else
            move        sinp_a:(R_I2)+N_I2,X0   sinp_b:(R_I1)+N_I1,A
        endif
        cmp     X0,A    B1,Y0           ; copy cnt0/2 into Y0 for loop
        tlt     X0,A
        tst     B                       ; guard against cnt=0 or cnt=1
        jeq     pf\_vmax_\ic\_l3   
        if "sinp_a"=="sinp_b"||"sinp_a"=='y'
            move        sinp_a:(R_I2)+N_I2,X1      
            move        sinp_b:(R_I1)+N_I1,B
        else
            move        sinp_a:(R_I2)+N_I2,X1   sinp_b:(R_I1)+N_I1,B
        endif

        ; inner loop 
        do      Y0,pf\_vmax_\ic\_l3
            move            sinp_a:(R_I2)+N_I2,X0     
            if "sinp_b"=="sout" 
                cmp     X1,B    A,sout:(R_O)+N_O       
                move            sinp_b:(R_I1)+N_I1,A
            else
                if "sinp_b"=='x'
                    cmp     X1,B    sinp_b:(R_I1)+N_I1,A    A,sout:(R_O)+N_O       
                else
                    cmp     X1,B    A,sout:(R_O)+N_O        sinp_b:(R_I1)+N_I1,A
                endif
            endif
            tlt     X1,B
            move            sinp_a:(R_I2)+N_I2,X1     
            if "sinp_b"=="sout" 
                cmp     X0,A    B,sout:(R_O)+N_O       
                move            sinp_b:(R_I1)+N_I1,B
            else
                if "sinp_b"=='x'
                    cmp     X0,A    sinp_b:(R_I1)+N_I1,B    B,sout:(R_O)+N_O       
                else
                    cmp     X0,A    B,sout:(R_O)+N_O        sinp_b:(R_I1)+N_I1,B
                endif
            endif
            tlt     X0,A
pf\_vmax_\ic\_l3                                ; jump to here for cnt=0 or cnt=1
        move            Y1,B
        tst     B    
        jeq     pf\_vmax_\ic\_l2                ; if cnt odd (including cnt=1),
        move            A,sout:(R_O)+N_O        ; store final element
pf\_vmax_\ic\_l2
        endm

