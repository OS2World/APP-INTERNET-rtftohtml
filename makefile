# Edit the following two entries for your configuration
LIBDIR=/opt/contrib/etc
BINDIR=/opt/contrib/bin


IDIRS=-ILibs/OTHER -ILibs/h

TRFILES=ansi-gen html-map mac-gen pc-gen pca-gen \
	ansi-sym html-trans mac-sym pc-sym pca-sym Libs/lib/rtf-ctrl 

OBJS= rtftohtml.o htmlout.o transinit.o html-unix.o

LIBS=Libs/OTHER/libtokenscan.a Libs/lib/librtf.a Libs/lib-unix/lib-unix.a


rtftohtml:      libs ${OBJS}
	${CC}   ${CFLAGS} ${OBJS} ${LIBS} -o rtftohtml

all:	bindist srcdist

libs:	
	cd Libs/OTHER ; ${MAKE} ${MFLAGS}
	cd Libs/h ; ${MAKE} ${MFLAGS}
	cd Libs/lib ; ${MAKE} ${MFLAGS}
	cd Libs/lib-unix ; \
		${MAKE} ${MFLAGS} CFLAGS='${CFLAGS} -DLIBDIR=\"${LIBDIR}\"'

rtftohtml.o:	rtftohtml.c rtftohtml.h Libs/h/rtf.h 
	${CC}   ${CFLAGS} -c ${IDIRS} $*.c

html-unix.o:	html-unix.c Libs/h/rtf-unix.h
	${CC}   ${CFLAGS} -c ${IDIRS} $*.c

htmlout.o:	htmlout.c rtftohtml.h Libs/h/rtf.h
	${CC}   ${CFLAGS} -c ${IDIRS} $*.c

transinit.o:	transinit.c rtftohtml.h Libs/h/rtf.h Libs/OTHER/tokenscan.h
	${CC}   ${CFLAGS} -c ${IDIRS} $*.c

install:	rtftohtml ${TRFILES}
	chmod 755 rtftohtml
	chmod 644 ${TRFILES}
	cp rtftohtml ${BINDIR}
	cp ${TRFILES} ${LIBDIR}

clean:	
	cd Libs/h ; ${MAKE} ${MFLAGS} clean
	cd Libs/lib ; ${MAKE} ${MFLAGS} clean
	cd Libs/OTHER ; ${MAKE} ${MFLAGS} clean
	cd Libs/lib-unix ; ${MAKE} ${MFLAGS} clean
	rm -f ${OBJS} rtftohtml 

