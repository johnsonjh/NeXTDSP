;
; decompress.asm
;	Decompress a sound compressed by Richard Crandall's Parallel Packing
;	technique.  The sound starts with a small subheader.  Each block is
;	one packed channel and starts with an algorithm code.  Each block is optionally
;	followed by a block of "residue" shorts for bit-faithful decompression.
;
;	Another asm source file must include file this after including i/o code.
;	See hostdecompress.asm and sndoutdecompress.asm.
;
; Restrictions:
;	max dmasize		4096	(see performsound.c and dspsounddi.asm)
;	max channelCount	2	from soundfile header
;	max encodeLength	512	from soundfile subheader
;	max numDropped		8	from soundfile subheader
;
;	Subheader method is 1 for bit-faithful, 0 for non-bit-faithful
;
; External Memory Usage:
;	x:$2000 (4096)	dma buffers used by dspsounddi.asm
;	x:$3000 (512)	packed buffer
;	x:$3200	(512)	unpacked and decoded left channel
;	x:$3400	(512)	unpacked and decoded right channel
;	x:$3600	(512)	unpacked left channel residue
;	x:$3800	(512)	unpacked right channel residue
;	p:$3a00		code
;
; Modification History:
;	02/20/90/mtm	Original version.
;	03/13/90/mtm	Better bit unpacking, input in word mode, restart check.
;	03/19/90/mtm	Buffered input.
;	05/16/90/mtm	Removed buffered input.
;	06/14/90/mtm	Use dspsounddi.asm.
;	07/13/90/mtm	Eat sound header new sent by host.
;	07/16/90/mtm	Use r5 instead of r6 in readChan.
;	07/26/90/mtm	Don't include i/o code.
;	08/15/90/mtm	Fix bit-faithful mode.
;	09/28/90/mtm	Don't try restarting when done, just flush (bug #7909).
;	10/02/90/mtm	Only dup mono channel sample in sndout case (bug #10031).
;	10/03/90/mtm	Support zero length sound header (bug #7912).
	
;---------------------------------------------------------------
; Equates
;---------------------------------------------------------------

BIT_FAITHFUL		equ	1	; method is 0 for non-bit-faithful

MAX_DMASIZE		equ	4096	; see dspsounddi.asm
MAX_ENCODE_LENGTH	equ	512	; max samples in an encode buffer

EXTERNAL_CODE	equ	XRAMLO+MAX_DMASIZE+MAX_ENCODE_LENGTH*5

;---------------------------------------------------------------
; Internal l memory
;---------------------------------------------------------------
	org	l:0
curSample	dc	0
numSamples	ds	1
	
;---------------------------------------------------------------
; Internal x memory
;---------------------------------------------------------------
	org	x:2
	
; Flags
flags		dc	0
FINISHED	equ	0	; finished flag bit number

; Decompression routines
routines	dc	null,xor,d1,d2,d3,d4,d3_11,d3_22,d4_222,d4_343,d4_101

; Number of unencoded (leading) samples for each routine
leaderCount	dc	0,1,1,2,3,4,3,3,4,4,4

; Masks for unpacking bits
masks		dc	$0,$1,$3,$7,$f
		dc	$1f,$3f,$7f,$ff
		dc	$1ff,$3ff,$7ff,$fff
		dc	$1fff,$3fff,$7fff,$ffff
		
; Multiply factors to implement bit shifting
; See "Fractional and Integer Arithmetic...", Motorola APR3/D, p. 9-11
rightShifts	dc	0,@pow(2,-1),@pow(2,-2),@pow(2,-3),@pow(2,-4)
		dc	@pow(2,-5),@pow(2,-6),@pow(2,-7),@pow(2,-8)
		dc	@pow(2,-9),@pow(2,-10),@pow(2,-11),@pow(2,-12)
		dc	@pow(2,-13),@pow(2,-14),@pow(2,-15)
leftShifts	dc	0,@cvi(@pow(2,1-1)),@cvi(@pow(2,2-1)),@cvi(@pow(2,3-1))
		dc	@cvi(@pow(2,4-1)),@cvi(@pow(2,5-1)),@cvi(@pow(2,6-1))
		dc	@cvi(@pow(2,7-1)),@cvi(@pow(2,8-1)),@cvi(@pow(2,9-1))
		dc	@cvi(@pow(2,10-1)),@cvi(@pow(2,11-1)),@cvi(@pow(2,12-1))
		dc	@cvi(@pow(2,13-1)),@cvi(@pow(2,14-1)),@cvi(@pow(2,15-1))

; Variables set from sound subheader
headerSize	ds	1
channelCount	ds	1
method		ds	1
numDropped	ds	1
encodeLength	ds	1
dropMask	ds	1	; calculated from numDropped


; Variables set from each buffer's encode type
algorithm	ds	1
numBits		ds	1
sign		ds	1
csign		ds	1

; Current buffer pointers for getResidue
currentBuf	ds	1
currentResidue	ds	1

;---------------------------------------------------------------
; External x memory
; External memory buffers start after the dma buffer used by dspsounddi.asm
;---------------------------------------------------------------
	org	x:XRAMLO+MAX_DMASIZE
	
; Buffer to hold packed data
packedBuf	ds	MAX_ENCODE_LENGTH

; Buffer for each unpacked and decoded channel
leftChannel	ds	MAX_ENCODE_LENGTH
rightChannel	ds	MAX_ENCODE_LENGTH

; Buffer for each unpacked residue
leftResidue	ds	MAX_ENCODE_LENGTH
rightResidue	ds	MAX_ENCODE_LENGTH
	
;---------------------------------------------------------------
; Macros
;---------------------------------------------------------------

;---------------------------------------------------------------
; Read an int (32 bits) from the host sent as 2 words
; Returns the low order short in a1
getInt16 macro
	jsr	getHost		; ignore high word
	jsr	getHost
	endm
	
;---------------------------------------------------------------
; Read an int (32 bits) from the host sent as 2 words
; Returns a 48 bit int in a
; Destroys y0, x0, x1
getInt48 macro
	jsr	getHost
	move	a1,y0
	jsr	getHost
	move	a1,x0
	move	#>@cvi(@pow(2,8-1)),x1	; left shift by 8 multiply factor
	mpy	x0,x1,a
	move	y0,a1
	rep	#8
	asr	a
	endm
	
;---------------------------------------------------------------
; Return the number of shorts needed to pack s samples using n bits per sample
; i.e. (n*s+15)/16
; Destroys n and s
pshorts	macro	n,s,acc
	mpy	n,s,acc		; numBits*packShorts
	asr	a  #>15,n	; integer multiply
	move	acc\0,acc\1
	add	n,acc  #@pow(2,-4),s	; right shift multiply factor
	move	acc\1,n
	mpy	n,s,acc
	endm
	
;---------------------------------------------------------------
; Copy shorts in buffer at rb into shorts in buffer at rs
; Result is sign extended
; cnt is the number shorts
; Advances 'rb' and 'rs'
; Destroys a and x0
moveSigned macro	rb,rs,cnt
	.loop	cnt
		clr	a
		btst	#15,x:(rb)
		.if	<cs>
			move	#<$ff,a
		.endi
		move	x:(rb)+,x0
		or	x0,a
		move	a1,x:(rs)+
	.endl
	endm

;---------------------------------------------------------------
; Copy shorts in buffer at rb into shorts in buffer at rs
; Result is NOT sign extended
; cnt is the number shorts
; Advances 'rb' and 'rs'
; Destroys a
moveUnsigned macro	rb,rs,cnt
	.loop	cnt
		move	x:(rb)+,a
		move	a1,x:(rs)+
	.endl
	endm

;---------------------------------------------------------------
; Negate acc if the sign bit (bit set in rs) in acc1 is set
; rc must contain the complement of rs
; Destroys y1
signx	macro	rs,rc,acc
	and	rs,acc  acc\1,y1 ; test sign bit
	move	y1,acc
	.if	<ne>
		and	rc,acc	 ; clear sign bit
		neg	acc
	.endi
	endm

;---------------------------------------------------------------
; Integer multiply by 3
times3	macro	accA,accB
	asl	accA  accA,accB
	add	accB,accA 
	endm

;---------------------------------------------------------------
; Integer multiply by 4
times4	macro	acc
	asl	acc
	asl	acc
	endm

;---------------------------------------------------------------
; This code follows after dspsound.asm code in internal p memory
;---------------------------------------------------------------
	org	p:
	
;---------------------------------------------------------------
; Read next (non-DMA) value from host
readHost
	jclr	#HRDF,x:HSR,readHost
	movep	x:HRX,a
	rts
	
;---------------------------------------------------------------
; Get parameters from host
; Destroys y1 and a
getParams
	move	x:headerSize,a
	tst	a
	jeq	noHeader
	.loop	a1
		jsr	getHost		; eat sound header
		nop
		nop
	.endl
noHeader
	getInt48			; originalSize in bytes
	move	x:channelCount,y1
	rep	y1
	asr	a
	move	a,l:numSamples
	getInt16
	move	a1,x:method
	getInt16
	move	a1,x:numDropped
	.if	x:method <eq> #>BIT_FAITHFUL
		move	x:numDropped,n0
		clr	b  #leftShifts,r0
		not	b
		move	b1,x0
		move	x:(r0+n0),x1		; left shift multiply factor
		mpy	x0,x1,a
		move	a0,a1
		not	a
		move	a1,x:dropMask		; dropMask = ~((~0)<<numDropped)
	.endi
	getInt16
	move	a1,x:encodeLength
	getInt16		; reserved ignored
	rts
	
;---------------------------------------------------------------
; Get the buffer encode type
; High byte is the algorithm, low byte is the number of bits used
; Creates sign and csign variables
; Destroys r0, n0, a, b, x0, and x1
getType
	jsr	getHost
	move	a1,x0
	move	#@pow(2,-8),x1	; multiply factor for right shift
	mpy	x0,x1,a  x0,b
	move	a1,x:algorithm
	.if	a1 <eq> #0	; algorithm=0 means buffer was not encoded
		move	#>16,b
		move	b1,x:numBits
		rts		; sign and csign don't matter
	.else
		move	#>$ff,x0
		and	x0,b
		move	b1,x:numBits
	.endi
	.if	x:method <eq> #>BIT_FAITHFUL
		move	x:numDropped,a
		sub	a,b
	.endi
	move	#>1,a
	sub	a,b			; numBits-1
	move	a1,x:sign
	tst	b  b1,n0
	.if	<ne>
		move	#leftShifts,r0
		move	#>1,x0
		move	x:(r0+n0),x1	; left shift multiply factor
		mpy	x0,x1,a		; 1<<numBits-1
		move	a0,a1
		move	a1,x:sign	; sign bit
	.endi
	not	a
	move	a1,x:csign		; complement of sign
	rts
	
;---------------------------------------------------------------
; Return the number of shorts in a packed and encoded buffer in a1
; Destroys r6, n6, y0, y1, a, and b
; In pseudo-C:
;	packShorts = encodeLength-leaderCount
;	return(leaderCount + (numBits*packShorts+15)/16)
shortsInBlock
	move	#leaderCount,r6
	move	x:algorithm,n6
	move	x:numBits,y0
	move	x:(r6+n6),y1	; number of leading samples
	move	x:encodeLength,a
	sub	y1,a  y1,b
	move	a1,y1		; packShorts
	pshorts	y0,y1,a
	add	b,a		; add number of leading samples
	rts
	
;---------------------------------------------------------------
; Read packed channel buffer
; Destroys r5, n5, x0, y0, y1, a, and b
readChan
	jsr	shortsInBlock
	move	#packedBuf,r5
	.loop	a1
		jsr	getHost
		move	a1,x:(r5)+
		nop
	.endl
	rts
	
;---------------------------------------------------------------
; Get residue bits.
; Destroys r0, n0, r2, r3, y0, y1, x0, a
getResidue
	move	#rightShifts,r0
	move	x:numDropped,n0
	move	x:currentResidue,r3
	move	x:(r0+n0),y1		; right shift multiply factor
	move	x:currentBuf,r2
	move	x:encodeLength,a
	move	x:dropMask,x0
	.loop	a1
		move	x:(r2),a	; dat[j]
		move	a1,y0
		and	x0,a		; dat[j] & dropMask
		move	a1,x:(r3)+
		mpy	y0,y1,a		; dat[j] >> numDropped
		move	a1,x:(r2)+
	.endl
	rts
	
;---------------------------------------------------------------
; Uncompand and send samples to stereo out - bit faithful version
; Count is passed in y0
; Destroys r0, n0, r1, r2, r3, x0, x1, y0, and a
; Inner loop code is duplicated to optimize for speed
sendFaithful
	move	#leftShifts,r0
	move	x:numDropped,n0
	move	#rightChannel,r1
	move	x:(r0+n0),x1		; left shift multiply factor
	move	#leftChannel,r0
	move	#leftResidue,r2
	move	#rightResidue,r3
	.if	x:channelCount <eq> #>2
		.loop	y0
			move	x:(r0)+,x0		; left channel
			mpy	x0,x1,a  x:(r2)+,y0	; residue
			move	a0,a1
			or	y0,a
			jsr	putHost
			move	x:(r1)+,x0		; right channel
			mpy	x0,x1,a  x:(r3)+,y0	; residue
			move	a0,a1
			or	y0,a
			jsr	putHost
			nop			; LA
		.endl
	.else
		.loop	y0
			move	x:(r0)+,x0		; left channel
			mpy	x0,x1,a  x:(r2)+,y0	; residue
			move	a0,a1
			or	y0,a
			jsr	putHost
	if	WRITE_SNDOUT
			jsr	putHost		; repeat same sample in right channel
	endif
			nop			; LA
		.endl
	.endi
	rts
	
;---------------------------------------------------------------
; Uncompand and send samples to stereo out - non-bit faithful version
; Count is passed in y0
; Destroys r0, n0, r1, x0, x1, and a
; Inner loop code is duplicated to optimize for speed
sendSamples
	move	#leftShifts,r0
	move	x:numDropped,n0
	move	#rightChannel,r1
	move	x:(r0+n0),x1		; left shift multiply factor
	move	#leftChannel,r0
	.if	x:channelCount <eq> #>2
		.loop	y0
			move	x:(r0)+,x0	; left channel
			mpy	x0,x1,a
			move	a0,a1
			jsr	putHost
			move	x:(r1)+,x0	; right channel
			mpy	x0,x1,a
			move	a0,a1
			jsr	putHost
			nop			; LA
		.endl
	.else
		.loop	y0
			move	x:(r0)+,x0	; left channel
			mpy	x0,x1,a
			move	a0,a1
			jsr	putHost
	if	WRITE_SNDOUT
			jsr	putHost		; repeat same sample in right channel
	endif
			nop			; LA
		.endl
	.endi
	rts
	
	if	*>$200
	warn	'INTERNAL P MEMORY OVERFLOW!!!'
	endif
	
;---------------------------------------------------------------
; This code follows buffers in external memory
;---------------------------------------------------------------
	org	p:EXTERNAL_CODE
	
;---------------------------------------------------------------
; Unpack bits into shorts
; On input:
;   r1	packed shorts (source, advanced)
;   r2	unpacked shorts (destination, advanced)
;   y0  number of resulting unpacked shorts (destroyed)
;   y1	number of bits per packed short (preserved)
; Register usage (destroyed):
;   r3	right shift multiply constants
;   n3	right shift amount
;   r4	left shift mulitply constants
;   n4  left shift amount
;   r5	masks
;   n5	bit phase
;   x0	scratch
;   x1	saved packed short
;   y0	scratch
;   b	"next" bit phase
;   a	scratch
unpack
	.if	y1 <eq> #0	; zero buffer and return if num bits==0
		clr	a
		rep	y0
		move	a1,x:(r2)+
		rts
	.endi
	move	#rightShifts,r3
	move	#leftShifts,r4
	move	#masks,r5
	clr	b  #16,n5	 ; phase = 0
	
	.loop	y0
	tst	b  b,n5		 ; phase == 0?
	jne	more
	move	#>16,b		 ; phase = 16
	sub	y1,b  x:(r1)+,x1 ; phase -= numBits, get next packed short
	move	b,n3
	jmp	nomask
more
	sub	y1,b		; phase -= numBits
	jgt	nosplit
	jlt	split
	move	x:(r5+n5),a	; mask[phase]
	and	x1,a 
	move	a1,x:(r2)+	; save unpacked short
	jmp	continue
split
	abs	b  x:(r5+n5),a	; mask[phase]
	and	x1,a  b,n4
	move	a1,x1
	move	x:(r4+n4),x0  b,y0
	mpy	x1,x0,a  #>16,b	  ; left shift (numBits - phase)
	sub	y0,b  x:(r1)+,x1  ; 16 - numBits - phase, get next packed short
	move	b,n3
	move	a0,y0
	move	x:(r3+n3),x0
	mpy	x1,x0,a		; right shift (16 - numBits - phase)
	or	y0,a
	move	a1,x:(r2)+	; save unpacked short
	jmp	continue
nosplit
	move	x:(r5+n5),a	; mask[phase]
	and	x1,a  b,n3
	move	a1,x1
nomask
	move	x:(r3+n3),x0
	mpy	x1,x0,a		; right shift (phase - numBits)
	move	a1,x:(r2)+	; save unpacked short
continue
	nop			; LA-1
	nop			; LA
	.endl
	rts
	
;---------------------------------------------------------------
; Unpack and decode compressed buffer into sample buffer pointed at by r2.
; Residue buffer passed in r3.
; Destroys r0, r1, r2, r3, r4, r5, r6, n4, n5, n6, x0, x1, y0, y1, a, b.
decompress
	move	r2,x:currentBuf
	move	r3,x:currentResidue
	move	r2,r6		; save pointer to sample buffer
	move	#routines,r4
	move	x:algorithm,n4
	move	#leaderCount,r5
	move    n4,n5
	move	x:(r4+n4),r0	; decompression routine address
	move	x:(r5+n5),x1	; number of leading samples
	move	x:encodeLength,a
	sub	x1,a  #packedBuf,r1
	move	a1,y0		; number of packed shorts (encodeLength-leader)
	move	x1,a
	move	#>1,x0
	sub	x0,a  x:numBits,y1
	move	a1,n6		; number of leading samples - 1
	move	y0,n2		; number of packed shorts
	jsr	(r0)
	rts
	
;---------------------------------------------------------------
; Decompression algorithm routines
; On entry:
;	r1	packed buffer
;	r2	sample buffer
;	n2	number of packed shorts
;	r6	also sample buffer
;	n6	number of leading samples - 1
;	x1	number of leading samples
;	y1	numBits
;	y0	also number of packed shorts
; The general template is:
;	call moveSigned to get unencoded leading samples
;	call unpack to unpack remaining bits to sample buffer
;	if bitFaithful, call getResidue to get eat residue bits
;	loop to decode samples
;---------------------------------------------------------------

;---------------------------------------------------------------
; Null encode - just copy samples
; NOTE: this also implies no unpacking, i.e. 16 bits data.
; In practice this never happens because the minimum number of
; dropped bits is 4, so another alogorithm is bound to win
; on 12 bits.  Null encode is useful only for debugging.
null
	move	x:encodeLength,x1
	moveUnsigned	r1,r2,x1
	rts
	
;---------------------------------------------------------------
; xor encode - exclusive or
; s[i] ^= s[i-1]
xor
	moveUnsigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	.loop	n2
		move	x:(r6)+,a	; s[i-1]
		move	x:(r6),x0	; s[i]
		eor	x0,a
		move	a1,x:(r6)
	.endl
	rts
	
;---------------------------------------------------------------
; d1 encode - first differences
; s[i] = s[i-1] - s[i]
d1
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,a	; s[i-1]
		move	x:(r6),b	; s[i]
		signx	x0,x1,b
		sub	b,a
		move	a1,x:(r6)
	.endl
	rts
	
;---------------------------------------------------------------
; d2 encode - second differences
; s[i] = s[i] + 2*s[i-1] - s[i-2]
d2
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,y1	; s[i-2]
		move	x:(r6)+,b	; s[i-1]
		asl	b
		sub	y1,b  x:(r6),a	; s[i]
		signx	x0,x1,a
		add	a,b
		move	b1,x:(r6)-
	.endl
	rts
	
;---------------------------------------------------------------
; d3 encode - third differences
; s[i] = -s[i] + 3*s[i-1] - 3*s[i-2] + s[i-3]
d3
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,y1	; s[i-3]
		move	x:(r6)+,b	; s[i-2]
		times3	b,a
		neg	b
		add	y1,b  x:(r6)+,a	; s[i-1]
		move	b1,y1
		times3	a,b
		add	y1,a  x:(r6),b	; s[i]
		signx	x0,x1,b
		sub	b,a
		move	a1,x:(r6)-n6
	.endl
	rts
	
;---------------------------------------------------------------
; d4 encode - fourth differences
; s[i] = s[i] + 4*s[i-1] - 6*s[i-2] + 4*s[i-3] - s[i-4]
d4
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,y1	; s[i-4]
		move	x:(r6)+,a	; s[i-3]
		times4	a
		sub	y1,a  x:(r6)+,b	; s[i-2]
		move	a,y1
		times3	b,a
		asl	b
		neg	b
		add	y1,b  x:(r6)+,a	; s[i-1]
		times4	a
		add	b,a  x:(r6),b	; s[i]
		signx	x0,x1,b
		add	a,b
		move	b1,x:(r6)-n6
	.endl
	rts
	
;---------------------------------------------------------------
; d3_11 encode - third differences, alternate coefficients
; s[i] = -s[i] + s[i-1] - s[i-2] + s[i-3]
d3_11
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,a	; s[i-3]
		move	x:(r6)+,y1	; s[i-2]
		sub	y1,a  x:(r6)+,b	; s[i-1]
		add	b,a  x:(r6),b	; s[i]
		signx	x0,x1,b
		sub	b,a
		move	a1,x:(r6)-n6
	.endl
	rts
	
;---------------------------------------------------------------
; d3_22 encode - third differences, alternate coefficients
; s[i] = -s[i] + 2*s[i-1] - 2*s[i-2] + s[i-3]
d3_22
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,a	; s[i-3]
		move	x:(r6)+,b	; s[i-2]
		asl	b
		sub	b,a  x:(r6)+,b	; s[i-1]
		asl	b
		add	b,a  x:(r6),b	; s[i]
		signx	x0,x1,b
		sub	b,a
		move	a1,x:(r6)-n6
	.endl
	rts
	
;---------------------------------------------------------------
; d4_222 encode - fourth differences, alternate coefficients
; s[i] = s[i] + 2*s[i-1] - 2*s[i-2] + 2*s[i-3] - s[i-4]
d4_222
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,a	; s[i-4]
		move	x:(r6)+,b	; s[i-3]
		asl	b
		sub	a,b  x:(r6)+,a	; s[i-2]
		asl	a
		sub	a,b  x:(r6)+,a	; s[i-1]
		asl	a
		add	a,b  x:(r6),a	; s[i]
		signx	x0,x1,a
		add	a,b
		move	b1,x:(r6)-n6
	.endl
	rts
	
;---------------------------------------------------------------
; d4_343 encode - fourth differences, alternate coefficients
; s[i] = s[i] + 3*s[i-1] - 4*s[i-2] + 3*s[i-3] - s[i-4]
d4_343
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,y1	; s[i-4]
		move	x:(r6)+,a	; s[i-3]
		times3	a,b
		sub	y1,a
		move	a,y1  x:(r6)+,b	; s[i-2]
		times4	b
		neg	b
		add	y1,b
		move	b,y1  x:(r6)+,a	; s[i-1]
		times3	a,b
		add	y1,a  x:(r6),b	; s[i]
		signx	x0,x1,b
		add	a,b
		move	b1,x:(r6)-n6
	.endl
	rts
	
;---------------------------------------------------------------
; d4_101 encode - fourth differences, alternate coefficients
; s[i] = s[i] + s[i-1] + s[i-3] - s[i-4]
d4_101
	moveSigned	r1,r2,x1
	jsr	unpack
	btst	#0,x:method
	jscs	getResidue
	move	x:sign,x0
	move	x:csign,x1
	.loop	n2
		move	x:(r6)+,a	; s[i-4]
		move	x:(r6)+,b	; s[i-3]
		sub	a,b  (r6)+	; ignore s[i-2]
	        move	x:(r6)+,a	; s[i-1]
		add	a,b  x:(r6),a	; s[i]
		signx	x0,x1,a
		add	a,b
		move	b1,x:(r6)-n6
	.endl
	rts
	
;---------------------------------------------------------------
; Return output sample count in y0
; Sets finished flag if this is the last output buffer
; Destroys y, a, b
; In pseudo-C:
;	length = encodeLength
;	if curSample + encodeLength >= numSamples
;       	length = numSamples - curSamples
;		finished = TRUE
calcLength
	move	x:encodeLength,y0
	move	#0,y1
	move	l:curSample,a
	add	y,a		; 48 bit curSample + encodeLength
	move	l:numSamples,b
	cmp	b,a
	.if	<ge>
		move	l:curSample,a
		sub	a,b
		move	b0,y0
		bset	#FINISHED,x:flags
	.endi
	rts
	
;---------------------------------------------------------------
; 48 bit curSample += encodelength
; Destroys y and a
incCurSample
	move	x:encodeLength,y0
	move	#0,y1
	move	l:curSample,a
	add	y,a
	move	a,l:curSample
	rts
	
;---------------------------------------------------------------
; Main loop
main
	; Get non-dma parameters
	jsr	readHost
	move	a1,x:headerSize
	jsr	readHost
	move	a1,x:channelCount

	jsr	start
	
	if	LEADPAD
		; Keep driver alive by starting output dma
		clr	a
		jsr	putHost
	endif
	
	jsr	getParams

loop
	; Read and decompress first channel
	jsr	getType
	jsr	readChan
	move	#leftChannel,r2
	move	#leftResidue,r3
	jsr	decompress

	; Read and decompress second channel
	.if	x:channelCount <eq> #>2
		jsr	getType
		jsr	readChan
		move	#rightChannel,r2
		move	#rightResidue,r3
		jsr	decompress
	.endi
	
	jsr	calcLength		; get output sample count
	
	; Uncompand and send samples to output
	.if	x:method <eq> #>BIT_FAITHFUL
		jsr	sendFaithful
	.else
		jsr	sendSamples
	.endi
	
	bclr	#FINISHED,x:flags
	.if	<cc>
		jsr	incCurSample	; curSample += encodeLength
	.else
		jsr	stop
	.endi
	
	jmp	loop
