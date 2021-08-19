;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/06/87/jos - initial file created
;;  05/24/87/jos - passed test/toscs.asm
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      oscs (UG macro) - Simplest oscillator
;;
;;  SYNOPSIS
;;      oscs pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,stab,atab0,nper0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_oscs_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = initial output vector address
;;      amp0      = initial amplitude in [-1.0,1.0)
;;      inc0h     = initial increment in table-samples/sample, high-order word
;;      inc0l     = initial increment in table-samples/sample, low-order word
;;      phs0h     = initial phase in table-samples, high-order word
;;      phs0l     = initial phase in table-samples, low-order word
;;      stab      = table memory space
;;      atab0     = initial table address
;;      nper0     = initial period length = 1/2 table length
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     --------------           --------------
;;      x:(R_X)+       current table address    atab0
;;      x:(R_X)+       current period length    nper0
;;      y:(R_Y)+       current output address   aout0
;;      y:(R_Y)+       current amplitude        amp0
;;      l:(R_L)P_L     index increment          inc0h,inc0l
;;      l:(R_L)P_L     current phase            phs0h,phs0l
;;
;;  DESCRIPTION
;;      Generate I_NTICK samples of a sinusoid. In pseudo-C:
;;
;;      atab = x:(R_X)+;           /* wave table address */
;;      nper = x:(R_X)+;           /* period length = 1/2 table length */
;;      amp  = y:(R_Y)+;           /* current amplitude */
;;      aout = y:(R_Y)+;           /* output address */
;;      inc  = l:(R_L)P_L;         /* table increment */
;;      phs  = l:(R_L)P_L;         /* current table read pointer */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;           sout:aout[n] = amp * stab:atab[TRUNC(phs)]
;;           phs += inc;
;;      }
;;
;;      TRUNC(phs) denotes the high-order word of phs, interpreted as an integer.
;;
;;      The index increment inc can be calculated as the period length nper, 
;;      times the desired frequency frq in Hz, divided by the sampling rate Fs
;;      i.e.,
;;                   nper * frq      TableLength * Frequency 
;;            inc =  ----------   =  ----------------------- 
;;                        Fs                SamplingRate     
;;
;;  USAGE RESTRICTIONS
;;      (1) The increment cannot be negative (cf. fm synthesis).
;;      (2) The wavetable must contain TWO periods (ie, start at length 2*nper0)
;;      (3) The desired signal period must be at least I_NTICK samples, i.e.,
;;
;;          Period = SamplingRate/Frequency >= I_NTICK
;;
;;      In other terms, the increment times I_NTICK must not exceed TableLength.
;;
;;      The purpose of these restrictions is to avoid bounds checking in the 
;;      inner loop. If the frequency is too high, the table pointer (phase)
;;      will get outside the table bounds forever.  There is no run-time checking
;;      for the frequency going too high.
;;  
;;  DSPWRAP ARGUMENT INFO
;;      oscs (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;         amp,inch,incl,phsh,phsl,(dspace)stab,(address)atab,nper
;;
;;  MINIMUM EXECUTION TIME
;;       (4*I_NTICK+16)/I_NTICK instruction cycles (4 inner loop + 16)
;;       Assuming I_NTICK=8, this is 6 fast-instruction cycles = 600ns.
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/oscs.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscm.asm - osc with mask on wavetable index (allows negative inc)
;;      /usr/lib/dsp/ugsrc/osci.asm - osc with linear interpolation on table lookup
;;
;;  BUGS
;;      Because the modulo addressing mode of the chip does not catch
;;      wraparound in general for addresses of the form y:(R0+N0), this
;;      oscillator requires two successive periods to be stored in the
;;      wavetable, and the maximum frequency allowed is the sampling-rate
;;      divided by the tick size (Fs/8 = 5.5KHz at 44.1KHz for example).
;;      Unfortunately, this means we can't use the onchip sine ROM at all,
;;      when the external memory size is 8K words. (32K memory can use the ROM.)
;;      to do so would require we write a sinewave into low onchip y RAM and use 
;;      the ROM for the second cycle, but this would interfere with the use
;;      of onchip y and l RAM by the dsp system and other UGs.
;;
;;  ALU REGISTER USE
;;      X1 = Amplitude scale factor
;;      X0 = Table lookup result
;;       Y = double-precision phase increment (frequency)
;;       B = double-precision instantaneous phase
;;      B1 = integer part of table index
;;      B0 = interpolation constant
;;       A = Amp*Table(B) = output signal
;;
oscs      macro pf,ic,sout,aout0,amp0,inc0h,inc0l,phs0h,phs0l,stab,atab0,nper0
          new_xarg pf\_oscs_\ic\_,atab,atab0
          new_xarg pf\_oscs_\ic\_,nper,nper0
          new_yarg pf\_oscs_\ic\_,aout,aout0
          new_yarg pf\_oscs_\ic\_,amp,amp0
          new_larg pf\_oscs_\ic\_,inc,inc0h,inc0l
          new_larg pf\_oscs_\ic\_,phs,phs0h,phs0l

          if phs0h>nper0
               fail 'oscs: Initial phase outside of table bounds!'
          endif
          if inc0l==0
               if nper0<I_NTICK*inc0h
                    fail 'oscs: Table index will get lost. Reduce freq. inc0h'
               endif
          else
               if nper0<(I_NTICK-1)*(inc0h+1)          ; round inc0l up
                    warn 'oscs: Table index may get lost. Reduce increment inc0'
               endif
          endif

; Set up data alu regs from state held in memory arguments
          move x:(R_X)+,R_I2       ; Current table pointer = h.o.w. of phase
          move y:(R_Y)+,R_O        ; Current output pointer
          move y:(R_Y)+,X1         ; X1 = amplitude
          move l:(R_L)P_L,Y        ; Y = frequency (phase increment)
          move l:(R_L),B           ; B = current phase
; 1st sample
          move B1,N_I2             ; initial phase is used
          add Y,B                  ; compute next phase
          move stab:(R_I2+N_I2),X0 ; X0 = Table[b1.b0]
          mpyr X1,X0,A B1,N_I2          ; apply amplitude, set up next phase
; Remaining samples
          do #I_NTICK-1,pf\_oscs_\ic\_wave1
               add Y,B A,sout:(R_O)+    ; add in phase increment, ship
               move stab:(R_I2+N_I2),X0 ; X0 = Table[b1.b0]
               mpyr X1,X0,A B1,N_I2     ; apply amplitude, set up next phase
pf\_oscs_\ic\_wave1 
          move A,sout:(R_O)+       ; ship last sample this tick
          move x:(R_X)+,A          ; table length to A1
          neg A x:X_ZERO,B0        ; 0 to B0 makes B the integer part of phase
          add B,A                  ; subtract table length from phase
          tge A,B                  ; use subtract result if non-negative
          move B10,l:(R_L)P_L      ; save phase in arg blk
          endm


