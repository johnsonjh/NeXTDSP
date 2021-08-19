;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      09/18/87/jms - initial file created from DSPAPSRC/template
;;      02/17/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vtvmvtv (AP macro) - vector multiply minus vector multiply
;;          - pointwise multiplication of two vectors, subtract pointwise multiply of two vectors
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vtvmvtv pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sinp_c,ainp_c0,iinp_c0,sinp_d,ainp_d0,iinp_d0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vtvmvtv_\ic is globally unique)
;;      sinp_a  =   input vector A memory space ('x' or 'y')
;;      ainp_a0 =   input vector A base address (address of A[1,1])
;;      iinp_a0 =   initial increment for input vector A
;;      sinp_b  =   input vector B memory space ('x' or 'y')
;;      ainp_b0 =   input vector B base address
;;      iinp_b0 =   initial increment for input vector B
;;      sinp_c  =   input vector C memory space ('x' or 'y')
;;      ainp_c0 =   input vector C base address
;;      iinp_c0 =   initial increment for input vector C
;;      sinp_d  =   input vector D memory space ('x' or 'y')
;;      ainp_d0 =   input vector D base address
;;      iinp_d0 =   initial increment for input vector D
;;      sout    =   output vector E memory space ('x' or 'y')
;;      aout0   =   output vector E base address
;;      iout0   =   initial increment for output vector E 
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
;;      x:(R_X)+    input vector D base address     ainp_d0
;;      x:(R_X)+    input vector D increment        iinp_d0
;;      x:(R_X)+    output vector E base address    aout0
;;      x:(R_X)+    output vector E increment       iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vtvmvtv array-processor macro computes E[k]=A[k]*B[k]-C[k]*D[k].
;;
;;  PSEUDO-C NOTATION
;;      ainp_a  =   x:(R_X)+;
;;      iinp_a  =   x:(R_X)+;
;;      ainp_b  =   x:(R_X)+;
;;      iinp_b  =   x:(R_X)+;
;;      ainp_c  =   x:(R_X)+;
;;      iinp_c  =   x:(R_X)+;
;;      ainp_d  =   x:(R_X)+;
;;      iinp_d  =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = 
;;              sinp_a:ainp_a[n*iinp_a] * sinp_b:ainp_b[n*iinp_b] - 
;;              sinp_c:ainp_c[n*iinp_c] * sinp_d:ainp_d[n*iinp_d];
;;      
;;  MEMORY USE 
;;      10    x-memory arguments
;;      29    program memory locations 
;;            +3 program memory location if sinp_a==sinp_b
;;            +3 program memory location if sinp_c==sinp_d
;;      
;;  EXECUTION TIME 
;;      cnt*5+36 instruction cycles
;;            +1 instruction cycle if sinp_a==sinp_b
;;            +1 instruction cycle if sinp_c==sinp_d
;;
;;  DSPWRAP ARGUMENT INFO
;;      vtvmvtv (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,
;;          (dspace)sinp_c,(input)ainp_c,iinp_c,
;;          (dspace)sinp_d,(input)ainp_d,iinp_d,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvtvmvtv(aS1,iS1,aS2,iS2,aS3,iS3,aS4,iS4,aD,iD,n);  /* D[k]=S1[k]*S2[k]-S3[k]*S4[k]
;;          aS1 =   Source vector 1 base address
;;          iS1 =   S1 address increment
;;          aS2 =   Source vector 2 base address
;;          iS2 =   S2 address increment
;;          aS3 =   Source vector 3 base address
;;          iS3 =   S3 address increment
;;          aS4 =   Source vector 4 base address
;;          iS4 =   S4 address increment
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count                                       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvtvmvtv'
;;      symobj  ax_vec,bx_vec,cx_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 7A1ff,7A2ff,7A3ff,7A4ff,7A5ff,7A6ff,7A7ff,7A8ff,7A9ff,7A10f,7A11f,7A12f
;;bx_vec    dc 7B1ff,7B2ff,7B3ff,7B4ff,7B5ff,7B6ff,7B7ff,7B8ff,7B9ff,7B10f,7B11f,7B12f
;;cx_vec    dc 7C1ff,7C2ff,7C3ff,7C4ff,7C5ff,7C6ff,7C7ff,7C8ff,7C9ff,7C10f,7C11f,7C12f
;;dx_vec    dc 7D1ff,7D2ff,7D3ff,7D4ff,7D5ff,7D6ff,7D7ff,7D8ff,7D9ff,7D10f,7D11f,7D12f
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      vtvmvtv   ap,1,x,ax_vec,1,x,bx_vec,1,x,cx_vec,1,x,dx_vec,1,x,out1_vec,1,6
;;      end_ap  'tvtvmvtv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vtvmvtv.asm
;;      
;;  REGISTER USE  
;;      A       accumulator 
;;      B       temp for testing for cnt==0
;;      X0,Y0   current A,B terms
;;      X1,Y1   current C,D terms
;;      R_I1    running pointer to A vector terms
;;      N_I1    increment for A vector
;;      R_I2    running pointer to B vector terms
;;      N_I2    increment for B vector
;;      R_Y     running pointer to C vector terms
;;      N_Y     increment for C vector terms
;;      R_L     running pointer to D vector terms
;;      N_L     increment for D vector terms
;;      R_O     running pointer to output (E) vector terms
;;      N_O     increment for output (E) vector
;;      M_X     temporary storage for R_L
;;      N_Y     temporary storage for R_Y
;;      
vtvmvtv   macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sinp_c,ainp_c0,iinp_c0,sinp_d,ainp_d0,iinp_d0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vtvmvtv_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vtvmvtv_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vtvmvtv_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vtvmvtv_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vtvmvtv_\ic\_,ainp_c,ainp_c0
        new_xarg    pf\_vtvmvtv_\ic\_,iinp_c,iinp_c0
        new_xarg    pf\_vtvmvtv_\ic\_,ainp_d,ainp_d0
        new_xarg    pf\_vtvmvtv_\ic\_,iinp_d,iinp_d0
        new_xarg    pf\_vtvmvtv_\ic\_,aout,aout0
        new_xarg    pf\_vtvmvtv_\ic\_,iout,iout0
        new_xarg    pf\_vtvmvtv_\ic\_,cnt,cnt0

        ; get inputs
        move            R_Y,N_Y         ; squirrel away R_Y until endm
        move            R_L,A           ; temp storage of R_L
        move            x:(R_X)+,R_I1   ; input vector A base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector A address increment to N_I1
        move            x:(R_X)+,R_I2   ; input vector B base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector B address increment to N_I2
        move            x:(R_X)+,R_Y    ; input vector C base address to R_Y
        move            x:(R_X)+,N_Y    ; input vector C address increment to N_Y
        move            x:(R_X)+,R_L    ; input vector D base address to R_L
        move            x:(R_X)+,N_L    ; input vector D address increment to N_L
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,b      ; count
        move            A,M_X           ; squirrel away R_L until endm
        ;;  Obviously, with a non-FFFFFF value in M_X, you must
        ;;  restore M_X before using R_X in this macro. 
          
        ;  loop and pipeline initialization
        ;;  load X0 and Y0 with a[0] and b[0], and test for cnt=0
        if "sinp_a"=="sinp_b"
            tst     B       sinp_a:(R_I1)+N_I1,X0
            jeq     pf\_vtvmvtv_\ic\_loop1
            move            sinp_b:(R_I2)+N_I2,Y0
        else
            if "sinp_a"=='x'
                tst     B       sinp_a:(R_I1)+N_I1,X0   sinp_b:(R_I2)+N_I2,Y0
            else
                tst     B       sinp_b:(R_I2)+N_I2,X0   sinp_a:(R_I1)+N_I1,Y0    
            endif
            jeq     pf\_vtvmvtv_\ic\_loop1
        endif

        ;;  load X1 and Y1 with c[0] and d[0], and do mpy
        if "sinp_c"=="sinp_d"
            mpy     X0,Y0,A sinp_c:(R_Y)+N_Y,X1      
            move            sinp_d:(R_L)+N_L,Y1
        else 
            if "sinp_c"=='x'
                mpy     X0,Y0,A sinp_c:(R_Y)+N_Y,X1     sinp_d:(R_L)+N_L,Y1
            else
                mpy     X0,Y0,A sinp_d:(R_L)+N_L,X1     sinp_c:(R_Y)+N_Y,Y1      
            endif
        endif

        ;;  do macr to finish calculation of out[0], and
        ;;  load a[1] and b[1] into X0 and Y0
        if "sinp_a"=="sinp_b"
            macr    -X1,Y1,A sinp_a:(R_I1)+N_I1,X0    
            move            sinp_b:(R_I2)+N_I2,Y0
        else
            if "sinp_a"=='x'
                macr    -X1,Y1,A sinp_a:(R_I1)+N_I1,X0  sinp_b:(R_I2)+N_I2,Y0
            else
                macr    -X1,Y1,A sinp_b:(R_I2)+N_I2,X0  sinp_a:(R_I1)+N_I1,Y0    
            endif
        endif

        ;;  load c[1] and d[1] into X1 and Y1
        if "sinp_c"=="sinp_d"
            move            sinp_c:(R_Y)+N_Y,X1      
            move            sinp_d:(R_L)+N_L,Y1
        else 
            if "sinp_c"=='x'
                move            sinp_c:(R_Y)+N_Y,X1     sinp_d:(R_L)+N_L,Y1
        else
                move            sinp_d:(R_L)+N_L,X1     sinp_c:(R_Y)+N_Y,Y1      
            endif
        endif

        ; inner loop 
        do      B,pf\_vtvmvtv_\ic\_loop1
            mpy     X0,Y0,A A,sout:(R_O)+N_O  
            if "sinp_a"=="sinp_b"
                macr   -X1,Y1,A sinp_a:(R_I1)+N_I1,X0    
                move            sinp_b:(R_I2)+N_I2,Y0
            else
                if "sinp_a"=='x'
                    macr    -X1,Y1,A sinp_a:(R_I1)+N_I1,X0  sinp_b:(R_I2)+N_I2,Y0
                else
                    macr    -X1,Y1,A sinp_b:(R_I2)+N_I2,X0  sinp_a:(R_I1)+N_I1,Y0    
                endif
            endif
            if "sinp_c"=="sinp_d"
                move             sinp_c:(R_Y)+N_Y,X1      
                move             sinp_d:(R_L)+N_L,Y1
            else 
                if "sinp_c"=='x'
                    move            sinp_c:(R_Y)+N_Y,X1     sinp_d:(R_L)+N_L,Y1
                else
                    move            sinp_d:(R_L)+N_L,X1     sinp_c:(R_Y)+N_Y,Y1      
                endif
            endif
pf\_vtvmvtv_\ic\_loop1
        move            M_Y,M_X        ; restore M_X to FFFFFF
        move            N_X,R_Y
        move            M_X,R_L        ; restore pointer registers
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.
        endm

