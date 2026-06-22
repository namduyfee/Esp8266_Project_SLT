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
#define TCP_AUTO_DIS_MS 40000

typedef enum
{
	TCP_NONE    = 0,
	TCP_OPF,
	TCP_CLSF,
	TCP_DLTF,
	
	TCP_RDF,
	
	TCP_ST_WRF,
	TCP_WRF,
	TCP_END_WRF,
	
	TCP_RET_RDF,
	TCP_END_RET_RDF,
	
	TCP_EFF_SYNCH,
	TCP_EFF_ASYNCH,
	
	TCP_RETURN_ID_RECEIVED,
	
	TCP_ACK, 
	TCP_NACK
} tcp_command_t;

typedef struct
{
	uint8_t* data;
	uint32_t tot_byte;
	
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
		volatile bool sent; 
	} send;
	
	void* arg;
	
	tcp_client_t* p_client;
	
} tcp_server_t;


err_t init_server_tpcp(uint16_t port, uint8_t max_client);  
void tcp_send_cb(void* arg);  

tcp_buf_t* tcp_make_frame(tcp_command_t cmd, void* data, uint32_t byte_data); 
tcp_buf_t* tcp_make_ret_read(void*data, uint32_t len, uint32_t offset); 
#endif