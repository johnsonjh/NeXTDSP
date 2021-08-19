; tscale.asm - Test program for ug macro scale.asm
;
; output should contain .1,.2,...,I_NTICK/10 successively halved
;
; verified: 4/8/87/jos
;
     include 'music_macros'        ; utility macros
     beg_orch 'tscale'             ; begin orchestra main program
          beg_yeb                  ; input vector
i           set 1
invec       dup I_NTICK,l1         ; generate length I_NTICK ramp
               dc 0.1*i
i              set i+1
            endm
          end_yeb
          new_yeb yvec,I_NTICK,0   ; Allocate y output vector
          new_xeb xvec1,I_NTICK,0  ; Allocate x output vector 1
          new_xeb xvec2,I_NTICK,0  ; Allocate x output vector 2
          beg_orcl                 ; begin orchestra loop
               scale orch,1,y,yvec,y,invec,.5   ; copy ramp to yvec
               outa orch,1,y,yvec               ; ramp out 1 (y to y)
               scale orch,2,x,xvec1,y,yvec,.5   ; ramp to xvec1
               outa orch,2,x,xvec1              ; ramp out 2 (y to x)
               scale orch,3,y,yvec,x,xvec2,0    ; zero y:yvec for test 3
               scale orch,5,y,yvec,x,xvec1,.5   ; ramp to yvec
               outa orch,3,y,yvec               ; ramp out 3 (x to y)
               scale orch,4,x,xvec2,x,xvec1,.25 ; ramp to xvec
               outa orch,4,x,xvec2              ; ramp out 4 (x to x)
               brk_orcl 1                       ; need only one tick
          end_orcl                              ; end of orchestra loop
     end_orch 'tscale'                          ; end of orchestra main program



