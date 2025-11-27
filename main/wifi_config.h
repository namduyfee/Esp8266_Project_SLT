#ifndef WIFI_CONFIG
#define WIFI_CONFIG


#include "my_lib.h"

#define MAX_RETRY_CONNECT 10
typedef struct
{
	char ssid[32];
	char pass[64];
	volatile bool is_connected;
	volatile bool is_start_empty;
	uint32_t retry_connect;
	
} wifi_cred_t;


void init_wifi(void);

void start_wifi(void); 
//void start_mdns(void);
extern wifi_cred_t wifi_cred;
extern wifi_cred_t tem_wifi_cred;

#endif

