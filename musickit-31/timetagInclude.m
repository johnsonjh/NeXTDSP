-setFirstTimeTag:(double) aTimeTag
  /* TYPE: Accessing time; Sets \fBfirstTimeTag\fR to \fIaTimeTag\fR
   * Sets the variable \fBfirstTimeTag\fR to \fIaTimeTag\fR and 
   * returns the receiver.
   * Illegal while the receiver is active. Returns nil in this case, else self.
   *
   * \fBfirstTimeTag\fR is used in subclasses such as PartSegment
   * to establish the smallest timeTag value that's considered for
   * performance.  
   */
{ 
    if (status != MK_inactive) 
      return nil;
    firstTimeTag = aTimeTag;
    return self;
}		

-setLastTimeTag:(double) aTimeTag
  /* TYPE: Accessing time; Sets \fBlastTimeTag\fR to \fIaTimeTag\fR
   * Sets the variable \fBlastTimeTag\fR to \fIaTimeTag\fR and 
   * returns the receiver.
   * Illegal while the receiver is active. Returns nil in this case, else self.
   *
   * \fBlastTimeTag\fR is used in subclasses such as PartSegment
   * to establish the greatest timeTag value that's considered for
   * performance.  
   */
{ 
    if (status != MK_inactive) 
      return nil;
    lastTimeTag = aTimeTag;
    return self;
}		


-(double)firstTimeTag 
  /* TYPE: Accessing time; Returns the value of \fBfirstTimeTag\fR.
   * Returns the value of the receiver's \fBfirstTimeTag\fR variable.
   */
{
	return firstTimeTag;
}

-(double)lastTimeTag 
  /* TYPE: Accessing time; Returns the value of \fBlastTimeTag\fR.
   * Returns the value of the receiver's \fBlastTimeTag\fR variable.
   */
{
	return lastTimeTag;
}

