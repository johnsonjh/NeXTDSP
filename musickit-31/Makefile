# Should do a pass on this that optimizes link times. FIXME
# 
# Modification history:
#
#  11/20/89/daj - Changed name of _SharedSynthKey to _SharedSynthInfo
#  01/08/90/daj - Fixed a few dependencies.  
#  02/01/90/daj - Moved _MKNameTable from private_classes to other_mfiles
#   3/13/90/daj - Removed _privatemsgs.h because it doesn't work with the new 
#                 compiler. Changed all classes to use categories instead:
#                 Added many _*.h files.
#   3/27/90/daj - Added ConductorDelegate.h
#   4/17/90/mmm - Changed release notes directory
#   4/17/90/mmm - added missing _UnitGenerator.h
#   4/23/90/daj - various changes for new header file system
#   4/05/90/daj - added _PartRecorder.h, which was missing from private header
#                 file list.
#   5/11/90/jos - removed profile_obj and relnotes_install from install target
#   5/16/90/daj - removed spurious DEBUG define
#   6/10/90/daj -  replaced profile_obj from install target
#   6/15/90/daj - added lock.m
#   8/03/90/daj -  added _MKSprintf.c and _MKectv.c
#   8/03/90/daj -  added noDVal.h and noDVal.m

# DSP_DIR = /usr/lib/dsp

NAME = musickit
 
#directory where make gets its includes.  Do not edit.
MAKE_DIR = /usr/local/lib/Makefiles

# MAKE_DIR = .

#  **2**  -SOURCE FILES-
#  OTHER_SRCS is used for
#  files with other suffixes that you wish to be treated like source
#  (i.e., printed with the source, copied with the source, etc.).
#  For files in OTHER_SRCS, you need to specify the corresponding .o file
#  in OTHER_SRCS_OFILES.

PUBLIC_CLASSES = \
	Midi.m\
	Conductor.m\
	Envelope.m\
	FilePerformer.m\
	FileWriter.m\
	Instrument.m\
	Note.m\
	NoteFilter.m\
	NoteReceiver.m\
	NoteSender.m\
	Orchestra.m\
	Part.m\
	PartPerformer.m\
	PartRecorder.m\
	PatchTemplate.m\
	Performer.m\
	Score.m\
	ScorePerformer.m\
	ScoreRecorder.m\
	ScorefilePerformer.m\
	ScorefileWriter.m\
	SynthData.m\
	SynthInstrument.m\
	SynthPatch.m\
	TuningSystem.m\
	UnitGenerator.m\
	Partials.m\
	WaveTable.m\
	Samples.m

PRIVATE_CLASSES = \
	_OrchloopbeginUG.m\
	_ParName.m\
	_ScorefileVar.m\
	_SharedSynthInfo.m
OTHER_MFILES = \
	_MKNameTable.m\
	parseMidi.m\
	writeMidi.m\
	parseScore.m\
	utilities.m\
	writeScore.m\
	tokenNames.m\
	time.m\
	conversion.m\
	noteRecorderCFuncs.m\
	writeBinary.m\
	midifile.m
MFILES = $(PUBLIC_CLASSES) $(PRIVATE_CLASSES) $(OTHER_MFILES)
CFILES = _MKSprintf.c
SFILES = 
PSWFILES = 
PSWMFILES = 
PSFILES = 
OTHER_SRCS = parNames.m equalTempered.m noteDispatcherMethods.m \
	orchloopbeginUGInclude.m mkerrors.m synthElementMethods.m \
	_synthElementMethods.m\
	timetagInclude.m synthElementCFuncs.m _MKecvt.c\
	noteRecorderMethods.m fastFft.c orchControl.m lock.m lock_primitives.m
OTHER_SRCS_OFILES =

#need a rule for specs that is recursive on make so that we don't have 
#to do the dependiencies.

#SPECFILES = $(PUBLIC_CLASSES:.m=.spec)

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex and yacc.  You may delete any
#  lines that you don't use.  All CFLAGS also get passed to Objective-C. 
#  DEBUGCFLAGS is used along with CFLAGS when "debug" is made.
#  Under CFLAGS, for each library that you use that supplies class
#  and message group files, you must list a -I$(OBJDIR)/XXX, where
#  XXX is the base name of the library.
OBJCFLAGS = 
# COMMONCFLAGS = -I. -I.. -I../dsp/src/lib 	*** 8/06/89/jos ***
# COMMONCFLAGS = -I. -I.. -Wcomment -Wreturn-type -Wunused -Wimplicit -W
COMMONCFLAGS = -I. -I.. -O -Wimplicit $(EXTERNAL_DEFINES)

# -W gives lots of spurious warnings, so I turn it off. 
# ditto for -Wimplicit 
# 
CFLAGS = $(COMMONCFLAGS)
# The following flags might speed up the code:
#	-fomit-frame-pointer -finline-functions
DEBUGCFLAGS = $(COMMONCFLAGS) -g
PROFCFLAGS = -pg $(COMMONCFLAGS)
# PROFCFLAGS = -pg $(COMMONCFLAGS) -g
SHLIBCFLAGS = $(COMMONCFLAGS) -g -I$(SHLIBIDIR)
PSWFLAGS =
ASFLAGS = 
SPECFLAGS = -v 2.0 -R
MKSHLIBFLAGS =
SHLIBVERS = 

# force this variable to be defined
shlib_ofiles:: SHLIBIDIR

SHLIBIDIR:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = $(PRIVATE_CLASSES:.m=.h)\
	_tokens.h\
	_MKNameTable.h\
	_Conductor.h\
	_Instrument.h\
	_Note.h\
	_NoteReceiver.h\
	_NoteSender.h\
	_Orchestra.h\
	_Part.h\
	_Partials.h\
	_Performer.h\
	_Score.h\
	_ScoreRecorder.h\
	_ScorefilePerformer.h\
	_SynthElement.h\
	_SynthPatch.h\
	_SynthInstrument.h\
	_SynthData.h\
	_UnitGenerator.h\
	_musickit.h\
	_midifile.h\
	_midi.h\
	_scorefile.h\
	_PatchTemplate.h\
	_TuningSystem.h\
	_time.h\
	_Envelope.h\
	_error.h\
	_noteRecorder.h\
	_PartPerformer.h\
	_PartRecorder.h\
	_parameter.h\
	_DSPMK.h\
	lock_primitives.h \
	_MKSprintf.h
PUBLIC_INCFILES = $(PUBLIC_CLASSES:.m=.h) \
	musickit.h\
	timeunits.h\
	devstatus.h\
	orch.h\
	dspwrap.h\
	errors.h\
	pitches.h\
	keynums.h\
	scorefileobject.h\
	params.h\
	ConductorDelegate.h\
	PerformerDelegate.h\
	noDVal.h

#  **6**  -BY_PRODUCTS GENERATED FROM BUILDS-
#  If your build procedure creates any files other than .o files and the
#  .c and .m files left over from pswrap, you should list them here so
#  they are removed when a make clean is done.
BY_PRODUCTS = orchloopbegin_.lnk

#this file included for standard functionality
include $(MAKE_DIR)/Makefile.lib

#  **7**  -ADDITIONAL INSTALLS-
#  If there are other actions that you need to do during an installation,
#  add them as actions under the below target, "additional_installs".
#  These actions will be carried out right after the library, include
#  files, class, and message group files have been installed.
#additional_installs:
#	put extra actions here with a tab in front of them
#	cp $(SPECFILES)  $(SRCDIR)/lib/$(NAME);

local_install: debug

#  all: profile_ofiles
all:$(PRODUCT) 

#install: DSTROOT $(LIBDIR) profile_ofiles common_install relnotes_install
#	install $(IFLAGS) -r -m 644 lib$(NAME).a $(LIBDIR)

install: DSTROOT $(LIBDIR) common_install profile_ofiles


DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
	fi


#  any extra rules or dependencies can be added after this line
#include Makefile.depends

# orchloopbeginUGInclude.m: orchloopbegin_.lnk
# orchloopbegin_.lnk: $(DSP_DIR)/ugsrc/orchloopbegin.asm $(DSP_DIR)/smsrc/beginend.asm
#	-/bin/rm orchloopbeginUGInclude.m
#	dspwrap -ug -noDoc -local $(DSP_DIR)/smsrc \
#		-macroDir $(DSP_DIR)/smsrc $(DSP_DIR)/ugsrc/orchloopbegin.asm
#	-/bin/rm OrchloopbeginUG.m 
#
# _OrchloopbeginUG.m: orchloopbegin_.lnk

_OrchloopbeginUG.o: _OrchloopbeginUG.m orchloopbeginUGInclude.m

Performer.o: noteDispatcherMethods.m

NoteFilter.o: noteDispatcherMethods.m

_ParName.o: parNames.m params.h

utilities.o: mkerrors.m

UnitGenerator.o: synthElementMethods.m synthElementCFuncs.m _synthElementMethods.m

SynthData.o: synthElementMethods.m _synthElementMethods.m

FileWriter.o: noteRecorderMethods.m

PartRecorder.o: noteRecorderMethods.m

Orchestra.o: orchControl.m

Conductor.o: lock.m lock_primitives.m

_MKSprintf.o: _MKecvt.c

clean::
	rm -f stripped_shlib_obj
