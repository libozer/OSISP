CC = gcc
CFLAGS = -pthread -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic -Werror -ggdb

.PHONY: clean

all: main

main: main.c
	$(CC) $(CFLAGS) main.c -o main

clean:
	rm -f main