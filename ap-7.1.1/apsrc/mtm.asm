;;  Copyright 1986 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      06/15/87/jms - initial file created from DSPAPSRC/template
;;      02/17/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      mtm (AP macro) - matrix times matrix 
;;          - multiply two two-dimensional matrices, creating an output matrix 
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      mtm    pf,ic,sinp_a,ainp_a0,sinp_b,ainp_b0,sout,aout0,b_num_rows0,b_num_cols0,a_num_rows0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_mtm_\ic is globally unique)
;;      sinp_a  =   input matrix A memory space ('x' or 'y')
;;      ainp_a0 =   input matrix A base address (address of A[1,1]
;;      sinp_b  =   input matrix B memory space ('x' or 'y')
;;      ainp_b0 =   input matrix B base address
;;      sout    =   output maxtrix C memory space ('x' or 'y')
;;      aout0   =   output matrix C base address
;;                  (C is dimensioned a_num_rows by b_num_cols) 
;;      b_num_rows0 = 1st dimension of input matrix B 
;;                  (= 2nd dimension of input matrix A)     
;;      b_num_cols0 = 2nd dimension of input matrix B
;;      a_num_rows0 = 1st dimension of input matrix A
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                         Initialization
;;      ------      -----------                         --------------
;;      x:(R_X)+    input matrix A base address         ainp_a0
;;      x:(R_X)+    input matrix B base address         ainp_b0
;;      x:(R_X)+    output matrix C base address        aout0
;;      x:(R_X)+    1st dimension of input matrix B     b_num_rows0
;;      x:(R_X)+    2nd dimension of input matrix B     b_num_cols0
;;      x:(R_X)+    1st dimension of input matrix A     a_num_rows0
;;      
;;  DESCRIPTION
;;      The mtm array-processor macro computes C=AB, where A, B, and C are.
;;      matrices.
;;
;;  PSEUDO-C NOTATION
;;      N_Y     =   R_Y;
;;      N_X     =   R_L;
;;      R_L     =   x:(R_X)+;
;;      R_Y     =   x:(R_X);
;;      X0      =   x:(R_X)+;
;;      R_O     =   x:(R_X)+;
;;      N_L     =   x:(R_X);
;;      A       =   x:(R_X)+;
;;      A       -=  1;
;;      N_I2    =   x:(R_X)+;
;;      
;;      for (a_row=0; a_row<x:(R_X)+; a_row++) {
;;          for (b_col=0; b_col<N_I2; b_col++) {
;;              R_I1 = R_L;
;;              R_I2 = R_Y; 
;;              b = 0.0;
;;      
;;              X1 = *R_I1++;
;;              Y1 = *R_I2;
;;              R_I2 += N_I2;
;;              for (i=0; i<A; i++) {
;;                  b += X1 * Y1; 
;;                  X1 = *R_I1++; 
;;                  Y1 = *R_I2;
;;                  R_I2 += N_I2;
;;                  }
;;              b += X1 * Y1;
;;              R_Y++;
;;              *R_O++ = b; 
;;              } 
;;          R_L += N_L;
;;          R_Y = X0;
;;          } 
;;      *R_Y = N_Y;         /* restore R_Y, R_L */
;;      *R_L = N_X;
;;      
;;  MEMORY USE 
;;      6 x-memory arguments
;;      sinp_a==sinp_b: 45 program memory locations
;;      sinp_a!=sinp_b: 42 program memory locations 
;;      
;;  EXECUTION TIME 
;;      sinp1==sinp2:   ra*{6+cb*[10+2*(rb-1)]}+24 instruction cycles
;;      sinp1!=sinp2:   ra*{5+cb*[ 8+2*(rb-1)]}+24 instruction cycles
;;      where   ra =    a_num_rows
;;              cb =    b_num_cols
;;              rb =    b_num_rows
;;      
;;  DSPWRAP ARGUMENT INFO
;;      mtm (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,
;;          (dspace)sinp_b,(input)ainp_b,
;;          (dspace)sout,(output)aout,
;;          b_num_rows,b_num_cols,a_num_rows
;;      
;;  DSPWRAP C SYNTAX
;;      ierr        = DSPAPmtm(A, B, C, b_rows, b_cols, a_rows); /*
;;          A       = matrix A base address in DSP memory
;;          B       = matrix B base address in DSP memory
;;          C       = matrix C base address in DSP memory
;;          b_rows  = 1st dimension of matrix B (and 2nd dimension of matrix A)
;;          b_cols  = 2nd dimension of matrix B     
;;          a_rows  = 1st dimension of matrix A                 */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tmtm'             ; begin ap main program
;;      new_xeb A_mat,6,0.5         ; allocate input A matrix
;;      new_yeb B_mat,15,0.5        ; allocate input B matrix
;;      new_yeb C_mat,10,0          ; allocate output matrix
;;      mtm ap,1,x,A_mat,y,B_mat,y,C_mat,3,5,2
;;      end_ap  'tmtm'             ; end of ap main program
;;
;;  SOURCE
;;      /usr/lib/dsp/apsrc/mtm.asm
;;      
;;  REGISTER USE
;;      A       2nd dimension of B matrix, decremented by 1
;;      B       accumulator    
;;      X0      point to B matrix base address
;;      X1      current term from A matrix
;;      Y1      current term from B matrix
;;      R_I1    running pointer to A matrix terms
;;      R_I2    running pointer to B matrix terms
;;      N_I2    2nd dimension of B matrix
;;      R_L     point to A matrix
;;      N_L     2nd dimension of A matrix
;;      R_O     point to output C matrix
;;      N_X     store R_L for duration of macro 
;;      R_Y     point to B matrix
;;      N_Y     store R_Y for duration of macro
;;      
mtm    macro pf,ic,sinp_a,ainp_a0,sinp_b,ainp_b0,sout,aout0,b_num_rows0,b_num_cols0,a_num_rows0

        ; argument definitions
        new_xarg    pf\_mtm_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_mtm_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_mtm_\ic\_,aout,aout0
        new_xarg    pf\_mtm_\ic\_,b_num_rows,b_num_rows0
        new_xarg    pf\_mtm_\ic\_,b_num_cols,b_num_cols0
        new_xarg    pf\_mtm_\ic\_,a_num_rows,a_num_rows0

        ; get inputs
        move            R_Y,N_Y         ; squirrel away R_Y until endm
        move            R_L,N_X         ; squirrel away R_L until endm
        move            x:(R_X)+,R_L    ; #ainp_a0, point to A matrix
        move            x:(R_X),R_Y     ; #ainp_b0, point to B matrix
        move            x:(R_X)+,X0     ; #ainp_b0, point to B matrix forever
        move            x:(R_X)+,R_O    ; #aout0,output to C matrix
        move            x:(R_X),N_L     ; #b_num_rows0, second dimension of A
        move            x:(R_X)+,A      ; #b_num_rows0
        move            x:(R_X)+,N_I2   ; #b_num_cols0, second dimension of B

        ; set up loop and pipelining
        move            #>1,B
        sub     B,A
        jle     pf\_mtm_\ic\_dim1

        ; inner loop 
        ; if b_num_rows0>1
        do      x:(R_X)+,pf\_mtm_\ic\_a_row_loop1          ; #a_num_rows0
            do      N_I2,pf\_mtm_\ic\_b_col_loop1          ; #b_num_cols0
                move            R_L,R_I1     
                move            R_Y,R_I2     
                clr     B
                if "sinp_a"=="sinp_b"
                    move            sinp_a:(R_I1)+,X1
                    move            sinp_b:(R_I2)+N_I2,Y1
                    do      A,pf\_mtm_\ic\_b_row_loop1     ; #b_num_rows0-1      
                        mac     X1,Y1,B sinp_a:(R_I1)+,X1
                        move            sinp_b:(R_I2)+N_I2,Y1
pf\_mtm_\ic\_b_row_loop1
                else
                    if "sinp_a"=='x'
                        move            sinp_a:(R_I1)+,X1       sinp_b:(R_I2)+N_I2,Y1
                    else      
                        move            sinp_b:(R_I2)+N_I2,X1   sinp_a:(R_I1)+,Y1
                    endif                    
                    rep     A                               ; #b_num_rows0-1       
                        if "sinp_a"=='x'         
                            mac     X1,Y1,B sinp_a:(R_I1)+,X1       sinp_b:(R_I2)+N_I2,Y1
                        else
                            mac     X1,Y1,B sinp_b:(R_I2)+N_I2,X1   sinp_a:(R_I1)+,Y1
                        endif
                endif
                macr    X1,Y1,B (R_Y)+     
                move            B,sout:(R_O)+     
pf\_mtm_\ic\_b_col_loop1
            move            (R_L)+N_L    
            move            X0,R_Y       ; #ainp_b0
pf\_mtm_\ic\_a_row_loop1
        jmp     pf\_mtm_\ic\_done   
        ; else since bnumrows0<=1
pf\_mtm_\ic\_dim1
        do      x:(R_X)+,pf\_mtm_\ic\_a_row_loop2          ; #a_num_rows0
            move            sinp_a:(R_L)+,X1
            do      N_I2,pf\_mtm_\ic\_b_col_loop2          ; #b_num_cols0    
                move            sinp_b:(R_Y)+,Y1
                mpyr    X1,Y1,B   
                move            B,sout:(R_O)+     
pf\_mtm_\ic\_b_col_loop2
            move            X0,R_Y                          ; #ainp_b0
pf\_mtm_\ic\_a_row_loop2
        ; endif
pf\_mtm_\ic\_done   
        move            N_X,R_L        
        move            N_Y,R_Y
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.
        endm

