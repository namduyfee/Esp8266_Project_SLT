#ifndef ESP_TCPIP

#define ESP_TCPIP

#include "my_lib.h"


typedef struct 
{
	void* content;
	uint16_t len;		
		
} data_t;

typedef struct
{
	struct tcp_pcb* tpcb_server;
	struct tcp_pcb* tpcb;
	
	data_t segment_recv;
	
	void* arg;
} client_tcp_t;

typedef struct
{
	struct tcp_pcb* tpcb;
	ip_addr_t ip;
	uint16_t port;
	
	uint16_t count_client; 
	uint8_t max_client;
	
	data_t segment_recv;

	void* arg;
} server_tcp_t;


err_t init_server_tpcp(uint16_t port, uint8_t max_client);  
err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err);
err_t server_recv_tcp(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err);
err_t server_sent_tcp(void* arg, struct tcp_pcb* tpcb, uint16_t len);

err_t client_tcp_close(struct tcp_pcb *cl_tpcb, client_tcp_t* client); 

void wifi_handler(void* buff, uint16_t len);


extern server_tcp_t SLT_server;
#endif