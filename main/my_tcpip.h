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

#include "spiffs_config.h"

#define TCP_POLL_CYCLE 2
#define TCP_AUTO_DIS_MS 8000

typedef enum
{
	TCP_NONE    = 0,
	TCP_OPF,
	TCP_CLSF,
	TCP_DLTF, 
	TCP_RDF,
	TCP_WRF,
	
	TCP_RET_OPF,
	TCP_RET_CLSF,
	TCP_RET_DLTF, 
	TCP_RET_RDF,
	TCP_RET_WRTF
		
} tcp_command_t;

typedef struct
{
	void* data;
	uint32_t len;			/**< total bytes of memory that is pointed by data */
	
} tcp_buf_t; 


typedef struct
{
	struct tcp_pcb* tpcb_server;
	struct tcp_pcb* tpcb;				/**< tcp client is creat */
	TickType_t lastTick; 				/**< auto disconnect tcp */

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
		
	} recv;
	
	struct
	{
		tcp_buf_t* buf; 
		
	} send;
	
	void* arg;
	
	tcp_client_t* p_client;
	
} tcp_server_t;


err_t init_server_tpcp(uint16_t port, uint8_t max_client);  
void tcp_send_cb(void* arg);  
void tcp_ret_cmd(file_command_t cmd, uint8_t state); 
#endif