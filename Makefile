CC = gcc
CFLAGS = -flto -O3 -Wall -Wextra
LIB = la.h -lm

.PHONY: clean

all: sphere ball

sphere: sphere.c
	$(CC) $(CFLAGS) $(LIB) -o $@ $< 

ball: ball.c
	$(CC) $(CFLAGS) $(LIB) -o $@ $<

clean:
	rm -rf sphere ball
