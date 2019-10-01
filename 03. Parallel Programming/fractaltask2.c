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
struct task *head = NULL;

struct task
{
    struct bitmap *bm;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int max;
    int y_start;
    int y_end;
    struct task *next;
};

struct task* task_create(struct bitmap *bm, double xcenter, double ycenter, double scale, int max, int y_start, int y_end){
  struct task *t = 0;
  t = malloc(sizeof(struct task));
  t->bm = bm;
  t->xmin = xcenter - scale;
  t->xmax = xcenter + scale;
  t->ymin = ycenter - scale;
  t->ymax = ycenter + scale;
  t->max = max;
  t->y_start = y_start;
  t->y_end =  y_end;
  return t;
}

struct task* task_dequeue(struct task *t){
//mutex
  struct task* hd = t;
  t = t->next;

  return hd;
}


void show_help();
void * compute_image(void * t);
int value_to_color( double complex z, int iter, int max );
static int compute_point( double x, double y, int max );





int main( int argc, char *argv[] )
{
	char c;
	const char *outfile = "fractal.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 640;
	int    image_height = 480;
	int    max = 100;
  int    task_count = 1;
  pthread_t mythread[task_count];
  pthread_mutex_t lock;

  if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return 1;
  }

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:k:h"))!=-1) {
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
      case 'k':
        task_count = atoi(optarg);
        break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}
  printf("fractal: x=%lf y=%lf scale=%lf max=%d outfile=%s\n",xcenter,ycenter,scale,max,outfile);
	struct bitmap *bm = bitmap_create(image_width,image_height);
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

  struct task* t;



  for(int i = 0; i < task_count; i++){

    int y_start = (float)i * (bitmap_height(bm)/task_count); //calculates row start iteratively
		int y_end = (float)(i+1.0)*(bitmap_height(bm)/task_count); //increemnts row start by 1 increment
		if (i == task_count-1) {
			y_end += (bitmap_height(bm) % task_count); //takes care of remainders in space
		}
    t = task_create(bm, xcenter, ycenter, scale, max, y_start, y_end);
    t->next = head;
    printf(%p)
    head = t;

  }

  printf("JESUS\n");
  printf("%d", head->max);

  int ii = 0;
  while(head != NULL){
    pthread_t tid;
    pthread_attr_t attr; //set of thread attributes
		pthread_attr_init(&attr); //get default attributes
		//int em; //error message detector

    printf("thread %d start\n", ii);
    ii++;

    pthread_mutex_lock(&lock);
    pthread_create(&tid, &attr, compute_image, head);
    head = task_dequeue(head);
    mythread[ii] = tid;
    pthread_mutex_unlock(&lock);
  }

  for(int i = 0; i < task_count; i++){
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

/*
Compute an entire image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/
//modified function signature takes void pointer
void * compute_image(void * t) {
	//declares viariables
	struct bitmap *bm;
	double xmin, xmax, ymin, ymax;
	int max,start,end,i,j;

	//assigns variables based on passed opinted to thread
	struct task * a1; //new buffer pointer to args struct
	a1 = (struct task *)t; //typecast as thread_args
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
	printf("Use: fractaltask [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=100)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in complex coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=640)\n");
	printf("-H <pixels> Height of the image in pixels. (default=480)\n");
	printf("-o <file>   Set output file. (default=fractal.bmp)\n");
  printf("-k <tasks>  Set number of tasks the work is divided into. (default=1)\n");
	printf("-h          Show this help text.\n");
}

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
static int compute_point( double x, double y, int max)
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
