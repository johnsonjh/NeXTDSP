/* This class was auto-generated by dspwrap from macro add2. 
   It should not be edited. */

#import "Add2UGyyy.h"

@implementation Add2UGyyy : Add2UG

/* times in seconds/sample */
#define COMPUTETIME (112 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))
#define OFFCHIP_COMPUTETIME (218 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))

static MKLeafUGStruct _leafUGStruct = {
    {1/* xArg  */,  2/* yArg  */,  0/* lArg */,
     10/* pLoop */,  0/* pSubr */,
     0/* xData */,  0/* yData */} /* memory requirements */, COMPUTETIME};

+(MKLeafUGStruct *)classInfo  
{   if (_leafUGStruct.master == NULL)
      _leafUGStruct.master = [self masterUGPtr];
    return &_leafUGStruct; }

+initialize /* Sent once on factory start-up. */
{
enum args { i1adr, aout, i2adr };
   static DSPMemorySpace _argSpaces[] = {DSP_MS_Y,DSP_MS_Y,DSP_MS_Y};
   static DSPDataRecord _dataRecP = {NULL, DSP_LC_P, 0, 1, 10}; 
   static int _dataP[] = {0x61d800,0x6ddc00,0x6edc00,0x4cd900,0x5edd00,
                          0x61080,0x89,0x4cd940,0x4edd00,0x5e5e51};
   static DSPFixup _fixupsP[] = {
   {DSP_LC_P, NULL, 1 /* decrement */, 6 /* refOffset */,  9 /* relAddress */}
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