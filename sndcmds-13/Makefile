#
#	Makefile - used to develop and maintain the sound commands
#	Copyright NeXT, Inc.  1990
#
#	Modification History:
#	03/21/90/mtm	Changed debug sound library path name
#	04/10/90/mtm	Added README
#	04/11/90/mtm	Added sndcompress and SndCmdsNotes.wn
#	05/21/90/mtm	Added testscript.
#	08/13/90/mtm	Added sndcompress.1, SndCmdsNotes.wn -> SndCmdsNotes.rtf
#
# NOTE: debug version is linked with the debug sound library, which grabs
#       dsp code FROM THE CURRENT WORKING DIRECTORY
#
NAME= sndcmds
BINDIR= /usr/bin
MANDIR= /usr/local/doc/sound/man
RELDIR= /usr/local/RelNotes
PRODUCTS= sndinfo sndplay sndconvert sndrecord sndcompress
CFILES= sndinfo.c sndplay.c sndconvert.c sndrecord.c snd_msgs.c sndcompress.c
OTHER_SRCS = dsprecsim16.snd dsprecsim16.asm dsprecsim24.snd dsprecsim24.asm
MANPAGES = sndinfo.1 sndplay.1 sndconvert.1 sndrecord.1 sndcompress.1
RELNOTES = SndCmdsNotes.rtf

CFLAGS= -O -Wimplicit
LDFLAGS= 
IFLAGS= -s
LIBS= -lsys_s

SOURCEFILES = $(CFILES) $(MANPAGES) $(RELNOTES) $(OTHER_SRCS)

all: $(PRODUCTS) 

clean:
	-/bin/rm -f *.o vers.c $(PRODUCTS) snd_msgs.o

installsrc: SRCROOT
	-/bin/rm -rf $(SRCROOT)
	mkdirs $(SRCROOT)
	chmod 755 $(SRCROOT)
	tar cf - $(SOURCEFILES) Makefile README testscript | (cd $(SRCROOT); tar xf -)
	chmod 444 $(SRCROOT)/*

install: all
	mkdirs $(DSTROOT)$(BINDIR)
	install $(IFLAGS) $(PRODUCTS) $(DSTROOT)$(BINDIR)
	mkdirs $(DSTROOT)$(MANDIR)
	install $(MANPAGES) $(DSTROOT)$(MANDIR)
	mkdirs $(DSTROOT)$(RELDIR)
	install $(RELNOTES) $(DSTROOT)$(RELDIR)

debug:
	$(MAKE) $(PRODUCTS)  "CFLAGS = -g -DDEBUG -I../../sound_proj" \
				"LIBS= ../../sound_proj/sound/libsound_g.a"

noshlib:
	$(MAKE) $(PRODUCTS) "LIBS= -lsound"


sndinfo: sndinfo.c vers.o /usr/include/sound/*
	$(CC) $(CFLAGS) -o sndinfo sndinfo.c vers.o $(LIBS)

sndplay: sndplay.c vers.o /usr/include/sound/*
	$(CC) $(CFLAGS) -o sndplay sndplay.c vers.o $(LIBS)

sndconvert: sndconvert.c vers.o /usr/include/sound/*
	$(CC) $(CFLAGS) -o sndconvert sndconvert.c vers.o $(LIBS)

sndrecord: sndrecord.c snd_msgs.o vers.o /usr/include/sound/*
	$(CC) $(CFLAGS) \
		-segcreate __SND dsprecsim16.snd dsprecsim16.snd \
		-segcreate __SND dsprecsim24.snd dsprecsim24.snd \
		-o sndrecord sndrecord.c snd_msgs.o vers.o $(LIBS)

sndcompress: sndcompress.c vers.o /usr/include/sound/*
	$(CC) $(CFLAGS) -o sndcompress sndcompress.c vers.o $(LIBS)

vers.c: $(OBJS)
	@rm -f $@
	vers_string -c $(NAME) > $@

SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi








