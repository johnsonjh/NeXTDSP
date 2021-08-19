;; The following interrupt vector installation is done in vectors.asm:

	ORG	P:$0
;;interrupt vector table

	JMP	>DEGMON_RESET		;;hardware reset vector
	JMP	>DEGMON_STACKERROR	;;stack overflow error
	JSR	>DEGMON_TRACER		;;TRACE interrupt handler
	JSR	>DEGMON_TRACER		;;SWI handler

;;the next group of handlers are initialized to do-nothing fast ISRs
	ORG	P:$0008	;;IRQA default handler
	NOP		
	NOP

	ORG	P:$000A ;;IRQB default handler
	NOP		
	NOP

	ORG	P:$000C	;;SSI rcv data default handler
	NOP		
	NOP

	ORG	P:$000E	;;SSI rcv data w/ exception
	NOP		
	NOP

	ORG	P:$0010	;;SSI transmit handler
	NOP		
	NOP

	ORG	P:$0012	;;SSI transmit w/ exception
	NOP
	NOP

	ORG	P:$0014	;;SCI rcv data
	NOP
	NOP

	ORG	P:$0016 ;;SCI rcv w/ exception
	NOP
	NOP

	ORG	P:$0018 ;;SCI transmit handler
	NOP
	NOP

	ORG	P:$001A ;;SCI Idle line
	NOP
	NOP

	ORG	P:$001C	;;SCI timer
	NOP
	NOP

	ORG	P:$001E ;;NMI <NEVER,NEVER,NEVER,ALTER!!>
	JSR	>DEGMON_TRACER	;;this is also used for the FREEZE command.

	ORG	P:$0020	;;Host receive data (try not to use this)
	NOP
	NOP

	ORG	P:$0022	;;Host transmit data (try not to use this)
	NOP
	NOP

;; eight Host Command handlers:

	ORG	P:$0024			;; iv_host_cmd (cf. vectors.asm)
	NOP
	NOP

	ORG	P:$0026			;; iv_xhm 
	NOP
	NOP

	ORG	P:$0028			;; iv_host_w_done
	NOP
	NOP

	ORG	P:$002A			;; iv_kernel_ack
	NOP
	NOP

	ORG	P:$002C			;; first available for user
	NOP
	NOP

	ORG	P:$002E
	NOP
	NOP

	ORG	P:$0030
	NOP
	NOP

	ORG	P:$0032			;; fourth and last for user
	NOP
	NOP


