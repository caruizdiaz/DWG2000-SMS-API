/*
 * main.c
 *
 *  Created on: Mar 28, 2012
 *      Author: caruizdiaz
 */


#include "util.h"
#include "dwg/dwg.h"
#include "networking/ip_socket.h"



void status_handler(dwg_ports_status_t *status)
{
	int index = 0;

	LOG(L_DEBUG, "\tNumber of ports: %d\n", status->size);

	for (index = 0; index < status->size; index++)
	{
		LOG(L_DEBUG, "\tPort%d: %d\n", index, status->status_array[index].status);
	}

	printf("done\n");
}

void msg_response_handler(dwg_sms_response_t *response)
{
	LOG(L_DEBUG, "\tresponse from %s\n", response->number);
}

int main(int argc, char** argv)
{

	dwg_message_callback_t callbacks	= {
				.status_callback = status_handler,
				.msg_response_callback	= msg_response_handler
	};

	dwg_start_server(7008, &callbacks);

	str_t des = { "0981146623", 10 };
	str_t msg = { "hola", 4 };

	for(;;)
	{
		getchar();
		dwg_send_sms(&des, &msg);
	}

}


