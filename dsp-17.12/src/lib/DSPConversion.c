/* DSPConversion.c - numerical conversion between host and DSP data formats.
   Copyright 1988, NeXT, Inc.
   Modification history:
       05/01/88/jos - created.
       09/27/88/jos - Added warning on fixed-point overflow in DSPIntToFix24.
       06/04/89/jos - Removed use of DSP_I_MAXNEG
       04/23/90/jos - flushed unsupported entry points.
*/

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"
#include <math.h>		/* DSPDoubleToFix48() */

/************************** FIXED-POINT <--> INT *****************************/

int DSPFix24ToInt(DSPFix24 ival)
{
    int ivalr;
    if (ival &	0x800000)
      ivalr = ival | 0xFF000000; /* sign extend */
    else
      ivalr = ival;
    return(ivalr);
}

DSPFix24 DSPIntToFix24(int ival)
{
    DSPFix24 ivalr;
    ivalr = ival & DSP_WORD_MASK; /* strip off sign extension, if any */
    if (ival > DSP_I_MAXPOS || ival < -DSP_I_MAXPOS-1)	
      _DSPError(DSP_EFPOVFL,"DSPIntToFix24: 24-bit fixed-point overflow");
    return(ivalr);
}

int DSPFix24ToIntArray( 
    DSPFix24 *fix24Array,
    int *intArray,
    int wordCount)
{
    int i;
    for (i=0; i<wordCount; i++)
      intArray[i] = DSPFix24ToInt(fix24Array[i]);
    return(0);
}

int DSPIntToFix24Array(
    int *intArray,
    DSPFix24 *fix24Array,
    int wordCount)
{
    int i;
    for (i=0; i<wordCount; i++)
      fix24Array[i] = DSPIntToFix24(intArray[i]);
    return(0);
}

/****************************** FLOAT <--> INT *******************************/

float DSPIntToFloat(int ival)
{
    double dval;
    dval = (double) ival;
    dval *= DSP_TWO_TO_M_23;
    return((float)dval);
}


int DSPFloatToIntCountClips(
    float fval,
    int *npc,
    int *nnc)
{
    int ival;
    if (fval > DSP_F_MAXPOS) { /* cf. dsp.h */
	fval = DSP_F_MAXPOS;
	*npc += 1;
    }
    else if (fval < DSP_F_MAXNEG) {
	fval = DSP_F_MAXNEG;
	*nnc += 1;
    }
    ival = DSP_FLOAT_TO_INT(fval);
    return(ival);
}


int DSPFloatToInt(float fval)
{
    int npc,nnc,ival;
    ival = DSPFloatToIntCountClips(fval,&npc,&nnc);
    return(ival);
}


int DSPFloatToIntArray(
    float *floatArray,
    int *intArray,
    int wordCount)
{
    int i;
    int npc = 0;
    int nnc = 0;
    for (i=0; i<wordCount; i++)
      intArray[i] = DSPFloatToIntCountClips(floatArray[i],&npc,&nnc);
    if (npc>0)
      _DSPError1(EDOM,"DSPFloatToIntArray: Clipped to +1 %s times",
		 _DSPCVS(npc));
    if (nnc>0)
      _DSPError1(EDOM,"DSPFloatToIntArray: Clipped to -1 %s times",
		 _DSPCVS(nnc));
    return(npc+nnc);
}


int DSPIntToFloatArray(
    int *intArray,
    float *floatArray,
    int wordCount)
{
    int i;
    for (i=0; i<wordCount; i++)
      floatArray[i] = ((float)intArray[i]) * (float)DSP_TWO_TO_M_23;
    return(0);
}

/************************** DSPFix24 <--> FLOAT ******************************/

float DSPFix24ToFloat(int ival)
{
    return(DSPIntToFloat(DSPFix24ToInt(ival)));
}


DSPFix24 DSPFloatToFix24(float fval)
{
    return(DSPIntToFix24(DSPFloatToInt(fval)));
}


int DSPFix24ToFloatArray(
    DSPFix24 *fix24Array,
    float *floatArray,
    int wordCount)
{
    int i;
    for (i=0; i<wordCount; i++)
      floatArray[i] = DSPFix24ToFloat(fix24Array[i]);
    return(0);
}


int DSPFloatToFix24Array(
    float *floatArray,
    DSPFix24 *fix24Array,
    int wordCount)
{
    int i,ec;
    ec = DSPFloatToIntArray(floatArray,fix24Array,wordCount);
    for (i=0; i<wordCount; i++)
      fix24Array[i] = DSPIntToFix24(fix24Array[i]);
    return(ec);
}

/***************************** DOUBLE <--> INT *******************************/

double DSPIntToDouble(int ival)
{
    double dval;
    dval = (double) DSPIntToFloat(ival);
    return(dval);
}


int DSPDoubleToIntCountClips(
    double dval,
    int *npc,
    int *nnc)
{
    int ival;
    if (dval > DSP_F_MAXPOS) { /* cf. dsp.h */
	dval = DSP_F_MAXPOS;
	*npc += 1;
    }
    else if (dval < DSP_F_MAXNEG) {
	dval = DSP_F_MAXNEG;
	*nnc += 1;
    }
    ival = DSP_DOUBLE_TO_INT(dval);
    return(ival);
}


int DSPDoubleToInt(double dval)
{
    int npc,nnc,ival;
    ival = DSPDoubleToIntCountClips(dval,&npc,&nnc);
    return(ival);
}


int DSPDoubleToIntArray(
    double *doubleArray,
    int *intArray,
    int wordCount)
{
    int i;
    int npc = 0;
    int nnc = 0;
    for (i=0; i<wordCount; i++)
      intArray[i] = DSPDoubleToIntCountClips(doubleArray[i],&npc,&nnc);
    if (npc>0)
      _DSPError1(EDOM,"DSPDoubleToIntArray: Clipped to +1 %s times",
		 _DSPCVS(npc));
    if (nnc>0)
      _DSPError1(EDOM,"DSPDoubleToIntArray: Clipped to -1 %s times",
		 _DSPCVS(nnc));
    return(npc+nnc);
}


int DSPIntToDoubleArray(
    int *intArray,
    double *doubleArray,
    int wordCount)
{
    int i;
    for (i=0; i<wordCount; i++)
      doubleArray[i] = DSPIntToDouble(intArray[i]);
    return(0);
}

/*********************** FIXED-POINT <--> DOUBLE *****************************/

double DSPFix24ToDouble(int ival)
{
    return(DSPIntToDouble(DSPFix24ToInt(ival)));
}


int DSPFix24ToDoubleArray(
    DSPFix24 *fix24Array,
    double *doubleArray,
    int wordCount)
{
    int i;
    for (i=0; i<wordCount; i++)
      doubleArray[i] = DSPFix24ToDouble(fix24Array[i]);
    return(0);
}


DSPFix24 DSPDoubleToFix24(double dval)
{
    return(DSPIntToFix24(DSPDoubleToInt(dval)));
}


int DSPDoubleToFix24Array(
    double *doubleArray,
    DSPFix24 *fix24Array,
    int wordCount)
{
    int i,ec;
    ec = DSPDoubleToIntArray(doubleArray,fix24Array,wordCount);
    for (i=0; i<wordCount; i++)
      fix24Array[i] = DSPIntToFix24(fix24Array[i]);
    return(ec);
}

/**************************** DSPFix48 <--> Int ******************************/

int DSPFix48ToInt(register DSPFix48 *aFix48P)
{
    unsigned v; 
    if (!aFix48P)
      return -1;
    return (v = (0xff & (aFix48P->high24 << 24)) | (aFix48P->low24));
}


DSPFix48 *DSPIntToFix48(int ival)
{
    DSPFix48 *aFix48P;
    DSP_MALLOC(aFix48P,DSPFix48,1);
    return DSPIntToFix48UseArg(ival ,aFix48P);
}


DSPFix48 *DSPIntToFix48UseArg(
    unsigned ival,
    register DSPFix48 *aFix48P)
{
    aFix48P->low24 = ival & 0xffffff;
    aFix48P->high24 = ival >> 24;
    return aFix48P;
}

/**************************** DSPFix48 <--> DOUBLE ***************************/

DSPFix48 *DSPDoubleToFix48UseArg(
    double dval,
    register DSPFix48 *aFix48P)
{
    /* FIXME Eventually make a faster conversion here which extracts mantissa
       and breaks into two pieces according to exponent without multiply. 
       */
    double hi24;
    int ival;

    dval = (dval < DSP_F_MAXNEG) ? DSP_F_MAXNEG : dval;
    dval = (dval > DSP_F_MAXPOS) ? DSP_F_MAXPOS : dval;
    hi24 = dval * DSP_TWO_TO_23; // Truncate instead of round as in DSP_DOUBLE_TO_INT()
    aFix48P->high24 = (int)hi24;
    aFix48P->low24 = (int)((hi24-((double)aFix48P->high24))*DSP_TWO_TO_24);
    return aFix48P;
}

/*
 * FIXME Eventually make a faster conversion here which extracts mantissa
 * and breaks into two pieces according to exponent without multiply. 
 */

DSPFix48 *_DSPDoubleIntToFix48UseArg(double dval,DSPFix48 *aFix48P)
{
    double shiftedDval;
    shiftedDval = dval * DSP_TWO_TO_M_24;
    aFix48P->high24 = (int)shiftedDval;
    aFix48P->low24 = 
      (int)((shiftedDval-(double)aFix48P->high24)*DSP_TWO_TO_24);
    return aFix48P;
}


DSPFix48 *DSPDoubleToFix48(double dval)
{
    DSPFix48 *aFix48P;
    DSP_MALLOC(aFix48P,DSPFix48,1);
    return DSPDoubleToFix48UseArg(dval,aFix48P);
}


double DSPFix48ToDouble(register DSPFix48 *aFix48P)
{
    if (!aFix48P)
      return -1.0; /* FIXME or some other value */
    return ((double) aFix48P->high24)*DSP_TWO_TO_24+((double)(aFix48P->low24));
}


