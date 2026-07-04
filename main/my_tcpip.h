#ifndef ESP_TCPIP

#define ESP_TCPIP

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
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
#define TCP_FRAME_HEADER_LEN 8
#define TCP_RX_BUF_SIZE 4096
#define TCP_MAX_PAYLOAD_LEN (TCP_RX_BUF_SIZE - TCP_FRAME_HEADER_LEN)

typedef enum
{
	TCP_NONE    = 0,
	TCP_OPF     = 1,
	TCP_CLSF    = 2,
	TCP_DLTF    = 3,

	TCP_ST_WRF  = 5,
	TCP_WRF     = 6,
	TCP_END_WRF = 7,

	TCP_EFF_SYNCH = 10,
	TCP_EFF_ASYNCH = 11,

	TCP_GET_INF_ESP_MODE = 12,
	TCP_SET_INF_ESP_MODE = 13,

	TCP_RETURN_ID_RECEIVED = 14,

	TCP_ACK  = 15,
	TCP_NACK = 16
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
	uint8_t rx_buf[TCP_RX_BUF_SIZE];
	uint32_t rx_len;

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

	void* arg;

	tcp_client_t* p_client;

} tcp_server_t;


err_t init_server_tpcp(uint16_t port, uint8_t max_client);
void tcp_send_cb(void* arg);
void tcp_queue_response(tcp_command_t cmd, void* data, uint32_t byte_data);
void tcp_queue_status_response(tcp_command_t status, tcp_command_t response_to, void* data, uint32_t byte_data);
void tcp_queue_ack(tcp_command_t response_to);
void tcp_queue_nack(tcp_command_t response_to);
tcp_buf_t* tcp_make_frame(tcp_command_t cmd, void* data, uint32_t byte_data);

#endif
