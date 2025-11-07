#ifndef ESP_NOW_CONFIG
#define ESP_NOW_CONFIG

#include "my_lib.h"


void wifi_init(void);
void config_espnow(void);
void send_to_all_peer(uint8_t *data_send, uint8_t len_data_send);

void init_all_peer(void);

extern uint8_t g_my_macadrr[6];

extern esp_now_peer_info_t g_peer_esp32;


#endif