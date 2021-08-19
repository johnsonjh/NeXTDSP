;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;      3/16/88/jos - changed (R_L)- to (R_L)P_L
;;      3/18/88/jos - changed from DO loops to REP loops
;;      4/12/88/jos - changed x:X_EPS to #>1
;;      4/18/88/jos - made aout0 into a memory argument!! Changed arg names
;;-----------------------------------------------------------------------------
;;  NAME
;;      slpdur (UG macro) - Linear envelope generation using slopes/durations
;;
;;  SYNOPSIS
;;      slpdur pf,ic,sout,aout0,val0,slp0,anslp0,dur0,andur0
;;
;;  MACRO ARGUMENTS
;;      pf    = global label prefix (any text unique to invoking macro)
;;      ic    = instance count (such that pf\_slpdur_\ic\_ is globally unique)
;;      sout  = output envelope memory space ('x' or 'y')
;;      aout0 = initial output address in memory sout (0 to $FFFF)
;;  The remaining args provide optional initialization for dsp memory args:
;;      val0  = Initial envelope Value ($800000 to $7FFFFF)
;;      slp0  = Slope 1 Value = initial envelope slope ($800000 to $7FFFFF)
;;      anslp0 = Slope 2 Pointer = address of next envelope slope (0 to $FFFF)
;;      dur0  = Duration 1 Value = initial segment duration ($800000 to $7FFFFF)
;;      andur0 = Duration 2 Pointer = address of next duration (0 to $FFFF)
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access     Argument use
;;      ----------     ------------
;;      x:(R_X)+       Current output
;;      x:(R_L)P_L     Current value [y:(R_L) not used]
;;      x:(R_X)+       Current slope 
;;      x:(R_X)+       Next-slope pointer
;;      y:(R_Y)+       Remaining duration
;;      y:(R_Y)+       Next-duration pointer
;;
;;  DESCRIPTION
;;      The slpdur unit-generator computes piecewise-linear envelope
;;      functions specified by slope and duration tables in dsp memory.
;;      The envelope and its slope/duration specifications are single 
;;      precision values (24 bits).  The slope list must be in x memory
;;      and the duration list must be in y memory.
;;      
;;      The "slope" of each envelope segment is the actual 24-bit number which
;;      is added to the current envelope value to produce the next envelope
;;      value.
;;      
;;      The "duration" of each segment is the number of ticks (not samples) the
;;      current slope is added to the current value before going to the
;;      next envelope segment. When the duration is counted down to zero, the 
;;      next slope-duration pair is selected from the envelope slope-duration
;;      tables.
;;      
;;      The last envelope segment is followed by a "dummy" envelope segment
;;      having a negative duration:
;;      
;;       END-SEGMENT
;;        DURATION          MEANING
;;      ------------        -------
;;      -1 ($FFFFFF)        Decay exponentially to zero 
;;      -2 ($FFFFFE)        Remain at current value
;;      
;;      The -1 duration means to decay exponentially to zero using the decay
;;      constant passed in the slope argument of the dummy end-segment. That
;;      is, if the end-slope is 0.99, the current envelope is drecreased by 
;;      0.99 EVERY OTHER sample (until the envelope is re-initialized) 
;;      providing an exponential decay to zero (time constant = 2/(1-0.99) 
;;      = 200 samples). If A is the envelope value at the beginning
;;      of an exponential decay, and the decay constant is r, the output
;;      during the dummy segment is A, r*A, r*A, r*r*A, r*r*A, r^3*A, 
;;      r^3*A, etc.  The reason for decreasing every other sample instead
;;      of every sample (A, r*A, r^2*A, r^3*A, etc.) is in order to obtain
;;      one instruction in the DO loop for termination decay instead of two.
;;      There is apparently no way to obtain an exponential decay in a one-
;;      instruction DO loop because MPY cannot write its input registers.
;;      This means the sampling rate during the final exponential decay is
;;      reduced by half.  This should not be audible when sampling rates
;;      of 44.1KHz or 22.05KHz are used.
;;      
;;      The -2 duration means "stick" at the current value. It is
;;      equivalent to a zero slope and infinite duration. The actual slope
;;      argument is ignored.
;;      
;;      The envelope generator makes use of 5 dsp memory arguments to store its
;;      running state. These arguments hold
;;      
;;           current value        (envelope value)
;;      
;;           current slope            (for the segment in progress)
;;           next-slope pointer       (pointer to next element of slope table)
;;      
;;           remaining duration       (count-down for segment in progress)
;;           next-duration pointer    (pointer to next element of dur table)
;;      
;;      The envelope can be initialized, reset, or altered at any time by
;;      overwriting these arguments in dsp memory.  When the envelope reaches
;;      the end of a segment, it automatically advances the next-segment 
;;      pointers and updates the current-value and remaining-duration 
;;      to be those found in the next segment.  Thus, normally the envelope
;;      is initialized only once per instance.
;;      
;;      In addition to the running-state arguments, there is an output-vector
;;      argument which places the envelope samples. The envelope output
;;      (following usual unit-generator conventions) is a length I_NTICK
;;      signal vector addressed sequentially as sout:(R_O)+.
;;  
;;  DSPWRAP ARGUMENT INFO
;;      slpdur  (prefix)pf,(instance)ic,(dspace)sout,(output)aout,val,slp,
;;              (address)anslp,dur,(address)andur
;;
;;  MINIMUM EXECUTION TIME
;;      (I_NTICK+11)/I_NTICK dsp instruction cycles (1 inner loop + 11 other)
;;      Typical execution time occurs within a single line segment.
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/slpdur.asm
;;
;;  ALU REGISTER USE
;;         X0 = 0 or 1 for duration decrement, next-slope pointer
;;         X1 = next-slope pointer
;;         Y0 = current duration
;;          A = current slope
;;          B = remaining duration
;;
slpdur    macro pf,ic,sout,aout0,val0,slp0,anslp0,dur0,andur0    ; entry
          new_xarg pf\_slpdur_\ic\_,aout,aout0          ; Output location
          new_larg pf\_slpdur_\ic\_,val,val0,0          ; Envelope State
          new_xarg pf\_slpdur_\ic\_,slp,slp0            ; Current slope
          new_xarg pf\_slpdur_\ic\_,anslp,anslp0        ; Next-slope pointer
          new_yarg pf\_slpdur_\ic\_,dur,dur0            ; Current 
          new_yarg pf\_slpdur_\ic\_,andur,andur0        ; Next-duration pointer
        
          move x:(R_X)+,R_O         ; output pointer
          move x:(R_X)+,A y:(R_Y),B ; current slope and duration
          tst B     #0,X0          ; set ccr according to duration
          jgt pf\_slpdur_\ic\_decr ; count duration down to zero
          jeq pf\_slpdur_\ic\_next ; 0 duration => advance to next segment
pf\_slpdur_\ic\_neg_dur            ; negative duration => special mode
          move x:(R_L),X0          ; load current val to X0
          move #-2,Y0              ; code for "stick"
          cmp Y0,B  x:(R_X)+,X1 y:(R_Y)+,Y0 ; Dur=-2?, prepare R_X,R_Y for exit
          jne pf\_slpdur_\ic\_dec  ; -2 => envelope sticks, else exp decay
          move X0,A                ; load current value into A
          rep #I_NTICK             ; compute a tick's worth of stuck envelope
               move A,sout:(R_O)+  ; inner loop
          jeq pf\_slpdur_\ic\_tickl   ;   store current val, do (R_Y)+, and exit
pf\_slpdur_\ic\_dec                   ; decay to zero:
          if "sout"=='y' 
               move x:(R_L),X0 A,Y0     ; current val to X0, decay ratio to Y0
               move X0,A                ; current val to A (=1st out this tick)
               rep #I_NTICK             ; compute a tick of decaying env
                    mpy X0,Y0,A A,X0 A,y:(R_O)+ ; inner loop
          else
               move x:(R_L),Y0          ; current val to Y0
               tfr Y0,A A,X0            ; current val to A, decay ratio to X0
               rep #I_NTICK             ; compute a tick of decaying env
                    mpy X0,Y0,A A,x:(R_O)+ A,Y0 ; inner loop
          endif
          jmp pf\_slpdur_\ic\_tickl     ; exit as above
pf\_slpdur_\ic\_next                    ; advance to next segment
          lua (R_Y)+,R_Y                ; advance from currentDur to nextDurPtr
          move x:(R_X)-,R_I1            ; load slope pointer, point to slope arg
          move y:(R_Y)-,R_I2            ; load dur pointer, point to dur arg
          move x:(R_I1)+,A              ; new current slope
          move y:(R_I2)+,B              ; new current duration
          move A,x:(R_X)+ B,y:(R_Y)+    ; new current slope,duration in arg blk
          move R_I1,x:(R_X)             ; new next-slope pointer
          move R_I2,y:(R_Y)-            ; new next-duration pointer
          tst B                         ; check new duration
          jlt pf\_slpdur_\ic\_neg_dur   ; if negative, we're at the end
          jeq pf\_slpdur_\ic\_cont      ; if zero, don't decrement duration:
pf\_slpdur_\ic\_decr                    ; both 0 and 1 mean single-tick segment
          move #>1,X0                   ; load duration decrement ($000001)
pf\_slpdur_\ic\_cont     
          sub X0,B x:(R_X)+,X0 A,Y0     ; decr count, incr R_X, slope to Y0
          move x:(R_L),A B,y:(R_Y)+     ; load cur val, store new count
          rep #I_NTICK                  ; compute a tick of envelope
               add Y0,A  A,sout:(R_O)+  ; inner loop, one-sample pipe delay
pf\_slpdur_\ic\_tickl    move A,x:(R_L)P_L y:(R_Y)+,Y0 ; save current val, incr R_Y
     endm


