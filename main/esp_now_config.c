
#include "esp_now_config.h"
#include "my_lib.h"
/*!
 * @file
 * @brief
 * 
 * @details
 *	frame				 : SLT + CMD + SIZE (W) + DATA_1, DATA_2, ... DATA_N + CHECKSUM 
 *		ADD_PEER		 : data = the address of this device
 *		GET_PEER		 : not need data
 *		ESPNOW_READ		 : 
 *		ESPNOW_WRITE	 :	
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
		if ( ( (data[len - 1] << 8) | data[len - 2]) == 
		    crc16_modbus((uint8_t*)&data[3], len - LEN_HEADER_ESPNOW - LEN_CRC_ESPNOW))
		{
			if (data[3] == BROADCAST)
			{
				espnow_add_peer((uint8_t*)mac_addr); 
			}
		
			else if (data[3] == ESPNOW_WRITE)
			{
				if (data[4] == 'a')
				{
					set_duty_pwm(&SLT.Pwm, 0, 650);
				}	
				else if (data[4] == 'b')
				{
					set_duty_pwm(&SLT.Pwm, 0, 800);
				}
			}			
		}

	}
}

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	
	if (status == ESP_NOW_SEND_SUCCESS) {
		uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		if (is_same_macadrr(mac_addr, broadcast))
		{
			set_duty_pwm(&SLT.Pwm, 1, 450);
			SLT.espnow.can_send = true;
			return;
		}
		
		SLT.espnow.sent = true;
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
	
	esp_wifi_get_mac(ESP_IF_WIFI_STA, SLT.espnow.my_addr);
	
	/* add peer broadcast */
	esp_now_peer_info_t peer = {0};
	uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 	
	memcpy(peer.peer_addr, broadcast, ESP_NOW_ETH_ALEN);
	peer.channel = CONFIG_ESPNOW_CHANNEL;
	peer.ifidx = WIFI_IF_STA;
	peer.encrypt = false;
	esp_now_add_peer(&peer); 
	
}

void espnow_add_peer(uint8_t* peer_addr)
{
	Peer_Typedef *tmp;
	if (SLT.espnow.tot_peer == 0)
		tmp = malloc(sizeof(Peer_Typedef));
	else {
		for (int i = 0; i < SLT.espnow.tot_peer; i++)
		{
			if (is_same_macadrr(peer_addr, (SLT.espnow.p_peer + i)->info.peer_addr))
			{
				return;
			}
		}
		tmp = realloc(SLT.espnow.p_peer, (SLT.espnow.tot_peer + 1) * sizeof(Peer_Typedef));
	}

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

uint16_t crc16_modbus(uint8_t *buf, uint32_t len)
{
	uint16_t crc = 0xffff;
	
	for (uint32_t i = 0; i < len; i++)
	{
		
		crc ^= buf[i];
		for (int j = 0; j < 8; j++)
		{
			
			if (crc & 1)
				crc = (crc >> 1) ^ 0xA001; 
			else 
				crc >>= 1;
		}
	}
	return crc;
}