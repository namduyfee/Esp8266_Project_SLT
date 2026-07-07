#ifndef MY_LIBRARY

#define MY_LIBRARY

#include "my_pwm.h"
#include "esp_now_config.h"
#include "wifi_config.h"
#include "gpio_config.h"
#include "spiffs_config.h"
#include "my_tcpip.h"
#include "my_effect.h"


#define MY_ID 0xff						/**< id of this esp */


typedef struct 
{
	Pwm_Typedef Pwm;
	tcp_server_t server;
	My_Esp_Now_Typedef espnow; 
	wifi_t wifi;
	effect_manage_t effMana;
	
	
} Object;

typedef struct
{
	tcp_command_t cmd;
	
	espnow_mode_t mode; 
	
	uint8_t my_id;
	
	char gw_code[9];
	
} request_config_espmode_t;

extern Object SLT; 
extern QueueHandle_t xTcpLoadf; 
extern QueueHandle_t xEffLoadf;
extern QueueHandle_t xNowRecv;
extern QueueHandle_t xNowRecvEffect;
extern QueueHandle_t xNowSend;	
extern QueueHandle_t xConfigEspMode;
extern QueueHandle_t xSendTcp;

extern SemaphoreHandle_t xTcpSwitchBufSend;
extern SemaphoreHandle_t xNowSendDone; 
extern SemaphoreHandle_t xMasterModeEff;

#endif

