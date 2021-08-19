RELNOTESDIR=/usr/local/RelNotes/
RELNOTES=MidiNotes.rtf
USER_BOMDIR=/usr/local/BOMs/
USER_BOM=mididriver.user_bom
SUBDIR=	mig libmidi midi midi_test include
SOURCES= Makefile  ${USER_BOM} ${RELNOTES}

all tags clean:
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@) || exit $?; \
	done

installhdrs: DSTROOT ${DSTROOT}
	echo ================= make $@  =================
	(cd mig; ${MAKE} all)
	(cd include; ${MAKE} DSTROOT=$$DSTROOT install)

install: DSTROOT ${DSTROOT} ${DSTROOT}${RELNOTESDIR} ${DSTROOT}${USER_BOMDIR}
	install -c -m 444 ${RELNOTES} ${DSTROOT}${RELNOTESDIR}${RELNOTES}
	install -c -m 444 ${USER_BOM} ${DSTROOT}${USER_BOMDIR}${USER_BOM}
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@) || exit $?; \
	done

installsrc: SRCROOT ${SRCROOT}
	-rm -rf ${SRCROOT}/*
	cp -p ${SOURCES} ${SRCROOT}
	@CWD=`pwd`; cd ${SRCROOT}; SRCROOT=`pwd`; cd $$CWD; \
	for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} SRCDIR=$$SRCROOT/$$i $@) || exit $?; \
	done

SRCROOT DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(SRCROOT) $(DSTROOT) ${DSTROOT}${RELNOTESDIR} ${DSTROOT}${USER_BOMDIR}:
	mkdirs $@
