/* 
 * Mach Operating System
 * Copyright (c) 1988 NeXT, Inc.
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */
/*
 * HISTORY
 * 10-Dec-88  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#ifndef	_MIDI_TYPES_
#define _MIDI_TYPES_
#include <kern/mach_interface.h>
#include <sys/message.h>
#include <sys/time.h>

/*
 * Interface message formats
 */

/*
 * Messages sent from/to the kernel
 */

/*
 * The midi_data message format is a basic message containing MIDI data.
 * it contains either cooked, raw, or packed data.  Both cooked and
 * raw data have a timestamp associated with each element,
 * packed data has a single timestamp for the entire block of data.
 *
 * A message contains an array of message elements of one of the following
 * types:
 *	RAW	- 4 bytes containing 1 byte of data and 3 bytes of timestamp
 *		  information.
 *	COOKED	- 8 bytes containing 4 bytes of timestamp information,
 *		  3 bytes of data, and 1 byte of data count.
 *	PACKED	- 1 byte of data.
 *
 * Timestamps are specified in units of quanta, which are either an absolute
 * time relative to a timestamp associated with the MIDI input or output
 * device, or a time relative to the previous event.  Quanta are, themselves,
 * associated with the MIDI input or output device being either a specified
 * number of microseconds, or relative to an external time source (eg, MIDI
 * Time Code).
 */
typedef struct midi_raw {
	unsigned int	quanta:24,	// when byte's due
			data:8;		// data byte
} midi_raw_data_t;
typedef midi_raw_data_t *midi_raw_t;

/*
 * Maximum number of MIDI messages that can be sent
 * in a call to midi_send_raw_data().
 */
#define MIDI_RAW_DATA_MAX	1000

typedef struct midi_cooked {
	// 1 byte message has data in data[0]
	// 2 byte message has data in data[0] and data[1]
	// 3 byte message has data in data[0], data[1], and data[2]
	unsigned int	quanta;		// when message's due
	unsigned char	ndata;		// nbytes this msg
	unsigned char	data[3];	// data
} midi_cooked_data_t;
typedef midi_cooked_data_t *midi_cooked_t;

/*
 * Maximum number of MIDI messages that can be sent
 * in a call to midi_send_cooked_data().
 */
#define MIDI_COOKED_DATA_MAX	500

typedef unsigned char midi_packed_data_t;
typedef midi_packed_data_t *midi_packed_t;

/*
 * Maximum number of MIDI messages that can be sent
 * in a call to midi_send_packed_data().
 */
#define MIDI_PACKED_DATA_MAX	4000

typedef union midi_data {
	midi_raw_data_t		*raw;
	midi_cooked_data_t	*cooked;
	midi_packed_data_t	*packed;
} midi_data_t;

typedef struct timeval timeval_t;

#ifndef	KERNEL
typedef port_t midi_dev_t;
typedef port_t midi_rx_t;
typedef port_t midi_time_t;
#endif	KERNEL

/*
 * Midi Types
 */
#define MIDI_TYPE_MASK		0xe0000000
#define MIDI_TYPE_RAW		0x20000000
#define MIDI_TYPE_COOKED	0x40000000
#define MIDI_TYPE_PACKED	0x80000000
#define MIDI_TIMESTAMP_MASK	0x1fffffff

/*
 * Midi error returns.
 */
#define MIDI_BAD_PARM	100	// bad parameter list in message
#define MIDI_PORT_BUSY	101	// access req'd to existing excl access port
#define MIDI_NOT_OWNER	102	// must be owner to do this
#define MIDI_NOTALIGNED	103	// bad data alignment.
#define MIDI_NO_OWNER	104	// must have owner to do operation
#define MIDI_MODE_WRONG	105	// mode wrong for performing operation
#define MIDI_BAD_PORT	106	// message sent on wrong port
#define MIDI_WILL_BLOCK	107	// next operation will block (queue full)
#define MIDI_UNSUP	107	// operation not supported

/*
 * Timer error returns.
 */
#define TIMER_BAD_PARM		100
#define TIMER_BAD_CNTRL		101
#define TIMER_MODE_WRONG	102
#define TIMER_UNSUP		103

/*
 * MIDI protocol defines.
 */
#define	MIDI_PROTO_RAW		0x0	// raw data input
#define MIDI_PROTO_COOKED	0x1	// cooked data input
#define MIDI_PROTO_PACKED	0x2	// packed data input

#define MIDI_PROTO_SYNC_SYS	0x0	// clock uses system clock as source
#define MIDI_PROTO_SYNC_CLOCK	0x1	// clock uses MIDI Clocks (receive)
					// generate MIDI Clocks (transmit)
#define MIDI_PROTO_SYNC_MTC	0x2	// clock uses MIDI Time Code (receive)
					// generate MIDI Time Code (transmit)

/*
 * Defines for system ignores.
 */
#define MIDI_IGNORE_EXCLUSIVE	0x0001
#define MIDI_IGNORE_MTC		0x0002
#define MIDI_IGNORE_SONG_POS_P	0x0004
#define MIDI_IGNORE_SONG_SELECT	0x0008
#define MIDI_IGNORE_TUNE_REQ	0x0040
#define MIDI_IGNORE_EOX		0x0080
#define MIDI_IGNORE_TIMING_CLCK	0x0100
#define MIDI_IGNORE_START	0x0400
#define MIDI_IGNORE_CONTINUE	0x0800
#define MIDI_IGNORE_STOP	0x1000
#define MIDI_IGNORE_ACTIVE_SENS	0x4000
#define MIDI_IGNORE_SYSTEM_RST	0x8000

#define MIDI_IGNORE_BIT(byte) (1<<((byte)&0xf))
/*
 * Identifying MIDI message types.
 */
#define MIDI_TYPE_STATUS(byte)		((byte)&0x80)

#define MIDI_TYPE_SYSTEM(byte)		(((byte)&0xf0) == 0xf0)
#define MIDI_TYPE_SYSTEM_COMMON(byte)	(((byte)&0xf8) == 0xf0)
#define MIDI_TYPE_SYSTEM_REALTIME(byte)	(((byte)&0xf8) == 0xf8)

#define MIDI_TYPE_1BYTE(byte)	(   MIDI_TYPE_SYSTEM_REALTIME(byte) \
				 || (byte) == 0xf6 || (byte) == 0xf7)
#define MIDI_TYPE_2BYTE(byte)	(   (((byte)&0xe0) == 0xc0) \
				 || (((byte)&0xe0) == 0xd0) \
				 || ((byte)&0xfd) == 0xf1)
#define MIDI_TYPE_3BYTE(byte)	(   ((byte)&0xc0) == 0x80 \
				 || ((byte)&0xe0) == 0xe0 \
				 || (byte) == 0xf2)

#define MIDI_TYPE_SYSTEM_EXCL(byte)	((byte) == 0xf0)
#define MIDI_TYPE_SYSTEM_EOX(byte)	((byte) == 0xf7)

/*
 * MIDI status bytes (from International MIDI Association document
 * MIDI-1.0, August 5, 1983)
 */
#define MIDI_RESETCONTROLLERS	0x79
#define MIDI_LOCALCONTROL	0x7a
#define MIDI_ALLNOTESOFF	0x7b
#define MIDI_OMNIOFF		0x7c
#define MIDI_OMNION		0x7d
#define MIDI_MONO		0x7e
#define MIDI_POLY		0x7f
#define	MIDI_NOTEOFF		0x80
#define	MIDI_NOTEON		0x90
#define	MIDI_POLYPRES		0xa0
#define	MIDI_CONTROL		0xb0
#define	MIDI_PROGRAM		0xc0
#define	MIDI_CHANPRES		0xd0
#define	MIDI_PITCH		0xe0
#define	MIDI_CHANMODE		MIDI_CONTROL
#define	MIDI_SYSTEM		0xf0
#define	MIDI_SYSEXCL		(MIDI_SYSTEM | 0x0)
#define MIDI_TIMECODEQUARTER	(MIDI_SYSTEM | 0x1)
#define	MIDI_SONGPOS		(MIDI_SYSTEM | 0x2)
#define	MIDI_SONGSEL		(MIDI_SYSTEM | 0x3)
#define	MIDI_TUNEREQ		(MIDI_SYSTEM | 0x6)
#define	MIDI_EOX		(MIDI_SYSTEM | 0x7)
#define MIDI_CLOCK		(MIDI_SYSTEM | 0x8)
#define MIDI_START		(MIDI_SYSTEM | 0xa)
#define MIDI_CONTINUE		(MIDI_SYSTEM | 0xb)
#define MIDI_STOP		(MIDI_SYSTEM | 0xc)
#define MIDI_ACTIVE		(MIDI_SYSTEM | 0xe)
#define MIDI_RESET		(MIDI_SYSTEM | 0xf)

#define MIDI_MAXDATA            0x7f
#define MIDI_OP(y)              (y & (MIDI_STATUSMASK))
#define MIDI_DATA(y)            (y & (MIDI_MAXDATA))
#define MIDI_MAXCHAN            0x0f
#define MIDI_NUMCHANS             16
#define MIDI_NUMKEYS             128
#define MIDI_ZEROBEND         0x2000
#define MIDI_DEFAULTVELOCITY     64

/*
 * Masks for disassembling MIDI status bytes
 */
#define	MIDI_STATUSBIT	0x80	/* indicates this is a status byte */
#define	MIDI_STATUSMASK	0xf0	/* bits indicating type of status req */
#define	MIDI_SYSRTBIT	0x08	/* differentiates SYSRT from SYSCOM */

/* MIDI Controller numbers */

#define MIDI_MODWHEEL           1
#define MIDI_BREATH             2
#define MIDI_FOOT               4
#define MIDI_PORTAMENTOTIME     5
#define MIDI_DATAENTRY          6
#define MIDI_MAINVOLUME         7
#define MIDI_BALANCE            8
#define MIDI_PAN                10
#define MIDI_EXPRESSION         11
#define EFFECTCONTROL1		12
#define EFFECTCONTROL2		13			

/* LSB for above */
#define MIDI_MODWHEELLSB        (1 + 31)  
#define MIDI_BREATHLSB          (2 + 31)
#define MIDI_FOOTLSB            (4 + 31)
#define MIDI_PORTAMENTOTIMELSB  (5 + 31)
#define MIDI_DATAENTRYLSB       (6 + 31)
#define MIDI_MAINVOLUMELSB      (7 + 31)
#define MIDI_BALANCELSB         (8 + 31)
#define MIDI_PANLSB             (10 + 31)
#define MIDI_EXPRESSIONLSB      (11 + 31)

#define MIDI_DAMPER             64
#define MIDI_PORTAMENTO         65
#define MIDI_SOSTENUTO          66
#define MIDI_SOFTPEDAL          67
#define MIDI_HOLD2              69
/*
 * Controller 91-95 definitions from original 1.0 MIDI spec
 */
#define MIDI_EXTERNALEFFECTSDEPTH 91
#define MIDI_TREMELODEPTH       92
#define MIDI_CHORUSDEPTH        93
#define MIDI_DETUNEDEPTH        94
#define MIDI_PHASERDEPTH        95
/*
 * Controller 91-95 definitions as of June 1990
 */
#define EFFECTS1		91
#define EFFECTS2		92
#define EFFECTS3		93
#define EFFECTS4		94
#define EFFECTS5		95
#define MIDI_DATAINCREMENT      96
#define MIDI_DATADECREMENT      97

/*
 * Timer control parameters.
 */
#define TIMER_CONTROL_PAUSE	0x1
#define TIMER_CONTROL_RESUME	0x2
#define TIMER_CONTROL_FORWARD	0x4
#define TIMER_CONTROL_REVERSE	0x8
#endif	_MIDI_TYPES_


