
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

//struct for args to be passed into the threads
struct image_arg
{
    struct bitmap *bm;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int max;
    int y_start;
    int y_end;
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


void * compute_image(void * image) //modified function signature takes void pointer
{
	//declares viariables
	struct bitmap *bm;
	double xmin, xmax, ymin, ymax;
	int max,start,end,i,j;

	//assigns variables based on passed opinted to thread
	struct image_arg * a1; //new buffer pointer to args struct
	a1 = (struct image_arg *)image; //typecast as thread_args
//	struct thread_args *args = args;
	bm = a1->bm;
	xmin = a1->xmin;
	xmax = a1->xmax;
	ymin = a1->ymin;
	ymax = a1->ymax;
	max = a1->max;
	start = a1->y_start;
	end = a1->y_end;

  int width = bitmap_width(bm);
	int height = bitmap_height(bm);

  for(j=start;j<end;j++) { //iterates from start to end (as passed) as opposed to 0 to height

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;
		//	printf("x = %f, y = %f\n",x,y);

    // Compute the color at that point.
    int color = compute_point(x, y, max);

    // Set the pixel in the bitmap.
    bitmap_set(bm, i, j, color);
		}
	}

	return 0;
}

void show_help()
{
	printf("Use: fractalthread [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=100)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in complex coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=640)\n");
	printf("-H <pixels> Height of the image in pixels. (default=480)\n");
	printf("-o <file>   Set output file. (default=fractal.bmp)\n");
  printf("-n <threads>Set number of threads to compute the image. (default=1)\n");
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

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Compute the fractal image
	// compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max, y_start, y_end);
  pthread_t mythread[thread_count];

  for(int i = 0; i < thread_count; i++){
    
    //divide the image into different rows
    int y_start = (float)i * (bitmap_height(bm)/thread_count); //calculates row start iteratively
		int y_end = (float)(i+1.0)*(bitmap_height(bm)/thread_count); //increemnts row start by 1 increment
		if (i == thread_count-1) {
			y_end += (bitmap_height(bm) % thread_count); //takes care of remainders in space
		}

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
    pthread_attr_t attr; //set of thread attributes
		pthread_attr_init(&attr); //get default attributes
		//int em; //error message detector

    printf("thread %d start\n", i);
    //pass multiple threads to functions
    pthread_create(&tid, &attr, compute_image, image);
    mythread[i] = tid;
  }


  //join multiple threads
  for(int i = 0; i < thread_count; i++){
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
