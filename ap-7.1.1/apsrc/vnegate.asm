;;  Copyright 1986 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      06/15/87/jms - initial file created from DSPAPSRC/template
;;      09/20/87/jms - added test for cnt=0; double up accumulators to
;;                     increase execution speed; cosmetic changes to code;
;;                     added more rigorous tests to tvnegate.asm 
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vnegate (AP macro) - vector negate - fill vector with neg of vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vnegate pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0 ; invoke ap macro vnegate
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vnegate_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   initial input vector memory address
;;      iinp0   =   initial increment for input vector address
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial output vector memory address
;;      iout0   =   initial increment for output vector address
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description             Initialization
;;      ------      -----------             --------------
;;      x:(R_X)+    Source address          ainp0
;;      x:(R_X)+    Source increment        iinp0
;;      x:(R_X)+    Destination address     aout0
;;      x:(R_X)+    Destination increment   iout0
;;      x:(R_X)+    element count           cnt0
;;      
;;  DESCRIPTION
;;      The vnegate array-processor macro computes the twos-complement negative 
;;      of every element of the input array, and copies those
;;      negatives to an output array.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout] = - sinp:ainp[n*iinp];
;;      }
;;      
;;  MEMORY USE 
;;      5 x-memory arguments
;;      sinp==sout: 26 program memory locations
;;      sinp!=sout: 24 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt even: cnt*2+24 instruction cycles
;;      cnt odd:  cnt*2+26 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vnegate (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvnegate(S,iS,D,iD,n);   /* vector negate:
;;          S   =   Source vector base address in DSP memory
;;          iS  =   S address increment      
;;          D   =   Destination vector base address 
;;          iD  =   D address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvnegate'             ; begin ap main program
;;      new_xeb out1vec,12,0.5
;;      new_yeb Y1vec,12,0
;;      vnegate    ap,1,x,out1vec,1,y,Y1vec,1,12
;;      end_ap  'tvnegate'             ; end of ap main program
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vnegate.asm
;;      
;;  REGISTER USE
;;      A       working register
;;      B       working register
;;      X1 or Y1 flag for cnt odd/even
;;      X0      cnt/2
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
vnegate    macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument declarations
        new_xarg    pf\_vnegate_\ic\_,ainp,ainp0   ; input address arg
        new_xarg    pf\_vnegate_\ic\_,iinp,iinp0   ; in address incr arg
        new_xarg    pf\_vnegate_\ic\_,aout,aout0   ; out address arg
        new_xarg    pf\_vnegate_\ic\_,iout,iout0   ; out address incr arg
        new_xarg    pf\_vnegate_\ic\_,cnt,cnt0     ; element count arg

        ; get inputs
        move                 x:(R_X)+,R_I1   ; input address to R_I1
        move                 x:(R_X)+,N_I1   ; input address incr to N_I1
        move                 x:(R_X)+,R_O    ; output address to R_O
        move                 x:(R_X)+,N_O    ; output address incr to N_O
        clr     A            x:(R_X)+,B      ; count to B

        ; set up loop and pipelining
        ; test for cnt=0, cnt=1
        lsr     B                           ; B gets cnt/2 
        jcc     pf\_vnegate_\ic\_l1
        move            #1,A1               ; A gets "cnt odd" flag
pf\_vnegate_\ic\_l1     
        ; X1 or Y1 gets "cnt even/odd" flag
        if "sinp"=='x'
            move            sinp:(R_I1)+N_I1,A  A,Y1 
        else          
            move    A,X1    sinp:(R_I1)+N_I1,A  
        endif
        neg     A       B1,X0               ; copy cnt0/2 into X0 for loop    
        tst     B                           ; guard against cnt=0 or cnt=1 and
        jeq     pf\_vnegate_\ic\_l3   
        move    sinp:(R_I1)+N_I1,B          ; continue pipeline initialization

        ; inner loop
        do      X0,pf\_vnegate_\ic\_l3
            if "sinp"=="sout"
                move            A,sout:(R_O)+N_O
                neg     B       sinp:(R_I1)+N_I1,A
                move            B,sout:(R_O)+N_O
                neg     A       sinp:(R_I1)+N_I1,B
            else
                if "sinp"=='x'
                    neg     B       sinp:(R_I1)+N_I1,A  A,sout:(R_O)+N_O
                    neg     A       sinp:(R_I1)+N_I1,B  B,sout:(R_O)+N_O
                else
                    neg     B       A,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,A  
                    neg     A       B,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,B  
                endif
            endif
pf\_vnegate_\ic\_l3            ; jump to here for cnt=0 or cnt=1
        if "sinp"=='x'
            move            Y1,B
        else
            move            X1,B
        endif
        tst     B    
        jeq     pf\_vnegate_\ic\_l2         ; if cnt odd (including cnt=1),
        move            A,sout:(R_O)+N_O    ; store final element
pf\_vnegate_\ic\_l2         
        endm

