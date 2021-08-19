SUBDIR= midi
HFILES=
DSTDIR=/usr/include
MIG_GENERATED=midi_reply.h midi_timer_reply.h midi_server.h midi_timer.h
SOURCEFILES=Makefile $(HFILES)
all:

clean:
	rm -f $(MIG_GENERATED)
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@) || exit $?; \
	done

installhdrs install:  $(DSTROOT)$(DSTDIR)
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@) || exit $?; \
	done

installsrc:  $(SRCDIR)
	tar cf - $(SOURCEFILES) | (cd $(SRCDIR); tar xpf -)
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} SRCDIR=$(SRCDIR)/$$i $@) || exit $?; \
	done

$(SRCDIR) $(DSTROOT)$(DSTDIR):
	mkdirs $@
