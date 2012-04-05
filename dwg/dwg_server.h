/*
 * dwg_server.h
 *
 *  Created on: Apr 5, 2012
 *      Author: caruizdiaz
 */

#ifndef DWG_SERVER_H_
#define DWG_SERVER_H_

#include "dwg.h"

#define READ_INTERVAL			5 // seconds
#define KEEP_ALIVE_INTERVAL		6 // seconds
#define SMSSRV_PORT				7008

#define SMS_ENQUEUE(_queue_, _sms_) pthread_mutex_lock(&_queue_.lock); \
									_queue_.sms	= _sms_; \
									pthread_mutex_unlock(&_queue_.lock);

void *dwg_server_gw_interactor(void *param);
void dwg_server_write_to_queue(sms_t *sms);

#endif /* DWG_SERVER_H_ */
