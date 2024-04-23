CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic -Werror -ggdb

.PHONY: clean

all: main
	
parent: main.c makefile
	$(CC) $(CFLAGS) main.c -o main

clean:
	rm -f main

run:
	./main