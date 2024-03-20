CC = gcc
CFLAGS_DEBUG = -W -Wall -Wextra -std=c11 -g
CFLAGS_RELEASE = -W -Wall -Wextra -std=c11 -O2
.PHONY: clean all
MODE ?= release
ifeq ($(MODE), release)
	CFLAGS = $(CFLAGS_DEBUG)
else
	CFLAGS = $(CFLAGS_RELEASE)
endif
all: dirwalk
dirwalk: dirwalk.c Makefile
	$(CC) $(CFLAGS) dirwalk.c -o dirwalk
clean:
	rm -f dirwalk
