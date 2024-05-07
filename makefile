# Makefile for client and server

CC = gcc
OBJCS = lab3.c

CFLAGS =  -g -Wall -lm
# setup for system
LIBS =

all: lab3

lab3: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS) 

clean:
	rm lab3