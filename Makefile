all: affine

CC=gcc

LIBS=-lm
CFLAGS=-O3 -pipe -s
DEBUGCFLAGS=-Og -pipe -g -Wall -Wextra

INPUT=affine.c
OUTPUT=affine

RM=/bin/rm

.PHONY: affine
affine:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(CFLAGS)

debug:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(DEBUGCFLAGS)

clean:
	if [ -e $(OUTPUT) ]; then $(RM) $(OUTPUT); fi
