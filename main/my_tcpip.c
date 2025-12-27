#include "my_tcpip.h"


server_tcp_t SLT_server;


err_t init_server_tpcp(uint16_t port, uint8_t max_client)
{
	
	struct tcp_pcb *server_listen_tpcb;
	
	ip_addr_t ipaddr;
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	
	server_listen_tpcb = tcp_new();
	
	if (!server_listen_tpcb)
	{
		return ERR_MEM;
	}
	
	if (tcp_bind(server_listen_tpcb, &ipaddr, port) != ERR_OK)
	{
		tcp_abort(server_listen_tpcb);
		return ERR_MEM;
	}
	
	server_listen_tpcb = tcp_listen_with_backlog(server_listen_tpcb, 5);
	
	
	if (!server_listen_tpcb)
	{
		tcp_close(server_listen_tpcb);
		SLT_server.tpcb = 0;
		printf("Error listen!\r\n");
		return ERR_MEM;
	}
	
	SLT_server.tpcb = server_listen_tpcb;
	SLT_server.port = port;
	SLT_server.max_client = max_client;
	SLT_server.count_client = 0; 
	SLT_server.recv.current_pos_file = 0; 
	tcp_accept(server_listen_tpcb, server_accept_tcp);
	
	tcp_arg(server_listen_tpcb, &SLT_server);
	return ERR_OK;
}

err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err)
{
	
	pwm_stop(0);
	
	server_tcp_t* server = (server_tcp_t*)arg;
	if (server->count_client >= server->max_client)
	{
		tcp_close(newpcb);
		
		pwm_start();  
		
		return ERR_MEM;
	}
	
	client_tcp_t* newclient = 0; 
	newclient = malloc(sizeof(server_tcp_t));
	
	
	if (newclient)
	{
		 
		newclient->tpcb = newpcb; 
		newclient->tpcb_server = server->tpcb;
		newclient->send.request = false;
		
		tcp_recv(newpcb, server_recv_tcp);
		tcp_sent(newpcb, server_sent_tcp);		
		tcp_arg(newpcb, newclient);
		tcp_err(newpcb, NULL);
		if(SLT_server.count_client < SLT_server.max_client)
			SLT_server.count_client++;
		return ERR_OK;
	}
	
	pwm_start();
	
	tcp_close(newpcb);
	return ERR_MEM;
	
} 


/**
 *	@brief	hàm x? lý segment tcp recv
 *	
 *	@details
 *
 */
err_t server_recv_tcp(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err)
{
	client_tcp_t* client = (client_tcp_t*)arg;
	
	if ((err != ERR_OK) || (p == NULL) || (client == NULL)) {
		if (p != NULL) {
			tcp_recved(tpcb, p->tot_len);
			pbuf_free(p);
		}
		if (((client != NULL) && (err != ERR_OK)) || (p == NULL)) {
			client_tcp_close(tpcb, client);
		}
		return ERR_MEM;
	}
	
	
	client->recv.segment.data.content = malloc(p->tot_len);	
	if (client->recv.segment.data.content == NULL)
	{
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);	
		client_tcp_close(tpcb, client);
		return ERR_MEM;
	}
	
	pbuf_copy_partial(p, client->recv.segment.data.content, p->tot_len, 0);
	
	if (((char*)client->recv.segment.data.content)[0] == 'S' && ((char*)client->recv.segment.data.content)[1] == 'L'  && ((char*)client->recv.segment.data.content)[2] == 'T')
	{
		client->recv.segment.command = ((char*)client->recv.segment.data.content)[3];
		
		if (client->recv.segment.command == FORMAT)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 
			const char *reply = "FORMAT RECEIVED...\n";
			tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
		}
		
		else if (client->recv.segment.command == OPEN)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 
			const char *reply = "OPEN RECEIVED...\n";
			tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
		}
		else if (client->recv.segment.command == CLOSE)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 	
			const char *reply = "CLOSE RECEIVED...\n";
			tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
		}

		else if (client->recv.segment.command == DELETE)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 	
			const char *reply = "DELETE RECEIVED...\n";
			tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);	
		}		
		else if (client->recv.segment.command == READ)
		{
			client->recv.segment.pos_in_file = (((uint8_t*)client->recv.segment.data.content)[7] << 24) | (((uint8_t*)client->recv.segment.data.content)[6] << 16) | 
											   (((uint8_t*)client->recv.segment.data.content)[5] << 8)  | (((uint8_t*)client->recv.segment.data.content)[4] << 0);
			
			
			client->recv.segment.data.len = (((uint8_t*)client->recv.segment.data.content)[11] << 24) | (((uint8_t*)client->recv.segment.data.content)[10] << 16) | 
											(((uint8_t*)client->recv.segment.data.content)[9] << 8)   | (((uint8_t*)client->recv.segment.data.content)[8] << 0);	
			
			client->send.request = true;
			
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL;
			
			const char *reply = "SLT";
			tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);	
		}
		else if (client->recv.segment.command == WRITE)
		{
			
			client->recv.segment.pos_in_file = (((uint8_t*)client->recv.segment.data.content)[7] << 24) | (((uint8_t*)client->recv.segment.data.content)[6] << 16) | 
											   (((uint8_t*)client->recv.segment.data.content)[5] << 8)  | (((uint8_t*)client->recv.segment.data.content)[4] << 0);	
			
			client->recv.segment.pos_data = 8;
			client->recv.segment.data.len = p->tot_len - 8;
					
			const char *reply = "WRITE RECEIVED...\n";
			tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);	
		}
		
	}
	else
	{
		client->recv.segment.command = WRITE;
		client->recv.segment.pos_data = 0;
		client->recv.segment.data.len = p->tot_len;
		client->recv.segment.pos_in_file = POS_CONTINUE;
		
		const char *reply = "WRITE RECEIVED...\n";
		tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);	
	}

	xQueueSendToBack(xBuffLoadf, &client->recv.segment, portMAX_DELAY);

	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);
	return ERR_OK;
}

err_t server_sent_tcp(void* arg, struct tcp_pcb* tpcb, uint16_t len)
{
	client_tcp_t* client = (client_tcp_t*)arg;
	
	if (client->send.request == true)
	{
		if (xQueueReceive(xBuffSendf, &client->send, portMAX_DELAY) == pdPASS)
		{
			if (client->send.data.content == NULL)
			{
				client->send.request = false;
			}
			else
			{
				tcp_write(tpcb, client->send.data.content, client->send.data.len, TCP_WRITE_FLAG_COPY);	 
				free(client->send.data.content);		
			}
		}
	}
	return ERR_OK; 
}
err_t client_tcp_close(struct tcp_pcb *cl_tpcb, client_tcp_t* client)
{
	if (client != NULL)
	{
		free(client);
	}
	
	if (cl_tpcb != NULL)
	{
		tcp_arg(cl_tpcb, NULL);
		tcp_sent(cl_tpcb, NULL);
		tcp_recv(cl_tpcb, NULL);
		if (tcp_close(cl_tpcb) == ERR_OK) {
			if (SLT_server.count_client > 0)
				SLT_server.count_client--;
			if (SLT_server.count_client <= 0)
				pwm_start(); 
			
			return ERR_OK;
		}
	}
	return ERR_OK;
}

void wifi_handler(void* buff, uint16_t len)
{
	char* buf = (char*)buff;
	
	int16_t index_ssid = -1, index_pass = -1;
	
	for (int16_t i = 0; i < len - 4; i++)
	{
		if (buf[i] == 's' && buf[i + 1] == 's' && buf[i + 2] == 'i' && 
		buf[i + 3] == 'd' && buf[i + 4] == '=')
			index_ssid = i;
		
		if (buf[i] == 'p' && buf[i + 1] == 'a' && buf[i + 2] == 's' && 
		buf[i + 3] == 's' && buf[i + 4] == '=')
			index_pass = i;
	}
	
	if (index_pass == -1 || index_ssid == -1)
	{
		return;
	}
	
	else
	{
		
		int i_ssid = 0, i_pass = 0;
		
		for (int i = 0; i < len; i++)
		{
			if (i > (index_ssid + 4) && i < index_pass)
			{
				tcp_wifi_cred.ssid[i - (index_ssid + 5)] = buf[i];
				i_ssid++;
			}
			if (i > (index_pass + 4)) {
				tcp_wifi_cred.pass[i - (index_pass + 5)] = buf[i];
				i_pass++;
			}
		}
		
		tcp_wifi_cred.ssid[i_ssid] = '\0';
		tcp_wifi_cred.pass[i_pass] = '\0'; 
		
		wifi_cred.retry_connect = REQEST_FROM_USER;
		
		xSemaphoreGive(xTryConnectWifi);

	}
}