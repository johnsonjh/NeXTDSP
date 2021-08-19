# 04/25/90/daj - Changes for shlib
# 05/03/90/jos - Changes to make unit generators from source.
# 05/03/90/jos - Added "make really_clean" for deleting all derived UG sources.
#   5/11/90/jos - removed profile_obj and relnotes_install from install target
#   6/07/90/daj/jos - removed duplicate installsrc files from other_src 
#   6/10/90/daj - replaced profile_obj in install target
#   6/10/90/daj - removed -g from profile flags

PROJ_SOURCE=/Net/marvin/ds/dspdev
#PROJ_SOURCE=/me/P
DSP_SOURCE=/usr/lib/dsp
#DSP_SOURCE=$(PROJ_SOURCE)/dsp_proj/dsp
MK_PROJ=/usr/include
#MK_PROJ=$(PROJ_SOURCE)/musickit_proj
MK_SRC=$(MK_PROJ)/musickit
LIBDSP_SRC=/usr/include
#LIBDSP_SRC=$(DSP_SOURCE)/src/lib
UGSRC=$(DSP_SOURCE)/ugsrc
SMSRC=$(DSP_SOURCE)/smsrc
DSPWRAP_SRC=/usr/bin
#DSPWRAP_SRC=$(PROJ_SOURCE)/dspwrap_proj/dspwrap
DSPWRAP=$(DSPWRAP_SRC)/dspwrap -ug -nodoc -l $(SMSRC) -d $(DSPWRAP_SRC) \
	-macroDir $(UGSRC)

NAME = unitgenerators

MAKE_DIR = /usr/local/lib/Makefiles

PUBLIC_CLASSES = $(MASTER_CLASSES) $(DERIVED_CLASSES) 

MASTER_CLASSES = Add2UG.m Allpass1UG.m AsympUG.m ConstantUG.m \
	DelayUG.m DswitchUG.m DswitchtUG.m InterpUG.m Mul1add2UG.m \
	Mul2UG.m OnepoleUG.m OnezeroUG.m OscgUG.m OscgafUGs.m OscgafUG.m \
	OscgafiUG.m Out1aUG.m Out1bUG.m Out2sumUG.m ScaleUG.m Scl1add2UG.m \
	Scl2add2UG.m SnoiseUG.m UnoiseUG.m 

# These should be produced by dspwrap as an optional part of the build process

ADD2_CLASSES = \
	Add2UGxxx.m Add2UGxxy.m Add2UGxyx.m Add2UGxyy.m \
	Add2UGyxx.m Add2UGyxy.m Add2UGyyx.m Add2UGyyy.m

ALLPASS1_CLASSES = Allpass1UGxx.m Allpass1UGxy.m Allpass1UGyx.m Allpass1UGyy.m

ASYMP_CLASSES = AsympUGx.m AsympUGy.m

CONSTANT_CLASSES = ConstantUGx.m ConstantUGy.m

DELAY_CLASSES = \
	DelayUGxxx.m DelayUGxxy.m DelayUGxyx.m DelayUGxyy.m \
	DelayUGyxx.m DelayUGyxy.m DelayUGyyx.m DelayUGyyy.m

DSWITCH_CLASSES = DswitchUGxx.m DswitchUGxy.m DswitchUGyx.m DswitchUGyy.m

DSWITCHT_CLASSES = DswitchtUGxx.m DswitchtUGxy.m DswitchtUGyx.m DswitchtUGyy.m

INTERP_CLASSES = \
	InterpUGxxxx.m InterpUGxxxy.m InterpUGxxyx.m InterpUGxxyy.m \
	InterpUGxyxx.m InterpUGxyxy.m InterpUGxyyx.m InterpUGxyyy.m \
	InterpUGyxxx.m InterpUGyxxy.m InterpUGyxyx.m InterpUGyxyy.m \
	InterpUGyyxx.m InterpUGyyxy.m InterpUGyyyx.m InterpUGyyyy.m

MUL1ADD2_CLASSES = \
	Mul1add2UGxxxx.m Mul1add2UGxxxy.m Mul1add2UGxxyx.m Mul1add2UGxxyy.m \
	Mul1add2UGxyxx.m Mul1add2UGxyxy.m Mul1add2UGxyyx.m Mul1add2UGxyyy.m \
	Mul1add2UGyxxx.m Mul1add2UGyxxy.m Mul1add2UGyxyx.m Mul1add2UGyxyy.m \
	Mul1add2UGyyxx.m Mul1add2UGyyxy.m Mul1add2UGyyyx.m Mul1add2UGyyyy.m

MUL2_CLASSES = \
	Mul2UGxxx.m Mul2UGxxy.m Mul2UGxyx.m Mul2UGxyy.m \
	Mul2UGyxx.m Mul2UGyxy.m Mul2UGyyx.m Mul2UGyyy.m

ONEPOLE_CLASSES = \
	OnepoleUGxy.m OnepoleUGyx.m OnepoleUGyy.m OnepoleUGxx.m

ONEZERO_CLASSES = \
	OnezeroUGxy.m OnezeroUGyx.m OnezeroUGyy.m OnezeroUGxx.m 

OSCG_CLASSES = \
	OscgUGxy.m OscgUGyx.m OscgUGyy.m OscgUGxx.m

OSCGAF_CLASSES = \
	OscgafUGxxxx.m OscgafUGxxxy.m OscgafUGxxyx.m OscgafUGxxyy.m \
	OscgafUGxyxx.m OscgafUGxyxy.m OscgafUGxyyx.m OscgafUGxyyy.m \
	OscgafUGyxxx.m OscgafUGyxxy.m OscgafUGyxyx.m OscgafUGyxyy.m \
	OscgafUGyyxx.m OscgafUGyyxy.m OscgafUGyyyx.m OscgafUGyyyy.m

OSCGAFI_CLASSES = \
	OscgafiUGxxxx.m OscgafiUGxxxy.m OscgafiUGxxyx.m OscgafiUGxxyy.m \
	OscgafiUGxyxx.m OscgafiUGxyxy.m OscgafiUGxyyx.m OscgafiUGxyyy.m \
	OscgafiUGyxxx.m OscgafiUGyxxy.m OscgafiUGyxyx.m OscgafiUGyxyy.m \
	OscgafiUGyyxx.m OscgafiUGyyxy.m OscgafiUGyyyx.m OscgafiUGyyyy.m

OUT1A_CLASSES = Out1aUGx.m Out1aUGy.m

OUT1B_CLASSES = Out1bUGx.m Out1bUGy.m

OUT2SUM_CLASSES = Out2sumUGx.m Out2sumUGy.m

SCALE_CLASSES = ScaleUGxy.m ScaleUGyx.m ScaleUGyy.m ScaleUGxx.m

SCL1ADD2_CLASSES = \
	Scl1add2UGxxx.m Scl1add2UGxxy.m Scl1add2UGxyx.m \
	Scl1add2UGxyy.m Scl1add2UGyxx.m Scl1add2UGyxy.m \
	Scl1add2UGyyx.m Scl1add2UGyyy.m

SCL2ADD2_CLASSES = \
	Scl2add2UGxxx.m Scl2add2UGxxy.m Scl2add2UGxyx.m \
	Scl2add2UGxyy.m Scl2add2UGyxx.m Scl2add2UGyxy.m \
	Scl2add2UGyyx.m Scl2add2UGyyy.m

SNOISE_CLASSES = SnoiseUGx.m SnoiseUGy.m

UNOISE_CLASSES = UnoiseUGx.m UnoiseUGy.m

$(ADD2_CLASSES): $(UGSRC)/add2.asm
	$(DSPWRAP) $(UGSRC)/add2.asm

$(ALLPASS1_CLASSES): $(UGSRC)/allpass1.asm
	$(DSPWRAP) $(UGSRC)/allpass1.asm

$(ASYMP_CLASSES): $(UGSRC)/asymp.asm
	$(DSPWRAP) $(UGSRC)/asymp.asm

$(CONSTANT_CLASSES): $(UGSRC)/constant.asm
	$(DSPWRAP) $(UGSRC)/constant.asm

$(DELAY_CLASSES): $(UGSRC)/delay.asm
	$(DSPWRAP) $(UGSRC)/delay.asm

$(DSWITCH_CLASSES): $(UGSRC)/dswitch.asm
	$(DSPWRAP) $(UGSRC)/dswitch.asm

$(DSWITCHT_CLASSES): $(UGSRC)/dswitcht.asm
	$(DSPWRAP) $(UGSRC)/dswitcht.asm

$(INTERP_CLASSES): $(UGSRC)/interp.asm
	$(DSPWRAP) $(UGSRC)/interp.asm

$(MUL1ADD2_CLASSES): $(UGSRC)/mul1add2.asm
	$(DSPWRAP) $(UGSRC)/mul1add2.asm

$(MUL2_CLASSES): $(UGSRC)/mul2.asm
	$(DSPWRAP) $(UGSRC)/mul2.asm

$(ONEPOLE_CLASSES): $(UGSRC)/onepole.asm
	$(DSPWRAP) $(UGSRC)/onepole.asm

$(ONEZERO_CLASSES): $(UGSRC)/onezero.asm
	$(DSPWRAP) $(UGSRC)/onezero.asm

$(OSCG_CLASSES): $(UGSRC)/oscg.asm
	$(DSPWRAP) $(UGSRC)/oscg.asm

$(OSCGAF_CLASSES): $(UGSRC)/oscgaf.asm
	$(DSPWRAP) $(UGSRC)/oscgaf.asm

$(OSCGAFI_CLASSES): $(UGSRC)/oscgafi.asm
	$(DSPWRAP) $(UGSRC)/oscgafi.asm

$(OUT1A_CLASSES): $(UGSRC)/out1a.asm
	$(DSPWRAP) $(UGSRC)/out1a.asm

$(OUT1B_CLASSES): $(UGSRC)/out1b.asm
	$(DSPWRAP) $(UGSRC)/out1b.asm

$(OUT2SUM_CLASSES): $(UGSRC)/out2sum.asm
	$(DSPWRAP) $(UGSRC)/out2sum.asm

$(SCALE_CLASSES): $(UGSRC)/scale.asm
	$(DSPWRAP) $(UGSRC)/scale.asm

$(SCL1ADD2_CLASSES): $(UGSRC)/scl1add2.asm
	$(DSPWRAP) $(UGSRC)/scl1add2.asm

$(SCL2ADD2_CLASSES): $(UGSRC)/scl2add2.asm
	$(DSPWRAP) $(UGSRC)/scl2add2.asm

$(SNOISE_CLASSES): $(UGSRC)/snoise.asm
	$(DSPWRAP) $(UGSRC)/snoise.asm

$(UNOISE_CLASSES): $(UGSRC)/unoise.asm
	$(DSPWRAP) $(UGSRC)/unoise.asm

DERIVED_CLASSES = $(ADD2_CLASSES) \
	$(ALLPASS1_CLASSES) \
	$(ASYMP_CLASSES) $(CONSTANT_CLASSES) $(DELAY_CLASSES) $(DSWITCH_CLASSES) \
	$(INTERP_CLASSES) $(MUL1ADD2_CLASSES) $(MUL2_CLASSES) $(ONEPOLE_CLASSES) \
	$(ONEZERO_CLASSES) $(OSCG_CLASSES) $(OSCGAF_CLASSES) $(OSCGAFI_CLASSES) \
	$(OUT1A_CLASSES) $(OUT1B_CLASSES) $(OUT2SUM_CLASSES) $(SCALE_CLASSES) \
	$(SCL1ADD2_CLASSES) $(SCL2ADD2_CLASSES) $(SNOISE_CLASSES) \
	$(UNOISE_CLASSES) $(DSWITCHT_CLASSES)

DERIVED_HEADERS = $(DERIVED_CLASSES:.m=.h)

DERIVED_INCLUDES = \
	add2UGInclude.m allpass1UGInclude.m \
	asympUGInclude.m constantUGInclude.m \
	delayUGInclude.m dswitchUGInclude.m interpUGInclude.m \
	dswitchtUGInclude.m mul1add2UGInclude.m mul2UGInclude.m \
	onepoleUGInclude.m onezeroUGInclude.m \
	oscgUGInclude.m	oscgafUGInclude.m oscgafiUGInclude.m \
	out2sumUGInclude.m out1aUGInclude.m out1bUGInclude.m \
	scaleUGInclude.m scl1add2UGInclude.m scl2add2UGInclude.m \
	snoiseUGInclude.m unoiseUGInclude.m

Add2UG.o: $(ADD2_CLASSES)

Allpass1UG.o: $(ALLPASS1_CLASSES)

AsympUG.o: $(ASYMP_CLASSES)

ConstantUG.o: $(CONSTANT_CLASSES)

DelayUG.o: $(DELAY_CLASSES)

DswitchUG.o: $(DSWITCH_CLASSES)

DswitchtUG.o: $(DSWITCHT_CLASSES)

InterpUG.o: $(INTERP_CLASSES)

Mul1add2UG.o: $(MUL1ADD2_CLASSES)

Mul2UG.o: $(MUL2_CLASSES)

OnepoleUG.o: $(ONEPOLE_CLASSES)

OnezeroUG.o: $(ONEZERO_CLASSES)

OscgUG.o: $(OSCG_CLASSES)

OscgafUG.o: $(OSCGAF_CLASSES)

OscgafiUG.o: $(OSCGAFI_CLASSES)

Out1aUG.o: $(OUT1A_CLASSES)

Out1bUG.o: $(OUT1B_CLASSES)

Out2sumUG.o: $(OUT2SUM_CLASSES)

ScaleUG.o: $(SCALE_CLASSES)

Scl1add2UG.o: $(SCL1ADD2_CLASSES)

Scl2add2UG.o: $(SCL2ADD2_CLASSES)

SnoiseUG.o: $(SNOISE_CLASSES)

UnoiseUG.o: $(UNOISE_CLASSES)


PRIVATE_CLASSES =
OTHER_SRCS = $(DERIVED_INCLUDES) 
OTHER_MFILES = ugUtilities.m 
MFILES = $(PUBLIC_CLASSES) $(PRIVATE_CLASSES) $(OTHER_MFILES)
CFILES = 
SFILES = 
PSWFILES = 
PSWMFILES = 
PSFILES = 
OTHER_SRCS_OFILES =

OBJCFLAGS =
# COMMONCFLAGS = -I/david/musickit -I. -I.. -O -Wimplicit -Wreturn-type $(EXTERNAL_DEFINES)
# COMMONCFLAGS = -I. -I.. -O -Wimplicit -Wreturn-type $(EXTERNAL_DEFINES)
COMMONCFLAGS = -I../musickit -I. -I.. -O -Wimplicit -Wreturn-type $(EXTERNAL_DEFINES)

CFLAGS = $(COMMONCFLAGS) 
DEBUGCFLAGS = $(COMMONCFLAGS) -g -DDEBUG
PROFCFLAGS = -pg $(COMMONCFLAGS)
# PROFCFLAGS = -pg $(COMMONCFLAGS) -g
SHLIBCFLAGS = $(COMMONCFLAGS) -g -I$(SHLIBIDIR)
PSWFLAGS =
ASFLAGS = 
MKSHLIBFLAGS =
SHLIBVERS = 

# force this variable to be defined
shlib_ofiles:: SHLIBIDIR

SHLIBIDIR:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

PRIVATE_INCFILES = $(PRIVATE_CLASSES:.m=.h) _unitGeneratorInclude.h timings.h _exportedPrivateMusickit.h
PUBLIC_INCFILES = $(PUBLIC_CLASSES:.m=.h) unitgenerators.h

BY_PRODUCTS = *.lnk *.asm *.lst *~ .places .gdbinit .*~

include $(MAKE_DIR)/Makefile.lib

local_install: debug

# all: profile_ofiles
all:$(PRODUCT)

install: DSTROOT $(LIBDIR) common_install profile_ofiles

#install: DSTROOT $(LIBDIR) profile_ofiles common_install relnotes_install
#	install $(IFLAGS) -r -m 644 lib$(NAME).a $(LIBDIR)

really_clean: clean
	/bin/rm -f $(DERIVED_CLASSES) $(DERIVED_HEADERS) $(DERIVED_INCLUDES)

DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
	fi


INCDIR = $(DSTROOT)/usr/include/musickit/$(NAME)
