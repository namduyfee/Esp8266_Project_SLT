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
		.can_send = true,
		.p_peer = NULL,
		.sent = false,
		.tot_pos_added = 0,
		.gateway_added = false,
		.broadcast_cnt = 0,
		.my_pos = 1,
		.mode_send = NONE_ESPNOW
	},
	.wifi = {
		.is_gateway = false,
		.gateway_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}		
	}
	
};

QueueHandle_t xBuffLoadf;
QueueHandle_t xBuffSendf;

QueueHandle_t xEspNowf;

QueueHandle_t xEspNowRecv;

SemaphoreHandle_t xSendEspNow;


void my_init_project(void)
{
	
	nvs_flash_init();
	spiffs_init();
	config_GPIO_OUT();
	config_input_pullup_gpio();
	
	my_pwm_start(&SLT.Pwm); 
	
	tcpip_adapter_init();
	
	my_start_wifi(&SLT.wifi);
	
	init_server_tpcp(80, 5);
	
	init_espnow();
}


void app_main(void) {
	
	
	my_init_project();
	
	xBuffLoadf = xQueueCreate(30, sizeof(tcp_recv_t));
	
	xBuffSendf = xQueueCreate(10, sizeof(tcp_data_t)); 
	
	xEspNowRecv = xQueueCreate(10, sizeof(buf_espnow_t));
	
	xSendEspNow = xSemaphoreCreateBinary(); 
	xSemaphoreGive(xSendEspNow);
	
	xTaskCreate(task_esp_now_send, "task_esp_now_send", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_tcp_file_bin, "task_tcp_file_bin", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
	xTaskCreate(task_select_master, "task_select_master", MIN_SIZE_OP_FILE, NULL, 4, NULL);
		 
	xTaskCreate(task_esp_now_recv, "task_esp_now_recv", MIN_SIZE_OP_FILE, NULL, 4, NULL);
	
}


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
				/* handler when button is pressed */
				
				if (xSemaphoreTake(xSendEspNow, portMAX_DELAY) == pdPASS)
				{

					int fd = open("/spiffs/gateway.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
					
					if (fd >= 0)
					{
						
						SLT.wifi.is_gateway = true;
						memcpy(SLT.wifi.gateway_addr, SLT.wifi.sta_macaddr, MAC_ADDR_LEN);
						lseek(fd, POS_ADDR_GATEWAY, SEEK_SET);
						write(fd, SLT.wifi.gateway_addr, MAC_ADDR_LEN);
						close(fd);
					}
					

					if (SLT.wifi.is_gateway == true)
					{
						SLT.espnow.mode_send = BROADCAST;
						SLT.espnow.broadcast_cnt = 0;
						
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
					}
					
					xSemaphoreGive(xSendEspNow);
				}
				
			}
		}		
		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
	}
}
/**
 *	@brief	task handler espnow receive
 */
void task_esp_now_recv()
{
	
	while (1)
	{
		if (xQueueReceive(xEspNowRecv, &SLT.espnow.recv.buf, portMAX_DELAY) == pdPASS)
		{
			
			uint8_t* data = (uint8_t*)SLT.espnow.recv.buf.data;
			uint32_t len = SLT.espnow.recv.buf.len;
			
			if (data != NULL && len > 0)
			{
				if (data[0] == 'S' && data[1] == 'L' && data[2] == 'T')
				{
					if (data[ESPNOW_INDEX_CMD] == BROADCAST)
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

						int fd = open("/spiffs/gateway.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);			/**< open and trunc file */
						
						if (fd >= 0)
						{
							memcpy(SLT.wifi.gateway_addr, &data[ESPNOW_INDEX_ADDR], MAC_ADDR_LEN);
							lseek(fd, POS_ADDR_GATEWAY, SEEK_SET);
							write(fd, SLT.wifi.gateway_addr, 6);
							SLT.wifi.is_gateway = false;
								
							/** after add peer gateway; send request add this peer to gateway */
							if (xSemaphoreTake(xSendEspNow, portMAX_DELAY) == pdPASS)
							{
								SLT.espnow.mode_send = ADD_PEER;
								SLT.espnow.gateway_added = false;
								set_duty_pwm(&SLT.Pwm, 3, 740);
								xSemaphoreGive(xSendEspNow);
							}	
							
							close(fd);
						}

						espnow_add_peer((uint8_t*)&data[ESPNOW_INDEX_ADDR], data[ESPNOW_INDEX_POS]);		/**< add gateway into list peer */
						
				
					}
					
					else if (data[ESPNOW_INDEX_CMD] == ADD_PEER)
					{
						espnow_add_peer((uint8_t*)&data[ESPNOW_INDEX_ADDR], data[ESPNOW_INDEX_POS]); 
						set_duty_pwm(&SLT.Pwm, 3, 780);
					}
					else if (data[ESPNOW_INDEX_CMD] == ESPNOW_WRITE)
					{
						if (data[ESPNOW_INDEX_DATA] == 'a')
						{
							set_duty_pwm(&SLT.Pwm, 0, 650);
						}	
						else if (data[ESPNOW_INDEX_DATA] == 'b')
						{
							set_duty_pwm(&SLT.Pwm, 0, 800);
						}
					}
					xSemaphoreGive(xSendEspNow);
				}
				
				free(SLT.espnow.recv.buf.data);				
			}

		}
		
	}
	
}
/**
 *	@brief	
 */
void task_esp_now_send() 
{
	/* send broadcast ; master signal */
	uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	
	uint8_t data_broadcast[7] = {SLT.espnow.my_pos};
	memcpy(&data_broadcast[1], SLT.wifi.sta_macaddr, MAC_ADDR_LEN);
	buf_espnow_t* buf_broadcast = (buf_espnow_t*)espnow_make_segment(data_broadcast, BROADCAST, sizeof(data_broadcast));
	
	
	/* send request add peer after listened broadcast */
	uint8_t data_add_peer[7] = {SLT.espnow.my_pos};
	memcpy(&data_add_peer[1], SLT.wifi.sta_macaddr, MAC_ADDR_LEN);
	buf_espnow_t* buf_add_peer = (buf_espnow_t*)espnow_make_segment(data_add_peer, ADD_PEER, sizeof(data_add_peer));
	
	/* data1 test */
	uint8_t data1[1] = {'a'};
	buf_espnow_t* buf_data1 = (buf_espnow_t*)espnow_make_segment(data1, ESPNOW_WRITE, sizeof(data1));

	/* data2 test */
	uint8_t data2[1] = {'b'};
	buf_espnow_t* buf_data2 = (buf_espnow_t*)espnow_make_segment(data2, ESPNOW_WRITE, sizeof(data2));
	
	
	esp_err_t ret; 
	void* data = buf_data1->data;
	uint32_t len = buf_data1->len;
	
	
	SLT.espnow.can_send = true;
	SLT.espnow.sent = false;
	
	
	
	while (1)
	{
		
		if (xSemaphoreTake(xSendEspNow, portMAX_DELAY) == pdPASS)
		{
			if (SLT.espnow.mode_send == BROADCAST)
			{ 	
				if (SLT.espnow.can_send == true)
				{
					SLT.espnow.can_send = false; 
					
					ret = esp_now_send(broadcast,
						(uint8_t*)buf_broadcast->data,
						buf_broadcast->len);
				
					if (ret != ESP_OK)
					{
						SLT.espnow.can_send  = true;
					}
	
				}						
				
				if(++SLT.espnow.broadcast_cnt >= MAX_BROADCAST_CNT)
					SLT.espnow.mode_send = ESPNOW_WRITE;
				
				xSemaphoreGive(xSendEspNow);
				vTaskDelay(pdMS_TO_TICKS(200));

			}
			else if (SLT.espnow.mode_send == ADD_PEER)
			{

				if (SLT.espnow.gateway_added == false)
				{
					if (SLT.espnow.can_send == true)
					{
						SLT.espnow.can_send = false; 
					
						ret = esp_now_send(SLT.wifi.gateway_addr,
							(uint8_t*)buf_add_peer->data,
							buf_add_peer->len);
				
						if (ret != ESP_OK)
						{
							SLT.espnow.can_send  = true;
						}
					}					
				}
				else
				{
					SLT.espnow.mode_send = ESPNOW_WRITE;
				}
				xSemaphoreGive(xSendEspNow);
				vTaskDelay(pdMS_TO_TICKS(100));
				
			}
			else if (SLT.espnow.mode_send == ESPNOW_READ || 
			         SLT.espnow.mode_send == ESPNOW_WRITE)
			{
				if (SLT.espnow.can_send == true)
				{
					if (SLT.espnow.tot_pos_added > 0)
					{
						if (SLT.espnow.sent == false)
						{
							/* retry send the last buffer */
						}
						else
						{
							/* send the next buffer */
							if (data != buf_data1->data)
							{
								data = buf_data1->data; 
								len = buf_data1->len;
							}
							else
							{
								data = buf_data2->data; 
								len = buf_data2->len;
							}
							SLT.espnow.sent = false; 

						}
						SLT.espnow.can_send  = false;
			
						ret = esp_now_send(SLT.espnow.p_peer->info.peer_addr, (uint8_t*)data, len);
						if (ret != ESP_OK)
						{
							SLT.espnow.can_send  = true;
						}				
					}			
				}
				xSemaphoreGive(xSendEspNow);
				vTaskDelay(pdMS_TO_TICKS(100));
			}
			else
			{
				xSemaphoreGive(xSendEspNow);
				vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
			}
		}
	}
}

/**
 *	@brief	handler request from tcp
 *	
 *	@details	
 *		- format
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
			if (SLT.server.recv.segment.command == FORMAT)
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}
				
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}
				
				esp_spiffs_format(NULL);
				spiffs_init(); 
			}
			
			else if (SLT.server.recv.segment.command == OPEN)
			{
				
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}
				
				if (fd < 0)
				{
					struct stat st;
					int ret = stat("/spiffs/data.bin", &st);
					fd = ret < 0 ?  open("/spiffs/data.bin", O_RDWR | O_CREAT | O_TRUNC, 0666) : 
									open("/spiffs/data.bin", O_RDWR | O_CREAT, 0666);
					
				}
				

			}
			else if (SLT.server.recv.segment.command == CLOSE)
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}

			}			
			else if (SLT.server.recv.segment.command == DELETE)
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}

				if (fd >= 0) 
				{
					close(fd);
					fd = -100;
				}
				unlink("/spiffs/data.bin");

			}
			
			else if (SLT.server.recv.segment.command == READ)
			{
				if (fd < 0)
				{
					if (SLT.server.recv.segment.data.content != NULL) 
					{
						free(SLT.server.recv.segment.data.content);
						SLT.server.recv.segment.data.content = NULL;
					}		
				}
				else
				{
					
					lseek(fd, SLT.server.recv.segment.pos_in_file, SEEK_SET); 
					uint32_t remaining = SLT.server.recv.segment.data.len;
					
					
					while (remaining > 0)
					{
						SLT.server.send.data.len = remaining <= 512 ? remaining : 512; 
						SLT.server.send.data.content = malloc(SLT.server.send.data.len); 
						
						if (SLT.server.send.data.content != NULL)
						{
							read(fd, SLT.server.send.data.content, SLT.server.send.data.len);
						
							xQueueSendToBack(xBuffSendf, &SLT.server.send, portMAX_DELAY);
							remaining = remaining - SLT.server.send.data.len;						
						}
						else
						{
							break;
						}
					}
					
					if (SLT.server.recv.segment.data.content != NULL) 
					{
						free(SLT.server.recv.segment.data.content);
						SLT.server.recv.segment.data.content = NULL;
					}
				}
				
				SLT.server.send.data.content = NULL;
				SLT.server.send.data.len = 0; 
				xQueueSendToBack(xBuffSendf, &SLT.server.send, portMAX_DELAY);
			}
			else if (SLT.server.recv.segment.command == WRITE)
			{
				
				if (fd < 0)
				{
					if (SLT.server.recv.segment.data.content != NULL)
					{
						free(SLT.server.recv.segment.data.content); 	
						SLT.server.recv.segment.data.content = NULL; 
					}	
				}
				else
				{				
					if (SLT.server.recv.segment.data.content != NULL)
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
							
							ssize_t to_write = SLT.server.recv.tot_len <= SLT.server.recv.segment.data.len ?
												SLT.server.recv.tot_len : SLT.server.recv.segment.data.len;
							
							ssize_t written = write(fd,
								&((uint8_t*)SLT.server.recv.segment.data.content)[SLT.server.recv.segment.pos_data], 
								to_write);
							
							if (written > 0)
								SLT.server.recv.tot_len = SLT.server.recv.tot_len - written; 
							
							SLT.server.recv.current_pos_file = lseek(fd, 0, SEEK_CUR);							
						}
						
						free(SLT.server.recv.segment.data.content); 		
						SLT.server.recv.segment.data.content = NULL;
					
					}

				}
			}
			else
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}				
			}
			
			
		}
	}
}


