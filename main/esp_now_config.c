#include "esp_now_config.h"

uint8_t my_macadrr[6] = {0x02, 0x03, 0x03, 0x04, 0x05, 0x06};

esp_now_peer_info_t peer;

void wifi_init() 
{
	tcpip_adapter_init();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_wifi_set_mode(WIFI_MODE_STA);  // STA mode
	esp_wifi_start();
}

void espnow_init() 
{
	wifi_init();
	esp_now_init();
	
	/*
	esp_err_t ret = esp_now_init();
	if (ret == ESP_OK) {
		ESP_LOGI("ESPNOW", "ESP-NOW Init OK");
	}
	else if (ret == ESP_ERR_ESPNOW_EXIST) {
		ESP_LOGW("ESPNOW", "ESP-NOW already initialized");
	}
	else {
		ESP_LOGE("ESPNOW", "ESP-NOW Init Failed: %d", ret);
	}
	*/
	esp_wifi_get_mac(ESP_IF_WIFI_STA, my_macadrr);
	
	esp_now_register_recv_cb(on_data_recv);
	esp_now_register_send_cb(on_data_sent);
	
}




















