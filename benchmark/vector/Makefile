CC=/usr/bin/gcc
CFLAGS=-Wall -Wextra -O3 -std=c11 -mavx2 -march=native 
LIBS=-lm -lfftw3
DEPS = mul.h fft/sr_vector.h fft/fftw.h fft/sr_vec_nonrec.h fft/negacyclic.h
CFILES = mul.c test/test_mul.c fft/sr_vector.c fft/fftw.c fft/sr_vec_nonrec.c fft/negacyclic.c

test/test_mul: $(CFILES) $(DEPS)
	$(CC) $(CFLAGS) $(CFILES) $(LIBS) -o $@

.PHONY: clean

clean:
	-rm test/test_mul
