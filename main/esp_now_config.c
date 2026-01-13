
#include "esp_now_config.h"
#include "my_lib.h"
/*!
 * @file
 * @brief
 * 
 * @details
 *	frame				 : SLT + CMD + SIZE + DATA_1, DATA_2, ... DATA_N + CHECKSUM 
 *	
 *		ADD_PEER		 : SLT + ADD_PEER (CMD) + SIZE + POSTION TO ADD + ADDRESS TO ADD + CHECKSUM
 *		GET_PEER		 : SLT + GET_PEER (CMD) + SIZE + POSTION TO GET + CHECKSUM (IF POSTION = 0xFF THEN GET ALL PEER) + CHECKSUM
 *		ESPNOW_READ		 : 
 *		ESPNOW_WRITE	 : SLT + ESPNOW_WRITE (CMD) + SIZE + SEQUENCE NUMBER + DATA1, .. DATAN + CHECKSUM	
 */

static void init_my_esp_now(void);
static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);

static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) 
{
	if (mac_addr == NULL || data == NULL || len <= 0) {
		return;
	}
	
	if (( (uint16_t)(data[len - 1] << 8) | data[len - 2]) == 
	crc16_modbus((uint8_t*)&data[ESPNOW_INDEX_CMD], len - ESPNOW_LEN_HEADER - ESPNOW_LEN_CRC))
	{
		buf_espnow_t tm;
		
		tm.data = malloc(len - 2);		/**< last 2 bytes are crc and it checked */
		tm.len     = len - 2;
		if (tm.data != NULL)
		{
			memcpy(tm.data, data, len - 2);
			xQueueSendToBack(xEspNowRecv, &tm, portMAX_DELAY);
		}
	}
}

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	if (status == ESP_NOW_SEND_SUCCESS) {
		
		if (xSemaphoreTake(xSendEspNow, portMAX_DELAY) == pdPASS)
		{
			if (SLT.espnow.mode_send == BROADCAST)
			{
				
			}
			else if (SLT.espnow.mode_send == ADD_PEER)
			{
				SLT.espnow.gateway_added = true; 
			}
			else if (SLT.espnow.mode_send == ESPNOW_READ || 
					 SLT.espnow.mode_send == ESPNOW_WRITE)
			{
				SLT.espnow.sent = true;
			}
			
			xSemaphoreGive(xSendEspNow);
		}
		
	}
	
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
	
	/* add peer broadcast */
	esp_now_peer_info_t peer = {0};
	uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 	
	memcpy(peer.peer_addr, broadcast, ESP_NOW_ETH_ALEN);
	peer.channel = CONFIG_ESPNOW_CHANNEL;
	peer.ifidx = WIFI_IF_STA;
	peer.encrypt = false;
	
	if (esp_now_is_peer_exist(broadcast) != true)
		esp_now_add_peer(&peer); 
	
	struct stat st;
	int ret = stat("/spiffs/gateway.bin", &st);
	int fd = -1;
	
	if (ret >= 0)
	{
		fd = open("/spiffs/gateway.bin", O_RDONLY | O_CREAT, 0666);
	}
	
	if (fd >= 0)
	{
		
		off_t current_f = lseek(fd, 6, SEEK_SET); 
		uint32_t len_f  = lseek(fd, 0, SEEK_END); 
	
		if (current_f + 7 <= len_f)
		{
			uint8_t* tm_info = (uint8_t*)malloc(1 + 6);		/**< 1 byte position and 6 byte address */
			if (tm_info != NULL)
			{
				while (current_f + 7 <= len_f)
				{
					lseek(fd, current_f, SEEK_SET);
					read(fd, tm_info, 1 + 6);
					espnow_add_peer(&tm_info[1], tm_info[0], false);
					current_f = lseek(fd, 0, SEEK_CUR);
				}
				free(tm_info);
			}
		}
		close(fd);
	}
}

void clear_all_peer(void)
{
	if (SLT.espnow.p_peer != NULL)
	{
		free(SLT.espnow.p_peer);
		SLT.espnow.p_peer = NULL;
	}
	if (SLT.espnow.tot_pos_added > 0)
		SLT.espnow.tot_pos_added = 0;
}
uint8_t espnow_add_peer(uint8_t* peer_addr, uint8_t position, bool save)
{
	Peer_Typedef *tmp = NULL;
	if (SLT.espnow.tot_pos_added == 0)
		tmp = malloc(sizeof(Peer_Typedef));
	else {
		for (int i = 0; i < SLT.espnow.tot_pos_added; i++)
		{
			if (position == (SLT.espnow.p_peer + i)->position)
			{
				esp_err_t ret = ESP_OK;
				
				if (esp_now_is_peer_exist((SLT.espnow.p_peer + i)->info.peer_addr) == true)
				{
					ret = esp_now_del_peer((SLT.espnow.p_peer + i)->info.peer_addr);	/**< return ESP_OK if delete complete */
				}
				
				if (ret != ESP_OK)
				{
					return 0;
				}
				
				if (save == true)
				{
					struct stat st;
					int ret2 = stat("/spiffs/gateway.bin", &st);
					int fd = ret2 < 0 ?  open("/spiffs/gateway.bin", O_RDWR | O_CREAT | O_TRUNC, 0666) : 
										open("/spiffs/gateway.bin", O_RDWR | O_CREAT, 0666);
	
					if (fd < 0)
					{
						return 0;
					}
					memcpy((SLT.espnow.p_peer + i)->info.peer_addr, peer_addr, MAC_ADDR_LEN); 
					(SLT.espnow.p_peer + i)->info.channel = CONFIG_ESPNOW_CHANNEL;
					(SLT.espnow.p_peer + i)->info.encrypt = false;
					(SLT.espnow.p_peer + i)->info.ifidx = ESP_IF_WIFI_STA;
					esp_now_add_peer(&(SLT.espnow.p_peer + i)->info);
				
					off_t current_f = lseek(fd, 6, SEEK_SET); 
					uint32_t len_f  = lseek(fd, 0, SEEK_END); 
					uint8_t* tm_info = (uint8_t*)malloc(1 + 6);						
					bool same_pos = false;

					if (tm_info == NULL)
					{
						close(fd);
						return 0;		
					}
				
					while (current_f + 7 <= len_f)
					{
						lseek(fd, current_f, SEEK_SET); 
						read(fd, tm_info, 7);
						if (tm_info[0] == position)
						{
							same_pos = true;
							break;
						}
						current_f = lseek(fd, 0, SEEK_CUR);
					}
					if (same_pos == true)
					{
						lseek(fd, current_f - 7, SEEK_SET);
						write(fd, &position, 1);
						write(fd, peer_addr, 6);
					}
					else
					{
						if (len_f >= 6)
							lseek(fd, 0, SEEK_END);
						else 
							lseek(fd, 6, SEEK_SET);
						
						write(fd, &position, 1);
						write(fd, peer_addr, 6);			
					}	
							
					free(tm_info);
					close(fd);
					return 1;
				}
				else
				{
					memcpy((SLT.espnow.p_peer + i)->info.peer_addr, peer_addr, MAC_ADDR_LEN); 
					(SLT.espnow.p_peer + i)->info.channel = CONFIG_ESPNOW_CHANNEL;
					(SLT.espnow.p_peer + i)->info.encrypt = false;
					(SLT.espnow.p_peer + i)->info.ifidx = ESP_IF_WIFI_STA;
					esp_now_add_peer(&(SLT.espnow.p_peer + i)->info);	
					return 1;
				}
			}
		}
		tmp = realloc(SLT.espnow.p_peer, (SLT.espnow.tot_pos_added + 1) * sizeof(Peer_Typedef));
	}
	
	if (tmp == NULL)
	{
		return 0;
	}
	
	SLT.espnow.p_peer = tmp;
	SLT.espnow.tot_pos_added++;
	(SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->position = position;
	
	
	memcpy((SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info.peer_addr, peer_addr, MAC_ADDR_LEN);
	(SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info.channel = CONFIG_ESPNOW_CHANNEL;
	(SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info.encrypt = false;
	(SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info.ifidx = ESP_IF_WIFI_STA;
	
	if (esp_now_is_peer_exist((SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info.peer_addr) != true) {
		esp_now_add_peer(&(SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info);  
		
	}
	if (save == true)
	{
		struct stat st;
		int ret = stat("/spiffs/gateway.bin", &st);
		int fd = ret < 0 ?  open("/spiffs/gateway.bin", O_RDWR | O_CREAT | O_TRUNC, 0666) : 
							open("/spiffs/gateway.bin", O_RDWR | O_CREAT, 0666);
	
		if (fd >= 0)
		{
			lseek(fd, 0, SEEK_END);
			write(fd, &position, 1);
			write(fd, peer_addr, 6);
			close(fd);
		
			return 1;
		}	
	}
	else
	{
		return 1;
	}
	
	return 0;
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

buf_espnow_t* espnow_make_segment(void* buf, uint8_t cmd, uint32_t len)
{
	buf_espnow_t* newbuf = malloc(sizeof(buf_espnow_t));
	if (newbuf == NULL)
		return NULL;
	
	newbuf->len = len + ESPNOW_LEN_CMD + ESPNOW_LEN_CRC + 
	                         ESPNOW_LEN_HEADER + ESPNOW_LEN_SIZE_DT; 
	
	newbuf->data = malloc(newbuf->len);
	
	if (newbuf->data == NULL) {
		free(newbuf);
		return NULL;
	}
	*((uint8_t*)newbuf->data + ESPNOW_INDEX_HEADER) = 'S'; 
	*((uint8_t*)newbuf->data + ESPNOW_INDEX_HEADER + 1) = 'L';
	*((uint8_t*)newbuf->data + ESPNOW_INDEX_HEADER + 2) = 'T';
	
	*((uint8_t*)newbuf->data + ESPNOW_INDEX_CMD) = cmd;
	
	memcpy((uint8_t*)newbuf->data + ESPNOW_INDEX_SIZE_DT,
		&len,sizeof(uint32_t));
	
	memcpy( (uint8_t*)newbuf->data + ESPNOW_INDEX_DATA, (uint8_t*)buf, len);
	
	uint16_t crc = crc16_modbus((uint8_t*)newbuf->data + ESPNOW_INDEX_CMD, 
	                            newbuf->len - ESPNOW_LEN_HEADER - ESPNOW_LEN_CRC); 
	
	*((uint8_t*)newbuf->data + newbuf->len - 1) = (crc >> 8) & 0xFF;
	*((uint8_t*)newbuf->data + newbuf->len - 2) = crc & 0xFF;
	
	return newbuf;
}

void espnow_free_segment(buf_espnow_t* buf)
{
	if (buf == NULL)
		return;
	buf_espnow_t* free_buf = (buf_espnow_t*)buf;
	
	if (free_buf->data != NULL)
		free(free_buf->data);
	free(buf);
}