
NAME=midi
DSTDIR=/usr/lib/kern_loader/Midi
CONFDIR=/private/etc
CFILES= midi_server.c midi_device.c midi_io.c midi_timer.c
MIGCFILES= midi_replyUser.c midi_serverServer.c \
	midi_timerServer.c midi_timerUser.c \
	midi_timer_replyUser.c midi_timer_replyServer.c
HFILES= midi_server_server.h midi_timer_server.h midi_timer_reply_server.h \
	midi_var.h midi_timer_var.h
LDFILES= Load_Commands.sect Unload_Commands.sect
SOURCEFILES= Makefile $(CFILES) $(HFILES) $(LDFILES)
SED=cat
# testing only - debug, loadable file...
G_LOADABLE=
#G_LOADABLE=-d $(NAME)_loadable

#LIBS=-lloadserv
CFLAGS= -O -g -MD -DKERNEL -DKERNEL_FEATURES -DMACH -DNeXT_MOD -I../include
OFILE_DIR= obj
VPATH = $(OFILE_DIR)
OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)

debug:
	make $(NAME)_reloc_g  \
		"OFILE_DIR=debug_obj"  \
		"G_LOADABLE=-d $(NAME)_loadable" \
		"CFLAGS=-g -DDEBUG -DXPR_DEBUG $(CFLAGS)"

tags:	
	ctags $(CFILES) $(HFILES)

all:	$(NAME)_reloc debug tags

installsrc: $(SRCDIR)
	tar cf - $(SOURCEFILES) | (cd $(SRCDIR); tar xpf -)

install: $(DSTROOT)$(DSTDIR) $(DSTROOT)$(CONFDIR) $(NAME)_reloc
	install $(NAME)_reloc $(DSTROOT)$(DSTDIR)

$(OFILE_DIR) $(DSTROOT)$(DSTDIR) $(DSTROOT)$(CONFDIR) $(SRCDIR):
	mkdirs $@

vers.o: $(OFILES)
	@rm -f $(OFILE_DIR)/$*.c
	vers_string -c $(NAME)_reloc > $(OFILE_DIR)/$*.c
	$(CC) -c $(OFILE_DIR)/$*.c -o $(OFILE_DIR)/$*.o

clean:
	-rm -rf $(NAME)_reloc $(NAME)_reloc_g $(OFILES) $(MIGCFILES) obj \
		debug_obj *.d vers.c Makedep 

$(NAME)_reloc $(NAME)_reloc_g: $(OFILE_DIR) $(OFILES) vers.o  \
		Load_Commands.sect Unload_Commands.sect
	kl_ld -n $(NAME) -l Load_Commands.sect -u Unload_Commands.sect \
		-i midi_var $(G_LOADABLE) -o $@ $(OFILES) vers.o $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
	md -u Makedep -d $*.d

midi_serverServer.c: ../mig/midi_serverServer.c
	@rm -f $@
	cp ../mig/$@ .

../mig/midi_serverServer.c: ../mig/midi_server.defs
	(cd ../mig; make)

midi_replyUser.c: ../mig/midi_replyUser.c
	@rm -f $@
	cp ../mig/$@ .

../mig/midi_replyUser.c: ../mig/midi_reply.defs
	(cd ../mig; make)

midi_timerServer.c: ../mig/midi_timerServer.c
	@rm -f $@
	cp ../mig/$@ .

../mig/midi_timerServer.c: ../mig/midi_timer.defs
	(cd ../mig; make)

midi_timerUser.c: ../mig/midi_timerUser.c
	@rm -f $@
	cp ../mig/$@ .

../mig/midi_timerUser.c: ../mig/midi_timer.defs
	(cd ../mig; make)

midi_timer_replyUser.c: ../mig/midi_timer_replyUser.c
	@rm -f $@
	cp ../mig/$@ .

../mig/midi_timer_replyUser.c: ../mig/midi_timer_reply.defs
	(cd ../mig; make)

midi_timer_replyServer.c: ../mig/midi_timer_replyServer.c
	@rm -f $@
	echo "#define timer_event midi_timer_event" > $@
	cat ../mig/$@ >> $@

../mig/midi_timer_replyServer.c: ../mig/midi_timer_reply.defs
	(cd ../mig; make)

-include Makedep
