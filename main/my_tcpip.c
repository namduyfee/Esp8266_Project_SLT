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
	
	server_listen_tpcb = tcp_listen(server_listen_tpcb);
	
	
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
	
	tcp_accept(server_listen_tpcb, server_accept_tcp);
	
	tcp_arg(server_listen_tpcb, &SLT_server);
	return ERR_OK;
}

err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err)
{
	
	server_tcp_t* server = (server_tcp_t*)arg;
	if (server->count_client >= server->max_client)
	{
		return -1;
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
		return ERR_OK;
	}
	
	client->segment_recv.content =  (char*)malloc(p->tot_len);
	client->segment_recv.len = p->tot_len;
	
	pbuf_copy_partial(p, client->segment_recv.content, p->tot_len, 0);
	
	xQueueSendToBack(xBuffLoadf, &client->segment_recv, 0);
	
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