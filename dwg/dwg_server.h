/*
 * dwg_server.h
 *
 *  Created on: Apr 5, 2012
 *      Author: caruizdiaz
 */

#ifndef DWG_SERVER_H_
#define DWG_SERVER_H_

#include "dwg.h"

#define READ_INTERVAL			3 // seconds
#define KEEP_ALIVE_INTERVAL		30 // KEEP_ALIVE_INTERVAL * READ_INTERVAL
#define SMSSRV_PORT				7008

#define SMS_ENQUEUE(_queue_, _sms_) pthread_mutex_lock(&_queue_.lock); \
									_queue_.sms	= _sms_; \
									pthread_mutex_unlock(&_queue_.lock);

void *dwg_server_gw_interactor(void *param);
void dwg_server_write_to_queue(void *message, int type, unsigned int port);
void dwg_initilize_server();
void dwg_kill_connection();
#endif /* DWG_SERVER_H_ */

