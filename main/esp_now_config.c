#include "esp_now_config.h"



uint8_t g_my_macadrr[6] = { 0xC8, 0xC9, 0xA3, 0x69, 0x88, 0x56 };

esp_now_peer_info_t g_peer_esp32 = { 0 };


void wifi_init(void) 
{
	tcpip_adapter_init();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_wifi_set_mode(WIFI_MODE_STA);  // STA mode
	esp_wifi_start();
}

void config_espnow(void) 
{
	wifi_init();
	esp_now_init();
	
	esp_now_register_recv_cb(on_data_recv);
	esp_now_register_send_cb(on_data_sent);
	
	init_all_peer();
	
}

void send_to_all_peer(uint8_t *data_send, uint8_t len_data_send)
{
	// the version of this rtos sdk not use 0xFF
	
	esp_now_send(NULL, data_send, len_data_send);
}

void init_all_peer(void)
{
	// init and add peer ESP32
	uint8_t peer_esp32_addr[6] = { 0x4C, 0xC3, 0x82, 0x0C, 0x96, 0x6C };
	memcpy(g_peer_esp32.peer_addr, peer_esp32_addr, 6);
	g_peer_esp32.channel = 0; // cùng kênh Wi-Fi 
	g_peer_esp32.encrypt = false; 
	esp_now_add_peer(&g_peer_esp32);
}