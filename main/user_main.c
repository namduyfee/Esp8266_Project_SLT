#include "my_lib.h"
#include "string.h"

#define MIN_DELAY 10 /*!< if delay task is short chip will be reset */
#define MIN_SIZE_OP_FILE 2048 /*!< task operate with spiffs file, it will need large size stack */ 
void task_esp_now_send(); 
void task_esp_now_recv();
void task_file_effect(); 
void task_select_master(); 
void task_send_tcp();
void task_update_effect_node(); 

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
		.my_id = 0,
		.mana_recv_wrf_mess = {
				
			.p_packet = NULL,
			.tot_packet = 0,
			.offset_st = 0,
			.tot_byte = 0		

		},
	},
	.server = {
		.tpcb = NULL,
		.count_client = 0,
		.p_client = NULL,
	}
	
};

SemaphoreHandle_t xNowPeersMana; 
SemaphoreHandle_t xTcpSwitchBufSend;
SemaphoreHandle_t xUpdateEffNode;
SemaphoreHandle_t xNowSendDone; 
SemaphoreHandle_t xNowReturnState;
	
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
	
	xUpdateEffNode = xSemaphoreCreateBinary(); 
	
	xTcpSwitchBufSend = xSemaphoreCreateBinary();
	
	xNowSendDone = xSemaphoreCreateBinary(); 
	
	xNowReturnState = xSemaphoreCreateBinary();
	
	my_init_project();
	
	xTaskCreate(task_esp_now_send, "task_esp_now_send", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_file_effect, "task_tcp_file_bin", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_select_master, "task_select_master", MIN_SIZE_OP_FILE, NULL, 4, NULL);
		 
	xTaskCreate(task_esp_now_recv, "task_esp_now_recv", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_send_tcp, "task_send_tcp", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_update_effect_node, "task_update_effect_node", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
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
								vTaskDelay(pdMS_TO_TICKS(5));
							
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
					if (send_q.buf.data != NULL && send_q.buf.tot_byte > 0)
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
 * @note
 *	- 2byte crc in buffer is deleted in recv callback
 *
 */

void task_esp_now_recv()
{
	TickType_t last_tick_br = xTaskGetTickCount(); 
	bool first_br = true;
	
	TickType_t last_write_st = xTaskGetTickCount();
	bool first_write_st = true;
	
	TickType_t last_write_end = xTaskGetTickCount();
	bool first_write_end = true;
	
	espnow_recv_queue_t espnow_recv;
	
	while (1)
	{
		if (xQueueReceive(xNowRecv, &espnow_recv, portMAX_DELAY) == pdPASS)
		{

			if (espnow_recv.buf.data != NULL && espnow_recv.buf.tot_byte > 0)
			{
				if (espnow_recv.buf.data[0] == 'N' && espnow_recv.buf.data[1] == 'O' && espnow_recv.buf.data[2] == 'W')
				{
					
					/** 
					 *	if recv broadcast
					 *		- erase and restore file that has info gateway and peers
					 *		- switch from apsta mode to sta mode
					 *		- send request add peer to master
					 */

					if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_BRC && 
					    (first_br == true || (xTaskGetTickCount() - last_tick_br > pdMS_TO_TICKS(TIME_BRC + 2000))))
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
								tm_gw_peer.id = espnow_recv.buf.data[NOW_INDEX_PAYLOAD]; 
								
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
										vTaskDelay(pdMS_TO_TICKS(5));
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
							
							espnow_send_queue_t add_peer_q; 
							add_peer_q.dest_id = SLT.espnow.gw_peer.id;
							add_peer_q.tmout_ms = 1000;
							add_peer_q.buf = espnow_make_frame_send(&payload, sizeof(payload), NOW_ADD_PEER); 
							if (add_peer_q.buf.data != NULL && add_peer_q.buf.tot_byte > 0)
								xQueueSend(xNowSend, &add_peer_q, pdMS_TO_TICKS(3000));
						}
					}
					
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_ADD_PEER)
					{
						if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
						{
							espnow_add_peer(espnow_recv.addr, espnow_recv.buf.data[NOW_INDEX_PAYLOAD]);
				
							xSemaphoreGive(xNowPeersMana);
						}
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_OPF)
					{
						uint8_t ack_of_cmd = NOW_OPF;
						
						espnow_send_queue_t ack_q;
						ack_q.dest_id = SLT.espnow.gw_peer.id;
						ack_q.tmout_ms = 1000;
						ack_q .buf = espnow_make_frame_send(&ack_of_cmd, sizeof(ack_of_cmd), NOW_ACK); 
						if (ack_q.buf.data != NULL && ack_q.buf.tot_byte > 0)
							xQueueSend(xNowSend, &ack_q, pdMS_TO_TICKS(3000));
						
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_ST_WRF)
					{
						if (first_write_st == true || xTaskGetTickCount() - last_write_st > pdMS_TO_TICKS(2000))
						{
							/** erase data old */
							if (SLT.espnow.mana_recv_wrf_mess.p_packet != NULL)
							{
								for (int i = 0; i < SLT.espnow.mana_recv_wrf_mess.tot_packet; i++)
								{
									if (SLT.espnow.mana_recv_wrf_mess.p_packet[i].data != NULL) 
									{
										free(SLT.espnow.mana_recv_wrf_mess.p_packet[i].data);
										SLT.espnow.mana_recv_wrf_mess.p_packet[i].data = NULL;
									}
								}
								free(SLT.espnow.mana_recv_wrf_mess.p_packet);
								SLT.espnow.mana_recv_wrf_mess.p_packet = NULL;
							}
							
							/** init for new request */
							memcpy(&SLT.espnow.mana_recv_wrf_mess.tot_packet, &espnow_recv.buf.data[NOW_INDEX_PAYLOAD], 4);
							memcpy(&SLT.espnow.mana_recv_wrf_mess.offset_st, &espnow_recv.buf.data[NOW_INDEX_PAYLOAD + 4], 4); 
							memcpy(&SLT.espnow.mana_recv_wrf_mess.tot_byte, &espnow_recv.buf.data[NOW_INDEX_PAYLOAD + 8], 4);
							
							SLT.espnow.mana_recv_wrf_mess.p_packet = malloc(sizeof(espnow_wrf_packet_t) * 
							                                               SLT.espnow.mana_recv_wrf_mess.tot_packet);
							for (int i = 0; i < SLT.espnow.mana_recv_wrf_mess.tot_packet; i++)
							{
								SLT.espnow.mana_recv_wrf_mess.p_packet[i].data = NULL;
							}
							SLT.espnow.mana_recv_wrf_mess.checksum = 0xffff;
							
							/** send ack st write */
							uint8_t ack_of_cmd = NOW_ST_WRF;
							
							espnow_send_queue_t ack_q; 
							ack_q.dest_id = SLT.espnow.gw_peer.id;
							ack_q.tmout_ms = 1000;
							ack_q.buf = espnow_make_frame_send(&ack_of_cmd, sizeof(ack_of_cmd), NOW_ACK); 
							if (ack_q.buf.data != NULL && ack_q.buf.tot_byte > 0)
								xQueueSend(xNowSend, &ack_q, pdMS_TO_TICKS(3000));
							
							first_write_st = false;
							last_write_st = xTaskGetTickCount();
						}
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_WRF)
					{
						uint32_t packet_number; memcpy(&packet_number, &espnow_recv.buf.data[NOW_INDEX_PAYLOAD], NOW_SZOF_PACKET_NUM);
						uint32_t offset; memcpy(&offset, &espnow_recv.buf.data[NOW_INDEX_PAYLOAD + 4], NOW_SZOF_OFFSET); 
						
						if (packet_number < SLT.espnow.mana_recv_wrf_mess.tot_packet && 
						    SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].data == NULL)
						{
							SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].offset = offset;
							SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].tot_byte = espnow_recv.buf.tot_byte 
																							 - NOW_SZOF_HEADER - NOW_SZOF_CMD 
																							 - NOW_SZOF_PACKET_NUM - NOW_SZOF_OFFSET; 
							
							SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].data = 
								malloc(SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].tot_byte);
							
							memcpy(SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].data,
								&espnow_recv.buf.data[NOW_INDEX_PAYLOAD + 8],
								SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].tot_byte);
							
							SLT.espnow.mana_recv_wrf_mess.checksum = crc16_modbus(SLT.espnow.mana_recv_wrf_mess.checksum,
								SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].data,
								SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].tot_byte); 							
						}
						
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_END_WRF)
					{
						if ( (first_write_end == true || xTaskGetTickCount() - last_write_end > pdMS_TO_TICKS(2000)) && 
						    SLT.espnow.mana_recv_wrf_mess.p_packet != NULL)
						{
							bool recv_all = true;
							uint32_t* packet_resend = NULL;
							uint32_t tot_packet_resend = 0;
							for (int i = 0; i <  SLT.espnow.mana_recv_wrf_mess.tot_packet; i++)
							{
								if (SLT.espnow.mana_recv_wrf_mess.p_packet[i].data == NULL)
								{
									if (packet_resend == NULL) 
									{
										tot_packet_resend = 1;
										packet_resend = malloc(NOW_SZOF_PACKET_NUM * tot_packet_resend); 
									}
									else
									{
										tot_packet_resend++;
										packet_resend = realloc(packet_resend, NOW_SZOF_PACKET_NUM * tot_packet_resend);
									}
									recv_all = false;
								}
							}
							if (recv_all == true)
							{
								uint16_t checksum; memcpy(&checksum, &espnow_recv.buf.data[NOW_INDEX_PAYLOAD], sizeof(checksum)); 
								if (checksum == SLT.espnow.mana_recv_wrf_mess.checksum)
								{
									
									first_write_end = false;
									last_write_end = xTaskGetTickCount();
									
									/** send ack write end */
									uint8_t ack_of_cmd = NOW_END_WRF;
									
									espnow_send_queue_t ack_q; 
									ack_q.dest_id = SLT.espnow.gw_peer.id;
									ack_q.tmout_ms = 1000;
									ack_q.buf = espnow_make_frame_send(&ack_of_cmd, sizeof(ack_of_cmd), NOW_ACK); 
									if (ack_q.buf.data != NULL && ack_q.buf.tot_byte > 0)
										xQueueSend(xNowSend, &ack_q, pdMS_TO_TICKS(3000));
									
									set_duty_pwm(&SLT.Pwm, 1, 0);
									set_duty_pwm(&SLT.Pwm, 3, 0);
									
								}
								else
								{
									/** send nack end write + resend all packet */
									uint8_t nack_of_cmd = NOW_END_WRF;
									uint32_t resend_all_packet = 0xffffffff;
									
									uint32_t tot_byte_pl = sizeof(nack_of_cmd) + sizeof(resend_all_packet);
									uint8_t payload[tot_byte_pl]; 
									
									memcpy(&payload, &nack_of_cmd, sizeof(nack_of_cmd));
									memcpy(&payload[sizeof(nack_of_cmd)], &resend_all_packet, sizeof(resend_all_packet));
									
									espnow_send_queue_t nack_q;
									nack_q.dest_id = SLT.espnow.gw_peer.id;
									nack_q.tmout_ms = 1000;
									nack_q.buf = espnow_make_frame_send(payload, tot_byte_pl, NOW_NACK);
									if (nack_q.buf.data != NULL && nack_q.buf.tot_byte > 0)
										xQueueSend(xNowSend, &nack_q, pdMS_TO_TICKS(3000));
									
								}
							}
							else
							{
								
								/** send nack + packets resend */
								if (packet_resend != NULL)
								{
									uint8_t nack_of_cmd = NOW_END_WRF;
									
									uint32_t tot_byte_pl = sizeof(nack_of_cmd) + tot_packet_resend * NOW_SZOF_PACKET_NUM; 
									uint8_t payload[tot_byte_pl];
									
									memcpy(&payload, &nack_of_cmd, sizeof(nack_of_cmd));
									memcpy(&payload[sizeof(nack_of_cmd)], packet_resend, tot_packet_resend * NOW_SZOF_PACKET_NUM);
								
									espnow_send_queue_t nack_q;
									nack_q.dest_id = SLT.espnow.gw_peer.id;
									nack_q.tmout_ms = 1000;
									nack_q.buf = espnow_make_frame_send(payload, tot_byte_pl, NOW_NACK);
									if (nack_q.buf.data != NULL && nack_q.buf.tot_byte > 0)
										xQueueSend(xNowSend, &nack_q, pdMS_TO_TICKS(3000));
									
									free(packet_resend);
									packet_resend = NULL;
									
								}
							}
						}
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_ACK)
					{
						SLT.espnow.state_return = NOW_ACK;
						xSemaphoreGive(xNowReturnState); 
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_NACK)
					{
						if (espnow_recv.buf.data[NOW_INDEX_PAYLOAD] == NOW_END_WRF)
						{
							
							SLT.espnow.wrf_resend.tot_req_pack = (espnow_recv.buf.tot_byte - NOW_SZOF_HEADER
								- NOW_SZOF_CMD - NOW_SZOF_CMD) / NOW_SZOF_PACKET_NUM;		/**< a NOW_SIZE_CMD for NOW_NACK, a NOW_SIZE_CMD for NOW_END_WRF */
								
							SLT.espnow.wrf_resend.request = malloc(SLT.espnow.wrf_resend.tot_req_pack * NOW_SZOF_PACKET_NUM);
							
							
							memcpy(SLT.espnow.wrf_resend.request,
								&espnow_recv.buf.data[NOW_INDEX_PAYLOAD + NOW_SZOF_CMD], 
								SLT.espnow.wrf_resend.tot_req_pack * NOW_SZOF_PACKET_NUM);
						}
						SLT.espnow.state_return = NOW_NACK;
						xSemaphoreGive(xNowReturnState);
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
	TickType_t lasttick = xTaskGetTickCount();
	
	espnow_send_queue_t queue_current = {.buf.data = NULL, .buf.tot_byte = 0};
	
	while (1)
	{	

		if (xSemaphoreTake(xNowPeersMana, 0) == pdPASS)
		{

			if (xQueueReceive(xNowSend, &queue_current, 0) == pdPASS)
			{
				
				if (queue_current.dest_id == NOW_ID_BRC)
				{
					lasttick = xTaskGetTickCount(); 
					while (xTaskGetTickCount() - lasttick < pdMS_TO_TICKS(TIME_BRC))
					{
						esp_now_send(address_brc, queue_current.buf.data, queue_current.buf.tot_byte);
						vTaskDelay(pdMS_TO_TICKS(100));
					}
				}
				else
				{
					
					bool id_exist = false;
					peer_info_t des_peer; 
					
					for (int i = 0; i < SLT.espnow.cnt_id_added; i++)
					{
						if (queue_current.dest_id == SLT.espnow.peer_list[i].id)
						{
							id_exist = true; 
							des_peer = SLT.espnow.peer_list[i];
							break; 
						}
					}
					
					if (id_exist == true)
					{
						
						lasttick = xTaskGetTickCount();

						while (xTaskGetTickCount() - lasttick < pdMS_TO_TICKS(queue_current.tmout_ms))
						{
							bool can_break = false;

							xSemaphoreTake(xNowSendDone, 0);

							esp_err_t ret = esp_now_send(des_peer.mac,
								queue_current.buf.data,
								queue_current.buf.tot_byte);

							if (ret == ESP_OK)
							{
								if (xSemaphoreTake(xNowSendDone, portMAX_DELAY) == pdPASS)
								{
									if (SLT.espnow.send_success)
										can_break = true;
								}
							}
							vTaskDelay(pdMS_TO_TICKS(10));
							if (can_break)
								break;
						}
					}
				}
				
				if (queue_current.buf.data != NULL) 
				{
					free(queue_current.buf.data);
					queue_current.buf.data = NULL; 
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
					
					xSemaphoreGive(xUpdateEffNode);
					
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
			vTaskDelay(pdMS_TO_TICKS(10));
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
 *	@brief	send data effect to all node after receive tcp
 *	@detail
 *		each node : 'NOW' + NOW_ST_WRF + (4B)tot number of packet + (4B)offset_start + (4B)tot size + (2B)checksum (this packet)
 *					'NOW' + NOW_WRF + (4B)number packet + (4B)offset + data + checksum (this packet)
 *					....
 *					'NOW' + NOW_END_WRF + (2B)checksum (of tot data) + checksum (this packet)	
 */			
void task_update_effect_node() 
{
	int fd; 
	
	while (1)
	{
		if (xSemaphoreTake(xUpdateEffNode, portMAX_DELAY) == pdPASS)
		{
			struct stat st;
			int ret = stat(PATH_EFFECT, &st);
			if (ret >= 0)
			{	
				fd = open(PATH_EFFECT, O_RDWR | O_CREAT, 0666);
				if (fd >= 0)
				{
//					// read number node
//					lseek(fd, 0, SEEK_SET); 
//					read(fd, &SLT.effMana.number_node, sizeof(SLT.effMana.number_node));
//					// read array offset start node
//					read(fd, SLT.effMana.offset_start_node, sizeof(uint32_t) * SLT.effMana.number_node);
//					// read array size node
//					read(fd, SLT.effMana.tot_size_node, sizeof(uint32_t) * SLT.effMana.number_node); 
					
					SLT.effMana.number_node = 2;
					SLT.effMana.offset_start_node[0] = 0; SLT.effMana.offset_start_node[1] = 0;
					SLT.effMana.tot_size_node[0] = 1500; SLT.effMana.tot_size_node[1] = 1500;
					
					for (int i = 0; i < SLT.effMana.number_node; i++)	// datatype i = int, because i-- is can called in code
					{
						
						if (i == SLT.espnow.my_id)
							continue;
						
						uint32_t des_id = i;
						
						/** send open file request to des_id */
						xSemaphoreTake(xNowReturnState, 0);
						
						espnow_send_queue_t open_q;
						open_q.dest_id = des_id;
						open_q.tmout_ms = 1000;
						open_q.buf = espnow_make_frame_send(NULL, 0, NOW_OPF);
						if (open_q.buf.data != NULL && open_q.buf.tot_byte > 0)
							xQueueSend(xNowSend, &open_q, portMAX_DELAY);
					
						if (xSemaphoreTake(xNowReturnState, pdMS_TO_TICKS(3000)) == pdPASS)
						{
							if (SLT.espnow.state_return == NOW_ACK)
							{
								/** send start write file to des_id */
								uint32_t max_byte_data = ESP_NOW_MAX_DATA_LEN - NOW_SZOF_HEADER - NOW_SZOF_CMD - NOW_SZOF_CRC
														- NOW_SZOF_OFFSET - NOW_SZOF_PACKET_NUM;
								
								uint32_t tot_packet =	SLT.effMana.tot_size_node[des_id] % max_byte_data == 0 ?
														(SLT.effMana.tot_size_node[des_id] / max_byte_data) :
														(SLT.effMana.tot_size_node[des_id] / max_byte_data) + 1;
								
								uint8_t payload_st_write[sizeof(tot_packet) + sizeof(SLT.effMana.offset_start_node[des_id])
									+ sizeof(SLT.effMana.tot_size_node[des_id])];
								
								memcpy(payload_st_write, &tot_packet, sizeof(tot_packet));
								
								memcpy(&payload_st_write[4], &SLT.effMana.offset_start_node[des_id], 
									sizeof(SLT.effMana.offset_start_node[des_id]));
								
								memcpy(&payload_st_write[8],&SLT.effMana.tot_size_node[des_id],
									sizeof(SLT.effMana.tot_size_node[des_id]));
								
								xSemaphoreTake(xNowReturnState, 0);
								
								espnow_send_queue_t write_st_q;
								write_st_q.dest_id = des_id;
								write_st_q.tmout_ms = 1000;
								write_st_q.buf = espnow_make_frame_send(payload_st_write, 8, NOW_ST_WRF);
								if (write_st_q.buf.data != NULL && write_st_q.buf.tot_byte > 0)
									xQueueSend(xNowSend, &write_st_q, portMAX_DELAY);
							
								if (xSemaphoreTake(xNowReturnState, pdMS_TO_TICKS(3000)) == pdPASS)
								{
									if (SLT.espnow.state_return == NOW_ACK)
									{
										/** send write packets to des_id */
										uint32_t packet_number = 0;
										uint32_t offset = 0; 
									
										uint16_t checksum = 0xffff;
									
										lseek(fd, 0, SEEK_SET);
									
										uint32_t offset_packet[tot_packet]; 
										uint32_t sizedata_packet[tot_packet];
									
										while (SLT.effMana.tot_size_node[des_id] > 0)
										{
											uint32_t tot_byte_read = max_byte_data < SLT.effMana.tot_size_node[des_id] ? max_byte_data : 
												SLT.effMana.tot_size_node[des_id];
						
											uint8_t* buf = malloc(tot_byte_read + NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET);
										
											offset = lseek(fd, 0, SEEK_CUR);
										
											memcpy(buf, &packet_number, NOW_SZOF_PACKET_NUM);
											memcpy(&buf[4], &offset, NOW_SZOF_OFFSET);
											read(fd, &buf[8], tot_byte_read);
										
											offset_packet[packet_number] = offset;
											sizedata_packet[packet_number] = tot_byte_read;
										
											checksum = crc16_modbus(checksum, &buf[8], tot_byte_read);
											
											espnow_send_queue_t write_q;
											write_q.dest_id = des_id;
											write_q.tmout_ms = 1000;
											write_q.buf = espnow_make_frame_send(buf, NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET + tot_byte_read, NOW_WRF); 
											if (write_q.buf.data != NULL && write_q.buf.tot_byte > 0)
												xQueueSend(xNowSend, &write_q, portMAX_DELAY);
										
											if (buf != NULL)
												free(buf);
										
											SLT.effMana.tot_size_node[des_id] -= tot_byte_read;
											packet_number++;
										}
										uint32_t lastick = xTaskGetTickCount();
										while (xTaskGetTickCount() - lastick < pdMS_TO_TICKS(4000))
										{
											bool can_break = false;
											
											/** send write end to des_id */
											xSemaphoreTake(xNowReturnState, 0);
											espnow_send_queue_t end_write_q; 
											end_write_q.dest_id = des_id;
											end_write_q.tmout_ms = 1000;
											end_write_q.buf = espnow_make_frame_send(&checksum, sizeof(checksum), NOW_END_WRF); 
											if (end_write_q.buf.data != NULL && end_write_q.buf.tot_byte > 0)
												xQueueSend(xNowSend, &end_write_q, portMAX_DELAY);
										
											if (xSemaphoreTake(xNowReturnState, pdMS_TO_TICKS(3000)) == pdPASS)
											{
												/** resend write packet */
												if (SLT.espnow.state_return == NOW_NACK)
												{
													if (SLT.espnow.wrf_resend.request != NULL && SLT.espnow.wrf_resend.request[0] == 0xffffffff) 
													{ 
														i--;
														can_break = true;
													}
													else if (SLT.espnow.wrf_resend.request != NULL && SLT.espnow.wrf_resend.request[0] != 0xffffffff)
													{
														for (int i = 0; i < SLT.espnow.wrf_resend.tot_req_pack; i++)
														{
															uint32_t packet_number = SLT.espnow.wrf_resend.request[i];
														
															lseek(fd, offset_packet[packet_number], SEEK_SET);
															uint8_t* buf = malloc(sizedata_packet[packet_number] + NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET);
															
															memcpy(buf, &packet_number, NOW_SZOF_PACKET_NUM);
															memcpy(&buf[4], &offset_packet[packet_number], NOW_SZOF_OFFSET);
															read(fd, &buf[8], sizedata_packet[packet_number]);
														
															espnow_send_queue_t write_q;
															write_q.dest_id = des_id;
															write_q.tmout_ms = 1000;
															write_q.buf = espnow_make_frame_send(buf,
																NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET + sizedata_packet[packet_number],
																NOW_WRF);
															if (write_q.buf.data != NULL && write_q.buf.tot_byte > 0)
																xQueueSend(xNowSend, &write_q, portMAX_DELAY);
															if (buf != NULL)
																free(buf);
														
														}
													}
												}
												else if(SLT.espnow.state_return == NOW_ACK)
												{
													can_break = true;
												}
											}
											else
											{
												i--;
												can_break = true;
											}
											
											if (can_break == true)
												break;
										}
									}
								
								}
							}
						}
					}
					close(fd); 
				}
			}
			set_duty_pwm(&SLT.Pwm, 3, 255);
			set_duty_pwm(&SLT.Pwm, 1, 255);
		}
	}
}

