/* Modification history:

   09/26/90/daj & lbj - Changed freeNoteSenders to make sure receiver's not
                        in performance. 
*/

-noteSenders
  /* TYPE: Processing
   * Returns a copy of the receiver's NoteSender List. 
   * It is the sender's responsibility to free the List.
   */
{
    return _MKCopyList(noteSenders);
}

-(BOOL)isNoteSenderPresent:(id)aNoteSender
  /* Returns YES if aNoteSender is a member of the receiver's 
     NoteSender List. */
{
    return ([noteSenders indexOf:aNoteSender] == -1) ? NO : YES;
}

-freeNoteSenders
  /* TYPE: Creating
   * Empties and frees contents of \fBnoteSenders\fR.
   * Returns the receiver.
   */
{
    id aList;
    if ([self inPerformance])
      return nil;
    aList = _MKCopyList(noteSenders);
    [self removeNoteSenders];
    [aList freeObjects];  /* Split this up because elements may try
			     and remove themselves from noteSenders
			     when they are freed. */
    [aList free];
    return self;
}

-removeNoteSenders
  /* Empties noteSenders by repeatedly sending removeNoteSender:
     with each element of the collection as the argument. */
{
    /* Need to use seq because we may be modifying the List. */
    register id *el;
    unsigned n;
    IMP selfRemoveNoteSender;
    if (!noteSenders)
      return self;
    selfRemoveNoteSender = [self methodFor:@selector(removeNoteSender:)];
#   define SELFREMOVENOTESENDER(x)\
    (*selfRemoveNoteSender)(self, @selector(removeNoteSender:), (x))
    for (el = NX_ADDRESS(noteSenders), n = [noteSenders count]; n--; el++)
      SELFREMOVENOTESENDER(*el);
    return self;
}

-noteSender
  /* Returns the default NoteSender. This is used when you don't care
     which NoteSender you get. */
{
    return [noteSenders objectAt:0];
}

























