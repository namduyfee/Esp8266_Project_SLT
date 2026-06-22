#include "my_tcpip.h"
#include "my_lib.h"

/*!
 *	@brief	tcp handler
 *	
 *	@details
 *		use dynamic memory and queue to transport data between tcp handler and task_tcp_file_bin in main
 *		
 *		Frame : "TCP" + CMD + OFSET (CMD = R, W) + DATA (CMD = W) 
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
		return ERR_MEM;
	}
	
	SLT.server.tpcb = server_listen_tpcb;
	SLT.server.port = port;
	SLT.server.max_client = max_client;
	SLT.server.count_client = 0; 

	
	SLT.server.p_client = NULL;
	
	tcp_accept(server_listen_tpcb, server_accept_tcp);
	
	tcp_arg(server_listen_tpcb, &SLT.server);
	return ERR_OK;
}

static err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err)
{
	tcp_server_t* server = (tcp_server_t*)arg;
	
	if (server->count_client >= server->max_client)
	{
		tcp_close(newpcb);
		return ERR_MEM;
	}
	
	tcp_client_t* newclient = NULL; 
	newclient = malloc(sizeof(tcp_server_t));
	
	if (newclient != NULL)
	{
		newclient->tpcb = newpcb; 
		newclient->tpcb_server = server->tpcb;
		newclient->lastTick = xTaskGetTickCount(); 
		
		SLT.server.p_client = newclient;
		
		tcp_recv(newpcb, tcp_recv_cb);
		tcp_sent(newpcb, tcp_sent_cb);		
		tcp_arg(newpcb, newclient);
		tcp_err(newpcb, tcp_error_cb);
		tcp_poll(newpcb, tcp_poll_cb, TCP_POLL_CYCLE); 
		
		if (SLT.server.count_client < SLT.server.max_client)
			SLT.server.count_client++;
		return ERR_OK;
	}
		
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
		SLT.server.p_client = NULL;
	}
	
	if (SLT.server.count_client > 0)
		SLT.server.count_client--;
	
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
	client->lastTick = xTaskGetTickCount();
	
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
	
	tcp_buf_t recv_buf; 
	
	recv_buf.data = malloc(p->tot_len);	
	
	if (recv_buf.data == NULL)
	{
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);	
		tcp_close_client(tpcb, client);
		return ERR_MEM;
	}
	
	pbuf_copy_partial(p, recv_buf.data, p->tot_len, 0);
	recv_buf.tot_byte = p->tot_len; 
	
	
	if (recv_buf.data[0] == 'T' && recv_buf.data[1] == 'C' && 
	    recv_buf.data[2] == 'P')
	{
		if (recv_buf.data[3] == TCP_EFF_ASYNCH || recv_buf.data[3] == TCP_EFF_SYNCH)
		{
			
			if (recv_buf.data[3] == TCP_EFF_SYNCH)
				SLT.effMana.master_mode = EFF_SYNCHRONOUS;
			else if(recv_buf.data[3] == TCP_EFF_ASYNCH)
				SLT.effMana.master_mode = EFF_ASYNCHRONOUS;
			
			xSemaphoreGive(xMasterModeEff);
			
		}
		else
		{
			file_request_t eff = {
				.cmd = F_NONE,
				.source = F_TCP_SOURCE
			};
		
			if (recv_buf.data[3] == TCP_OPF)
			{
				eff.cmd = F_OP; 
			}
		
			else if (recv_buf.data[3] == TCP_CLSF)
			{
				eff.cmd = F_CLS; 
			}
			else if (recv_buf.data[3] == TCP_DLTF)
			{
				eff.cmd = F_DLT; 
			}		
			else if (recv_buf.data[3] == TCP_RDF)
			{
				eff.cmd = F_RD;
			
				uint8_t index_offset = 4;
				if (recv_buf.tot_byte >= (index_offset + 4))
				{
					eff.read.offset = (recv_buf.data[index_offset + 3] << 24) | 
									  (recv_buf.data[index_offset + 2] << 16) | 
									  (recv_buf.data[index_offset + 1] << 8)  | 
									  (recv_buf.data[index_offset] << 0);			
				}
				else 
					eff.read.offset = 0; 	

				uint8_t index_len = 8;
				if (recv_buf.tot_byte >= (index_len + 4))
				{
					eff.read.tot_byte = (recv_buf.data[index_len + 3] << 24) | 
								   (recv_buf.data[index_len + 2] << 16) | 
								   (recv_buf.data[index_len + 1] << 8)  | 
								   (recv_buf.data[index_len] << 0);	
				}
				else 
					eff.read.tot_byte = 0;
			
			}
			else if (recv_buf.data[3] == TCP_ST_WRF)
			{
				eff.cmd = F_ST_WR;
				
				uint8_t index_offset = 4;
				if (recv_buf.tot_byte >= (index_offset + 4))
				{
					
					eff.write_start.offset =	(recv_buf.data[index_offset + 3] << 24) | 
												(recv_buf.data[index_offset + 2] << 16) | 
												(recv_buf.data[index_offset + 1] << 8)  | 
												(recv_buf.data[index_offset] << 0);
				}
				else 
					eff.write_start.offset = 0; 
			
			
				uint8_t index_tot_len = 8;
				if (recv_buf.tot_byte >= (index_tot_len + 4))
				{
					eff.write_start.tot_byte = (recv_buf.data[index_tot_len + 3] << 24) | 
											  (recv_buf.data[index_tot_len + 2] << 16) | 
											  (recv_buf.data[index_tot_len + 1] << 8)  | 
											  (recv_buf.data[index_tot_len] << 0);	
				}
				else 
					eff.write_start.tot_byte = 0;
 
			}
			else if (recv_buf.data[3] == TCP_WRF)
			{
				eff.cmd = F_WR;
			
				uint8_t index_offset = 4;	
				if (recv_buf.tot_byte >= (index_offset + 4))
				{
					eff.write.offset = (recv_buf.data[index_offset + 3] << 24) | 
									   (recv_buf.data[index_offset + 2] << 16) | 
									   (recv_buf.data[index_offset + 1] << 8)  | 
									   (recv_buf.data[index_offset] << 0);	
				}
				else 
					eff.write.offset = 0;
			
				if (recv_buf.tot_byte > 8)
				{
					eff.write.buf.tot_byte = recv_buf.tot_byte - 8; 
					eff.write.buf.data = malloc(sizeof(uint8_t) * eff.write.buf.tot_byte);
					if (eff.write.buf.data != NULL)
					{
						memcpy(eff.write.buf.data, (uint8_t*)recv_buf.data + 8, eff.write.buf.tot_byte); 
					}
				}
				else
				{
					eff.write.buf.data = NULL; 
					eff.write.buf.tot_byte = 0; 
				}
			
			}
			else if (recv_buf.data[3] == TCP_END_WRF)
			{
			
				eff.cmd = F_END_WR; 
				eff.write_end.checksum = (((uint8_t*)recv_buf.data)[5] << 8)  | 
										 (((uint8_t*)recv_buf.data)[4] << 0);
			}
		
			if (eff.cmd != F_NONE)
				xQueueSendToBack(xTcpLoadf, &eff, pdMS_TO_TICKS(4000));
		}

	}
	
	
	if (recv_buf.data != NULL)
		free(recv_buf.data);
	recv_buf.data = NULL;
	

	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);
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
		SLT.server.p_client = NULL;
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

			return ERR_OK;
		}
	}
	
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
 * @brief tcp send callback is called by tcpip_callback 
 */
void tcp_send_cb(void* arg)
{	
	tcp_buf_t* buf = (tcp_buf_t*)arg;
	struct tcp_pcb* pcb = SLT.server.p_client->tpcb;
	
	if (pcb == NULL || pcb->state == CLOSED)
		return;

	err_t ret = tcp_write(pcb, buf->data, buf->tot_byte, TCP_WRITE_FLAG_COPY);

	if (ret == ERR_OK)
	{
		tcp_output(pcb);	
		SLT.server.send.sent = true; 
	}
	else
	{
		SLT.server.send.sent = false; 
	}
	xSemaphoreGive(xTcpSwitchBufSend);
	
}

/**
 *	@brief make frame tcp
 */

#define TCP_LEN_HEADER 4
tcp_buf_t* tcp_make_frame(tcp_command_t cmd, void* data, uint32_t byte_data)
{
	
	uint32_t byte_of_frame = (data != NULL && byte_data != 0) ? TCP_LEN_HEADER + byte_data : TCP_LEN_HEADER; 
	uint8_t* retCmd = malloc(byte_of_frame);

	if (retCmd == NULL)
		return NULL;
	
	uint8_t header[TCP_LEN_HEADER] = {'T', 'C', 'P'};
	

	header[3] = (uint8_t)cmd;
	
	if (cmd == TCP_ACK)
		header[3] = 'Y';
	else if (cmd == TCP_NACK)
		header[3] = 'N';
	else if (cmd == TCP_RETURN_ID_RECEIVED)
	{
		uint32_t tem = *((uint32_t*)data);
		if (tem == 7) 
		{
			header[3] = 'H'; 
		}
	}
	
	memcpy(retCmd, header, TCP_LEN_HEADER); 
				
	if (data != NULL && byte_data != 0)
	{
		memcpy(&retCmd[TCP_LEN_HEADER], data, byte_data); 
	}
	
	tcp_buf_t* tSendBuf = malloc(sizeof(tcp_buf_t));
	if (tSendBuf == NULL)
	{
		if (retCmd != NULL)
			free(retCmd);
		return NULL;
	}
	
	tSendBuf->data = retCmd;
	tSendBuf->tot_byte = byte_of_frame;
	
	return tSendBuf; 
}

/**
 *	@brief make frame return read 
 */

#define TCP_LEN_HEADER_RET_READ 12
tcp_buf_t* tcp_make_ret_read(void*data, uint32_t len, uint32_t offset)
{
	uint8_t* retCmd = malloc(sizeof(uint8_t) * (len + TCP_LEN_HEADER_RET_READ)) ; 
	if (retCmd == NULL)
		return NULL;
	
	uint8_t header[TCP_LEN_HEADER_RET_READ] = {'T', 'C', 'P', TCP_RET_RDF}; 	
	memcpy(&header[4], &offset, 4);
	memcpy(&header[8], &len, 4); 
	
	memcpy(retCmd, header, TCP_LEN_HEADER_RET_READ); 
	memcpy(retCmd + TCP_LEN_HEADER_RET_READ, data, len); 
					
	tcp_buf_t* tSendBuf = malloc(sizeof(tcp_buf_t));
	if (tSendBuf == NULL)
	{
		if (retCmd != NULL)
			free(retCmd);
		return NULL;
	}
	
	tSendBuf->data = retCmd;
	tSendBuf->tot_byte = len + TCP_LEN_HEADER_RET_READ;
	
	return tSendBuf;
}