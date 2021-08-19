# daj - apr 21,90 - Added Examp8.score
# DAJ - Aug 9 90 - Note: If you add a .playscore, you must also add it to 
#             ../scorefiles_user.bom. It would be better if the .bom file 
#              could be generated automatically here, but I don't have time 
#              to mess with it. 
# DAJ - Aug 20, 89 - Removed BachFugue from the release because of driver bug
#              Also removed it from user.bom.  
# DAJ - Aug 23, 89 - Added BachFugue again
#              Also added it to user.bom.  
NAME = score

SCORE_DIR = ${DSTROOT}/NextLibrary/Music/Scores
SCORE_SRC_DIR = ${SRCROOT}/scorefiles

IFLAGS = -q -c

all:
	echo 'nothing to be remade' >/dev/null

SCORE_FILES = Examp1.score Examp2.score Examp3.score Examp4.score\
	Examp5.score \
	Examp6.score Examp7.score Emma.score Gamelan.score \
	Jungle.score Twilight.score Suntan.score \
	BeadGame.score Throb.score WTC1.score BachFugue.score 
PLAYSCORE_FILES = $(SCORE_FILES:.score=.playscore)
SCOREONLY_FILES = Examp8.score

INSTALL_FILES = $(SCORE_FILES) $(PLAYSCORE_FILES) $(SCOREONLY_FILES)

$(SCORE_DIR): foobarFOOBARfoobar
	mkdirs -m 775 $@

$(SCORE_SRC_DIR): foobarFOOBARfoobarFOOBAR
	mkdirs -m 775 $@

install: $(SCORE_DIR)
	install $(IFLAGS) -m 644 $(INSTALL_FILES) $(SCORE_DIR)

installsrc: $(SCORE_SRC_DIR)
	tar cf - Makefile $(INSTALL_FILES) | (cd $(SCORE_SRC_DIR); tar xf -)
	chmod 644 $(SCORE_SRC_DIR)/*

clean:
	echo 'nothing to be cleaned' >/dev/null


foobarFOOBARfoobar:

foobarFOOBARfoobarFOOBAR:

