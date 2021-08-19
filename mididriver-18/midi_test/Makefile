
CFILES= timer_track.c timer_start_stop.c \
	midi_read.c midi_write.c midi_thru.c
OFILES= $(CFILES:.c=.o)
MIDILIB=../libmidi/libmidi_g.a
DEBUG=-DDEBUG
SOURCEFILES= Makefile $(CFILES) $(HFILES)

CFLAGS= -g $(DEBUG) -MD -Wimplicit -I../include -DMACH -DNeXT_MOD

all:	timer_track timer_start_stop midi_read midi_write midi_thru

install:

installsrc: $(SRCDIR)
	tar cf - $(SOURCEFILES) | (cd $(SRCDIR); tar xpf -)

clean:
	-rm -f $(OFILES) timer_track $(MIGCFILES) Makedep

$(SRCDIR):
	mkdirs $@

timer_track: timer_track.o  ${MIDILIB}
	${CC} ${CFLAGS} -o $@ timer_track.o ${MIDILIB}

timer_start_stop: timer_start_stop.o  ${MIDILIB}
	${CC} ${CFLAGS} -o $@ timer_start_stop.o ${MIDILIB}

midi_read: midi_read.o  ${MIDILIB}
	${CC} ${CFLAGS} -o $@ midi_read.o ${MIDILIB}

midi_write: midi_write.o  ${MIDILIB}
	${CC} ${CFLAGS} -o $@ midi_write.o ${MIDILIB}

midi_thru: midi_thru.o  ${MIDILIB}
	${CC} ${CFLAGS} -o $@ midi_thru.o ${MIDILIB}

$(OFILES): $$(@:.o=.c)
	$(CC) $(CFLAGS) -c $*.c
	md -u Makedep -d $*.d

-include Makedep



