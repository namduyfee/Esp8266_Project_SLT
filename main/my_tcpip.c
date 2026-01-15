#include "my_tcpip.h"
#include "my_lib.h"

/*!
 *	@brief	tcp handler
 *	
 *	@details
 *		use dynamic memory and queue to transport data between tcp handler and task_tcp_file_bin in main
 *		
 *		Frame : "SLT" + CMD + OFSET (CMD = R, W) + SIZE (CMD = R, W) + DATA (CMD = W)	
 *		
 *	@note
 *		only one client at a time
 */

static err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err);
static err_t server_recv_tcp(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err);
static err_t server_sent_tcp(void* arg, struct tcp_pcb* tpcb, uint16_t len);
static err_t client_tcp_close(struct tcp_pcb *cl_tpcb, tcp_client_t* client); 


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
		SLT.server.tpcb = 0;
		printf("Error listen!\r\n");
		return ERR_MEM;
	}
	
	SLT.server.tpcb = server_listen_tpcb;
	SLT.server.port = port;
	SLT.server.max_client = max_client;
	SLT.server.count_client = 0; 
	SLT.server.recv.current_pos_file = 0; 
	SLT.server.recv.tot_len = 0;
	
	tcp_accept(server_listen_tpcb, server_accept_tcp);
	
	tcp_arg(server_listen_tpcb, &SLT.server);
	return ERR_OK;
}

static err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err)
{
	
	pwm_stop(0);
	
	tcp_server_t* server = (tcp_server_t*)arg;
	if (server->count_client >= server->max_client)
	{
		tcp_close(newpcb);
		
		pwm_start();  
		
		return ERR_MEM;
	}
	
	tcp_client_t* newclient = 0; 
	newclient = malloc(sizeof(tcp_server_t));
	
	
	if (newclient)
	{
		 
		newclient->tpcb = newpcb; 
		newclient->tpcb_server = server->tpcb;
		newclient->send.request = false;
//		SLT.server.client = newclient;
		
		tcp_recv(newpcb, server_recv_tcp);
		tcp_sent(newpcb, server_sent_tcp);		
		tcp_arg(newpcb, newclient);
		tcp_err(newpcb, NULL);
		if(SLT.server.count_client < SLT.server.max_client)
			SLT.server.count_client++;
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
static err_t server_recv_tcp(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err)
{
	tcp_client_t* client = (tcp_client_t*)arg;
	
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
	
	if (((char*)client->recv.segment.data.content)[0] == 'S' && ((char*)client->recv.segment.data.content)[1] == 'L'  && 
	    ((char*)client->recv.segment.data.content)[2] == 'T')
	{
		client->recv.segment.command = ((char*)client->recv.segment.data.content)[3];
		
		if (client->recv.segment.command == FORMAT)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 
		}
		
		else if (client->recv.segment.command == OPEN)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 
		}
		else if (client->recv.segment.command == CLOSE)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 	
		}

		else if (client->recv.segment.command == DELETE)
		{
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL; 	
		}		
		else if (client->recv.segment.command == READ)
		{
			client->recv.segment.pos_in_file = (((uint8_t*)client->recv.segment.data.content)[7] << 24) | (((uint8_t*)client->recv.segment.data.content)[6] << 16) | 
											   (((uint8_t*)client->recv.segment.data.content)[5] << 8)  | (((uint8_t*)client->recv.segment.data.content)[4] << 0);
			
			
			client->recv.segment.data.len = (((uint8_t*)client->recv.segment.data.content)[11] << 24) | (((uint8_t*)client->recv.segment.data.content)[10] << 16) | 
											(((uint8_t*)client->recv.segment.data.content)[9] << 8)   | (((uint8_t*)client->recv.segment.data.content)[8] << 0);	
			
//			client->send.request = true;
//						
//			uint8_t header [12] = {'S', 'L', 'T'}; header[3] = WRITE;
//			/* offset */
//			header[4] = ((uint8_t*)client->recv.segment.data.content)[4]; 
//			header[5] = ((uint8_t*)client->recv.segment.data.content)[5]; 
//			header[6] = ((uint8_t*)client->recv.segment.data.content)[6]; 
//			header[7] = ((uint8_t*)client->recv.segment.data.content)[7]; 
//			/* size */
//			header[8]  = ((uint8_t*)client->recv.segment.data.content)[8]; 
//			header[9]  = ((uint8_t*)client->recv.segment.data.content)[9]; 
//			header[10] = ((uint8_t*)client->recv.segment.data.content)[10]; 
//			header[11] = ((uint8_t*)client->recv.segment.data.content)[11];			
//			
//			tcp_write(tpcb, header, sizeof(header), TCP_WRITE_FLAG_COPY);	
			
			free(client->recv.segment.data.content);
			client->recv.segment.data.content = NULL;
		}
		else if (client->recv.segment.command == WRITE)
		{
			
			client->recv.segment.pos_in_file = (((uint8_t*)client->recv.segment.data.content)[7] << 24) | (((uint8_t*)client->recv.segment.data.content)[6] << 16) | 
											   (((uint8_t*)client->recv.segment.data.content)[5] << 8)  | (((uint8_t*)client->recv.segment.data.content)[4] << 0);	
			
			client->recv.segment.tot_len = (((uint8_t*)client->recv.segment.data.content)[11] << 24) | (((uint8_t*)client->recv.segment.data.content)[10] << 16) | 
										   (((uint8_t*)client->recv.segment.data.content)[9] << 8)   | (((uint8_t*)client->recv.segment.data.content)[8] << 0);
			
			client->recv.segment.pos_data = 12;
			client->recv.segment.data.len = p->tot_len - 12;
					
		}
		
	}
	else
	{
		client->recv.segment.command = WRITE;
		client->recv.segment.pos_data = 0;
		client->recv.segment.data.len = p->tot_len;
		client->recv.segment.pos_in_file = POS_CONTINUE;
		
		client->recv.segment.tot_len = REMAINING; 

	}
	if (client->recv.segment.command != READ)
	{
		const char* rely = " ack\n";
		tcp_write(tpcb, rely, strlen(rely), TCP_WRITE_FLAG_COPY);
		tcp_output(tpcb); 
	}

	xQueueSendToBack(xBuffLoadf, &client->recv.segment, portMAX_DELAY);

	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);
	return ERR_OK;
}

static err_t server_sent_tcp(void* arg, struct tcp_pcb* tpcb, uint16_t len)
{
	
//	tcp_client_t* client = (tcp_client_t*)arg;
//	
//	if (client->send.request == true)
//	{
//		if (xQueueReceive(xBuffSendf, &client->send, portMAX_DELAY) == pdPASS)
//		{
//			if (client->send.data.content == NULL)
//			{
//				client->send.request = false;
//			}
//			else
//			{
//				tcp_write(tpcb, client->send.data.content, client->send.data.len, TCP_WRITE_FLAG_COPY);	 
//				if (client->send.data.content != NULL)
//					free(client->send.data.content);		
//			}
//		}
//	}
	
	return ERR_OK; 
}
static err_t client_tcp_close(struct tcp_pcb *cl_tpcb, tcp_client_t* client)
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
			if (SLT.server.count_client > 0)
				SLT.server.count_client--;
			if (SLT.server.count_client <= 0)
				pwm_start(); 
			
			return ERR_OK;
		}
	}
	return ERR_OK;
}