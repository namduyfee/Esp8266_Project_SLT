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
#include "my_tcpip.h"


#define MAX_BROADCAST_CNT 50
#define MAC_ADDR_LEN 6
#define CONFIG_ESPNOW_CHANNEL 1

#define ESPNOW_INDEX_HEADER 0
#define ESPNOW_LEN_HEADER 3

#define ESPNOW_INDEX_CMD (ESPNOW_INDEX_HEADER + ESPNOW_LEN_HEADER)
#define ESPNOW_LEN_CMD 1

#define ESPNOW_INDEX_SIZE_DT (ESPNOW_INDEX_CMD + ESPNOW_LEN_CMD)
#define ESPNOW_LEN_SIZE_DT 4

#define ESPNOW_INDEX_DATA (ESPNOW_INDEX_SIZE_DT + ESPNOW_LEN_SIZE_DT)

#define ESPNOW_INDEX_POS (ESPNOW_INDEX_DATA)
#define ESPNOW_LEN_POS 1

#define ESPNOW_INDEX_ADDR (ESPNOW_INDEX_POS + ESPNOW_LEN_POS)
#define ESPNOW_LEN_ADDR 6

#define ESPNOW_LEN_CRC 2


typedef enum
{
	NONE_ESPNOW		   = -1,	
	ADD_PEER           = 0,
	GET_PEER		   = 1,
	ESPNOW_READ        = 2,
	ESPNOW_WRITE	   = 3,
	BROADCAST          = 4,
	FB_BROADCST		   = 5				/**< feedback broadcast */
		
} command_espnow_t; 

typedef struct
{
	void* data;
	uint32_t len;		/**< total byte */
	
} buf_espnow_t;

typedef struct Peer
{
	esp_now_peer_info_t info;
	struct
	{
		buf_espnow_t buf; 
	} send;
	
	struct
	{
		buf_espnow_t buf;
	} recv;
	
	uint8_t position;				/**< position of peer in system */
	
} Peer_Typedef;

typedef struct My_Esp_Now
{	
	bool gateway_added; 
	
	volatile bool sent;					/**< the last buffer is sent */
	
	volatile bool can_send;				/**< can send to wifi buffer */
	
	Peer_Typedef* p_peer; 
	
	uint32_t tot_pos_added;				/**< total position added */ 
	
	int8_t mode_send;
	
	uint8_t my_pos;
	
	uint8_t broadcast_cnt;
	struct
	{
		buf_espnow_t buf;
		
	} recv;
	
} My_Esp_Now_Typedef;


void init_espnow(void); 
uint8_t espnow_add_peer(uint8_t* peer_addr, uint8_t position, bool save);   
bool is_same_macadrr(const uint8_t *mac1, const uint8_t *mac2);  
void clear_all_peer(void);
uint16_t crc16_modbus(uint8_t *buf, uint32_t len); 

buf_espnow_t* espnow_make_segment(void* buf, uint8_t cmd, uint32_t len);
void espnow_free_segment(buf_espnow_t* buf);


#endif