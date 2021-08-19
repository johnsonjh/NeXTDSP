#import "Conductor.h"

#import	<sys/message.h>
#import	<dpsclient/dpsclient.h>
#import <dpsclient/dpsNeXT.h>

extern void _MKLock(void) ;
    /* Waits for Music Kit to become available for messaging. */
extern void _MKUnlock(void) ;
    /* Gives up lock so that Music Kit can run again. */

extern void _MKAddPort(port_name_t aPort, 
		       DPSPortProc aHandler,
		       unsigned max_msg_size, 
		       void *anArg,int priority);
extern void _MKRemovePort(port_name_t aPort);

@interface Conductor(Private)

+(MKMsgStruct *)_afterPerformanceSel:(SEL)aSelector 
 to:(id)toObject 
 argCount:(int)argCount, ...;
+(MKMsgStruct *)_newMsgRequestAtTime:(double)timeOfMsg
  sel:(SEL)whichSelector to:(id)destinationObject
  argCount:(int)argCount, ...;
+(void)_scheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr;
+(MKMsgStruct *)_cancelMsgRequest:(MKMsgStruct *)aMsgStructPtr;
+(double)_adjustTimeNoTE:(double)desiredTime ;
+(double)_getLastTime;
+_adjustDeltaTThresholds;
-(void)_scheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr;
-(MKMsgStruct *)_rescheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr
  atTime:(double)timeOfNewMsg sel:(SEL)whichSelector
  to:(id)destinationObject argCount:(int)argCount, ...;

@end


