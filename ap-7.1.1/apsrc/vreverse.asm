;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn 
;;      
;;  Modification history
;;  --------------------
;;      07/09/87/jms - initial file created from DSPAPSRC/template
;;      07/22/87/jms - fixed cnt/2 bugs 
;;      02/15/88/jms - removed "andi #$fe,ccr"; replace ror with lsr, both in initialization 
;;                   - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      05/25/89/mtm - fix bugs in DSPWRAP C SYNTAX
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vreverse (AP macro) - vector reverse elements - reverse the elements of the vector
;;      
;;  SYNOPSIS 
;;      include 'ap_macros'     ; load standard DSP macro package
;;      vreverse   pf,ic,sinp,ainp0,sout,aout0,cnt0    ; invoke ap macro vreverse
;;      
;;  MACRO ARGUMENTS 
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vreverse_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   initial input vector memory address
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial address for output scalar
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS 
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    Input Vector base address       ainp0
;;      x:(R_X)+    Output                          aout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION 
;;      The vreverse array-processor macro writes the input vector to the
;;      output vector in reverse order.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      odd = cnt & 1;
;;      cntOver2 = cnt >> 1;
;;      if (odd) cntOver2++;
;;      ainp_end = ainp + cnt - 1;
;;      aout_end = aout + cnt - 1;
;;      i = 0;
;;      a = *ainp++;
;;      while (i++ < cntOver2) {
;;          b = sinp:*ainp_end--;   sout:*aout_end-- = a;
;;          a = sinp:*ainp++;       sout:*aout++ = b; 
;;      }
;;      
;;  MEMORY USE 
;;      3 x-memory arguments
;;      sinp==sout: 27 program memory locations
;;      sinp!=sout: 25 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt even: cnt*2+25 instruction cycles
;;      cnt odd:  cnt*2+26 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO 
;;      vreverse (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,
;;          (dspace)sout,(output)aout,cnt
;;          
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvreverse(aS,aD,n);  /* Vector reverse:
;;          aS  =   Source vector base address
;;          aD  =   Destination vector base address
;;          n   =   element count       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvreverse'
;;      symobj  ix_vec
;;      beg_xeb
;;          radix   16
;;ix_vec    dc 101,102,103,104,105,106,107
;;          radix   A
;;      end_xeb
;;      new_xeb t1x_vec,7,$777
;;      vreverse   ap,1,x,ix_vec,x,t1x_vec,7
;;      end_ap  'tvreverse'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vreverse.asm
;;      
;;  REGISTER USE 
;;      A       input term from first half of input vector
;;      B       input term from second half of input vector 
;;      X0      cnt
;;      X1      cnt/2
;;      Y0      temp (=1)
;;      R_I1    ainp
;;      R_I2    ainp_end
;;      R_L     aout_end
;;      N_L     temp storage for R_L
;;      R_O     aout
;;      
vreverse   macro pf,ic,sinp,ainp0,sout,aout0,cnt0

        ; argument declarations
        new_xarg    pf\_vreverse_\ic\_,ainp,ainp0   ; input address arg
        new_xarg    pf\_vreverse_\ic\_,aout,aout0   ; output address arg
        new_xarg    pf\_vreverse_\ic\_,cnt,cnt0     ; element count arg

        ; get inputs
        move            x:(R_X)+,R_I1   ; input address goes to R_I1
        move            x:(R_X)+,R_O    ; output address goes to R_O
        move            x:(R_X)+,A      ; cnt into A
        clr     B       R_L,N_L         ; save R_L for duration of macro

        ; set up loop and pipelining
        move    A1,X0                   ; copy cnt into X0
        lsr     A       #1,B1           ; A gets cnt/2 
        jcc     pf\_vreverse_\ic\_l1
        add     B,A                     ; if odd, increment cnt/2     
pf\_vreverse_\ic\_l1
        move            A1,X1           ; copy cnt0/2 into X1
        tst     A       B1,Y0                
        jeq     pf\_vreverse_\ic\_l2       ; guard against cnt=0
        move            R_I1,A          ; A = input start address
        add     X0,A    R_O,B           ; B = output start address    
        move            A1,R_L
        add     X0,B    sinp:(R_I1)+,A  ; initialize a for loop
        sub     Y0,B    (R_L)-          ; R_L = input end address
        move            B1,R_I2         ; R_I2 = output end address

        ; inner loop 
        do      X1,pf\_vreverse_\ic\_l2     
            if "sinp"=="sout"
                move    sinp:(R_L)-,B  
                move    A,sout:(R_I2)-
                move    sinp:(R_I1)+,A 
                move    B,sout:(R_O)+ 
            else
                if "sinp"=='x'
                    move    sinp:(R_L)-,B  A,sout:(R_I2)-
                    move    sinp:(R_I1)+,A b,sout:(R_O)+ 
                else
                    move    A,sout:(R_I2)- sinp:(R_L)-,B  
                    move    B,sout:(R_O)+  sinp:(R_I1)+,A 
                endif
            endif
pf\_vreverse_\ic\_l2     
        move    N_L,R_L
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.
        endm

