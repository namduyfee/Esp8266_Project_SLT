#ifndef WIFI_CONFIG
#define WIFI_CONFIG


#include "my_lib.h"

typedef struct
{
	char ssid[32];
	char pass[64];
	volatile bool is_connected;
	volatile bool is_call_discnt;
	
} wifi_cred_t;


void init_wifi(void);

void start_wifi(void); 
//void start_mdns(void);
extern wifi_cred_t wifi_cred;

#endif

