#import "Partials.h"

@interface Partials(Private)

-_writeBinaryScorefileStream:(NXStream *)aStream;
- _setPartialNoCopyCount: (int)howMany
  freqRatios: (short *)fRatios
  ampRatios: (float *)aRatios
  phases: (double *)phs
  orDefaultPhase: (double)defPhase;

@end

