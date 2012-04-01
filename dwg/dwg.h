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
	int length;
	short type;
	short flag;
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

} dwg_sms_req_t;

typedef struct dwg_sms_res
{
	char count_of_number;
	char number[24];
	char port;
	char result;
	char count_of_slice;
	char succeded_slices;
} dwg_sms_res_t;

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

#define DWG_TYPE_STATUS					7
#define DWG_TYPE_STATUS_RESPONSE		8
#define DWG_TYPE_KEEP_ALIVE				0
#define DWG_TYPE_SEND_SMS				1
#define DWG_TYPE_SEND_SMS_RESP			2
#define DWG_TYPE_SEND_SMS_RESULT		3
#define DWG_TYPE_SEND_SMS_RESULT_RESP	4

#define DWG_ENCODING_ASCII			0
#define DWG_ENCODING_UNICODE		1

#define DWG_MSG_TYPE_SMS			1
#define DWG_MSG_TYPE_MMS			1

#define ADD_MSG_OFFSET(_field_, _offset_, _output_) memcpy(&_output_[_offset_], &_field_, sizeof(_field_)); \
													_offset_ += sizeof(_field_);

#define GET_MSG_OFFSET(_field_, _offset_, _input_) memcpy(_field_, &_input_[offset], sizeof(_field_)); \
													_offset_ += sizeof(_field_);


void dwg_build_keep_alive(str_t *output);
void dwg_build_sms(sms_t *sms, int port, str_t *output);
void dwg_get_msg_header(str_t *input, dwg_msg_header_t *output);
void dwg_build_sms_ack(str_t *output);
dwg_msg_des_header_t dwg_deserialize_message(str_t *input, str_t *body);
void dwg_build_status_response(str_t *output);

#endif /* DWG_H_ */
