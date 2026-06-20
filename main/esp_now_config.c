
#include "esp_now_config.h"
#include "my_lib.h"
/*!
 * @file
 * @brief
 * 
 * @details
 *	frame				 : KEY + CMD + DATA_1, DATA_2, ... DATA_N + CHECKSUM 
 *	
 *		ADD_PEER		 : KEY + ADD_PEER (CMD) + POSTION TO ADD + ADDRESS TO ADD + CHECKSUM
 *		GET_PEER		 : KEY + GET_PEER (CMD) + POSTION TO GET + CHECKSUM (IF POSTION = 0xFF THEN GET ALL PEER) + CHECKSUM
 *		ESPNOW_READ		 : 
 *		ESPNOW_WRITE	 : KEY + ESPNOW_WRITE (CMD) + SEQUENCE NUMBER + DATA1, .. DATAN + CHECKSUM	
 */

static void init_my_esp_now(void);
static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);

static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) 
{
	if (mac_addr == NULL || data == NULL || len <= 0) {
		return;
	}
	if (data[0] != 'N' || data[1] != 'O' || data[2] != 'W')
	{
		return; 
	}	
	
	if (( (uint16_t)(data[len - 1] << 8) | data[len - 2]) == 
	crc16_modbus(0xffff, (uint8_t*)data, len - NOW_SZOF_CRC))
	{
		espnow_recv_queue_t tm;
		
		memcpy(tm.addr, mac_addr, MAC_ADDR_LEN);
		
		tm.buf.data = malloc(len - 2);		/**< last 2 bytes are crc and it checked */
		tm.buf.tot_byte     = len - 2;
		if (tm.buf.data != NULL)
		{
			memcpy(tm.buf.data, data, len - 2);
			
			if (xQueueSendToBack(xNowRecv, &tm, pdMS_TO_TICKS(00)) != pdPASS)
				free(tm.buf.data); 
		}
	}
}

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
	
	SLT.espnow.send_success = (status == ESP_NOW_SEND_SUCCESS);
	xSemaphoreGive(xNowSendDone);
	
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
	
	/** add peer broadcast */
	esp_now_peer_info_t peer = {0};
	uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	memcpy(peer.peer_addr, broadcast, ESP_NOW_ETH_ALEN);
	peer.channel = CONFIG_ESPNOW_CHANNEL;
	
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode);
	
	if (mode == WIFI_MODE_STA)
		peer.ifidx = WIFI_IF_STA;
	else if(mode == WIFI_MODE_AP)
		peer.ifidx = WIFI_IF_AP;
	
	peer.encrypt = false;
	
	if (esp_now_is_peer_exist(broadcast) != true)
		esp_now_add_peer(&peer); 
	
	/** read peer list infomation in nvs */
	nvs_handle handle; 
	if (nvs_open(NVS_ESPNOW_NAMESP, NVS_READONLY, &handle) == ESP_OK)
	{
		size_t size = sizeof(SLT.espnow.peer_list);
		if (nvs_get_blob(handle, NVS_ADDED_PEERS_INF, SLT.espnow.peer_list, &size) == ESP_OK
		    && nvs_get_u8(handle, NVS_COUNT_PEER, &SLT.espnow.cnt_id_added) == ESP_OK)
		{
			for (int i = 0; i < SLT.espnow.cnt_id_added; i++)
			{
				
				esp_now_peer_info_t peer_i = {0};
				memcpy(peer_i.peer_addr, SLT.espnow.peer_list[i].mac, ESP_NOW_ETH_ALEN);
				peer_i.channel = CONFIG_ESPNOW_CHANNEL;
	
				wifi_mode_t mode;
				esp_wifi_get_mode(&mode);
	
				if (mode == WIFI_MODE_STA)
					peer_i.ifidx = WIFI_IF_STA;
				else if (mode == WIFI_MODE_AP)
					peer_i.ifidx = WIFI_IF_AP;
	
				peer_i.encrypt = false;
				
				if (esp_now_is_peer_exist(SLT.espnow.peer_list[i].mac) != true)
					esp_now_add_peer(&peer_i);
			}
		}
		
		nvs_close(handle); 
	}
	
}

void clear_all_peer(void)
{

	SLT.espnow.cnt_id_added = 0;
	nvs_handle handle; 
	if (nvs_open(NVS_ESPNOW_NAMESP, NVS_READWRITE, &handle) == ESP_OK)
	{
		nvs_set_u8(handle, NVS_COUNT_PEER, SLT.espnow.cnt_id_added);
		nvs_commit(handle); 
		nvs_close(handle); 
	}
}

void espnow_add_peer(uint8_t* peer_addr, uint8_t id)
{
	
	esp_now_peer_info_t peer = {0};
	memcpy(peer.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
	peer.channel = CONFIG_ESPNOW_CHANNEL;
	
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode);
	
	if (mode == WIFI_MODE_STA)
		peer.ifidx = WIFI_IF_STA;
	else if (mode == WIFI_MODE_AP)
		peer.ifidx = WIFI_IF_AP;
	
	peer.encrypt = false;
	
	if (esp_now_is_peer_exist(peer_addr) != true)
		esp_now_add_peer(&peer); 
	
	nvs_handle handle; 
	size_t size = sizeof(SLT.espnow.peer_list);
	
	if (nvs_open(NVS_ESPNOW_NAMESP, NVS_READWRITE, &handle) == ESP_OK)
	{
		bool id_exist = false; 
		
		for (int i = 0; i < SLT.espnow.cnt_id_added; i++)
		{
			if (id == SLT.espnow.peer_list[i].id)
			{
				memcpy(SLT.espnow.peer_list[i].mac, peer_addr, MAC_ADDR_LEN);
				nvs_set_blob(handle, NVS_ADDED_PEERS_INF, SLT.espnow.peer_list, size);
				nvs_commit(handle);
				id_exist = true;
			}
		
		}	
		
		if (id_exist == false)
		{
			if (SLT.espnow.cnt_id_added < MAX_PEER)
			{
				SLT.espnow.peer_list[SLT.espnow.cnt_id_added].id = id;
				memcpy(SLT.espnow.peer_list[SLT.espnow.cnt_id_added].mac, peer_addr, MAC_ADDR_LEN); 
				nvs_set_blob(handle, NVS_ADDED_PEERS_INF, SLT.espnow.peer_list, size);
				
				SLT.espnow.cnt_id_added++;
				nvs_set_u8(handle, NVS_COUNT_PEER, SLT.espnow.cnt_id_added);
				nvs_commit(handle);
			}
			
		}
		nvs_close(handle);
	}
}

bool is_same_macadrr(const uint8_t *mac1, const uint8_t *mac2)
{
	if (mac1 == NULL || mac2 == NULL)
	{
		return mac1 == mac2;
	}
	
	return memcmp(mac1, mac2, MAC_ADDR_LEN) == 0;
}

uint16_t crc16_modbus(uint16_t crc, uint8_t *buf, uint32_t len)
{	
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

buf_espnow_t espnow_make_frame_send(void* payload, uint32_t len_payload, command_espnow_t cmd)
{
	buf_espnow_t buf = {.data = NULL, .tot_byte = 0};
	
	buf.tot_byte = NOW_SZOF_HEADER + NOW_SZOF_CMD + len_payload + NOW_SZOF_CRC; 
	buf.data = malloc(buf.tot_byte);
	
	if (buf.data == NULL || buf.tot_byte == 0)
		return buf; 
	
	buf.data[0] = 'N'; buf.data[1] = 'O'; buf.data[2] = 'W';
	buf.data[NOW_INDEX_CMD] = cmd; 
	
	if (payload != NULL && len_payload != 0)
		memcpy(&buf.data[NOW_INDEX_PAYLOAD], payload, len_payload); 
	
	uint16_t crc = crc16_modbus(0xffff, buf.data, buf.tot_byte - NOW_SZOF_CRC); 
	
	buf.data[buf.tot_byte - 2] = crc & 0xff; 
	buf.data[buf.tot_byte - 1] = (crc >> 8) & 0xff;
	
	return buf; 
}