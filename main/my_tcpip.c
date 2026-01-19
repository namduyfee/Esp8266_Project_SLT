#include "my_tcpip.h"
#include "my_lib.h"

/*!
 *	@brief	tcp handler
 *	
 *	@details
 *		use dynamic memory and queue to transport data between tcp handler and task_tcp_file_bin in main
 *		
 *		Frame : "TCP" + CMD + OFSET (CMD = R, W) + SIZE (CMD = R, W) + DATA (CMD = W)	
 *		
 *	@note
 *		only one client at a time
 */

static err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err);
static err_t tcp_poll_cb(void* arg, struct tcp_pcb* tpcb); 
static void tcp_error_cb(void *arg, err_t err);
static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err); 
static err_t tcp_sent_cb(void* arg, struct tcp_pcb* tpcb, uint16_t len);
static err_t tcp_close_client(struct tcp_pcb *cl_tpcb, tcp_client_t* client); 


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
	
	SLT.server.send.buf = NULL;
	SLT.server.client = NULL;
	
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
	
	tcp_client_t* newclient = NULL; 
	newclient = malloc(sizeof(tcp_server_t));
	
	
	if (newclient != NULL)
	{
		 
		newclient->tpcb = newpcb; 
		newclient->tpcb_server = server->tpcb;
		newclient->send.request = false;
		newclient->lastTick = xTaskGetTickCount(); 
		
		SLT.server.client = newclient;
		
		tcp_recv(newpcb, tcp_recv_cb);
		tcp_sent(newpcb, tcp_sent_cb);		
		tcp_arg(newpcb, newclient);
		tcp_err(newpcb, tcp_error_cb);
		tcp_poll(newpcb, tcp_poll_cb, TCP_POLL_CYCLE); 
		
		if(SLT.server.count_client < SLT.server.max_client)
			SLT.server.count_client++;
		return ERR_OK;
	}
	pwm_start();
	
	tcp_close(newpcb);
	return ERR_MEM;
	
} 
/**
 * @brief	tcp error callback
 * 
 * @note
 *	- not call tcp_write(), tcp_close()... in function
 */
static void tcp_error_cb(void *arg, err_t err)
{
	tcp_client_t* client = (tcp_client_t*)arg;
	if (client != NULL)
	{
		free(client);
		SLT.server.client = NULL;
	}
	
	if (SLT.server.count_client > 0)
		SLT.server.count_client--;
	if (SLT.server.count_client <= 0)
		pwm_start();
	
}

/**
 * @brief	tcp poll callback
 */
static err_t tcp_poll_cb(void* arg, struct tcp_pcb* tpcb)
{
	tcp_client_t* client = (tcp_client_t*)arg;
	
	if (xTaskGetTickCount() - client->lastTick > pdMS_TO_TICKS(TCP_AUTO_DIS_MS))
	{
		tcp_close_client(tpcb, client);
	}
	return ERR_OK;
}
/**
 *	@brief	tcp recv callback
 *	
 *	@details
 *
 */
static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err)
{
	tcp_client_t* client = (tcp_client_t*)arg;
	
	if ((err != ERR_OK) || (p == NULL) || (client == NULL)) {
		if (p != NULL) {
			tcp_recved(tpcb, p->tot_len);
			pbuf_free(p);
		}
		if (((client != NULL) && (err != ERR_OK)) || (p == NULL)) {
			tcp_close_client(tpcb, client);
		}
		return ERR_MEM;
	}
	
	
	client->recv.segment.buf.data = malloc(p->tot_len);	
	if (client->recv.segment.buf.data == NULL)
	{
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);	
		tcp_close_client(tpcb, client);
		return ERR_MEM;
	}
	
	pbuf_copy_partial(p, client->recv.segment.buf.data, p->tot_len, 0);
	
	if (((char*)client->recv.segment.buf.data)[0] == 'T' && ((char*)client->recv.segment.buf.data)[1] == 'C'  && 
	    ((char*)client->recv.segment.buf.data)[2] == 'P')
	{
		client->recv.segment.command = ((char*)client->recv.segment.buf.data)[3];
		
		if (client->recv.segment.command == TCP_OPEN)
		{
			free(client->recv.segment.buf.data);
			client->recv.segment.buf.data = NULL; 
		}
		else if (client->recv.segment.command == TCP_CLOSE)
		{
			free(client->recv.segment.buf.data);
			client->recv.segment.buf.data = NULL; 	
		}
		else if (client->recv.segment.command == TCP_DELETE)
		{
			free(client->recv.segment.buf.data);
			client->recv.segment.buf.data = NULL; 	
		}		
		else if (client->recv.segment.command == TCP_READ)
		{
			if (p->tot_len >= 8)
			{
				client->recv.segment.pos_in_file = (((uint8_t*)client->recv.segment.buf.data)[7] << 24) | 
											   (((uint8_t*)client->recv.segment.buf.data)[6] << 16) | 
											   (((uint8_t*)client->recv.segment.buf.data)[5] << 8)  | 
				                               (((uint8_t*)client->recv.segment.buf.data)[4] << 0);			
			}
			else 
				client->recv.segment.pos_in_file = 0; 	

			
			if (p->tot_len >= 12)
			{
				client->recv.segment.buf.len = (((uint8_t*)client->recv.segment.buf.data)[11] << 24) | 
											   (((uint8_t*)client->recv.segment.buf.data)[10] << 16) | 
											   (((uint8_t*)client->recv.segment.buf.data)[9] << 8)   | 
											   (((uint8_t*)client->recv.segment.buf.data)[8] << 0);	
			}
			else 
				client->recv.segment.buf.len = 0;
			
			free(client->recv.segment.buf.data);
			client->recv.segment.buf.data = NULL;
		}
		else if (client->recv.segment.command == TCP_WRITE)
		{
			if (p->tot_len >= 8)
			{
				client->recv.segment.pos_in_file = (((uint8_t*)client->recv.segment.buf.data)[7] << 24) | 
												   (((uint8_t*)client->recv.segment.buf.data)[6] << 16) | 
												   (((uint8_t*)client->recv.segment.buf.data)[5] << 8)  | 
												   (((uint8_t*)client->recv.segment.buf.data)[4] << 0);	
			}
			else 
				client->recv.segment.pos_in_file = 0;
			
			if (p->tot_len >= 12)
			{
				client->recv.segment.tot_len = (((uint8_t*)client->recv.segment.buf.data)[11] << 24) | 
											   (((uint8_t*)client->recv.segment.buf.data)[10] << 16) | 
											   (((uint8_t*)client->recv.segment.buf.data)[9] << 8)   | 
											   (((uint8_t*)client->recv.segment.buf.data)[8] << 0);
			}
			else 
				client->recv.segment.tot_len = 0; 
			
			if (p->tot_len > 12)
			{
				client->recv.segment.pos_data = 12;
				client->recv.segment.buf.len = p->tot_len - 12;
			}
			else
			{
				client->recv.segment.pos_data = 0;
				client->recv.segment.buf.len = 0;
			}

		}
		
	}
	else
	{
		client->recv.segment.command = TCP_NONE;
		client->recv.segment.pos_data = 0;
		client->recv.segment.buf.len = p->tot_len;
		client->recv.segment.pos_in_file = POS_CONTINUE;
		
		client->recv.segment.tot_len = REMAINING; 

	}
	
	client->lastTick = xTaskGetTickCount();
	
	xQueueSendToBack(xBuffLoadf, &client->recv.segment, portMAX_DELAY);

	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);
	return ERR_OK;
}
/**
 * @brief	tcp sent callback
 */
static err_t tcp_sent_cb(void* arg, struct tcp_pcb* tpcb, uint16_t len)
{
	tcp_client_t* client = (tcp_client_t*)arg;
	client->lastTick = xTaskGetTickCount();
	return ERR_OK; 
}

/**
 * @brief handler close tcp
 */
static err_t tcp_close_client(struct tcp_pcb *cl_tpcb, tcp_client_t* client)
{

	if (client != NULL)
	{
		free(client);
		SLT.server.client = NULL;
	}
	
	if (cl_tpcb != NULL)
	{
		tcp_arg(cl_tpcb, NULL);
		tcp_sent(cl_tpcb, NULL);
		tcp_recv(cl_tpcb, NULL);
		tcp_err(cl_tpcb, NULL);
		tcp_poll(cl_tpcb, NULL, 0);
		
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

/**
 * @brief tcp send callback is called by tcpip_callback 
 */
void tcp_send_cb(void* arg)
{
	tcp_buf_t* tSendBuf = (tcp_buf_t*)arg;
	
	if (SLT.server.client->tpcb != NULL && SLT.server.client->tpcb->state != CLOSED)
	{
		size_t remaining = tSendBuf->len; 
		
		while (remaining > 0)
		{
			if (tSendBuf->data == NULL || tSendBuf == NULL)
				break;
			
			if (tcp_sndqueuelen(SLT.server.client->tpcb) < TCP_SND_QUEUELEN)
			{
				size_t numByteEmpty = tcp_sndbuf(SLT.server.client->tpcb);
				
				if (numByteEmpty > 0)
				{
					size_t to_write = remaining <= numByteEmpty ? remaining : numByteEmpty;
					if (tcp_write(SLT.server.client->tpcb,
						(uint8_t*)tSendBuf->data + (tSendBuf->len - remaining),
						to_write,
						TCP_WRITE_FLAG_COPY) == ERR_OK)
					{
						remaining = remaining - to_write;
					}
				}
			}
		}
		
		if (tSendBuf->data != NULL)
			free(tSendBuf->data);
		if (tSendBuf != NULL)
			free(tSendBuf);
	}
	else
	{
		if (tSendBuf->data != NULL)
			free(tSendBuf->data);
		if (tSendBuf != NULL)
			free(tSendBuf);		
	}

}

#define TCP_LEN_RETURN_CMD 5
void tcp_ret_cmd(command_tcp_t cmd, uint8_t state)
{
	
	uint8_t* retCmd = malloc(sizeof(uint8_t) * TCP_LEN_RETURN_CMD); 
	if (retCmd == NULL)
		return;
	uint8_t header[TCP_LEN_RETURN_CMD] = {'T', 'C', 'P'};
	header[3] = (uint8_t)cmd; 			
	header[4] = state; 
	
	memcpy(retCmd, header, TCP_LEN_RETURN_CMD); 
					
	tcp_buf_t* tSendBuf = malloc(sizeof(tcp_buf_t));
	if (tSendBuf == NULL)
	{
		if (retCmd != NULL)
			free(retCmd);
		return;
	}
	
	tSendBuf->data = retCmd;
	tSendBuf->len = TCP_LEN_RETURN_CMD;

	if (tcpip_callback(tcp_send_cb, tSendBuf) != ERR_OK)
	{
		if (tSendBuf->data != NULL)
			free(tSendBuf->data);
		if (tSendBuf != NULL)
			free(tSendBuf);
	} 
	
}