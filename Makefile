# Makefile to compile dwgsms library
#
# Author: Carlos Ruiz Diaz
#         caruizdiaz.com
#

CFLAGS := -fPIC


all: 	dwg.o dwg_server.o ip_socket.o util.o dwg_charset.o
	gcc dwg.o dwg_server.o ip_socket.o util.o dwg_charset.o $(CFLAGS) -shared -o "libdwgsms.pub.so" -lpthread

dwg.o: 	dwg/dwg.c	
	gcc -c dwg/dwg.c -o dwg.o $(CFLAGS)

dwg_charset.o: dwg/dwg_charset.c
	gcc -c dwg/dwg_charset.c -o dwg_charset.o $(CFLAGS)

dwg_server.o: dwg/dwg_server.c
	gcc -c dwg/dwg_server.c -o dwg_server.o $(CFLAGS)

ip_socket.o: networking/ip_socket.c
	gcc -c networking/ip_socket.c -o ip_socket.o $(CFLAGS)

util.o:	util.c
	gcc -c util.c -o util.o $(CFLAGS)

clean:
	rm -rf dwg.o dwg_server.o ip_socket.o util.o libdwgsms.pub.so dwg_charset.o

install:
	cp -f libdwgsms.pub.so /usr/lib/

