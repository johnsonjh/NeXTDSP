;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  08/02/88/jos - initial file created from oscgf.asm
;;  09/25/88/jos - Moved R_L access after 1st line (fails at top of orcl)
;;  09/29/88/jos - fixed phase clipping bug by clearing b2 on exit
;;  04/25/89/mmm - Changed so that optimized case is sina==x and sinf==y,
;;              rather than sina==x and sout==y.
;;              It is better to have flexibility in the output space.
;;              Also got rid of two instructions outside the loop.
;;  06/19/89/mmm - Added more 4-instruction cases.
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      oscgaf (UG macro) - Oscillator with amplitude and frequency envelopes
;;
;;  SYNOPSIS
;;      oscgaf pf,ic,sout,aout0,sina,aina0,sinf,ainf0,inc0,phs0h,phs0l,stab,atab0,mtab0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_oscgaf_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = output vector address
;;      sina      = amplitude envelope memory space ('x' or 'y')
;;      aina0     = amplitude envelope input signal address
;;      sinf      = frequency envelope signal memory space ('x' or 'y')
;;      ainf0     = frequency envelope input signal address
;;      inc0      = increment in table-samples/sample, high-order word (no low!)
;;      phs0h     = phase in table-samples, high-order word
;;      phs0l     = phase in table-samples, low-order word
;;      stab      = table memory space ('x' or 'y')
;;      atab0     = table address
;;      mtab0     = table length - 1 = power of 2 minus 1
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     --------------           --------------
;;      x:(R_X)+       table address            atab0
;;      x:(R_X)+       table increment          inc0
;;      x:(R_X)+       amplitude env address    aina0
;;      y:(R_Y)+       output address           aout0
;;      y:(R_Y)+       frequency env address    ainf0
;;      y:(R_Y)+       table address mask       mtab0
;;      l:(R_L)P_L     current phase            phs0h,phs0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of a sinusoid, reading an external signal
;;      inputs for scaling both amplitude and frequency.  Execution is 20% 
;;      faster if 1) the increment envelope patchpoint space "sinf" is not
;;      the same as the amplitude envelope patchpoint space "sina", OR 2)
;;      amplitude space is 'x' and the output space is 'y'.
;;
;;      In pseudo-C:
;;
;;      atab = x:(R_X)+;           /* wave table address */
;;      inc  = x:(R_X)+;           /* table increment */
;;      aina = x:(R_X)+;           /* amplitude envelope patchpoint address */
;;      aout = y:(R_Y)+;           /* output patchpoint address */
;;      ainf = y:(R_Y)+;           /* frequency envelope patchpoint address */
;;      mtab = y:(R_Y)+;           /* wave table length minus 1 */
;;      phs  = l:(R_L)P_L;         /* current table read pointer */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           sout:aout[n] = sina:aina[n] * stab:atab[phs1];
;;           inf = sinf:ainf[n];        /* input frequency-scaling signal */
;;           phs += inc * inf;          /* long (48 bit) addition here */
;;           phs1 &= mtab;
;;      }
;;
;;      phs1 denotes the high-order word of phs, interpreted as an integer.
;;      See the documentation for oscfg regarding the scaling of parameter
;;      inc and frequency envelope inf.
;;
;;  USAGE RESTRICTIONS
;;      The wavetable length mtab+1 must be a power of 2
;;
;;  DSPWRAP ARGUMENT INFO
;;      oscgaf (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         (dspace)sina,(input)aina,(dspace)sinf,(input)ainf,
;;         inc,phs0h,phs0l,
;;         (dspace)stab,(address)atab,mtab
;;
;;  MAXIMUM EXECUTION TIME
;;      194 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      162 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/oscgaf.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscgf.asm - same as this one but without an amplitude envelope.
;;      /usr/lib/dsp/ugsrc/oscg.asm - same but without an amplitude or frequency envelope.
;;
;;  ALU REGISTER USE
;;      Y1 = table lookup result
;;      X1 = ina, inf = multiplicative amplitude or frequency envelope
;;      X0 = inc = fixed increment multiplied times input signal
;;      Y0 = address mask
;;       B = double-precision instantaneous phase
;;           B1 = integer part of table index
;;           B0 = interpolation constant (not used as such here)
;;       A = output sample = amp * Y1
;;
oscgaf    macro pf,ic,sout,aout0,sina,aina0,sinf,ainf0,inc0,phs0h,phs0l,stab,atab0,mtab0

          new_xarg pf\_oscgaf_\ic\_,atab,atab0
          new_xarg pf\_oscgaf_\ic\_,inc,inc0
          new_xarg pf\_oscgaf_\ic\_,aina,aina0

          new_yarg pf\_oscgaf_\ic\_,aout,aout0
          new_yarg pf\_oscgaf_\ic\_,ainf,ainf0
          new_yarg pf\_oscgaf_\ic\_,mtab,mtab0

          new_larg pf\_oscgaf_\ic\_,phs,phs0h,phs0l

; Set up data alu regs from state held in memory arguments

          move y:(R_Y)+,R_O        ; Output signal pointer
          move y:(R_Y)+,R_I2       ; Input freq signal pointer
          move y:(R_Y)+,Y0  x:(R_X)+,X1 ; Address mask, Wavetable base address
          move x:(R_X)+,X0         ; Increment scaler
          clr B x:(R_X)+,R_I1      ; Input amp signal pointer
          move l:(R_L),B10           ; Instantaneous phase
          move R_X,x:(R_L)         ; Save R_X for later restoral
          move X1,R_X              ; R_X used for wavetable base address

          and Y0,B      sout:-(R_O),A   ; mask phase in range, make A valid
          move B1,N_X

          do #I_NTICK,pf\_oscgaf_\ic\_wave1
            if "sina"!="sinf"
              if "sina"=='x'&&"sinf"=='y'
                move sina:(R_I1)+,X1  sinf:(R_I2)+,Y1
                mac  X0,Y1,B   A,sout:(R_O)+
                and  Y0,B      stab:(R_X+N_X),Y1
              else
                move sina:(R_I1)+,Y1  sinf:(R_I2)+,X1
                mac  X0,X1,B   A,sout:(R_O)+
                and  Y0,B      stab:(R_X+N_X),X1
              endif
            else
              if "sina"=='x'&&"sout"=='y'
                move sinf:(R_I2)+,Y1
                mac  X0,Y1,B  sina:(R_I1)+,X1  A,sout:(R_O)+
                and  Y0,B     stab:(R_X+N_X),Y1
              else
                move sina:(R_I1)+,X1
                move sinf:(R_I2)+,Y1
                mac  X0,Y1,B  A,sout:(R_O)+
                and  Y0,B     stab:(R_X+N_X),Y1
              endif
            endif
            mpyr Y1,X1,A   B1,N_X
pf\_oscgaf_\ic\_wave1 

          move A,sout:(R_O)+  ; last output
          move x:(R_L),R_X      ; Restore R_X
          move B10,l:(R_L)P_L     ; save phase in memory argument
          endm


