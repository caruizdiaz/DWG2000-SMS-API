/*
 * dwg.c
 *
 *  Created on: Mar 28, 2012
 *      Author: caruizdiaz
 *
 */

#include "dwg.h"

static void dwg_build_msg_header(int length, int type, str_t *output);
static void dwg_serialize_sms_req(dwg_sms_req_t *msg, str_t *output);
static int swap_bytes_32(int input);
static short swap_bytes_16(short input);

void dwg_build_sms(sms_t *sms, int port, str_t *output)
{
	dwg_sms_req_t request;
	str_t body, header;

	STR_ALLOC(body, (sizeof(dwg_sms_req_t) - sizeof(str_t)) + sms->content.len);

	request.port			= port;
	request.encoding		= DWG_ENCODING_ASCII;
	request.type			= DWG_MSG_TYPE_SMS;
	request.count_of_number	= 1;

	bzero(request.number, sizeof(request.number));
	strcpy(request.number, sms->destination.s);

	short length_16	= swap_bytes_16((short) sms->content.len);
	memcpy(request.content_length, &length_16, sizeof(request.content_length));

	STR_ALLOC(request.content, sms->content.len);
	memcpy(request.content.s, sms->content.s, sms->content.len);

	dwg_serialize_sms_req(&request, &body);
	dwg_build_msg_header(body.len, DWG_MSG_TYPE_SMS, &header);

	STR_ALLOC((*output), header.len + body.len);

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], body.s, body.len);

//	hexdump(output->s, output->len);
}

dwg_msg_des_header_t dwg_deserialize_message(str_t *input, str_t *body)
{
	dwg_msg_header_t header;
	dwg_msg_des_header_t des_header;
	//int length, type, result;

	dwg_get_msg_header(input, &header);

	memcpy(&des_header.length, header.length, sizeof(header.length));
	memcpy(&des_header.type, header.type, sizeof(header.type));

	des_header.length	= swap_bytes_32(des_header.length);
	des_header.type		= swap_bytes_16(des_header.type);

	//printf("length: %d\n", des_header.length);
	//printf("type: %d\n", des_header.type);

	STR_ALLOC((*body), des_header.length);

	memcpy(body->s, &input->s[DWG_MSG_HEADER_SIZE], des_header.length);

	return des_header;
}

void dwg_build_status_response(str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	dwg_build_msg_header(1, DWG_TYPE_STATUS_RESPONSE, &header);

	STR_ALLOC((*output), header.len + 1);
	output->len	= header.len + 1;

//	hexdump(header.s, header.len);

	memcpy(output->s, header.s, header.len);
//	hexdump(output->s, output->len);

	memcpy(&output->s[header.len], &response, sizeof(char));
//	hexdump(output->s, output->len);
}

void dwg_build_sms_ack(str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	dwg_build_msg_header(1, DWG_TYPE_SEND_SMS_RESP, &header);

	STR_ALLOC((*output), header.len + 1);
	output->len	= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));
}

void dwg_build_keep_alive(str_t *output)
{
	dwg_build_msg_header(0, DWG_TYPE_KEEP_ALIVE, output);
}

void dwg_deserialize_sms_res(str_t *input, dwg_sms_res_t *msg)
{
	int offset	= 0;

	GET_MSG_OFFSET(msg->count_of_number, offset, input->s);
	GET_MSG_OFFSET(msg->number, offset, input->s);
	GET_MSG_OFFSET(msg->port, offset, input->s);
	GET_MSG_OFFSET(msg->result, offset, input->s);
	GET_MSG_OFFSET(msg->count_of_slice, offset, input->s);
	GET_MSG_OFFSET(msg->succeded_slices, offset, input->s);
}

static void dwg_serialize_sms_req(dwg_sms_req_t *msg, str_t *output)
{
	int offset	= 0,
		size	= sizeof(dwg_sms_req_t) - sizeof(str_t) /* str_t content */ + msg->content.len;

	output->s	= malloc(size);
//	output->len	= size;

	ADD_MSG_OFFSET(msg->port, offset, output->s);
	ADD_MSG_OFFSET(msg->encoding, offset, output->s);
	ADD_MSG_OFFSET(msg->type, offset, output->s);
	ADD_MSG_OFFSET(msg->count_of_number, offset, output->s);
	ADD_MSG_OFFSET(msg->number, offset, output->s);
	ADD_MSG_OFFSET(msg->content_length, offset, output->s);

	memcpy(&output->s[offset], msg->content.s, msg->content.len);
	offset += msg->content.len;

	output->len	= offset;
	//printf("size %d os %d len %d\n", sze, offset, msg->content.len);
}

static void dwg_build_msg_header(int length, int type, str_t *output)
{
	dwg_msg_header_t header;
	int offset	= 0;

	STR_ALLOC((*output), sizeof(dwg_msg_header_t));

	bzero(header.ID.MAC, sizeof(header.ID.MAC));
	bzero(header.flag, sizeof(header.flag));

	header.ID.MAC[0]	= 0x0;
	header.ID.MAC[1]	= 0x1f;
	header.ID.MAC[2]	= 0xd6;
	header.ID.MAC[3]	= 0xc7;
	header.ID.MAC[4]	= 0x0d;
	header.ID.MAC[5]	= 0xb2;
	header.ID.MAC[6]	= 0;
	header.ID.MAC[7]	= 0;

	bzero(header.ID.time, sizeof(header.ID.time));
	bzero(header.ID.serial_number, sizeof(header.ID.serial_number));

	header.ID.serial_number[0] = 0x0;
	header.ID.serial_number[1] = 0x0;
	header.ID.serial_number[2] = 0x3f;
	header.ID.serial_number[3] = 0x3d;

	// 0d 1f 76 b6
	header.ID.time[0] = 0x0d;
	header.ID.time[1] = 0x1f;
	header.ID.time[2] = 0x76;
	header.ID.time[3] = 0xb6;

	int serial_32 = swap_bytes_32(6);
	length	= swap_bytes_32(length);
	short type_16	= swap_bytes_16(type);

	memcpy(header.length, &length, sizeof(header.length));
	memcpy(header.type, &type_16, sizeof(header.type));
	memcpy(header.ID.serial_number, &serial_32, sizeof(header.ID.serial_number));

	ADD_MSG_OFFSET(header.length, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(header.ID.MAC, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(header.ID.time, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(header.ID.serial_number, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(header.type, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(header.flag, offset, output->s);
	//printf("offset: %d\n", offset);

}

static short swap_bytes_16(short input)
{
	return (input>>8) | (input<<8);
}

static int swap_bytes_32(int input)
{
	return ((input>>24)&0xff) | // move byte 3 to byte 0
            ((input<<8)&0xff0000) | // move byte 1 to byte 2
            ((input>>8)&0xff00) | // move byte 2 to byte 1
            ((input<<24)&0xff000000); // byte 0 to byte 3
}


void dwg_get_msg_header(str_t *input, dwg_msg_header_t *output)
{
	int offset = 0;

	GET_MSG_OFFSET(output->length, offset, input->s);
	GET_MSG_OFFSET(output->ID.MAC, offset, input->s);
	GET_MSG_OFFSET(output->ID.time, offset, input->s);
	GET_MSG_OFFSET(output->ID.serial_number, offset, input->s);
	GET_MSG_OFFSET(output->type, offset, input->s);
	GET_MSG_OFFSET(output->flag, offset, input->s);
}
