
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
	crc16_modbus((uint8_t*)data, len - NOW_LEN_CRC))
	{
		buf_espnow_t tm;
		tm.data = malloc(len - 2);		/**< last 2 bytes are crc and it checked */
		tm.len     = len - 2;
		if (tm.data != NULL)
		{
			memcpy(tm.data, data, len - 2);
			
			if (xQueueSendToBack(xNowRecv, &tm, pdMS_TO_TICKS(200)) != pdPASS)
				free(tm.data); 
		}
	}
}

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	uint8_t br_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	
	if (status == ESP_NOW_SEND_SUCCESS) 
	{
		if (memcmp(br_addr, mac_addr, 6) == 0) 
		{
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
	
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode); 
	
	if (mode == WIFI_MODE_STA)
		peer.ifidx = WIFI_IF_STA;
	else if(mode == WIFI_MODE_AP)
		peer.ifidx = WIFI_IF_AP;
	
	peer.encrypt = false;
	
	if (esp_now_is_peer_exist(broadcast) != true)
		esp_now_add_peer(&peer); 
	
	struct stat st;
	int ret = stat(PATH_GWAY_PEERS, &st);
	int fd = -1;
	
	if (ret >= 0)
	{
		fd = open(PATH_GWAY_PEERS, O_RDONLY | O_CREAT, 0666);
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
					espnow_add_peer(&tm_info[1], tm_info[0], false, PATH_GWAY_PEERS);
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
	if (SLT.espnow.p_peer != NULL && SLT.espnow.tot_pos_added > 0)
	{
		for (int i = 0; i < SLT.espnow.tot_pos_added; i++)
		{
			espnow_free_all_node(SLT.espnow.p_peer + i);
		}
		
		if (SLT.espnow.p_peer != NULL)
		{
			free(SLT.espnow.p_peer);
			SLT.espnow.p_peer = NULL;
		}
		if (SLT.espnow.tot_pos_added > 0)
			SLT.espnow.tot_pos_added = 0;
	}
}

void init_new_peer(Peer_Typedef* p_peer, uint8_t* peer_addr, uint8_t position)
{
	
	memcpy(p_peer->info.peer_addr, peer_addr, MAC_ADDR_LEN); 
	p_peer->info.channel = CONFIG_ESPNOW_CHANNEL;
	p_peer->info.encrypt = false;
	
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode);
	
	if (mode == WIFI_MODE_STA)
		p_peer->info.ifidx = WIFI_IF_STA;
	else if (mode == WIFI_MODE_AP)
		p_peer->info.ifidx = WIFI_IF_AP;
	
	p_peer->position = position;
	
	p_peer->send.p_hnode = NULL;

	p_peer->recv.buf = NULL;
	p_peer->recv.tot_buf = 0;
}

uint8_t espnow_add_peer(uint8_t* peer_addr, uint8_t position, bool save, const char* path)
{
	Peer_Typedef *tmp = NULL;
	if (SLT.espnow.tot_pos_added == 0)
		tmp = malloc(sizeof(Peer_Typedef));
	else
	{
		for (int i = 0; i < SLT.espnow.tot_pos_added; i++)
		{
			if (position == (SLT.espnow.p_peer + i)->position)
			{
				if (memcmp(peer_addr, (SLT.espnow.p_peer + i)->info.peer_addr, 6) == 0)
					return 1; 
				
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
					int ret2 = stat(path, &st);
					int fd = ret2 < 0 ?  open(path, O_RDWR | O_CREAT | O_TRUNC, 0666) : 
										open(path, O_RDWR | O_CREAT, 0666);
	
					if (fd < 0)
					{
						return 0;
					}
					
					init_new_peer(SLT.espnow.p_peer + i, peer_addr, position); 
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
					init_new_peer(SLT.espnow.p_peer + i, peer_addr, position); 
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
	
	init_new_peer(SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1, 
		peer_addr,
		position); 
	
	if (esp_now_is_peer_exist((SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info.peer_addr) != true)
	{
		esp_now_add_peer(&(SLT.espnow.p_peer + SLT.espnow.tot_pos_added - 1)->info);  
		
	}
	if (save == true)
	{
		struct stat st;
		int ret = stat(path, &st);
		int fd = ret < 0 ?  open(path, O_RDWR | O_CREAT | O_TRUNC, 0666) : 
							open(path, O_RDWR | O_CREAT, 0666);
	
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
	if (mac1 == NULL || mac2 == NULL)
	{
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
/**
	* @brief	send espnow with cmd
	*/
buf_espnow_t espnow_make_seg_cmd(command_espnow_t cmd, void* buf, uint32_t len)
{	
	uint8_t* tm_buf = (uint8_t*) buf;
	
	uint32_t len_send = NOW_LEN_HEADER + NOW_LEN_CMD + NOW_LEN_CRC + len;
	uint8_t* des_buf = malloc(len_send * sizeof(uint8_t));
	
	buf_espnow_t send_buf = {.data = NULL, .len = 0};
	
	if (des_buf == NULL)
		return send_buf; 
	
	des_buf[0] = 'N'; des_buf[1] = 'O'; des_buf[2] = 'W';
	des_buf[3] = cmd;
	
	if (buf != NULL && len != 0)
		memcpy(&des_buf[4], tm_buf, len);
	
	uint16_t crc = crc16_modbus(des_buf, len_send - NOW_LEN_CRC);
	des_buf[len_send - 2] = crc & 0xFF;
	des_buf[len_send - 1] = (crc >> 8) & 0xFF;
	
	send_buf.data = des_buf;
	send_buf.len = len_send;
	
	return send_buf;
}

uint8_t espnow_make_node_send(Peer_Typedef* p_peer, espnow_send_queue_t q_send)
{
	espnow_send_node_t* new_node = malloc(sizeof(espnow_send_node_t));
	
	if (new_node == NULL) 
	{ 
		if (q_send.buf.data != NULL)
			free(q_send.buf.data); 
		return 0;
	}
	new_node->buf = q_send.buf;
	new_node->retry_cnt = q_send.retry_cnt; 
	new_node->next = NULL;
	
	if (p_peer->send.p_hnode == NULL) 
		p_peer->send.p_hnode = new_node; 
	else
	{
		espnow_send_node_t* tm_node = p_peer->send.p_hnode;
		
		while (tm_node->next != NULL)
			tm_node = tm_node->next;
		
		tm_node->next = new_node; 
	}
	return 1; 
}

void espnow_swt_node_send(Peer_Typedef* p_peer)
{
	if (p_peer->send.p_hnode != NULL && p_peer->send.p_hnode->buf.data != NULL)
		free(p_peer->send.p_hnode->buf.data);
	
	if (p_peer->send.p_hnode != NULL)
	{
		espnow_send_node_t* tm_node = p_peer->send.p_hnode->next;
		free(p_peer->send.p_hnode); 
		p_peer->send.p_hnode = tm_node;
	}
}

void espnow_free_all_node(Peer_Typedef* p_peer)
{
	 
	while (p_peer->send.p_hnode != NULL)
	{
		espnow_send_node_t* tm_node = p_peer->send.p_hnode->next;
		
		if (p_peer->send.p_hnode->buf.data != NULL)
			free(p_peer->send.p_hnode->buf.data);
		
		free(p_peer->send.p_hnode); 
		
		p_peer->send.p_hnode = tm_node; 
	}
	p_peer->send.p_hnode = NULL;
}

