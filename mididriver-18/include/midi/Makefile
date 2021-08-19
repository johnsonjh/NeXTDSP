DSTDIR=/usr/include/midi
INSTFLAGS=-m 444
HFILES= midi_types.h midi_error.h midi_timer_error.h \
	midi_reply_handler.h midi_timer_reply_handler.h

MIG_GENERATED= midi_server.h midi_timer.h
SOURCEFILES= Makefile $(HFILES)

all:

clean:
	-rm -f $(MIG_GENERATED)

install: $(DSTROOT)$(DSTDIR)
	for i in $(HFILES) $(MIG_GENERATED); \
	do \
		install ${INSTFLAGS} $$i ${DSTROOT}${DSTDIR}; \
	done

installsrc: $(SRCDIR)
	cp -p $(SOURCEFILES) $(SRCDIR)

$(SRCDIR) $(DSTROOT)$(DSTDIR):
	mkdirs $@
