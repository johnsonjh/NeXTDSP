;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      04/03/88/jms - initial file created from DSPAPSRC/template
;;	05/20/89/mtm - change HMF to HMS
;;	06/06/89/mtm - removed DSPWRAP SWITCHES comment section
;;	08/22/89/mtm - changed N_DMA to N_IO
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      fftr2a (AP macro) - radix 2 FFT
;;          - radix 2 decimation-in-time FFT, complex input and output, in-place, output shuffled
;;
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      fftr2a  pf,ic,points0,data0,coef0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_fftr2a_\ic is globally unique)
;;      points0 =   number of (complex) points
;;      data0   =   base address for complex data. WARNING: real (in x) and
;;                  imaginary data (in y memory) both start at same address.
;;      coef0   =   base address for (complex) sine/cosine lookup table.
;;                  WARNING for data0 applies here.  Macro sincos generates
;;                  lookup table. Coefficient table is length points0/2.
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    number of complex data points   points0
;;      x:(R_X)+    base address for complex data   data0
;;      x:(R_X)+    base address for coefficients   coef0
;;      
;;  DESCRIPTION
;;      The fftr2a array-processor macro computes A=FFT(A).
;;
;;  PSEUDO-C NOTATION
;;      points  =   x:(R_X)+;
;;      *data   =   x:(R_X)+;
;;      *coef   =   x:(R_X)+;
;;
;;      fftr2a(points,data,coef);
;;      
;;  MEMORY USE 
;;      3   x-memory arguments
;;      63  program memory locations 
;;      
;;  EXECUTION TIME
;;      64 + 6N + 29log(N) + 5N*log(N) instruction cycles (approximate)
;;      where   N = points
;;              log(N) is base 2 log
;;
;;  DSPWRAP ARGUMENT INFO
;;      fftr2a (prefix)pf,(instance)ic,points,data,coef
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPfftr2a(n, data, coef);     /* radix 2 FFT
;;          n   =   number of complex data points
;;       data   =   base address for complex data
;;       coef   =   base address for sine/cosine lookup table, length n/2 */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap 'tfftr2a'
;;      new_xeb coefx_16,8,$777 ; We'll use both x and y memory at this address.
;;      sincos  16,coefx_16     ; load tables with sine and cosine values 
;;      symobj ax_vec,ay_vec
;;      beg_xeb ; .9999... in the imaginary side
;;ax_vec      dc 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;;      end_xeb
;;      beg_yeb
;;ay_vec      dc 0,$7fffff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;;      end_yeb
;;      vmove    ap,1,y,ay_vec,1,y,ax_vec,1,16 ; copy ay_vec into x space parallel to ax_vec
;;      fftr2a ap,1,16,ax_vec,coefx_16
;;      end_ap 'tfftr2a'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/fftr2a.asm
;;      
;;  REGISTER USE 
;;      A       accumulator
;;      B       accumulator
;;      N_IO   temporary storage for R_Y
;;      N_HMS   temporary storage for R_L
;;      R_I1    running pointer A for input data 
;;      N_I1    number of butterflies per group (n/2,n/4,n/8...)
;;      R_I2    running pointer for coefficients
;;      N_I2    increment for bit-reversed addressing (=n/4)
;;      M_I2    0, to activate bit-reversed addressing
;;      R_L     running pointer B for input data 
;;      N_L     offset for input data (=n/2)
;;      R_O     running pointer B for output terms 
;;      N_O     offset for output data (=n/2)
;;      N_X     number of groups per pass (1,2,4,8...)
;;      R_Y     running pointer A for output terms 
;;      N_Y     offset for output data (=n/2)
;;      X0      real coefficient      
;;      X1      real input datum
;;      Y0      imaginary coefficient
;;      Y1      imaginary input datum
;;      
;; The core of this code is taken from the routine fftr2a.asm
;; from Motorola's Dr. Bub, written by Kevin Kloker and Garth Hillman

fftr2a  macro pf,ic,points0,data0,coef0

        ; argument definitions
        new_xarg    pf\_fftr2a_\ic\_,points,points0
        new_xarg    pf\_fftr2a_\ic\_,data,data0
        new_xarg    pf\_fftr2a_\ic\_,coef,coef0

        ; get first input term --- others are fetched in do loop, below 
        move            x:(R_X)+,B  ; no. of points
        tst     B       B,A
        jeq     pf\_fftr2a_\ic\_end ; protect against cnt = 0

        ; further initialization
        lsr     B       #1,N_X      ; initialize groups per pass
        lsr     B       B,N_I1      ; initialize butterflies per group
        clr     B       B,N_I2      ; initialize coefficient pointer offset
        move            B,M_I2      ; initialize coefficient address modifier 
                                    ; for bit-reversed addressing
        ; evaluate #@cvi(@log(#points)/@log(2)+0.5) from Motorola code
        do #24,pf\_fftr2a_\ic\_l2
            lsl     A               ; if reached highest-order bit,
            jcc     pf\_fftr2a_\ic\_l1      
            movec           LC,B        ; then store loop count and bail out
            enddo
            jmp     pf\_fftr2a_\ic\_l2
pf\_fftr2a_\ic\_l1  
            nop                         ; else decrement loop count
pf\_fftr2a_\ic\_l2  
        move            #>1,X0      ; Found power of two, almost.  Fix loop count,
        sub     X0,B    R_Y,N_IO   ; (while saving these two registers)
        lsl     A       R_L,N_HMS   
        jcc     pf\_fftr2a_\ic\_l3
        add     X0,B                ; and round up if needed. 
pf\_fftr2a_\ic\_l3 
        tst     B                   ; Protect against points=1
        jeq     pf\_fftr2a_\ic\_end_pass 

        ; inner loop
        do      B1,pf\_fftr2a_\ic\_end_pass
            move            x:(R_X)+,R_I1   ;initialize A input pointer
            move            R_I1,R_Y        ;initialize A output pointer
            lua     (R_I1)+N_I1,R_L         ;initialize B input pointer
            move            x:(R_X)-,R_I2   ;initialize C input pointer
            lua     (R_L)-,R_O              ;initialize B output pointer
            move            N_I1,N_L        ;initialize pointer offsets
            move            N_I1,N_Y
            move            N_I1,N_O
 
            ; one group of butterflies
            do      N_X,pf\_fftr2a_\ic\_end_grp
                move            x:(R_L),X1  y:(R_I2),Y0 ; first real datum, imag coef
                move            x:(R_O),A   y:(R_I1),B  ; preload first ouput, imag datum      
                move            x:(R_I2)+N_I2,X0        ; real coef
     
                ; Radix 2 DIT butterfly 
                do      N_I1,pf\_fftr2a_\ic\_end_bfy
                    mac     X1,Y0,B y:(R_L)+,Y1      
                    macr   -X0,Y1,B A,x:(R_O)+  y:(R_I1),A
                    subl    B,A     x:(R_I1),B  B,y:(R_Y)
                    mac    -X1,X0,B x:(R_I1)+,A A,y:(R_O)
                    macr   -Y1,Y0,B x:(R_L),X1
                    subl    B,A     B,x:(R_Y)+  y:(R_I1),B
pf\_fftr2a_\ic\_end_bfy
                move    A,x:(R_O)+N_O       y:(R_L)+N_L,Y1 ; output final term, update R_L
                move    x:(R_I1)+N_I1,X1    y:(R_Y)+N_Y,Y1 ; update pointers
pf\_fftr2a_\ic\_end_grp
            move    N_I1,B1
            lsr     B       N_X,A1      ; divide butterflies per group by two
            lsl     A       B1,N_I1     ; multiply groups per pass by two
            move    A1,N_X
pf\_fftr2a_\ic\_end_pass

        ; restore registers
	
        move            N_HMS,R_L
        move            N_IO,R_Y
        move            #-1,M_I2

pf\_fftr2a_\ic\_end
        ; update R_X for next macro 
        move            (R_X)+
        move            (R_X)+

        endm

