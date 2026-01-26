#include "my_lib.h"
#include "string.h"

#define MIN_DELAY 10 /*!< if delay task is short chip will be reset */
#define MIN_SIZE_OP_FILE 2048 /*!< task operate with spiffs file, it will need large size stack */ 
void task_esp_now_send(); 
void task_esp_now_recv();
void task_tcp_file_bin(); 
void task_select_master(); 


Object SLT = {
	.Pwm = {
		.gpio_channel = {
		GPIO_CHANNEL_0,
		GPIO_CHANNEL_1,
		GPIO_CHANNEL_2,
		GPIO_CHANNEL_3,
		GPIO_CHANNEL_4,
		GPIO_CHANNEL_5,
		GPIO_CHANNEL_6,
		GPIO_CHANNEL_7
	},
		.duty = {0, 0, 0, 0, 0, 0, 0, 0},
		.num_channel_en = 0
	},
	
	.espnow = {
		.p_peer = NULL,
		.tot_pos_added = 0,
		.gateway_added = false,
		.my_pos = 10,
		.is_broadcast = false,
		.can_send = true,
	},
	.wifi = {
		.is_gateway = false,
		.gateway_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}		
	},
	.server = {
		.tpcb = NULL,
		.port = 80,
		.count_client = 0,
		.max_client = 1,
		.recv = {
			.current_pos_file = 0,
			.tot_len = 0,
			.cmd = TCP_NONE
		},
		.client = NULL
	}
	
};

QueueHandle_t xBuffLoadf;					/**< get data from tcp recv callback */
QueueHandle_t xEspNowRecv;					/**< get data from espnow recv callback */
QueueHandle_t xEspNowSend;

SemaphoreHandle_t xEspNowf;

void my_init_project(void)
{
	
	nvs_flash_init();
	spiffs_init();
	config_GPIO_OUT();
	config_input_pullup_gpio();
	
	my_pwm_start(&SLT.Pwm); 
	
	tcpip_adapter_init();
	
	my_start_wifi(&SLT.wifi);
	
	init_server_tpcp(80, 1);
	
	init_espnow();
}


void app_main(void) {
	
	
	my_init_project();
	
	xBuffLoadf = xQueueCreate(30, sizeof(tcp_recv_t));
	
	xEspNowRecv = xQueueCreate(10, sizeof(buf_espnow_t));
	
	xEspNowSend = xQueueCreate(20, sizeof(espnow_send_queue_t)); 
	
	
	xEspNowf = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xEspNowf); 
		
	xTaskCreate(task_esp_now_send, "task_esp_now_send", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_tcp_file_bin, "task_tcp_file_bin", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_select_master, "task_select_master", MIN_SIZE_OP_FILE, NULL, 4, NULL);
		 
	xTaskCreate(task_esp_now_recv, "task_esp_now_recv", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
}

/**
 * @brief	task handler select master
 * 
 * @details	
 *	- press button to become master
 *	- erase and init file that store info gateway and peers
 *	- switch from sta mode to apsta mode
 *	- broadcast
 * @note
 *	- semaphore
 *	
 */
void task_select_master()
{
	
	while (1)
	{
		if(gpio_get_level(BUT_SEL_MASTER) == PRES_SEL_MASTER)   
		{
			int count = 0;
			
			while (gpio_get_level(BUT_SEL_MASTER) == PRES_SEL_MASTER)
			{
				vTaskDelay(pdMS_TO_TICKS(20));
				
				if (count < 100) {
					count++;
				}
			}
			
			if (count >= 100)
			{
				/* handle when button is pressed */
				

				if (xSemaphoreTake(xEspNowf, portMAX_DELAY) == pdPASS)
				{	
					/** erase and restore file that has info gateway and peer */
					struct stat st;
					int ret = stat("/spiffs/gateway.bin", &st);
					if(ret >= 0)
						unlink("/spiffs/data.bin");
					
					int fd = open("/spiffs/gateway.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
					
					if (fd >= 0)
					{
						
						SLT.wifi.is_gateway = true;
						memcpy(SLT.wifi.gateway_addr, SLT.wifi.sta_macaddr, MAC_ADDR_LEN);
						lseek(fd, POS_ADDR_GATEWAY, SEEK_SET);
						write(fd, SLT.wifi.gateway_addr, MAC_ADDR_LEN);
						close(fd);
					}
					xSemaphoreGive(xEspNowf);
				}	
				
				/** if info of gateway is stored, switch wifi mode and begin broadcast */
				if (SLT.wifi.is_gateway == true)
				{		
					wifi_mode_t mode;
					esp_wifi_get_mode(&mode);
						
					esp_now_deinit();
					clear_all_peer();
					
					if (mode != WIFI_MODE_APSTA)
					{
						esp_wifi_stop();
						esp_wifi_set_mode(WIFI_MODE_APSTA);
							
						char result[32];
						snprintf(result, sizeof(result), "SLT_ESP%d", SLT.espnow.my_pos);
				
						wifi_config_t ap_config = {
							.ap = {
							.max_connection = 4,
							.password = "12345678",
							.channel = CONFIG_ESPNOW_CHANNEL, 
							.authmode = WIFI_AUTH_WPA_WPA2_PSK		
							}	
						};
						memcpy(ap_config.ap.ssid, result, strlen(result));
						ap_config.ap.ssid_len = strlen(result);
							
						esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);		
						esp_wifi_start();				
					
					}
					init_espnow();
					
					uint8_t data[7] = {SLT.espnow.my_pos};
					memcpy(&data[1], SLT.wifi.sta_macaddr, 6); 
					
					espnow_send_queue_t send_q = {
						.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
						.retry_cnt = 0,
					};
					send_q.buf = espnow_make_seg_cmd(NOW_BRC, data, 7); 
					xQueueSend(xEspNowSend, &send_q, portMAX_DELAY); 
				}
			}
		}		
		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
	}
}
/**
 * @brief	task handler espnow receive
 *	
 * @details
 *	- get data from espnow recv callback through queue			
 *
 *
 */
void task_esp_now_recv()
{
	TickType_t last_tick_br = xTaskGetTickCount(); 
	bool first_br = true;
	
	while (1)
	{
		if (xQueueReceive(xEspNowRecv, &SLT.espnow.recv.buf, portMAX_DELAY) == pdPASS)
		{
			uint8_t* data = (uint8_t*)SLT.espnow.recv.buf.data;
			uint32_t len = SLT.espnow.recv.buf.len;
			
			if (data != NULL && len > 0)
			{
				if (data[0] == 'N' && data[1] == 'O' && data[2] == 'W')
				{
					
					/** 
					 *	if recv broadcast
					 *		- erase and restore file that has info gateway and peers
					 *		- switch from apsta mode to sta mode
					 *		- send request add peer to master
					 */
					if ( data[NOW_INDEX_CMD] == NOW_BRC && (first_br == true || 
					                                       (xTaskGetTickCount() - last_tick_br > pdMS_TO_TICKS(6000)) ) )
					{ 
						
						esp_now_deinit();
						clear_all_peer();			/**< delete list peer to creat new list peer */
						
						wifi_mode_t mode;
						esp_wifi_get_mode(&mode);
						
						if (mode != WIFI_MODE_STA)
						{
							esp_wifi_stop();
							esp_wifi_set_mode(WIFI_MODE_STA);
							esp_wifi_start();
							esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, 0);
						}
						init_espnow();
						
						
						/** store gateway macaddr */	
						bool saved = false;
						
						if (xSemaphoreTake(xEspNowf, portMAX_DELAY) == pdPASS)
						{
							
							struct stat st;
							int ret = stat("/spiffs/gateway.bin", &st);
							if (ret >= 0)
								unlink("/spiffs/data.bin");
						
							int fd = open("/spiffs/gateway.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);			/**< open and trunc file */
						
							if (fd >= 0)
							{
								memcpy(SLT.wifi.gateway_addr, &data[NOW_INDEX_ADDR], MAC_ADDR_LEN);
								lseek(fd, POS_ADDR_GATEWAY, SEEK_SET);
								write(fd, SLT.wifi.gateway_addr, 6);
								SLT.wifi.is_gateway = false;
								close(fd);
								
								espnow_add_peer((uint8_t*)&data[NOW_INDEX_ADDR], data[NOW_INDEX_POS], true);		/**< add gateway into list peer */
								saved = true;
							}
							xSemaphoreGive(xEspNowf);
						}
						if (saved == true)
						{
							/** after add peer gateway; send request add this peer to gateway */
							if (SLT.espnow.p_peer != NULL)
							{
								uint8_t data[7] = {SLT.espnow.my_pos}; 
								memcpy(&data[1], SLT.wifi.sta_macaddr, 6);
								
								espnow_send_queue_t send_q;
								uint8_t gateway_add[6] = {0x84, 0xF3, 0xEB, 0xA6, 0xD8, 0x4F}; 
								
								memcpy(send_q.addr, gateway_add, 6); 
								//memcpy(send_q.addr, SLT.wifi.gateway_addr, 6); 
								send_q.buf = espnow_make_seg_cmd(NOW_ADD_PEER, data, 7); 
								send_q.retry_cnt = 50;
								send_q.position = SLT.espnow.p_peer->position;
								xQueueSend(xEspNowSend, &send_q, portMAX_DELAY);
								SLT.espnow.gateway_added = false;
								set_duty_pwm(&SLT.Pwm, 0, 0);
							}
							
							first_br = false;
							last_tick_br = xTaskGetTickCount();
						}
					}
					
					else if (data[NOW_INDEX_CMD] == NOW_ADD_PEER)
					{
						set_duty_pwm(&SLT.Pwm, 0, 0);
						set_duty_pwm(&SLT.Pwm, 1, 0);
						
						espnow_add_peer((uint8_t*)&data[NOW_INDEX_ADDR], data[NOW_INDEX_POS], true);
						
						if (SLT.espnow.p_peer != NULL)
						{
							espnow_send_queue_t send_q;
							memcpy(send_q.addr, SLT.espnow.p_peer->info.peer_addr, 6); 
							uint8_t data [] = "a"; 
							send_q.position = SLT.espnow.p_peer->position;
							send_q.buf = espnow_make_seg_cmd(NOW_WRF, data, sizeof(data)); 
							send_q.retry_cnt = 10;
							xQueueSend(xEspNowSend, &send_q, portMAX_DELAY);
						}

					}
					else if (data[NOW_INDEX_CMD] == NOW_WRF)
					{
						if (data[NOW_INDEX_DATA] == 'a')
						{
							
							if (SLT.espnow.p_peer != NULL)
							{
								espnow_send_queue_t send_q;
								memcpy(send_q.addr, SLT.espnow.p_peer->info.peer_addr, 6); 
								uint8_t data [] = "b"; 
								send_q.position = SLT.espnow.p_peer->position;
								send_q.buf = espnow_make_seg_cmd(NOW_WRF, data, sizeof(data)); 
								send_q.retry_cnt = 10;
								xQueueSend(xEspNowSend, &send_q, portMAX_DELAY);
							}

						}	
						else if (data[NOW_INDEX_DATA] == 'b')
						{
							
							
							if (SLT.espnow.p_peer != NULL)
							{
								espnow_send_queue_t send_q;
								memcpy(send_q.addr, SLT.espnow.p_peer->info.peer_addr, 6); 
								uint8_t data [] = "a"; 
								send_q.position = SLT.espnow.p_peer->position;
								send_q.buf = espnow_make_seg_cmd(NOW_WRF, data, sizeof(data)); 
								send_q.retry_cnt = 10;
								xQueueSend(xEspNowSend, &send_q, portMAX_DELAY);
							}
						}
					}
				}
				if (SLT.espnow.recv.buf.data != NULL) {
					free(SLT.espnow.recv.buf.data);	
					SLT.espnow.recv.buf.data = NULL;
				}
			}

		}
		
	}
	
}
/**
 * @brief	task send espnow
 *	
 * @note
 *	- vtaskdelay must be called after givesemaphore; unless this task will takesemaphore sooner other task
 */
void task_esp_now_send() 
{
	uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
	TickType_t br_lasttick = xTaskGetTickCount();
	SLT.espnow.can_send = true;
	
	uint8_t add_send_all_per[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
	espnow_send_queue_t rd_queue;
	
	while (1)
	{	
		
		if (SLT.espnow.is_broadcast == true)
		{			
			if (SLT.espnow.can_send == true)
			{
				SLT.espnow.can_send = false;
				
				if (esp_now_send(rd_queue.addr,
					rd_queue.buf.data,
					rd_queue.buf.len) != ESP_OK)
				{
					SLT.espnow.can_send = true;
				}
			}
			
			if (xTaskGetTickCount() - br_lasttick > pdMS_TO_TICKS(4000))
			{
				SLT.espnow.is_broadcast = false;
				if (rd_queue.buf.data != NULL)
					free(rd_queue.buf.data);
				SLT.espnow.can_send = true;
			}
		}
		else
		{
			if (xQueueReceive(xEspNowSend, &rd_queue, 0) == pdPASS)
			{
				bool free_queue = true;
				
				if (memcmp(rd_queue.addr, broadcast, MAC_ADDR_LEN) == 0) 
				{
					br_lasttick = xTaskGetTickCount();
					SLT.espnow.is_broadcast = true;
					free_queue = false; 
				}
				else if(memcmp(rd_queue.addr, add_send_all_per, MAC_ADDR_LEN) == 0)
				{
					free_queue = false;
				}
				else
				{
					for (int i = 0; i < SLT.espnow.tot_pos_added; i++)
					{
						if ((SLT.espnow.p_peer + i)->position == rd_queue.position)
						{
							free_queue = false;
							espnow_make_node_send(SLT.espnow.p_peer + i, rd_queue);
							break;
						}
					}
				}
				
				if (free_queue == true)
					if (rd_queue.buf.data != NULL)
						free(rd_queue.buf.data);
				
			}
			if (SLT.espnow.is_broadcast != true)
			{
				for (int i = 0; i < SLT.espnow.tot_pos_added; i++)
				{

					if ((SLT.espnow.p_peer + i)->send.p_hnode != NULL)
					{
						if ((SLT.espnow.p_peer + i)->send.p_hnode->retry_cnt > 0) 
						{
							if (SLT.espnow.can_send == true)
							{
								SLT.espnow.can_send = false;
								if (esp_now_send((SLT.espnow.p_peer + i)->info.peer_addr,
									(SLT.espnow.p_peer + i)->send.p_hnode->buf.data,
									(SLT.espnow.p_peer + i)->send.p_hnode->buf.len) != ESP_OK)
								{
									SLT.espnow.can_send = true;
								}
								else
								{
									set_duty_pwm(&SLT.Pwm, 1, 0);
									(SLT.espnow.p_peer + i)->send.p_hnode->retry_cnt--;
								}
							}
						}
						else 
						{
							espnow_swt_node_send((SLT.espnow.p_peer + i));
							SLT.espnow.can_send = true; 
						}
					}
				}
			}		
		}
		vTaskDelay(pdMS_TO_TICKS(NOW_SEND_CYCLE_MS));
	}
}

/**
 *	@brief	handler request from tcp
 *	
 *	@details	
 *		- open
 *		- close
 *		- delete
 *		- read
 *		- write
 *		
 *	@note
 *		
 */

void task_tcp_file_bin()
{
	gpio_set_level(GPIO_NUM_15, 0);
	gpio_set_level(GPIO_NUM_0, 1);
	
	int fd = -100; 
	while (1)
	{
		if (xQueueReceive(xBuffLoadf, &SLT.server.recv.segment, portMAX_DELAY) == pdPASS)
		{	
			if (SLT.server.recv.segment.command != TCP_NONE)
			{
				SLT.server.recv.cmd = SLT.server.recv.segment.command;
			}
			if (SLT.server.recv.cmd == TCP_OPF)
			{
				
				if (SLT.server.recv.segment.buf.data != NULL) 
				{
					free(SLT.server.recv.segment.buf.data);
					SLT.server.recv.segment.buf.data = NULL;
				}
				
				if (fd < 0)
				{
					struct stat st;
					int ret = stat("/spiffs/data.bin", &st);
					fd = ret < 0 ?  open("/spiffs/data.bin", O_RDWR | O_CREAT | O_TRUNC, 0666) : 
									open("/spiffs/data.bin", O_RDWR | O_CREAT, 0666);
					
				}
				
				if (fd >= 0)
				{
					tcp_ret_cmd(TCP_RET_OPF, true); 
				}
				else 
				{
					tcp_ret_cmd(TCP_RET_OPF, false);
				}
				SLT.server.recv.cmd = TCP_NONE;
			}
			else if (SLT.server.recv.cmd == TCP_CLSF)
			{
				if (SLT.server.recv.segment.buf.data != NULL) 
				{
					free(SLT.server.recv.segment.buf.data);
					SLT.server.recv.segment.buf.data = NULL;
				}
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}
				
				if (fd < 0)
				{
					tcp_ret_cmd(TCP_RET_CLSF, true); 
				}
				else 
				{
					tcp_ret_cmd(TCP_RET_CLSF, false);
				}
				SLT.server.recv.cmd = TCP_NONE;
			}			
			else if (SLT.server.recv.cmd == TCP_DLTF)
			{
				if (SLT.server.recv.segment.buf.data != NULL) 
				{
					free(SLT.server.recv.segment.buf.data);
					SLT.server.recv.segment.buf.data = NULL;
				}

				if (fd >= 0) 
				{
					close(fd);
					fd = -100;
				}
				unlink("/spiffs/data.bin");
				
				struct stat st;
				int ret = stat("/spiffs/data.bin", &st);
				
				if (ret < 0)
				{
					tcp_ret_cmd(TCP_RET_DLTF, true); 
				}
				else 
				{
					tcp_ret_cmd(TCP_RET_DLTF, false);
				}
				SLT.server.recv.cmd = TCP_NONE;
			}
			
			else if (SLT.server.recv.cmd == TCP_RDF)
			{
				if (fd < 0)
				{
					if (SLT.server.recv.segment.buf.data != NULL) 
					{
						free(SLT.server.recv.segment.buf.data);
						SLT.server.recv.segment.buf.data = NULL;
					}
						tcp_ret_cmd(TCP_RET_RDF, false);
				}
				else
				{

					
					if (SLT.server.recv.segment.pos_in_file >= 
					    lseek(fd, 0, SEEK_END))
					{
						tcp_ret_cmd(TCP_RET_RDF, false);
					}
					else
					{

						
						uint32_t len_f = lseek(fd, 0, SEEK_END);
						
						off_t current_off = lseek(fd, SLT.server.recv.segment.pos_in_file, SEEK_SET); 
						
						uint32_t remaining = SLT.server.recv.segment.buf.len;
						
						tcp_ret_cmd(TCP_RET_RDF, true);
						
						SLT.server.send.buf = malloc(sizeof(tcp_buf_t));
						
						SLT.server.send.buf->data = malloc(sizeof(uint32_t) * 2);
						
						((uint32_t*)SLT.server.send.buf->data)[0] = current_off;
						((uint32_t*)SLT.server.send.buf->data)[1] = remaining <= len_f - current_off ? remaining : len_f - current_off;
						SLT.server.send.buf->len = sizeof(uint32_t) * 2;
						tcpip_callback(tcp_send_cb, SLT.server.send.buf); 
					
						while (remaining > 0)
						{
							bool can_break = false;
							lseek(fd, current_off, SEEK_SET);
							
							SLT.server.send.buf = malloc(sizeof(tcp_buf_t));
							if (SLT.server.send.buf == NULL)
							{
								continue;
							}
							SLT.server.send.buf->len = remaining <= 512 ? remaining : 512; 
							
							if (len_f - current_off < SLT.server.send.buf->len)
							{
								SLT.server.send.buf->len = len_f - current_off;
								SLT.server.send.buf->data = malloc(SLT.server.send.buf->len);
								if (SLT.server.send.buf->data != NULL)
								{
									read(fd, SLT.server.send.buf->data, SLT.server.send.buf->len);
									
									if (tcpip_callback(tcp_send_cb, SLT.server.send.buf) != ERR_OK)
									{
										if (SLT.server.send.buf->data != NULL) {
											free(SLT.server.send.buf->data);
											SLT.server.send.buf->data = NULL;
										}
										if (SLT.server.send.buf != NULL) {
											free(SLT.server.send.buf);
											SLT.server.send.buf = NULL;
										}
									}
									else
									{
										current_off = lseek(fd, 0, SEEK_CUR);
										can_break = true;
									}
								}
								else
								{
									if (SLT.server.send.buf != NULL) {
										free(SLT.server.send.buf);
										SLT.server.send.buf = NULL;
									}
								}

							}
							else
							{
								SLT.server.send.buf->data = malloc(SLT.server.send.buf->len); 
						
								if (SLT.server.send.buf->data != NULL)
								{
									read(fd, SLT.server.send.buf->data, SLT.server.send.buf->len);
								
									if (tcpip_callback(tcp_send_cb, SLT.server.send.buf) != ERR_OK)
									{
										if (SLT.server.send.buf->data != NULL) {
											free(SLT.server.send.buf->data);
											SLT.server.send.buf->data = NULL;
										}
										if (SLT.server.send.buf != NULL) {
											free(SLT.server.send.buf);
											SLT.server.send.buf = NULL;
										}
									}
									else
									{
										current_off = lseek(fd, 0, SEEK_CUR);
										remaining = remaining - SLT.server.send.buf->len;
									}						
								}
								else
								{
									if (SLT.server.send.buf != NULL) {
										free(SLT.server.send.buf);
										SLT.server.send.buf = NULL;
									}
								}
							}
							if (can_break == true)
								break;
						}						
					}

					if (SLT.server.recv.segment.buf.data != NULL) 
					{
						free(SLT.server.recv.segment.buf.data);
						SLT.server.recv.segment.buf.data = NULL;
					}
				}
				SLT.server.recv.cmd = TCP_NONE;
			}
			else if (SLT.server.recv.cmd == TCP_WRF)
			{
				if (fd < 0)
				{
					if (SLT.server.recv.segment.buf.data != NULL)
					{
						free(SLT.server.recv.segment.buf.data); 	
						SLT.server.recv.segment.buf.data = NULL; 
					}	
					SLT.server.recv.cmd = TCP_NONE;
				}
				else
				{				
					if (SLT.server.recv.segment.buf.data != NULL)
					{
						if (SLT.server.recv.segment.tot_len != REMAINING)
						{
							SLT.server.recv.tot_len = SLT.server.recv.segment.tot_len; 
						}
						if (SLT.server.recv.tot_len > 0)
						{
							if (SLT.server.recv.segment.pos_in_file == POS_CONTINUE)
							{
								lseek(fd, SLT.server.recv.current_pos_file, SEEK_SET);
							}
							else
							{
								lseek(fd, SLT.server.recv.segment.pos_in_file, SEEK_SET);
							}
							
							ssize_t to_write = SLT.server.recv.tot_len <= SLT.server.recv.segment.buf.len ?
												SLT.server.recv.tot_len : SLT.server.recv.segment.buf.len;
							
							ssize_t written = 0; 
							
							if (to_write > 0)
							{
								written = write(fd,
									&((uint8_t*)SLT.server.recv.segment.buf.data)[SLT.server.recv.segment.pos_data], 
									to_write);
							}
							
							if (written > 0)
								SLT.server.recv.tot_len = SLT.server.recv.tot_len - written; 
							
							SLT.server.recv.current_pos_file = lseek(fd, 0, SEEK_CUR);
							
							if (SLT.server.recv.tot_len == 0) 
							{
								tcp_ret_cmd(TCP_RET_WRT, true);
							}
						}
						else 
							SLT.server.recv.cmd = TCP_NONE;
						
						if (SLT.server.recv.segment.buf.data != NULL)
						{
							free(SLT.server.recv.segment.buf.data); 		
							SLT.server.recv.segment.buf.data = NULL;				
						}
					}
					else
						SLT.server.recv.cmd = TCP_NONE;

				}
			}
			else
			{
				if (SLT.server.recv.segment.buf.data != NULL) 
				{
					free(SLT.server.recv.segment.buf.data);
					SLT.server.recv.segment.buf.data = NULL;
				}				
			}
			
		}
	}
}


