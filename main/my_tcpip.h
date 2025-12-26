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
	
} data_t; 

typedef struct 
{
	data_t data;
	uint16_t pos_data;
	off_t pos_in_file;
	uint8_t command;
	
} recv_buf_t;

typedef struct
{
	struct tcp_pcb* tpcb_server;
	struct tcp_pcb* tpcb;
	
	struct
	{
		recv_buf_t segment; 
		
	} recv;
	
	struct
	{
		data_t data;
		bool request; 
		
	} send;
  
void* arg;
} client_tcp_t;

typedef struct
{
	struct tcp_pcb* tpcb;
	ip_addr_t ip;
	uint16_t port;
	
	uint16_t count_client; 
	uint8_t max_client;
	
	struct
	{
		recv_buf_t segment; 
		off_t current_pos_file; 
		
	} recv;
	
	struct
	{
		data_t data; 
		
	} send;
	
	void* header;

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