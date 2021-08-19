/* 
Modification history:

  10/26/89/daj - Changed sfNonAsciiErr to sfNonScorefileErr to accomodate new
                 binary scorefile format.
  02/01/90/daj - Added MK_spsCantGetUGErr. Also added array bounds so that
                 error is generated at compile time if it's wrong.
*/
static const char *const _errors[MK_highestErr-MK_ERRORBASE] =  {
/* ---------------- Representation errors --------------- */
/*  MK_musicKitErr */         /* Generic Music Kit error. */ 
"Music Kit: %s.",
/*  MK_machErr */            
"Music Kit: %s. Mach Error: %s",  
/*  MK_cantOpenFileErr */
"Music Kit: Can't open file %s.",
/*  MK_cantCloseFileErr */
"Music Kit: Can't close file %s.",
/*  MK_outOfOrderErr */
"Music Kit: Note at time %f out-of-order. Current time is %f.",
/*  MK_samplesNoResampleErr */
"Music Kit: Samples object cannot resample.",
/*  MK_noMoreTagsErr */
"Music Kit: No more noteTags.",
/*  MK_notScorefileObjectTypeErr */
"Music Kit: The class %s does not have the appropriate methods to be used as a scorefile object type.",
/* ---------------- Synthesis errors --------------------- */
/*  MK_orchBadFreeErr */
"Music Kit: Unit Generators are still in use.",
/*  MK_synthDataCantClearErr */
"Music Kit: Can't clear SynthData memory.",
/*  MK_synthDataLoadErr */
"Music Kit: Can't load SynthData.",
/*  MK_synthDataReadonlyLoadErr */
"Music Kit: Can't clear or load read-only SynthData.",
/*  MK_synthInsOmitNoteErr */
"SynthInstrument: Omitting note at time %f.",
/*  MK_synthInsNoClass */
"Music Kit: No SynthPatch class set in SynthInstrument.",
/*  MK_ugLoadErr */
"Music Kit: Can't load unit generator %s.",
/*  MK_ugBadArgErr */
"Music Kit: Argument %d out of bounds for %s.",
/*  MK_ugBadAddrPokeErr */
"Music Kit: Could not put address %d into argument %s of %s.",
/*  MK_ugBadDatumPokeErr */
"Music Kit: Could not put datum %d into argument %s of %s.",
/*  MK_ugOrchMismatchErr */
"Music Kit: Attempt to put address into argument of unit generator of a different orchestra.",
/*  MK_ugArgSpaceMismatchErr */
"Music Kit: Attempt to put %s-space address into %s-space argument %s of %s.",
/*  MK_ugNonAddrErr */
"Music Kit: Attempt to set address-valued argument %s of %s to datum value.",
/*  MK_ugNonDatumErr */
"Music Kit: Attempt to set argument %s of %s to an address.",


/* ------------------------- Scorefile language parse errors -------------- */
/* These don't have "Scorefile error:" at the beginning because the scorefile
   error printing function gives enough information. */

/* Illegal constructs */
/*  MK_sfBadExprErr */
"Illegal expression.",
/*  MK_sfBadDefineErr */
"Illegal %s definition.",
/*  MK_sfBadParValErr */
"Illegal parameter value.",
/*  MK_sfNoNestDefineErr */
"%s definitions cannot be nested.",

/* Missing constructs */
/*  MK_sfBadDeclErr */
"Missing or illegal %s declaration.",
/*  MK_sfMissingStringErr */
"Missing '%s'.",
/*  MK_sfBadNoteTypeErr */
"Missing noteType or duration.",
/*  MK_sfBadNoteTagErr */
"Missing noteTag.",
/*  MK_sfMissingBackslashErr */
"Back-slash must proceed newline.",
/*  MK_sfMissingSemicolonErr */
"Illegal statement. (Missing semicolon?) ",
/*  MK_sfUndeclaredErr */
"Undefined %s: %s",
/*  MK_sfBadAssignErr */
"You can't assign to a %s.",
/*  MK_sfBadIncludeErr */
"A %s must appear in the same file as the matching %s",
/*  MK_sfBadParamErr */
"Parameter name expected here.",
/*  MK_sfNumberErr */
"Numeric value expected here.", 
/*  MK_sfStringErr */
"String value expected here.", 
/*  MK_sfGlobalErr */
"A %s may not be global.",
/*  MK_sfCantFindGlobalErr */
"Can't find global %s.",

/* Duplicate constructs */
/*  MK_sfMulDefErr */
"%s is already defined as a %s.",
/*  MK_sfDuplicateDeclErr */
"Duplicate declaration for %s.",

/* Construct in wrong place */
/*  MK_sfNotHereErr */
"A %s may not appear here.",
/*  MK_sfWrongTypeDeclErr */
"%s may not be declared as a %s here.",
/*  MK_sfBadHeaderStmtErr */
"A header statement or declaration may not begin with %s.",
/*  MK_sfBadStmtErr */
"A body statement or declaration may not begin with %s.",

/*  MK_sfBadInitErr */
"Illegal %s initialization.",
/*  MK_sfNoTuneErr */
"Argument to 'tune' must be a pitch variable or number.",
/*  MK_sfNoIncludeErr */
"Can't 'include' a file when not reading from a file.",
/*  MK_sfCantFindFileErr */
"Can't find file %s.",
/*  MK_sfCantWriteErr */
"Can't write %s.",
/*  MK_sfOutOfOrderErr */
"%s values must be increasing.",
/*  MK_sfUnmatchedCommentErr */
"'comment' without matching 'endComment'.",
/*  MK_sfInactiveNoteTagErr */
"%s without active noteTag.",
/*  MK_sfCantFindClass */
"Can't find class %s.", 
/*  MK_sfBoundsErr */
"Lookup value out of bounds.",
/*  MK_sfTypeConversionErr */
"Illegal type conversion.",
/*  MK_sfReadOnlyErr */
"Can't set %s. It is a readOnly variable.",
/*  MK_sfArithErr */
"Arithmetic error.",
/*  MK_sfNonScorefileErr */
"This doesn't look like a scorefile.",
/*  MK_sfTooManyErrorsErr */
"Too many parser errors. Quitting.",

/* ------------------------- UnitGenerator Library errors -------------- */
/*  MK_ugsNotSetRunErr */
"Unitgenerator Library: %s must be set before running %s.",
/*  MK_ugsPowerOf2Err */
"Unitgenerator Library: Table size of %s must be a power of 2.",
/*  MK_ugsNotSetGetErr */
"Unitgenerator Library: %s of %s must be set before getting %s.",

/* ------------------------- SynthPatch Library errors -------------- */
/*  MK_spsCantGetMemoryErr */
"Synthpatch Library: Out of %s memory at time %.3f.",
/*  MK_spsSineROMSubstitutionErr */
"Synthpatch Library: Out of wavetable memory at time %.3f. Using sine ROM.",
/*  MK_spsInvalidPartialsDatabaseKeywordErr */
"Synthpatch Library: Invalid timbre database keyword: %s.",
/*  MK_spsOutOfRangeErr */
"Synthpatch Library: %s out of range.",
/*  MK_spsCantGetUGErr */
"Synthpatch Library: Can't allocate %s at time %.3f.",

};

