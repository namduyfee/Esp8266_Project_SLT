#ifndef MY_LIBRARY

#define MY_LIBRARY

#include "my_pwm.h"
#include "my_callback.h"
#include "esp_now_config.h"
#include "wifi_config.h"
#include "gpio_config.h"
#include "spiffs_config.h"
#include "my_tcpip.h"

typedef struct 
{
	Pwm_Typedef Pwm;
	tcp_server_t server;
	My_Esp_Now_Typedef espnow; 
	wifi_t wifi;
	
} Object;

extern Object SLT; 

extern QueueHandle_t xBuffLoadf;
extern QueueHandle_t xBuffSendf;
extern QueueHandle_t xEspNowRecv;

extern SemaphoreHandle_t xEspNowf;
extern SemaphoreHandle_t xSendEspNow;

#endif

