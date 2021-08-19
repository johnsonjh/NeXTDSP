#ifdef SHLIB
#include "shlib.h"
#endif

#import <objc/objc.h>

/* GLOBALS */
int _MKSPiVal = 0; /* For now, each guy gets his own copy */
double _MKSPdVal = 0;
id _MKSPoVal = 0;
char *_MKSPsVal = 0;

