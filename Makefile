#
# Makefile
#

CC = gcc

CFLAGS = -Wall -O2 -pthread
LIBS =

all: mandel-fork-sem mandel-fork-shared2Dbuf

## Mandel

mandel-fork-sem: mandel-lib.o mandel-fork-sem.o
	$(CC) $(CFLAGS) -o mandel-fork-sem mandel-lib.o mandel-fork-sem.o $(LIBS)

mandel-fork-shared2Dbuf: mandel-lib.o mandel-fork-shared2Dbuf.o
	$(CC) $(CFLAGS) -o mandel-fork-shared2Dbuf mandel-lib.o mandel-fork-shared2Dbuf.o $(LIBS)

mandel-lib.o: mandel-lib.h mandel-lib.c
	$(CC) $(CFLAGS) -c -o mandel-lib.o mandel-lib.c $(LIBS)

mandel-fork-sem.o: mandel-fork-sem.c
	$(CC) $(CFLAGS) -c -o mandel-fork-sem.o mandel-fork-sem.c $(LIBS)

mandel-fork-shared2Dbuf.o: mandel-fork-shared2Dbuf.c
	$(CC) $(CFLAGS) -c -o mandel-fork-shared2Dbuf.o mandel-fork-shared2Dbuf.c $(LIBS)

clean:
	rm -f *.s *.o mandel-fork-sem mandel-fork-shared2Dbuf