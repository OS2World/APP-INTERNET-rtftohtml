INCLUDES=../h/rtf-unix.h
CC=gcc
lib-unix.a:	fileops.o
	ar r lib-unix.a fileops.o
	- ranlib lib-unix.a 

fileops.o:	fileops.c ${INCLUDES}
	${CC} ${CFLAGS} -c -I../h -I../OTHER $*.c

clean:
	rm -f lib-unix.a *.o
