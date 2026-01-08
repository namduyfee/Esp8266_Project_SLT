#ifndef WIFI_CONFIG
#define WIFI_CONFIG


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

#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"

#define MAX_RETRY_CONNECT 7
#define REQEST_FROM_USER 100
#define POS_ADDR_GATEWAY 0
typedef struct wifi
{
	uint8_t sta_macaddr[6];
	uint8_t gateway_addr[6]; 
	bool is_gateway;
	
} wifi_t;

void init_wifi(void);

void my_start_wifi(wifi_t* wifi);  

#endif

