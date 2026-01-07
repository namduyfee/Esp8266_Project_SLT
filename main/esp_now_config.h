#ifndef ESP_NOW_CONFIG
#define ESP_NOW_CONFIG

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

#include "esp_wifi.h"
#include "esp_now.h"


#define MAC_ADDR_LEN 6
#define CONFIG_ESPNOW_CHANNEL 1
#define LEN_HEADER_ESPNOW 3
#define LEN_CRC_ESPNOW 2
#define MAX_BROADCAST_CNT 21

#define INDEX_CMD 3
#define INDEX_POS (INDEX_CMD + 1)
typedef enum
{
	NONE_ESPNOW		   = -1,	
	ADD_PEER           = 0,
	GET_PEER		   = 1,
	ESPNOW_READ        = 2,
	ESPNOW_WRITE	   = 3,
	BROADCAST          = 4	
		
} command_espnow_t; 


void init_espnow(void); 
uint8_t espnow_add_peer(uint8_t* peer_addr, uint8_t position);   
bool is_same_macadrr(const uint8_t *mac1, const uint8_t *mac2);  

uint16_t crc16_modbus(uint8_t *buf, uint32_t len); 

typedef struct
{
	void* content;
	uint32_t len;		/**< total byte */
	
} data_espnow_t;

typedef struct Peer
{
	esp_now_peer_info_t info;
	struct
	{
		data_espnow_t data; 
	} send;
	
	struct
	{
		data_espnow_t data;
	} recv;
	
	uint8_t position;				/**< position of peer in system */
	
} Peer_Typedef;

typedef struct My_Esp_Now
{
	uint8_t my_addr[6];
	
	bool gateway_added; 
	
	volatile bool sent;					/**< the last buffer is sent */
	
	volatile bool can_send;				/**< can send to wifi buffer */
	
	Peer_Typedef* p_peer; 
	
	uint32_t tot_peer; 
	
	int8_t mode_send;
	
} My_Esp_Now_Typedef;

#endif