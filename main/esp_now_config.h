#ifndef ESP_NOW_CONFIG
#define ESP_NOW_CONFIG

#include "my_lib.h"

#define CONFIG_ESPNOW_CHANNEL 1

void init_espnow(void); 

void init_all_peer(void);
void init_my_esp_now(void);
bool is_same_macadrr(const uint8_t *mac_addr1, const uint8_t *mac_addr2); 

typedef struct
{
	void* content;
	uint32_t len;		/**< total byte */
	
} data_espnow_t;

typedef struct Peer
{
	union
	{
		esp_now_peer_info_t sta;
		esp_now_peer_info_t ap; 	
	} inf;
	
	struct
	{
		data_espnow_t data; 
	} send;
	
	struct
	{
		data_espnow_t data;
	} recv;
	
} Peer_Typedef;

typedef struct My_Esp_Now
{
	uint8_t addr[6];
	volatile bool can_send; 
	
} My_Esp_Now_Typedef;


extern Peer_Typedef g_peer_esp8266;
extern My_Esp_Now_Typedef g_my_esp_now;


#endif