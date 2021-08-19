/* 
	synthpatches.h 
	Copyright 1989, NeXT, Inc.

	This header file is part of the Music Kit SynthPatch Library.
*/
#ifndef SYNTHPATCHES_H
#define SYNTHPATCHES_H

/* This is the header file for the Music Kit SynthPatch Library. */
   

/*
  In the naming scheme used, the name refers to the algorithm, the number 
  refers to the number of oscillators or (for fm) modulators and v stands for 
  vibrato capabilities.  
*/

/* Basic Wavetable synthesis, no envelopes. */
#import "Simp.h"

/* Wavetable synthesis with amplitude and frequency envelopes. */
#import "Wave1.h"      /* Wave table, non-interpolating oscillator */
#import "Wave1v.h"     /* Wave table, vibrato, non-interpolating oscillator */
#import "DBWave1v.h"   /* same as Wave1v, plus timbre data base */ 
#import "Wave1i.h"     /* Wave table, interpolating oscillator */
#import "Wave1vi.h"    /* Wave table  vibrato, interpolating oscillator */
#import "DBWave1vi.h"  /* same as Wave1vi, plus timbre data base */ 
#import "DBWave2vi.h"  /* 2-oscillator version of DBWave1vi */

/* Frequency modulation synthesis. */
#import "Fm1.h"        /* Simple FM, non-interpolating oscillators */
#import "Fm1v.h"       /* Simple FM, vibrato, non-interpolating oscillators */
#import "Fm1i.h"       /* Simple FM, interpolating carrier */
#import "Fm1vi.h"      /* Simple FM, vibrato, interpolating carrier */
#import "Fm2pvi.h"     /* parallel FM, vibrato, interpolating carrier */
#import "Fm2pnvi.h"    /* parallel FM, noise, vibrato, interpolating carrier */
#import "Fm2cvi.h"     /* cascade FM, vibrato, interpolating carrier */
#import "Fm2cnvi.h"    /* cascade FM, noise, vibrato, interpolating carrier */
#import "DBFm1vi.h"    /* like Fm1vi, plus timbre data base for carrier */ 

/* Plucked string synthesis. */ 
#import "Pluck.h"           

#endif SYNTHPATCHES_H

