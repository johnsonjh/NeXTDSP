/* This class was auto-generated by dspwrap from macro asymp. 
   It should not be edited. */

#import "AsympUGx.h"

@implementation AsympUGx : AsympUG

/* times in seconds/sample */
#define COMPUTETIME (82 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))
#define OFFCHIP_COMPUTETIME (122 * (DSP_CLOCK_PERIOD / DSPMK_I_NTICK))

static MKLeafUGStruct _leafUGStruct = {
    {2/* xArg  */,  1/* yArg  */,  1/* lArg */,
     11/* pLoop */,  0/* pSubr */,
     0/* xData */,  0/* yData */} /* memory requirements */, COMPUTETIME};

+(MKLeafUGStruct *)classInfo  
{   if (_leafUGStruct.master == NULL)
      _leafUGStruct.master = [self masterUGPtr];
    return &_leafUGStruct; }

+initialize /* Sent once on factory start-up. */
{
enum args { aout, trg, rate, amp };
   static DSPMemorySpace _argSpaces[] = {DSP_MS_X,DSP_MS_N,DSP_MS_N,DSP_MS_N};
   static DSPDataRecord _dataRecP = {NULL, DSP_LC_P, 0, 1, 11}; 
   static int _dataP[] = {0x66d800,0xf19800,0x48e200,0x21c5c2,0x2000f6,
                          0x60f80,0x88,0x21c5c2,0x455ef6,0x565e00,
                          0x485a00};
   static DSPFixup _fixupsP[] = {
   {DSP_LC_P, NULL, 1 /* decrement */, 6 /* refOffset */,  8 /* relAddress */}
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
