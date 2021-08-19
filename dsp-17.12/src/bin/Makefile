# Makefile for dsp/src/bin software.
#
# 02/25/89/jos - DSPMusic taken out of release
# 02/26/89/jos - dspboot, dspping, and dsptestap taken out of release
# 03/18/89/jos - Changed .. to ../.. since bin moved to src/bin
# 03/25/89/jos - man pages now mandatory
# 03/25/89/jos - dsptmq, dspmsg, dspload dsptestap out.
# 04/17/89/jos - dspasm removed since ~dspdev/dspasm_proj now exists
#		 lnk56000 bug is now a problem (unless installsrc is left
#		 hacked such that products are made before installsrc)
# 05/12/89/jos - Removed dspwrap dsploadwrap
# 05/15/89/jos - dspasm added back in, but it only installs dspasm{a,r}l
# 06/27/89/jos - dspasm, dspimg, and dspmsg moved to new "dsp/tools" directory
#

NAME = bin
include ../../Makefile.config

MAKE_FILES = Makefile Makefile.c Makefile.m Makefile.cm

# utilities to install in final release
PROG_DIRS = dsptest dspbeep dspabort

# utilities only necessary for the build
BUILD_PROG_DIRS = $(PROG_DIRS)

all install:
	@for i in $(PROG_DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done

clean lean:
	@for i in $(BUILD_PROG_DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done

local_install: ../../src/lib/libdsp_g.a
	@for i in $(BUILD_PROG_DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) local_install) || exit $? ; done

installsrc: $(BIN_SOURCE_DIR)
	tar cf - $(MAKE_FILES) | (cd $(BIN_SOURCE_DIR); tar xfp -)
	(cd $(BIN_SOURCE_DIR); chmod 644 $(MAKE_FILES))
	@for i in $(BUILD_PROG_DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) installsrc) || exit $? ; done

../../src/lib/libdsp_g.a:
	(cd ../../src/lib ; echo "../../src/lib:"; make $(MAKE_FLAGS) debug)


