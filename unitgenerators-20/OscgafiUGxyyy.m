/* This class was auto-generated by dspwrap from macro oscgafi. 
   It should not be edited. */

#import "OscgafiUGxyyy.h"

@implementation OscgafiUGxyyy : OscgafiUG

/* times in seconds/sample */
#define COMPUTETIME (488 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))
#define OFFCHIP_COMPUTETIME (704 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))

static MKLeafUGStruct _leafUGStruct = {
    {3/* xArg  */,  3/* yArg  */,  1/* lArg */,
     32/* pLoop */,  0/* pSubr */,
     0/* xData */,  0/* yData */} /* memory requirements */, COMPUTETIME};

+(MKLeafUGStruct *)classInfo  
{   if (_leafUGStruct.master == NULL)
      _leafUGStruct.master = [self masterUGPtr];
    return &_leafUGStruct; }

+initialize /* Sent once on factory start-up. */
{
enum args { aina, atab, inc, ainf, aout, mtab, phs };
   static DSPMemorySpace _argSpaces[] = {DSP_MS_Y,DSP_MS_Y,DSP_MS_N,DSP_MS_Y,DSP_MS_X,DSP_MS_N,DSP_MS_N};
   static DSPDataRecord _dataRecP = {NULL, DSP_LC_P, 0, 1, 32}; 
   static int _dataP[] = {0x6edc00,0x69dc00,0x65d800,0xf09800,0x47d81b,
                          0x41e200,0x606200,0x6c6200,0x20f000,0x22b400,
                          0x56f45e,0x1,0x212510,0x61080,0x9c,
                          0x21bd56,0x219c61,0x4fed23,0x218500,0x5eec00,
                          0x200074,0x21c771,0x4dd9f3,0x21c7aa,0x4dd800,
                          0x2125f1,0x19b45e,0x1,0x475e10,0x60e200,
                          0x6ce200,0x415a00};
   static DSPFixup _fixupsP[] = {
   {DSP_LC_P, NULL, 1 /* decrement */, 14 /* refOffset */,  28 /* relAddress */}
   };
   _leafUGStruct.master = NULL;
   _leafUGStruct.argSpaces = _argSpaces;
   _leafUGStruct.data[(int)DSP_LC_P] = &_dataRecP;
   _dataRecP.data = _dataP;
   _leafUGStruct.fixups[(int)DSP_LC_P - (int)DSP_LC_P_BASE] = _fixupsP;
   MKInitUnitGeneratorClass(&_leafUGStruct);
   _leafUGStruct.reserved1 = MK_2COMPUTETIMES;
   _leafUGStruct.offChipComputeTime = OFFCHIP_COMPUTETIME;
   return self;
}
@end
