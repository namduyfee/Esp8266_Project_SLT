
#include "my_tcpip.h"


void my_init_tcpip(void)
{
	loadf_pcb_tcp();
	wifi_pcb_tcp();
	
}

/*
 *	PCB TCP CONIFG WIFI
 *	WIFI AP MODE
 **/
void wifi_pcb_tcp(void)
{
	struct tcp_pcb *set_wifi;
	ip_addr_t ipaddr;
	
	IP4_ADDR(&ipaddr, 192, 168, 4, 1);
	
	set_wifi = tcp_new();
	
	if (!set_wifi)
	{
		return;
	}
	
	if (tcp_bind(set_wifi, &ipaddr, 5000) != ERR_OK)
	{
		tcp_abort(set_wifi);
		return;
	}
	
	set_wifi = tcp_listen(set_wifi);
	
	if (!set_wifi)
	{
		return;
	}
	
	tcp_accept(set_wifi, wifi_server_accept);
	
}

err_t wifi_server_accept(void* arg, struct tcp_pcb* pcb, err_t err)
{
	hw_timer_enable(false);
	
	tcp_recv(pcb, wifi_server_recv);
	tcp_sent(pcb, wifi_server_sent);
	
	tcp_arg(pcb, NULL);
	tcp_err(pcb, NULL);
	return ERR_OK;
}

err_t wifi_server_recv(void* arg, struct tcp_pcb* pcb, struct pbuf *p, err_t err)
{
	if (p == NULL) {
		// Client closed connection or tcp_abort(pcb)
		tcp_close(pcb);
		hw_timer_enable(true);
		return ERR_OK;
	}

	char buf[512] = {0};
	int len_read = ((sizeof(buf)) <= p->tot_len) ? sizeof(buf) : p->tot_len;

	
	pbuf_copy_partial(p, buf, len_read, 0);
	
	int16_t index_ssid = -1, index_pass = -1;
	
	for (int16_t i = 0; i < len_read - 4; i++)
	{
		if (buf[i] == 's' && buf[i + 1] == 's' && buf[i + 2] == 'i' && 
		buf[i + 3] == 'd' && buf[i+4] == '=')
			index_ssid = i;
		
		if (buf[i] == 'p' && buf[i + 1] == 'a' && buf[i + 2] == 's' && 
		buf[i + 3] == 's' && buf[i+4] == '=')
			index_pass = i;
	}
	
	if (index_pass == -1 || index_ssid == -1)
	{
		const char *reply = "invalid infomation\n";
		tcp_write(pcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
	}
	
	else
	{
		
		int i_ssid = 0, i_pass = 0;
		
		for (int i = 0; i < len_read; i++)
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
		
		const char *reply = "connecting...\n";
		tcp_write(pcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
		
	}
	
	tcp_recved(pcb, p->tot_len);
	
	pbuf_free(p);
	
	return ERR_OK;
}

err_t wifi_server_sent(void* arg, struct tcp_pcb* pcb, uint16_t len)
{
	return ERR_OK;
}

/*
 *	PCB TCP LOAD FILE BIN
 *	WIFI STA MODE
 **/

void loadf_pcb_tcp(void)
{
	struct tcp_pcb *loadf;
	ip_addr_t ipaddr;
	
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	
	loadf = tcp_new();
	
	if (!loadf)
	{
		return;
	}
	
	if (tcp_bind(loadf, &ipaddr, 5002) != ERR_OK)
	{
		tcp_abort(loadf);
		return;
	}
	
	loadf = tcp_listen(loadf);
	
	if (!loadf)
	{
		return;
	}
	
	tcp_accept(loadf, loadf_server_accept);
	
}

err_t loadf_server_accept(void* arg, struct tcp_pcb* pcb, err_t err)
{
	hw_timer_enable(false);
	
	tcp_recv(pcb, loadf_server_recv);
	tcp_sent(pcb, loadf_server_sent);
	
	tcp_arg(pcb, NULL);
	tcp_err(pcb, NULL);
	return ERR_OK;
}

err_t loadf_server_recv(void* arg, struct tcp_pcb* pcb, struct pbuf *p, err_t err)
{
	if (p == NULL) {
		
		// Client closed connection or tcp_abort(pcb)
		tcp_close(pcb);
		hw_timer_enable(true);
		return ERR_OK;
	}
	
	/*
	 *	Send total buf to queue
	 **/
	
	BufItem_Typedf buf_it;
	buf_it.payload = (uint8_t*)(malloc(p->tot_len));
	buf_it.len = p->tot_len;
	pbuf_copy_partial(p, buf_it.payload, p->tot_len, 0);
	xQueueSendToBack(xBuffLoadf, &buf_it, 0);
	
	/* 
		// send each small buffer to queue
		
		struct pbuf* tm_buf = p;
		while (tm_buf != NULL)
		{
			BufItem_Typedf buf_it;
			buf_it.payload = (char*)(malloc(tm_buf->len));
			buf_it.len = tm_buf->len;
			xQueueSendToBack(xBuffLoadf, &buf_it, 0);
			tm_buf = tm_buf->next;
		}
	*/
	
	
	tcp_recved(pcb, p->tot_len);
	pbuf_free(p);
	const char *reply = "RECEIVED...\n";
	tcp_write(pcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
	
	return ERR_OK;
}

err_t loadf_server_sent(void* arg, struct tcp_pcb* pcb, uint16_t len)
{
	return ERR_OK;
}