
#include "esp_now_config.h"
#include "my_lib.h"
/*!
 * @file
 * @brief
 * 
 * @details
 *	frame : "SLT" + command + data_1 + ... data_n 
 *		ADD_PEER		 : data = the address of this device
 *		GET_PEER		 : not need data
 *		ESPNOW_READ		 : 
 *		ESPNOW_WRITE	 :
 */


extern void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);
extern void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);


void init_espnow(void) 
{
	esp_now_init();
	esp_now_register_recv_cb(on_data_recv);
	esp_now_register_send_cb(on_data_sent);
	init_my_esp_now();
}

void init_my_esp_now(void)
{	
	esp_wifi_get_mac(ESP_IF_WIFI_STA, SLT.espnow.my_addr); 
	if (SLT.is_gateway == false)
	{
		SLT.espnow.p_peer = (Peer_Typedef*)malloc(sizeof(Peer_Typedef));
		
		if (SLT.espnow.p_peer != NULL)
		{
			memcpy(SLT.espnow.p_peer->info.peer_addr, SLT.gateway_addr, MAC_ADDR_LEN);
			SLT.espnow.p_peer->info.channel = CONFIG_ESPNOW_CHANNEL;
			SLT.espnow.p_peer->info.encrypt = false;
			SLT.espnow.p_peer->info.ifidx = ESP_IF_WIFI_STA;
			SLT.espnow.tot_peer++;
			esp_now_add_peer(&SLT.espnow.p_peer->info);			
		}

	}
}

void espnow_add_peer(uint8_t* peer_addr)
{
	Peer_Typedef *tmp;

	tmp = realloc(SLT.espnow.p_peer, (SLT.espnow.tot_peer + 1) * sizeof(Peer_Typedef));

	if (tmp == NULL)
	{
		return;
	}
	
	SLT.espnow.p_peer = tmp;
	SLT.espnow.tot_peer++;
	
	memcpy((SLT.espnow.p_peer + SLT.espnow.tot_peer - 1)->info.peer_addr, peer_addr, MAC_ADDR_LEN);
	(SLT.espnow.p_peer + SLT.espnow.tot_peer - 1)->info.channel = CONFIG_ESPNOW_CHANNEL;
	(SLT.espnow.p_peer + SLT.espnow.tot_peer - 1)->info.encrypt = false;
	(SLT.espnow.p_peer + SLT.espnow.tot_peer - 1)->info.ifidx = ESP_IF_WIFI_STA;
	
	esp_now_add_peer(&(SLT.espnow.p_peer + SLT.espnow.tot_peer - 1)->info);            
}

bool is_same_macadrr(const uint8_t *mac1, const uint8_t *mac2)
{
	if (mac1 == NULL || mac2 == NULL) {
		return mac1 == mac2;
	}

	return memcmp(mac1, mac2, MAC_ADDR_LEN) == 0;
}
