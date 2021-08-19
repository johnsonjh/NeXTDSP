;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/06/87/jos - initial file created
;;  05/06/87/jos - passed test/toscm{,2,3,4}.asm
;;  07/31/87/jos - generalized to allow wavetable and output in any space
;;  10/17/87/jos - changed name from oscm ("mask") to oscg ("general")
;;                   because now "m" denotes "modulo" (e.g., oscm,delaym)
;;  09/30/88/mmm - fixed clobbering of low-order inc word (space error)
;;  04/27/89/mmm - rewrote to allow 3-instruction case where sout==y
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      oscg (UG macro) - Simplest oscillator with general address mask
;;
;;  SYNOPSIS
;;      oscg pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,stab,atab0,mtab0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_oscg_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = initial output vector address
;;      amp0      = initial amplitude in [-1.0,1.0)
;;      inc0h     = initial increment in table-samples/sample, high-order word
;;      inc0l     = initial increment in table-samples/sample, low-order word
;;      phs0h     = initial phase in table-samples, high-order word
;;      phs0l     = initial phase in table-samples, low-order word
;;      stab      = table memory space ('x' or 'y')
;;      atab0     = address of first word of wave table
;;      mtab0     = table length - 1 = power of 2 minus 1
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     --------------           --------------
;;      x:(R_X)+       current table address    atab0
;;      x:(R_X)+       table address mask       mtab0
;;      y:(R_Y)+       current output address   aout0
;;      y:(R_Y)+       current amplitude        amp0
;;      l:(R_L)P_L     index increment          inc0h,inc0l
;;      l:(R_L)P_L     current phase            phs0h,phs0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of a sinusoid. In pseudo-C:
;;
;;      atab = x:(R_X)+;           /* wave table address */
;;      mtab = x:(R_X)+;           /* wave table length minus 1 */
;;      aout = y:(R_Y)+;           /* output address */
;;      amp  = y:(R_Y)+;           /* current amplitude */
;;      inc  = l:(R_L)P_L;         /* table increment */
;;      phs  = l:(R_L)P_L;         /* current table read pointer */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           sout:aout[n] = amp * stab:atab[phs1];
;;           phs += inc;
;;           phs1 &= mtab;
;;      }
;;
;;      phs1 denotes the high-order word of phs, interpreted as an integer.
;;
;;      The index increment inc can be calculated as the table length ntab=mtab+1,
;;      times the desired frequency frq in Hz, divided by the sampling rate Fs
;;      i.e.,
;;                   ntab * frq      TableLength * Frequency 
;;            inc =  ----------   =  ----------------------- 
;;                        Fs                SamplingRate     
;;
;;      Note that negative increments are allowed.
;;
;;      /usr/lib/dsp/ugsrc/oscs.asm is slightly faster than this one (oscg), but if the 
;;      onchip sine ROM is used, oscg is the same speed (when sout=y) because
;;      oscs has to use external memory for its wavetable (assuming 8K words
;;      of external memory in use; if 32K memory is in use, half of the 
;;      wavetable could be the onchip sine ROM and oscs would be a faster 
;;      sinewave generator in that case).
;;
;;  USAGE RESTRICTIONS
;;      The wavetable length mtab+1 must be a power of 2
;;      The wavetable increment inc0h must be nonnegative
;;
;;  DSPWRAP ARGUMENT INFO
;;      oscg (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         amp,inc0h,inc0l,phs0h,phs0l,(dspace)stab,(address)atab,mtab
;;
;;  MAXIMUM EXECUTION TIME
;;      152 DSP clock cycles for one "tick" which equals 16 audio samples.
;;
;;  MINIMUM EXECUTION TIME
;;      120 DSP clock cycles for one "tick".
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/oscg.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscs.asm - simplest oscillator (uses modulo addressing mode)
;;      /usr/lib/dsp/ugsrc/osci.asm - osc with linear interpolation on table lookup
;;
;;  ALU REGISTER USE
;;      Y1 = Amplitude scale factor
;;      Y0 = Table lookup result
;;       X = double-precision phase increment (frequency)
;;           X0 = wavetable address mask also
;;       B = double-precision instantaneous phase
;;           B1 = integer part of table index
;;           B0 = interpolation constant (not used as such here)
;;       A = Amp*Table(B) = output signal
;;
oscg      macro pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,stab,atab0,mtab0
          new_xarg pf\_oscg_\ic\_,atab,atab0
          new_xarg pf\_oscg_\ic\_,mtab,mtab0
          new_yarg pf\_oscg_\ic\_,aout,aout0
          new_yarg pf\_oscg_\ic\_,amp,amp0
          new_larg pf\_oscg_\ic\_,inc,inc0h,inc0l
          new_larg pf\_oscg_\ic\_,phs,phs0h,phs0l

; Set up data alu regs from state held in memory arguments

          move x:(R_X)+,R_I2       ; Wavetable base address
          move x:(R_X),X0          ; address mask to X0
          clr B  l:(R_L)P_L,Y        ; Y = phase increment
          move l:(R_L),B10         ; B = current phase
          and  X0,B  y:(R_Y)+,R_O  ; mask initial phase, get current output pointer
          move B1,N_I2             ; current phase
          move y:(R_Y)+,X1         ; X1 = amplitude

          do #I_NTICK,pf\_oscg_\ic\_wave1
                and X0,B       stab:(R_I2+N_I2),X0
                mpyr X1,X0,A   B1,N_I2                  
                if "sout"=='y'
                  add Y,B   x:(R_X),X0   A,sout:(R_O)+    ; Ship output     
                else
                  add Y,B   x:(R_X),X0
                  move      A,sout:(R_O)+    ; Ship output     
                endif
pf\_oscg_\ic\_wave1 
          move  B10,l:(R_L)P_L ; save phase in memory argument
          move  x:(R_X)+,B          ; increment R_X
          endm


