# Master Makefile for DSP software

NAME =
include Makefile.config

MAKE_FILES = Makefile Makefile.config Makefile.asm

DIRS = smsrc src ugsrc test

all local_install lean:
	@for i in $(DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done

BOM_DIR=${DSTROOT}/usr/local/BOMs

#	install $(IFLAGS) -m 664 DSPNotes.rtf $(RELNOTES_DIR)
install: $(RELNOTES_DIR)
	@for i in $(DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done
	-/bin/rm -f $(BOM_DIR)/dsp.user_bom
	mkdirs $(BOM_DIR)
	cp dsp.user_bom $(BOM_DIR)

INSTALLSRC_SRCS = $(MAKE_FILES) README DSPNotes.wn DSPNotes.rtf dsp.user_bom
INSTALLSRC_BINARIES = testinstall testinstallsrc

installsrc: $(SOURCE_DIR)
	tar cf - $(INSTALLSRC_SRCS) | (cd $(SOURCE_DIR); tar xfp -)
	(cd $(SOURCE_DIR); chmod 644 $(INSTALLSRC_SRCS))
	tar cf - $(INSTALLSRC_BINARIES) | (cd $(SOURCE_DIR); tar xfp -)
	(cd $(SOURCE_DIR); chmod 755 $(INSTALLSRC_BINARIES))
	@for i in $(DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done
	(cd $(SOURCE_DIR); ls -algR > SOURCE_BOM)

clean:
	/bin/rm -rf $(LCLBINDIR)
	@for i in $(DIRS); do \
		(cd ./$$i; echo "$$i:"; \
		make $(MAKE_FLAGS) $@) || exit $? ; done
