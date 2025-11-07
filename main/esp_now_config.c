#include "esp_now_config.h"

uint8_t my_macadrr[6] = {0xC8, 0xC9, 0xA3, 0x69, 0x88, 0x56};

esp_now_peer_info_t peer = {0};

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
//	esp_now_init();
	
	
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

void send_to_all_peer(uint8_t *data_send, uint8_t len_data_send)
{
	// the version of this rtos sdk not use 0xFF
	
	esp_now_send(NULL, data_send, len_data_send);
}

void init_peer()
{
	// init and add peer ESP32
	uint8_t peer_esp32[6] = {0x4C, 0xC3, 0x82, 0x0C, 0x96, 0x6C};
	memcpy(peer_esp32.peer_addr, peer_esp32, 6);
	peer_esp32.channel = 0; // cùng kênh Wi-Fi 
	peer_esp32.encrypt = false; 
	esp_now_add_peer(&peer_esp32);
	
	
	
}























