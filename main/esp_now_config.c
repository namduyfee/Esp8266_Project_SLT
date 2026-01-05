
#include "esp_now_config.h"
#include "my_lib.h"
/*!
 * @file
 * @brief
 * 
 * @details
 *	frame with command	 : SLT + command + data_1 + ... data_n 
 *		ADD_PEER		 : data = the address of this device
 *		GET_PEER		 : not need data
 *		ESPNOW_READ		 : 
 *		ESPNOW_WRITE	 :	
 *	frame not with command : 
 */

static void init_my_esp_now(void);
static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);

static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) 
{
	if (mac_addr == NULL || data == NULL || len <= 0) {
		return;
	}
	
	if (data[0] == 'S' && data[1] == 'L' && data[2] == 'T')
	{
		if (data[3] == ADD_PEER)
		{
			espnow_add_peer((uint8_t*)&data[4]); 
		}
		
		else if (data[3] == ESPNOW_WRITE)
		{
			if (data[4] == 'a')
			{
				set_duty_pwm(&SLT.Pwm, 0, 300);
			}	
			else if (data[4] == 'b')
			{
				set_duty_pwm(&SLT.Pwm, 0, 800);
			}
		}
	}
}

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	
	if (status == ESP_NOW_SEND_SUCCESS) {
		
		if (SLT.espnow.gateway_added == false && is_same_macadrr(mac_addr, SLT.gateway_addr))
		{
			SLT.espnow.gateway_added = true;
		}
		else {
			SLT.espnow.sent = true; 
			set_duty_pwm(&SLT.Pwm, 1, 450);
		}
		
	}
	/*
	if (is_same_macadrr(mac_addr, g_peer_esp32.inf.peer_addr) == true)
	{
			
	}
	*/
	
	SLT.espnow.can_send = true;
}

void init_espnow(void) 
{
	esp_now_init();
	esp_now_register_recv_cb(on_data_recv);
	esp_now_register_send_cb(on_data_sent);
	init_my_esp_now();
}

static void init_my_esp_now(void)
{	
	esp_now_peer_info_t peer = {0};
	uint8_t broadcard[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 	
	memcpy(peer.peer_addr, broadcard, ESP_NOW_ETH_ALEN);
	peer.channel = CONFIG_ESPNOW_CHANNEL;
	peer.ifidx = WIFI_IF_STA;
	peer.encrypt = false;
	esp_now_add_peer(&peer); 
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
