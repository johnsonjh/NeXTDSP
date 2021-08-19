#
#	Makefile - used to develop and maintain dspwrap
#	Copyright NeXT, Inc.  1989
#
# Modification History:
#	08/14/89/mtm - Revised, added header
#		       Install bins and dirs as 755.
#
NAME= dspwrap
BINDIR= $(DSTROOT)/usr/bin
DOCDIR= $(DSTROOT)/usr/local/doc/dspwrap_proj
MANDIR= $(DOCDIR)/man
PRODUCTS= dspwrap dsploadwrap
CFILES= dspwrap.c dsploadwrap.c DSPWriteC.c writeUG.c
OFILES = $(CFILES:.c=.o)
MANPAGES = dspwrap.1 dsploadwrap.1
SOURCE_FILES = $(CFILES) $(MANPAGES)
INSTALL_FILES = $(SOURCE_FILES) Makefile README

# Install flags
IFLAGS = -q -c
BINIFLAGS = -s -c

# Compiler flags
BASE_CFLAGS = -Wimplicit -g
CFLAGS = $(BASE_CFLAGS) -O
LIBS= -ldsp_s -lsys_s
DEBUG_LIBS = -ldsp_g -lsys_s
PROFILE_LIBS = -ldsp_p -lsys_s
DSP_LIB_DIR = /Net/marvin/ds/dspdev/dsp_proj/xdsp/src/lib
DEBUG_CFLAGS = $(BASE_CFLAGS) -g -L$(DSP_LIB_DIR) -I$(DSP_LIB_DIR)
PROFILE_CFLAGS = $(BASE_CFLAGS) -pg -g -L$(DSP_LIB_DIR) -I$(DSP_LIB_DIR)

all: $(PRODUCTS) 

lean:
	/bin/rm -f $(OFILES) vers.o vers.c
	
clean: lean
	/bin/rm -f $(PRODUCTS) TAGS core

# Local install makes a debug version
local_install debug:
	$(MAKE) "CFLAGS = $(DEBUG_CFLAGS)" "LIBS = $(DEBUG_LIBS)"

profile:
	$(MAKE) "CFLAGS = $(PROFILE_CFLAGS)" "LIBS = $(PROFILE_LIBS)"

installsrc: SRCROOT
	/bin/rm -rf $(SRCROOT)
	mkdirs -m 755 $(SRCROOT)
	tar cf - $(INSTALL_FILES) | (cd $(SRCROOT); tar xfp -)
	(cd $(SRCROOT); chmod 644 $(INSTALL_FILES))

bin_install: all
	mkdirs -m 755 $(BINDIR)
	install $(BINIFLAGS) -m 755 $(PRODUCTS) $(BINDIR)

doc_install:
	/bin/rm -rf $(DOCDIR)
	mkdirs -m 755 $(MANDIR)
	install $(IFLAGS) -m 644 $(MANPAGES) $(MANDIR)

install: bin_install doc_install

dspwrap: dspwrap.c vers.o
	$(CC) $(CFLAGS) -o dspwrap dspwrap.c vers.o $(LIBS)

dsploadwrap: dsploadwrap.c DSPWriteC.o vers.o writeUG.o
	$(CC) $(CFLAGS) -o dsploadwrap dsploadwrap.c \
		DSPWriteC.o writeUG.o vers.o $(LIBS)

vers.c:
	@/bin/rm -f $@
	-vers_string -c $(NAME) > $@

SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi

TAGS:
	etags $(CFILES)

