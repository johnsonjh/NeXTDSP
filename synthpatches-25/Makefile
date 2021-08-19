# 04/25/90 - daj - Added globals.m. Changed PartialDB.m to partialDB.m.
#                  Changes for shlib
#   5/11/90/jos - removed profile_obj and relnotes_install from install target
#   6/10/90/daj - replaced profile_obj in install target
#   8/14/90/daj - removed -g from profile_obj (you need it when doing scatter loading)

NAME = synthpatches
 
MAKE_DIR = /usr/local/lib/Makefiles
# MAKE_DIR = .

PUBLIC_CLASSES = Simp.m DBWave1vi.m Wave1i.m Wave1vi.m Fm1i.m Fm1vi.m \
		DBWave1v.m Wave1.m Wave1v.m Fm1.m Fm1v.m \
		Fm2pnvi.m Fm2pvi.m Fm2cnvi.m Fm2cvi.m DBFm1vi.m DBWave2vi.m \
	        Pluck.m

PRIVATE_CLASSES = 
OTHER_MFILES = globals.m partialsDB.m
MFILES = $(PUBLIC_CLASSES) $(PRIVATE_CLASSES) $(OTHER_MFILES)
CFILES = 
SFILES = 
PSWFILES = 
PSWMFILES = 
PSFILES = 
OTHER_SRCS = partialsDBInclude.m 
OTHER_SRCS_OFILES =

OBJCFLAGS = 
# COMMONCFLAGS = -I. -I.. -O -Wimplicit -Wreturn-type $(EXTERNAL_DEFINES)
COMMONCFLAGS = -I../musickit -I. -I.. -O -Wimplicit -Wreturn-type $(EXTERNAL_DEFINES)
# COMMONCFLAGS = -I/david/musickit -I. -I.. -O -Wimplicit -Wreturn-type $(EXTERNAL_DEFINES)
# Comment the following line in (changing david to you) when using your own 
#	clone of the musickit. Comment the above line out.
# COMMONCFLAGS = -I. -I/Net/marvin/ds/david/musickit/ -I/Net/marvin/ds/david/

#  Since we've combined libdsp and musickit to one project, we need to
#  have the include here, rather than in SGS_ENV, because the SGS_ENV gets
#  stripped in an install.

CFLAGS = $(COMMONCFLAGS) 
DEBUGCFLAGS = $(COMMONCFLAGS) -g -DDEBUG
PROFCFLAGS = -pg $(COMMONCFLAGS)
# PROFCFLAGS = -pg $(COMMONCFLAGS) -g
SHLIBCFLAGS = $(COMMONCFLAGS) -g -I$(SHLIBIDIR)
PSWFLAGS =
ASFLAGS = 
SPECFLAGS = -v 2.0 -R
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

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = $(PRIVATE_CLASSES:.m=.h) \
	partialsDB.h\
	_synthPatchInclude.h \
	_exportedPrivateMusickit.h\
	_Fm1i.h \
	_Wave1i.h \
	_MKNameTable.h
PUBLIC_INCFILES = $(PUBLIC_CLASSES:.m=.h) synthpatches.h

#  **6**  -BY_PRODUCTS GENERATED FROM BUILDS-
#  If your build procedure creates any files other than .o files and the
#  .c and .m files left over from pswrap, you should list them here so
#  they are removed when a make clean is done.
BY_PRODUCTS =

#this file included for standard functionality
include $(MAKE_DIR)/Makefile.lib

#  **7**  -ADDITIONAL INSTALLS-
#  If there are other actions that you need to do during an installation,
#  add them as actions under the below target, "additional_installs".
#  These actions will be carried out right after the library, include
#  files, class, and message group files have been installed.
#additional_installs:
#	put extra actions here with a tab in front of them
#	cp $(SPECFILES)  $(SRCDIR)/lib/$(NAME);

local_install: debug

all:$(PRODUCT)
# all: profile_ofiles

install: DSTROOT $(LIBDIR) common_install profile_ofiles

#install: DSTROOT $(LIBDIR) profile_ofiles common_install relnotes_install
#	install $(IFLAGS) -r -m 644 lib$(NAME).a $(LIBDIR)

DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
	fi


#  any extra rules or dependencies can be added after this line

INCDIR = $(DSTROOT)/usr/include/musickit/$(NAME)
