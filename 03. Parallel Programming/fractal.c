
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <complex.h>

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of max.
You can modify this function to make more interesting colors.
*/

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

void compute_image( struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max )
{
	int i,j;

	int width = bitmap_width(bm);
	int height = bitmap_height(bm);

	// For every pixel in the image...

	for(j=0;j<height;j++) {
		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the color at that point.
			int color = compute_point(x,y,max);

			// Set the pixel in the bitmap.
			bitmap_set(bm,i,j,color);
		}
	}
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

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:h"))!=-1) {
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
	compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max);

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"fractal: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}
