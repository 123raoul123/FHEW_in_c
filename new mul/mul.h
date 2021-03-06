#ifndef MUL_H
#define MUL_H

#define CPLXDIM 512
#define REALDIM (2*CPLXDIM)
#define ROOTDIM (2*REALDIM)

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define W(N,k) (cexp(2.0 * M_PI * I * (double)k / (double) N))
#define calc_cos(N,k) (cos(2.0 * M_PI * (double)k / (double) N))
#define calc_sin(N,k) (sin(2.0 * M_PI * (double)k / (double) N))

#include <stdint.h>
#include <complex.h>

typedef struct {
  uint32_t v[REALDIM];
} ring_t;

typedef struct {
	double real[CPLXDIM];
	double imag[CPLXDIM];
} cplx;

typedef struct
{	
	double *real;
	double *imag;
}cplx_ptr;

void print_complex(const double complex *a, int N);
void print_double(const cplx *x,int N);
void print_cplx(const cplx_ptr *x,int N);
//void to_complex(const ring_t *x, double complex *cplx_x);
//void to_real(const double complex *cplx_x, ring_t *x);


void init();
// void test(ring_t *r, const ring_t *x, const ring_t *y);
void fftw_mul(ring_t *r, const ring_t *x, const ring_t *y);
void naive_real_mul(ring_t *r, const ring_t *x, const ring_t *y);
void naive_complex_mul(ring_t *r, const ring_t *x, const ring_t *y);
void split_radix_mul(ring_t *r, const ring_t *x, const ring_t *y);
void normal_fft_mul(ring_t *r, const ring_t *x, const ring_t *y);
void sr_precomp_mul(ring_t *r, const ring_t *x, const ring_t *y);
void sr_vector_mul(ring_t *r, const ring_t *x, const ring_t *y);
void sr_vector_nonrec_mul(ring_t *r, const ring_t *x, const ring_t *y);
void twisted_mul(ring_t *r, const ring_t *x, const ring_t *y);
void negacyc_lut_fft_mul(ring_t *r, const ring_t *x, const ring_t *y);

#endif