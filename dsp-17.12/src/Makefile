# Makefile for DSP C software
#
# DEPENDENCIES
#
# lib   requires ../smsrc to have been made
# bin   requires lib to have been made
#
NAME = src
include ../Makefile.config

MAKE_FILES = Makefile

DIRS = lib bin

all install local_install clean lean:
	@for i in $(DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done

INSTALLSRC_SRCS = $(MAKE_FILES)

installsrc: $(SOURCE_DIR)
	tar cf - $(INSTALLSRC_SRCS) | (cd $(SOURCE_DIR); tar xfp -)
	(cd $(SOURCE_DIR); chmod 644 $(INSTALLSRC_SRCS))
	@for i in $(DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done

