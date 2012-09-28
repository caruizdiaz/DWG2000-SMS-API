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

typedef struct sms_outqueue
{
	struct sms_outqueue *next;
	struct sms_outqueue *prev;

	sms_t *sms;
	int gw_port;

} sms_outqueue_t;

static sms_outqueue_t *_sms_queue	= NULL;
static pthread_mutex_t _mutex		= PTHREAD_MUTEX_INITIALIZER;
static int _socket_fd				= -1;
static _bool _stop_now				= FALSE;

static _bool write_to_gw(int fd, str_t* buffer);

void dwg_kill_connection()
{
	_stop_now	= TRUE;
	close(_socket_fd);
}

void dwg_initilize_server()
{
	_sms_queue	= malloc(sizeof(sms_outqueue_t));
	clist_init(_sms_queue, next, prev);
}

void dwg_server_write_to_queue(sms_t *sms, unsigned int port)
{
	pthread_mutex_lock(&_mutex);
	if (_sms_queue == NULL)
	{
		LOG(L_ERROR, "%s: SMS queue not initialized\n", __FUNCTION__);
		return;
	}

	sms_outqueue_t *item	= malloc(sizeof(sms_outqueue_t));

	item->gw_port	= port;
	item->sms		= sms;

	clist_append(_sms_queue, item, next, prev);
	pthread_mutex_unlock(&_mutex);
}

void *dwg_server_gw_interactor(void *param)
{
	char buffer[BUFFER_SIZE];
	connection_info_t *cnn_info	= ((connection_info_t *)param);
	int	bytes_read 				= 0,
		time_elapsed			= 0;
	sms_outqueue_t	*sms_item	= NULL;
	_bool keep_alive			= FALSE;
	dwg_outqueue_t outqueue, *oq_item;

	_socket_fd	= cnn_info->client_fd;

	fcntl(cnn_info->client_fd, F_SETFL, O_NONBLOCK);	// set the socket to non-blocking mode

	clist_init((&outqueue), next, prev);

	for(;;)
	{
		str_t from_gw	= { NULL, 0 };

		bzero(buffer, sizeof(buffer));

		bytes_read	= read(cnn_info->client_fd, buffer, sizeof(buffer));

		if (_stop_now)
		{
			free(cnn_info->ip.s);
			free(cnn_info);
			break;
		}

		if (bytes_read > BUFFER_SIZE)
		{
			LOG(L_WARNING, "%s: Buffer overflow receiving %d bytes\n", __FUNCTION__, bytes_read);
			continue;
		}

		keep_alive		= FALSE;
		sleep(READ_INTERVAL);
		time_elapsed	+= READ_INTERVAL;

		if (time_elapsed >= KEEP_ALIVE_INTERVAL)
		{
			time_elapsed	= 0;
			keep_alive		= TRUE;
		}

//		LOG(L_DEBUG, "%s: time left %d/%d\n", __FUNCTION__, time_elapsed, KEEP_ALIVE_INTERVAL);


		if (bytes_read == 0)
		{
			LOG(L_WARNING, "%s: empty message\n", __FUNCTION__);
			break;
		}
		else if (bytes_read > 0)
		{
			from_gw.s	= buffer;
			from_gw.len	= bytes_read;

//			hexdump(from_gw.s, from_gw.len);

			dwg_process_message(&cnn_info->ip, &from_gw, &outqueue);
		}

		pthread_mutex_lock(&_mutex);
		clist_foreach(_sms_queue, sms_item, next)
		{
			dwg_outqueue_t	*oq_sms	= malloc(sizeof(dwg_outqueue_t));

			/*
			 * Add built sms to the main outqueue
			 */
			dwg_build_sms(sms_item->sms, sms_item->gw_port, &oq_sms->content);

			LOG(L_DEBUG, "%s: Sending sms to [%.*s], using port %d\n", __FUNCTION__, 	sms_item->sms->destination.len,
																						sms_item->sms->destination.s,
																						sms_item->gw_port);

			//free(item->sms->content.s);
			//free(item->sms->destination.s);
			//free(item);

			clist_rm(sms_item, next, prev);

			clist_append((&outqueue), oq_sms, next, prev);
		}
		pthread_mutex_unlock(&_mutex);

		if (keep_alive)
		{
			dwg_outqueue_t	*oq_ka	= malloc(sizeof(dwg_outqueue_t));
			dwg_build_keep_alive(&oq_ka->content);

			clist_append((&outqueue), oq_ka, next, prev);
		}

		clist_foreach((&outqueue), oq_item, next)
		{
//			hexdump(oq_item->content.s, oq_item->content.len);

			write_to_gw(cnn_info->client_fd, &oq_item->content);

			clist_rm(oq_item, next, prev);

//			sleep(1);
		}
	}

	return NULL;
}

static _bool write_to_gw(int fd, str_t* buffer)
{
	int written = 0;
	if (buffer->len > 0 && (written = write(fd, buffer->s, buffer->len)) != buffer->len)
	{
		LOG(L_ERROR, "%s: write(): wrote %d/%d bytes only\n", __FUNCTION__, written, buffer->len);
		return 0;
	}

	return TRUE;
}
