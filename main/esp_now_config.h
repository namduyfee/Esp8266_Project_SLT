#ifndef ESP_NOW_CONFIG
#define ESP_NOW_CONFIG

#include "my_lib.h"

void wifi_init();
void espnow_init();
void send_to_all_peer(uint8_t *data_send, uint8_t len_data_send);
extern uint8_t my_macadrr[6];



#endif