#ifndef MY_LIBRARY

#define MY_LIBRARY

#include "my_pwm.h"
#include "my_callback.h"
#include "esp_now_config.h"
#include "wifi_config.h"
#include "gpio_config.h"
#include "spiffs_config.h"
#include "my_tcpip.h"
#include "my_effect.h"

typedef struct 
{
	Pwm_Typedef Pwm;
	tcp_server_t server;
	My_Esp_Now_Typedef espnow; 
	wifi_t wifi;
	effect_manage_t effMana; 
	
	file_mana_t eff_file;
	
} Object;

extern Object SLT; 

extern QueueHandle_t xEffLoadf;
extern QueueHandle_t xNowRecv;
extern SemaphoreHandle_t xUseWifi;

#endif

