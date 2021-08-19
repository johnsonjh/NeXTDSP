/*
 * ------------ Performer delegate description
 */

@interface PerformerDelegate : Object
/*
 * The following methods may be implemented by the delegate. The
 * messages get sent, if the delegate responds to them, after the
 * Performer's status has changed.
 */

- performerDidActivate:sender;
- performerDidPause:sender;
- performerDidResume:sender;
- performerDidDeactivate:sender;

@end



