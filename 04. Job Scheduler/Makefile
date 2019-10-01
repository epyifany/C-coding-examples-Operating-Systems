FLAGS= -Wall -g -std=gnu99
TARGET = jobsched
LIBRARY= -lpthread -lm

all:		$(TARGET)

default: $(TARGET)

jobsched: jobsched.c jobs.h
	gcc $(TARGET).c $(FLAGS) -o $@ $(LIB)

clean:
	-rm -f jobsched
