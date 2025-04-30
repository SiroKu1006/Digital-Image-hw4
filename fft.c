#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include"fft.h"

/*-------------------------------------------------------------------------
   Perform a 2D FFT inplace given a complex 2D array
   The direction dir, 1 for forward, -1 for reverse
   The size of the array (nx,ny)
   Return false if there are memory problems or
      the dimensions are not powers of 2
*/
int FFT2D(COMPLEX *c,int nx,int ny,int dir)
{
   int i,j;
   int m,twopm;
   double *real,*imag;
   COMPLEX *p;

   /* Transform the rows */
   real = (double *)malloc(nx * sizeof(double));
   imag = (double *)malloc(nx * sizeof(double));
   if (real == NULL || imag == NULL)
      return(FALSE);
   if (!Powerof2(nx,&m,&twopm) || twopm != nx)
      return(FALSE);
   for (j=0;j<ny;j++) {
	  p = c + j;	// new pointer
	  for (i=0;i<nx;i++) {
         real[i] = p->real; //   c[i][j].real;
         imag[i] = p->imag; //   c[i][j].imag;
		 p = p + nx;	// new pointer
//printf("row, forward, %f, %f\n", real[i], imag[i]);
      }
      FFT(dir,m,real,imag);
	  p = c + j;	// new pointer
	  for (i=0;i<nx;i++) {
         p->real = real[i];	// c[i][j].real
         p->imag = imag[i]; // c[i][j].imag
//printf("row, inverse, %f, %f\n", real[i], imag[i]);
		 p = p + nx;	// new pointer
      }
   }
   free(real);
   free(imag);

   /* Transform the columns */
   real = (double *)malloc(ny * sizeof(double));
   imag = (double *)malloc(ny * sizeof(double));
   if (real == NULL || imag == NULL)
      return(FALSE);
   if (!Powerof2(ny,&m,&twopm) || twopm != ny)
      return(FALSE);
   for (i=0;i<nx;i++) {
	  p = c + (i*nx);	// new pointer
      for (j=0;j<ny;j++) {
         real[j] = p->real; //   c[i][j].real;
         imag[j] = p->imag; //   c[i][j].imag;
		 p++;	// new pointer
//printf("col, forward, %f, %f\n", real[j], imag[j]);
      }
      FFT(dir,m,real,imag);
	  p = c + (i*nx);	// new pointer
      for (j=0;j<ny;j++) {
         p->real = real[j];	// c[i][j].real
         p->imag = imag[j]; // c[i][j].imag
		 p++;	// new pointer
//printf("col, forward, %f, %f\n", real[j], imag[j]);
      }
   }
   free(real);
   free(imag);

   return(TRUE);
}

/*-------------------------------------------------------------------------
   This computes an in-place complex-to-complex FFT
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform

     Formula: forward
                  N-1
                  ---
              1   \          - j k 2 pi n / N
      X(n) = ---   >   x(k) e                    = forward transform
              N   /                                n=0..N-1
                  ---
                  k=0

      Formula: reverse
                  N-1
                  ---
                  \          j k 2 pi n / N
      X(n) =       >   x(k) e                    = forward transform
                  /                                n=0..N-1
                  ---
                  k=0
*/
int FFT(int dir,int m,double *x,double *y)
{
   long nn,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   nn = 1;
   for (i=0;i<m;i++)
      nn *= 2;

   /* Do the bit reversal */
   i2 = nn >> 1;
   j = 0;
   for (i=0;i<nn-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0;
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<nn;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1;
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1)
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<nn;i++) {
         x[i] /= (double)nn;
         y[i] /= (double)nn;
      }
   }

   return(TRUE);
}

int Powerof2(int nx, int *m, int *twopm)
{
	int i, r;
	if (nx <= 0)
		return FALSE;
	//
	r = nx;
	i = 0;
	while (r > 1)
	{
		r = r/2;
		i++;
	}
	*m = i;
	//
	r = 1;
	for (i=1; i <= *m; i++)
		r = r*2;
	*twopm = r;
	if (nx == r)
		return TRUE;
	else
		return FALSE;
}
