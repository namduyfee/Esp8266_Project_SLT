#ifndef WIFI_CONFIG
#define WIFI_CONFIG


#include "my_lib.h"

typedef struct
{
	char ssid[32];
	char pass[64];
	
	volatile bool is_connect;
	
} wifi_cred_t;


void init_wifi(void);
void start_wifi_apsta(void);
void start_wifi_sta(void);
void start_wifi(void); 
extern wifi_cred_t wifi_cred;

#endif

