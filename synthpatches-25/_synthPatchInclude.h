/* 
	_synthPatchInclude.h 
	Copyright 1989, NeXT, Inc.

	This header file is part of the Music Kit SynthPatch Library.
*/
/* 
Modification history:

  09/19/89/daj - Changed macros to use Note C-function parameter access.

*/
/* Various useful macros for SynthPatches. */

#import <string.h>

extern int _MKSPiVal; /* For now, each guy gets his own copy */
extern double _MKSPdVal;
extern id _MKSPoVal;
extern char *_MKSPsVal;

#define TWO_TO_M_23 0.00000011920928955078125  /* 2^(-23) */
    
#define FREQ_SCALE 512.0 /* Shift frequency increment 8 bits for fm */

/* Macros for checking the validity of parameter values. I return 0 or 1
   so that these expressions can be bitwise-ored as well as used as boolean
   expressions. */

#define iValid(intPar) ((intPar) != MAXINT)
#define dValid(dblPar) (!MKIsNoDVal(dblPar))
#define oValid(objPar) ((objPar) != nil)
#define sValid(strPar) (strlen(strPar))

/* Macros for retrieving parameter values */

#define doublePar(note,par,default) \
  (dValid (_MKSPdVal=MKGetNoteParAsDouble(note,par)) ? _MKSPdVal : default)
#define intPar(note,par,default) \
  (iValid (_MKSPiVal=MKGetNoteParAsInt(note,par)) ? _MKSPiVal : default)
#define envPar(note,par,default) \
  (oValid (_MKSPoVal=MKGetNoteParAsEnvelope(note,par)) ? _MKSPoVal : default)
#define wavePar(note,par,default) \
  (oValid (_MKSPoVal=MKGetNoteParAsWaveTable(note,par)) ? _MKSPoVal : default)
#define stringParNoCopy(note,par,default) \
  (sValid (_MKSPsVal=MKGetNoteParAsStringNoCopy(note,par)) ? _MKSPsVal : default)

/* The following macros may be used to update a synthpatch parameter only
   if the parameter is actually present in a note.  Returns true if var
   was set, otherwise returns false. */

#define updateDoublePar(note, par, var) \
  (dValid (dValid (_MKSPdVal=MKGetNoteParAsDouble(note,par)) ? (var=_MKSPdVal) : MK_NODVAL))
#define updateIntPar(note, par, var) \
  (iValid (iValid (_MKSPiVal=MKGetNoteParAsInt(note,par)) ? (var=_MKSPiVal) : MAXINT))
#define updateEnvPar(note, par, var) \
  (oValid (oValid (_MKSPoVal=MKGetNoteParAsEnvelope(note,par)) ? (var=_MKSPoVal) : nil))
#define updateWavePar(note, par, var) \
  (oValid (oValid (_MKSPoVal=MKGetNoteParAsWaveTable(note,par)) ? (var=_MKSPoVal) : nil))
#define updateStringParNoCopy(note, par, var) \
  (sValid (sValid (_MKSPsVal=MKGetNoteParAsStringNoCopy(note,par)) ? (var=_MKSPsVal) : ""))

#define updateFreq(note, var) \
  (dValid (dValid (_MKSPdVal=[note freq]) ? (var=_MKSPdVal) : MK_NODVAL))

#define volumeToAmp(vol) pow(10.,((double)vol-127.0)/64.0)

