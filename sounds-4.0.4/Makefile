
#
# Sounds project - the NeXT-supplied soundfiles
#
# This project depends on sndcmds
#

#
# The products will get installed in this directory
#
SOUNDDIR = /NextLibrary/Sounds

#
# The following soundfiles cannot be derived (the product IS the source)
#
SNDFILES = Bonk.snd Pop.snd Tink.snd Basso.snd Frog.snd Funk.snd

#
# The following file will get a SystemBeep.snd softlink to it
#
SYSTEMBEEP = Bonk.snd

#
# The following dsp programs are built to produce soundfiles
#
DSPFILES = 

#
# The following are needed to build the dsp sounds
#
OTHERSRCS = 

#
#

SRCFILES = $(SNDFILES) $(DSPFILES) $(OTHERSRCS)
DSPPRODUCTS = $(DSPFILES:.asm=.snd)
PRODUCTS= $(DSPPRODUCTS) $(SNDFILES)
IFLAGS =

.SUFFIXES: .asm .lod .snd
.asm.snd:
	-/bin/rm -f $*.snd
	asm56000 -a -b -l $<
	sndconvert -s44100 -c2 -i"(c) 1989 NeXT, Inc." $*.lod 


all: $(PRODUCTS)

clean:
	-/bin/rm -f $(DSPPRODUCTS) *.lst *.lod

installsrc: SRCROOT
	-/bin/rm -rf $(SRCROOT)
	mkdir $(SRCROOT)
	chmod 755 $(SRCROOT)
	tar cf - $(SRCFILES) Makefile | (cd $(SRCROOT); tar xf -)
	chmod 444 $(SRCROOT)/*

install: all 
	mkdirs $(DSTROOT)$(SOUNDDIR)
	-/bin/rm -f $(DSTROOT)$(SOUNDDIR)/*
	install $(IFLAGS) $(PRODUCTS) $(DSTROOT)$(SOUNDDIR)
	ln -s $(SYSTEMBEEP) $(DSTROOT)$(SOUNDDIR)/SystemBeep.snd
	chmod 444 $(DSTROOT)$(SOUNDDIR)/*

SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi

$(DSPPRODUCTS): 

