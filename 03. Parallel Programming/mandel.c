#include "bitmap.h"

#include <pthread.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

struct thread_args {
	struct bitmap *b;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	double itermax;
	int rowStart;
	int rowEnd;
};



int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void * compute_image(void * args);



void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] )
{

//	printf("start\n");
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int    maxThreads = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

//	printf("c0\n");


	//getting of options
	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:n:h"))!=-1) {
//		printf("c1\n");
		switch(c) {
			case 'n':
				maxThreads = atoi(optarg);
				break;
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

//	printf("here\n");

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s\n",xcenter,ycenter,scale,max,outfile);

//	printf("checkpoint\n");
	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);



//	printf("c2\n");
	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	//static char *input[] = {MANDEL,"-x","-1.3689","-y",".005","-s",sstr,"-o",fn,"-m",mstr,NULL};

//	printf("c3\n");
	//int maxThreads = 1;
	int i;

//	float incr = scale*2.0/(float)maxThreads;

	 //thread identifier
	//pthread_attr_t attr; //set of thread attributes
	//pthread_attr_init(&attr); //get default attributes
	//pthread_create(&tid,&attr,compute_image,


//	printf("c4\n");


	//initialize struct array and array of threads
	struct thread_args a0[maxThreads];
	pthread_t workers[maxThreads];


	//increments from 0 to max threads
	for (i = 0; i < maxThreads; i++) {


		//assigns variable values to defaults
		a0[i].b = bm;
		a0[i].xmin = xcenter-scale;
		a0[i].xmax = xcenter+scale;
		a0[i].ymin = ycenter-scale;
		a0[i].ymax = ycenter+scale;
		a0[i].itermax = max;


		a0[i].rowStart = (float)i * (image_height/maxThreads); //calculates row start iteratively
		a0[i].rowEnd = (float)(i+1.0)*(image_height/maxThreads); //increemnts row start by 1 increment
		if (i == maxThreads-1) {
			a0[i].rowEnd += (image_height % maxThreads); //takes care of remainders in space
		}
//		printf("rs = %d, re = %d\n",a0[i].rowStart,a0[i].rowEnd);
		pthread_t tid; //declares thread id

		pthread_attr_t attr; //set of thread attributes
		pthread_attr_init(&attr); //get default attributes
		int em; //error message detector
		em = pthread_create(&tid,&attr,compute_image,(void *)&a0[i]); //creates thread with typecast void pointer
		printf("thread %d started!\n",i);
		if (em != 0) {  //if there is an error
			printf("error\n");
			exit(1); //exits if error
		}
		workers[i] = tid; //sets thread id to workers array

	}

//	printf("c5\n");



//	printf("c6\n");

	//thread joining process
	for (i = 0; i < maxThreads; i++) {
		pthread_join(workers[i], NULL); //joins that worker thread
		printf("thread %d stopped!\n",i);
	}
//	printf("c7\n");

	// Compute the Mandelbrot image
//	compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max);

//	compute_image(args.b,args.xmin,args.xmax,args.ymin,args.ymax,args.itermax);

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		printf("error\n");
		return 1;
	}

//	printf("final\n");
	return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

//void compute_image( struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max )

void * compute_image(void * x) //modified function signature takes void pointer
{
	//declares viariables
	struct bitmap *bm;
	double xmin, xmax, ymin, ymax;
	int max,start,end,i,j;

	//assigns variables based on passed opinted to thread
	struct thread_args * a1; //new buffer pointer to args struct
	a1 = (struct thread_args *)x; //typecast as thread_args
//	struct thread_args *args = args;
	bm = a1->b;
	xmin = a1->xmin;
	xmax = a1->xmax;
	ymin = a1->ymin;
	ymax = a1->ymax;
	max = a1->itermax;
	start = a1->rowStart;
	end = a1->rowEnd;

	int width = bitmap_width(bm);
	int height = bitmap_height(bm);

	// For every pixel in the image...

	for(j=start;j<end;j++) { //iterates from start to end (as passed) as opposed to 0 to height

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;
		//	printf("x = %f, y = %f\n",x,y);

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
		//	if (i > (xmin+xmax)/2) {
		//	printf("start = %d, end = %d\n",start,end);
		//	printf("i = %d, j = %d, iters = %d\n",i,j,iters);
			bitmap_set(bm,i,j,iters);
		//	}
		}
	}

	return 0;
}


/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max )
{
	int gray = 255*i/max;
	return MAKE_RGBA(gray,gray,gray,0);
}
