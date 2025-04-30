#ifndef FFT_H
#define FFT_H

struct COMPLEX_STRUCT
{
   double real;
   double imag;
};
typedef struct COMPLEX_STRUCT COMPLEX;

#define TRUE 1
#define FALSE 0

int Powerof2(int nx, int *m, int *twopm);

int FFT(int dir,int m,double *x,double *y);
int FFT2D(COMPLEX *c,int nx,int ny,int dir);

#endif /* FFT_H */
