#include "my_tcpip.h"
#include "my_lib.h"
#include <stdlib.h>

/*!
 *	@brief	tcp handler
 *
 *	@details
 *
 *	@note
 *		use dynamic memory and queue to transport data between tcp handler and task_tcp_file_bin in main
 *		only one client at a time
 */

static err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err);
static err_t tcp_poll_cb(void* arg, struct tcp_pcb* tpcb);
static void tcp_error_cb(void *arg, err_t err);
static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err);
static err_t tcp_sent_cb(void* arg, struct tcp_pcb* tpcb, uint16_t len);
static err_t tcp_close_client(struct tcp_pcb *cl_tpcb, tcp_client_t* client);
static void tcp_process_rx_buffer(tcp_client_t* client);
static void tcp_process_frame(uint8_t cmd, const uint8_t* payload, uint32_t payload_len);
static uint32_t tcp_read_u32_le(const uint8_t* data);
static void tcp_keep_partial_magic(tcp_client_t* client);
static void tcp_free_buf(tcp_buf_t* buf);


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
	newclient = calloc(1, sizeof(tcp_client_t));

	if (newclient != NULL)
	{
		newclient->tpcb = newpcb;
		newclient->tpcb_server = server->tpcb;
		newclient->lastTick = xTaskGetTickCount();
		newclient->rx_len = 0;

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

	if (client == NULL)
		return ERR_OK;

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

	if (client == NULL)
	{
		if (p != NULL)
		{
			tcp_recved(tpcb, p->tot_len);
			pbuf_free(p);
		}
		return ERR_ARG;
	}

	client->lastTick = xTaskGetTickCount();

	if ((err != ERR_OK) || (p == NULL))
	{
		if (p != NULL)
		{
			tcp_recved(tpcb, p->tot_len);
			pbuf_free(p);
		}
		if ((err != ERR_OK) || (p == NULL))
		{
			tcp_close_client(tpcb, client);
		}
		return ERR_MEM;
	}

	uint32_t copied = 0;
	while (copied < p->tot_len)
	{
		tcp_process_rx_buffer(client);

		uint32_t free_len = TCP_RX_BUF_SIZE - client->rx_len;
		if (free_len == 0)
		{
			tcp_recved(tpcb, p->tot_len);
			pbuf_free(p);
			tcp_close_client(tpcb, client);
			return ERR_MEM;
		}

		uint32_t remain = p->tot_len - copied;
		uint32_t copy_len = (remain < free_len) ? remain : free_len;
		uint16_t copied_now = pbuf_copy_partial(p,
							 &client->rx_buf[client->rx_len],
							 (uint16_t)copy_len,
							 (uint16_t)copied);

		if (copied_now == 0)
		{
			tcp_recved(tpcb, p->tot_len);
			pbuf_free(p);
			tcp_close_client(tpcb, client);
			return ERR_MEM;
		}

		client->rx_len += copied_now;
		copied += copied_now;
		tcp_process_rx_buffer(client);
	}

	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);
	return ERR_OK;
}

static uint32_t tcp_read_u32_le(const uint8_t* data)
{
	return ((uint32_t)data[0] << 0)  |
	       ((uint32_t)data[1] << 8)  |
	       ((uint32_t)data[2] << 16) |
	       ((uint32_t)data[3] << 24);
}

static void tcp_keep_partial_magic(tcp_client_t* client)
{
	uint32_t keep = 0;

	if (client->rx_len >= 2 &&
		client->rx_buf[client->rx_len - 2] == 'T' &&
		client->rx_buf[client->rx_len - 1] == 'C')
	{
		keep = 2;
	}
	else if (client->rx_len >= 1 &&
			 client->rx_buf[client->rx_len - 1] == 'T')
	{
		keep = 1;
	}

	if (keep != 0)
	{
		memmove(client->rx_buf, &client->rx_buf[client->rx_len - keep], keep);
	}

	client->rx_len = keep;
}

static void tcp_process_rx_buffer(tcp_client_t* client)
{
	while (client->rx_len >= TCP_FRAME_HEADER_LEN)
	{
		if (client->rx_buf[0] != 'T' || client->rx_buf[1] != 'C' || client->rx_buf[2] != 'P')
		{
			uint32_t next_magic = 1;

			while (next_magic + 2 < client->rx_len)
			{
				if (client->rx_buf[next_magic] == 'T' &&
					client->rx_buf[next_magic + 1] == 'C' &&
					client->rx_buf[next_magic + 2] == 'P')
				{
					break;
				}
				next_magic++;
			}

			if (next_magic + 2 >= client->rx_len)
			{
				tcp_keep_partial_magic(client);
				return;
			}

			memmove(client->rx_buf, &client->rx_buf[next_magic], client->rx_len - next_magic);
			client->rx_len -= next_magic;

			if (client->rx_len < TCP_FRAME_HEADER_LEN)
				return;
		}

		uint8_t cmd = client->rx_buf[3];
		uint32_t payload_len = tcp_read_u32_le(&client->rx_buf[4]);

		if (payload_len > TCP_MAX_PAYLOAD_LEN)
		{
			client->rx_len = 0;
			tcp_queue_nack((tcp_command_t)cmd);
			return;
		}

		uint32_t frame_len = TCP_FRAME_HEADER_LEN + payload_len;

		if (client->rx_len < frame_len)
			return;

		tcp_process_frame(cmd, &client->rx_buf[TCP_FRAME_HEADER_LEN], payload_len);

		if (client->rx_len > frame_len)
		{
			memmove(client->rx_buf, &client->rx_buf[frame_len], client->rx_len - frame_len);
		}
		client->rx_len -= frame_len;
	}
}

static void tcp_process_frame(uint8_t cmd, const uint8_t* payload, uint32_t payload_len)
{

	if (cmd == TCP_EFF_ASYNCH || cmd == TCP_EFF_SYNCH)
	{
		if (cmd == TCP_EFF_SYNCH)
			SLT.effMana.master_mode = EFF_SYNCHRONOUS;
		else
			SLT.effMana.master_mode = EFF_ASYNCHRONOUS;

		xSemaphoreGive(xMasterModeEff);
		return;
	}

	if (cmd == TCP_GET_INF_ESP_MODE)
	{
		request_config_espmode_t tmp = {
			.cmd = TCP_GET_INF_ESP_MODE
		};

		xQueueSendToBack(xConfigEspMode, &tmp, pdMS_TO_TICKS(3000));
		return;
	}

	if (cmd == TCP_SET_INF_ESP_MODE)
	{
		if (payload_len >= 10)
		{
			request_config_espmode_t tmp;

			tmp.cmd = TCP_SET_INF_ESP_MODE;
			tmp.mode = payload[0];
			tmp.my_id = payload[1];

			memset(tmp.gw_code, 0, sizeof(tmp.gw_code));
			tmp.gw_code[8] = '\0';
			memcpy(tmp.gw_code, &payload[2], 8);

			xQueueSendToBack(xConfigEspMode, &tmp, pdMS_TO_TICKS(3000));
		}
		else
		{
			tcp_queue_nack(TCP_SET_INF_ESP_MODE);
		}
		return;
	}

	file_request_t eff = {
		.cmd = F_NONE,
		.source = F_TCP_SOURCE
	};

	if (cmd == TCP_OPF)
	{
		eff.cmd = F_OP;
	}
	else if (cmd == TCP_CLSF)
	{
		eff.cmd = F_CLS;
	}
	else if (cmd == TCP_DLTF)
	{
		eff.cmd = F_DLT;
	}
	else if (cmd == TCP_ST_WRF)
	{
		if (payload_len >= 8)
		{
			eff.cmd = F_ST_WR;
			eff.write_start.offset = tcp_read_u32_le(payload);
			eff.write_start.tot_byte = tcp_read_u32_le(&payload[4]);
		}
	}
	else if (cmd == TCP_WRF)
	{
		if (payload_len >= 4)
		{
			eff.cmd = F_WR;
			eff.write.offset = tcp_read_u32_le(payload);
			eff.write.buf.tot_byte = payload_len - 4;
			eff.write.buf.data = NULL;

			if (eff.write.buf.tot_byte > 0)
			{
				eff.write.buf.data = malloc(sizeof(uint8_t) * eff.write.buf.tot_byte);
				if (eff.write.buf.data != NULL)
				{
					memcpy(eff.write.buf.data, &payload[4], eff.write.buf.tot_byte);
				}
				else
				{
					eff.cmd = F_NONE;
				}
			}
		}
	}
	else if (cmd == TCP_END_WRF)
	{
		if (payload_len >= 2)
		{
			eff.cmd = F_END_WR;
			eff.write_end.checksum = ((uint16_t)payload[1] << 8) | payload[0];
		}
	}

	if (eff.cmd != F_NONE)
	{
		if (xQueueSendToBack(xTcpLoadf, &eff, pdMS_TO_TICKS(4000)) != pdPASS)
		{
			if (eff.cmd == F_WR && eff.write.buf.data != NULL)
			{
				free(eff.write.buf.data);
				eff.write.buf.data = NULL;
			}
			tcp_queue_nack((tcp_command_t)cmd);
		}
	}
	else if (cmd != TCP_NONE)
	{
		tcp_queue_nack((tcp_command_t)cmd);
	}
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

	if (client == NULL)
		return ERR_OK;

	client->lastTick = xTaskGetTickCount();

	return ERR_OK;
}
/**
 * @brief tcp send callback is called by tcpip_callback
 */
void tcp_send_cb(void* arg)
{
	tcp_buf_t* buf = (tcp_buf_t*)arg;
	tcp_client_t* client = SLT.server.p_client;

	if (buf == NULL || client == NULL || client->tpcb == NULL || client->tpcb->state == CLOSED)
	{
		xSemaphoreGive(xTcpSwitchBufSend);
		return;
	}

	err_t ret = tcp_write(client->tpcb, buf->data, buf->tot_byte, TCP_WRITE_FLAG_COPY);

	if (ret == ERR_OK)
	{
		tcp_output(client->tpcb);
	}
	xSemaphoreGive(xTcpSwitchBufSend);
}

static void tcp_free_buf(tcp_buf_t* buf)
{
	if (buf != NULL)
	{
		if (buf->data != NULL)
		{
			free(buf->data);
			buf->data = NULL;
		}
		free(buf);
	}
}

void tcp_queue_response(tcp_command_t cmd, void* data, uint32_t byte_data)
{
	tcp_buf_t* p_buf = tcp_make_frame(cmd, data, byte_data);

	if (p_buf != NULL)
	{
		if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
		{
			tcp_free_buf(p_buf);
		}
	}
}

void tcp_queue_status_response(tcp_command_t status, tcp_command_t response_to, void* data, uint32_t byte_data)
{
	uint8_t response_to_byte = (uint8_t)response_to;

	if (data == NULL || byte_data == 0)
	{
		tcp_queue_response(status, &response_to_byte, sizeof(response_to_byte));
		return;
	}

	uint32_t payload_len = sizeof(response_to_byte) + byte_data;
	uint8_t* payload = malloc(payload_len);

	if (payload == NULL)
		return;

	payload[0] = response_to_byte;
	memcpy(&payload[1], data, byte_data);
	tcp_queue_response(status, payload, payload_len);
	free(payload);
}

void tcp_queue_ack(tcp_command_t response_to)
{
	tcp_queue_status_response(TCP_ACK, response_to, NULL, 0);
}

void tcp_queue_nack(tcp_command_t response_to)
{
	tcp_queue_status_response(TCP_NACK, response_to, NULL, 0);
}

/**
 *	@brief make frame tcp
 */

tcp_buf_t* tcp_make_frame(tcp_command_t cmd, void* data, uint32_t byte_data)
{

	uint32_t payload_len = (data != NULL && byte_data != 0) ? byte_data : 0;
	uint32_t byte_of_frame = TCP_FRAME_HEADER_LEN + payload_len;
	uint8_t* retCmd = malloc(byte_of_frame);

	if (retCmd == NULL)
		return NULL;

	retCmd[0] = 'T';
	retCmd[1] = 'C';
	retCmd[2] = 'P';
	
	retCmd[3] = (uint8_t)cmd;
	
	retCmd[4] = payload_len & 0xff;
	retCmd[5] = (payload_len >> 8) & 0xff;
	retCmd[6] = (payload_len >> 16) & 0xff;
	retCmd[7] = (payload_len >> 24) & 0xff;

	if (payload_len != 0)
	{
		memcpy(&retCmd[TCP_FRAME_HEADER_LEN], data, payload_len);
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
