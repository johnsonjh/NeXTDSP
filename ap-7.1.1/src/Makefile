#
#	Makefile - used to develop and maintain ap/src
#	Copyright NeXT, Inc.  1989
#
# Modification History:
#	08/01/89/mtm - initial coding
#	04/19/90/mtm - disable make of bin

SOURCE_DIR = src
INSTALLSRC_FILES = Makefile
#NESTED_DIRS = lib bin
NESTED_DIRS = lib

# Include the common makefile
include ../Makefile.common

all local_install install lean clean installsrc::
	$(MAKE) nested "NESTED_TARGET = $@"


