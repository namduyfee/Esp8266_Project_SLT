#include "my_lib.h"
#include "string.h"

#define MIN_DELAY 10 /*!< if delay task is short chip will be reset */
#define MIN_SIZE_OP_FILE 2048 /*!< task operate with spiffs file, it will need large size stack */ 
void task_esp_now_send(); 
void task_esp_now_recv();

void task_file_tcp(); 

void task_file_effect(); 
void task_select_master(); 
void task_send_tcp();
void task_update_effect_node(); 
void task_make_effect();
void task_effect_synchr_asynchr();

Object SLT = {
	.Pwm = {
		.gpio_channel = {
		GPIO_PWM_CHAN_0,
		GPIO_PWM_CHAN_1,
		GPIO_PWM_CHAN_2,
		GPIO_PWM_CHAN_3,
		GPIO_PWM_CHAN_4,	
		GPIO_PWM_CHAN_5,
		GPIO_PWM_CHAN_6,
		GPIO_PWM_CHAN_7
	},
		.duty = {0, 0, 0, 0, 0, 0, 0, 0},
		.num_channel_en = 0
	},
	
	.espnow = {
		.gw_peer.id = 0xFE,
		.gw_peer.mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		.cnt_id_added = 0,
		.my_id = 2,
		.mana_recv_wrf_mess = {
				
		.p_packet = NULL,
		.tot_packet = 0,
		.offset_st = 0,
		.tot_byte = 0
	},
		.brc_new = false
	},
	.server = {
		.tpcb = NULL,
		.count_client = 0,
		.p_client = NULL,
	},
	.effMana = {
		.update_master_mana_gr = false,
		.master_mode = EFF_ASYNCHRONOUS,
	},
};

SemaphoreHandle_t xNowPeersMana; 
SemaphoreHandle_t xTcpSwitchBufSend;
SemaphoreHandle_t xUpdateEffNode;
SemaphoreHandle_t xNowSendDone; 
SemaphoreHandle_t xNowReturnState;
SemaphoreHandle_t xMasterManaEffGr;
SemaphoreHandle_t xMasterModeEff;
SemaphoreHandle_t xLoadEffect;



QueueHandle_t xTcpLoadf;
QueueHandle_t xEffLoadf;					/**< get data from tcp recv callback */
QueueHandle_t xNowRecv;						/**< get data from espnow recv callback */
QueueHandle_t xNowSend;						 
QueueHandle_t xSendTcp;
QueueHandle_t xNowResendWrf;
QueueHandle_t xEffRequest;

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
	
	/** Queue creat */
	xTcpLoadf = xQueueCreate(30, sizeof(file_request_t));
	
	xEffLoadf = xQueueCreate(30, sizeof(file_request_t));
	
	xNowRecv = xQueueCreate(20, sizeof(espnow_recv_queue_t));
	
	xNowSend = xQueueCreate(20, sizeof(espnow_send_queue_t)); 
	
	xSendTcp = xQueueCreate(20, sizeof(tcp_buf_t*)); 
	
	xNowResendWrf = xQueueCreate(10, sizeof(espnow_wrf_resend)); 
	
	xEffRequest = xQueueCreate(10, sizeof(effect_request_t)); 
	
	/** Semaphore creat */
	xNowPeersMana = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xNowPeersMana);
	
	xUpdateEffNode = xSemaphoreCreateBinary(); 
	
	xTcpSwitchBufSend = xSemaphoreCreateBinary();
	
	xNowSendDone = xSemaphoreCreateBinary(); 
	
	xNowReturnState = xSemaphoreCreateBinary();
	
	xLoadEffect = xSemaphoreCreateBinary();
	xSemaphoreGive(xLoadEffect);
	
	xMasterManaEffGr = xSemaphoreCreateBinary();
	xSemaphoreGive(xMasterManaEffGr);
	
	xMasterModeEff = xSemaphoreCreateBinary();
	xSemaphoreTake(xMasterModeEff, 0); 
	
	/** Task creat */
	my_init_project();
	
	xTaskCreate(task_esp_now_send, "task_esp_now_send", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_file_effect, "task_file_effect", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_file_tcp, "task_file_tcp", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_select_master, "task_select_master", MIN_SIZE_OP_FILE, NULL, 4, NULL);
		 
	xTaskCreate(task_esp_now_recv, "task_esp_now_recv", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_send_tcp, "task_send_tcp", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_update_effect_node, "task_update_effect_node", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_make_effect, "task_make_effect", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_effect_synchr_asynchr, "task_effect_synchr_asynchr", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
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
	
	uint32_t last_time_brc = xTaskGetTickCount();
	uint32_t last_time_new_brc = xTaskGetTickCount();
	
	bool first_brc = true; 
	
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
								
								tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
								tcpip_adapter_ip_info_t ip_info;
								IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
								IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
								IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
								tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
								tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
								
								esp_wifi_start();
								
							}
							nvs_commit(handle);
							
							init_espnow();
							
							last_time_new_brc = xTaskGetTickCount();
							first_brc = true;
							
							SLT.espnow.brc_new = true;
							
						}
						
						nvs_close(handle); 
					}
					xSemaphoreGive(xNowPeersMana);
				}
				
			}
		}		
		// send broadcast 
		if (is_same_macadrr(SLT.espnow.gw_peer.mac, SLT.wifi.ap_macaddr) == true
		    && SLT.espnow.gw_peer.id == SLT.espnow.my_id)
		{
			uint32_t time_loop = SLT.espnow.brc_new == true ? 500 : 1000;
			
			if (first_brc == true || (xTaskGetTickCount() - last_time_brc > pdMS_TO_TICKS(time_loop)))
			{

				
				if (SLT.espnow.brc_new == true)
					if (xTaskGetTickCount() - last_time_new_brc > pdMS_TO_TICKS(5000))
						SLT.espnow.brc_new = false;
				
				/** send request broadcast */
				uint8_t payload = {SLT.espnow.my_id}; 
				espnow_send_queue_t send_q; 
				send_q.dest_id = NOW_ID_BRC;
				send_q.tmout_ms = 500;
				send_q .buf = espnow_make_frame_send(&payload, sizeof(payload), NOW_BRC); 
				if (send_q.buf.data != NULL && send_q.buf.tot_byte > 0)
					xQueueSend(xNowSend, &send_q, portMAX_DELAY);
				
				first_brc = false;
				last_time_brc = xTaskGetTickCount();
				
				
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
	uint32_t tem = 0;
	

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
					if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_BRC)
					{
						
						if (SLT.espnow.brc_new != true)
						{
							if (is_same_macadrr(SLT.espnow.gw_peer.mac, espnow_recv.addr) != true
									|| SLT.espnow.gw_peer.id != espnow_recv.buf.data[NOW_INDEX_CMD + 1])
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
										tm_gw_peer.id = espnow_recv.buf.data[NOW_INDEX_CMD + 1]; 
								
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
									}
									xSemaphoreGive(xNowPeersMana);
								}
						
								if (send_add_req_to_gw == true)
								{
									
									/** send request add peer to gateway */
									uint8_t payload = SLT.espnow.my_id; 
									
									espnow_send_queue_t add_peer_q; 
									add_peer_q.dest_id = SLT.espnow.gw_peer.id;
									add_peer_q.tmout_ms = 4000;
									add_peer_q.buf = espnow_make_frame_send(&payload, sizeof(payload), NOW_ADD_PEER); 
									if (add_peer_q.buf.data != NULL && add_peer_q.buf.tot_byte > 0)
										xQueueSend(xNowSend, &add_peer_q, pdMS_TO_TICKS(5000));
								}
							}
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
						
						file_request_t eff = {
							.cmd = F_OP,
							.source = F_NOW_SOURCE
						};
						
						if (xQueueSendToBack(xEffLoadf, &eff, pdMS_TO_TICKS(4000)) != pdPASS)
						{
							uint8_t nack_of_cmd = NOW_OPF;
						
							espnow_send_queue_t nack_q;
							nack_q.dest_id = SLT.espnow.gw_peer.id;
							nack_q.tmout_ms = 1000;
							nack_q .buf = espnow_make_frame_send(&nack_of_cmd, sizeof(nack_of_cmd), NOW_NACK); 
							if (nack_q.buf.data != NULL && nack_q.buf.tot_byte > 0)
								xQueueSend(xNowSend, &nack_q, pdMS_TO_TICKS(3000));
						}
					
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_CLSF)
					{
						
						file_request_t eff = {
							.cmd = F_CLS,
							.source = F_NOW_SOURCE
						};
						
						if (xQueueSendToBack(xEffLoadf, &eff, pdMS_TO_TICKS(4000)) != pdPASS)
						{
							uint8_t nack_of_cmd = NOW_CLSF;
						
							espnow_send_queue_t nack_q;
							nack_q.dest_id = SLT.espnow.gw_peer.id;
							nack_q.tmout_ms = 1000;
							nack_q .buf = espnow_make_frame_send(&nack_of_cmd, sizeof(nack_of_cmd), NOW_NACK); 
							if (nack_q.buf.data != NULL && nack_q.buf.tot_byte > 0)
								xQueueSend(xNowSend, &nack_q, pdMS_TO_TICKS(3000));
						}
						
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_DLTF)
					{
						
						file_request_t eff = {
							.cmd = F_DLT,
							.source = F_NOW_SOURCE
						};
						
						if (xQueueSendToBack(xEffLoadf, &eff, pdMS_TO_TICKS(4000)) != pdPASS)
						{
							uint8_t nack_of_cmd = NOW_DLTF;
						
							espnow_send_queue_t nack_q;
							nack_q.dest_id = SLT.espnow.gw_peer.id;
							nack_q.tmout_ms = 1000;
							nack_q .buf = espnow_make_frame_send(&nack_of_cmd, sizeof(nack_of_cmd), NOW_NACK); 
							if (nack_q.buf.data != NULL && nack_q.buf.tot_byte > 0)
								xQueueSend(xNowSend, &nack_q, pdMS_TO_TICKS(3000));
						}
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
							
							if (SLT.espnow.mana_recv_wrf_mess.p_packet != NULL)
							{
								for (int i = 0; i < SLT.espnow.mana_recv_wrf_mess.tot_packet; i++)
								{
									SLT.espnow.mana_recv_wrf_mess.p_packet[i].data = NULL;
								}	
							}

							first_write_st = false;
							last_write_st = xTaskGetTickCount();
							
							/**< send st write to task handle file effect */
							file_request_t eff = {
								.cmd = F_ST_WR,
								.source = F_NOW_SOURCE
							};
							
							eff.write_start.offset = SLT.espnow.mana_recv_wrf_mess.offset_st;
							eff.write_start.tot_byte = SLT.espnow.mana_recv_wrf_mess.tot_byte;
							
							if (xQueueSendToBack(xEffLoadf, &eff, pdMS_TO_TICKS(4000)) != pdPASS)
							{
								/** send nack st write */
								uint8_t nack_of_cmd = NOW_ST_WRF;
							
								espnow_send_queue_t nack_q; 
								nack_q.dest_id = SLT.espnow.gw_peer.id;
								nack_q.tmout_ms = 1000;
								nack_q.buf = espnow_make_frame_send(&nack_of_cmd, sizeof(nack_of_cmd), NOW_NACK); 
								if (nack_q.buf.data != NULL && nack_q.buf.tot_byte > 0)
									xQueueSend(xNowSend, &nack_q, pdMS_TO_TICKS(3000));
							}
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
							if (SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].data != NULL)
							{
								tem += SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].tot_byte;
								if (tem > 100000)
								{
									//set_duty_pwm(&SLT.Pwm, 3, 255);
								}
								else if (tem > 8000)
								{
									//set_duty_pwm(&SLT.Pwm, 3, 0);
								}
								memcpy(SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].data,
									&espnow_recv.buf.data[NOW_INDEX_PAYLOAD + 8],
									SLT.espnow.mana_recv_wrf_mess.p_packet[packet_number].tot_byte);
							}
						}
						
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_END_WRF)
					{
						if ((first_write_end == true || xTaskGetTickCount() - last_write_end > pdMS_TO_TICKS(2000)) && 
						    SLT.espnow.mana_recv_wrf_mess.p_packet != NULL)
						{
							
							bool recv_all = true;
							uint32_t* packet_resend = NULL;
							uint32_t tot_packet_resend = 0;
							uint16_t checksum_calculate = 0xffff;
							
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
									
									packet_resend[tot_packet_resend - 1] = i;
									
									recv_all = false;
								}
								else
								{
									checksum_calculate = crc16_modbus(checksum_calculate,
										SLT.espnow.mana_recv_wrf_mess.p_packet[i].data,
										SLT.espnow.mana_recv_wrf_mess.p_packet[i].tot_byte);
								}
							}
							if (recv_all == true)
							{
								uint16_t checksum; memcpy(&checksum, &espnow_recv.buf.data[NOW_INDEX_PAYLOAD], sizeof(checksum)); 
								if (checksum == checksum_calculate)
								{
									
									first_write_end = false;
									last_write_end = xTaskGetTickCount();
									
									/** send write to task handle file effect */
									for (int i = 0; i <  SLT.espnow.mana_recv_wrf_mess.tot_packet; i++)
									{
										file_request_t eff = {
											.cmd = F_WR,
											.source = F_NOW_SOURCE
										};
										eff.write.offset = SLT.espnow.mana_recv_wrf_mess.p_packet[i].offset;
										eff.write.buf.tot_byte = SLT.espnow.mana_recv_wrf_mess.p_packet[i].tot_byte;
										
										/** do not assign directly SLT.espnow.mana_recv_wrf_mess.p_packet[i].data
										 *	because SLT.espnow.mana_recv_wrf_mess.p_packet[i].data will is free below
										 */
										eff.write.buf.data = malloc(eff.write.buf.tot_byte);
										if (eff.write.buf.data != NULL)	
										{
											memcpy(eff.write.buf.data,
												SLT.espnow.mana_recv_wrf_mess.p_packet[i].data,
												eff.write.buf.tot_byte);
											
											if (xQueueSendToBack(xEffLoadf, &eff, pdMS_TO_TICKS(4000)) != pdPASS)
											{
												if (eff.write.buf.data != NULL) 
												{
													free(eff.write.buf.data);
													eff.write.buf.data = NULL;
												}
											}
										}
									}
									/** send end write to task handle file effect */
									file_request_t eff = {
										.cmd = F_END_WR,
										.source = F_NOW_SOURCE
									};
									eff.write_end.checksum = checksum;
									
									if (xQueueSendToBack(xEffLoadf, &eff, pdMS_TO_TICKS(4000)) != pdPASS)
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
								
								/** free after recv all packet */
								for (int i = 0; i <  SLT.espnow.mana_recv_wrf_mess.tot_packet; i++)
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
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_EFF_ASYNC)
					{
						effect_request_t eff_req_tmp;
						eff_req_tmp.mode = EFF_ASYNCHRONOUS; 
						eff_req_tmp.number_of_group = espnow_recv.buf.data[NOW_INDEX_CMD + 1];
						
						memcpy(eff_req_tmp.gproup_request, &espnow_recv.buf.data[NOW_INDEX_CMD + 2], eff_req_tmp.number_of_group);
						
						memcpy(eff_req_tmp.state, &espnow_recv.buf.data[NOW_INDEX_CMD + 2 + eff_req_tmp.number_of_group], eff_req_tmp.number_of_group  * sizeof(uint16_t));
						
						xQueueSendToBack(xEffRequest, &eff_req_tmp, portMAX_DELAY);
						
					}
					else if (espnow_recv.buf.data[NOW_INDEX_CMD] == NOW_EFF_SYNC)
					{
						effect_request_t eff_req_tmp;
						eff_req_tmp.mode = EFF_SYNCHRONOUS; 
						eff_req_tmp.number_of_group = espnow_recv.buf.data[NOW_INDEX_CMD + 1];
						
						memcpy(eff_req_tmp.gproup_request, &espnow_recv.buf.data[NOW_INDEX_CMD + 2], eff_req_tmp.number_of_group);
						
						memcpy(eff_req_tmp.state, &espnow_recv.buf.data[NOW_INDEX_CMD + 2 + eff_req_tmp.number_of_group], eff_req_tmp.number_of_group * sizeof(uint16_t));
						
						xQueueSendToBack(xEffRequest, &eff_req_tmp, portMAX_DELAY);
						
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
							espnow_wrf_resend resend_q;
							
							resend_q.tot_req_pack = (espnow_recv.buf.tot_byte - NOW_SZOF_HEADER
								- NOW_SZOF_CMD - NOW_SZOF_CMD) / NOW_SZOF_PACKET_NUM;		/**< a NOW_SIZE_CMD for NOW_NACK, a NOW_SIZE_CMD for NOW_END_WRF */
								
							resend_q.request = malloc(resend_q.tot_req_pack * NOW_SZOF_PACKET_NUM);
							
							memcpy(resend_q.request,
								&espnow_recv.buf.data[NOW_INDEX_PAYLOAD + NOW_SZOF_CMD], 
								resend_q.tot_req_pack * NOW_SZOF_PACKET_NUM);
							
							if (xQueueSend(xNowResendWrf, &resend_q, pdMS_TO_TICKS(4000)) != pdPASS)
							{
								if (resend_q.request != NULL) 
								{
									free(resend_q.request);
									resend_q.request = NULL;
								}
							}
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
	espnow_send_queue_t queue_current = {.buf.data = NULL, .buf.tot_byte = 0};
	
	while (1)
	{	

		if (xQueueReceive(xNowSend, &queue_current, portMAX_DELAY) == pdPASS)
		{
			if (xSemaphoreTake(xNowPeersMana, portMAX_DELAY) == pdPASS)
			{
				bool id_exist = false; 
				bool brc = false;
				uint8_t dest_mac[6]; memset(dest_mac, 0, 6); 
				
				if (queue_current.dest_id == NOW_ID_BRC)
				{
					brc = true;
					memcpy(dest_mac, address_brc, 6);
				}
				else
				{
					for (int i = 0; i < SLT.espnow.cnt_id_added; i++)
					{
						if (queue_current.dest_id == SLT.espnow.peer_list[i].id)
						{
							id_exist = true;
							memcpy(dest_mac, SLT.espnow.peer_list[i].mac, 6);
							break; 
						}
					}
					
				}
				
				if (brc == true || id_exist == true)
				{
					TickType_t lasttick = xTaskGetTickCount();

					while (xTaskGetTickCount() - lasttick < pdMS_TO_TICKS(queue_current.tmout_ms))
					{
						bool can_break = false;

						xSemaphoreTake(xNowSendDone, 0);

						esp_err_t ret = esp_now_send(dest_mac,
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
							
						if (can_break) {
							break;
						}
							
						vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));

					}
				}
				
				if (queue_current.buf.data != NULL) 
				{
					free(queue_current.buf.data);
					queue_current.buf.data = NULL; 
				}
				
				xSemaphoreGive(xNowPeersMana);
			} 
			
		}
		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
	}
}

/**
 *	@brief	handle file tcp
 *
 */
void task_file_tcp()
{	
	file_mana_t tcp_file_mana;
	
	int fd = -100; 
	int fd_tmp = -100; 
	
	file_request_t file_req = {
		.cmd = F_NONE, 
	};  
	
	
	while (1)
	{
		if (xQueueReceive(xTcpLoadf, &file_req, portMAX_DELAY) == pdPASS)
		{	
			if (file_req.cmd == F_OP)
			{
				
				if (fd < 0)
				{
					struct stat st;
					int ret = stat(PATH_TCP_FILE, &st);
					fd = ret < 0 ?  open(PATH_TCP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666) : 
									open(PATH_TCP_FILE, O_RDWR | O_CREAT, 0666);
				}
				
				if (file_req.source == F_TCP_SOURCE)
				{
					tcp_buf_t* p_buf = fd >= 0 ? tcp_make_ret(TCP_ACK, NULL, 0) : tcp_make_ret(TCP_NACK, NULL, 0);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL) 
							{
								free(p_buf->data); 
								p_buf->data = NULL;
							}
							if (p_buf != NULL) 
							{
								free(p_buf); 
								p_buf = NULL;
							}
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
				
				if (file_req.source == F_TCP_SOURCE)
				{
					tcp_buf_t* p_buf = fd < 0 ? tcp_make_ret(TCP_ACK, NULL, 0) : tcp_make_ret(TCP_NACK, NULL, 0);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL) 
							{
								free(p_buf->data); 
								p_buf->data = NULL;
							}
							if (p_buf != NULL) 
							{
								free(p_buf); 
								p_buf = NULL;
							}
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
				unlink(PATH_TCP_FILE);
				
				struct stat st;
				int ret = stat(PATH_TCP_FILE, &st);
				
				if (file_req.source == F_TCP_SOURCE)
				{
					tcp_buf_t* p_buf = ret < 0 ? tcp_make_ret(TCP_ACK, NULL, 0) : tcp_make_ret(TCP_NACK, NULL, 0);
					if (p_buf != NULL)
						if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
						{
							if (p_buf != NULL && p_buf->data != NULL) 
							{
								free(p_buf->data); 
								p_buf->data = NULL;
							}
							if (p_buf != NULL) 
							{
								free(p_buf); 
								p_buf = NULL;
							}
						}
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
								{
									free(p_buf->data); 
									p_buf->data = NULL;
								}
								if (p_buf != NULL) 
								{
									free(p_buf); 
									p_buf = NULL;
								}
							}
					}
					else
					{
						if (file_req.read.offset >= lseek(fd, 0, SEEK_END) || 
						    file_req.read.tot_byte > (lseek(fd, 0, SEEK_END) - file_req.read.offset))
						{
							tcp_buf_t* p_buf = tcp_make_ret(TCP_NACK, NULL, 0);
							if (p_buf != NULL)
								if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
								{
									if (p_buf != NULL && p_buf->data != NULL) 
									{
										free(p_buf->data); 
										p_buf->data = NULL;
									}
									if (p_buf != NULL) 
									{
										free(p_buf); 
										p_buf = NULL;
									}
								}
						}
						else
						{
						
							tcp_buf_t* p_buf = tcp_make_ret(TCP_ACK, NULL, 0);
							if (p_buf != NULL)
								if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
								{
									if (p_buf != NULL && p_buf->data != NULL) 
									{
										free(p_buf->data); 
										p_buf->data = NULL; 
									}
									if (p_buf != NULL) 
									{
										free(p_buf); 
										p_buf = NULL;
									}
								}
						
							off_t current_off = lseek(fd, file_req.read.offset, SEEK_SET); 
							uint32_t remaining = file_req.read.tot_byte;
						
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
										{
											free(p_buf->data); 
											p_buf->data = NULL;
										}
										if (p_buf != NULL) 
										{
											free(p_buf);
											p_buf = NULL;
										}
									}
								}
							
								if (data != NULL) 
								{
									free(data); 
								}
								data = NULL; 
							}						
						}
					}
				}
			}
			else if (file_req.cmd == F_ST_WR)
			{
				if (fd < 0)
				{
					if (file_req.source == F_TCP_SOURCE)
					{
						tcp_buf_t* p_buf = tcp_make_ret(TCP_NACK, NULL, 0);
						if (p_buf != NULL)
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL) {
									free(p_buf->data); 
									p_buf->data = NULL;
								}
								if (p_buf != NULL) {
									free(p_buf); 
									p_buf = NULL;
								}
							}
					}
				}
				else
				{
					if (fd_tmp >= 0)
					{
						close(fd_tmp);
						fd_tmp = -100; 
					}
					fd_tmp = open(PATH_TCP_FILE_TMP, O_RDWR | O_CREAT | O_TRUNC, 0666);  
				
					if (fd_tmp >= 0)
					{
						tcp_file_mana.write.tot_byte = file_req.write_start.tot_byte;
						tcp_file_mana.write.remaining = file_req.write_start.tot_byte; 
						tcp_file_mana.write.offset_start = file_req.write_start.offset;
						tcp_file_mana.write.checksum = 0xffff;
					
					}
				
					if (file_req.source == F_TCP_SOURCE)
					{
						uint32_t max_segment_size = TCP_MSS; 
					
						tcp_buf_t* p_buf = fd_tmp >= 0 ? tcp_make_ret(TCP_ACK, &max_segment_size, sizeof(max_segment_size)) : 
														 tcp_make_ret(TCP_NACK, NULL, 0);
					
						if (p_buf != NULL)
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL) {
									free(p_buf->data); 
									p_buf->data = NULL;
								}
								if (p_buf != NULL) {
									free(p_buf); 
									p_buf = NULL;
								}
							}
					}
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
						
						if (tcp_file_mana.write.remaining > 0 && 
						    file_req.write.offset >= tcp_file_mana.write.offset_start)
						{
							if (fd_tmp >= 0)
							{

								lseek(fd_tmp, file_req.write.offset - tcp_file_mana.write.offset_start, SEEK_SET);
								
								ssize_t to_write = tcp_file_mana.write.remaining <= file_req.write.buf.tot_byte ?
													tcp_file_mana.write.remaining : file_req.write.buf.tot_byte;
								
								ssize_t written = 0; 
							
								if (to_write > 0)
								{
									written = write(fd_tmp,
										file_req.write.buf.data, 
										to_write);
								}
								
								if (written > 0) 
								{
									tcp_file_mana.write.checksum = crc16_modbus(tcp_file_mana.write.checksum, file_req.write.buf.data, written); 
									tcp_file_mana.write.remaining = tcp_file_mana.write.remaining - written; 
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
				if (tcp_file_mana.write.checksum == file_req.write_end.checksum && fd >= 0 && fd_tmp >= 0)
				{
					/** write all file */
					if (tcp_file_mana.write.offset_start == 0 && tcp_file_mana.write.tot_byte > 2)
					{
						
						if (fd >= 0) 
						{
							close(fd); 
							fd = -100;
						}
						
						struct stat st;
						int ret = stat(PATH_TCP_FILE, &st);
						if (ret >= 0)
							unlink(PATH_TCP_FILE);
						
						if (fd_tmp >= 0) 
						{
							close(fd_tmp); 
							fd_tmp = -100;
						}
						
						rename(PATH_TCP_FILE_TMP, PATH_TCP_FILE); 
						
						fd = open(PATH_TCP_FILE, O_RDWR, 0666);
					}
					else
					{
						int remaining = tcp_file_mana.write.tot_byte; 
									
						off_t posfd = tcp_file_mana.write.offset_start; 
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
						{
							free(bufA);
							bufA = NULL;
						}
						
						if (fd_tmp >= 0) 
						{
							close(fd_tmp); 
							fd_tmp = -100;
						}
				 				
						struct stat st;
						int ret = stat(PATH_TCP_FILE_TMP, &st);
						if (ret >= 0)
							unlink(PATH_TCP_FILE_TMP);
						
					}
					if (file_req.source == F_TCP_SOURCE)
					{
						/** send ack write end tcp */
						tcp_buf_t* p_buf = tcp_make_ret(TCP_ACK, NULL, 0);
					
						if (p_buf != NULL)
							if (xQueueSend(xSendTcp, &p_buf, pdMS_TO_TICKS(500)) != pdPASS)
							{
								if (p_buf != NULL && p_buf->data != NULL) {
									free(p_buf->data); 
									p_buf->data = NULL; 
								}
								if (p_buf != NULL) {
									free(p_buf); 
									p_buf = NULL;
								}
							}
					}
					// send effect to other node
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
								if (p_buf != NULL && p_buf->data != NULL) {
									free(p_buf->data); 
									p_buf->data = NULL;
								}
								if (p_buf != NULL) {
									free(p_buf); 
									p_buf = NULL;
								}
							}
					}
					if (fd_tmp >= 0) 
					{
						close(fd_tmp); 
						fd_tmp = -100;
					}
				 				
					struct stat st;
					int ret = stat(PATH_TCP_FILE_TMP, &st);
					if (ret >= 0)
						unlink(PATH_TCP_FILE_TMP);
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
	}
}

/**
 *	@brief	handle file effect
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
	file_mana_t effect_file_mana; 
	
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
				
				if (file_req.source == F_NOW_SOURCE)
				{
					uint8_t state = fd >= 0 ? NOW_ACK : NOW_NACK;
					
					uint8_t state_of_cmd = NOW_OPF;
					
					espnow_send_queue_t state_q;
					state_q.dest_id = SLT.espnow.gw_peer.id;
					state_q.tmout_ms = 1000;
					state_q .buf = espnow_make_frame_send(&state_of_cmd, sizeof(state_of_cmd), state);
					
					if (state_q.buf.data != NULL && state_q.buf.tot_byte > 0)
						xQueueSend(xNowSend, &state_q, pdMS_TO_TICKS(3000));
					
				}

			}
			else if (file_req.cmd == F_CLS)
			{
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}

				if (file_req.source == F_NOW_SOURCE)
				{
					uint8_t state = fd < 0 ? NOW_ACK : NOW_NACK;
					
					uint8_t state_of_cmd = NOW_CLSF;
						
					espnow_send_queue_t state_q;
					state_q.dest_id = SLT.espnow.gw_peer.id;
					state_q.tmout_ms = 1000;
					state_q .buf = espnow_make_frame_send(&state_of_cmd, sizeof(state_of_cmd), state);
					
					if (state_q.buf.data != NULL && state_q.buf.tot_byte > 0)
						xQueueSend(xNowSend, &state_q, pdMS_TO_TICKS(3000));
					
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
				
				if (file_req.source == F_NOW_SOURCE)
				{
					uint8_t state = ret < 0 ? NOW_ACK : NOW_NACK;
					
					uint8_t state_of_cmd = NOW_DLTF;
						
					espnow_send_queue_t state_q;
					state_q.dest_id = SLT.espnow.gw_peer.id;
					state_q.tmout_ms = 1000;
					state_q .buf = espnow_make_frame_send(&state_of_cmd, sizeof(state_of_cmd), state);
					
					if (state_q.buf.data != NULL && state_q.buf.tot_byte > 0)
						xQueueSend(xNowSend, &state_q, pdMS_TO_TICKS(3000));
					
				}
			}
			
			else if (file_req.cmd == F_RD)
			{

			}
			else if (file_req.cmd == F_ST_WR)
			{
				if (fd < 0)
				{

					if (file_req.source == F_NOW_SOURCE)
					{
						uint8_t nack_of_cmd = NOW_ST_WRF;
						
						espnow_send_queue_t state_q;
						state_q.dest_id = SLT.espnow.gw_peer.id;
						state_q.tmout_ms = 1000;
						state_q .buf = espnow_make_frame_send(&nack_of_cmd, sizeof(nack_of_cmd), NOW_NACK);
					
						if (state_q.buf.data != NULL && state_q.buf.tot_byte > 0)
							xQueueSend(xNowSend, &state_q, pdMS_TO_TICKS(3000));
					}
				}
				else
				{
					if (fd_tmp >= 0)
					{
						close(fd_tmp);
						fd_tmp = -100; 
					}
					fd_tmp = open(PATH_EFFECT_TMP, O_RDWR | O_CREAT | O_TRUNC, 0666);  
				
					if (fd_tmp >= 0)
					{
						effect_file_mana.write.tot_byte = file_req.write_start.tot_byte;
						effect_file_mana.write.remaining = file_req.write_start.tot_byte; 
						effect_file_mana.write.offset_start = file_req.write_start.offset;
						effect_file_mana.write.checksum = 0xffff;
					
					}
				
					if (file_req.source == F_NOW_SOURCE)
					{
						
						uint8_t state = fd_tmp >= 0 ? NOW_ACK : NOW_NACK;
					
						uint8_t state_of_cmd = NOW_ST_WRF;
						
						espnow_send_queue_t state_q;
						state_q.dest_id = SLT.espnow.gw_peer.id;
						state_q.tmout_ms = 1000;
						state_q .buf = espnow_make_frame_send(&state_of_cmd, sizeof(state_of_cmd), state);
					
						if (state_q.buf.data != NULL && state_q.buf.tot_byte > 0)
							xQueueSend(xNowSend, &state_q, pdMS_TO_TICKS(3000));
					}
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
						
						if (effect_file_mana.write.remaining > 0 && 
						    file_req.write.offset >= effect_file_mana.write.offset_start)
						{
							if (fd_tmp >= 0)
							{

								lseek(fd_tmp, file_req.write.offset - effect_file_mana.write.offset_start, SEEK_SET);
								
								ssize_t to_write = effect_file_mana.write.remaining <= file_req.write.buf.tot_byte ?
													effect_file_mana.write.remaining : file_req.write.buf.tot_byte;
								
								ssize_t written = 0; 
							
								if (to_write > 0)
								{
									written = write(fd_tmp,
										file_req.write.buf.data, 
										to_write);
								}
								
								if (written > 0) 
								{
									effect_file_mana.write.checksum = crc16_modbus(effect_file_mana.write.checksum, file_req.write.buf.data, written); 
									effect_file_mana.write.remaining = effect_file_mana.write.remaining - written; 
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
				if (effect_file_mana.write.checksum == file_req.write_end.checksum && fd >= 0 && fd_tmp >= 0)
				{
					/** write all file */
					if (effect_file_mana.write.offset_start == 0 && effect_file_mana.write.tot_byte > 2)
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
						
						fd = open(PATH_EFFECT, O_RDWR, 0666);
					}
					else
					{
						int remaining = effect_file_mana.write.tot_byte; 
									
						off_t posfd = effect_file_mana.write.offset_start; 
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
						{
							free(bufA);
							bufA = NULL;
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
					
					if (file_req.source == F_NOW_SOURCE)
					{
						
						/** send ack write end espnow */
						uint8_t ack_of_cmd = NOW_END_WRF;
									
						espnow_send_queue_t ack_q; 
						ack_q.dest_id = SLT.espnow.gw_peer.id;
						ack_q.tmout_ms = 1000;
						ack_q.buf = espnow_make_frame_send(&ack_of_cmd, sizeof(ack_of_cmd), NOW_ACK); 
						if (ack_q.buf.data != NULL && ack_q.buf.tot_byte > 0)
							xQueueSend(xNowSend, &ack_q, pdMS_TO_TICKS(3000));
					}
					
				    // load new effect to run
					xSemaphoreGive(xLoadEffect);
				}
				else
				{
					if (file_req.source == F_NOW_SOURCE)
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
			if (tcp_send_buf != NULL && tcp_send_buf->data != NULL) {
				free(tcp_send_buf->data);
				tcp_send_buf->data = NULL;
			}
			if (tcp_send_buf != NULL) {
				free(tcp_send_buf);
			}
			
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
	int fd = -1; 
	
	uint8_t number_node;	/**< number of node in eff*/
	uint32_t offset_start_node[MAX_PEER];	/**< array offset start in file of each node*/
	uint32_t tot_size_node[MAX_PEER];		/**< total byte in file of each node */
	

	while (1)
	{
		if (xSemaphoreTake(xUpdateEffNode, portMAX_DELAY) == pdPASS)
		{
			struct stat st;
			int ret = stat(PATH_TCP_FILE, &st);
			if (ret >= 0)
			{	
				fd = open(PATH_TCP_FILE, O_RDONLY, 0666);
				if (fd >= 0)
				{
					
					xSemaphoreTake(xMasterManaEffGr, portMAX_DELAY);
					
					// read number node
					lseek(fd, 0, SEEK_SET); 
					read(fd, &number_node, sizeof(number_node));
					// read array offset start node
					read(fd, offset_start_node, sizeof(uint32_t) * number_node);
					// read array size node
					read(fd, tot_size_node, sizeof(uint32_t) * number_node); 
					
					/** read infor for master mana effect */
					for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
					{
		
						SLT.effMana.master_mana_gr[i].num_state_of_gr = 0;
						if (SLT.effMana.master_mana_gr[i].timeExistOfSta != NULL)
							free(SLT.effMana.master_mana_gr[i].timeExistOfSta);
						SLT.effMana.master_mana_gr[i].timeExistOfSta = NULL;
					}
			
					lseek(fd, sizeof(number_node) + sizeof(uint32_t) * number_node + sizeof(uint32_t) * number_node, SEEK_SET); 
			
					uint8_t num_of_gr; 
					read(fd, &num_of_gr, sizeof(num_of_gr)); 
					for (int i = 0; i < num_of_gr; i++)
					{
						uint8_t id_gr;
						read(fd, &id_gr, sizeof(id_gr)); 
						uint16_t num_of_state_of_gr; 
						read(fd, &num_of_state_of_gr, sizeof(num_of_state_of_gr)); 
				
						SLT.effMana.master_mana_gr[id_gr].num_state_of_gr = num_of_state_of_gr;
						SLT.effMana.master_mana_gr[id_gr].timeExistOfSta = malloc(sizeof(uint8_t) * num_of_state_of_gr); 
						read(fd, SLT.effMana.master_mana_gr[id_gr].timeExistOfSta, sizeof(uint8_t) * num_of_state_of_gr); 
				
					}
					SLT.effMana.update_master_mana_gr = true;
					
					/** send effect to all node */
					for (int i = 0; i < number_node; i++)	// datatype i = int, because i-- is can called in code
					{
						uint8_t id_node = 0;
						
						/** start send */
						lseek(fd, offset_start_node[i], SEEK_SET);
						
						
						read(fd, &id_node, sizeof(id_node));
						
						
						if (id_node == SLT.espnow.my_id) 
						{
							// open file effect
							file_request_t eff_open = {
								.cmd = F_OP,
								.source = F_NONE_SOURCE
							}; 
							xQueueSendToBack(xEffLoadf, &eff_open, pdMS_TO_TICKS(4000));
							
							// start write request
							file_request_t eff_st_write = {
								.cmd = F_ST_WR,
								.source = F_NONE_SOURCE
							}; 
							eff_st_write.write_start.offset = 0;
							eff_st_write.write_start.tot_byte = tot_size_node[i] - sizeof(id_node); 
							xQueueSendToBack(xEffLoadf, &eff_st_write, pdMS_TO_TICKS(4000));
							
							// write request 
							uint16_t checksum = 0xffff;
							
							uint32_t remaining = tot_size_node[i] - sizeof(id_node); 
							while(remaining > 0)
							{
								
								file_request_t eff_write = {
									.cmd = F_WR,
									.source = F_NONE_SOURCE
								}; 
								
								eff_write.write.offset = tot_size_node[i] - sizeof(id_node) - remaining;
								
								uint32_t tot_send = 512 < remaining ? 512 : remaining;
								
								eff_write.write.buf.data = malloc(sizeof(uint8_t) * tot_send);
								
								uint32_t tot_readed = read(fd, eff_write.write.buf.data, sizeof(uint8_t) * tot_send);
								eff_write.write.buf.tot_byte = tot_readed;
								
								checksum = crc16_modbus(checksum, eff_write.write.buf.data, eff_write.write.buf.tot_byte);
								
								if (xQueueSendToBack(xEffLoadf, &eff_write, portMAX_DELAY) == pdTRUE)
								{
									remaining -= eff_write.write.buf.tot_byte; 
								}
								else
								{
									if (eff_write.write.buf.data != NULL)
										free(eff_write.write.buf.data);
								}
								
							}
							
							// end write request
							
							file_request_t  eff_end_write  = {
								.cmd = F_END_WR,
								.source = F_NONE_SOURCE
							};
							eff_end_write.write_end.checksum = checksum; 
							xQueueSendToBack(xEffLoadf, &eff_end_write, pdMS_TO_TICKS(4000));
							
							// close request
							file_request_t  eff_close = {
								.cmd = F_CLS,
								.source = F_NONE_SOURCE
							};
							xQueueSendToBack(xEffLoadf, &eff_close, pdMS_TO_TICKS(4000));
							
							continue;
						}
						
						
						/** send open file request to id_node */
						xSemaphoreTake(xNowReturnState, 0);
						
						espnow_send_queue_t open_q;
						open_q.dest_id = id_node;
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
								
								uint32_t offset_st_write_node = 0;
								uint32_t tot_size_write_node = tot_size_node[i] - sizeof(id_node); 
								
								uint32_t tot_packet_node =	tot_size_write_node % max_byte_data == 0 ?
															(tot_size_write_node / max_byte_data) :
															(tot_size_write_node / max_byte_data) + 1;
								
								 
								uint8_t payload_st_write[sizeof(tot_size_write_node) + sizeof(offset_st_write_node)
									+ sizeof(tot_packet_node)];
								
								memcpy(payload_st_write, &tot_packet_node, sizeof(tot_packet_node));
								
								memcpy(&payload_st_write[4], &offset_st_write_node,
									sizeof(offset_st_write_node));
								
								memcpy(&payload_st_write[8],&tot_size_write_node,
									sizeof(tot_size_write_node));
								
								xSemaphoreTake(xNowReturnState, 0);
								
								espnow_send_queue_t write_st_q;
								write_st_q.dest_id = id_node;
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
										
										uint32_t offset_write_node = offset_st_write_node; 
										uint32_t offset_read_data = offset_start_node[i] + sizeof(id_node); 
										
										uint16_t checksum = 0xffff;
										
										uint32_t offset_write_packet[tot_packet_node];
										uint32_t offset_read_packet[tot_packet_node];
										uint32_t sizedata_packet[tot_packet_node];
										
										while (tot_size_write_node > 0)
										{
											uint32_t tot_byte_read = max_byte_data < tot_size_write_node ? max_byte_data : 
												tot_size_write_node;
											
											uint8_t* buf = malloc(tot_byte_read + NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET);
											
											if (buf == NULL)
												continue;
											
											memcpy(buf, &packet_number, NOW_SZOF_PACKET_NUM);
											memcpy(&buf[4], &offset_write_node, NOW_SZOF_OFFSET);
											
											lseek(fd, offset_read_data, SEEK_SET);
											read(fd, &buf[8], tot_byte_read);
											
											offset_write_packet[packet_number] = offset_write_node;
											offset_read_packet[packet_number] = offset_read_data;
											sizedata_packet[packet_number] = tot_byte_read;
											
											checksum = crc16_modbus(checksum, &buf[8], tot_byte_read);
											
											espnow_send_queue_t write_q;
											write_q.dest_id = id_node;
											write_q.tmout_ms = 1000;
											write_q.buf = espnow_make_frame_send(buf, NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET + tot_byte_read, NOW_WRF); 
											if (write_q.buf.data != NULL && write_q.buf.tot_byte > 0)
												xQueueSend(xNowSend, &write_q, portMAX_DELAY);											
											
											
											offset_write_node += tot_byte_read;
											offset_read_data += tot_byte_read;
											tot_size_write_node -= tot_byte_read;
											packet_number++;
											
											if (buf != NULL) {
												free(buf);
												buf = NULL;
											}
										}
										uint32_t lastick = xTaskGetTickCount();
										while (xTaskGetTickCount() - lastick < pdMS_TO_TICKS(4000))
										{
											bool can_break = false;
											
											/** send write end to des_id */
											xSemaphoreTake(xNowReturnState, 0);
											espnow_send_queue_t end_write_q; 
											end_write_q.dest_id = id_node;
											end_write_q.tmout_ms = 1000;
											end_write_q.buf = espnow_make_frame_send(&checksum, sizeof(checksum), NOW_END_WRF); 
											if (end_write_q.buf.data != NULL && end_write_q.buf.tot_byte > 0)
												xQueueSend(xNowSend, &end_write_q, portMAX_DELAY);
										
											if (xSemaphoreTake(xNowReturnState, pdMS_TO_TICKS(3000)) == pdPASS)
											{
												/** resend write packet */
												if (SLT.espnow.state_return == NOW_NACK)
												{
													espnow_wrf_resend resend_q = {.tot_req_pack = 0, .request = NULL};
													xQueueReceive(xNowResendWrf, &resend_q, pdMS_TO_TICKS(4000));
													if (resend_q.request == NULL)
													{
														i--;
														can_break = true;
													}
													else
													{
														if (resend_q.request[0] == 0xffffffff) 
														{ 
															i--;
															can_break = true;
														}
														else
														{
														
															for (int j = 0; j < resend_q.tot_req_pack; j++)
															{
																uint32_t packet_number = resend_q.request[j];
																
																uint8_t* buf = malloc(sizedata_packet[packet_number] + NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET);
															
																memcpy(buf, &packet_number, NOW_SZOF_PACKET_NUM);
																memcpy(&buf[4], &offset_write_packet[packet_number], NOW_SZOF_OFFSET);
																
																lseek(fd, offset_read_packet[packet_number], SEEK_SET);
																read(fd, &buf[8], sizedata_packet[packet_number]);
																espnow_send_queue_t write_q;
																write_q.dest_id = id_node;
																write_q.tmout_ms = 1000;
																write_q.buf = espnow_make_frame_send(buf,
																	NOW_SZOF_PACKET_NUM + NOW_SZOF_OFFSET + sizedata_packet[packet_number],
																	NOW_WRF);
																if (write_q.buf.data != NULL && write_q.buf.tot_byte > 0)
																	xQueueSend(xNowSend, &write_q, portMAX_DELAY);
																
																if (buf != NULL) {
																	free(buf);
																	buf = NULL;
																}
														
															}
														}
														if (resend_q.request != NULL) 
														{
															free(resend_q.request);
															resend_q.request = NULL;
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
					fd = -1;
					
					xSemaphoreGive(xMasterManaEffGr);
				}
			}
		}
	}
}

/**
 * request frame: KEY + CMD (NOW_EFF_SYNC | NOW_EFF_ASYNC) + number of group + list group + list state + CRC
 *
 */
void task_make_effect()
{
	
	uint16_t state_current_of_gr[MAX_NUM_OF_GROUP];
	uint8_t time_update_state_of_gr[MAX_NUM_OF_GROUP];
	uint32_t last_time_update_state_of_gr[MAX_NUM_OF_GROUP];
	
	uint8_t duties[MAX_NUM_CHANNEL]; memset(duties, 0, sizeof(duties));
	
	bool setting_first = false;
	
	bool data_available = false;
	
	/** init struct effect */
	bool allocate_first = true;
	
	SLT.effMana.brNess = 0;
	SLT.effMana.speedEf = 0;
	SLT.effMana.numGroup = 0;
	
	
	for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
	{
		
		SLT.effMana.offStartGr[i] = 0;
		SLT.effMana.totByteGr[i] = 0;
		SLT.effMana.id_group[i] = 0;
		
		SLT.effMana.p_group[i].numState = 0;
		SLT.effMana.p_group[i].numObject = 0;
		SLT.effMana.p_group[i].p_timExistOfSta = NULL;
		SLT.effMana.p_group[i].p_object = NULL;
		SLT.effMana.p_group[i].id_object = NULL;
	}
	effect_request_t eff_req_tmp = {.mode = EFF_ASYNCHRONOUS};
	
	while (1)
	{
		if (xSemaphoreTake(xLoadEffect, 0) == pdTRUE)
		{
			
			struct stat st;
			int ret = stat(PATH_EFFECT, &st);
			
			int fd = -1;
			if(ret >= 0)
				fd = open(PATH_EFFECT, O_RDONLY, 0666);
			
			if (fd >= 0)
			{
				
				lseek(fd, 0, SEEK_SET); 
				
				/** brightness ; speed ; number of group */
				read(fd, &SLT.effMana.brNess, sizeof(SLT.effMana.brNess));
				read(fd, &SLT.effMana.speedEf, sizeof(SLT.effMana.speedEf)); 
				read(fd, &SLT.effMana.numGroup, sizeof(SLT.effMana.numGroup)); 
				
				/** offset start of Groups */
				read(fd, SLT.effMana.offStartGr, sizeof(uint32_t) * SLT.effMana.numGroup);
				
				read(fd, SLT.effMana.totByteGr, sizeof(uint32_t) * SLT.effMana.numGroup);
				
				/** reset state id group */
				memset(SLT.effMana.id_group, 0, sizeof(SLT.effMana.id_group));
				
				/** read infor for each group */
				for (int i = 0; i < SLT.effMana.numGroup; i++)
				{
					lseek(fd, SLT.effMana.offStartGr[i], SEEK_SET);
					
					/** get id of group */
					int8_t id_group = -1;	
					read(fd, &id_group, sizeof(id_group));
					
					if (id_group == -1)
						continue;
					
					SLT.effMana.id_group[i] = id_group;
					
					/** read number of state of groups */
					read(fd, &SLT.effMana.p_group[i].numState, sizeof(SLT.effMana.p_group[i].numState));
					
					/** allocate and read time exist of states of groups */
					if (SLT.effMana.p_group[i].p_timExistOfSta != NULL && allocate_first != true) 
					{
						free(SLT.effMana.p_group[i].p_timExistOfSta);
						SLT.effMana.p_group[i].p_timExistOfSta = NULL;
					}
					
					SLT.effMana.p_group[i].p_timExistOfSta = malloc(sizeof(uint8_t) * SLT.effMana.p_group[i].numState);
					read(fd, SLT.effMana.p_group[i].p_timExistOfSta, sizeof(uint8_t) * SLT.effMana.p_group[i].numState);
					
					/** read number of object of group */
					read(fd,
						&SLT.effMana.p_group[i].numObject,
						sizeof(SLT.effMana.p_group[i].numObject));
					
					/** allocate list id of object */
					if (SLT.effMana.p_group[i].id_object != NULL && allocate_first != true)
					{
						free(SLT.effMana.p_group[i].id_object); 
						SLT.effMana.p_group[i].id_object = NULL;
					}
					SLT.effMana.p_group[i].id_object = malloc(sizeof(uint8_t) * SLT.effMana.p_group[i].numObject);
					
					/** allocate list object */
					if (SLT.effMana.p_group[i].p_object != NULL && allocate_first != true)
					{
						
						free(SLT.effMana.p_group[i].p_object);
						SLT.effMana.p_group[i].p_object = NULL;
					}
					SLT.effMana.p_group[i].p_object = malloc(sizeof(object_t) * SLT.effMana.p_group[i].numObject);
					
					for (int j = 0; j < SLT.effMana.p_group[i].numObject; j++)
					{
						/** read id objects of group */
						read(fd, &SLT.effMana.p_group[i].id_object[j], sizeof(SLT.effMana.p_group[i].id_object[j]));
						
						/** tot pin of object */
						read(fd, &SLT.effMana.p_group[i].p_object[j].numPin, sizeof(SLT.effMana.p_group[i].p_object[j].numPin));
						/** pins of object */
						read(fd, SLT.effMana.p_group[i].p_object[j].p_pin, sizeof(uint8_t) * SLT.effMana.p_group[i].p_object[j].numPin);
						
						/** brightness states of object */
						
						if (SLT.effMana.p_group[i].p_object[j].brNessofState != NULL && allocate_first != true) 
						{
							free(SLT.effMana.p_group[i].p_object[j].brNessofState);
							SLT.effMana.p_group[i].p_object[j].brNessofState = NULL;
						}
						
						SLT.effMana.p_group[i].p_object[j].brNessofState = malloc(sizeof(uint8_t) * SLT.effMana.p_group[i].numState);
						read(fd, SLT.effMana.p_group[i].p_object[j].brNessofState, sizeof(uint8_t) * SLT.effMana.p_group[i].numState);
					}
				}
				close(fd);
				
				data_available = true;
				
				/** start from state 0 for all group exist */
				for (int i = 0; i < SLT.effMana.numGroup; i++)
				{

					state_current_of_gr[i] = 0;
					time_update_state_of_gr[i] = SLT.effMana.p_group[i].p_timExistOfSta[0];
					last_time_update_state_of_gr[i] = xTaskGetTickCount();
					
				}
				
				setting_first = true;
				allocate_first = false;
				
			}
			
		}
		
		bool new_request = false;
		
		if (xQueueReceive(xEffRequest, &eff_req_tmp, 0) == pdTRUE)
		{
			new_request = true;
			
		}
		
		if (data_available == true)
		{
			bool new_pwm_setting = false;
			
			
			// update state for requested groups
			if (new_request == true)
			{
				for (int i = 0; i < eff_req_tmp.number_of_group; i++)
				{
					
					bool group_exist = false;
					
					uint8_t group_number = eff_req_tmp.gproup_request[i];
					
					uint8_t group_index = 0;
					
					for (int j = 0; j < SLT.effMana.numGroup; j++)
					{
						if (SLT.effMana.id_group[j] == group_number) 
						{
							group_index = j;
							group_exist = true;
							break;
						}
					}
					
					if (group_exist == false)
						continue;
					
					
					uint16_t state_number_of_group = eff_req_tmp.state[i];
					
					new_pwm_setting = true;
					
					for (int j = 0; j < SLT.effMana.p_group[group_index].numObject; j++)
					{
						uint16_t state_number_of_object = state_number_of_group;
						
						for (int k = 0; k < SLT.effMana.p_group[group_index].p_object[j].numPin; k++)
						{
							uint8_t index_channel = SLT.effMana.p_group[group_index].p_object[j].p_pin[k];
							duties[index_channel] = SLT.effMana.p_group[group_index].p_object[j].brNessofState[state_number_of_object];
						}
					}
					
					state_current_of_gr[group_index] = state_number_of_group;
					last_time_update_state_of_gr[group_index] = xTaskGetTickCount();
					time_update_state_of_gr[group_index] = SLT.effMana.p_group[group_index].p_timExistOfSta[state_number_of_group];
				}	
				
			}
			else
			{
				// update state over time
				if (eff_req_tmp.mode == EFF_ASYNCHRONOUS)
				{

					for (int i = 0; i < SLT.effMana.numGroup; i++)
					{
						if (setting_first == true)
						{
							new_pwm_setting = true;
							setting_first = false;
						
							for (int j = 0; j < SLT.effMana.p_group[i].numObject; j++)
							{
								for (int k = 0; k < SLT.effMana.p_group[i].p_object[j].numPin; k++)
								{
									uint8_t index_channel = SLT.effMana.p_group[i].p_object[j].p_pin[k];
									duties[index_channel] = SLT.effMana.p_group[i].p_object[j].brNessofState[state_current_of_gr[i]];
								}
							}
						
						}
						else if (xTaskGetTickCount() - last_time_update_state_of_gr[i] > (time_update_state_of_gr[i] * pdMS_TO_TICKS(UNIT_OF_TIME_EXIST))) 
						{
		
							new_pwm_setting = true;
							
							
							uint16_t index_state_next = (state_current_of_gr[i] + 1) % SLT.effMana.p_group[i].numState;
							for (int j = 0; j < SLT.effMana.p_group[i].numObject; j++)
							{
								for (int k = 0; k < SLT.effMana.p_group[i].p_object[j].numPin; k++)
								{
									uint8_t index_channel = SLT.effMana.p_group[i].p_object[j].p_pin[k];
									duties[index_channel] = SLT.effMana.p_group[i].p_object[j].brNessofState[index_state_next];
								}
							}
							state_current_of_gr[i] = index_state_next;
							last_time_update_state_of_gr[i] = xTaskGetTickCount();
							time_update_state_of_gr[i] = SLT.effMana.p_group[i].p_timExistOfSta[index_state_next];
						}
					}
				}
			}
			
			if (new_pwm_setting == true) 
				set_duties_pwm(&SLT.Pwm, duties, sizeof(duties));
			
		}
		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
		
	}
}

void task_effect_synchr_asynchr()
{
	int8_t mode = EFF_ASYNCHRONOUS; 
	
	uint8_t number_of_gr_exist = 0; 
	
	uint16_t state_cur_gr[MAX_NUM_OF_GROUP]; 
	uint32_t last_tick_gr[MAX_NUM_OF_GROUP];
	uint32_t time_update_state_of_gr[MAX_NUM_OF_GROUP]; 
	
	for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
	{
		last_tick_gr[i] = xTaskGetTickCount();
		state_cur_gr[i] = 0; 
	}
	
	bool data_available = false;
	bool first_setting = false;
	
	for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
	{
		SLT.effMana.master_mana_gr[i].num_state_of_gr = 0;
		SLT.effMana.master_mana_gr[i].timeExistOfSta = NULL;
	}
	
	if (is_same_macadrr(SLT.espnow.gw_peer.mac, SLT.wifi.ap_macaddr) == true
	&& SLT.espnow.gw_peer.id == SLT.espnow.my_id)
	{
		struct stat st;
		int ret = stat(PATH_TCP_FILE, &st);
			
		int fd = -1;
		if(ret >= 0)
			fd = open(PATH_TCP_FILE, O_RDONLY, 0666);
			
		if (fd >= 0)
		{
			lseek(fd, 0, SEEK_SET);
			uint8_t num_of_node;
			read(fd, &num_of_node, sizeof(num_of_node));
			
			lseek(fd, sizeof(num_of_node) + sizeof(uint32_t) * num_of_node + sizeof(uint32_t) * num_of_node, SEEK_SET); 
			
			uint8_t num_of_gr; 
			read(fd, &num_of_gr, sizeof(num_of_gr)); 
			for (int i = 0; i < num_of_gr; i++)
			{
				uint8_t id_gr;
				read(fd, &id_gr, sizeof(id_gr)); 
				uint16_t num_of_state_of_gr; 
				read(fd, &num_of_state_of_gr, sizeof(num_of_state_of_gr)); 
				
				SLT.effMana.master_mana_gr[id_gr].num_state_of_gr = num_of_state_of_gr;
				SLT.effMana.master_mana_gr[id_gr].timeExistOfSta = malloc(sizeof(uint8_t) * num_of_state_of_gr); 
				read(fd, SLT.effMana.master_mana_gr[id_gr].timeExistOfSta, sizeof(uint8_t) * num_of_state_of_gr); 
				
			}
			
			SLT.effMana.update_master_mana_gr = true; 
			
			close(fd);
			
		}
	}
	
	while (1)
	{
		if (is_same_macadrr(SLT.espnow.gw_peer.mac, SLT.wifi.ap_macaddr) == true
			&& SLT.espnow.gw_peer.id == SLT.espnow.my_id)
		{
			if (xSemaphoreTake(xMasterManaEffGr, portMAX_DELAY) == pdTRUE)
			{
				if (SLT.effMana.update_master_mana_gr == true)
				{
					number_of_gr_exist = 0; 
					
					for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
					{
						
						if (SLT.effMana.master_mana_gr[i].num_state_of_gr == 0 || SLT.effMana.master_mana_gr[i].timeExistOfSta == NULL)
							continue;
							
						state_cur_gr[i] = 0;
						last_tick_gr[i] = xTaskGetTickCount();
						time_update_state_of_gr[i] = SLT.effMana.master_mana_gr[i].timeExistOfSta[0];
						
						first_setting = true;
						
						number_of_gr_exist++;
						
						data_available = true;
					}
					
					SLT.effMana.update_master_mana_gr = false;
					
				}
				if (data_available == true)
				{
					
					if (xSemaphoreTake(xMasterModeEff, 0) == pdTRUE)
					{
						mode = SLT.effMana.master_mode; 
						for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
						{
						
							if (SLT.effMana.master_mana_gr[i].num_state_of_gr == 0 || SLT.effMana.master_mana_gr[i].timeExistOfSta == NULL)
								continue;
						
							state_cur_gr[i] = 0;
							last_tick_gr[i] = xTaskGetTickCount();
							time_update_state_of_gr[i] = SLT.effMana.master_mana_gr[i].timeExistOfSta[0];
						}
						if (mode == EFF_SYNCHRONOUS)
							first_setting = true;
					}
					
					if (mode == EFF_SYNCHRONOUS)
					{
						uint8_t gr_update[8]; memset(gr_update, 0, sizeof(gr_update)); 
						uint16_t state_gr[8]; memset(state_gr, 0, sizeof(state_gr)); 
						uint8_t index_tmp = 0;
						
						
						uint8_t tot_gr_request = 0; 
						
						
						if (first_setting == true)
						{
							first_setting = false;
							for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
							{
								if (SLT.effMana.master_mana_gr[i].num_state_of_gr != 0 && SLT.effMana.master_mana_gr[i].timeExistOfSta != NULL)
								{
									
									gr_update[index_tmp] = i; 
									state_gr[index_tmp] = 0; 
									
									index_tmp++; 
									
									tot_gr_request++; 
								}
							}
							
						}
						else
						{
							for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
							{
								if (SLT.effMana.master_mana_gr[i].num_state_of_gr != 0 && SLT.effMana.master_mana_gr[i].timeExistOfSta != NULL)
								{
									if (xTaskGetTickCount() - last_tick_gr[i] > (time_update_state_of_gr[i] * pdMS_TO_TICKS(UNIT_OF_TIME_EXIST)))
									{
										uint16_t index_state_next = (state_cur_gr[i] + 1) % SLT.effMana.master_mana_gr[i].num_state_of_gr;
									
										state_cur_gr[i] = index_state_next;
										last_tick_gr[i] = xTaskGetTickCount();
										time_update_state_of_gr[i] = SLT.effMana.master_mana_gr[i].timeExistOfSta[index_state_next];
									
										gr_update[index_tmp] = i; 
										state_gr[index_tmp] = index_state_next; 
										index_tmp++; 
									
										tot_gr_request++; 
									}
								}
							
							}	
						}

						if (tot_gr_request != 0)
						{
							uint8_t number_of_state_byte = sizeof(uint16_t) * tot_gr_request;
							uint32_t tot_byte_pl = sizeof(tot_gr_request) + sizeof(uint8_t) * tot_gr_request + number_of_state_byte; 
							uint8_t *payload = malloc(tot_byte_pl);
							
							payload[0] = tot_gr_request; 
							memcpy(&payload[1], gr_update, tot_gr_request * sizeof(uint8_t));
							memcpy(&payload[1 + tot_gr_request], state_gr, tot_gr_request * sizeof(uint16_t));
													
							for (int i = 0; i < SLT.espnow.cnt_id_added; i++)
							{
							
								espnow_send_queue_t send_q;
								send_q.dest_id = SLT.espnow.peer_list[i].id;
								send_q.tmout_ms = 300;
								send_q .buf = espnow_make_frame_send(payload, tot_byte_pl, NOW_EFF_SYNC); 
								if (send_q.buf.data != NULL && send_q.buf.tot_byte > 0)
									xQueueSend(xNowSend, &send_q, portMAX_DELAY);
							}
				
							effect_request_t eff_req_tmp;
							eff_req_tmp.mode = EFF_SYNCHRONOUS; 
				
							eff_req_tmp.number_of_group = tot_gr_request;
				
							memcpy(eff_req_tmp.gproup_request, &payload[1], tot_gr_request);	
							
							memcpy(eff_req_tmp.state, &payload[1 + tot_gr_request], tot_gr_request * sizeof(uint16_t));	
				
							xQueueSendToBack(xEffRequest, &eff_req_tmp, portMAX_DELAY);
						
						
							if (payload != NULL)
								free(payload);
						}
					
					}
					else if (mode == EFF_ASYNCHRONOUS)
					{
						
						uint8_t number_of_state_byte = sizeof(uint16_t) * number_of_gr_exist; 
						uint8_t tot_byte_pl = sizeof(number_of_gr_exist) + sizeof(uint8_t) * number_of_gr_exist + number_of_state_byte; 
						uint8_t *payload = malloc(tot_byte_pl);
						
						/** state 0 */
						memset(payload, 0, tot_byte_pl);
						
						payload[0] = number_of_gr_exist; 
						uint8_t index = 1; 
						for (int i = 0; i < MAX_NUM_OF_GROUP; i++)
						{
							
							if (SLT.effMana.master_mana_gr[i].num_state_of_gr != 0 && SLT.effMana.master_mana_gr[i].timeExistOfSta != NULL)
							{
								payload[index] = i;
								index++;
							}
							
						}
						
						for (int i = 0; i < SLT.espnow.cnt_id_added; i++)
						{
							
							espnow_send_queue_t send_q;
							send_q.dest_id = SLT.espnow.peer_list[i].id;
							send_q.tmout_ms = 300;
							send_q .buf = espnow_make_frame_send(payload, tot_byte_pl, NOW_EFF_ASYNC); 
							if (send_q.buf.data != NULL && send_q.buf.tot_byte > 0)
								xQueueSend(xNowSend, &send_q, portMAX_DELAY);
						}
				
						effect_request_t eff_req_tmp;
						eff_req_tmp.mode = EFF_ASYNCHRONOUS; 
				
						eff_req_tmp.number_of_group = number_of_gr_exist;
				
						memcpy(eff_req_tmp.gproup_request, &payload[1], number_of_gr_exist);		
				
						memset(eff_req_tmp.state, 0, sizeof(eff_req_tmp.state));		
				
						xQueueSendToBack(xEffRequest, &eff_req_tmp, portMAX_DELAY);
						
						
						if (payload != NULL)
							free(payload); 
						
						mode = -1;
					}
				}

				xSemaphoreGive(xMasterManaEffGr); 
			}

		}

		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
	}
}