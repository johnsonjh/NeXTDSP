;
;Stereo to mono, 8 to 16 bit
;
	include	"dspsound.asm"
main
	jsr	start
loop
	jsr	getHost
	move	#>$80,x0
	move	a,y0
	mpy	x0,y0,a
	move	a0,x:0
	move	x:0,a
	jsr	putHost
	jsr	putHost
	jcs	stop
	jmp	loop


