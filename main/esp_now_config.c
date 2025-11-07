#include "esp_now_config.h"

My_Esp_Now_Typedef g_my_esp_now;

Peer_Typedef g_peer_esp32;
Peer_Typedef *g_peer[] = {&g_peer_esp32};
const uint32_t g_len_peer = sizeof(g_peer) / sizeof(g_peer[0]);
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
	init_my_esp_now();
	init_all_peer();
	
}

void init_my_esp_now(void)
{
	uint8_t my_macaddr[6] = {0xC8, 0xC9, 0xA3, 0x69, 0x88, 0x56};
	memcpy(g_my_esp_now.addr, my_macaddr, 6);
	g_my_esp_now.frame[0] = data_esp_now;
	g_my_esp_now.len_frame[0] = len_test_data_esp_now;
	
	g_my_esp_now.frame[1] = data_frame2;
	g_my_esp_now.len_frame[1] = len_test_data_2;
	
}

void init_all_peer(void)
{
	// init and add peer ESP32
	uint8_t peer_esp32_addr[6] = { 0x4C, 0xC3, 0x82, 0x0C, 0x96, 0x6C };
	memcpy(g_peer_esp32.inf.peer_addr, peer_esp32_addr, 6);
	g_peer_esp32.inf.channel = 0; // cùng kênh Wi-Fi 
	g_peer_esp32.inf.encrypt = false; 
	g_peer_esp32.total_request = 0;
	esp_now_add_peer(&g_peer_esp32.inf);
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

void check_send_request(void)
{
	for (int i = 0; i < g_len_peer; i++)
	{
		if (TOTAL_REQUEST == PEER_NOT_REQUEST)
			continue;
		for (int j = 0; j < TOTAL_REQUEST; j++)
		{
			esp_now_send(NULL, g_my_esp_now.frame[(*g_peer[i]).request[j]], g_my_esp_now.len_frame[(*g_peer[i]).request[j]]);
			vTaskDelay(pdMS_TO_TICKS(10));
		}
			
	}
	
}