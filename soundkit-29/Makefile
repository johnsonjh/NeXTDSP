#
# Makefile for the NeXT sound library
#

NAME= soundkit
LIBDIR= /usr/lib
INCDIR= /usr/include

MFILES = Sound.m SoundView.m SoundMeter.m soundkit_globals.m
HFILES = soundkit.h Sound.h SoundView.h SoundMeter.h
PSWFILES = soundWraps.psw

#
#

CFLAGS= -O -Wall
ARFLAGS= ruv
OFILE_DIR= obj
VPATH = $(OFILE_DIR)
OFILES= $(PSWFILES:.psw=.o) $(MFILES:.m=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) $(NAME)_vers.o
VERSFLAGS = -l
PSWFLAGS = -a -r -S shlib.h
OBJCFLAGS = 

SOURCEFILES= $(MFILES) $(HFILES) $(PSWFILES)

.m.o:
	$(CC) $(CFLAGS) -c $*.m -o $(OFILE_DIR)/$*.o
.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
.s.o:
	$(AS) -o $(OFILE_DIR)/$*.o $*.s

.psw.o:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
	
.psw.c:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw
	
.psw.h:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw

.pswm.o:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.m $*.pswm
	$(CC) $(CFLAGS) $(OBJCFLAGS) -c $*.m -o $(OFILE_DIR)/$*.o

.pswm.c:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.m $*.pswm
	
.pswm.h:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.m $*.pswm

#all: profile
all: profile_ofiles

$(NAME):
	$(MAKE) lib$(NAME).a "OFILE_DIR = obj"

debug:
	$(MAKE) lib$(NAME)_g.a  "OFILE_DIR = debug_obj" \
				"CFLAGS = -g -DDEBUG -Wall"

profile:
	$(MAKE) lib$(NAME)_p.a  "OFILE_DIR = profile_obj" \
				"CFLAGS = -pg -Wall"

profile_ofiles:
	mkdirs profile_obj
	$(MAKE) $(OFILES)  "OFILE_DIR = profile_obj" \
				"CFLAGS = -pg -Wall"

shlib_ofiles:
	mkdirs shlib_obj
	$(MAKE) $(OFILES)  "OFILE_DIR = shlib_obj" \
		"CFLAGS = -g -O -Wall -DSHLIB -I$(SHLIBIDIR)"

installsrc: SRCROOT
	-/bin/rm -rf $(SRCROOT)
	mkdir $(SRCROOT)
	chmod 755 $(SRCROOT)
	tar cf - $(SOURCEFILES) Makefile | (cd $(SRCROOT); tar xf -)
	chmod 444 $(SRCROOT)/*


install: all
#	mkdirs $(DSTROOT)$(LIBDIR)
#	install $(IFLAGS) lib$(NAME).a $(DSTROOT)$(LIBDIR)
#	install $(IFLAGS) lib$(NAME)_p.a $(DSTROOT)$(LIBDIR)
	-/bin/rm -rf $(DSTROOT)$(INCDIR)/$(NAME)
	mkdirs $(DSTROOT)$(INCDIR)/$(NAME)
	install $(IFLAGS) $(HFILES) $(DSTROOT)$(INCDIR)/$(NAME)

debug_install: debug 
	mkdirs $(DSTROOT)$(LIBDIR)
	install $(IFLAGS) lib$(NAME)_g.a $(DSTROOT)$(LIBDIR)
	-/bin/rm -rf $(DSTROOT)$(INCDIR)/$(NAME)
	mkdirs $(DSTROOT)$(INCDIR)/$(NAME)
	install $(IFLAGS) $(HFILES) $(DSTROOT)$(INCDIR)/$(NAME)

clean:
	-/bin/rm -rf lib$(NAME)* obj debug_obj profile_obj shlib_obj *.o \
		$(NAME)_vers.c MakeDependencies \
		$(PSWFILES:.psw=.c) $(PSWFILES:.psw=.h)

diff: SRCROOT
	@for f in $(SOURCEFILES); \
	    do (echo $$f; \
	    csh -f -c "diff -c $(SRCROOT)/$$f $$f; exit 0") \
	done

SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi

$(OFILE_DIR):
	mkdirs $(OFILE_DIR)

force_version::
	-/bin/rm -f $(NAME)_vers.c

$(NAME)_vers.c: $(OFILES)
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



