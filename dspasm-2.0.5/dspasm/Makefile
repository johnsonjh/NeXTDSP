TARGET		= asm56000
GLOBALS		= dspglb.o
#GLOBALS	= dspglb.o xs.o
OBJECTS		= amode.o define.o dspasm.o encode.o eval.o macro.o output.o \
		  pseudo.o procop.o procxy.o scs.o symtab.o util.o
INCLUDES	= dspdcl.h dspdef.h
SOURCES		= amode.c define.c dspasm.c dspglb.c encode.c eval.c macro.c \
		  output.c pseudo.c procop.c procxy.c scs.c symtab.c util.c \
		  $(INCLUDES)
LIBS		= -lm
SASM		= -DSASM=0
CPU		= -DM68020=1
#CPU		= -DVAX=1
COMPILER	= -DPCC=1
#COMPILER	= -DGCC=1
OS		= -DBSD=1
#OS		= -DATT=1
OPT		= -O
#OPT		=
#PROFILE	= -pg
PROFILE		=
STRIP		= -s
#STRIP		=
#DEBUG		= -g
DEBUG		=
CFLAGS		= $(SASM) $(CPU) $(COMPILER) $(OS) $(OPT) $(PROFILE) $(DEBUG)
LDFLAGS		= $(PROFILE) $(STRIP) $(DEBUG)
MV		= mv -f
RM		= rm -f

#.c.o:
#		xstr -c $<;$(MV) $< $<x;$(MV) x.c $<
#		$(CC) $(CFLAGS) -c $<
#		$(MV) $<x $<

#
#4/2/89 lrb@NeXT
#
.c.o:
		$(CC) $(CFLAGS) -bsd -DNeXT=1 -c $*.c -o $*.o


$(TARGET):	$(OBJECTS) $(GLOBALS) Makefile
		$(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(GLOBALS) $(LIBS)

dspglb.o:	$(INCLUDES) dspglb.c Makefile
		$(CC) $(CFLAGS) -c dspglb.c

xs.o:		strings
		xstr;$(CC) $(CFLAGS) -c xs.c

strings:	$(OBJECTS)

$(OBJECTS):	$(INCLUDES) Makefile

$(SOURCES):
		sccs get $@

clean:
		@$(RM) $(TARGET) *.o *x x* strings core *.CKP

touch:
		@touch $(OBJECTS); touch $(TARGET)

#
#4/2/89 lrb@NeXT
#
INSTALLDIR = /usr/bin
SOURCEDIR = /dspasm
SOURCEFILES = $(SOURCES) Makefile

all: $(TARGET)
install:: $(DSTROOT)$(INSTALLDIR) all
	install $(IFLAGS) $(TARGET) $(DSTROOT)$(INSTALLDIR)
$(DSTROOT)$(INSTALLDIR):
	mkdirs $(DSTROOT)$(INSTALLDIR)

installsrc: SRCROOT
	-/bin/rm -rf $(SRCROOT)$(SOURCEDIR)
	mkdirs $(SRCROOT)$(SOURCEDIR)
	chmod 755 $(SRCROOT)$(SOURCEDIR)
	tar cf - $(SOURCEFILES) | (cd $(SRCROOT)$(SOURCEDIR); tar xf -)
	chmod 444 $(SRCROOT)$(SOURCEDIR)/*
SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi


