#include <complex.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <immintrin.h>
#include "mul.h"
#include "fft/sr_vector.h"
#include "fft/fftw.h"
#include "fft/sr_vec_nonrec.h"

cplx_ptr vec_x,vec_y,vec_res;
cplx_ptr vctr_x,vctr_y,vctr_res;

/******************************************************************
*
* SUPPORT CODE
*
******************************************************************/
void print_cplx(const cplx_ptr *x,int N){
  for (int i = 0; i < N; ++i)
    printf("cplxpoly[%d] = %f + i * %f\n",i,x->real[i],x->imag[i]);
  printf("\n");
}

/******************************************************************
*
* CONVERSION
*
******************************************************************/
void to_complex(const ring_t *x, double complex *cplx_x)
{
  for(int i=0;i<CPLXDIM;++i)
    cplx_x[i] = x->v[i] + I*x->v[i+CPLXDIM];
}

void to_real(const double complex *cplx_x, ring_t *x)
{
  for(int i=0;i<CPLXDIM;++i){
    x->v[i] = round(creal(cplx_x[i]));
    x->v[i+CPLXDIM] = round(cimag(cplx_x[i]));
  }
}

void init(){
  //PRECOMP TABLES FOR VECTOR FFT
  init_table_vctr();
  //PRECOMP FFTW
  FFTsetup();
  posix_memalign((void**)&vec_x.real,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vec_x.imag,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vec_y.real,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vec_y.imag,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vec_res.real,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vec_res.imag,32, CPLXDIM * sizeof(double));

  init_vctr();
  posix_memalign((void**)&vctr_x.real,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vctr_x.imag,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vctr_y.real,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vctr_y.imag,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vctr_res.real,32, CPLXDIM * sizeof(double));
  posix_memalign((void**)&vctr_res.imag,32, CPLXDIM * sizeof(double));
}

/******************************************************************
*
* FFTW MULTIPLICATION
*
******************************************************************/
void fftw_mul(ring_t *r, const ring_t *x, const ring_t *y){
  double complex cplx_x[CPLXDIM+1];
  double complex cplx_y[CPLXDIM+1];
  double complex cplx_res[CPLXDIM+1];

  FFTWforward(cplx_x,x);
  FFTWforward(cplx_y,y);

  for (int i = 0; i < CPLXDIM+1; ++i)
  {
    cplx_res[i] = cplx_x[i] * cplx_y[i];
  }
  FFTWbackward(r,cplx_res);
}
/******************************************************************
*
* SPLIT RADIX PRECOMPUTED AND VECTORIZED NON RECURSIVE FFT MULTIPLICATION
*
******************************************************************/
void sr_vector_nonrec_mul(ring_t *r, const ring_t *x, const ring_t *y){
  int j = CPLXDIM;
  for (int i = 0; i < CPLXDIM; ++i)
  {
    vctr_x.real[i] = x->v[i];
    vctr_x.imag[i] = x->v[j];
    
    vctr_y.real[i] = y->v[i];
    vctr_y.imag[i] = y->v[j]; 
    ++j;
  }

  fft_vector_nonrec_forward(&vctr_x);
  fft_vector_nonrec_forward(&vctr_y);
    __m256d real_x,imag_x,real_y,imag_y,imag_temp,real_temp;
  // double a,b,c,d;
  for (int i = 0; i < CPLXDIM; i+=4)
  {
    real_x = _mm256_load_pd(vctr_x.real+i);
    imag_x = _mm256_load_pd(vctr_x.imag+i);
    real_y = _mm256_load_pd(vctr_y.real+i);
    imag_y = _mm256_load_pd(vctr_y.imag+i);

    //(a + ib) * (c + id) = (ac - bd) + i(ad+bc)
    //real_temp = bd
    real_temp = _mm256_mul_pd(imag_x,imag_y);
    //imag_temp = ad
    imag_temp = _mm256_mul_pd(real_x,imag_y);
    //REPLACED FOR COMMENTED SECTION
    //real_x = ac
    real_x = _mm256_mul_pd(real_x,real_y);
    //imag_x = bc
    imag_x = _mm256_mul_pd(imag_x,real_y);
    //real_x = ac - bd => real_x - real_temp
    real_x = _mm256_sub_pd(real_x,real_temp);
    //imag_x = ad + bc => imag_temp + imag_x
    imag_x = _mm256_add_pd(imag_x,imag_temp);
    //THESE ARE NOT WORKING 
    // real_x = _mm256_fmsub_pd(real_x,real_y,real_temp);
    // imag_x = _mm256_fmadd_pd(imag_x,real_y,imag_temp);
    //WE WANT TO DIVIDE EVERYTHING BY CPLXDIM
    real_y = _mm256_setr_pd(CPLXDIM,CPLXDIM,CPLXDIM,CPLXDIM);
    real_x = _mm256_div_pd(real_x,real_y);
    imag_x = _mm256_div_pd(imag_x,real_y);

    _mm256_store_pd(vctr_res.real+i,real_x);
    _mm256_store_pd(vctr_res.imag+i,imag_x);

    // a = vec_x.real[i];
    // b = vec_x.imag[i]; 
    // c = vec_y.real[i];
    // d = vec_y.imag[i];
    // //(a + ib) * (c + id) = (ac - bd) + i(ad+bc)
    // vec_res.real[i] = ((a*c) - (b*d))/CPLXDIM;
    // vec_res.imag[i] = ((a*d) + (b*c))/CPLXDIM;
  }
  fft_vector_nonrec_backward(&vctr_res,r);
}
/******************************************************************
*
* SPLIT RADIX PRECOMPUTED AND VECTORIZED FFT MULTIPLICATION
*
******************************************************************/
void sr_vector_mul(ring_t *r, const ring_t *x, const ring_t *y){
  // printf("\n\n**************split-radix FAST**************\n");
  int j = CPLXDIM;
  for (int i = 0; i < CPLXDIM; ++i)
  {
    vec_x.real[i] = x->v[i];
    vec_x.imag[i] = x->v[j];
    
    vec_y.real[i] = y->v[i];
    vec_y.imag[i] = y->v[j]; 
    ++j;
  }

  fft_vector_forward(&vec_x);
  fft_vector_forward(&vec_y);
  
  __m256d real_x,imag_x,real_y,imag_y,imag_temp,real_temp;
  // double a,b,c,d;
  for (int i = 0; i < CPLXDIM; i+=4)
  {
    real_x = _mm256_load_pd(vec_x.real+i);
    imag_x = _mm256_load_pd(vec_x.imag+i);
    real_y = _mm256_load_pd(vec_y.real+i);
    imag_y = _mm256_load_pd(vec_y.imag+i);

    //(a + ib) * (c + id) = (ac - bd) + i(ad+bc)
    //real_temp = bd
    real_temp = _mm256_mul_pd(imag_x,imag_y);
    //imag_temp = ad
    imag_temp = _mm256_mul_pd(real_x,imag_y);
    //REPLACED FOR COMMENTED SECTION
    //real_x = ac
    real_x = _mm256_mul_pd(real_x,real_y);
    //imag_x = bc
    imag_x = _mm256_mul_pd(imag_x,real_y);
    //real_x = ac - bd => real_x - real_temp
    real_x = _mm256_sub_pd(real_x,real_temp);
    //imag_x = ad + bc => imag_temp + imag_x
    imag_x = _mm256_add_pd(imag_x,imag_temp);

    real_y = _mm256_setr_pd(CPLXDIM,CPLXDIM,CPLXDIM,CPLXDIM);
    real_x = _mm256_div_pd(real_x,real_y);
    imag_x = _mm256_div_pd(imag_x,real_y);

    //THESE ARE NOT WORKING 
    // real_x = _mm256_fmsub_pd(real_x,real_y,real_temp);
    // imag_x = _mm256_fmadd_pd(imag_x,real_y,imag_temp);
    _mm256_store_pd(vec_res.real+i,real_x);
    _mm256_store_pd(vec_res.imag+i,imag_x);

    // a = vec_x.real[i];
    // b = vec_x.imag[i]; 
    // c = vec_y.real[i];
    // d = vec_y.imag[i];
    // //(a + ib) * (c + id) = (ac - bd) + i(ad+bc)
    // vec_res.real[i] = ((a*c) - (b*d))/CPLXDIM;
    // vec_res.imag[i] = ((a*d) + (b*c))/CPLXDIM;
  }
  // print_cplx(&vec_res,CPLXDIM);
  fft_vector_backward(&vec_res,r); 

}


/******************************************************************
*
*	NAIVE SCHOOLBOOK MULTIPLICATION
*
******************************************************************/
/* Very simple schoolbook multiplication. Works. */
void naive_real_mul(ring_t *r, const ring_t *x, const ring_t *y)
{
  int i,j;
  for(i=0;i<REALDIM;i++)
    r->v[i] = 0;

  for(i=0;i<REALDIM;i++)
  {
    for(j=0;j<REALDIM;j++)
    {
      if(i+j < REALDIM)
        r->v[i+j] += x->v[i] * y->v[j];
      else
        r->v[i+j-REALDIM] -= x->v[i] * y->v[j];
    }
  }
}