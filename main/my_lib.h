#ifndef MY_LIBRARY

#define MY_LIBRARY

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/hw_timer.h"

#include "esp_now.h"
#include "esp_log.h"
#include "esp_wifi.h"


#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"


#include "pwm_timer1.h"
#include "my_callback.h"
#include "esp_now_config.h"
#include "wifi_config.h"
#include "esp_http_config.h"
#include "gpio_config.h"
#include <fcntl.h>
#include "spiffs_config.h"

extern uint8_t data_esp_now [];

extern uint8_t len_test_data_esp_now;

extern uint8_t data_frame2 [];
extern uint8_t len_test_data_2; 


extern SemaphoreHandle_t xRecvPassWifi;


#endif

