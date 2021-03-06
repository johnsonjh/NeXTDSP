/* This class was auto-generated by dspwrap from macro dswitcht. 
   It should not be edited. */

#import "DswitchtUGxy.h"

@implementation DswitchtUGxy : DswitchtUG

/* times in seconds/sample */
#define COMPUTETIME (108 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))
#define OFFCHIP_COMPUTETIME (158 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))

static MKLeafUGStruct _leafUGStruct = {
    {3/* xArg  */,  3/* yArg  */,  0/* lArg */,
     18/* pLoop */,  0/* pSubr */,
     0/* xData */,  0/* yData */} /* memory requirements */, COMPUTETIME};

+(MKLeafUGStruct *)classInfo  
{   if (_leafUGStruct.master == NULL)
      _leafUGStruct.master = [self masterUGPtr];
    return &_leafUGStruct; }

+initialize /* Sent once on factory start-up. */
{
enum args { i1adr, scale1, scale2, aout, i2adr, tickdelay };
   static DSPMemorySpace _argSpaces[] = {DSP_MS_Y,DSP_MS_N,DSP_MS_N,DSP_MS_X,DSP_MS_Y,DSP_MS_N};
   static DSPDataRecord _dataRecP = {NULL, DSP_LC_P, 0, 1, 18}; 
   static int _dataP[] = {0xcb9800,0x47f400,0x1,0x44d87c,0x29040,
                          0x21c513,0x20000b,0x29008,0x5f5c00,0x6edc00,
                          0xf89800,0x29050,0x21d100,0x56fe00,0x4ed900,
                          0x61180,0x91,0xf83ee1};
   static DSPFixup _fixupsP[] = {
   {DSP_LC_P, NULL, 1 /* decrement */, 16 /* refOffset */,  17 /* relAddress */}
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
