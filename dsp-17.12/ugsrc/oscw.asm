;;  Copyright 1989,1990 by NeXT Inc.
;;  Author - J.0. Smith
;;
;;  Modification history
;;  --------------------
;;  08/29/87/jos - initial file created from oscs.asm
;;  10/05/87/jos - passed test/toscw.asm, both relative and absolute cases
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      oscw (UG macro) - Oscillator based on 2D vector rotation
;;
;;  SYNOPSIS
;;      oscw pf,ic,sout,aout0,c0,s0,u0,v0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_oscw_\ic\_ is globally unique)
;;      sout      = output waveform memory space ('x' or 'y')
;;      aout0     = initial output vector address
;;      c0        = initial value of cos(2*pi*fc/fs) where fc=osc freq, fs=srate
;;      s0        = initial value of sin(2*pi*fc/fs) where fc=osc freq, fs=srate
;;      u0        = initial value of first state variable
;;      v0        = initial value of second state variable
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use             Initialization
;;      ----------     --------------           --------------
;;      y:(R_Y)+       current output address   aout0
;;      x:(R_X)+       current c address        c0
;;      y:(R_Y)+       current s address        s0
;;      x:(R_X)+       current u address        u0
;;      y:(R_Y)+       current v address        v0
;;
;;  DESCRIPTION
;;
;;      Generate I_NTICK samples of a sinusoid by extracting the x-axis
;;      projection of a circular rotation in a plane.  Each sample is computed
;;      by rotating the 2D vector (u,v) through a fixed angle Theta = 2*pi*Fc/Fs,
;;      where Fc is the oscillation frequency and Fs is the sampling rate in Hz.
;;      Equivalently, the signal is generated as the real part of exp(j*w*n+phi) 
;;      where w equals 2*pi*Fc/Fs, n is the sample number (from 0), and phi
;;      is a phase offset.
;;
;;      The coefficients c and s can be computed as the cosine and sine,
;;      respectively, of rotation angle, i.e.,
;;
;;      c=cos(2*pi*Fc/Fs) and s=sin(2*pi*Fc/Fs).
;;
;;      The initial values of u and v correspond to the initial phase offset via 
;;
;;      u0=cos(p) and v0=sin(p).
;;
;;      In pseudo-C:
;;
;;      aout = y:(R_Y)+;           /* output address */
;;      c    = x:(R_X)+;           /* cosine of pole angle in radians */
;;      s    = y:(R_Y)+;           /* sine of pole angle in radians */
;;      u    = x:(R_X);            /* real part of motion around unit circle */
;;      v    = y:(R_Y);            /* imag part of motion around unit circle */
;;
;;      for (n=0;n<I_NTICK;n++) {
;;          sout_aout[n] = u;      /* output real part */
;;          t = c*u - s*v;         /* update real part */
;;          v = s*u + c*v;         /* update imag part */
;;          u = t;
;;      }
;;
;;      x:(R_X)+ =u;               /* for next time */
;;      y:(R_Y)+ =v;
;;
;;  DSPWRAP ARGUMENT INFO
;;      oscw (prefix)pf,(instance)ic,(dspace)sout,(output)aout,c,s,u,v
;;
;;  MINIMUM EXECUTION TIME
;;       (4*I_NTICK+7)/I_NTICK instruction cycles (4 inner loop + 7)
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/oscw.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/ugsrc/oscs.asm - simplest osc (also 4 instr. inner loop)
;;      /usr/lib/dsp/ugsrc/oscm.asm - masked wavetable index (for negative inc)
;;      /usr/lib/dsp/ugsrc/osci.asm - linear interpolation on table lookup
;;
;;  ALU REGISTER USE
;;      X0 = c
;;      Y0 = s
;;      X1 = u
;;      Y1 = v
;;       A = u
;;       B = v
;;

          define pfx "pf\_oscw_\ic\_"
	  define macpfx """pfx"""

oscw      macro pf,ic,sout,aout0,c0,s0,u0,v0
          new_yarg macpfx,aout,aout0
          new_xarg macpfx,c,c0
          new_yarg macpfx,s,s0
          new_xarg macpfx,u,u0
          new_yarg macpfx,v,v0

          move y:(R_Y)+,R_O             ; output pointer
          move x:(R_X)+,X0 y:(R_Y)+,Y0  ; c,s
          move x:(R_X),X1 y:(R_Y),B     ; u,v
          do #I_NTICK,pfx\tickloop
               mpy  X0,X1,A   B,Y1           ; A=c*u(n-1), Y1=v(n-1)
               macr -Y0,Y1,A ;B,sout:(R_O)+  ; A=u(n)=c*u(n-1)-s*v(n-1)
               mpy  Y0,X1,B   A,X1           ; B=s*u(n-1), X1=u(n)
               macr X0,Y1,B   A,sout:(R_O)+  ; B=v(n)=s*u(n-1)+c*v(n-1)
pfx\tickloop    
          move X1,x:(R_X)+    B,y:(R_Y)+     ; (u,v) for next time
          endm

