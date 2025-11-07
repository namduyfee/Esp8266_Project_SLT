#ifndef ESP_NOW_CONFIG
#define ESP_NOW_CONFIG

#include "my_lib.h"


#define ESP_NOW_MAX_LEN 255

#define PEER_NOT_REQUEST 0

#define TOTAL_REQUEST ((*g_peer[i]).total_request)

void wifi_init(void);
void config_espnow(void);

void init_all_peer(void);
void init_my_esp_now(void);
bool is_same_macadrr(const uint8_t *mac_addr1, const uint8_t *mac_addr2); 

void check_send_request(void);

typedef struct Peer
{
	esp_now_peer_info_t inf;
	volatile uint8_t request[ESP_NOW_MAX_LEN];
	
	volatile uint8_t total_request;
	
	volatile uint8_t buffer_receive[ESP_NOW_MAX_LEN];
	
} Peer_Typedef;

typedef struct My_Esp_Now
{
	uint8_t addr[6];
	uint8_t *frame[ESP_NOW_MAX_LEN];
	
	uint8_t len_frame[ESP_NOW_MAX_LEN];
	
} My_Esp_Now_Typedef;


extern Peer_Typedef g_peer_esp32;
extern My_Esp_Now_Typedef g_my_esp_now;

extern Peer_Typedef *g_peer[];
const extern uint32_t g_len_peer;

#endif