# Pass the make target to each subdirectory Makefile

PROJ_DIRS = dspasm dsplib dsplnk

all install clean:
	@for i in $(PROJ_DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $@) || exit $? ; done

installsrc: SRCROOT
	-/bin/rm -rf $(SRCROOT)
	mkdirs $(SRCROOT)
	chmod 755 $(SRCROOT)
	tar cf - Makefile | (cd $(SRCROOT); tar xf -)
	chmod 444 $(SRCROOT)/*
	@for i in $(PROJ_DIRS); do \
		(cd ./$$i; echo "$$i:"; make $@) || exit $? ; done

SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi
		
