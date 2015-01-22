CC = gcc
INCLUDES = -I . -I ./includes
CFLAGS = -g -O2 -Wall ${INCLUDES} -fno-strict-aliasing
CLEANFILES = *.o
LIBS = -lmysqlclient -lz -lconfuse

OBJS = main.o logg.o listener.o sock_ntop.o mydhcp.o dhcp_route.o dhcp_db.o

all:	moondhcp clean

moondhcp:	${OBJS}
		${CC} ${CFLAGS} -o $@ ${OBJS} ${LIBS}
	
clean:
	rm -f ${CLEANFILES}