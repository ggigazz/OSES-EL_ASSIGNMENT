/*
 * Copyright (C) Your copyright.
 *
 * Author: AC
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the PG_ORGANIZATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY	THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS-IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#define q	11		    /* for 2^11 points */
#define N	(1<<q)		/* N-point FFT, iFFT */
#define NTHREAD 2048

typedef float real;
typedef struct{real Re; real Im;} complex;
complex v[N];

#ifndef PI
# define PI	3.14159265358979323846264338327950288
#endif

struct paramRead{
	int fd;
	int buffer;
	int index;
};


void fft( complex *v, int n, complex *tmp )
{
  if(n>1) {			/* otherwise, do nothing and return */
    int k,m;    complex z, w, *vo, *ve;
    ve = tmp; vo = tmp+n/2;
    for(k=0; k<n/2; k++) {
      ve[k] = v[2*k];
      vo[k] = v[2*k+1];
    }
    fft( ve, n/2, v );		/* FFT on even-indexed elements of v[] */
    fft( vo, n/2, v );		/* FFT on odd-indexed elements of v[] */
    for(m=0; m<n/2; m++) {
      w.Re = cos(2*PI*m/(double)n);
      w.Im = -sin(2*PI*m/(double)n);
      z.Re = w.Re*vo[m].Re - w.Im*vo[m].Im;	/* Re(w*vo[m]) */
      z.Im = w.Re*vo[m].Im + w.Im*vo[m].Re;	/* Im(w*vo[m]) */
      v[  m  ].Re = ve[m].Re + z.Re;
      v[  m  ].Im = ve[m].Im + z.Im;
      v[m+n/2].Re = ve[m].Re - z.Re;
      v[m+n/2].Im = ve[m].Im - z.Im;
    }
  }
  return;
}

void calculation(complex *v){

	complex scratch[N];
	float abs[N];
	int k;
	int m;
	int i;
	int minIdx, maxIdx;

	// FFT computation
	  fft( v, N, scratch );

	// PSD computation
	  for(k=0; k<N; k++) {
		abs[k] = (50.0/2048)*((v[k].Re*v[k].Re)+(v[k].Im*v[k].Im));
	  }

	  minIdx = (0.5*2048)/50;   // position in the PSD of the spectral line corresponding to 30 bpm
	  maxIdx = 3*2048/50;       // position in the PSD of the spectral line corresponding to 180 bpm

	// Find the peak in the PSD from 30 bpm to 180 bpm
	  m = minIdx;
	  for(k=minIdx; k<(maxIdx); k++) {
	    if( abs[k] > abs[m] )
		m = k;
	  }

	// Print the heart beat in bpm
	  printf( "\n\n\n%d bpm\n\n\n", (m)*60*50/2048 );

}

void * calculation_wrapper (){
	complex vett[N];
	int i;
	for (i=0; i<NTHREAD; i++){
		vett[i].Re=v[i].Re;
		vett[i].Im=v[i].Im;
	}
	printf("\n\nStart Calculation\n\n");
	calculation(vett);
	pthread_exit(NULL);
}

void * read_wrapper(void* rp){
	read(((struct paramRead *)rp)->fd, &(((struct paramRead *)rp)->buffer), sizeof(int));
//	printf("Read nr %d value %d ", ((struct paramRead *)rp)->index, ((struct paramRead *)rp)->buffer);
	v[((struct paramRead *)rp)->index].Re = ((struct paramRead *)rp)->buffer;
	v[((struct paramRead *)rp)->index].Im = 0;
//	printf("Valore scritto %f %f\n\n", v[((struct paramRead *)rp)->index].Re, v[((struct paramRead *)rp)->index].Im);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
  pthread_t threadRead[NTHREAD], threadSum;
  char *app_name = argv[0];
  char *dev_name = "/dev/heartbeatmod";
  int fd = -1, k, i = 0;
  char c;
  struct paramRead rp;

  if ((fd = open(dev_name, O_RDWR)) < 0) {
  	fprintf(stderr, "%s: unable to open %s: %s\n", app_name, dev_name, strerror(errno));
  	return( 1 );
  }

// Initialize struct for reading
  rp.fd = fd;
  rp.buffer = 0;

  //Initialize name of the thread
  for ( i = 0; i < NTHREAD; i++)
  		  threadRead[i] = i;

  while(1){
	  for ( i = 0; i < NTHREAD; i++){
		  rp.index = i;
		  pthread_create(&threadRead[i], NULL, read_wrapper, (void *) &rp);
		  pthread_detach(threadRead[i]);
		  usleep(20000);
	  }
	  pthread_create(&threadSum, NULL, calculation_wrapper, NULL);
	  pthread_detach(threadSum);

  }
  close( fd );
  exit(EXIT_SUCCESS);
}
