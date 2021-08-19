# Generic makefile for C programs
# To use this makefile, just create a makefile containing
#
# NAME = <progname>
# include <this makefile>
#
MFILES = $(OTHER_MFILES)
CFILES = $(NAME).c $(OTHER_CFILES)
LOADLIBES = $(OTHER_LIBS) -ldsp_s -lsys_s
DEBUG_LOADLIBES = $(OTHER_LIBS) -ldsp_g -lsys_s
PROFILE_LOADLIBES = $(OTHER_LIBS) -ldsp_p -lsys_s
local_install: c_local_install
install: c_install
include ../Makefile.cm


