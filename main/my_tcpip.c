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
	
	server_tcp_t* server = (server_tcp_t*)arg;
	if (server->count_client >= server->max_client)
	{
		tcp_close(newpcb);
		return ERR_MEM;
	}
	
	client_tcp_t* newclient = 0; 
	newclient = malloc(sizeof(server_tcp_t));
	
	if (newclient)
	{
		newclient->tpcb = newpcb; 
		newclient->tpcb_server = server->tpcb;

		
		tcp_recv(newpcb, server_recv_tcp);
		tcp_sent(newpcb, server_sent_tcp);		
		tcp_arg(newpcb, newclient);
		tcp_err(newpcb, NULL);
		if(SLT_server.count_client < SLT_server.max_client)
			SLT_server.count_client++;
		return ERR_OK;
	}

	tcp_close(newpcb);
	return ERR_MEM;
	
} 

uint8_t start = 0; 
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
	
	
	client->recv.segment.content = malloc(p->tot_len);	
	if (client->recv.segment.content == NULL)
	{
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);	
		client_tcp_close(tpcb, client);
		return ERR_MEM;
	}
	
	client->recv.segment.len = p->tot_len;
	
	pbuf_copy_partial(p, client->recv.segment.content, p->tot_len, 0);
	
	if (((char*)client->recv.segment.content)[0] == 'S' && ((char*)client->recv.segment.content)[1] == 'L'  && ((char*)client->recv.segment.content)[2] == 'T')
	{
//		client->recv.segment.pos_data = 7; 
//		client->recv.segment.len -= 7; 
		
		client->recv.segment.pos_data = 0; 	
		client->recv.segment.pos_in_file = (((uint8_t*)client->recv.segment.content)[6] << 3) | (((uint8_t*)client->recv.segment.content)[5] << 2) | (((uint8_t*)client->recv.segment.content)[4] << 1) 
																								| (((uint8_t*)client->recv.segment.content)[3] << 0); 
	}
	else
	{
		client->recv.segment.pos_data = 0;
		client->recv.segment.pos_in_file = POS_CONTINUE;
	}

	xQueueSendToBack(xBuffLoadf, &client->recv.segment, 0);
	
	const char *reply = "RECEIVED...\n";
	tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);
	return ERR_OK;
}

err_t server_sent_tcp(void* arg, struct tcp_pcb* tpcb, uint16_t len)
{
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