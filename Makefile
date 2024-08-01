CC = gcc
CFLAGS = -flto -O3 -Wall -Wextra
LIB = la.h -lm
NC = -lcursesw

.PHONY: clean

all: sphere ball

sphere: sphere.c
	$(CC) $(CFLAGS) $(LIB) $(NC) -o $@ $< 

ball: ball.c
	$(CC) $(CFLAGS) $(LIB) -o $@ $<

clean:
	rm -rf sphere ball
