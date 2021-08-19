#ifdef SHLIB
#include "shlib.h"
#endif

#import <musickit/musickit.h>

double _MKUGLog2 (double x)
{
  return log(x)/log(2.0);
}

BOOL _MKUGIsPowerOf2 (int n)
{
  double y;
  return ((modf(_MKUGLog2((double)n),&y)) == 0.0);
}

int _MKUGNextPowerOf2(int n)
{
    double y;
    double logN = _MKUGLog2((double)n);
    if ((modf(logN ,&y)) == 0.0)
      return n;
    return (int)pow(2.0,(double)(((int)logN)+1));
}



