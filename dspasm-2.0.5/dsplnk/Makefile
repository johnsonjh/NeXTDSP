TARGET		= lnk56000
GLOBALS		= lnkglb.o
#GLOBALS	= lnkglb.o xs.o
OBJECTS		= dsplnk.o eval.o memap.o output.o recfld.o symtab.o util.o \
		  fake_free.o
INCLUDES	= lnkdef.h lnkdcl.h
SOURCES		= dsplnk.c eval.c memap.c output.c recfld.c symtab.c util.c \
		  fake_free.c lnkglb.c $(INCLUDES)
#LIBS		= -lm
LIBS		= -lsys_s
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
# FOR DEBUG: STRIP		=
# FOR DEBUG: DEBUG		= -g
DEBUG		=
CFLAGS		= $(CPU) $(COMPILER) $(OS) $(OPT) $(PROFILE) $(DEBUG)
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


$(TARGET):	$(OBJECTS) $(GLOBALS)
		$(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(GLOBALS) $(LIBS)

lnkglb.o:	$(INCLUDES) lnkglb.c
		$(CC) $(CFLAGS) -c lnkglb.c

xs.o:		strings
		xstr;$(CC) $(CFLAGS) -c xs.c

strings:	$(OBJECTS)

$(OBJECTS):	$(INCLUDES)

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
SOURCEDIR = /dsplnk
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


