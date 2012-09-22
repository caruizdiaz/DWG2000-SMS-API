/*
 * dwg_server.c
 *
 *  Created on: Apr 5, 2012
 *      Author: caruizdiaz
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>

#define BUFFER_SIZE 	5000

#include "dwg_server.h"
#include "clist.h"
#include "../util.h"
#include "../networking/ip_socket.h"

/*typedef struct sms_wait_queue
{
	int socket_fd;
	sms_t *sms;
	pthread_mutex_t lock;
} sms_wait_queue_t; */

typedef struct sms_outqueue
{
	struct sms_outqueue *next;
	struct sms_outqueue *prev;

	sms_t *sms;
	int gw_port;

} sms_outqueue_t;

static sms_outqueue_t *_sms_queue;
static pthread_mutex_t _mutex	= PTHREAD_MUTEX_INITIALIZER;
static int _socket_fd			= -1;

void dwg_initilize_server()
{
	_sms_queue	= malloc(sizeof(sms_outqueue_t));
	clist_init(_sms_queue, next, prev);
}

void dwg_server_write_to_queue(sms_t *sms)
{
	pthread_mutex_lock(&_mutex);

	sms_outqueue_t *item	= malloc(sizeof(sms_outqueue_t));
	clist_append(_sms_queue, item, next, prev);

	pthread_mutex_unlock(&_mutex);
}

void *dwg_server_gw_interactor(void *param)
{
	char buffer[BUFFER_SIZE];
	int client_fd			= *((int *)param),
		bytes_read 			= 0,
		time_elapsed		= 0;
	sms_outqueue_t	*item	= NULL;

	_socket_fd	= client_fd;

	fcntl(client_fd, F_SETFL, O_NONBLOCK);	// set the socket to non-blocking mode

	for(;;)
	{
		int written	= 0;
		str_t from_gw	= { NULL, 0 },
			  to_gw		= { NULL, 0 };

		bzero(buffer, sizeof(buffer));

		bytes_read	= read(client_fd, buffer, sizeof(buffer));

		if (bytes_read > BUFFER_SIZE)
		{
			LOG(L_WARNING, "%s: Buffer overflow receiving %d bytes\n", __FUNCTION__, bytes_read);
			continue;
		}

		if (bytes_read < 0)
		{
			//LOG(L_DEBUG, "%s: Nothing to read\n", __FUNCTION__);
			sleep(READ_INTERVAL);
			time_elapsed	+= READ_INTERVAL;
		}
		else if (bytes_read == 0)
		{
			LOG(L_WARNING, "%s: empty message\n", __FUNCTION__);
			break;
		}
		else
		{
			from_gw.s	= buffer;
			from_gw.len	= bytes_read;

			to_gw	= *((str_t *) dwg_process_message(&from_gw));
		}

		pthread_mutex_lock(&_mutex);

		clist_foreach(_sms_queue, item, next)
		{
			dwg_build_sms(item->sms, item->gw_port, &to_gw);

			LOG(L_DEBUG, "%s: Sending sms to [%.*s], using port %d\n", __FUNCTION__, item->sms->destination.len,
																				 item->sms->destination.s,
																				 item->gw_port);
		}

		pthread_mutex_unlock(&_mutex);
/*
		if (to_gw.len == 0 && time_elapsed >= KEEP_ALIVE_INTERVAL * READ_INTERVAL)
		{
			LOG(L_DEBUG, "%s: sending keep alive request\n", __FUNCTION__);

			dwg_build_keep_alive(&to_gw);
			time_elapsed	= 0;
		}
*/
//		hexdump(to_gw.s, to_gw.len);

		if (to_gw.len > 0 && (written = write(client_fd, to_gw.s, to_gw.len)) != to_gw.len)
		{
			LOG(L_ERROR, "%s: write(): wrote %d/%d bytes only\n", __FUNCTION__, written, to_gw.len);
			//close(client_fd);
			return NULL;
		}
	}

	return NULL;
}
