#ifndef MY_CALLBACK_FUNCTION

#define MY_CALLBACK_FUNCTION

#include "my_lib.h"

void timer_cb(void* arg);

void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);
esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

#endif

