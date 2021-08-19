# NeXT Unit Generator Directory Makefile

# 04/12/89/jos - Suppressed man page generation since .cat not acceptable
#		 See "#*" below.
# 04/12/89/jos - Removed Makefile from installation

NAME = ugsrc
ASMFILES = \
	delay.asm oscgaf.asm oscgafi.asm out2sum.asm dswitch.asm oscg.asm \
	mul2.asm add2.asm onezero.asm allpass1.asm onepole.asm unoise.asm \
	scale.asm asymp.asm constant.asm orchloopbegin.asm orchloopend.asm \
	biquad.asm osci.asm oscs.asm oscgf.asm oscw.asm sawtooth.asm \
	slpdur.asm twopole.asm unoisehp.asm delayticks.asm impulses.asm \
	scl1add2.asm patch.asm ramp.asm readticks.asm scl2add2.asm \
	interp.asm mul1add2.asm dswitcht.asm out1a.asm out1b.asm snoise.asm \
	test/tscale.asm test/toscg5.asm

SRCS = $(ASMFILES) README

UG_SUMMARY = UG_MACROS_SUMMARY

#* MANPAGES = $(ASMFILES:.asm=.cat) $(UG_SUMMARY)
#* OTHER_INSTALLS = install_man_pages

OTHER_LOCAL_INSTALLS = $(MANPAGES)

include ../Makefile.asm

$(MANPAGES): manpage_hack

manpage_hack: $(ASMFILES) $(BOOT_BIN_DIR)/dspwrap
	$(BOOT_BIN_DIR)/dspwrap -ug -noLink -o ./ $(ASMFILES)
	sort < $(UG_SUMMARY) > temp.tmp
	/bin/rm -f $(UG_SUMMARY)
	mv temp.tmp $(UG_SUMMARY)
	touch manpage_hack

OTHER_LEANS = manpage_hack $(MANPAGES)

$(BOOT_BIN_DIR)/dspwrap:
	(cd ../src/bin/dspwrap; make boot)

