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
	
	uint8_t gateway_addr[6]; 
	bool is_gateway;
	
	My_Esp_Now_Typedef espnow; 
	
} Object;

extern Object SLT; 

extern SemaphoreHandle_t xRecvPassWifi;
extern SemaphoreHandle_t xTryConnectWifi;

extern QueueHandle_t xBuffLoadf;

extern QueueHandle_t xBuffSendf;

#endif

