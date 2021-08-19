#
#	Makefile - used to develop and maintain apsrc
#	Copyright NeXT, Inc.  1989
#
# Modification History:
#	07/31/89/mtm - revised
#	08/24/89/mtm - removed ./ from macro and added to cd cmd lines
#	04/18/90/mtm - put the ./'s back in! (JOS environment differs somehow...)
#
SOURCE_DIR = apsrc
INSTALLSRC_FILES = $(ASM_FILES) Makefile arrayproc.h.header DSPAPUtilities.h \
	README

# Directories
INSTALL_MAN_DIR = $(DSTROOT)/usr/lib/dsp/apman
INSTALL_BIN_DIR = $(DSTROOT)/usr/lib/dsp/apbin
# Need this for install because AP_BIN_FILES get VPATH appended
INSTALL_BIN_PATH = $(DSTROOT)/usr/lib/dsp
INSTALL_SRC_DIR = $(DSTROOT)/usr/lib/dsp/apsrc
INSTALL_INCLUDE_DIR = $(DSTROOT)/usr/include/dsp
LOCAL_BIN_DIR = apbin
LOCAL_MAN_DIR = apman
LOCAL_CSRC_DIR = apcsrc
LOCAL_INCLUDE_DIR = src/lib/dsp
LOCAL_INCLUDE_PATH = src
LOCAL_INSTALL_BIN_DIR = ./../$(LOCAL_BIN_DIR)
LOCAL_INSTALL_MAN_DIR = ./../$(LOCAL_MAN_DIR)
LOCAL_INSTALL_CSRC_DIR = ./../$(LOCAL_CSRC_DIR)
LOCAL_INSTALL_INCLUDE_DIR = ./../$(LOCAL_INCLUDE_DIR)

DERIVED_DIRS = $(LOCAL_MAN_DIR) $(LOCAL_BIN_DIR) $(LOCAL_CSRC_DIR) \
	       $(LOCAL_INCLUDE_PATH)
LOCAL_INSTALL_DIRS = $(LOCAL_INSTALL_BIN_DIR) $(LOCAL_INSTALL_MAN_DIR) \
		     $(LOCAL_INSTALL_CSRC_DIR) $(LOCAL_INSTALL_INCLUDE_DIR)
INSTALL_DIRS = $(INSTALL_MAN_DIR) $(INSTALL_BIN_DIR) $(INSTALL_SRC_DIR)
PRECIOUS_INSTALL_DIRS = $(INSTALL_INCLUDE_DIR)

# Places to look for dependents
VPATH = $(LOCAL_BIN_DIR):$(LOCAL_INCLUDE_DIR)

# Lean doesn't remove .lnk so assembly won't be necessary
LEAN_GARBAGE = $(DERIVED_DIRS) *_*.lst *_*.asm $(AP_MAN_FILES)
CLEAN_GARBAGE = *_*.lnk

# Array processing DSP fast-load format files
AP_BIN_FILES = $(ASM_FILES:.asm=.dsp)

# Array processing man pages
# Only summary files are installed
AP_SUMMARY_FILES = AP_MACROS_SUMMARY AP_CALL_SUMMARY AP_FUNCTION_DOC
AP_MAN_FILES = $(ASM_FILES:.asm=.l) $(AP_SUMMARY_FILES)

# Suffixes to support dspwrap
.SUFFIXES: .dsp .asm

# dspwrap location and arguments
DSPWRAP_DIR = /usr/bin
DSPWRAP_FLAGS =
LOCAL_DSPWRAP_DIR = ../../../dspwrap_proj/dspwrap/
LOCAL_DSPWRAP_FLAGS = -dsploadwrapDir $(LOCAL_DSPWRAP_DIR) \
		      -l ../../../music_proj/music/dsp/smsrc

# Include the ap macro names
MACRO_PREFIX = 
include ../Makefile.macros

# Files
ASM_FILES = $(AP_MACROS:=.asm)
AP_CSRC_FILES = $(ASM_FILES:.asm=.c)
AP_INCLUDE_FILES = $(ASM_FILES:.asm=.h)
AP_HEADER_FILE = arrayproc.h

# Include the common makefile
include ../Makefile.common

# Rule to generate .dsp from .asm
# Also generates .lnk, .c, and .h
.asm.dsp:
	$(DSPWRAP_DIR)/dspwrap -ap $(DSPWRAP_FLAGS) -nodoc $*.asm
	/bin/mv $*.dsp $(LOCAL_BIN_DIR)
	/bin/mv DSPAP$*.c $(LOCAL_CSRC_DIR)
	/bin/mv DSPAP$*.h $(LOCAL_INCLUDE_DIR)
	
# Targets
all:: make_local_dirs $(LOCAL_MAN_DIR) $(AP_BIN_FILES) header_file

# Use clone version of dspwrap and dsploadwrap
local_dspwrap:
	$(MAKE) "DSPWRAP_DIR = $(LOCAL_DSPWRAP_DIR)" \
		"DSPWRAP_FLAGS = $(LOCAL_DSPWRAP_FLAGS)"

# These dirs can not be targets because local_install has them as dependents
make_local_dirs:
	mkdirs -m 755 $(LOCAL_BIN_DIR) $(LOCAL_CSRC_DIR) $(LOCAL_INCLUDE_DIR)

# Invoke dspwrap with all asm file names so summary files work right
$(LOCAL_MAN_DIR): $(ASM_FILES)
	mkdirs -m 755 $(LOCAL_MAN_DIR)
	$(DSPWRAP_DIR)/dspwrap -ap $(DSPWRAP_FLAGS) -nolink $(ASM_FILES)
	/bin/mv $(AP_MAN_FILES) $@

# This target is needed so MACRO_PREFIX is set right for AP_INCLUDE_FILES
header_file:
	$(MAKE) $(LOCAL_INCLUDE_DIR)/$(AP_HEADER_FILE) "MACRO_PREFIX = DSPAP"

$(LOCAL_INCLUDE_DIR)/$(AP_HEADER_FILE): $(AP_INCLUDE_FILES)
	/bin/cat arrayproc.h.header $(AP_INCLUDE_FILES) \
		>$(LOCAL_INCLUDE_DIR)/$(AP_HEADER_FILE)
	cp -p DSPAPUtilities.h $(LOCAL_INCLUDE_DIR)

# Install bin files, man files, and h file
# Force the install dirs to be removed and made
# Do local_install first so ap/apcsrc is up to date
install: local_install
	/bin/rm -rf $(INSTALL_DIRS)
	mkdirs -m 755 $(INSTALL_DIRS) $(PRECIOUS_INSTALL_DIRS)
	(cd ./$(LOCAL_MAN_DIR); \
		install $(IFLAGS) -m 644 $(AP_SUMMARY_FILES) $(INSTALL_MAN_DIR))
	install $(IFLAGS) -m 444 $(AP_BIN_FILES) $(INSTALL_BIN_PATH)
	install $(IFLAGS) -m 644 $(ASM_FILES) $(INSTALL_SRC_DIR)
	(cd ./$(LOCAL_INCLUDE_DIR); \
		install $(IFLAGS) -m 444 \
			$(AP_HEADER_FILE) DSPAPUtilities.h $(INSTALL_INCLUDE_DIR))

# Install local bin files, man files, c files, and h file
# (only if local install dirs are out of date)
local_install: all $(LOCAL_INSTALL_DIRS)

# Note: install destination automatically gets VPATH appended
# Need to remove first because install does not update dir date
$(LOCAL_INSTALL_BIN_DIR): $(LOCAL_BIN_DIR)
	/bin/rm -rf $@
	mkdirs -m 755 $@
	install $(IFLAGS) -m 644 $(AP_BIN_FILES) ../

$(LOCAL_INSTALL_MAN_DIR): $(ASM_FILES)
	/bin/rm -rf $@
	mkdirs -m 755 $@
	(cd ./$(LOCAL_MAN_DIR); \
		install $(IFLAGS) -m 644 $(AP_MAN_FILES) ../$@)

$(LOCAL_INSTALL_CSRC_DIR): $(LOCAL_CSRC_DIR)
	/bin/rm -rf $@
	mkdirs -m 755 $@
	$(MAKE) local_install_csrc "MACRO_PREFIX = DSPAP"

# This target is needed so MACRO_PREFIX is set right for AP_CSRC_FILES
local_install_csrc:
	(cd ./$(LOCAL_CSRC_DIR); \
		install $(IFLAGS) -m 644 $(AP_CSRC_FILES) \
			../$(LOCAL_INSTALL_CSRC_DIR))

$(LOCAL_INSTALL_INCLUDE_DIR): $(LOCAL_INCLUDE_DIR)
	/bin/rm -rf $@
	mkdirs -m 755 $@
	(cd ./$(LOCAL_INCLUDE_DIR); \
		install $(IFLAGS) -m 644 $(AP_HEADER_FILE) DSPAPUtilities.h ../../../$@)


