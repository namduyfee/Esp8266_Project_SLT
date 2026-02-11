#include "my_lib.h"
#include "string.h"

#define MIN_DELAY 10 /*!< if delay task is short chip will be reset */
#define MIN_SIZE_OP_FILE 2048 /*!< task operate with spiffs file, it will need large size stack */ 
void task_esp_now_send(); 
void task_esp_now_recv();
void task_file_effect(); 
void task_select_master(); 
void task_send_tcp();

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
		.my_pos = 1,
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
		.p_client = NULL,
	}
	
};

QueueHandle_t xEffLoadf;					/**< get data from tcp recv callback */
QueueHandle_t xNowRecv;						/**< get data from espnow recv callback */
QueueHandle_t xNowSend;						
SemaphoreHandle_t xNowPeersMana; 
SemaphoreHandle_t xTcpSwitchBufSend; 
QueueHandle_t xSendTcp;

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
	
	xEffLoadf = xQueueCreate(30, sizeof(file_request_t));
	
	xNowRecv = xQueueCreate(20, sizeof(espnow_recv_queue_t));
	
	xNowSend = xQueueCreate(20, sizeof(espnow_send_queue_t)); 
	
	xSendTcp = xQueueCreate(20, sizeof( tcp_buf_t* )); 
	
	xNowPeersMana = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xNowPeersMana);
	
	xTcpSwitchBufSend = xSemaphoreCreateBinary();
	
	xTaskCreate(task_esp_now_send, "task_esp_now_send", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_file_effect, "task_tcp_file_bin", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_select_master, "task_select_master", MIN_SIZE_OP_FILE, NULL, 4, NULL);
		 
	xTaskCreate(task_esp_now_recv, "task_esp_now_recv", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_send_tcp, "task_send_tcp", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
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
				
				if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
				{	
					
					struct stat st;
					int ret = stat(PATH_GWAY_PEERS, &st);
					if (ret >= 0)
						unlink(PATH_GWAY_PEERS);
						
					int fd = open(PATH_GWAY_PEERS, O_RDWR | O_CREAT | O_TRUNC, 0666);	/**< open and trunc file */
					
					if (fd >= 0)
					{
						memcpy(SLT.wifi.gateway_addr, SLT.wifi.ap_macaddr, MAC_ADDR_LEN);
						lseek(fd, POS_ADDR_GATEWAY, SEEK_SET);
						write(fd, SLT.wifi.gateway_addr, MAC_ADDR_LEN);
						close(fd);
					}
					
					
					/** switch sta to ap */
					wifi_mode_t mode;
					esp_wifi_get_mode(&mode);
						
					esp_now_deinit();
					clear_all_peer();
					
					if (mode != WIFI_MODE_AP)
					{
						esp_wifi_stop();
						esp_wifi_set_mode(WIFI_MODE_AP);
							
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
					

					/** send request broadcast */
					uint8_t data[] = {SLT.espnow.my_pos}; 
					
					espnow_send_queue_t send_q; 
					send_q.retry_cnt = 0; 
					send_q.position = NOW_POS_PEER_BRC;
					send_q.buf = espnow_make_seg_cmd(NOW_BRC, data, sizeof(data));
					
					if (send_q.buf.data != NULL && send_q.buf.len > 0)
						xQueueSend(xNowSend, &send_q, portMAX_DELAY);
					
					xSemaphoreGive(xNowPeersMana);
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
	
	espnow_recv_queue_t espnow_recv;
	
	
	while (1)
	{
		if (xQueueReceive(xNowRecv, &espnow_recv, portMAX_DELAY) == pdPASS)
		{
			uint8_t* data = (uint8_t*)espnow_recv.buf.data;
			uint32_t len = espnow_recv.buf.len;
			
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
					if (data[NOW_INDEX_CMD] == NOW_BRC && (first_br == true || 
					                                       (xTaskGetTickCount() - last_tick_br > pdMS_TO_TICKS(TIME_BRC + 2000))))
					{ 
						if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
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
							
							struct stat st;
							int ret = stat(PATH_GWAY_PEERS, &st);
							if (ret >= 0)
								unlink(PATH_GWAY_PEERS);
							
							int fd = open(PATH_GWAY_PEERS, O_RDWR | O_CREAT | O_TRUNC, 0666);			/**< open and trunc file */
							
							if (fd >= 0)
							{
								memcpy(SLT.wifi.gateway_addr, espnow_recv.addr, MAC_ADDR_LEN);
								lseek(fd, POS_ADDR_GATEWAY, SEEK_SET);
								write(fd, SLT.wifi.gateway_addr, 6);
								SLT.wifi.is_gateway = false;
								close(fd);
							}
							
							espnow_add_peer(espnow_recv.addr, data[NOW_INDEX_POS], true, PATH_GWAY_PEERS);
							
							first_br = false;
							last_tick_br = xTaskGetTickCount();
							
							xSemaphoreGive(xNowPeersMana);
						}
						
						
						if (SLT.espnow.p_peer != NULL)
						{
							
							uint8_t data[7] = {SLT.espnow.my_pos}; 
							memcpy(&data[1], SLT.wifi.sta_macaddr, 6);
							
							espnow_send_queue_t send_q;
							send_q.retry_cnt = 10;
							send_q.position = SLT.espnow.p_peer->position;
							send_q.buf = espnow_make_seg_cmd(NOW_ADD_PEER, data, 7); 
							
							if (send_q.buf.data != NULL && send_q.buf.len != 0)
							{ 
							
								if (xQueueSend(xNowSend, &send_q, pdMS_TO_TICKS(200)) != pdPASS)
								{
									free(send_q.buf.data); 
								}
							}
							SLT.espnow.gateway_added = false;
						}
						
					}
					
					else if (data[NOW_INDEX_CMD] == NOW_ADD_PEER)
					{
						if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
						{
							espnow_add_peer(espnow_recv.addr, data[NOW_INDEX_POS], true, PATH_GWAY_PEERS);
							xSemaphoreGive(xNowPeersMana);
						}
					}
					else if (data[NOW_INDEX_CMD] == NOW_ST_WRF)
					{
						set_duty_pwm(&SLT.Pwm, 0, 210);
							
						uint8_t data[] = {NOW_WRF}; 
							
						espnow_send_queue_t send_q;
						send_q.retry_cnt = 10;
						send_q.position = SLT.espnow.p_peer->position;
						send_q.buf = espnow_make_seg_cmd(NOW_ACK, data, 7); 
							
						if (send_q.buf.data != NULL && send_q.buf.len != 0)
						{ 
							
							if (xQueueSend(xNowSend, &send_q, pdMS_TO_TICKS(200)) != pdPASS)
							{
								free(send_q.buf.data); 
							}
						}
					}
					else if (data[NOW_INDEX_CMD] == NOW_WRF)
					{

					}
				}
				if (espnow_recv.buf.data != NULL) {
					free(espnow_recv.buf.data);	
					espnow_recv.buf.data = NULL;
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
	
	espnow_send_queue_t rd_queue;
	
	while (1)
	{	

		if (xSemaphoreTake(xNowPeersMana, 0) == pdPASS)
		{
			if (SLT.espnow.is_broadcast == true)
			{

				if (SLT.espnow.can_send == true)
				{
					SLT.espnow.can_send = false;
				
					if (esp_now_send(broadcast,
						rd_queue.buf.data,
						rd_queue.buf.len) != ESP_OK)
					{
						SLT.espnow.can_send = true;
					}
				}
			
				if (xTaskGetTickCount() - br_lasttick > pdMS_TO_TICKS(TIME_BRC))
				{
					if (rd_queue.buf.data != NULL)
						free(rd_queue.buf.data);
					
					SLT.espnow.is_broadcast = false;
					SLT.wifi.is_gateway = true;
					SLT.espnow.can_send = true;
				}
			}
			else
			{
				if (xQueueReceive(xNowSend, &rd_queue, 0) == pdPASS)
				{
					bool free_queue = true;
				
					if (rd_queue.position == NOW_POS_PEER_BRC) 
					{
						br_lasttick = xTaskGetTickCount();
						SLT.espnow.is_broadcast = true;
						free_queue = false; 
					}
					else if (rd_queue.position == NOW_POS_ALL_PEER)
					{
						for (int i = 0; i < SLT.espnow.tot_pos_added; i++)
						{
							if (SLT.espnow.p_peer != NULL)
							{
								
								espnow_send_queue_t tm_queue = rd_queue; 
								
								tm_queue.buf.data = malloc(rd_queue.buf.len); 
								tm_queue.buf.len = rd_queue.buf.len; 
							
								memcpy(tm_queue.buf.data, rd_queue.buf.data, rd_queue.buf.len);
								
								espnow_make_node_send(SLT.espnow.p_peer + i, tm_queue);
							}
						}
						
						if (rd_queue.buf.data != NULL)
							free(rd_queue.buf.data);
						
						free_queue = false;
						
					}
					else
					{
						for (int i = 0; i < SLT.espnow.tot_pos_added; i++)
						{
							if (SLT.espnow.p_peer != NULL && (SLT.espnow.p_peer + i)->position == rd_queue.position)
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
			xSemaphoreGive(xNowPeersMana);			
		}
		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
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


void task_file_effect()
{	
	int fd = -100; 
	int fd_tmp = -100; 
	
	file_request_t file_req = {
		.cmd = F_NONE, 
	};  
	
	while (1)
	{
		if (xQueueReceive(xEffLoadf, &file_req, portMAX_DELAY) == pdPASS)
		{	
			if (file_req.cmd == F_OP)
			{
				
				if (fd < 0)
				{
					struct stat st;
					int ret = stat(PATH_EFFECT, &st);
					fd = ret < 0 ?  open(PATH_EFFECT, O_RDWR | O_CREAT | O_TRUNC, 0666) : 
									open(PATH_EFFECT, O_RDWR | O_CREAT, 0666);
				}
				
				if (fd >= 0)
				{
					tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_OP, true); 
					if(p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
				else 
				{
					tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_OP, false); 
					if(p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
			}
			else if (file_req.cmd == F_CLS)
			{
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}
				
				if (fd < 0)
				{
					tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_CLS, true); 
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						} 
				}
				else 
				{
					tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_CLS, false);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
			}			
			else if (file_req.cmd == F_DLT)
			{

				if (fd >= 0) 
				{
					close(fd);
					fd = -100;
				}
				unlink(PATH_EFFECT);
				
				struct stat st;
				int ret = stat(PATH_EFFECT, &st);
				
				if (ret < 0)
				{
					tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_DLT, true);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
				else 
				{
					tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_DLT, false);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
			}
			
			else if (file_req.cmd == F_RD)
			{
				if (fd < 0)
				{
					tcp_buf_t* p_buf = tcp_make_st_ret_read('F', 0, 0);
					
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
				else
				{
					if (file_req.read.offset >= lseek(fd, 0, SEEK_END) || 
					    file_req.read.len > (lseek(fd, 0, SEEK_END) - file_req.read.offset))
					{
						tcp_buf_t* p_buf = tcp_make_st_ret_read('f', 0, 0);
						if (p_buf != NULL)
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL)
									free(p_buf->data); 
								if (p_buf != NULL)
									free(p_buf); 
							}
					}
					else
					{
						
						uint32_t len_f = lseek(fd, 0, SEEK_END);
						off_t current_off = lseek(fd, file_req.read.offset, SEEK_SET); 
						uint32_t remaining = file_req.read.len;
						
						
						tcp_buf_t* p_buf = tcp_make_st_ret_read(true, current_off, len_f);
						if (p_buf != NULL)
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL)
									free(p_buf->data); 
								if (p_buf != NULL)
									free(p_buf); 
							}
						
						
						TickType_t last_tick = xTaskGetTickCount(); 
						while (remaining > 0)
						{
							
							if (xTaskGetTickCount() - last_tick > pdMS_TO_TICKS(5000))
								break; 
							
							uint32_t len = remaining <= 512 ? remaining : 512; 
							uint8_t* data = malloc(len);
							
							if (data == NULL)
								continue; 
							
							lseek(fd, current_off, SEEK_SET);
							read(fd, data, len);
							
							tcp_buf_t* p_buf = tcp_make_ret_read(data, len, current_off); 
							
							if (p_buf != NULL)
							{
								if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) == pdPASS)
								{
									current_off = lseek(fd, 0, SEEK_CUR);
									remaining = remaining - len;
								}
								else
								{
									if (p_buf != NULL && p_buf->data != NULL)
										free(p_buf->data); 
									if (p_buf != NULL)
										free(p_buf);
								}
							}
							
							if (data != NULL)
								free(data); 
							data = NULL; 
						}						
					}
				}
			}
			else if (file_req.cmd == F_ST_WR)
			{
				if (fd_tmp >= 0)
				{
					close(fd_tmp);
					fd_tmp = -100; 
				}
				fd_tmp = open(PATH_EFFECT_TMP, O_RDWR | O_CREAT | O_TRUNC, 0666);  
							
				SLT.eff_file.write.tot_len = file_req.write_start.tot_len;
				SLT.eff_file.write.remaining = file_req.write_start.tot_len; 
				SLT.eff_file.write.offset_start = file_req.write_start.offset;
				SLT.eff_file.write.checksum = 0xffff;
				
//				espnow_send_queue_t send_q;
//				send_q.retry_cnt = 5;
//				send_q.position = NOW_POS_ALL_PEER;
//				send_q.buf = espnow_make_seg_cmd(NOW_ST_WRF, NULL, 0); 
//							
//				if (send_q.buf.data != NULL && send_q.buf.len != 0)
//					if (xQueueSend(xNowSend, &send_q, pdMS_TO_TICKS(200)) != pdPASS)
//					{
//						free(send_q.buf.data); 
//					}
				
			}
			else if (file_req.cmd == F_WR)
			{
				if (fd < 0)
				{
					if (file_req.write.buf.data != NULL)
					{
						free(file_req.write.buf.data); 	
						file_req.write.buf.data = NULL; 
					}		
				}
				else
				{
					if (file_req.write.buf.data != NULL)
					{
						
						if (SLT.eff_file.write.remaining > 0 && 
						    file_req.write.offset >= SLT.eff_file.write.offset_start)
						{
							if (fd_tmp >= 0)
							{
								
								tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_WRT, 't');
								if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
								{
									if (p_buf != NULL && p_buf->data != NULL)
										free(p_buf->data); 
									if (p_buf != NULL)
										free(p_buf); 
								}
								
								lseek(fd_tmp, file_req.write.offset - SLT.eff_file.write.offset_start, SEEK_SET);
								
								ssize_t to_write = SLT.eff_file.write.remaining <= file_req.write.buf.len ?
													SLT.eff_file.write.remaining : file_req.write.buf.len;
							
								ssize_t written = 0; 
							
								if (to_write > 0)
								{
									written = write(fd_tmp,
										file_req.write.buf.data, 
										to_write);
								}
								
								if (written > 0) 
								{
									SLT.eff_file.write.checksum = crc16_modbus(SLT.eff_file.write.checksum, file_req.write.buf.data, written); 
									SLT.eff_file.write.remaining = SLT.eff_file.write.remaining - written; 
								}
								
							}
						}
						else
						{
							tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_WRT, 'f');
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL)
									free(p_buf->data); 
								if (p_buf != NULL)
									free(p_buf); 
							}
						}
						
						if (file_req.write.buf.data != NULL)
						{
							free(file_req.write.buf.data); 	
							file_req.write.buf.data = NULL; 
						}
					}
				}
			}
			else if (file_req.cmd == F_END_WR)
			{
				if (SLT.eff_file.write.checksum == file_req.write_end.checksum && fd >= 0 && fd_tmp >= 0)
				{
					
					int remaining = SLT.eff_file.write.tot_len; 
									
					off_t posfd = SLT.eff_file.write.offset_start; 
					off_t posfd_tmp = 0;
								
					uint8_t* bufA = malloc(sizeof(uint8_t) * 512);
									
									
					while (remaining > 0)
					{
						lseek(fd, posfd, SEEK_SET);
						lseek(fd_tmp, posfd_tmp, SEEK_SET);

						size_t len = remaining > 512 ? 
							512 : remaining;
										
						read(fd_tmp, bufA, len);
						ssize_t written = write(fd, bufA, len);
										
						if (written > 0)
						{
							remaining -= written;
							posfd += written;
							posfd_tmp += written;
						}
					}
					if (bufA != NULL)
						free(bufA);
					
					tcp_buf_t* p_buf = tcp_make_ret_doc(F_RET_WRT, 'T');
					if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
					{
						if (p_buf != NULL && p_buf->data != NULL)
							free(p_buf->data); 
						if (p_buf != NULL)
							free(p_buf); 
					}
				}
				
				if (fd_tmp >= 0) 
				{
					close(fd_tmp); 
					fd_tmp = -100;
				}
				 				
				struct stat st;
				int ret = stat(PATH_EFFECT_TMP, &st);
				if (ret >= 0)
					unlink(PATH_EFFECT_TMP);
			}
		}
	}
}

/**
 *	@brief	task handl and 
 */
void task_send_tcp()
{
	tcp_buf_t* tcp_send_buf = NULL;
	
	xSemaphoreTake(xTcpSwitchBufSend, 0);
	SLT.server.send.sent = false;
	
	while (1)
	{
		if (tcp_send_buf == NULL)
		{
			xQueueReceive(xSendTcp, &tcp_send_buf, portMAX_DELAY); 
		}
		
		while (tcpip_callback(tcp_send_cb, tcp_send_buf) != ERR_OK)
		{
			vTaskDelay(pdMS_TO_TICKS(5));
		}
		
		xSemaphoreTake(xTcpSwitchBufSend, portMAX_DELAY);
		
		if (SLT.server.send.sent == true) 
		{
			if (tcp_send_buf != NULL && tcp_send_buf->data != NULL)
				free(tcp_send_buf->data);
			if (tcp_send_buf != NULL)
				free(tcp_send_buf);
			
			tcp_send_buf = NULL;
		}
	}
}

