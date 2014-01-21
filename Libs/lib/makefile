INCLUDES=../h/rtf.h ../h/stdcharnames.h ../OTHER/tokenscan.h

all:	librtf.a mkrtf-ctrl rtf-ctrl
CC=gcc
librtf.a:	reader.o
	ar r librtf.a reader.o
	- ranlib librtf.a 

reader.o:	reader.c ${INCLUDES}
	${CC} ${CFLAGS} -c -I../h -I../OTHER $*.c

mkrtf-ctrl:	mkrtf-ctrl.c ${INCLUDES}
	 ${CC} ${CFLAGS} -I../h -I../OTHER mkrtf-ctrl.c -o mkrtf-ctrl

rtf-ctrl:
	./mkrtf-ctrl

rtf-ctl.o:	reader.c ${INCLUDES}
	${CC} ${CFLAGS} -c -I../h -I../OTHER $*.c

clean:
	rm -f librtf.a *.o

