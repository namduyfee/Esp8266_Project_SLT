#ifndef ESP_TCPIP

#define ESP_TCPIP

#include "my_lib.h"

#define POS_CONTINUE -1

typedef enum
{
	OPEN	= 0,
	CLOSE	= 1,
	DELETE	= 2, 
	READ	= 3,
	WRITE	= 4, 
	FORMAT  = 5
	
} command_file_t; 

typedef struct
{
	void* content;
	uint32_t len;
	
} tcp_data_t; 

typedef struct 
{
	tcp_data_t data;
	uint16_t pos_data;
	off_t pos_in_file;
	uint8_t command;
	
} tcp_recv_t;

typedef struct
{
	struct tcp_pcb* tpcb_server;
	struct tcp_pcb* tpcb;
	
	struct
	{
		tcp_recv_t segment; 
		
	} recv;
	
	struct
	{
		tcp_data_t data;
		bool request; 
		
	} send;
  
void* arg;
} tcp_client_t;

typedef struct
{
	struct tcp_pcb* tpcb;
	ip_addr_t ip;
	uint16_t port;
	
	uint16_t count_client; 
	uint8_t max_client;
	
	struct
	{
		tcp_recv_t segment; 
		off_t current_pos_file; 
		
	} recv;
	
	struct
	{
		tcp_data_t data; 
		
	} send;
	
	void* header;

	void* arg;
} tcp_server_t;


err_t init_server_tpcp(uint16_t port, uint8_t max_client);  
err_t server_accept_tcp(void* arg, struct tcp_pcb* newpcb, err_t err);
err_t server_recv_tcp(void* arg, struct tcp_pcb* tpcb, struct pbuf *p, err_t err);
err_t server_sent_tcp(void* arg, struct tcp_pcb* tpcb, uint16_t len);

err_t client_tcp_close(struct tcp_pcb *cl_tpcb, tcp_client_t* client); 

void wifi_handler(void* buff, uint16_t len);


extern tcp_server_t SLT_server;
#endif