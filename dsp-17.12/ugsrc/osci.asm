;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith, James A. Moorer
;;
;;  Modification history
;;  --------------------
;;  01/13/88/jam - copied from oscg
;;  03/16/88/jos - installed missing post-increment on R_X at macro exit
;;  03/18/88/jos - fixed problem with sign bit of low-order word of phase
;;  03/18/88/jos - added space argument for delta table and compacted code
;;  09/29/88/jos - fixed phase clipping bug by clearing b2 on exit
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      osci (UG macro) - Interpolating oscillator
;;
;;  SYNOPSIS
;;      osci pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,stab,atab0,sdel,adel0,mtab0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (such that pf\_osci\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = initial output vector address
;;      amp0      = initial amplitude in [-1.0,1.0)
;;      inc0h     = initial increment in table-samples/sample, high-order word
;;      inc0l     = initial increment in table-samples/sample, low-order word
;;      phs0h     = initial phase in table-samples, high-order word
;;      phs0l     = initial phase in table-samples, low-order word
;;      stab      = base table address memory space ('x' or 'y')
;;      atab0     = address of first word of wave table
;;      sdel      = delta wave table address memory space ('x' or 'y')
;;      adel0     = address of first word of delta wave table
;;      mtab0     = table length - 1 = power of 2 minus 1 (both tables)
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     --------------           --------------
;;      x:(R_X)+       current base address     atab0
;;      x:(R_X)+       current delta address    adel0
;;      x:(R_X)+       table address mask       mtab0
;;      y:(R_Y)+       current output address   aout0
;;      y:(R_Y)+       current amplitude        amp0
;;      l:(R_L)P_L     current phase            phs0h,phs0l
;;      l:(R_L)P_L     index increment          inc0h,inc0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of a sinusoid. In pseudo-C:
;;
;;      atab = x:(R_X)+;           /* base wave table address */
;;      adel = x:(R_X)+;           /* delta wave table address */
;;      mtab = x:(R_X)+;           /* wave table length minus 1 */
;;      aout = y:(R_Y)+;           /* output address */
;;      amp  = y:(R_Y)+;           /* current amplitude */
;;      phs  = l:(R_L)P_L;         /* current table read pointer */
;;      inc  = l:(R_L)P_L;         /* table increment */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           sout:aout[n] = amp * (stab:atab[phs1] + phs0 * sdel:adel[phs1+1]);
;;           phs += inc;
;;           phs1 &= mtab;
;;      }
;;
;;      phs1 denotes the high-order word of phs, interpreted as an integer.
;;      phs0 denotes the low-order word of phs, interpreted as an unsigned
;;              fraction (0 to 1-epsilon).
;;
;;      The index increment inc can be calculated as the table length 
;;      ntab=mtab+1, times the desired frequency frq in Hz, divided 
;;      by the sampling rate Fs i.e.,
;;
;;                   ntab * frq      TableLength * Frequency 
;;            inc =  ----------   =  ----------------------- 
;;                        Fs                SamplingRate     
;;
;;
;;  USAGE RESTRICTIONS
;;      The wavetable length mtab+1 must be a power of 2
;;      The base table and delta table must be the same length
;;      The base table and the delta table must be in opposing memory banks 
;;
;;  DSPWRAP ARGUMENT INFO
;;      osci (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         amp,inch,incl,phsh,phsl,
;;         (dspace)stab,(address)atab,(dspace)sdel,(address)adel,mtab
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/osci.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscg.asm - osc with general power-of-two table
;;      /usr/lib/dsp/ugsrc/oscs.asm = oscg with no mask in inner loop (fastest)
;;      /usr/lib/dsp/ugsrc/oscgf.asm = oscgaf without an amplitude envelope.
;;
;;  ALU REGISTER USE
;;      Y1 = Amplitude scale factor
;;      Y0 = Table lookup result
;;       X = double-precision phase increment (frequency)
;;           X0 = wavetable address mask also
;;       B = double-precision instantaneous phase
;;           B1 = integer part of table index
;;           B0 = interpolation constant
;;       A = Amp*Table(B) = output signal, plus other intermediates
;;    R_I1 = Pointer to wave table
;;    R_I2 = Pointer to delta table
;;
osci      macro pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,stab,atab0,sdel,adel0,mtab0
          new_xarg pf\_osci_\ic\_,atab,atab0
          new_xarg pf\_osci_\ic\_,adel,adel0
          new_xarg pf\_osci_\ic\_,mtab,mtab0
          new_yarg pf\_osci_\ic\_,aout,aout0
          new_yarg pf\_osci_\ic\_,amp,amp0
          new_larg pf\_osci_\ic\_,phs,phs0h,phs0l
          new_larg pf\_osci_\ic\_,inc,inc0h,inc0l

; Set up data alu regs from state held in memory arguments

            move x:(R_X)+,R_I1       ; Wavetable base address
            move x:(R_X)+,R_I2       ; Delta table base address
            move y:(R_Y)+,R_O        ; Current output pointer
            move l:(R_L)P_L,b        ; b = current phase

            move l:(R_L),x              ; Pick up phase increment again
            add x,b                     ; Add in phase increment
            move x:(R_X),x0             ; Pick up address mask again
            and x0,b                    ; Wrap phase necessary
            move b1,N_I1                ; Integer part of phase to N_I1
            clr A b1,N_I2               ; Copy to N_I2 for other table
            move b0,A1                  ; Fractional part of phase to A
            if @SCP("pf",'noint')
              clr a                     ; disable linear interpolation (test)
            endif
            move stab:(R_I1+N_I1),y1    ; y1 = table lookup of f(n)
            lsr a  sdel:(R_I2+N_I2),y0  ; y0 = lookup of f(n+1)-f(n)
            move a1,x1
            mpy x1,y0,a         l:(R_L),x       ; A=fraction*delta, x=increment
            add y1,a            y:(R_Y),y1      ; A+=Table, y1=amp

            do #I_NTICK-1,pf\_osci_\ic\_wave1

                add x,b  a,y0           ; add phase increment, sample to y0 
                mpy y1,y0,a x:(R_X),x0  ; apply amplitude, load address mask
                and x0,b  a,sout:(R_O)+ ; apply address mask, ship output   
                move b1,N_I1            ; current phase, integer part       
                clr A b1,N_I2           ; watch out for sign bit of b0 below
                move b0,a1              ; current phase, fractional part
                if @SCP("pf",'noint')
                  clr a                 ; disable linear interpolation (test)
                endif
                move stab:(R_I1+N_I1),y1        ; y1 = table lookup of f(n)
                lsr a  sdel:(R_I2+N_I2),y0      ; y0 = lookup of f(n+1)-f(n)
                move a1,x1                 ; interpolation constant to x1
                mpy x1,y0,a     l:(R_L),x  ; times delta value, restore incr
                add y1,a        y:(R_Y),y1 ; plus table value, get amp   

pf\_osci_\ic\_wave1

            move a,y0
            move y:(R_Y),y1              ; Pick up gain term
            mpy y1,y0,a (R_L)B_L         ; finish off last output sample
            move a,sout:(R_O)+           ; last output
            move #<0,b2                  ; if negative, following will clip:
            move b,l:(R_L)               ; save phase for next time
            move x:(R_X)+,X0 y:(R_Y)+,Y0 ; post-increment R_X and R_Y
            move (R_L)P_L                ; post-increment R_L twice
            move (R_L)P_L

          endm

        if 0

                move l:(R_L),x          ; Pick up phase increment again
                add x,b                 ; Phase increment
                move x:(R_X),x0         ; Pick up address mask again
                and x0,b                ; Modulo table length
                move b1,N_I2            ; Get it into a register
                nop
                clr A l:(R_I2+N_I2),y   ; Pick up base and displacement
                move b0,a1              ; Low-order bits
                lsr a                   ; make signed fraction (sign=0)
                move a1,x1              ; Put it where we can multiply it
                if "stab' == 'x'
                    mpy x1,y0,a
                    add y1,a
                else
                    mpy x1,y1,a
                    add y0,a
                endif
                move a,y0
                move y:(R_Y),y1         ; Pick up gain term
                mpy y1,y0,a
                move a,sout:(R_O)+

        endif



