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
void dwg_server_write_to_queue(sms_t *sms, unsigned int port);
void dwg_initilize_server();
void dwg_kill_connection();
#endif /* DWG_SERVER_H_ */
/*
000000: 35 39 35 39 38 31 31 34 36 36 32 33 00 00 00 00  595981146623....
000010: 00 00 00 00 00 00 00 00 |00| 00| 32 30 31 32 30 34  ..........201204
000020: 31 31 31 34 30 35 34 31 00| 04| 00 |00 05| 54 77 6d  11140541.....Twm
000030: 6a 64                                            jd
-> 0
-> 12593
*/
