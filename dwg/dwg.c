/*
 * dwg.c
 *
 *  Created on: Mar 28, 2012
 *      Author: caruizdiaz
 *
 */

//#define WITH_DEBUG

#include <time.h>
#include "dwg.h"
#include "clist.h"
#include "dwg_server.h"
#include "../networking/ip_socket.h"


/*
 * DWG header-body pair
 */
typedef struct dwg_hbp
{
	struct dwg_hbp *next;
	struct dwg_hbp *prev;

	dwg_msg_des_header_t hdr;
	str_t body;
} dwg_hbp_t;

static _bool dwg_build_msg_header(int length, short type, str_t *output);
static _bool dwg_build_msg_header_with_header(dwg_msg_des_header_t *hdr, str_t *output);
static _bool dwg_serialize_sms_req(dwg_sms_request_t *msg, str_t *output);
static int swap_bytes_32(int input);
static short swap_bytes_16(short input);
static _bool get_messages(str_t *input, dwg_hbp_t *hbp);
static void unicode2ascii(str_t *unicode, str_t* ascii);
static void ascii2unicode(str_t *ascii, str_t* unicode)
;
static dwg_message_callback_t *_callbacks	= NULL;
listener_data_t *listener_data;
_bool _is_api_2_0							= FALSE;

void dwg_start_server(int port, dwg_message_callback_t *callbacks)
{
	dwg_initilize_server();

	listener_data = ip_start_listener(port, dwg_server_gw_interactor, DIR_DUAL);

	_callbacks	= callbacks;
}

void dwg_stop_server()
{
	if (listener_data != NULL)
		ip_stop_listener(listener_data);

	dwg_kill_connection();
}

void dwg_send_sms(str_t *destination, str_t *message, unsigned int port)
{
	sms_t *sms	= malloc(sizeof(sms_t));

	str_copy(sms->destination, (*destination));
	str_copy(sms->content, (*message));

	dwg_server_write_to_queue(sms, port);
}

_bool dwg_build_sms(sms_t *sms, int port, str_t *output)
{
	dwg_sms_request_t request;
	str_t body, header;
	short length_16;

	/*STR_ALLOC(body, (sizeof(dwg_sms_request_t) - sizeof(str_t)) + sms->content.len);
	if (!body.s)
	{
		LOG(L_ERROR, "%s.%d: No more memory trying to allocate %d bytes\n", __FUNCTION__, __LINE__, body.len);
		return FALSE;
	}*/

	request.port			= port;
	request.encoding		= (_is_api_2_0) ? DWG_ENCODING_UNICODE : DWG_ENCODING_ASCII;
	request.type			= DWG_MSG_TYPE_SMS;
	request.count_of_number	= 1;

	bzero(request.number, sizeof(request.number));
	memcpy(request.number, sms->destination.s, sms->destination.len);

	if (request.encoding == DWG_ENCODING_ASCII)
	{
		length_16	= swap_bytes_16((short) sms->content.len);
		memcpy(request.content_length, &length_16, sizeof(request.content_length));

		STR_ALLOC(request.content, sms->content.len);
		if (!request.content.s)
		{
			LOG(L_ERROR, "%s.%d: No more memory trying to allocate %d bytes\n", __FUNCTION__, __LINE__, sms->content.len);
			return FALSE;
		}

		memcpy(request.content.s, sms->content.s, sms->content.len);
	}
	else
	{
		str_t unicode;

		ascii2unicode(&sms->content, &unicode);
		length_16	= swap_bytes_16((short) unicode.len);
		memcpy(request.content_length, &length_16, sizeof(request.content_length));

		STR_ALLOC(request.content, unicode.len);
		if (!request.content.s)
		{
			LOG(L_ERROR, "%s.%d: No more memory trying to allocate %d bytes\n", __FUNCTION__, __LINE__, unicode.len);
			return FALSE;
		}

		memcpy(request.content.s, unicode.s, unicode.len);

		STR_FREE_NON_0(unicode);
	}

	dwg_serialize_sms_req(&request, &body);
	dwg_build_msg_header(body.len, DWG_MSG_TYPE_SMS, &header);

	STR_ALLOC((*output), (header.len + body.len));
	if (!output->s)
	{
		LOG(L_ERROR, "%s.%d: No more memory trying to allocate %d bytes\n", __FUNCTION__, __LINE__, header.len + body.len);
		return FALSE;
	}

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], body.s, body.len);

	STR_FREE_NON_0(body);
	STR_FREE_NON_0(request.content);
//	hexdump(output->s, output->len);

	return TRUE;
}

void dwg_deserialize_sms_response(str_t *input, dwg_sms_response_t *response)
{
	int offset	= 0;

	response->count_of_number	= (int) input->s[offset];
	offset++;

	bzero(response->number, sizeof(response->number));
	memcpy(response->number, &input->s[1], sizeof(response->number));
	offset += sizeof(response->number);

	response->port		= (int) input->s[offset];
	offset++;

	response->result	= (dwg_sms_result_code_t) input->s[offset];
	offset++;

	response->count_of_slice	= (int) input->s[offset];
	offset++;

	response->succeded_slices	= (int) input->s[offset];
	offset++;

/*	printf("result: %d. port; %d '%s'\n", response->result, response->port, response->number);
	int i = 0;
	for (i = 0; i < 24; i++)
		printf("%c", response->number[i]); */
}

_bool dwg_deserialize_sms_received(str_t *msg_body, dwg_sms_received_t *received)
{
	int offset	= 0;
	str_t sms_content;

	bzero(received->number, sizeof(received->number));
	memcpy(received->number, msg_body->s, sizeof(received->number));
	offset += sizeof(received->number);

	received->str_number.s	= received->number;
	received->str_number.len= strlen(received->number);

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

	if (aux < 0)
	{
		LOG(L_ERROR, "%s: error getting message length. Original length: %d, swapped length: %d. Using workaround to fix it\n", __FUNCTION__, swap_bytes_16(aux), aux);
		aux	= msg_body->len - DWG_SMS_HEADER_SIZE;

		if (aux < 0)
		{
			LOG(L_ERROR, "%s: Couldn't be able to fix message length\n", __FUNCTION__);
			return FALSE;
		}

		LOG(L_ERROR, "%s: fixed length is: %d\n", __FUNCTION__, aux);
	}

//	ascii2unicode(&ascii_number, &received->unicode_number);

	STR_ALLOC(sms_content, aux + 1);
	if (!sms_content.s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, aux + 1);
		return FALSE;
	}

	sms_content.len--;

	memcpy(sms_content.s, &msg_body->s[offset], aux);
	sms_content.s[aux] = '\0';

	received->message	= sms_content;

	if (_is_api_2_0 && (received->encoding == DWG_ENCODING_ASCII))
	{
		LOG(L_ERROR, "%s: encoding gsm7bit not supported\n", __FUNCTION__);
//		STR_FREE(sms_content);
		return FALSE;
	}

	if (received->encoding == DWG_ENCODING_ASCII)
	{
//		STR_FREE(sms_content);
		return TRUE;
	}

	unicode2ascii(&sms_content, &received->message);

	STR_FREE(sms_content);
	return TRUE;
}

static void unicode2ascii(str_t *unicode, str_t* ascii)
{
	short uni_array[unicode->len / 2];
	int i;

//	hexdump(unicode->s, unicode->len);

//	printf("before conversion [%.*s]\n", unicode->len, unicode->s);
	memcpy(&uni_array, unicode->s, unicode->len);
	STR_ALLOC((*ascii), (unicode->len / 2));
	if (!ascii->s)
	{
		LOG(L_ERROR, "%s: no more memory trying to allocate %d bytes\n", __FUNCTION__,  (unicode->len / 2));
		return;
	}

	for(i = 0; i < (unicode->len / 2); i++)
		ascii->s[i]	= swap_bytes_16(uni_array[i]);

//	printf("after conversion [%.*s]\n", ascii->len, ascii->s);
}

static void ascii2unicode(str_t *ascii, str_t* unicode)
{
	int i;

//	hexdump(unicode->s, unicode->len);

//	printf("before [%.*s]\n", unicode->len, unicode->s);
//	memcpy(&uni_array, unicode->s, unicode->len);
	STR_ALLOC((*unicode), ascii->len * 2);
	if (!unicode->s)
	{
		LOG(L_ERROR, "%s: no more memory trying to allocate %d bytes\n", __FUNCTION__,  ascii->len * 2);
		return;
	}

	for(i = 0; i < ascii->len; i++)
	{
		unicode->s[i * 2]		= 0;
		unicode->s[i * 2 + 1]	= ascii->s[i];
		//printf("%c|%c", uni_array[i], swap_bytes_16(uni_array[i]));
	}
	//printf("after [%.*s]\n", ascii->len, ascii->s);
}

static _bool get_messages(str_t *input, dwg_hbp_t *hbp)
{
	str_t *current_offset	= malloc(sizeof(str_t));
	int total_bytes			= 0;
	dwg_hbp_t *hbp_item		= NULL;

	clist_init(hbp, next, prev);

	do
	{
		current_offset->s	= input->s + total_bytes;
		current_offset->len	= input->len - total_bytes;

/*		printf("len now: %d\n", current_offset->len);
		printf("============\n");
		hexdump(current_offset->s, current_offset->len);
*/
		hbp_item			= malloc(sizeof(dwg_hbp_t));
		if (!hbp_item)
		{
			LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, sizeof(dwg_hbp_t));
			return FALSE;
		}

		hbp_item->hdr		= dwg_deserialize_message(current_offset, &hbp_item->body);

		if (!hbp_item->body.s)
			return FALSE;

//		printf("hdr len: %d | type: %d\n", hbp_item->hdr.length, hbp_item->hdr.type);
//		hexdump(current_offset->s, current_offset->len);

//		hexdump(current_offset->s, DWG_MSG_HEADER_SIZE + hbp_item->hdr.length);

		clist_append(hbp, hbp_item, next, prev);

		total_bytes += DWG_MSG_HEADER_SIZE + hbp_item->hdr.length;

//		printf("bytes %d/%d\n", total_bytes, input->len);
	}
	while (total_bytes < input->len);

	free(current_offset);

	return TRUE;
}

dwg_msg_des_header_t dwg_deserialize_message(str_t *input, str_t *body)
{
	dwg_msg_header_t header;
	dwg_msg_des_header_t des_header;
	//int length, type, result;

/*	printf("hdr!!!\n");
	hexdump(input->s, 24);
*/
	dwg_get_msg_header(input, &header);

	memcpy(&des_header.length, header.length, sizeof(header.length));
	memcpy(&des_header.type, header.type, sizeof(header.type));
	memcpy(&des_header.serial, header.ID.serial_number, sizeof(header.ID.serial_number));
	memcpy(&des_header.MAC, header.ID.MAC, sizeof(header.ID.MAC));
	memcpy(&des_header.timestamp, header.ID.time, sizeof(header.ID.time));
	memcpy(&des_header.flag, header.flag, sizeof(header.flag));

	des_header.length	= swap_bytes_32(des_header.length);
	des_header.type		= swap_bytes_16(des_header.type);
	des_header.serial	= swap_bytes_32(des_header.serial);
	//des_header.MAC		= swap_bytes_64(des_header.MAC);
	des_header.timestamp= swap_bytes_32(des_header.timestamp);
	des_header.flag		= swap_bytes_16(des_header.flag);

//	printf("length: %d\n", des_header.length);
//	printf("type: %d\n", des_header.type);
//	printf("A\n");
	STR_ALLOC((*body), des_header.length);

	if (body->s == NULL)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, des_header.length);
		body->s	= NULL;
		return des_header;
	}

//	printf("B\n");
	memcpy(body->s, &input->s[DWG_MSG_HEADER_SIZE], des_header.length);

//	printf("after memcpy\n");

	return des_header;
}

_bool dwg_build_rssi_response(dwg_msg_des_header_t *original_hdr, str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	original_hdr->length	= 1; // update length
	original_hdr->type		= DWG_TYPE_RECV_RSSI_RESP;
	dwg_build_msg_header_with_header(original_hdr, &header);

	STR_ALLOC((*output), header.len + 1);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, header.len + 1);
		return FALSE;
	}

	output->len	= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));

	STR_FREE_NON_0(header);
	return TRUE;
}

_bool dwg_build_auth_response(dwg_msg_des_header_t *original_hdr, str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	original_hdr->length	= 1; // update length
	original_hdr->type		= DWG_TYPE_RECV_AUTH_RESP;
	dwg_build_msg_header_with_header(original_hdr, &header);

	STR_ALLOC((*output), header.len + 1);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, header.len + 1);
		return FALSE;
	}

	output->len	= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));

	STR_FREE_NON_0(header);
	return TRUE;
}

_bool dwg_build_status_response(dwg_msg_des_header_t *original_hdr, str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	original_hdr->length	= 1; // update length
	original_hdr->type		= DWG_TYPE_STATUS_RESPONSE;
	dwg_build_msg_header_with_header(original_hdr, &header);

	STR_ALLOC((*output), header.len + 1);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, header.len + 1);
		return FALSE;
	}

	output->len	= header.len + 1;

//	hexdump(header.s, header.len);

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));

	STR_FREE_NON_0(header);
	return TRUE;
}

_bool dwg_build_sms_ack(dwg_msg_des_header_t *original_hdr, str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	original_hdr->length	= 1; // update length
	original_hdr->type		= DWG_TYPE_SEND_SMS_RESP;
	dwg_build_msg_header_with_header(original_hdr, &header);

	STR_ALLOC((*output), header.len + 1);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, header.len + 1);
		return FALSE;
	}
	output->len				= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));

	STR_FREE_NON_0(header);
	return TRUE;
}

_bool dwg_build_sms_res_ack(dwg_msg_des_header_t *original_hdr, str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	original_hdr->length	= 1; // update length
	original_hdr->type		= DWG_TYPE_SEND_SMS_RESULT_RESP;
	dwg_build_msg_header_with_header(original_hdr, &header);

	STR_ALLOC((*output), header.len + 1);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, header.len + 1);
		return FALSE;
	}
	output->len	= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));

	STR_FREE_NON_0(header);
	return TRUE;
}

_bool dwg_build_sms_recv_ack(dwg_msg_des_header_t *original_hdr, str_t *output)
{
	str_t header;
	char response	= SMS_RC_SUCCEED;

	original_hdr->length	= 1; // update length
	original_hdr->type		= DWG_TYPE_RECV_SMS_RESULT;
	dwg_build_msg_header_with_header(original_hdr, &header);

	STR_ALLOC((*output), header.len + 1);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, header.len + 1);
		return FALSE;
	}
	output->len	= header.len + 1;

	memcpy(output->s, header.s, header.len);
	memcpy(&output->s[header.len], &response, sizeof(char));

	STR_FREE_NON_0(header);

	return TRUE;
}

_bool dwg_build_keep_alive(str_t *output)
{
	return dwg_build_msg_header(0, DWG_TYPE_KEEP_ALIVE, output);
}

_bool dwg_serialize_sms_req(dwg_sms_request_t *msg, str_t *output)
{
	int offset	= 0,
		size	= sizeof(dwg_sms_request_t) - sizeof(str_t) /* str_t content */ + msg->content.len;

	STR_ALLOC((*output), size);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, size);
		return FALSE;
	}

	ADD_MSG_OFFSET(msg->port, offset, output->s);
	ADD_MSG_OFFSET(msg->encoding, offset, output->s);
	ADD_MSG_OFFSET(msg->type, offset, output->s);
	ADD_MSG_OFFSET(msg->count_of_number, offset, output->s);
	ADD_MSG_OFFSET(msg->number, offset, output->s);
	ADD_MSG_OFFSET(msg->content_length, offset, output->s);

	memcpy(&output->s[offset], msg->content.s, msg->content.len);
	offset += msg->content.len;

	output->len	= offset;

	return TRUE;
}

_bool dwg_build_msg_header(int length, short type, str_t *output)
{
	dwg_msg_des_header_t hdr;

	hdr.length		= length;
	hdr.type		= type;
	hdr.timestamp	= time(NULL);
	hdr.flag		= 0;
/*
 * The following, can actually contain any value, it does not matter as long as all fields combined together identify uniquely
 * the message.
 */
	hdr.MAC[0]	= 0x0;
	hdr.MAC[1]	= 0x1f;
	hdr.MAC[2]	= 0xd6;
	hdr.MAC[3]	= 0xc7;
	hdr.MAC[4]	= 0x6e;
	hdr.MAC[5]	= 0x7e;
	hdr.MAC[6]	= 0;
	hdr.MAC[7]	= 0;

	hdr.serial		= rand() % 9999;

	return dwg_build_msg_header_with_header(&hdr, output);
}

_bool dwg_build_msg_header_with_header(dwg_msg_des_header_t *hdr, str_t *output)
{
	int offset = 0;

	STR_ALLOC((*output), DWG_MSG_HEADER_SIZE);
	if (!output->s)
	{
		LOG(L_ERROR, "%s: No more memory trying to allocate %d bytes\n", __FUNCTION__, DWG_MSG_HEADER_SIZE);
		return FALSE;
	}

	hdr->length		= swap_bytes_32(hdr->length);
	hdr->type		= swap_bytes_16(hdr->type);
	hdr->timestamp	= swap_bytes_32(hdr->timestamp);
	hdr->flag		= swap_bytes_16(hdr->flag);
	hdr->serial		= swap_bytes_32(hdr->serial);

	ADD_MSG_OFFSET(hdr->length, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(hdr->MAC, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(hdr->timestamp, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(hdr->serial, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(hdr->type, offset, output->s);
	//printf("offset: %d\n", offset);
	ADD_MSG_OFFSET(hdr->flag, offset, output->s);
	//printf("offset: %d\n", offset);

	return TRUE;
}

static short swap_bytes_16(short input)
{
	return (input>>8) | (input<<8);
}

static int swap_bytes_32(int input)
{
	return ((input>>24)&0xff) 		| 		// move byte 3 to byte 0
            ((input<<8)&0xff0000) 	| 		// move byte 1 to byte 2
            ((input>>8)&0xff00) 	| 		// move byte 2 to byte 1
            ((input<<24)&0xff000000); 		// byte 0 to byte 3
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


void dwg_process_message(str_t *ip_from, str_t *input, dwg_outqueue_t *outqueue)
{
	dwg_outqueue_t *output	= NULL;
    dwg_hbp_t	*hbp		= malloc(sizeof(dwg_hbp_t));
    dwg_hbp_t	*item, *aux = NULL;

    LOG(L_DEBUG, "%s: received %d bytes\n", __FUNCTION__, input->len);

    if (!get_messages(input, hbp))
    {
    	LOG(L_ERROR, "%s: error processing message\n", __FUNCTION__);
    	return;
    }

    clist_foreach_safe(hbp, item, aux, next)
    {
    	output	= NULL;

		switch(item->hdr.type)
		{
			case DWG_TYPE_KEEP_ALIVE:
				LOG(L_DEBUG, "%s: received DWG_TYPE_KEEP_ALIVE\n", __FUNCTION__);
				break;
			case DWG_TYPE_STATUS:
				LOG(L_DEBUG, "%s: received DWG_TYPE_STATUS\n", __FUNCTION__);

				int index;
				dwg_ports_status_t *ports_status	= malloc(sizeof(dwg_ports_status_t));

				ports_status->size			= (int) item->body.s[0];
				ports_status->status_array	= malloc(sizeof(dwg_port_status_t) * ports_status->size);

				for (index = 0; index < ports_status->size; index++)
				{
					ports_status->status_array[index].port	= index;
					ports_status->status_array[index].status = (int) item->body.s[index + 1];
				}

				DWG_CALL_IF_NOT_NULL(_callbacks->status_callback, ip_from, ports_status);

				output	= malloc(sizeof(dwg_outqueue_t));
				if (!dwg_build_status_response(&item->hdr, &output->content))
				{
					LOG(L_ERROR, "%s: error building reponse", __FUNCTION__);

					free(output);
					output	= NULL;
				}

				break;
			case DWG_TYPE_SEND_SMS:
				LOG(L_DEBUG, "%s: received DWG_TYPE_SEND_SMS\n", __FUNCTION__);
				break;
			case DWG_TYPE_SEND_SMS_RESP:
				LOG(L_DEBUG, "%s: received DWG_TYPE_SEND_SMS_RESP\n", __FUNCTION__);

				if ((int) item->body.s[0] == 0)
				{
					LOG(L_DEBUG, "%s: SMS was received by the gw\n", __FUNCTION__);
				}
				else
				{
					LOG(L_ERROR, "%s: Error sending sms\n", __FUNCTION__);
				}

				output	= malloc(sizeof(dwg_outqueue_t));
				if (!dwg_build_sms_ack(&item->hdr, &output->content))
				{
					LOG(L_ERROR, "%s: error building response", __FUNCTION__);

					free(output);
					output	= NULL;
				}
				break;
			case DWG_TYPE_SEND_SMS_RESULT:
				LOG(L_DEBUG, "%s: received DWG_TYPE_SEND_SMS_RESULT\n", __FUNCTION__);

				dwg_sms_response_t	*response	= malloc(sizeof(dwg_sms_response_t));
				dwg_deserialize_sms_response(&item->body, response);

				DWG_CALL_IF_NOT_NULL(_callbacks->msg_response_callback, ip_from, response);

				output	= malloc(sizeof(dwg_outqueue_t));
				if (!dwg_build_sms_res_ack(&item->hdr, &output->content))
				{
					LOG(L_ERROR, "%s: error building response", __FUNCTION__);

					free(output);
					output	= NULL;
				}

				break;
			case DWG_TYPE_RECV_SMS:
				LOG(L_DEBUG, "%s: received DWG_TYPE_RECV_SMS\n", __FUNCTION__);

				dwg_sms_received_t *received	= malloc(sizeof(dwg_sms_received_t));

				if (!dwg_deserialize_sms_received(&item->body, received))
				{
					LOG(L_ERROR, "%s: error deserializing received message\n", __FUNCTION__);
					hexdump(item->body.s, item->body.len);
					break;
				}

				DWG_CALL_IF_NOT_NULL(_callbacks->msg_sms_recv_callback, ip_from, received);

				output	= malloc(sizeof(dwg_outqueue_t));
				if (!dwg_build_sms_recv_ack(&item->hdr, &output->content))
				{
					LOG(L_ERROR, "%s: error building response", __FUNCTION__);

					free(output);
					output	= NULL;
				}
				break;
			case DWG_TYPE_RECV_AUTH:
				LOG(L_DEBUG, "%s: received DWG_TYPE_RECV_AUTH\n", __FUNCTION__);

				output	= malloc(sizeof(dwg_outqueue_t));
				if (!dwg_build_auth_response(&item->hdr, &output->content))
				{
					LOG(L_ERROR, "%s: error building response", __FUNCTION__);
					free(output);
					output	= NULL;
				}
				_is_api_2_0	= TRUE;

				break;
			case DWG_TYPE_RECV_RSSI:
				LOG(L_DEBUG, "%s: received DWG_TYPE_RECV_RSSI\n", __FUNCTION__);

				output	= malloc(sizeof(dwg_outqueue_t));
				if (!dwg_build_rssi_response(&item->hdr, &output->content))
				{
					LOG(L_ERROR, "%s: error building response", __FUNCTION__);

					free(output);
					output	= NULL;
				}

				break;

			default:
				LOG(L_DEBUG, "%s: Received unknown code %d\n", __FUNCTION__, item->hdr.type);

				hexdump(item->body.s, item->body.len);
				break;
		}

//		printf("A\n");
		if (output != NULL)
		{
			clist_append(outqueue, output, next, prev);
		}

		clist_rm(item, next, prev);
//		printf("B\n");

//		printf("C %p %d\n", item->body.s, item->body.len);

		if (item->body.s != NULL)
			free(item->body.s);

		//printf("D\n");
		free(item);

//		printf("end of iteration\n");
    }

    //free()

//	hexdump(output->s, output->len);
}
