/* Modification history:

   4/23/90/daj - Flushed instance var and added arg to _pause: 

*/
#import "UnitGenerator.h"

@interface _OrchloopbeginUG : UnitGenerator
{
}
+_setXArgsAddr:(int)xArgsAddr y:(int)yArgsAddr l:(int)lArgsAddr 
 looper:(int)looperWord;
-_unpause;
-_pause:(int)looperWord;
@end



















