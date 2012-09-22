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

typedef struct listener_data
{
	int sockfd;
	pthread_t thread;

} listener_data_t;

typedef enum sock_direction { DIR_DUAL, DIR_SINGLE } sock_direction_t;

typedef void * (*dflt_func_ptr_t) (void *);
listener_data_t *ip_start_listener(int port, dflt_func_ptr_t callback, sock_direction_t direction);
void ip_stop_listener(listener_data_t *ldata);

#endif /* IP_SOCKET_H_ */
