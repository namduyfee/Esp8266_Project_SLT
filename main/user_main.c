#include "my_lib.h"
#include "string.h"

#define MIN_DELAY 10 /*!< if delay task is short chip will be reset */
#define MIN_SIZE_OP_FILE 2048 /*!< task operate with spiffs file, it will need large size stack */ 
void task_esp_now_send(); 
void task_esp_now_recv();
void task_file_effect(); 
void task_select_master(); 
void task_send_tcp();
void task_init_effect(); 

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

		.cnt_id_added = 0,
		.my_id = 10,
	},
	.server = {
		.tpcb = NULL,
		.count_client = 0,
		.p_client = NULL,
	}
	
};

SemaphoreHandle_t xNowPeersMana; 
SemaphoreHandle_t xTcpSwitchBufSend;
SemaphoreHandle_t xUpdateEffect;
SemaphoreHandle_t xUpdatedEffect;
SemaphoreHandle_t xNowCanSend; 
SemaphoreHandle_t xNowCanUpdateSend; 

	
QueueHandle_t xEffLoadf;					/**< get data from tcp recv callback */
QueueHandle_t xNowRecv;						/**< get data from espnow recv callback */
QueueHandle_t xNowSend;						 
QueueHandle_t xSendTcp;

void my_init_project(void)
{
	 
	esp_err_t err = nvs_flash_init();

	if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		nvs_flash_erase();
		nvs_flash_init();
	}

	
	spiffs_init();
	
	esp_event_loop_create_default();
	
	config_GPIO_OUT();
	config_input_pullup_gpio(); 
	
	tcpip_adapter_init();
	
	my_start_wifi();
	
	init_server_tpcp(80, 1);
	
	init_espnow();
	
	my_pwm_start(&SLT.Pwm);
}


void app_main(void) {
	
	
	xEffLoadf = xQueueCreate(30, sizeof(file_request_t));
	
	xNowRecv = xQueueCreate(20, sizeof(espnow_recv_queue_t));
	
	xNowSend = xQueueCreate(20, sizeof(espnow_send_queue_t)); 
	
	xSendTcp = xQueueCreate(20, sizeof(tcp_buf_t*)); 
	
	xNowPeersMana = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xNowPeersMana);
	
	xUpdateEffect = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xUpdateEffect); 
	
	xTcpSwitchBufSend = xSemaphoreCreateBinary();
	
	xUpdatedEffect = xSemaphoreCreateBinary(); 
	
	xNowCanSend = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xNowCanSend);
	
	xNowCanUpdateSend = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xNowCanUpdateSend);
	
	my_init_project();
	
	xTaskCreate(task_esp_now_send, "task_esp_now_send", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_file_effect, "task_tcp_file_bin", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_select_master, "task_select_master", MIN_SIZE_OP_FILE, NULL, 4, NULL);
		 
	xTaskCreate(task_esp_now_recv, "task_esp_now_recv", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_send_tcp, "task_send_tcp", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_init_effect, "task_init_effect", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
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
				bool can_brc_espnow = false;
				
				if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
				{
					nvs_handle handle; 
					
					if (nvs_open(NVS_ESPNOW_NAMESP, NVS_READWRITE, &handle) == ESP_OK)
					{
						peer_info_t tm_gw_peer;
						size_t size = sizeof(tm_gw_peer);
						
						tm_gw_peer.id = SLT.espnow.my_id; 
						memcpy(tm_gw_peer.mac, SLT.wifi.ap_macaddr, MAC_ADDR_LEN); 

						if (nvs_set_blob(handle, NVS_GW_PEER_INF, &tm_gw_peer, size) == ESP_OK)
						{
							SLT.espnow.gw_peer.id = tm_gw_peer.id; 
							memcpy(SLT.espnow.gw_peer.mac, tm_gw_peer.mac, MAC_ADDR_LEN);
						
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
								snprintf(result, sizeof(result), "SLT_ESP%d", SLT.espnow.my_id);
				
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
							nvs_commit(handle);
							
							init_espnow();
							
							can_brc_espnow = true; 
						}
						
						nvs_close(handle); 
					}
					xSemaphoreGive(xNowPeersMana);
				}
				
				if (can_brc_espnow == true)
				{
					/** send request broadcast */
					uint8_t payload = SLT.espnow.my_id; 
							
					espnow_send_queue_t send_q; 
					send_q.dest_id = NOW_ID_BRC;
					send_q .buf = espnow_make_frame_send(&payload, sizeof(payload), NOW_BRC); 
					if (send_q.buf.data != NULL && send_q.buf.len > 0)
						xQueueSend(xNowSend, &send_q, portMAX_DELAY);		
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
	uint32_t tem = 0; 
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

					if ( data[NOW_INDEX_CMD] == NOW_BRC && 
					    (first_br == true || (xTaskGetTickCount() - last_tick_br > pdMS_TO_TICKS(TIME_BRC + 2000)))
					   )
					{
						bool send_add_req_to_gw = false; 
						
						if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
						{
							nvs_handle handle;
							if (nvs_open(NVS_ESPNOW_NAMESP, NVS_READWRITE, &handle) == ESP_OK)
							{
								peer_info_t tm_gw_peer; 
								size_t size = sizeof(tm_gw_peer);
								
								memcpy(tm_gw_peer.mac, espnow_recv.addr, MAC_ADDR_LEN); 
								tm_gw_peer.id = data[NOW_INDEX_PAYLOAD]; 
								
								if (nvs_set_blob(handle, NVS_GW_PEER_INF, &tm_gw_peer, size) == ESP_OK)
								{
									SLT.espnow.gw_peer.id = tm_gw_peer.id; 
									memcpy(SLT.espnow.gw_peer.mac, tm_gw_peer.mac, MAC_ADDR_LEN);
									
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
									
									nvs_commit(handle);
									
									init_espnow();
									
									espnow_add_peer(SLT.espnow.gw_peer.mac, SLT.espnow.gw_peer.id);
									
									send_add_req_to_gw = true; 
								}
								
								nvs_close(handle); 

								first_br = false;
								last_tick_br = xTaskGetTickCount();
							}
							xSemaphoreGive(xNowPeersMana);
						}
						
						if (send_add_req_to_gw == true)
						{
							/** send request add peer to gateway */
							uint8_t payload = SLT.espnow.my_id; 
							
							espnow_send_queue_t send_q; 
							send_q.dest_id = SLT.espnow.gw_peer.id;
							send_q .buf = espnow_make_frame_send(&payload, sizeof(payload), NOW_ADD_PEER); 
							if (send_q.buf.data != NULL && send_q.buf.len > 0)
								xQueueSend(xNowSend, &send_q, pdMS_TO_TICKS(3000));
						}
					}
					
					else if (data[NOW_INDEX_CMD] == NOW_ADD_PEER)
					{

						if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
						{
							espnow_add_peer(espnow_recv.addr, data[NOW_INDEX_PAYLOAD]);
				
							xSemaphoreGive(xNowPeersMana);
						}
						
						for (uint8_t i = 0; i < 10; i++)
						{
							espnow_send_queue_t send_q; 
							send_q.dest_id = data[NOW_INDEX_PAYLOAD];
							send_q.buf = espnow_make_frame_send(&i, sizeof(i), NOW_WRF); 
							if (send_q.buf.data != NULL && send_q.buf.len > 0)
								xQueueSend(xNowSend, &send_q, pdMS_TO_TICKS(1000));
						}
					}
					else if (data[NOW_INDEX_CMD] == NOW_ST_WRF)
					{

					}
					else if (data[NOW_INDEX_CMD] == NOW_WRF)
					{
						tem |= (1 << data[NOW_INDEX_PAYLOAD]);
						
						if (tem == 0b1111111111)
						{
							set_duty_pwm(&SLT.Pwm, 1, 255); 
							set_duty_pwm(&SLT.Pwm, 3, 255);	
						}
						else
						{
							set_duty_pwm(&SLT.Pwm, 1, 0); 
							set_duty_pwm(&SLT.Pwm, 3, 0);
						}
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
	uint8_t address_brc[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
	TickType_t br_lasttick = xTaskGetTickCount();
	bool is_broadcast = false; 
	
	espnow_send_queue_t queue_current = {.buf.data = NULL, .buf.len = 0};
	
	while (1)
	{	

		if (xSemaphoreTake(xNowPeersMana, 0) == pdPASS)
		{
			if (is_broadcast == true)
			{
				if (xTaskGetTickCount() - br_lasttick > pdMS_TO_TICKS(TIME_BRC))
				{
					is_broadcast = false;
					if (queue_current.buf.data != NULL && queue_current.buf.len > 0) 
					{
						free(queue_current.buf.data);
						queue_current.buf.data = NULL; 
						queue_current.buf.len = 0; 
					}
					xSemaphoreGive(xNowCanUpdateSend);
				}
				else
				{
					esp_now_send(address_brc, queue_current.buf.data, queue_current.buf.len);
					vTaskDelay(pdMS_TO_TICKS(100));
				}
			}
			else
			{
				if (xSemaphoreTake(xNowCanUpdateSend, 0) == pdPASS)
				{
					if (queue_current.buf.data != NULL && queue_current.buf.len > 0)
					{
						free(queue_current.buf.data);
						queue_current.buf.data = NULL; 
						queue_current.buf.len = 0;
					}
					if (xQueueReceive(xNowSend, &queue_current, 0) == pdPASS) 
					{
						if (queue_current.dest_id == NOW_ID_BRC)
						{
							is_broadcast = true; 
							br_lasttick = xTaskGetTickCount();
						}
					}
					else
					{
						xSemaphoreGive(xNowCanUpdateSend);
					}
				}
				
				if (is_broadcast == false)
				{
					if (xSemaphoreTake(xNowCanSend, 0) == pdPASS)
					{
						esp_err_t ret = -1;
						
						if (queue_current.buf.data != NULL && queue_current.buf.len > 0)
						{
							for (int i = 0; i < SLT.espnow.cnt_id_added; i++)
							{
								if (queue_current.dest_id == SLT.espnow.peer_list[i].id)
								{
									ret = esp_now_send(SLT.espnow.peer_list[i].mac, queue_current.buf.data, queue_current.buf.len);
									break;
								}
							}
						}
						if (ret != ESP_OK)
							xSemaphoreGive(xNowCanSend); 
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
				
				if (file_req.source == F_TCP_SOURCE)
				{
					tcp_buf_t* p_buf = fd >= 0 ? tcp_make_ret(TCP_ACK, NULL, 0) : tcp_make_ret(TCP_NACK, NULL, 0);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
				else if (file_req.source == F_NOW_SOURCE)
				{
					
					
				}

			}
			else if (file_req.cmd == F_CLS)
			{
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}
				
				if (file_req.source == F_TCP_SOURCE)
				{
					tcp_buf_t* p_buf = fd < 0 ? tcp_make_ret(TCP_ACK, NULL, 0) : tcp_make_ret(TCP_NACK, NULL, 0);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
				else if (file_req.source == F_NOW_SOURCE)
				{
					
					
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
				
				if (file_req.source == F_TCP_SOURCE)
				{
					tcp_buf_t* p_buf = ret < 0 ? tcp_make_ret(TCP_ACK, NULL, 0) : tcp_make_ret(TCP_NACK, NULL, 0);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
				else if (file_req.source == F_NOW_SOURCE)
				{
					
					
				}
			}
			
			else if (file_req.cmd == F_RD)
			{
				if (file_req.source == F_TCP_SOURCE)
				{
					if (fd < 0)
					{
						tcp_buf_t* p_buf = tcp_make_ret(TCP_NACK, NULL, 0);
					
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
							tcp_buf_t* p_buf = tcp_make_ret(TCP_NACK, NULL, 0);
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
						
							tcp_buf_t* p_buf = tcp_make_ret(TCP_ACK, NULL, 0);
							if (p_buf != NULL)
								if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
								{
									if (p_buf != NULL && p_buf->data != NULL)
										free(p_buf->data); 
									if (p_buf != NULL)
										free(p_buf); 
								}
						
							off_t current_off = lseek(fd, file_req.read.offset, SEEK_SET); 
							uint32_t remaining = file_req.read.len;
						
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
			}
			else if (file_req.cmd == F_ST_WR)
			{
				if (fd_tmp >= 0)
				{
					close(fd_tmp);
					fd_tmp = -100; 
				}
				fd_tmp = open(PATH_EFFECT_TMP, O_RDWR | O_CREAT | O_TRUNC, 0666);  
				
				if (fd_tmp >= 0)
				{
					SLT.eff_file.write.tot_len = file_req.write_start.tot_len;
					SLT.eff_file.write.remaining = file_req.write_start.tot_len; 
					SLT.eff_file.write.offset_start = file_req.write_start.offset;
					SLT.eff_file.write.checksum = 0xffff;
					
				}
				
				if (file_req.source == F_TCP_SOURCE)
				{
					uint32_t max_segment_size = TCP_MSS; 
					
					tcp_buf_t* p_buf = fd_tmp >= 0 ? tcp_make_ret(TCP_ACK, &max_segment_size, sizeof(max_segment_size)) : 
													 tcp_make_ret(TCP_NACK, NULL, 0);
					
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL)
								free(p_buf->data); 
							if (p_buf != NULL)
								free(p_buf); 
						}
				}
				else if (file_req.source == F_NOW_SOURCE)
				{
					 
					
				}
				
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
					/** write all file */
					if (SLT.eff_file.write.offset_start == 0 && SLT.eff_file.write.tot_len > 2)
					{
						
						if (fd >= 0) 
						{
							close(fd); 
							fd = -100;
						}
						
						struct stat st;
						int ret = stat(PATH_EFFECT, &st);
						if (ret >= 0)
							unlink(PATH_EFFECT);
						
						if (fd_tmp >= 0) 
						{
							close(fd_tmp); 
							fd_tmp = -100;
						}
						
						rename(PATH_EFFECT_TMP, PATH_EFFECT); 
						
						fd = open(PATH_EFFECT, O_RDWR | O_CREAT, 0666);
					}
					else
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

					if (file_req.source == F_TCP_SOURCE)
					{
						tcp_buf_t* p_buf = tcp_make_ret(TCP_ACK, NULL, 0);
					
						if (p_buf != NULL)
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL)
									free(p_buf->data); 
								if (p_buf != NULL)
									free(p_buf); 
							}
					}
					else if (file_req.source == F_NOW_SOURCE)
					{
						
					
					}
					
					xSemaphoreTake(xUpdatedEffect, 0);
					
					xSemaphoreGive(xUpdateEffect);
					
					xSemaphoreTake(xUpdatedEffect, portMAX_DELAY); 
				}
				else
				{
					if (file_req.source == F_TCP_SOURCE)
					{
						tcp_buf_t* p_buf = tcp_make_ret(TCP_NACK, NULL, 0);
					
						if (p_buf != NULL)
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL)
									free(p_buf->data); 
								if (p_buf != NULL)
									free(p_buf); 
							}
					}
					else if (file_req.source == F_NOW_SOURCE)
					{
						
						
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
/**
 *	@brief	task init infomation for the effect
 *	
 */
void task_init_effect() 
{
	effect_manage_t eff_tmp;
	int fd; 
	
	while (1)
	{
		if (xSemaphoreTake(xUpdateEffect, portMAX_DELAY) == pdPASS)
		{
			struct stat st;
			int ret = stat(PATH_EFFECT, &st);
			if (ret >= 0)
			{
				fd = open(PATH_EFFECT, O_RDWR | O_CREAT, 0666);
				if (fd >= 0)
				{
					uint32_t len_file = lseek(fd, 0, SEEK_END); 
					if (len_file >= 1 )
					{
						lseek(fd, 0, SEEK_SET);
						read(fd, &eff_tmp.brNess, 1);
					}
					if (len_file >= 2)
					{
						lseek(fd, 1, SEEK_SET);
						read(fd, &eff_tmp.speedEf, 1);
					}
					if (len_file >= 3)
					{
						lseek(fd, 2, SEEK_SET); 
						read(fd, &eff_tmp.numGroup, 1); 
					}
					
					if (len_file >= (3 + 4 * eff_tmp.numGroup))
					{
//						eff_tmp.offStartGr = malloc(sizeof(uint32_t) * eff_tmp.numGroup);
//						lseek(fd, 3, SEEK_SET); 
//						read(fd, eff_tmp.offStartGr, eff_tmp.numGroup * sizeof(uint32_t));
					}
					
				}
			}
			xSemaphoreGive(xUpdatedEffect);
		}
	}
}

