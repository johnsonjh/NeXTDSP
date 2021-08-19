#
# Makefile for the generic kernel server
#

INCDIR= ../include
KERNSERV= $(INCDIR)/kernserv
MIDI= $(INCDIR)/midi
MIG= mig

MIGFILES=midi_server.defs midi_reply.defs midi_timer.defs \
	midi_timer_reply.defs
LHFILES= $(MIGFILES:.defs=.h)
HFILES=	$(INCDIR)/midi_server.h		$(MIDI)/midi_server.h \
	$(INCDIR)/midi_reply.h \
	$(INCDIR)/midi_timer.h		$(MIDI)/midi_timer.h \
	$(INCDIR)/midi_timer_reply.h

MIGFLAGS= -I$(INCDIR)
SOURCEFILES= Makefile $(MIGFILES)
.SUFFIXES: .defs .h

all:	$(HFILES)

$(HFILES): $(LHFILES)
	@rm -f $@
	cp -p $*.h $@

.defs.h:
	${MIG} ${MIGFLAGS} $<
	sed '/msg_send/s/MSG_OPTION_NONE/SEND_SWITCH/' $*User.c > $*_user.c
	rm -f $*User.c
	mv $*_user.c $*User.c

clean:
	rm -f *User.c *Server.c *.h

install: all

installsrc: $(SRCDIR)
	cp -p $(SOURCEFILES) $(SRCDIR)

$(SRCDIR):
	mkdirs $(SRCDIR)


