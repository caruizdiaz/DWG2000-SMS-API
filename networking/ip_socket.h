/*
 * ip_socket.h
 *
 *  Created on: Nov 22, 2011
 *      Author: caruizdiaz
 */

#ifndef IP_SOCKET_H_
#define IP_SOCKET_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "../util.h"

#define BUFFER_SIZE 	5000
#define HEADER_SIZE		sizeof(int) * 2

typedef enum sock_direction { DIR_DUAL, DIR_SINGLE } sock_direction_t;

typedef void * (*dflt_func_ptr_t) (void *);
pthread_t *ip_start_listener(int port, dflt_func_ptr_t callback, sock_direction_t direction);
void ip_stop_listener(pthread_t *listener_thread);

#endif /* IP_SOCKET_H_ */
