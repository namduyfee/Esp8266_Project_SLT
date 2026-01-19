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
#include "lwip/opt.h"
#include "lwip/tcpip.h"

#define POS_CONTINUE -1
#define REMAINING -1

#define TCP_POLL_CYCLE 2
#define TCP_AUTO_DIS_MS 8000
typedef enum
{
	TCP_NONE    = -1,
	TCP_OPEN	= 0,
	TCP_CLOSE	= 1,
	TCP_DELETE	= 2, 
	TCP_READ	= 3,
	TCP_WRITE	= 4,
	
	TCP_OPEN_RET	= 5,
	TCP_CLOSE_RET,
	TCP_DELETE_RET, 
	TCP_READ_RET,
	TCP_WRITE_RET
		
} command_tcp_t; 

typedef struct
{
	void* data;
	uint32_t len;			/**< total bytes of memory that is pointed by data */
	
} tcp_buf_t; 

typedef struct 
{
	tcp_buf_t buf;				/**< store content and len */
	uint16_t pos_data;			/**< start position of data to save in buffer */
	off_t pos_in_file;			/**< position in file to save */
	command_tcp_t command;		/**< command with file */
	int tot_len;				/**< total length message */
	
} tcp_recv_t;

typedef struct
{
	struct tcp_pcb* tpcb_server;
	struct tcp_pcb* tpcb;				/**< tcp client is creat */
	TickType_t lastTick; 				/**< auto disconnect tcp */
	struct
	{
		tcp_recv_t segment; 
		
	} recv;
	
	struct
	{
		tcp_buf_t buf;
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
		command_tcp_t cmd; 
		tcp_recv_t segment; 
		off_t current_pos_file;
		int tot_len;				/**< total bytes of message */
	} recv;
	
	struct
	{
		tcp_buf_t* buf; 
		
	} send;
	
	void* arg;
	
	tcp_client_t* client;
	
} tcp_server_t;


err_t init_server_tpcp(uint16_t port, uint8_t max_client);  
void tcp_send_cb(void* arg);  
void tcp_ret_cmd(command_tcp_t cmd, uint8_t state); 
#endif