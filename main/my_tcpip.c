
#include "my_tcpip.h"


void my_init_tcpip(void)
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
		return ERR_OK;
	}

	char buf[512] = {0};
	int len_read = ((sizeof(buf)) <= p->tot_len) ? sizeof(buf) - 1 : p->tot_len;

	
	pbuf_copy_partial(p, buf, len_read, 0);
	
	buf[len_read] = '\0';
	
	tcp_recved(pcb, p->tot_len);
	
	const char *reply = " ESP ACK\n";
	tcp_write(pcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);
	
	sscanf(buf, "ssid=%31[^&]&pass=%63s", tem_wifi_cred.ssid, tem_wifi_cred.pass);	
	
	vTaskDelay(pdMS_TO_TICKS(1));
	
	wifi_cred.retry_connect = 0;
	wifi_config_t sta_cfg = {0};
	strcpy((char*)sta_cfg.sta.ssid, tem_wifi_cred.ssid);
	strcpy((char*)sta_cfg.sta.password, tem_wifi_cred.pass);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
	esp_wifi_disconnect();
	esp_wifi_connect();
	
	pbuf_free(p);
	return ERR_OK;
}

err_t wifi_server_sent(void* arg, struct tcp_pcb* pcb, uint16_t len)
{
	return ERR_OK;
}