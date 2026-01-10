#ifndef ESP_TCPIP

#define ESP_TCPIP

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"

#include "tcpip_adapter.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#define POS_CONTINUE -1
#define REMAINING -1
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
	tcp_data_t data;			/**< store content and len */
	uint16_t pos_data;			/**< start position of data to save in buffer */
	off_t pos_in_file;			/**< position in file to save */
	command_file_t command;			/**< command with file */
	int tot_len;				/**< total length message */
	
} tcp_recv_t;

typedef struct
{
	struct tcp_pcb* tpcb_server;
	struct tcp_pcb* tpcb;				/**< tcp client is creat */
	
	struct
	{
		tcp_recv_t segment; 
		
	} recv;
	
	struct
	{
		tcp_data_t data;
		bool request;					/**< flag to read file and send */
		
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
		int tot_len;				/**< total length message */
	} recv;
	
	struct
	{
		tcp_data_t data; 
		
	} send;
	
	void* header;

	void* arg;
} tcp_server_t;


err_t init_server_tpcp(uint16_t port, uint8_t max_client);  

#endif