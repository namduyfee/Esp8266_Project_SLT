#include "esp_now_config.h"
#include "nvs_flash.h"
My_Esp_Now_Typedef g_my_esp_now;
Peer_Typedef g_peer_esp8266;

void init_espnow(void) 
{

	esp_now_init();
	esp_now_register_recv_cb(on_data_recv);
	esp_now_register_send_cb(on_data_sent);
	init_my_esp_now();
	init_all_peer();
	
}

void init_my_esp_now(void)
{
	uint8_t my_macaddr[6] = {0x84, 0xF3, 0xEB, 0xA6, 0xD8, 0x4F};
	memcpy(g_my_esp_now.addr, my_macaddr, 6);
}

void init_all_peer(void)
{
	// init and add peer ESP8266
	
	uint8_t peer_esp8266_addr[6] = {0x18, 0xFE, 0x34, 0xEE, 0x4E, 0x99};

	memcpy(g_peer_esp8266.inf.sta.peer_addr, peer_esp8266_addr, 6);
	
	g_peer_esp8266.inf.sta.channel = CONFIG_ESPNOW_CHANNEL; // cùng kênh Wi-Fi 

	g_peer_esp8266.inf.sta.encrypt = false; 
	g_peer_esp8266.inf.sta.ifidx = ESP_IF_WIFI_STA;
	esp_now_add_peer(&g_peer_esp8266.inf.sta);
	                 
}

bool is_same_macadrr(const uint8_t *mac_addr1, const uint8_t *mac_addr2)
{
	// avoid access to NULL pointer
	if (mac_addr1 == NULL || mac_addr2 == NULL) {
		if (mac_addr1 == NULL && mac_addr2 == NULL)
			return true;
		else
			return false;
	}
	
	// both not NULL	
	for (int i = 0; i < 6; i++)
	{
		if (mac_addr1[i] != mac_addr2[i])
			return false;
	}
	return true;
}
