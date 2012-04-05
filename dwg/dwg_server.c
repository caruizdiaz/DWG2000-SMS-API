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

#include "dwg_server.h"
#include "../util.h"
#include "../networking/ip_socket.h"

typedef struct sms_wait_queue
{
	int socket_fd;
	sms_t *sms;
	pthread_mutex_t lock;
} sms_wait_queue_t;


static sms_wait_queue_t _sms_queue;

void dwg_server_write_to_queue(sms_t *sms)
{
	SMS_ENQUEUE(_sms_queue, sms);
}

void *dwg_server_gw_interactor(void *param)
{
	char buffer[BUFFER_SIZE];
	int client_fd		= *((int *)param),
		bytes_read 		= 0,
		time_elapsed	= 0;

	_sms_queue.socket_fd	= client_fd;

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

		pthread_mutex_lock(&_sms_queue.lock);
		if (_sms_queue.sms != NULL)
		{
			/*
			 * select a port randomically
			 */
			int gw_port	= rand() % 3;

			dwg_build_sms(_sms_queue.sms, gw_port, &to_gw);

			LOG(L_DEBUG, "%s: Sending sms to %s, using port %d\n", __FUNCTION__, _sms_queue.sms->destination.s, gw_port);
			_sms_queue.sms	= NULL;
		}
		pthread_mutex_unlock(&_sms_queue.lock);

		if (to_gw.len == 0 && time_elapsed >= KEEP_ALIVE_INTERVAL)
		{
			LOG(L_DEBUG, "%s: sending keep alive request\n", __FUNCTION__);

			dwg_build_keep_alive(&to_gw);
			time_elapsed	= 0;
		}

//		hexdump(to_gw.s, to_gw.len);

		if (to_gw.len > 0 && (written = write(client_fd, to_gw.s, to_gw.len)) != to_gw.len)
		{
			LOG(L_ERROR, "%s: write(): wrote %d/%d bytes only\n", __FUNCTION__, written, to_gw.len);
			close(client_fd);
			return NULL;
		}
	}

	return NULL;
}
