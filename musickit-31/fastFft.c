/* 
Modification history:

  11/20/89/daj - Changed 1/sqrt(2) to constant
  12/15/89/daj - Changed functions to static
*/


/*
 * fft_real.c
 * Routines for split-radix, real-only transforms.
 * R. E. Crandall, NeXT Educational Technology Center
 * c. 1989 NeXT, Inc.  All Rights Reserved.
 *
 * These routines are adapted from
 * Sorenson, et. al., (1987)
 * I.E.E.E. Trans. Acous. Sp. and Sig. Proc., ASSP-35, 6, 849-863
 *
 * When all x[j] are real the standard DFT of (x[0],x[1],...,x[N-1]),
 * call it x^, has the property of Hermitian symmetry: x^[j] = x^[N-j]*.
 * Thus we only need to find the set
 * (x^[0].re, x^[1].re,..., x^[N/2].re, x^[N/2-1].im, ..., x^[1].im)
 * which, like the original signal x, has N elements.
 * The two key routines perform forward (real-to-Hermitian) FFT,
 * and backward (Hermitian-to-real) FFT, respectively.
 * For example, the sequence:
 *
 * fft_real_to_hermitian(x, N);
 * fftinv_hermitian_to_real(x, N);
 *
 * is an identity operation on the signal x.
 * To convolve two pure-real signals x and y, one goes:
 *
 * fft_real_to_hermitian(x, N);
 * fft_real_to_hermitian(y, N);
 * mul_hermitian(y, x, N);
 * fftinv_hermitian_to_real(x, N);
 *
 * and x is the pure-real cyclic convolution of x and y.
 */



#import<math.h>

#define TWOPI (double)(2*3.14159265358979323846264338327950)

#if 0
void mul_hermitian(double *a, double *b, int n)
/* b becomes b*a in Hermitian representation. */
{
	int k, half = n>>1;
	register double c, d, e, f;
	
	b[0] *= a[0];
	b[half] *= a[half];
	for(k=1;k<half;k++) {
	        c = a[k]; d = b[k]; e = a[n-k]; f = b[n-k];
		b[n-k] = c*f + d*e;
		b[k] = c*d - e*f;
	}
}
#endif

static void scramble_real(double *x, int n)
{	register int i,j,k;
	double tmp;
	for(i=0,j=0;i<n-1;i++) {
		if(i<j) {
	  	tmp = x[j];
	  	x[j]=x[i];
	  	x[i]=tmp;
	  	}
		k = n/2;
		while(k<=j) {
			j -= k;
			k>>=1;
		}
  		j += k;
 	}
}

#if 0
void fft_real_to_hermitian(double *z, int n)
/* Output is {Re(z^[0]),...,Re(z^[n/2),Im(z^[n/2-1]),...,Im(z^[1]).
   This is a decimation-in-time, split-radix algorithm.
 */
{	double *x, e, a, a3, sqrthalf = 1/sqrt((double)2.0);
	double cc1, ss1, cc3, ss3;
	int nn = n>>1, is, id, i0, i1, i2, i3, i4, i5, i6, i7, i8;
	double t1, t2, t3, t4, t5, t6;
	int n2, n4, n8, i, j;
	
	scramble_real(z, n);
	x = z-1;  /* FORTRAN indexing compatibility. */
	is = 1;
	id = 4;
	do{
	   for(i0=is;i0<=n;i0+=id) {
		i1 = i0+1;
		e = x[i0];
		x[i0] = e + x[i1];
		x[i1] = e - x[i1];
	   }
	   is = (id<<1)-1;
	   id <<= 2;
	} while(is<n);
	n2 = 2;
	while(nn>>=1) {
		n2 <<= 1;
		n4 = n2>>2;
		n8 = n2>>3;
		e = TWOPI/n2;
		is = 0;
		id = n2<<1;
		do {
			for(i=is;i<n;i+=id) {
				i1 = i+1;
				i2 = i1 + n4;
				i3 = i2 + n4;
				i4 = i3 + n4;
				t1 = x[i4]+x[i3];
				x[i4] -= x[i3];
				x[i3] = x[i1] - t1;
				x[i1] += t1;
				if(n4==1) continue;
				i1 += n8;
				i2 += n8;
				i3 += n8;
				i4 += n8;
				t1 = (x[i3]+x[i4])*sqrthalf;
				t2 = (x[i3]-x[i4])*sqrthalf;
				x[i4] = x[i2] - t1;
				x[i3] = -x[i2] - t1;
				x[i2] = x[i1] - t2;
				x[i1] += t2;
			}
			is = (id<<1) - n2;
			id <<= 2;
		} while(is<n);
		a = e;
		for(j=2;j<=n8;j++) {
		    	a3 = 3*a;
			cc1 = cos(a);
			ss1 = sin(a);
			cc3 = cos(a3);
			ss3 = sin(a3);
			a = e*j;
			is = 0;
			id = n2<<1;
		        do {
				for(i=is;i<n;i+=id) {
					i1 = i+j;
					i2 = i1 + n4;
					i3 = i2 + n4;
					i4 = i3 + n4;
					i5 = i + n4 - j + 2;
					i6 = i5 + n4;
					i7 = i6 + n4;
					i8 = i7 + n4;
					t1 = x[i3]*cc1 + x[i7]*ss1;
					t2 = x[i7]*cc1 - x[i3]*ss1;
					t3 = x[i4]*cc3 + x[i8]*ss3;
					t4 = x[i8]*cc3 - x[i4]*ss3;
					t5 = t1 + t3;
					t6 = t2 + t4;
					t3 = t1 - t3;
					t4 = t2 - t4;
					t2 = x[i6] + t6;
					x[i3] = t6 - x[i6];
					x[i8] = t2;
					t2 = x[i2] - t3;
					x[i7] = -x[i2] - t3;
					x[i4] = t2;
					t1 = x[i1] + t5;
					x[i6] = x[i1] - t5;
					x[i1] = t1;
					t1 = x[i5] + t4;
					x[i5] -= t4;
					x[i2] = t1;
				}
			        is = (id<<1) - n2;
				id <<= 2;
			} while(is<n);
		}
	}
}
#endif

static void fftinv_hermitian_to_real(double *z, int n)
    /* Input is {Re(z^[0]),...,Re(z^[n/2),Im(z^[n/2-1]),...,Im(z^[1]).
       This is a decimation-in-frequency, split-radix algorithm.
       */
{	
#   define SQRTHALF ((double)0.7071067811865475) /* 1/sqrt((double)2.0) */
    double *x, e, a, a_3;
    double cc1, ss1, cc3, ss3;
    int nn = n>>1, is, id, i0, i1, i2, i3, i4, i5, i6, i7, i8;
    double t1, t2, t3, t4, t5;
    int n2, n4, n8, i, j;
    
    x = z-1;
    n2 = n<<1;
    while(nn >>= 1) {
	is = 0;
	id = n2;
	n2 >>= 1;
	n4 = n2>>2;
	n8 = n4>>1;
	e = TWOPI/n2;
	do {
	    for(i=is;i<n;i+=id) {
		i1 = i+1;
		i2 = i1 + n4;
		i3 = i2 + n4;
		i4 = i3 + n4;
		t1 = x[i1] - x[i3];
		x[i1] += x[i3];
		x[i2] += x[i2];
		x[i3] = t1 - 2.0*x[i4];
		x[i4] = t1 + 2.0*x[i4];
		if(n4==1) continue;
		i1 += n8;
		i2 += n8;
		i3 += n8;
		i4 += n8;
		t1 = (x[i2]-x[i1])*SQRTHALF;
		t2 = (x[i4]+x[i3])*SQRTHALF;
		x[i1] += x[i2];
		x[i2] = x[i4]-x[i3];
		x[i3] = -2.0*(t2+t1);
		x[i4] = 2.0*(t1-t2);
	    }
	    is = (id<<1) - n2;
	    id <<= 2;
	} while(is<n-1);
	a = e;
	for(j=2;j<=n8;j++) {
	    a_3 = 3*a;
	    cc1 = cos(a);
	    ss1 = sin(a);
	    cc3 = cos(a_3);
	    ss3 = sin(a_3);
	    a = e*j;
	    is = 0;
	    id = n2<<1;
	    do {
		for(i=is;i<n;i+=id) {
		    i1 = i+j;
		    i2 = i1+n4;
		    i3 = i2+n4;
		    i4 = i3+n4;
		    i5 = i+n4-j+2;
		    i6 = i5+n4;
		    i7 = i6+n4;
		    i8 = i7+n4;
		    t1 = x[i1] - x[i6];
		    x[i1] += x[i6];
		    t2 = x[i5] - x[i2];
		    x[i5] += x[i2];
		    t3 = x[i8] + x[i3];
		    x[i6] = x[i8] - x[i3];
		    t4 = x[i4] + x[i7];
		    x[i2] = x[i4] - x[i7];
		    t5 = t1 - t4;
		    t1 += t4;
		    t4 = t2 - t3;
		    t2 += t3;
		    x[i3] = t5*cc1 + t4*ss1;
		    x[i7] = -t4*cc1 + t5*ss1;
		    x[i4] = t1*cc3 - t2*ss3;
		    x[i8] = t2*cc3 + t1*ss3;
		}
		is = (id<<1) - n2;
		id <<= 2;
	    } while(is<n-1);
	}
    }
    is = 1;
    id = 4;
    do {
	for(i0=is;i0<=n;i0+=id){
	    i1 = i0+1;
	    e = x[i0];
	    x[i0] = e + x[i1];
	    x[i1] = e - x[i1];
	}
	is = (id<<1) - 1;
	id <<= 2;
    } while(is<n);
    scramble_real(z, n);
    e = 1/(double)n;
    for(i=0;i<n;i++) z[i] *= e;				
}





