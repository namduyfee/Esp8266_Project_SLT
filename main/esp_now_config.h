#ifndef ESP_NOW_CONFIG
#define ESP_NOW_CONFIG

#include "my_lib.h"

void wifi_init();
void espnow_init();

void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);


#endif