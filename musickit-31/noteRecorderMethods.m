-(MKTimeUnit)timeUnit
  /* TYPE: Querying; Returns the receiver's recording mode.
   * Returns YES if the receiver is set to do post-tempo recording.
   */
{
    return timeUnit;
}

-setTimeUnit:(MKTimeUnit)aTimeUnit
  /* TYPE: Modifying; Sets the receiver's recording mode to evaluate tempo.
   * Sets the receiver's realization recording mode to do post-tempo recording.
   * Returns the receiver.
   * Illegal while the receiver is active. Returns nil in this case, otherwise
   * self.
   */
{
    if ([self inPerformance] && (timeUnit != aTimeUnit))
      return nil;
    timeUnit = aTimeUnit;
    return self;
}
























































































