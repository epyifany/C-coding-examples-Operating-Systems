
all: fractal fractalthread fractaltask

fractalthread: fractalthread.o bitmap.o
	gcc fractalthread.o bitmap.o -o fractalthread -lpthread -lm

fractaltask: fractaltask.o bitmap.o
	gcc fractaltask.o bitmap.o -o fractaltask -lpthread -lm

fractal: fractal.o bitmap.o
	gcc fractal.o bitmap.o -o fractal -lpthread -lm

fractal.o: fractal.c
	gcc --std=c99 -Wall -g -c fractal.c -o fractal.o

fractalthread.o: fractalthread.c
	gcc --std=c99 -Wall -g -c fractalthread.c -o fractalthread.o

fractaltask.o: fractaltask.c
	gcc --std=c99 -Wall -g -c fractaltask.c -o fractaltask.o

bitmap.o: bitmap.c
	gcc --std=c99 -Wall -g -c bitmap.c -o bitmap.o

clean:
	rm -f fractal.o bitmap.o fractal fractalthread fractaltask fractalthread.o fractaltask.o
