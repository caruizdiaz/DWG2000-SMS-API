/*
 * dwg.h
 *
 *  Created on: Mar 28, 2012
 *      Author: caruizdiaz
 */

#ifndef DWG_H_
#define DWG_H_

#include <string.h>
#include <stdlib.h>
#include "../util.h"

typedef struct sms
{
	str_t destination;
	str_t content;
} sms_t;

#define DWG_MSG_HEADER_SIZE 	24

typedef struct dwg_msg_header_id
{
	char MAC[8];
	char time[4];
	char serial_number[4];
} dwg_msg_header_id_t;

typedef struct dwg_msg_header
{
	char length[4];
	dwg_msg_header_id_t ID;
	char type[2];
	char flag[2];

} dwg_msg_header_t;

typedef struct dwg_msg_des_header
{
	int32_t length;
	char MAC[8];
	int32_t timestamp;
	int32_t serial;
	int16_t type;
	int16_t flag;
} dwg_msg_des_header_t;

typedef struct dwg_sms_req
{
	char port;
	char encoding;
	char type;
	char count_of_number;
	char number[24];
	char content_length[2];
	str_t content;

} dwg_sms_request_t;

typedef enum dwg_sms_result_code
{
	SMS_RC_SUCCEED = 0,
	SMS_RC_FAIL,
	SMS_RC_TIMEOUT,
	SMS_RC_BAD_REQUEST,
	SMS_RC_PORT_UNAVAILABLE,
	SMS_RC_PARTIAL_SUCCEED,
	SMS_RC_OTHER_ERROR = 255
} dwg_sms_result_code_t;

typedef struct dwg_sms_res
{
	int count_of_number;
	char number[24];
	int port;
	dwg_sms_result_code_t result;
	int count_of_slice;
	int succeded_slices;
} dwg_sms_response_t;

typedef struct dwg_sms_received
{
	char number[24];
	int type;
	int port;
	char timestamp[16];
	int timezone;
	int encoding;
	str_t message;
} dwg_sms_received_t;

typedef struct dwg_port_status
{
	int port;
	int status;
} dwg_port_status_t;

typedef struct dwg_ports_status
{
	int size;
	dwg_port_status_t *status_array;
} dwg_ports_status_t;

#define DWG_TYPE_STATUS					7
#define DWG_TYPE_STATUS_RESPONSE		8
#define DWG_TYPE_KEEP_ALIVE				0
#define DWG_TYPE_SEND_SMS				1
#define DWG_TYPE_SEND_SMS_RESP			2
#define DWG_TYPE_SEND_SMS_RESULT		3
#define DWG_TYPE_SEND_SMS_RESULT_RESP	4
#define DWG_TYPE_RECV_SMS				5
#define DWG_TYPE_RECV_SMS_RESULT		6
#define DWG_TYPE_RECV_RSSI				0x0d
#define DWG_TYPE_RECV_RSSI_RESP			0x0e
#define DWG_TYPE_RECV_AUTH				0x0f
#define DWG_TYPE_RECV_AUTH_RESP			0x10

#define DWG_ENCODING_ASCII			0
#define DWG_ENCODING_UNICODE		1

#define DWG_MSG_TYPE_SMS			1
#define DWG_MSG_TYPE_MMS			1

#define ADD_MSG_OFFSET(_field_, _offset_, _output_) memcpy(&_output_[_offset_], &_field_, sizeof(_field_)); \
													_offset_ += sizeof(_field_);

#define GET_MSG_OFFSET(_field_, _offset_, _input_) memcpy(_field_, &_input_[offset], sizeof(_field_)); \
													_offset_ += sizeof(_field_);

#define DWG_CALL_IF_NOT_NULL(_func_, _gwid_, _param_) if (_func_!= NULL) \
												_func_(_gwid_, _param_);

typedef void (*status_callback_fptr) (str_t *gw_ip, dwg_ports_status_t *status);
typedef void (*msg_response_callback_fptr) (str_t *gw_ip, dwg_sms_response_t *res);
typedef void (*msg_sms_recv_callback_fptr) (str_t *gw_ip, dwg_sms_received_t *recv);

typedef struct dwg_message_callback
{
	status_callback_fptr status_callback;
	msg_response_callback_fptr msg_response_callback;
	msg_sms_recv_callback_fptr msg_sms_recv_callback;

} dwg_message_callback_t;

typedef struct dwg_outqueue
{
	struct dwg_outqueue *next;
	struct dwg_outqueue *prev;

	str_t content;
} dwg_outqueue_t;

void dwg_build_keep_alive(str_t *output);
void dwg_build_sms(sms_t *sms, int port, str_t *output);
void dwg_get_msg_header(str_t *input, dwg_msg_header_t *output);
void dwg_build_sms_ack(dwg_msg_des_header_t *original_hdr, str_t *output);
void dwg_build_sms_res_ack(dwg_msg_des_header_t *original_hdr, str_t *output);
dwg_msg_des_header_t dwg_deserialize_message(str_t *input, str_t *body);
void dwg_build_status_response(dwg_msg_des_header_t *original_hdr, str_t *output);
void dwg_build_auth_response(dwg_msg_des_header_t *original_hdr, str_t *output);
void dwg_build_rssi_response(dwg_msg_des_header_t *original_hdr, str_t *output);
void dwg_send_sms(str_t *destination, str_t *message, unsigned int port);
void dwg_start_server(int port, dwg_message_callback_t *callbacks);
void dwg_process_message(str_t *ip_from, str_t *input, dwg_outqueue_t *outqueue);
void dwg_deserialize_sms_received(str_t *msg_body, dwg_sms_received_t *received);
void dwg_build_sms_recv_ack(dwg_msg_des_header_t *original_hdr, str_t *output);

void print_something(const char *str);


#endif /* DWG_H_ */
