;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  10/17/87/jos - initial file created from oscg.asm
;;  08/02/88/jos - added amplitude parameter (no extra cost!)
;;  08/02/88/jos - verified
;;  09/29/88/jos - fixed phase clipping bug by clearing b2 on exit
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      oscgf (UG macro) - Oscillator with multiplicative frequency input
;;
;;  SYNOPSIS
;;      oscgf pf,ic,sout,aout0,sinp,ainp0,amp0,inc0h,phs0h,phs0l,stab,atab0,mtab0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_oscgf_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = output vector address
;;      sinp      = input signal memory space ('x' or 'y')
;;      ainp0     = input signal address
;;      amp0      = output amplitude
;;      inc0h     = increment in table-samples/sample, high-order word (no low!)
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
;;      x:(R_X)+       output amplitude         amp0
;;      y:(R_Y)+       output address           aout0
;;      y:(R_Y)+       input address            ainp0
;;      y:(R_Y)+       table address mask       mtab0
;;      l:(R_L)P_L     current phase            phs0h,phs0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of a sinusoid, reading an external signal
;;      input for scaling frequency.  Execution is 20% faster if the output
;;      signal memory space "sout" is 'y' memory.  In pseudo-C:
;;
;;      atab = x:(R_X)+;           /* wave table address */
;;      inc  = x:(R_X)+;           /* table increment */
;;      amp  = x:(R_X)+;           /* output amplitude */
;;      aout = y:(R_Y)+;           /* output address */
;;      ainp = y:(R_Y)+;           /* input address */
;;      mtab = y:(R_Y)+;           /* wave table length minus 1 */
;;      phs  = l:(R_L)P_L;         /* current table read pointer */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           sout:aout[n] = amp * stab:atab[phs1];
;;           inp = sinp:ainp[n];        /* input frequency-scaling signal */
;;           phs += inc * inp;          /* long (48 bit) addition here */
;;           phs1 &= mtab;
;;      }
;;
;;      phs1 denotes the high-order word of phs, interpreted as an integer.
;;
;;      There are two basic cases handled by oscgf. 
;;         (1) Phase Modulation (PM): 
;;                 inc is regarded as the PM index, or "deviation," and
;;                 inp is regarded as the phase-modulation signal.
;;         (2) Vibrato:
;;                 inc is regarded as the base frequency, and
;;                 inp is regarded as a multiplicative frequency envelope.
;;
;;      Since oscgf provides no low-order word "inc0l" for the phase increment
;;      as does oscg, it is desirable to reduce amplitude of inp so that
;;      the product inc * inp straddles comfortably the binary point
;;      in the middle of phs.  One convention, for example, might be to
;;      multiply all increments by 256 and divide all vibrato envelopes
;;      (or PM signals, as the case may be) by 256. This convention will
;;      provide 8 bits of table increment to the right of the binary point
;;      in the phase register which is usually enough.  This convention also
;;      allows vibrato to increase the frequency by a factor of up to 256.
;;
;;  USAGE RESTRICTIONS
;;      The wavetable length mtab+1 must be a power of 2
;;
;;  DSPWRAP ARGUMENT INFO
;;      oscgf (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         (dspace)sinp,(input)ainp,amp,inch,phsh,phsl,
;;         (dspace)stab,(address)atab,mtab
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/oscgf.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscg.asm - same as this one but without an input signal
;;
;;  ALU REGISTER USE
;;      Y1 = table lookup result
;;      X1 = amp, inf = amplitude or multiplicative frequency envelope
;;      X0 = inc = fixed increment multiplied times input signal
;;      Y0 = address mask
;;       B = double-precision instantaneous phase
;;           B1 = integer part of table index
;;           B0 = interpolation constant (not used as such here)
;;       A = output sample = amp * Y1
;;
oscgf     macro pf,ic,sout,aout0,sinp,ainp0,amp0,inc0h,phs0h,phs0l,stab,atab0,mtab0

          new_xarg pf\_oscgf_\ic\_,atab,atab0
          new_xarg pf\_oscgf_\ic\_,inc,inc0h
          new_xarg pf\_oscgf_\ic\_,amp,amp0

          new_yarg pf\_oscgf_\ic\_,aout,aout0
          new_yarg pf\_oscgf_\ic\_,ainp,ainp0
          new_yarg pf\_oscgf_\ic\_,mtab,mtab0

          new_larg pf\_oscgf_\ic\_,phs,phs0h,phs0l

; Set up data alu regs from state held in memory arguments

          move x:(R_X)+,R_I2       ; Wavetable base address
          move x:(R_X)+,X0         ; Increment = input scale
;!        move x:(R_X)+,<amp>      ; We use amplitude in its arg slot

          move y:(R_Y)+,R_O        ; Output signal pointer
          move y:(R_Y)+,R_I1       ; Input signal pointer
          move y:(R_Y)+,Y0         ; Address mask

          move l:(R_L),B           ; Instantaneous phase

          and Y0,B      sout:-(R_O),A   ; mask phase in range, make A valid
          move          B1,N_I2         ; extract wavetable address
          move          sinp:(R_I1)+,X1 ; input frequency

          do #I_NTICK,pf\_oscgf_\ic\_wave1
                if "sout"=='x'
                  mac X0,X1,B    A,sout:(R_O)+          ; B += inp*inc, ship A
                  move x:(R_X),X1                       ; load amplitude
                else
                  mac X0,X1,B    A,sout:(R_O)+   x:(R_X),X1 ; fast version
                endif
                and Y0,B        stab:(R_I2+N_I2),Y1 ; mask phase, table lookup
                mpy Y1,X1,A     B1,N_I2     ; apply amp, extract table address
                move sinp:(R_I1)+,X1        ; input frequency
pf\_oscgf_\ic\_wave1 
          if "sout"=='x'
            move x:(R_X)+,X1    ; increment past amplitude address arg
            move A,sout:(R_O)+  ; last output
          else
            move x:(R_X)+,X1    A,sout:(R_O)+   ; fast version
          endif
          move #<0,B2           ; must clear sign extension to avoid clip:
          move B,l:(R_L)P_L     ; save phase in memory argument
          endm


