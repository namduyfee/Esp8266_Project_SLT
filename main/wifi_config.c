
#include "wifi_config.h"
#include "esp_now_config.h"
#include "my_lib.h"

static void wifi_event_handler(void* arg,esp_event_base_t event_base,int32_t event_id,void* event_data); 

void init_wifi(void)
{

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
	
}

uint32_t generate_esp32_unique_id(uint8_t mac[6]) {
    
	// hash FNV-1a (32-bit)
	uint32_t hash = 2166136261U; // FNV offset basis
	for (int i = 0; i < 6; i++) {
		hash ^= mac[i];
		hash *= 16777619U;       // FNV prime
	}
    
	return hash; 
}

void my_start_wifi(void)
{
	init_wifi();
	
	esp_wifi_get_mac(ESP_IF_WIFI_STA, SLT.wifi.sta_macaddr); 
	esp_wifi_get_mac(ESP_IF_WIFI_AP, SLT.wifi.ap_macaddr); 
	
	SLT.espnow.my_code = generate_esp32_unique_id(SLT.wifi.ap_macaddr);
	
	char id_string[9];
	snprintf(id_string, sizeof(id_string), "%08X", SLT.espnow.my_code);
	
	esp_wifi_set_mode(WIFI_MODE_AP);
				
	char result[32];
	snprintf(result, sizeof(result), "SLT V8-%s", id_string);

	wifi_config_t ap_config = {
		.ap = {
		.max_connection = 4,
		.password = "12345678",
		.channel = CONFIG_ESPNOW_CHANNEL, 
		.authmode = WIFI_AUTH_WPA_WPA2_PSK		
		}	
	};
	memcpy(ap_config.ap.ssid, result, strlen(result));
	ap_config.ap.ssid_len = strlen(result);
				
	esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
				
	tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
	tcpip_adapter_ip_info_t ip_info;
	IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
	IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
	tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
				
	esp_wifi_start();
	
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	switch (event_id) {
		
	case WIFI_EVENT_AP_START: {
			break;
		}
		
	case WIFI_EVENT_STA_START: {
			break;
		}
        
	default:
		break;
	}
	
}
