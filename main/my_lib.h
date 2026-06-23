#ifndef MY_LIBRARY

#define MY_LIBRARY

#include "my_pwm.h"
#include "esp_now_config.h"
#include "wifi_config.h"
#include "gpio_config.h"
#include "spiffs_config.h"
#include "my_tcpip.h"
#include "my_effect.h"


#define MY_ID 0						/**< id of this esp */
#define KEY_NOW "NOW"				/**< only esp have KEY_NOW is same then they can receive each other's packets. KEY_NOW must is 3 characters*/


typedef struct 
{
	Pwm_Typedef Pwm;
	tcp_server_t server;
	My_Esp_Now_Typedef espnow; 
	wifi_t wifi;
	effect_manage_t effMana;
	
	
} Object;

extern Object SLT; 
extern QueueHandle_t xTcpLoadf; 
extern QueueHandle_t xEffLoadf;
extern QueueHandle_t xNowRecv;
extern QueueHandle_t xNowSend;	
extern SemaphoreHandle_t xTcpSwitchBufSend;
extern SemaphoreHandle_t xNowSendDone; 
extern SemaphoreHandle_t xMasterModeEff;

#endif

