
#include "wifi_config.h"

wifi_cred_t wifi_cred =
{
	.is_connected = false,
	.retry_connect = 0
};

wifi_cred_t tem_wifi_cred;

wifi_cred_t tcp_wifi_cred;

void init_wifi(void)
{
	esp_event_loop_init(wifi_event_handler, NULL);
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
}


void start_wifi(void)
{
	init_wifi();
//	esp_wifi_set_mode(WIFI_MODE_APSTA);
	esp_wifi_set_mode(WIFI_MODE_AP);
//	nvs_handle nvs;
//	size_t len;
//	esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs);
//	if (err == ESP_OK)
//	{
//		len = sizeof(wifi_cred.ssid);
//		if (nvs_get_str(nvs, "ssid", wifi_cred.ssid, &len) == ESP_OK)
//		{
//			len = sizeof(wifi_cred.pass);
//			nvs_get_str(nvs, "pass", wifi_cred.pass, &len);
//		}
//		nvs_close(nvs);
//	}
//	
//	strcpy((char*)tem_wifi_cred.ssid, wifi_cred.ssid);
//	strcpy((char*)tem_wifi_cred.pass, wifi_cred.pass);
//	
//	if (strlen(wifi_cred.ssid) != 0)
//	{
//		wifi_cred.last_available = true;
//		
//		wifi_config_t sta_cfg = {0};
//		strcpy((char*)sta_cfg.sta.ssid, wifi_cred.ssid);
//		strcpy((char*)sta_cfg.sta.password, wifi_cred.pass);
//		esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
//	}	
//	else 
//		wifi_cred.last_available = false;
	
	wifi_config_t ap_config = {
		.ap = {
			.ssid = "ESP_SLT",
			.ssid_len = 0,
			.max_connection = 4,
			.password = "12345678",
			.authmode = WIFI_AUTH_WPA_WPA2_PSK		
		}	
	};
	esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
	

	esp_wifi_start();
	esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, 0); 
	
}
