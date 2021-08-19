/*  Modification history:

    daj/04/23/90 - Created from _musickit.h 

*/

typedef enum __MKToken {   
	 _MK_undef = 0400,  
	 _MK_param = ((int)MK_waveTable + 1), /* 285 */
	 _MK_objDefStart,
	 _MK_typedVar,
	 _MK_untypedVar,
	 _MK_uMinus,
	 _MK_intVarDecl,
	 _MK_doubleVarDecl,
	 _MK_stringVarDecl,
	 _MK_varDecl,
	 _MK_envVarDecl,
	 _MK_waveVarDecl,
	 _MK_objVarDecl,
	 _MK_envelopeDecl,
	 _MK_waveTableDecl,
	 _MK_objectDecl,
	 _MK_include,
	 _MK_print,
	 _MK_time,
	 _MK_part,
	 _MK_partInstance,
	 _MK_scoreInstance,
	 _MK_begin,
	 _MK_end,
	 _MK_comment,
	 _MK_endComment,
	 _MK_to,
	 _MK_tune,
	 _MK_ok,
	 _MK_noteTagRange,
	 _MK_dB,
	 _MK_ran,
	 _MK_dataFile,
	 _MK_xEnvValue,
	 _MK_yEnvValue,
	 _MK_smoothingEnvValue,
	 _MK_hNumWaveValue,
	 _MK_ampWaveValue,
	 _MK_phaseWaveValue,
	 _MK_lookupEnv,
	 _MK_info,
	 _MK_putGlobal,
	 _MK_getGlobal,
	 _MK_seed,
	 _MK_ranSeed,
	 _MK_LEQ,
	 _MK_GEQ,
	 _MK_EQU,
	 _MK_NEQ,
	 _MK_OR,
	 _MK_AND,
	 _MK_repeat,
	 _MK_if,
	 _MK_else,
	 _MK_while,
	 _MK_do,
	 /* End marker */
	 _MK_highestToken
    } _MKToken;


/* MKTokens */
#define _MK_VALIDTOKEN(_x) \
   ((((int)(_x))>=((int)_MK_undef))&&(((int)(_x))<((int)_MK_highestToken)))

/* This may be used to write names of _MKTokens, MKDataTypes,
   MKMidiPars and MKNoteTypes. */
extern const char * _MKTokName();
extern const char * _MKTokNameNoCheck();

