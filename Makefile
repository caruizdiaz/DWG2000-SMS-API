# Makefile to compile dwgsms library
#
# Author: Carlos Ruiz Diaz
#         caruizdiaz.com
#

all: 	dwg.o dwg_server.o ip_socket.o util.o
	gcc dwg.o dwg_server.o ip_socket.o util.o -shared -o "libdwgsms.pub.so" -lpthread

dwg.o: 	dwg/dwg.c	
	gcc -c dwg/dwg.c -o dwg.o

dwg_server.o: dwg/dwg_server.c
	gcc -c dwg/dwg_server.c -o dwg_server.o

ip_socket.o: networking/ip_socket.c
	gcc -c networking/ip_socket.c -o ip_socket.o

util.o:	util.c
	gcc -c util.c -o util.o

clean:
	rm -rf dwg.o dwg_server.o ip_socket.o util.o libdwgsms.pub.so
