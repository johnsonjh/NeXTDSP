;
;Mono to stereo, buffering in external ram
;
	include	"dspsound.asm"
main
	jsr	start
loop
	jsr	getHost
	jsr	putHost
	jsr	putHost
	jcs	stop
    	nop
_endput
	jmp	loop


