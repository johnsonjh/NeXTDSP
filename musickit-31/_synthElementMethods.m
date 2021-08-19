/* Modification history:

   04/21/90/daj - Changed _addShardSynthClaim to be void type
*/

-(MKOrchMemStruct *)_setSynthPatch:aSynthPatch     
  /* Private method used by SynthPatch to add the receiver to itself. */
{
    synthPatch = aSynthPatch;
    return [self _resources];
}

-(void)_setShared:aSharedKey
  /* makes object shared. If aSharedKey is nil, makes it unshared.
     Private method. */
{
    _sharedKey = aSharedKey;
}

-(void)_addSharedSynthClaim
  /* increment ref count */
{
    _MKAddSharedSynthClaim(_sharedKey);
}


