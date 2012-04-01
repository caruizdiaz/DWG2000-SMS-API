/*
 * ip_socket.c
 *
 *  Created on: Nov 22, 2011
 *      Author: caruizdiaz
 */

#include "ip_socket.h"
#include "../util.h"

static void *bytes_listener(void *param);
static void *string_listener(void *param);
static void *async_processing(void *param);

pthread_t *ip_start_listener(int port, dflt_func_ptr_t callback, sock_direction_t direction)
{
	int opt = 1, sock_fd = 0, offset = 0;
	struct sockaddr_in server;
	pthread_t *listener_thread	= NULL;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		LOG(L_ERROR, "%s | socket(): %s", __FUNCTION__, strerror(errno));
		return NULL;
	}

	setsockopt (sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	server.sin_family 		= AF_INET;
	server.sin_port 		= htons(port);
	server.sin_addr.s_addr 	= INADDR_ANY;

	bzero(&(server.sin_zero), 8);

	if(bind(sock_fd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1)
	{
		LOG(L_ERROR, "%s | bind(): %s\n", __FUNCTION__, strerror(errno));
		return NULL;
	}

	if(listen(sock_fd, 5000) == -1)
	{
		LOG(L_ERROR, "%s: %s\n", __FUNCTION__, strerror(errno));
		return NULL;
	}

	listener_thread		= malloc(sizeof(pthread_t));

	/*
	 * serialize parameters to pass to the listener thread. The memory must be dyn-allocated to guarantee
	 * the value received on thread is first accessed before the calling function ( the thread's creator)
	 * returns
	 */
	char *param	=	 malloc(sizeof(int) + sizeof(dflt_func_ptr_t) + sizeof(sock_direction_t));

	memcpy(param, &sock_fd, sizeof(int));
	offset	+= sizeof(int);
	memcpy(&param[offset], &callback, sizeof(dflt_func_ptr_t));
	offset	+= sizeof(dflt_func_ptr_t);

	if (pthread_create(listener_thread, NULL, string_listener, (void *) param) != 0)
	{
		LOG(L_ERROR, "%s | pthread_create(): %s\n", __FUNCTION__, strerror(errno));
		return NULL;
	}

	return listener_thread;
}

void ip_stop_listener(pthread_t *listener_thread)
{
	if (listener_thread != NULL)
		pthread_kill(listener_thread, 2);
}

static void *string_listener(void *param)
{
	char *params					= param;
	struct sockaddr_in client;
	_uint sin_size					= 0;
	int client_fd, sock_fd, offset 	= 0;
	dflt_func_ptr_t callback;

	memcpy(&sock_fd, params, sizeof(int));
	offset	+= sizeof(int);
	memcpy(&callback, &params[offset], sizeof(dflt_func_ptr_t));
	offset	+= sizeof(dflt_func_ptr_t);

	free(param);

	setbuf(stdout, NULL);

	for(;;)
	{
		char buffer[BUFFER_SIZE];
		int bytes_read 		= 0;
		pthread_t thread;

		sin_size = sizeof(struct sockaddr_in);

		if ((client_fd = accept(sock_fd, (struct sockaddr *) &client, &sin_size)) == -1)
		{
			LOG(L_ERROR, "%s | accept(): %s\n", __FUNCTION__, strerror(errno));
		    continue;
		}

		LOG(L_DEBUG, "Connection from %s\n", inet_ntoa(client.sin_addr));

		int *sock_df_param	= malloc(sizeof(int));
		*sock_df_param		= client_fd;

		if (pthread_create(&thread, NULL, callback, (void *) sock_df_param) != 0)
		{
			LOG(L_DEBUG, "%s: Error creating thread for connection\n",__FUNCTION__);
			close(client_fd);
		}
	}

	return NULL;
}
