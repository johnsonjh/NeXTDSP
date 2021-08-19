#
# Makefile for the midi interface library
#

NAME= midi
DSTDIR= /usr/lib

CFILES= midi_error.c midi_timer_error.c \
	midi_reply_handler.c midi_timer_reply_handler.c
MIGCFILES=  midi_serverUser.c midi_timerUser.c
MIGCINCLUDES= midi_replyServer.c midi_timer_replyServer.c
SOURCEFILES= Makefile $(CFILES)

CFLAGS= -MD -Wimplicit -DMACH -DNeXT_MOD -I../include
ARFLAGS= ruv
OFILE_DIR= obj
VPATH = $(OFILE_DIR)
OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)
SED=cat
#SED=sed 's/static const/static/'

debug:
	make lib$(NAME)_g.a  "OFILE_DIR = debug_obj"  \
				"CFLAGS = -g -DDEBUG $(CFLAGS)"

profile:
	make lib$(NAME)_p.a  "OFILE_DIR = prof_obj"  \
				"CFLAGS = -pg $(CFLAGS)"

$(NAME): lib$(NAME).a

all:	$(NAME) debug profile

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
	md -u Makedep -d $*.d

midi_reply_handler.o: midi_replyServer.c

midi_timer_reply_handler.o: midi_timer_replyServer.c

midi_replyServer.c: ../mig/midi_replyServer.c
	@rm -f $@
	${SED} ../mig/$@ > $@

../mig/midi_replyServer.c: ../mig/midi_reply.defs
	cd ../mig; make

midi_serverUser.c: ../mig/midi_serverUser.c
	@rm -f $@
	${SED} ../mig/$@ > $@

../mig/midi_serverUser.c: ../mig//midi_server.defs
	cd ../mig; make

midi_timer_replyServer.c: ../mig/midi_timer_replyServer.c
	@rm -f $@
	${SED} ../mig/$@ > $@

../mig/midi_timer_replyServer.c: ../mig/midi_timer_reply.defs
	cd ../mig; make

midi_timerUser.c: ../mig/midi_timerUser.c
	@rm -f $@
	${SED} ../mig/$@ > $@

../mig/midi_timerUser.c: ../mig//midi_timer.defs
	cd ../mig; make

installsrc: $(SRCDIR)
	tar cf - $(SOURCEFILES) | (cd $(SRCDIR); tar xpf -)

install: $(DSTROOT)$(DSTDIR) all
	install $(IFLAGS) lib$(NAME).a $(DSTROOT)$(DSTDIR)
	install $(IFLAGS) lib$(NAME)_p.a $(DSTROOT)$(DSTDIR)

$(OFILE_DIR) $(DSTROOT)$(DSTDIR) $(SRCDIR):
	mkdirs $@

vers.o: $(OFILES)
	@rm -f $(OFILE_DIR)/$*.c
	vers_string -l lib$(NAME).a > $(OFILE_DIR)/$*.c
	$(CC) -c $(OFILE_DIR)/$*.c -o $(OFILE_DIR)/$*.o

clean:
	-rm -rf $(OFILES) $(MIGCFILES) $(MIGCINCLUDES) \
		obj debug_obj prof_obj Makedep *.d \
		lib$(NAME).a lib$(NAME)_g.a lib$(NAME)_p.a

lib$(NAME).a: $(OFILE_DIR) $(OFILES) vers.o
	@rm -f lib$(NAME).a
	$(AR) $(ARFLAGS) lib$(NAME).a $(OFILES) vers.o
	ranlib lib$(NAME).a

lib$(NAME)_g.a: $(OFILE_DIR) $(OFILES) vers.o
	@rm -f lib$(NAME)_g.a
	$(AR) $(ARFLAGS) lib$(NAME)_g.a $(OFILES) vers.o
	ranlib lib$(NAME)_g.a

lib$(NAME)_p.a: $(OFILE_DIR) $(OFILES) vers.o
	@rm -f lib$(NAME)_p.a
	$(AR) $(ARFLAGS) lib$(NAME)_p.a $(OFILES) vers.o
	ranlib lib$(NAME)_p.a

-include Makedep



