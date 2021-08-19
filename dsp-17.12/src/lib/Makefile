#  NeXT DSP Library Makefile... includes Makefile.lib and Makefile.common
#
# 02/14/89/jos - removed -bsd flag
# 03/18/89/jos - Changed .. to ../.. since lib moved to src/lib
# 05/01/89/jos - removed _DSPAPUtilities.c (to arrayproc project temporarily)
# 05/12/89/jos - removed _DSPWriteUG.c
# 06/27/89/jos - moved _DSPMakeIncludeFiles.c to dspmsg
# 02/17/90/jos - added sounddriver.c temporarily to list of c files

#  **1**  -LIBRARY NAME-
#  The base name of the library goes here. This should also be the same
#  name as the SGS project the library is built under.  For example, if
#  you enter "appkit" as the name, then the name of the SGS project
#  should be "appkit", and the library created will be "libappkit.a".
NAME = dsp

#  **2**  -SOURCE FILES-
#  Here list the .m, .c, .psw, .pswm, .s, .l, .lm, .y, .ym, .ps and
#  .spec files.  You may delete any lines that you don't use.
#  OTHER_SRCS is used for files with other suffixes that you wish
#  to be treated like source (i.e., printed with the source, copied
#  with the source, etc.).  For files in OTHER_SRCS, you need to
#  specify the corresponding .o file in OTHER_SRCS_OFILES.
CFILES = \
	DSPLoad.c \
	DSPControl.c \
	DSPBoot.c \
	DSPReadFile.c \
	DSPTransfer.c \
	DSPConversion.c \
	_DSPRelocate.c\
	DSPStructMisc.c \
	_DSPUtilities.c \
	DSPObject.c \
	_DSPMachSupport.c \
	DSPError.c \
	DSPGlobals.c \
	_DSPCV.c \
	_DSPString.c \
	foodriver.c \
	pointers.c

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex, yacc and speculate.  You may
#  delete lines that you don't use.  All CFLAGS also get passed to
#  Objective-C.
#  DEBUGCFLAGS is substituted for CFLAGS when "debug" is made.
#  PROFCFLAGS is substituted for CFLAGS when "profile" is made.
#  Under CFLAGS, for each library that you use that supplies class
#  and message group files, you must use a -I$(OBJDIR)/XXX, where
#  XXX is the base name of the library.

#
# *** FIXME: DSPTransfer bombs out in cc1 when both -g and -O are present!
#
BASE_CFLAGS = -g -fwritable-strings -Wimplicit
CFLAGS = -O -I. $(BASE_CFLAGS)
SHLIBCFLAGS = $(CFLAGS)
DEBUGCFLAGS = -I. $(BASE_CFLAGS)
PROFCFLAGS = -pg $(BASE_CFLAGS)
LFLAGS = -G

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = dsp/_dsp.h dsp/_dsputilities.h dsp/_DSPMachSupport.h \
		   dsp/_libdsp.h dsp/_DSPMessage.h dsp/_DSPTransfer.h \
		   dsp/_DSPObject.h dsp/obsolete.h \
		   foodriver.h foodriver_client.h snddriver.h
PUBLIC_INCFILES =  dsp/dsp.h dsp/dsp_errno.h dsp/dsp_structs.h dsp/libdsp.h \
		   dsp/DSPMessage.h dsp/DSPTransfer.h dsp/DSPControl.h \
		   dsp/DSPConversion.h dsp/DSPStructMisc.h \
		   dsp/DSPObject.h dsp/DSPError.h
PRIVATE_DERIVED_INCFILES = dsp/_dsp_message_names.h
PUBLIC_DERIVED_INCFILES = dsp/dsp_messages.h dsp/dsp_memory_map_mk.h \
			  		     dsp/dsp_memory_map_ap.h
DERIVED_INCFILES = $(PRIVATE_DERIVED_INCFILES) $(PUBLIC_DERIVED_INCFILES)
HFILES = $(PRIVATE_INCFILES) $(PUBLIC_INCFILES)
SFILES = alias.s
OTHER_SRCS = spec_dsp shlib.h foodriver_client.c

SHLIBVERS = A

local_install: debug TAGS

#this file included for standard functionality
include Makefile.lib

#  **6**  -ALL AND INSTALL TARGETS-
#  You must define your ownb all and install targets here.  "all" should
#  build all products of your project.  "install" should install these
#  products in the filesystem whose root is $(DSTROOT).  There are certain
#  targets defined in Makefile.lib which you can use to fulfill the all
#  and install targets.  These are optimized, debug, profile,
#  optimized_install, debug_install, profile_install and common_install.

all: profile shlib

install: common_install shlib_install profile_install


#  The following actions will be carried out right after the library, include
#  files, class, and message group files have been installed.
additional_installs:
#	put extra actions here with a tab in front of them

# You may comment in this line to get dependencies for the include files
# you use.  To generate the dependencies, do a "make depend".
# If you enable this line, also enable the dependency in Makefile.common .
#include Makefile.depends

dsp/dsp_messages.h dsp/dsp_memory_map_mk.h dsp/dsp_memory_map_ap.h \
	dsp/_dsp_message_names.h: \
		dsp/smsrc_dependencies.h
	cp ../../smsrc/dsp_messages.h dsp
	cp ../../smsrc/dsp_memory_map_mk.h dsp
	cp ../../smsrc/dsp_memory_map_ap.h dsp
	cp ../../smsrc/_dsp_message_names.h dsp

dsp/smsrc_dependencies.h: .ALWAYS ../../smsrc/dsp_messages.h
	if	   [ -f dsp/smsrc_dependencies.h ] \
 	  && cmp ../../smsrc/dsp_messages.h dsp/dsp_messages.h \
	  && cmp ../../smsrc/dsp_memory_map_mk.h dsp/dsp_memory_map_mk.h \
	  && cmp ../../smsrc/dsp_memory_map_ap.h dsp/dsp_memory_map_ap.h \
 	  && cmp ../../smsrc/_dsp_message_names.h dsp/_dsp_message_names.h; \
	then echo 'dsp/dsp_messages.h not changed'; \
	else echo touching; touch dsp/smsrc_dependencies.h; \
	fi

../../smsrc/dsp_messages.h:
	(cd ../../smsrc; $(MAKE) dsp_memory_map_mk.h; $(MAKE) dsp_memory_map_ap.h)

.ALWAYS:

# $(OFILES): dsp/dsp_messages.h

# Files which really depend on dsp/dsp_messages.h:
DSPBoot.o:		dsp/dsp_messages.h
DSPLoad.o:		dsp/dsp_messages.h
DSPObject.o:		dsp/dsp_messages.h
_DSPUtilities.o:	dsp/dsp_messages.h
DSPTest.o:		dsp/dsp_messages.h
DSPMessage.o:		dsp/dsp_messages.h
DSPTransfer.o:		dsp/dsp_messages.h
DSPControl.o:		dsp/dsp_messages.h

# Files which really depend on dsp/_dsp_message_names.h:
DSPGlobals.o: dsp/_dsp_message_names.h

foodriver.o: foodriver_client.c
