# Generic makefile for Objective C programs
# To use this makefile, just create a makefile containing
#
# NAME = <progname>
# include <this makefile>
#
MFILES = $(NAME).m $(OTHER_MFILES)
CFILES = $(OTHER_CFILES)
LOADLIBES = $(OTHER_LIBS) -lNeXT_s -ldsp_s -lsys_s
DEBUG_LOADLIBES = $(OTHER_LIBS) -lNeXT_s -ldsp_g -lsys_s
PROFILE_LOADLIBES = $(OTHER_LIBS) -lNeXT_s -ldsp_p -lsys_s
local_install: m_local_install
install: m_install
include ../Makefile.cm

# The following is overkill. If we use the -MD compile option, it's not needed.
# $(MFILES:.m=.o): $(HFILES)

