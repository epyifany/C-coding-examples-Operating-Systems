
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <pthread.h>

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of max.
You can modify this function to make more interesting colors.
*/


struct image_arg
{
    struct bitmap *bm;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int max;
    double y_start;
    double y_end;
};

int value_to_color( double complex z, int iter, int max )
{
  int gray = iter * 255 / max;
  return MAKE_RGBA(gray,gray,gray,0);
}

/*
Compute the number of iterations at point x, y
in the complex space, up to a maximum of max.
Return the color of the pixel at that point.

This example computes the Newton fractal:
z = z - alpha * p(z) / p'(z)

Assuming alpha = 1+i and p(z) = z^2 - 1 .

You can modify this function to match your selected fractal computation.
https://en.wikipedia.org/wiki/Newton_fractal
*/

static int compute_point( double x, double y, int max )
{
	// The initial value of z is given by x and y:
	double complex z = x + I*y;
	double complex alpha = 1 + I;

	int iter = 0;

	// Stop if magnitude too large or iterations exceed max
	while( cabs(z)<4 && iter < max ) {

		// Compute the new value of z from the previous:
		z = z - alpha*((cpow(z,2) - 1 ) / (2*z));
		iter++;
	}

	return value_to_color(z,iter,max);
}

/*
Compute an entire image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/



void compute_image(struct image_arg* image)
{
	int i,j;
	int width = bitmap_width(image->bm);
	int height = bitmap_height(image->bm);

  printf("x min %f x max %f \n", image-> xmin, image-> xmax);
  printf("ymin %f ymax %f \n", image-> ymin, image-> ymax);
  printf("max %d \n", image-> max);
  printf("%f %f \n", image-> y_start, image-> y_end);

	// For every pixel in the image...
	for(j = image->y_start; j < image->y_end; j++) {
		for(i = 0; i < width; i++) {



			// Determine the point in x,y space for that pixel.
			double x = image->xmin + i*(image->xmax - image->xmin)/width;
			double y = image->ymin + j*(image->ymax - image->ymin)/height;

			// Compute the color at that point.
			int color = compute_point(x, y, image->max);

			// Set the pixel in the bitmap.
			bitmap_set(image->bm, i, j, color);
		}
	}
  // free(image);

}

void show_help()
{
	printf("Use: fractal [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=100)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in complex coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=640)\n");
	printf("-H <pixels> Height of the image in pixels. (default=480)\n");
	printf("-o <file>   Set output file. (default=fractal.bmp)\n");
  printf("-n <threads>Set number of threads to compute the image. (default=1)\n");
  printf("-k <tasks>  Set number of tasks the work is divided into. (default=1)\n");
	printf("-h          Show this help text.\n");
}

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	const char *outfile = "fractal.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 640;
	int    image_height = 480;
	int    max = 100;
  int    thread_count = 1;

  int    y_start;
  int    y_end;
  // int    return_sig;
  pthread_t mythread[thread_count];
  //void* ret = NULL;

  //WARNING!!!!!!!!!
  //task_count ++;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:n:h"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
      case 'n':
        thread_count = atoi(optarg);
        break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

	// Display the configuration of the image.
	printf("fractal: x=%lf y=%lf scale=%lf max=%d outfile=%s\n",xcenter,ycenter,scale,max,outfile);

	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);
  // y_end = y_end / 3;

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Compute the fractal image
	// compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max, y_start, y_end);


  for(int i = 0; i < thread_count; i++){
    y_start = (float)i * (bitmap_height(bm)/thread_count); //calculates row start iteratively
		y_end = (float)(i+1.0)*(bitmap_height(bm)/thread_count); //increemnts row start by 1 increment
		if (i == thread_count-1) {
			y_end += (bitmap_height(bm) % thread_count); //takes care of remainders in space
		}



    //
    // y_start = i * bitmap_height(bm) / thread_count;
    // if (i != thread_count - 1){
    //   y_end = y_start + bitmap_height(bm) / thread_count;
    // }
    // else{
    //   y_end = bitmap_height(bm);
    // }

    struct image_arg* image = malloc(sizeof(struct image_arg));
    image->bm = bm;
    image->xmin = xcenter - scale;
    image->xmax = xcenter + scale;
    image->ymin = ycenter - scale;
    image->ymax = ycenter + scale;
    image->max = max;
    image->y_start = y_start;
    image->y_end =  y_end;

    pthread_t tid;

    printf("thread %d start\n", i);
    pthread_create(&tid, NULL, (void *) compute_image, (void *) image);
    mythread[i] = tid;
  }

  for(int i = 0; i < thread_count; i++){
    printf("wow\n");
    pthread_join(mythread[i], NULL);
    printf("thread %d end\n", i);
  }

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"fractal: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}
