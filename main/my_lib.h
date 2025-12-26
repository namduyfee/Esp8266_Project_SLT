#ifndef MY_LIBRARY

#define MY_LIBRARY

#include <fcntl.h>
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
#include "esp_event.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_netif.h"
#include "driver/pwm.h"

#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#include "my_pwm.h"
#include "my_callback.h"
#include "esp_now_config.h"
#include "wifi_config.h"
#include "gpio_config.h"
#include "spiffs_config.h"
#include "my_tcpip.h"


extern SemaphoreHandle_t xRecvPassWifi;
extern SemaphoreHandle_t xTryConnectWifi;

extern QueueHandle_t xBuffLoadf;

extern QueueHandle_t xBuffSendf;

#endif

