#include "esp_now_config.h"
#include "nvs_flash.h"
My_Esp_Now_Typedef g_my_esp_now;

Peer_Typedef g_peer_esp8266;
Peer_Typedef *g_send_list_peer[20][20] = {
	{&g_peer_esp8266, NULL},
	{NULL}
};


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
	uint8_t my_macaddr[6] = {0x18, 0xFE, 0x34, 0xEE, 0x4E, 0x99};
//	uint8_t my_macaddr[6] = {0x84, 0xF3, 0xEB, 0xA6, 0xD8, 0x4F};
//	uint8_t my_macaddr[6] = { 0xC8, 0xC9, 0xA3, 0x69, 0x88, 0x56 };
	memcpy(g_my_esp_now.addr, my_macaddr, 6);
	g_my_esp_now.start = 0; 
	g_my_esp_now.can_send = true;
	for (int i = 0; i < ESP_NOW_MAX_LEN; i++)
		g_my_esp_now.send_frame[i] = NULL;
	
	
}

void init_all_peer(void)
{
	// init and add peer ESP8266
	uint8_t peer_esp8266_addr[6] = { 0x84, 0xF3, 0xEB, 0xA6, 0xD8, 0x4F };
//	uint8_t peer_esp8266_addr[6] = { 0x18, 0xFE, 0x34, 0xEE, 0x4E, 0x99 };
//	uint8_t peer_esp8266_addr[6] = { 0xC8, 0xC9, 0xA3, 0x69, 0x88, 0x56 }; 
	memcpy(g_peer_esp8266.inf_sta.peer_addr, peer_esp8266_addr, 6);
	for (int i = 0; i < ESP_NOW_MAX_LEN; i++)
		g_peer_esp8266.buffer_receive[i] = NULL;
	
	g_peer_esp8266.inf_sta.channel = CONFIG_ESPNOW_CHANNEL; // cùng kênh Wi-Fi 

	g_peer_esp8266.inf_sta.encrypt = false; 
	g_peer_esp8266.inf_sta.ifidx = ESP_IF_WIFI_STA;
	esp_now_add_peer(&g_peer_esp8266.inf_sta);
	                 
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
				vTaskDelay(pdMS_TO_TICKS(10));
			}
		}
		else
		{
			uint8_t j = 0;
			
			while (g_send_list_peer[i][j] != NULL)
			{
				if (g_my_esp_now.can_send == true) {
					g_my_esp_now.can_send = false;
					esp_err_t ret = esp_now_send((*g_send_list_peer[i][j]).inf_sta.peer_addr, g_my_esp_now.send_frame[i], g_my_esp_now.len_send_frame[i]);
					while (ret != ESP_OK)
					{
						vTaskDelay(pdMS_TO_TICKS(10));
						ret = esp_now_send((*g_send_list_peer[i][j]).inf_sta.peer_addr, g_my_esp_now.send_frame[i], g_my_esp_now.len_send_frame[i]);
					}			
					j++;
					vTaskDelay(pdMS_TO_TICKS(10));
				}	
			}
			i++;
		}
	}
	g_my_esp_now.start = 0;
}