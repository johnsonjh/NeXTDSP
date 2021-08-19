/* DSPConversion.h - Functions having to do with data type conversion
 * Copyright 1989,1990, by NeXT, Inc. 
 * J.O. Smith
 */

extern int DSPFix24ToInt(DSPFix24 ival);

extern DSPFix24 DSPIntToFix24(int ival);

extern int DSPFix24ToIntArray(
    DSPFix24 *fix24Array,
    int *intArray,
    int wordCount);

extern int DSPIntToFix24Array(
    int *intArray,
    DSPFix24 *fix24Array,
    int wordCount);

extern float DSPIntToFloat(int ival);

extern int DSPFloatToIntCountClips(
    float fval,
    int *npc,			/* no. positive clips */
    int *nnc);			/* no. negative clips */

extern int DSPFloatToInt(float fval);

extern int DSPFloatToIntArray(
    float *floatArray,
    int *intArray,
    int wordCount);

extern int DSPIntToFloatArray(
    int *intArray,
    float *floatArray,
    int wordCount);

extern float DSPFix24ToFloat(int ival);

extern DSPFix24 DSPFloatToFix24(float fval);

extern int DSPFix24ToFloatArray(
    DSPFix24 *fix24Array,
    float *floatArray,
    int wordCount);

extern int DSPFloatToFix24Array(
    float *floatArray,
    DSPFix24 *fix24Array,
    int wordCount);

extern double DSPIntToDouble(int ival);

extern int DSPDoubleToIntCountClips(
    double dval,
    int *npc,			/* no. positive clips */
    int *nnc);			/* no. negative clips */

extern int DSPDoubleToInt(double dval);

extern int DSPDoubleToIntArray(
    double *doubleArray,
    int *intArray,
    int wordCount);

extern int DSPIntToDoubleArray(
    int *intArray,
    double *doubleArray,
    int wordCount);

extern double DSPFix24ToDouble(int ival);

extern int DSPFix24ToDoubleArray(
    DSPFix24 *fix24Array,
    double *doubleArray,
    int wordCount);

extern DSPFix24 DSPDoubleToFix24(double dval);

extern int DSPDoubleToFix24Array(
    double *doubleArray,
    DSPFix24 *fix24Array,
    int wordCount);


extern int DSPFix48ToInt(register DSPFix48 *aFix48P);
/* 
 * Returns *aFix48P as an int, masking out the upper two bytes of the
 * DSPFix48 datum.  If aFix48P is NULL, it returns -1.
 */


extern DSPFix48 *DSPIntToFix48(int ival);
/* 
 * Returns a pointer to a new DSPFix48 with the value as represented by ival.
 */


extern DSPFix48 *DSPIntToFix48UseArg(
    unsigned ival,
    register DSPFix48 *aFix48P);
/* 
 * Returns, in *aFix48P, the value as represented by ival. 
 * aFix48P must point to a valid DSPFix48 struct. 
 */


extern DSPFix48 *DSPDoubleToFix48UseArg(
    double dval,
    register DSPFix48 *aFix48P);
/* 
 * The double is assumed to be between -1.0 and 1.0.
 * Returns, in *aFix48P, the value as represented by dval. 
 * aFix48P must point to a valid DSPFix48 struct. 
 */


extern DSPFix48 *DSPDoubleToFix48(double dval);
/* 
 * Returns, a pointer to a new DSPFix48 
 * with the value as represented by dval. 
 * The double is assumed to be between -1.0 and 1.0. 
 */


extern double DSPFix48ToDouble(register DSPFix48 *aFix48P);
/* 
 * Returns *aFix48P as a double between 2^47 and -2^47.
 * If aFix48P is NULL, returns -1.0. 
 */


