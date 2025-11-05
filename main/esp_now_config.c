#include "esp_now_config.h"


esp_now_peer_info_t peer;

void wifi_init() 
{
	tcpip_adapter_init();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_wifi_set_mode(WIFI_MODE_STA);  // STA mode ??
	esp_wifi_start();
}

void espnow_init() 
{
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
	
	esp_now_register_recv_cb(on_data_recv);
	esp_now_register_send_cb(on_data_sent);
	
}

void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) 
{
	ESP_LOGI("ESPNOW", "Received data from " MACSTR ", len=%d", MAC2STR(mac_addr), len);
}

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	ESP_LOGI("ESPNOW", "Send to " MACSTR ", status=%d", MAC2STR(mac_addr), status);
}



















