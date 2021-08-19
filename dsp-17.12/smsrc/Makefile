#  NeXT DSP Run-time System Makefile

NAME = smsrc
include ../Makefile.config

BOOT_BIN_DIR = ../smsrc/

#
# Sources and products
#
ASMFILES = \
	allocsys.asm hmdispatch.asm allocusr.asm hmlib.asm hmlib_mk.asm \
	verrev.asm jsrlib.asm sys_li.asm beginend.asm memmap.asm \
	misc.asm sys_xe.asm config.asm mkmon8k.asm apmon8k.asm \
	sys_ye.asm defines.asm reg_defines.asm \
	reset_boot.asm vectors.asm iv_decl.asm dspmsgs.asm handlers.asm \
	music_macros.asm ap_macros.asm \
	degnext.asm degmon.asm degmon_vectors.asm ioequ.asm

# Other DSP system files (for loading DSP system, debugging, info)

MEM_MAP_FILES = mkmon8k.mem apmon8k.mem
BOOT_PROGS = dspimg dspmsg
SRCS = $(ASMFILES) README $(MEM_MAP_FILES)

DERIVED_MK_SRC = dsp_memory_map_mk.h sys_memory_map_mk.asm
DERIVED_AP_SRC = dsp_memory_map_ap.h sys_memory_map_ap.asm
DERIVED_APMK_SRC = dsp_messages.h sys_messages.asm _dsp_message_names.h

#if DO AP AS WELL AS MK
PRODUCTS = degnext.lod mkmon8k.lod apmon8k.lod mkmon8k.dsp apmon8k.dsp 
DERIVED_SRC = $(DERIVED_MK_SRC) $(DERIVED_AP_SRC) $(DERIVED_APMK_SRC)
#else
#PRODUCTS = mkmon8k.dsp degnext.lod
#DERIVED_SRC = $(DERIVED_MK_SRC) $(DERIVED_APMK_SRC)
#endif

GARBAGE = core sm.bag

#
# Make targets
#
all: $(PRODUCTS) $(DERIVED_SRC)
	echo make all, home directory is $$HOME" > /dev/null

local_install: $(LOCAL_DSP_SYSTEM_DIR)
	echo make local_install, home directory is $$HOME" > /dev/null

# *** ASSUMPTION ***
# LOCAL_DSP_SYSTEM_DIR contains ONLY the files installed here.
# That way we can use the date of the directory to trigger installation.
# Note that "make clean" annihilates this directory.

$(LOCAL_DSP_SYSTEM_DIR): $(PRODUCTS) $(DERIVED_SRC) $(LOD_FILES)
	-mkdirs -m 755 $(LOCAL_DSP_SYSTEM_DIR)
	install $(IFLAGS) -m 644 $(PRODUCTS) $(LOCAL_DSP_SYSTEM_DIR)
	install $(IFLAGS) -m 644 $(MEM_MAP_FILES) $(LOCAL_DSP_SYSTEM_DIR)

installsrc: $(SMSRC_DIR) $(BOOT_PROGS)
	tar cf - Makefile $(SRCS) $(BOOT_PROGS) include_dirs.asm \
		include_dirs.asm.i | (cd $(SMSRC_DIR); tar xfp -)
	(cd $(SMSRC_DIR); chmod 644 Makefile $(SRCS) include_dirs.asm \
		include_dirs.asm.i)
	(cd $(SMSRC_DIR); chmod 755 $(BOOT_PROGS))

degnext.lod: degmon.asm vectors.asm
	time asm56000 -A -B -L -OS,SO degnext
	
mkmon8k.lod: $(ASMFILES)
	time asm56000 -A -B -L -OS,SO mkmon8k
	
mkmon8k.lnk: $(ASMFILES)
	time asm56000 -B -Lmkmon8kr.lst mkmon8k
	
mkmon8k.dsp: mkmon8k.lod $(BOOT_BIN_DIR)dspimg
	$(BOOT_BIN_DIR)dspimg mkmon8k.lod mkmon8k.dsp

apmon8k.lod: $(ASMFILES)
	time asm56000 -A -B -L -OS,SO apmon8k

mkmon8k.lnk: $(ASMFILES)
	time asm56000 -B -Lapmon8kr.lst apmon8k
	
apmon8k.dsp: apmon8k.lod $(BOOT_BIN_DIR)dspimg
	$(BOOT_BIN_DIR)dspimg apmon8k.lod apmon8k.dsp

$(DERIVED_MK_SRC) $(DERIVED_APMK_SRC): mkmon8k.lod  $(BOOT_BIN_DIR)dspmsg
	$(BOOT_BIN_DIR)dspmsg -v mkmon8k.lod

$(DERIVED_AP_SRC): apmon8k.lod  $(BOOT_BIN_DIR)dspmsg
	$(BOOT_BIN_DIR)dspmsg -v apmon8k.lod

install: $(PRODUCTS) $(SYSTEMDIR) $(SMSRC_INSTALL_DIR) $(DERIVED_SRC) $(BINDIR)
	install $(IFLAGS) -m 644 $(PRODUCTS) $(SYSTEMDIR)
	install $(IFLAGS) -m 644 sys_messages.asm $(SMSRC_INSTALL_DIR)
	install $(IFLAGS) -m 644 sys_memory_map_ap.asm $(SMSRC_INSTALL_DIR)
	install $(IFLAGS) -m 644 sys_memory_map_mk.asm $(SMSRC_INSTALL_DIR)
	install $(IFLAGS) -m 644 $(MEM_MAP_FILES) $(SYSTEMDIR)
	install $(IFLAGS) -m 644 $(SRCS) $(SMSRC_INSTALL_DIR)
	install $(IFLAGS) -m 644 include_dirs.asm.i \
				 $(SMSRC_INSTALL_DIR)/include_dirs.asm
	install $(IFLAGS) $(BINIFLAGS) -m 755 dspimg $(BINDIR)
	install $(IFLAGS) $(BINIFLAGS) -m 755 dspmsg $(BINDIR)

clean:	lean
	-/bin/rm -rf $(LOCAL_DSP_SYSTEM_DIR)
	-/bin/rm $(DERIVED_SRC)

lean:
	-/bin/rm -f $(PRODUCTS) $(GARBAGE)
	-/bin/rm -f *.bak *.lst *.lnk *.lod *.dsp *.img

$(BOOT_BIN_DIR)dspimg:
	(cd ../tools/dspimg; make local_install)

$(BOOT_BIN_DIR)dspmsg:
	(cd ../tools/dspmsg; make local_install)
