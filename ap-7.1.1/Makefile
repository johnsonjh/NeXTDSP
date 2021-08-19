#
#	Makefile - used to develop and maintain ap
#	Copyright NeXT, Inc.  1989
#
# Modification History:
#	07/31/89/mtm - clean ups

SOURCE_DIR =
DERIVED_DIRS = apcsrc apbin apman src/lib/dsp
LEAN_GARBAGE = $(DERIVED_DIRS)
MAKE_FILES = Makefile Makefile.common Makefile.macros
INSTALLSRC_FILES = $(MAKE_FILES) aptest aptest_aux README
NESTED_DIRS = apsrc src

# Include the common makefile
include Makefile.common

# Make test scripts executable after installsrc
installsrc::
	(cd $(SRCROOT); chmod 755 aptest aptest_aux)

all local_install install lean clean installsrc::
	$(MAKE) nested "NESTED_TARGET = $@"

