#include "esp_now_config.h"

My_Esp_Now_Typedef g_my_esp_now;

Peer_Typedef g_peer_esp32;
Peer_Typedef *g_send_list_peer[20][20] = {
	{&g_peer_esp32, NULL},
	{NULL}
};

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
	g_my_esp_now.can_send = true;
	for (int i = 0; i < ESP_NOW_MAX_LEN; i++)
		g_my_esp_now.send_frame[i] = NULL;
	
	g_my_esp_now.send_frame[0] = data_esp_now;
	g_my_esp_now.len_send_frame[0] = len_test_data_esp_now;
	
	g_my_esp_now.send_frame[1] = data_frame2;
	g_my_esp_now.len_send_frame[1] = len_test_data_2;
	
}

void init_all_peer(void)
{
	// init and add peer ESP32
	uint8_t peer_esp32_addr[6] = { 0x4C, 0xC3, 0x82, 0x0C, 0x96, 0x6C };
	memcpy(g_peer_esp32.inf.peer_addr, peer_esp32_addr, 6);
	for (int i = 0; i < ESP_NOW_MAX_LEN; i++)
		g_peer_esp32.buffer_receive[i] = NULL;
	
	g_peer_esp32.inf.channel = 0; // cùng kênh Wi-Fi 
	g_peer_esp32.inf.encrypt = false; 
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

void send_esp_now(void)
{
	uint8_t i = 0;
	while (g_my_esp_now.send_frame[i] != NULL)
	{
		if (g_send_list_peer[i][0] == NULL)
		{
			if (g_my_esp_now.can_send == true)
			{
				g_my_esp_now.can_send = false;
				
				esp_err_t ret = esp_now_send(NULL, g_my_esp_now.send_frame[i], g_my_esp_now.len_send_frame[i]);
				while (ret != ESP_OK)
				{
					vTaskDelay(pdMS_TO_TICKS(10));
					ret = esp_now_send(NULL, g_my_esp_now.send_frame[i], g_my_esp_now.len_send_frame[i]);
				}
				i++;
			}
		}
		else
		{
			uint8_t j = 0;
			
			while (g_send_list_peer[i][j] != NULL)
			{
				if (g_my_esp_now.can_send == true) {
					esp_err_t ret = esp_now_send((*g_send_list_peer[i][j]).inf.peer_addr, g_my_esp_now.send_frame[i], g_my_esp_now.len_send_frame[i]);
					while (ret != ESP_OK)
					{
						vTaskDelay(pdMS_TO_TICKS(10));
						ret = esp_now_send((*g_send_list_peer[i][j]).inf.peer_addr, g_my_esp_now.send_frame[i], g_my_esp_now.len_send_frame[i]);
					}			
					j++;
				}	
			}
			i++;
		}
	}
}