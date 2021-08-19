TARGET		= lib56000
OBJECTS		= dsplib.o
INCLUDES	= dsplib.h
SYS		=
#SYS		= -DAPOLLO=1
CPU		= -DM68020=1
#CPU		= -DVAX=1
COMPILER	= -DPCC=1
#COMPILER	= -DGCC=1
OS		= -DBSD=1
#OS		= -DATT=1
OPT		= -O
#OPT		=
STRIP		= -s
#STRIP		=
#DEBUG		= -g
DEBUG		=
CFLAGS		= $(SYS) $(CPU) $(COMPILER) $(OS) $(OPT) $(DEBUG)
LDFLAGS		= $(STRIP) $(DEBUG)

#
#4/2/89 lrb@NeXT
#
SOURCES = dsplib.c dsplib.h
.c.o:
		$(CC) $(CFLAGS) -bsd -DNeXT=1 -c $*.c -o $*.o


$(TARGET):	$(OBJECTS)
		$(CC) $(LDFLAGS) $(OBJECTS) -o $@
#		@mv $@ $$HOME/bin

$(OBJECTS):	$(INCLUDES)

clean:
		@rm -f $(TARGET) *.o core *.CKP

#
#4/2/89 lrb@NeXT
#
INSTALLDIR = /usr/bin
SOURCEDIR = /dsplib
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


