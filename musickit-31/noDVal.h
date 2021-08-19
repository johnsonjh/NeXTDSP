/* This is used by the Music Kit to signal "no value" from functions and 
   methods that return double. The value MK_NODVAL is a particular "NaN" 
   ("not a number"). You cannot test its value directly. 
   Instead, use MKIsNoDVal().
   
   Example:
   
   	double myFunction(int arg)
	{
		if (arg == 2)
			return MK_NODVAL;
		else return (double) arg * 2;
	}
  
  	main()
	{
		double d = myFunction(2);
		if (MKIsNoDVal(d))
			printf("Illegal value.\n");
	}
*/
  
#ifndef MK_NODVAL

#define _MK_NANHI 0x7ff80000 /* High bits of a particular non-signaling NaN */
#define _MK_NANLO 0x0        /* Low bits of a particular non-signaling NaN */

#ifdef MK_INLINE 

static inline double MKGetNoDVal(void)
  /* Returns the special NaN that the Music Kit uses to signal "no value". */
{
	union {double d; int i[2];} u;
	u.i[0] = _MK_NANHI;
	u.i[1] = _MK_NANLO;
	return u.d;
}

static inline int MKIsNoDVal(double val)
  /* Compares val to see if it is the special NaN that the Music Kit uses
     to signal "no value". */
{
	union {double d; int i[2];} u;
	u.d = val;
	return (u.i[0] == _MK_NANHI); /* Don't bother to check low bits. */
}
#else

extern double MKGetNoDVal(void);
extern int MKIsNoDVal(double val);

#endif

#define MK_NODVAL MKGetNoDVal()     /* For convenience */

#endif


