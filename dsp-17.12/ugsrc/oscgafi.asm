;;  Copyright 1989 by NeXT Inc.
;;  Author - Michael McNabb
;;
;;  Modification history
;;  --------------------
;;  04/06/89/mmm - initial file created from oscgaf.asm
;;  04/18/89/mmm - rearranged order of loop and out-of-loop instructions to
;;                 reduce memory references.
;;  05/02/89/mmm - fixed negative phase bug, made use of R_Y to re-compact to
;;                 12 instructions.
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      oscgafi (UG macro) - Interpolating oscillator with amplitude and frequency
;;                       envelopes
;;
;;  SYNOPSIS
;;      oscgafi pf,ic,sout,aout0,sina,aina0,sinf,ainf0,inc0,phs0h,phs0l,stab,atab0,mtab0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_oscgafi_\ic\_ is globally unique)
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
;;      Generate I_NTICK samples of a sinusoid, reading external signal
;;      inputs for scaling both amplitude and frequency.  Execution is 9%
;;      faster if the amplitude envelope patchpoint space "sina" is 'x'.
;;
;;      In pseudo-C:
;;
;;      atab = x:(R_X)+;           /* wave table address */
;;      inc  = x:(R_X)+;           /* table increment */
;;      aina = x:(R_X)+;           /* amplitude envelope patchpoint address */
;;      aout = y:(R_Y)+;           /* output patchpoint address */
;;      ainf = y:(R_Y)+;           /* frequency envelope patchpoint address */
;;      mtab = y:(R_Y)+;           /* wave table length minus 1 */
;;      phs  = l:(R_L)P_L;         /* current table read pointer (phs1,phs0) */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           phs1 &= mtab;
;;           phs2 = (phs1+1) & mtab;
;;           s1   = stab:atab[phs1];            /* current sample */
;;           s2   = stab:atab[phs2];            /* following sample */
;;           diff = s2 - s1;
;;           samp = s1 + (diff * phs0);         /* interpolate */
;;           sout:aout[n] = sina:aina[n] * samp; /* scale by amplitude signal */
;;           inf = sinf:ainf[n];                /* input frequency-scaling signal */
;;           phs += inc * inf;                  /* long (48 bit) addition here */
;;      }
;;
;;      phs1 and phs0 denote the high-order and low-order words of phs, 
;;      interpreted as an integer.
;;      See the documentation for oscgf regarding the scaling of parameter
;;      inc and frequency envelope inf.
;;
;;  USAGE RESTRICTIONS
;;      The wavetable length mtab+1 must be a power of 2
;;
;;  DSPWRAP ARGUMENT INFO
;;      oscgafi (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         (dspace)sina,(input)aina,(dspace)sinf,(input)ainf,
;;         inc,phsh,phsl,(dspace)stab,(address)atab,mtab
;;
;;  MAXIMUM EXECUTION TIME
;;      488 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      456 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/oscgafi.asm
;;
;;  TEST PROGRAM
;;
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscgaf.asm - same as this one but non-interpolating
;;
;;  ALU REGISTER USE
;;      Y1 = final output sample,  difference sample
;;      X1 = amplitude envelope signal, freq envelope signal, interpolation fraction
;;      X0 = inc = fixed increment multiplied times input signal
;;      Y0 = address mask
;;       B = double-precision instantaneous phase
;;           B1 = integer part of table index
;;           B0 = interpolation constant
;;       A = phase+1, output sample after interpolation and amp scaling
;;
oscgafi   macro pf,ic,sout,aout0,sina,aina0,sinf,ainf0,inc0,phs0h,phs0l,stab,atab0,mtab0

          new_xarg pf\_oscgafi_\ic\_,atab,atab0
          new_xarg pf\_oscgafi_\ic\_,inc,inc0
          new_xarg pf\_oscgafi_\ic\_,aina,aina0

          new_yarg pf\_oscgafi_\ic\_,aout,aout0
          new_yarg pf\_oscgafi_\ic\_,ainf,ainf0
          new_yarg pf\_oscgafi_\ic\_,mtab,mtab0

          new_larg pf\_oscgafi_\ic\_,phs,phs0h,phs0l

; Set up data alu regs from state held in memory arguments

          move y:(R_Y)+,R_O        ; Output signal pointer
          move y:(R_Y)+,R_I1       ; Input increment signal pointer
          move x:(R_X)+,R_I2       ; Wavetable base address
          move x:(R_X)+,X0  y:(R_Y)+,Y0  ; Increment scaler, Address mask
          clr B x:(R_X)+,Y1         ; amplitude envelope input

          move l:(R_L),B10           ; Instantaneous phase

          move R_X,x:(R_L)         ; Save R_X for later restoral
          move R_Y,y:(R_L)         ; Save R_Y for later restoral
          move Y1,R_X              ; R_X used for amp envelope address
          move R_I2,R_Y            ; R_Y used also for wavetable base address

          and  Y0,B   #>1,A        ; mask phase, put a 1 in A
          add  B,A   B0,X1         ; A = phase+1, low phase to X1

          do #I_NTICK,pf\_oscgafi_\ic\_wave1
                and  Y0,A     B1,N_I2           ; mask phase+1, index1 to offset reg
                tfr  X1,A     A1,N_Y            ; low phase to A, index2 to offset reg
                lsr  A   stab:(R_I2+N_I2),Y1    ; table lookup 1
                move A1,X1                      ; X1 now has interpolation fraction
                move stab:(R_Y+N_Y),A           ; fraction to X1, table lookup 2
                sub  Y1,A                       ; get diff
                tfr  Y1,A     A,Y1              ; exchange sample 1 and diff
                macr X1,Y1,A  sinf:(R_I1)+,X1   ; interpolate, load increment
                if "sina"=='x'
                  mac  X0,X1,B   A,Y1  sina:(R_X)+,X1 ; compute new phase, get amp
                else
                  mac  X0,X1,B   A,Y1
                  move sina:(R_X)+,X1
                endif
                mpyr X1,Y1,A  B0,X1             ; apply amp, low phase to X1
                and  Y0,B     #>1,A     A,Y1    ; mask phase, 1 to a, sample to y1
                add  B,A      Y1,sout:(R_O)+    ; A=phase+1, move out sample
pf\_oscgafi_\ic\_wave1 
          move x:(R_L),R_X      ; Restore R_X
          move y:(R_L),R_Y      ; Restore R_Y
          move B10,l:(R_L)P_L     ; save phase in memory argument
          endm


