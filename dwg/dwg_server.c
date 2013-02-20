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

#define BUFFER_SIZE 	20000

#include "dwg_server.h"
#include "dwg.h"
#include "dwg_charset.h"
#include "clist.h"
#include "../util.h"
#include "../networking/ip_socket.h"

typedef struct msg_outqueue
{
	struct msg_outqueue *next;
	struct msg_outqueue *prev;

	void *message;
	int type;
	int gw_port;

} msg_outqueue_t;
/*
typedef struct sms_outqueue
{
	struct sms_outqueue *next;
	struct sms_outqueue *prev;

	sms_t *sms;
	int gw_port;

} sms_outqueue_t;
*/
//static sms_outqueue_t *_sms_queue	= NULL;
static msg_outqueue_t *_msg_queue	= NULL;
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
	_msg_queue	= malloc(sizeof(msg_outqueue_t));
	clist_init(_msg_queue, next, prev);
	dwg_initialize_translation_table();
	/*printf("---\n");
	dwg_initialize_translation_table();
	printf("*---\n");*/
}

void dwg_server_write_to_queue(void *message, int type, unsigned int port)
{
	pthread_mutex_lock(&_mutex);

	if (_msg_queue == NULL)
	{
		LOG(L_ERROR, "%s: message queue not initialized\n", __FUNCTION__);
		return;
	}

	msg_outqueue_t *item	= malloc(sizeof(msg_outqueue_t));

	item->gw_port	= port;
	item->message	= message;
	item->type		= type;

	clist_append(_msg_queue, item, next, prev);

	pthread_mutex_unlock(&_mutex);
}
/*
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
} */

void *dwg_server_gw_interactor(void *param)
{
	char buffer[BUFFER_SIZE];
	connection_info_t *cnn_info			= (connection_info_t *) param;
	int	bytes_read 						= 0,
		time_elapsed					= 0;
	msg_outqueue_t	*msg_item, *msg_item_aux;
	_bool keep_alive					= FALSE;
	dwg_outqueue_t outqueue, *oq_item, *aux;

	memset(&outqueue, 0, sizeof(dwg_outqueue_t));
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

			dwg_process_message(&cnn_info->ip, &from_gw, &outqueue);
		}

		pthread_mutex_lock(&_mutex);
		clist_foreach_safe(_msg_queue, msg_item, msg_item_aux, next)
		{
			dwg_outqueue_t	*oq_sms	= malloc(sizeof(dwg_outqueue_t));

			/*
			 * Add built msg to the main outqueue
			 */
			if (msg_item->type == 0 && !dwg_build_sms((sms_t *) msg_item->message, msg_item->gw_port, &oq_sms->content))
			{
				LOG(L_ERROR, "%s: Error building SMS\n", __FUNCTION__);

				free(oq_sms);
				clist_rm(msg_item, next, prev);

				continue;
			}
			else if (!dwg_build_ussd((str_t *) msg_item->message, msg_item->gw_port, 1, &oq_sms->content))
			{
				LOG(L_ERROR, "%s: Error building USSD\n", __FUNCTION__);

				free(oq_sms);
				clist_rm(msg_item, next, prev);

				continue;
			}

			/*if (!dwg_build_sms(msg_item->sms, msg_item->gw_port, &oq_sms->content))
			{
				LOG(L_ERROR, "%s: Error building SMS\n", __FUNCTION__);

				free(oq_sms);
				clist_rm(msg_item, next, prev);

				continue;
			}*/
/*
			LOG(L_DEBUG, "%s: Sending sms to [%.*s], using port %d\n", __FUNCTION__, 	msg_item->sms->destination.len,
																						msg_item->sms->destination.s,
																						msg_item->gw_port);*/

			clist_rm(msg_item, next, prev);
			free(msg_item);

			clist_append((&outqueue), oq_sms, next, prev);
		}
		pthread_mutex_unlock(&_mutex);

		if (keep_alive)
		{
			dwg_outqueue_t	*oq_ka	= malloc(sizeof(dwg_outqueue_t));
			dwg_build_keep_alive(&oq_ka->content);

			clist_append((&outqueue), oq_ka, next, prev);
		}
		/*
		 * Processing of the main queue
		 */

		clist_foreach_safe((&outqueue), oq_item, aux, next)
		{
//			hexdump(oq_item->content.s, oq_item->content.len);
			if (!write_to_gw(cnn_info->client_fd, &oq_item->content))
			{
				LOG(L_ERROR, "%s: Error writing outgoing SMS to gateway\n", __FUNCTION__);
				continue;
			}

			clist_rm(oq_item, next, prev);

			if (oq_item->content.s != NULL)
				free(oq_item->content.s);

			free(oq_item);
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
