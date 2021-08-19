/*
 * ------------ Conductor delegate description
 */

@interface ConductorDelegate : Object
/*
 * The following methods may be implemented by the instance delegate. The
 * messages get sent, if the delegate responds to them, after the
 * Conductor's status has changed.
 */

- conductorDidPause:sender;
- conductorDidResume:sender;

/* These methods may be implemented by the class deleage.  During a clocked
 * conducted performance, the messages get sent, if the delegate responds to 
 * them, when the "delta time" erodes due to heavy computation. 
 */
- conductorCrossedLowDeltaTThreshold;
- conductorCrossedHighDeltaTThreshold;

/* These two functions control the high and low watermark for the delta time
 * notification mechanism. For example, to receive a message when the Conductor
 * has falls behind such that the effective delta time is less than 1/4 of 
 * the value of MKGetDeltaT(), you'd call MKSetLowDeltaTThreshold(.25);  
 * Similarly, to receive a message when the Conductor has recovered such that 
 * the effective delta time is more than 3/4 of the value of MKGetDeltaT(),
 * you'd call MKSetHighDeltaTThreshold(.75);
 */
void MKSetLowDeltaTThreshold(double percentageOfDeltaT);
void MKSetHighDeltaTThreshold(double percentageOfDeltaT);

@end



