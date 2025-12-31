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

typedef struct
{
	char ssid[32];
	char pass[64];
	volatile bool is_connected;
	volatile bool last_available;
	volatile uint32_t retry_connect;
	
} wifi_cred_t;


void init_wifi(void);

void my_start_wifi(void); 

extern wifi_cred_t wifi_cred;
extern wifi_cred_t tem_wifi_cred;

extern wifi_cred_t tcp_wifi_cred;

#endif

