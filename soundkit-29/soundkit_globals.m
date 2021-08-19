/* Required for compatiblity with 1.0 to turn off .const and .cstring */
#pragma CC_NO_MACH_TEXT_SECTIONS

#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

#import <objc/hashtable.h>

/* Global const data (in the text section) */
extern const char _literal1[];
const char * const _NXSoundPboardType = _literal1;
static const char _soundkit_constdata_pad1[124] = { 0 }; 

/* Literal const data (in the text section) */
static const char _literal1[] = "NeXT sound pasteboard type\0";
static const char _soundkit_constdata_pad2[100] = { 0 }; 

/* new, atomized sound pasteboard type */
NXAtom NXSoundPboardType = _literal1;

/* Global data (in the data section) */
char _soundkit_data_pad[252] = { 0 };
