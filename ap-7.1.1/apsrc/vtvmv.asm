;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      09/01/87/jms - initial file created from DSPAPSRC/template
;;      02/17/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vtvmv (AP macro) - vector multiply then subtract
;;          - pointwise multiplication of two vectors, subtract another vector 
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vtvmv  pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sinp_c,ainp_c0,iinp_c0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vtvmv_\ic is globally unique)
;;      sinp_a  =   input vector A memory space ('x' or 'y')
;;      ainp_a0 =   input vector A base address (address of A[1,1])
;;      iinp_a0 =   initial increment for input vector A
;;      sinp_b  =   input vector B memory space ('x' or 'y')
;;      ainp_b0 =   input vector B base address
;;      iinp_b0 =   initial increment for input vector B
;;      sinp_c  =   input vector C memory space ('x' or 'y')
;;      ainp_c0 =   input vector C base address
;;      iinp_c0 =   initial increment for input vector C
;;      sout    =   output vector D memory space ('x' or 'y')
;;      aout0   =   output vector D base address
;;      iout0   =   initial increment for output vector D 
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector A base address     ainp_a0
;;      x:(R_X)+    input vector A increment        iinp_a0
;;      x:(R_X)+    input vector B base address     ainp_b0
;;      x:(R_X)+    input vector B increment        iinp_b0
;;      x:(R_X)+    input vector C base address     ainp_c0
;;      x:(R_X)+    input vector C increment        iinp_c0
;;      x:(R_X)+    output vector D base address    aout0
;;      x:(R_X)+    output vector D increment       iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vtvmv array-processor macro computes D[k]=A[k]*B[k]-C[k].
;;
;;  PSEUDO-C NOTATION
;;      ainp_a  =   x:(R_X)+;
;;      iinp_a  =   x:(R_X)+;
;;      ainp_b  =   x:(R_X)+;
;;      iinp_b  =   x:(R_X)+;
;;      ainp_c  =   x:(R_X)+;
;;      iinp_c  =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = 
;;              sinp_a:ainp_a[n*iinp_a] * sinp_b:ainp_b[n*iinp_b] - 
;;              sinp_c:ainp_c[n*iinp_c]; 
;;      
;;  MEMORY USE 
;;      9 x-memory arguments
;;      24  program memory locations 
;;            +2 program memory locations if sinp_a==sinp_b
;;            +1 program memory location if sinp_c==sout
;;      
;;  EXECUTION TIME 
;;      sinp_a==sinp_b: cnt*4+25 instruction cycles
;;      sinp_a!=sinp_b: cnt*4+24 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vtvmv (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,
;;          (dspace)sinp_c,(input)ainp_c,iinp_c,
;;          (dspace)sout,(output)aout,iout,cnt
;;          
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvtvmv(aS1,iS1,aS2,iS2,aS3,iS3,aD,iD,n);   /* D[k]=S1[k]*S2[k]-S3[k]
;;          aS1 =   Source vector 1 base address
;;          iS1 =   S1 address increment
;;          aS2 =   Source vector 2 base address
;;          iS2 =   S2 address increment
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count                               */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvtvmv'
;;      symobj  ax_vec,bx_vec,cx_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff
;;bx_vec    dc 721fff,722fff,723fff,724fff,725fff,726fff,727fff
;;cx_vec    dc 701,702,703,704,705,706,707
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      vtvmv    ap,1,x,ax_vec,1,x,bx_vec,1,x,cx_vec,1,x,out1_vec,1,7
;;      end_ap  'tvtvmv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vtvmv.asm
;;      
;;  REGISTER USE  
;;      A       accumulator 
;;      B       temp for testing for cnt==0
;;      X0,Y0   current A,B terms
;;      X1,Y1   constant (-1) and current C term
;;      R_I1    running pointer to A vector terms
;;      N_I1    increment for A vector
;;      R_I2    running pointer to B vector terms
;;      N_I2    increment for B vector
;;      R_L     running pointer to C vector terms
;;      N_L     increment for C vector terms
;;      R_O     running pointer to output (D) vector terms
;;      N_O     increment for output (D) vector
;;      N_X     temporary storage for R_L
;;      
vtvmv    macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sinp_c,ainp_c0,iinp_c0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vtvmv_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vtvmv_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vtvmv_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vtvmv_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vtvmv_\ic\_,ainp_c,ainp_c0
        new_xarg    pf\_vtvmv_\ic\_,iinp_c,iinp_c0
        new_xarg    pf\_vtvmv_\ic\_,aout,aout0
        new_xarg    pf\_vtvmv_\ic\_,iout,iout0
        new_xarg    pf\_vtvmv_\ic\_,cnt,cnt0

        ; get inputs
        move            R_L,N_X         ; squirrel away R_L until endm
        move            x:(R_X)+,R_I1   ; input vector A base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector A address increment to N_I1
        move            x:(R_X)+,R_I2   ; input vector B base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector B address increment to N_I2
        move            x:(R_X)+,R_L    ; input vector C base address to R_L
        move            x:(R_X)+,N_L    ; input vector C address increment to N_L
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,B      ; count

        ; set up loop and pipelining
        if "sinp_a"=="sinp_b"
            move            sinp_b:(R_I2)+N_I2,Y0
            tst     B       sinp_a:(R_I1)+N_I1,X0
        else
            if "sinp_a"=='x'
                tst     B       sinp_a:(R_I1)+N_I1,X0   sinp_b:(R_I2)+N_I2,Y0
            else
                tst     B       sinp_b:(R_I2)+N_I2,X0   sinp_a:(R_I1)+N_I1,Y0    
            endif
        endif
        jeq     pf\_vtvmv_\ic\_loop1         ; guard against cnt==0
        if "sinp_c"=='y'||"sinp_c"=="sout"
            move            #$800000,X1
            move            sinp_c:(R_L)+N_L,Y1
        else
            move            #$800000,Y1
            move            sinp_c:(R_L)+N_L,X1
        endif
        ;; We get A*B-C by first doing A*B, as one would expect. But then
        ;; we multiply C by, in effect, -1 (#$800000 from above).  Note that
        ;; the macr instruction has no minus sign.  The macr does the
        ;; subtract *and* round in one instruction. 
        mpy     X0,Y0,A sinp_a:(R_I1)+N_I1,X0
        macr    X1,Y1,A sinp_b:(R_I2)+N_I2,Y0

        ; inner loop 
        do      B,pf\_vtvmv_\ic\_loop1
            if "sinp_c"=="sout"
                mpy     X0,Y0,A A,sout:(R_O)+N_O  
                move            sinp_c:(R_L)+N_L,Y1
            else
                if "sinp_c"=='x'
                    mpy     X0,Y0,A sinp_c:(R_L)+N_L,X1 A,sout:(R_O)+N_O  
                else
                    mpy     X0,Y0,A A,sout:(R_O)+N_O    sinp_c:(R_L)+N_L,Y1
                endif
            endif
            if "sinp_a"=="sinp_b"
                macr    X1,Y1,A sinp_a:(R_I1)+N_I1,X0 
                move            sinp_b:(R_I2)+N_I2,Y0 
            else
                if "sinp_a"=='x'
                    macr    X1,Y1,A sinp_a:(R_I1)+N_I1,X0   sinp_b:(R_I2)+N_I2,Y0 
                else
                    macr    X1,Y1,A sinp_b:(R_I2)+N_I2,X0   sinp_a:(R_I1)+N_I1,Y0 
                endif
            endif
pf\_vtvmv_\ic\_loop1
        move    N_X,R_L   
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.
        endm

