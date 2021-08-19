/* This overrides the normal free() since there seems to be a bug in it */

static int free_cnt = 0;

fake_free(ptr)
     char *ptr;
{
#ifdef FREE_BUG_HAS_BEEN_FIXED
  free(ptr);
#else
  free_cnt += 1;
#endif
}

