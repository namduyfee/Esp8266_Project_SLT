#ifndef ESP_TCPIP

#define ESP_TCPIP

#include "my_lib.h"


typedef struct buf_item
{
	void* payload;
	uint16_t len;
	
} BufItem_Typedf;

void my_init_tcpip(void); 

err_t wifi_server_accept(void* arg, struct tcp_pcb* pcb, err_t err);

err_t wifi_server_recv(void* arg, struct tcp_pcb* pcb, struct pbuf *p, err_t err);

err_t wifi_server_sent(void* arg, struct tcp_pcb* pcb, uint16_t len);

void wifi_pcb_tcp(void);

void loadf_pcb_tcp(void);

err_t loadf_server_accept(void* arg, struct tcp_pcb* pcb, err_t err);

err_t loadf_server_recv(void* arg, struct tcp_pcb* pcb, struct pbuf *p, err_t err);
 
err_t loadf_server_sent(void* arg, struct tcp_pcb* pcb, uint16_t len);

#endif

