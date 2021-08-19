#
# Application makefile. 
#
#
# Modifcation history
# DAJ/July 10, 90 - Changed install to strip the binary.
# DAJ/July 26, 90 - Removed chmod u+s
# DAJ/Aug 13, 90 - Added chmod u+s again (because of fixed priority policy)
# DAJ/Aug 13, 90 - Flushed installing of man pages
# DAJ/Sept 26, 90 - Added scorefiles.user_bom in installsrc and install

# Source files for this application. 
#
MFILES1 = playscore.m
MFILES2 = convertscore.m

#
# Libraries used by this application.
#
LIBS1 = -lmusic_s -lmidi -ldsp_s -lNeXT_s -ldsp_s -lsys_s 
LIBFLAGS =

LIBS2 = -lmusic_s -ldsp_s -lNeXT_s -lsys_s

#
# Flags to pass on to the compiler and linker.
#
CFLAGS = 
#
# Rules.
#
MAKEFILES = Makefile makefile

BOMFILE = scorefiles.user_bom

SRCFILES = $(MFILES1) $(MFILES2) $(BOMFILE) $(MAKEFILES)
DERIVED = $(PSWFILES:.psw=.c) 
GARBAGE = $(DERIVED) core errs 

all: playscore convertscore

playscore: playscore.o
	${CC} ${CFLAGS} -o $@ playscore.o ${LIBS1} ${LIBFLAGS}

convertscore: convertscore.o
	${CC} ${CFLAGS} -o $@ convertscore.o ${LIBS2} ${LIBFLAGS}

installsrc: $(SRCROOT)
	tar cf - $(SRCFILES) | (cd $(SRCROOT); tar xf -)
	chmod 664 $(SRCROOT)/*
#	cp playscore.1 $(SRCROOT)
#	chmod 775 playscore.1 
	
RELNOTES_DIR = ${DSTROOT}/usr/local/doc/music_proj/man

install: playscore convertscore $(DSTROOT) userbom_install
	install -s -m 4755 playscore $(DSTROOT)/usr/bin
	install -s -m 755 convertscore $(DSTROOT)/usr/bin
#	install -m 664 playscore.1 $(RELNOTES_DIR)

userbom_install:
	if [ -f $(BOMFILE) ]; \
	then \
	    mkdirs ${DSTROOT}/usr/local/BOMs ; \
	    install -m 444 $(BOMFILE) ${DSTROOT}/usr/local/BOMs ; \
	fi

clean: 
	-rm -f *.o playscore convertscore $(DERIVED) $(GARBAGE) 

help: 
	@echo '  make $(NAME) - to make the application'
	@echo '  make clean -    to remove all files but the source'


$(SRCROOT): foobarFOOBARfoobar
	mkdirs -m 775 $(SRCROOT)

$(DSTROOT): foobarFOOBARfoobarFOOBAR
	mkdirs -m 775 $(DSTROOT)/usr/bin

foobarFOOBARfoobar:

foobarFOOBARfoobarFOOBAR:
