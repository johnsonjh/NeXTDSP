/* 
	unitgenerators.h 
	Copyright 1989, NeXT, Inc.

	This file is part of the Music Kit UnitGenerator Library.
*/
#ifndef UNITGENERATORS_H
#define UNITGENERATORS_H

#import <musickit/musickit.h>

/* This is a convenient way to get the .h files for the Music Kit Unit
   Generator library. */

/* Allpass1UG - from dsp macro allpass1. See Allpass1UG.h for details. */
#import "Allpass1UGxx.h"
#import "Allpass1UGxy.h"
#import "Allpass1UGyx.h"
#import "Allpass1UGyy.h"

/* Add2UG  - from dsp macro add2. See Add2UG.h for details. 
*/
#import	"Add2UGxxx.h" 
#import "Add2UGxxy.h" 
#import "Add2UGxyx.h" 
#import "Add2UGxyy.h" 
#import "Add2UGyxx.h" 
#import "Add2UGyxy.h" 
#import "Add2UGyyx.h" 
#import "Add2UGyyy.h" 

/* AsympUG - from dsp macro asymp. See AsympUG.h for details. */
#import "AsympUGx.h" 
#import "AsympUGy.h" 

/* ConstantUG - from dsp macro constant. See ConstantUG.h for details. */
#import "ConstantUGx.h" 
#import "ConstantUGy.h" 

/* DelayUG - from dsp macro delay. See DelayUG.h for details. */
#import "DelayUGxxx.h" 
#import "DelayUGxxy.h" 
#import "DelayUGxyx.h" 
#import "DelayUGxyy.h" 
#import "DelayUGyxx.h" 
#import "DelayUGyxy.h" 
#import "DelayUGyyx.h" 
#import "DelayUGyyy.h" 

/* DswitchUG - from dsp macro dswitch. See DswitchUG.h for details. */
#import "DswitchUGxx.h" 
#import "DswitchUGxy.h" 
#import "DswitchUGyx.h" 
#import "DswitchUGyy.h" 

/* DswitchtUG - from dsp macro dswitcht. See DswitchtUG.h for details. */
#import "DswitchtUGxx.h" 
#import "DswitchtUGxy.h" 
#import "DswitchtUGyx.h" 
#import "DswitchtUGyy.h" 

/* InterpUG - from dsp macro interp. See InterpUG.h for details. */
#import "InterpUGxxxx.h" 
#import "InterpUGxxxy.h" 
#import "InterpUGxxyx.h" 
#import "InterpUGxxyy.h" 
#import "InterpUGxyxx.h" 
#import "InterpUGxyxy.h" 
#import "InterpUGxyyx.h" 
#import "InterpUGxyyy.h" 
#import "InterpUGyxxx.h" 
#import "InterpUGyxxy.h" 
#import "InterpUGyxyx.h" 
#import "InterpUGyxyy.h" 
#import "InterpUGyyxx.h" 
#import "InterpUGyyxy.h" 
#import "InterpUGyyyx.h" 
#import "InterpUGyyyy.h" 

/* Mul1add2UG - from dsp macro mul1add2. See Mul1add2UG.h for details. */
#import "Mul1add2UGxxxx.h" 
#import "Mul1add2UGxxxy.h" 
#import "Mul1add2UGxxyx.h" 
#import "Mul1add2UGxxyy.h" 
#import "Mul1add2UGxyxx.h" 
#import "Mul1add2UGxyxy.h" 
#import "Mul1add2UGxyyx.h" 
#import "Mul1add2UGxyyy.h" 
#import "Mul1add2UGyxxx.h" 
#import "Mul1add2UGyxxy.h" 
#import "Mul1add2UGyxyx.h" 
#import "Mul1add2UGyxyy.h" 
#import "Mul1add2UGyyxx.h" 
#import "Mul1add2UGyyxy.h" 
#import "Mul1add2UGyyyx.h" 
#import "Mul1add2UGyyyy.h" 

/* Mul2UG - from dsp macro mul2. See Mul2UG.h for details. */
#import "Mul2UGxxx.h" 
#import "Mul2UGxxy.h" 
#import "Mul2UGxyx.h" 
#import "Mul2UGxyy.h" 
#import "Mul2UGyxx.h" 
#import "Mul2UGyxy.h" 
#import "Mul2UGyyx.h" 
#import "Mul2UGyyy.h" 

/* OnepoleUG - from dsp macro onepole. See OnepoleUG.h for details. */
#import "OnepoleUGxx.h" 
#import "OnepoleUGxy.h" 
#import "OnepoleUGyx.h" 
#import "OnepoleUGyy.h" 

/* OnezeroUG - from dsp macro onezero. See OnezeroUG.h for details. */
#import "OnezeroUGxx.h" 
#import "OnezeroUGxy.h" 
#import "OnezeroUGyx.h" 
#import "OnezeroUGyy.h" 

/* OscgUG - from dsp macro oscg. See OscgUG.h for details. */
#import "OscgUGxy.h" 
#import "OscgUGyx.h" 
#import "OscgUGyy.h" 
#import "OscgUGxx.h" 

/* OscgafUG - from dsp macro oscgaf. See OscgafUG.h and OscgafUGs.h 
   for details. */
#import "OscgafUGxxxx.h" 
#import "OscgafUGxxxy.h" 
#import "OscgafUGxxyx.h" 
#import "OscgafUGxxyy.h" 
#import "OscgafUGxyxx.h" 
#import "OscgafUGxyxy.h" 
#import "OscgafUGxyyx.h" 
#import "OscgafUGxyyy.h" 
#import "OscgafUGyxxx.h" 
#import "OscgafUGyxxy.h" 
#import "OscgafUGyxyx.h" 
#import "OscgafUGyxyy.h" 
#import "OscgafUGyyxx.h" 
#import "OscgafUGyyxy.h" 
#import "OscgafUGyyyx.h" 
#import "OscgafUGyyyy.h" 

/* OscgafiUG - from dsp macro oscgafi. See OscgafUGi.h and OscgafUGs.h 
   for details. */
#import "OscgafiUGxxxx.h" 
#import "OscgafiUGxxxy.h" 
#import "OscgafiUGxxyx.h" 
#import "OscgafiUGxxyy.h" 
#import "OscgafiUGxyxx.h" 
#import "OscgafiUGxyxy.h" 
#import "OscgafiUGxyyx.h" 
#import "OscgafiUGxyyy.h" 
#import "OscgafiUGyxxx.h" 
#import "OscgafiUGyxxy.h" 
#import "OscgafiUGyxyx.h" 
#import "OscgafiUGyxyy.h" 
#import "OscgafiUGyyxx.h" 
#import "OscgafiUGyyxy.h" 
#import "OscgafiUGyyyx.h" 
#import "OscgafiUGyyyy.h" 

/* Out1aUG - from dsp macro out1a. See Out1aUG.h for details. */
#import "Out1aUGx.h" 
#import "Out1aUGy.h" 

/* Out1bUG - from dsp macro out1b. See Out1bUG.h for details. */
#import "Out1bUGx.h" 
#import "Out1bUGy.h" 

/* Out2sumUG - from dsp macro out2sum. See Out2sumUG.h for details. */
#import "Out2sumUGx.h" 
#import "Out2sumUGy.h" 

/* ScaleUG - from dsp macro scale. See ScaleUG.h for details. */
#import "ScaleUGxy.h" 
#import "ScaleUGyx.h" 
#import "ScaleUGyy.h" 
#import "ScaleUGxx.h" 

/* Scl1add2UG - from dsp macro scl1add2. See Scl1add2UG.h for details. */
#import "Scl1add2UGxxx.h" 
#import "Scl1add2UGxxy.h" 
#import "Scl1add2UGxyx.h" 
#import "Scl1add2UGxyy.h" 
#import "Scl1add2UGyxx.h" 
#import "Scl1add2UGyxy.h" 
#import "Scl1add2UGyyx.h" 
#import "Scl1add2UGyyy.h" 

/* Scl2add2UG - from dsp macro scl2add2. See Scl2add2UG.h for details. */
#import "Scl2add2UGxxx.h" 
#import "Scl2add2UGxxy.h" 
#import "Scl2add2UGxyx.h" 
#import "Scl2add2UGxyy.h" 
#import "Scl2add2UGyxx.h" 
#import "Scl2add2UGyxy.h" 
#import "Scl2add2UGyyx.h" 
#import "Scl2add2UGyyy.h" 

/* SnoiseUG - from dsp macro unoise. See SnoiseUG.h for details. */
#import "SnoiseUGx.h" 
#import "SnoiseUGy.h"

/* UnoiseUG - from dsp macro unoise. See UnoiseUG.h for details. */
#import "UnoiseUGx.h" 
#import "UnoiseUGy.h"

#define MK_OSCFREQSCALE 256.0 /* Used by Oscg and Oscgaf */

#endif UNITGENERATORS_H




