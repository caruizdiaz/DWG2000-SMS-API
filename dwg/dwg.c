/*
 * dwg.c
 *
 *  Created on: Mar 28, 2012
 *      Author: caruizdiaz
 *
 */

#include "dwg.h"
#include "dwg_server.h"
#include "../networking/ip_socket.h"

static void dwg_build_msg_header(int length, int type, str_t *output);
static void dwg_serialize_sms_req(dwg_sms_request_t *msg, str_t *output);
static int swap_bytes_32(int input);
static short swap_bytes_16(short input);

static dwg_message_callback_t *_callbacks	= NULL;

typedef void (*func) (int a, str_t *b);
typedef void (*func2) (int a, dwg_ports_status_t *b);
typedef void (*func3) (dwg_ports_status_t *b);

void make_cb4(func3 f3)
{
	printf("----->\n");

	dwg_ports_status_t	*sts = malloc(sizeof(dwg_ports_status_t));
	sts->size = 3;
	//printf("yep\n");

	sts->status_array		= (dwg_port_status_t *) malloc(sizeof(dwg_port_status_t) * sts->size);
	sts->status_array->port = 123;
	sts->status_array->status = 999;

	sts->status_array[1].port = 111;
	sts->status_array[1].status = 3333;

	sts->status_array[2].port = 222;
	sts->status_array[2].status = 3333;

	sts->status_array[3].port = 333;
	sts->status_array[3].status = 3333;

	printf("yup\n");
	f3(sts);
	printf("yong!!\n");
}

void make_cb3(dwg_ports_status_t *s)
{
	s->size	= 3;
	printf("A!\n");

	s->status_array		= (dwg_port_status_t *) malloc(sizeof(dwg_port_status_t *) * s->size);
	s->status_array->port = 123;
	s->status_array->status = 999;

	s->status_array[1].port = 111;
	s->status_array[1].status = 3333;
/*
	s->status_array[2].port = 222;
	s->status_array[2].status = 444; */

	printf("done A!\n");
}

void make_cb2(func2 ptr)
{
	dwg_ports_status_t *s = malloc(sizeof (dwg_ports_status_t));
	s->size	= 3;
	printf("A\n");

	s->status_array		= (dwg_port_status_t *) malloc(sizeof(dwg_port_status_t *) * s->size);
	s->status_array->port = 123;
	s->status_array->status = 999;

	s->status_array[1].port = 111;
	s->status_array[1].status = 3333;

	s->status_array[2].port = 222;
	s->status_array[2].status = 444;

	printf("done A\n");

	ptr(444, s);
}
void make_cb(func ptr)
{
	str_t *s =  malloc(sizeof(str_t));
	//str_t s;

	STR_ALLOC((*s), 10);
	memcpy(s->s, "hol!\0", 5);
	s->len = 4;

	ptr(123, s);
}

void print_something(const char *str)
{
	printf("-> %s\n", str);
}

listener_data_t *listener_data;

void dwg_start_server(int port, dwg_message_callback_t *callbacks)
{
	listener_data = ip_start_listener(port, dwg_server_gw_interactor, DIR_DUAL);
	LOG(L_DEBUG, "status: %p\n", callbacks->status_callback);
	LOG(L_DEBUG, "rcv: %p\n", callbacks->msg_sms_recv_callback);
	LOG(L_DEBUG, "resp: %p\n", callbacks->msg_response_callback);

	_callbacks	= callbacks;
}

void dwg_stop_server()
{
	if (listener_data != NULL)
		ip_stop_listener(listener_data);
}

void dwg_send_sms(str_t *destination, str_t *message)
{
	sms_t *sms	= malloc(sizeof(sms_t));

	str_copy(sms->destination, (*destination));
	str_copy(sms->content, (*message));

	dwg_server_write_to_queue(sms);
}

void dwg_build_sms(sms_t *sms, int port, str_t *output)
{
	dwg_sms_request_t request;
	str_t body, header;

	STR_ALLOC(body, (sizeof(dwg_sms_request_t) - sizeof(str_t)) + sms->content.len);

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

void dwg_deserialize_sms_response(str_t *input, dwg_sms_response_t *response)
{
	str_t body;
	int offset	= 0;
	dwg_msg_des_header_t des_header = dwg_deserialize_message(input, &body);

	response->count_of_number	= (int) body.s[offset];
	offset++;

	bzero(response->number, sizeof(response->number));
	memcpy(response->number, &body.s[1], sizeof(response->number));
	offset += sizeof(response->number);

	response->port		= (int) body.s[offset];
	offset++;

	response->result	= (dwg_sms_result_code_t) body.s[offset];
	offset++;

	response->count_of_slice	= (int) body.s[offset];
	offset++;

	response->succeded_slices	= (int) body.s[offset];
	offset++;
}

void dwg_deserialize_sms_received(str_t *msg_body, dwg_sms_received_t *received)
{
	int offset	= 0;

	bzero(received->number, sizeof(received->number));
	memcpy(received->number, msg_body->s, sizeof(received->number));

	offset += sizeof(received->number);

	received->type	= (int) msg_body->s[offset];
	offset++;

	received->port	= (int) msg_body->s[offset];
	offset++;

	bzero(received->timestamp, sizeof(received->timestamp));
	memcpy(received->timestamp, &msg_body->s[offset], sizeof(received->timestamp) - 1 /* exclude the null terminator extra space*/);
	offset += sizeof(received->timestamp) - 1;

	received->timezone	= (int) msg_body->s[offset];
	offset++;

	received->encoding	= (int) msg_body->s[offset];
	offset++;

	short aux	= 0;
	memcpy(&aux, &msg_body->s[offset], 2);
	aux	= swap_bytes_16(aux);

	offset += 2;

	STR_ALLOC(received->message, aux + 1);
	received->message.len--;

	memcpy(received->message.s, &msg_body->s[offset], aux);
	received->message.s[aux] = '\0';

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

void dwg_build_sms_res_ack(str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	dwg_build_msg_header(1, DWG_TYPE_SEND_SMS_RESULT_RESP, &header);

	STR_ALLOC((*output), header.len + 1);
	output->len	= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));
}

void dwg_build_sms_recv_ack(str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	dwg_build_msg_header(1, DWG_TYPE_RECV_SMS_RESULT, &header);

	STR_ALLOC((*output), header.len + 1);
	output->len	= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));
}

void dwg_build_keep_alive(str_t *output)
{
	dwg_build_msg_header(0, DWG_TYPE_KEEP_ALIVE, output);
}

static void dwg_serialize_sms_req(dwg_sms_request_t *msg, str_t *output)
{
	int offset	= 0,
		size	= sizeof(dwg_sms_request_t) - sizeof(str_t) /* str_t content */ + msg->content.len;

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
	return ((input>>24)&0xff) | 		// move byte 3 to byte 0
            ((input<<8)&0xff0000) | 	// move byte 1 to byte 2
            ((input>>8)&0xff00) | 		// move byte 2 to byte 1
            ((input<<24)&0xff000000); 	// byte 0 to byte 3
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

str_t *dwg_process_message(str_t *input)
{
	str_t *output			= malloc(sizeof(str_t)),
		  body;
	static int heart_beat	= 0;
    dwg_msg_des_header_t des_header;

    LOG(L_DEBUG, "%s: received %d bytes\n", __FUNCTION__, input->len);

	des_header	= dwg_deserialize_message(input, &body);

	switch(des_header.type)
	{
		case DWG_TYPE_KEEP_ALIVE:
			LOG(L_DEBUG, "%s: received DWG_TYPE_KEEP_ALIVE\n", __FUNCTION__);
			break;
		case DWG_TYPE_STATUS:
			LOG(L_DEBUG, "%s: received DWG_TYPE_STATUS\n", __FUNCTION__);

			int index;
			dwg_ports_status_t *ports_status	= malloc(sizeof(dwg_ports_status_t));

			ports_status->size			= (int) body.s[0];
			ports_status->status_array	= malloc(sizeof(dwg_port_status_t) * ports_status->size);

			for (index = 0; index < ports_status->size; index++)
			{
				ports_status->status_array[index].port	= index;
				ports_status->status_array[index].status = (int) body.s[index + 1];
			}

			DWG_CALL_IF_NOT_NULL(_callbacks->status_callback, ports_status);

/*
			LOG(L_DEBUG, "\tNumber of ports: %d\n", (int) body.s[0]);
			LOG(L_DEBUG, "\t\tPORT0: %d\n", (int) body.s[1]);
			LOG(L_DEBUG, "\t\tPORT1: %d\n", (int) body.s[2]);
			LOG(L_DEBUG, "\t\tPORT2: %d\n", (int) body.s[3]);
			LOG(L_DEBUG, "\t\tPORT3: %d\n", (int) body.s[4]);
			LOG(L_DEBUG, "\t\tPORT4: %d\n", (int) body.s[5]);
			LOG(L_DEBUG, "\t\tPORT5: %d\n", (int) body.s[6]);
			LOG(L_DEBUG, "\t\tPORT6: %d\n", (int) body.s[7]);
			LOG(L_DEBUG, "\t\tPORT7: %d\n", (int) body.s[8]);
*/
			dwg_build_status_response(output);
			break;
		case DWG_TYPE_SEND_SMS:
			LOG(L_DEBUG, "%s: received DWG_TYPE_SEND_SMS\n", __FUNCTION__);
			break;
		case DWG_TYPE_SEND_SMS_RESP:
			LOG(L_DEBUG, "%s: received DWG_TYPE_SEND_SMS_RESP\n", __FUNCTION__);

			if ((int) body.s[0] == 0)
			{
				LOG(L_DEBUG, "%s: SMS was received by the gw\n", __FUNCTION__);
			}
			else
			{
				LOG(L_ERROR, "%s: Error sending sms\n", __FUNCTION__);
				hexdump(output->s, output->len);
			}

			dwg_build_sms_ack(output);

			break;
		case DWG_TYPE_SEND_SMS_RESULT:
			LOG(L_DEBUG, "%s: received DWG_TYPE_SEND_SMS_RESULT\n", __FUNCTION__);

			dwg_sms_response_t	*response	= malloc(sizeof(dwg_sms_response_t));
			dwg_deserialize_sms_response(input, response);

			DWG_CALL_IF_NOT_NULL(_callbacks->msg_response_callback, response);

			dwg_build_sms_recv_ack(output);

			break;
		case DWG_TYPE_SEND_SMS_RESULT_RESP:
			LOG(L_DEBUG, "%s: received DWG_TYPE_SEND_SMS_RESULT_RESP\n", __FUNCTION__);
			dwg_build_sms_res_ack(output);
			break;
		case DWG_TYPE_RECV_SMS:
			LOG(L_DEBUG, "%s: received DWG_TYPE_RECV_SMS\n", __FUNCTION__);

			dwg_sms_received_t *received	= malloc(sizeof(dwg_sms_received_t));

			dwg_deserialize_sms_received(&body, received);

			DWG_CALL_IF_NOT_NULL(_callbacks->msg_sms_recv_callback, received);

			break;
		default:
			LOG(L_DEBUG, "%s: Received unknown code %d\n", __FUNCTION__, des_header.type);

			hexdump(input->s, input->len);
			break;
	}

//	hexdump(output->s, output->len);

	return output;
}
