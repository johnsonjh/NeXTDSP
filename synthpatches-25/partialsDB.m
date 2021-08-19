#ifdef SHLIB
#include "shlib.h"
#endif

/* 
Modification history:

  09/22/89/daj - Changed _MKNameTableAddNameNoCopy() call to conform
                 with changes to _MKNameTable. Added init of hashtable to 
		 nil.
  09/22/89/daj - Changed synth definition to have const data.
  10/15/89/daj - Changed to use HashTable instead of _MKNameTable (because
                 of changes to the latter. Also, added some optimizations.
  04/23/90/daj - Changed to partialsDB.m (made it no longer a class).
  07/24/90/daj - Changed sscanf to atoi to support separate-threaded Music Kit
                 performance.
  08/17/90/daj - Changed to use new float/int format Partials objects. It saves
                 300K in the size of libmusic and in vmem space. Also changed 
		 to use MKGetPartialsClass(). I experimented with more radical
		 changes: Initializing of data base could be made lazy, but
		 it would only make a 40 or 50K difference at most. Harmonic
		 number arrays could be made into ranges, but this would only
		 save about 75K at most. Short arrays could be made into 
		 unsigned char arrays but that would only save about 50K.
		 These are small wins compared with 300K, so I'm calling it
		 a day.  Note, however, that a lot of ugly hacking was done
		 to Partials to accomodate this change.  Partials should be
		 cleaned up (and the data base made public) in a future release.
		 Made nameStr not static -- there's no need for it to be.
*/
#import <objc/List.h>
#import <objc/HashTable.h>
#import "partialsDB.h"
#import <musickit/musickit.h>
#import "_exportedPrivateMusickit.h"
#import <string.h>
#import <stdlib.h>

/* Creates a set of Partials objects which contain the spectra of analyzed
   voices and instruments. Created by Michael McNabb */

static id hashtable = nil;

struct synth {
	const double minfrq;
	const double maxfrq;
	const int numharms;
	const short * const hrms;
	const float * const amps; };

#include "partialsDBInclude.m"

static IMP withinRange = NULL;

void _MKSPInitializeTimbreDataBase(void)
{
  int i, j;
  id p,list;
  id partialsClass;
  IMP setAll; 
  IMP setRange; 
  IMP addObj; 
  struct synth *s, **ss;
  if (hashtable) return;
  partialsClass = MKGetPartialsClass();
  withinRange = [partialsClass instanceMethodFor:
		 @selector(freqWithinRange:)];
  setAll = [partialsClass instanceMethodFor:
	    @selector(_setPartialNoCopyCount:freqRatios:ampRatios:
		    phases:orDefaultPhase:)];  
  setRange = [partialsClass instanceMethodFor:
		  @selector(setFreqRangeLow:high:)];  
  addObj = [List instanceMethodFor:@selector(addObject:)];  
  hashtable = [HashTable newKeyDesc:"*" valueDesc:"@" capacity:NUMTIMBRES];
  for (i=0; i<NUMTIMBRES; i++) {
    list = [List new];
    ss = (struct synth **)mmm_tables[i];
    for (j=0; j<mmm_table_lens[i]; j++) {
      p = [partialsClass new];
      s = ss[j];
      (*setAll)(p,
		@selector(_setPartialNoCopyCount:freqRatios:ampRatios:
			phases:orDefaultPharse:),
		(int) s->numharms,
		(int *) s->hrms,
		(float *) s->amps,
		NULL,0.0);
      (*setRange)(p,@selector(setFreqRangeLow:high:),
		  (double) s->minfrq,
		  (double) s->maxfrq);
      (*addObj)(list,@selector(addObject:),p);
    }
    [hashtable insertKey:(const void *) mmm_table_names[i] value:(void *)list];
  }
}

id _MKSPPartialForTimbre(char *timbreString,double freq,int index)
{
  char nameStr[8];
  int len = strlen(timbreString);
  int nameLen = strcspn(timbreString,"0123456789");
  id list;
  register id *partial;
  register int count, i;
  if (nameLen==0) {
    _MKErrorf(MK_spsInvalidPartialsDatabaseKeywordErr,timbreString);
    return nil;
  }
  if (nameLen < len) {
    index = atoi((const char *)(timbreString+nameLen));
/*    sscanf(timbreString+nameLen, "%d", &index);  // DAJ -- thread-unsafe */
    strncpy(nameStr,timbreString,nameLen);
    nameStr[nameLen] = '\0';
  }
  list = (id)[hashtable valueForKey:(const void *)
	      ((nameLen<len) ? nameStr : timbreString)];
  if (list == nil) {
    _MKErrorf(MK_spsInvalidPartialsDatabaseKeywordErr,timbreString);
    return nil;
  }
  count = [list count];
  if (index) {
    if (index < 1) index = 1;
    if (index > count) index = count;
    return (NX_ADDRESS(list)[index-1]);
  }
  else {
      partial = NX_ADDRESS(list);
      for (i=0; i<count; i++,partial++)
	if ((BOOL)(*withinRange)(*partial,
				 @selector(freqWithinRange:),freq))
	  break;
  }
  return *partial;
}

