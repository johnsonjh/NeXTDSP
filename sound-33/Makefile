#
# Makefile for the NeXT sound library
#
#	Modification History:
#	02/15/90/mtm	Added decompress.asm
#	03/26/90/mtm	Added a target for playtest and recordtest
#	04/02/90/mtm	Added compress.asm
#	04/08/90/mtm	Added dspsound2.asm and dspsoundssi.asm
#	04/09/90/mtm	make clean now removes profile_obj
#	04/10/90/mtm	Added README
#	04/10/90/mtm	Added filtertest
#	04/11/90/mtm	Removed -shLib from shlib cc flags.
#	04/11/90/mtm	Added snddriver_client.h, SoundNotes.wn.
#	04/16/90/mtm	Added ssiplay.asm
#	04/20/90/mtm	Added shlib cflag -i$(SHLIBIDIR)/shlib.h.
#	05/11/90/mtm	Added derecord22m.asm.
#	05/21/90/mtm	Added testscript.
#	05/31/90/mtm	SoundNotes.wn -> SoundNotes.rtf
#	07/13/90/mtm	Added dspsounddi.asm.
#	07/18/90/mtm	Added ssicompress.asm and hostcompress.asm
#	07/24/90/mtm	Added user BOM.
#	07/25/90/mtm	Added compress.c
#	07/26/90/mtm	Added hostdecompress.asm and sndoutdecompress.asm
#			Removed dspsound2.asm. Added compresstest.
#	07/31/90/mtm	Added rfiletest.c.
#	08/08/90/mtm	Added resample files.
#	08/10/90/mtm	Remove downby2.c.
#	10/08/90/mtm	Remove resample files, add downby2.c back in (bug #10407).
#
#	Note: .snd files are treated as source.  This project therefore
#	does not have a dependency on dspasm, but you have to remember
#	to check out, build, and return .snd files when you modify
#	a .asm file.

NAME= sound
LIBDIR= /usr/lib
INCDIR= /usr/include
DSPDIR= /usr/lib/sound
RELNOTEDIR= /usr/local/RelNotes
USERBOMDIR = /usr/local/BOMs
RELNOTES = SoundNotes.rtf
USERBOM = $(NAME).user_bom

HFILES= sound.h \
	soundstruct.h \
	sounderror.h \
	utilsound.h \
	filesound.h \
	editsound.h \
	sounddriver.h \
	accesssound.h \
	convertsound.h \
	performsound.h \
	snddriver_client.h

CFILES= sounderror.c \
	utilsound.c \
	filesound.c \
	editsound.c \
	sounddriver.c \
	accesssound.c \
	convertsound.c \
	performsound.c

DSPFILES= booter.asm codec.asm mulawcodec.asm  mono.asm monobyte.asm \
	  dsprecord.asm \
	  encodemulawsquelch.asm mulawcodecsquelch.asm decodemulawsquelch.asm \
	  hostdecompress.asm sndoutdecompress.asm \
	  ssicompress.asm hostcompress.asm ssiplay.asm derecord22m.asm

TESTFILES= playtest.c converttest.c squelchtest.c recordtest.c decodetest.c \
	   decodemutest.c hosttest.c chaintest.c downby2test.c rchaintest.c \
	   filtertest.c compresstest.c rfiletest.c

OTHERSRCS= snddriver_client.c \
	   upsamplecodec.c squelch.c downby2.c compress.c resample.c \
	   convertlod.c portdefs.asm dspsound.asm dspsoundssi.asm \
	   dspsounddi.asm decompress.asm compress.asm \
	   $(TESTFILES) $(USERBOM)

CFLAGS= -g
ARFLAGS= ruv
OFILE_DIR= debug_obj
VPATH = $(OFILE_DIR)
OFILES= $(CFILES:.c=.o) $(NAME)_vers.o
VERSFLAGS= -l

DSPPRODUCTS= $(DSPFILES:.asm=.snd)
SOURCEFILES= $(HFILES) $(CFILES) $(DSPFILES) $(OTHERSRCS) \
	     $(DSPPRODUCTS) $(RELNOTES)
SRCPROTECTION= 444

BACKUPDIR= $(HOME)/sound_proj/sound-bak

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
.s.o:
	$(AS) -o $(OFILE_DIR)/$*.o $*.s

all: profile_ofiles

$(NAME):
	$(MAKE) lib$(NAME).a "OFILE_DIR = obj" "CFLAGS = -O -Wimplicit"

profile:
	$(MAKE) lib$(NAME)_p.a "OFILE_DIR = profile_obj" \
				 "CFLAGS = -O -pg -Wall"
profile_ofiles:
	mkdirs profile_obj
	$(MAKE) $(OFILES) "OFILE_DIR = profile_obj" \
				"CFLAGS = -O -pg -Wall"

shlib_ofiles:
	mkdirs shlib_obj
	$(MAKE) $(OFILES) "OFILE_DIR = shlib_obj" \
		"CFLAGS = -g -O -i$(SHLIBIDIR)/shlib.h -DSHLIB -I$(SHLIBIDIR) -Wall"

debug:
	$(MAKE) lib$(NAME)_g.a  "OFILE_DIR = debug_obj" \
				"CFLAGS = -g -DDEBUG -Wall"

installsrc: SRCROOT
	-/bin/rm -rf $(SRCROOT)
	mkdirs $(SRCROOT)
	chmod 755 $(SRCROOT)
	tar cf - $(SOURCEFILES) Makefile README testscript | (cd $(SRCROOT); tar xf -)
	chmod $(SRCPROTECTION) $(SRCROOT)/*


install: $(DSTROOT) all install_headers
	mkdirs $(DSTROOT)$(RELNOTEDIR)
	install $(RELNOTES) $(DSTROOT)$(RELNOTEDIR)
	mkdirs $(DSTROOT)$(USERBOMDIR)
	install $(USERBOM) $(DSTROOT)$(USERBOMDIR)

install_headers:
	-/bin/rm -rf $(DSTROOT)$(INCDIR)/$(NAME)
	mkdirs $(DSTROOT)$(INCDIR)/$(NAME)
	install $(IFLAGS) $(HFILES) $(DSTROOT)$(INCDIR)/$(NAME)
	-/bin/rm -rf $(DSTROOT)$(DSPDIR)
	mkdirs $(DSTROOT)$(DSPDIR)
	install $(IFLAGS) $(DSPPRODUCTS) $(DSTROOT)$(DSPDIR)

debug_install: $(DSTROOT) debug $(NAME) install_headers
	mkdirs $(DSTROOT)$(LIBDIR)
	install $(IFLAGS) lib$(NAME)_g.a $(DSTROOT)$(LIBDIR)
	install $(IFLAGS) lib$(NAME).a $(DSTROOT)$(LIBDIR)

clean:
	-/bin/rm -rf lib$(NAME)* obj debug_obj shlib_obj profile_obj \
		*.lod *.lst *.o \
		$(NAME)_vers.c MakeDependencies *test convertlod

diff: SRCROOT
	@for f in $(SOURCEFILES); \
	    do (echo $$f; \
	    csh -f -c "diff -c $(SRCROOT)/$$f $$f; exit 0") \
	done

SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi

$(DSTROOT):
	mkdirs $(DSTROOT)

$(SRCROOT):
	-/bin/rm -rf $(SRCROOT)
	mkdirs $(SRCROOT)

$(OFILE_DIR):
	mkdirs $(OFILE_DIR)

force_version::
	-/bin/rm -f $(NAME)_vers.c

$(NAME)_vers.c: $(OBJS)
	@rm -f $@
	vers_string $(VERSFLAGS) $(NAME) > $@

lib$(NAME).a: $(OFILE_DIR) $(OFILES)
	$(AR) $(ARFLAGS) lib$(NAME).a $(OFILES)
	ranlib lib$(NAME).a

lib$(NAME)_g.a: $(OFILE_DIR) $(OFILES)
	$(AR) $(ARFLAGS) lib$(NAME)_g.a $(OFILES)
	ranlib lib$(NAME)_g.a

lib$(NAME)_p.a: $(OFILE_DIR) $(OFILES)
	$(AR) $(ARFLAGS) lib$(NAME)_p.a $(OFILES)
	ranlib lib$(NAME)_p.a

depend MakeDependencies: $(SOURCEFILES)
	-/bin/rm -f MakeDependencies
	$(CC) -M $(CFLAGS) $(CFILES) | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' > MakeDependencies

-include MakeDependencies

$(CFILES): soundstruct.h sounderror.h
accesssound.c: accesssound.h sounddriver.h
convertsound.c: convertsound.h accesssound.h sounddriver.h
convertsound.c: upsamplecodec.c squelch.c compress.c resample.c downby2.c
editsound.c: editsound.h  utilsound.h
filesound.c: filesound.h  utilsound.h
performsound.c: performsound.h filesound.h accesssound.h sounddriver.h
sounddriver.c: sounddriver.h snddriver_client.c
snddriver_client.c: sounddriver.h
sounderror.c: sounderror.h
utilsound.c: utilsound.h filesound.h performsound.h

#
# These rules are optional until the 56k assembler is released
#
.SUFFIXES: .asm .lod .snd
.asm.snd:
	asm56000 -a -b -l $<
	convertlod $*.lod

booter.snd:
	asm56000 -a -b -l $*.asm
	convertlod $*.lod no_header

$(DSPPRODUCTS): portdefs.asm dspsound.asm dspsoundssi.asm dspsounddi.asm compress.asm decompress.asm

convertlod: debug convertlod.c
	$(CC) -O -g -o convertlod convertlod.c libsound_g.a

dspimages: convertlod
	$(MAKE) $(DSPPRODUCTS) "OFILE_DIR = debug_obj"

test_files:
	cc -g -o playtest playtest.c $(TESTLIB)
	cc -g -o recordtest recordtest.c $(TESTLIB)
	cc -g -o converttest converttest.c $(TESTLIB)
	cc -g -o squelchtest squelchtest.c $(TESTLIB)
	cc -g -o decodetest decodetest.c $(TESTLIB)
	cc -g -o decodemutest decodemutest.c $(TESTLIB)
	cc -g -o hosttest hosttest.c $(TESTLIB)
	cc -g -o chaintest chaintest.c $(TESTLIB)
	cc -g -o downby2test downby2test.c $(TESTLIB)
	cc -g -o rchaintest rchaintest.c $(TESTLIB)
	cc -g -o filtertest filtertest.c $(TESTLIB)
	cc -g -o compresstest compresstest.c $(TESTLIB)
	cc -g -o rfiletest rfiletest.c $(TESTLIB)

tests:
	make test_files "TESTLIB =  -lsys_s"

debug_tests: debug
	make test_files "TESTLIB =  libsound_g.a"

playtest: debug
	cc -g -o playtest playtest.c libsound_g.a

recordtest: debug
	cc -g -o recordtest recordtest.c libsound_g.a

compresstest: debug
	cc -g -o compresstest compresstest.c libsound_g.a

rfiletest: debug
	cc -g -o rfiletest rfiletest.c libsound_g.a

backup:
	@if [ -r $(BACKUPDIR) ] ; then \
		rm -rf $(BACKUPDIR)bak; \
		mv $(BACKUPDIR) $(BACKUPDIR)bak; \
	fi
	make installsrc SRCROOT=$(HOME)/sound_proj/sound-bak \
		"SRCPROTECTION = 664"
