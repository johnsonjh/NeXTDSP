NAME = test
include ../Makefile.config

ASM_FILES = mk_ex1.asm mk_ex2.asm mk_ex3.asm ap_ex1.asm config_standalone.asm
SHELL_SCRIPTS = dspasm dspasml
#AP_EX2_FILES = README Makefile vpvtvpv.asm tvpvtvpv.asm DSPAPvpvtvpvI.c DSPAPvpvtvpvI.h ctest.c

clean:  lean
#	(cd ./ap_ex2; make clean)

lean:
	-/bin/rm -f *.lod *.lst *.lnk *.log
	-/bin/rm -f outy.io htx.io dspmsgs.io .tmpcor.tmp ts.cmd

local_install:
	echo 'nothing to do for local install' > /dev/null

installsrc: $(SOURCE_DIR)
	tar cf - Makefile README $(ASM_FILES) $(SHELL_SCRIPTS) | \
		(cd $(SOURCE_DIR); tar xfp -)
#	(cd ./ap_ex2; tar cf - $(AP_EX2_FILES) | \
#		(mkdirs $(SOURCE_DIR)/ap_ex2; cd $(SOURCE_DIR)/ap_ex2; \
#		tar xfp -))

install: $(DSP_INSTALL_ROOT)/$(NAME)
	install $(IFLAGS) -m 644 README $(ASM_FILES) \
				$(DSP_INSTALL_ROOT)/$(NAME)
	install $(IFLAGS) -m 755 $(SHELL_SCRIPTS) $(DSP_INSTALL_ROOT)/$(NAME)
#	mkdirs $(DSP_INSTALL_ROOT)/$(NAME)/ap_ex2
#	(cd ./ap_ex2; install $(IFLAGS) -m 644 $(AP_EX2_FILES) \
#			$(DSP_INSTALL_ROOT)/$(NAME)/ap_ex2)
#	(cd $(DSP_INSTALL_ROOT)/$(NAME)/ap_ex2; make all; ctest; make clean)
