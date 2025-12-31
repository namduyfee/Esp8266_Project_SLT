#include "my_lib.h"
#include "string.h"

#define MIN_DELAY 10

void task_esp_now(); 
void esp_recv_inf_wifi();
void task_tcp_file_bin(); 

void task_wifi_sta(); 

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
	
	.is_gateway = false,
	
	.gateway_addr = {0x84, 0xF3, 0xEB, 0xA6, 0xD8, 0x4F},

	.espnow = {
		.can_send	= true,
		.p_peer		= NULL,
		.sent		= false,
		.gateway_added = false,
		.tot_peer   = 0
		
	}
};

uint8_t data_esp_now[] = "SLT1"; 
uint32_t len_test_data_esp_now = sizeof(data_esp_now)/sizeof(data_esp_now[0]);
uint8_t data_esp_now2 [] = "SLT2"; 
uint32_t len_test_data_esp_now2 = sizeof(data_esp_now) / sizeof(data_esp_now[0]);

SemaphoreHandle_t xRecvPassWifi; 
SemaphoreHandle_t xTryConnectWifi; 

QueueHandle_t xBuffLoadf;
QueueHandle_t xBuffSendf;

void my_init_project(void)
{
	
	nvs_flash_init();
	spiffs_init();
	config_GPIO_OUT();
	config_input_pullup_gpio();
	
	my_pwm_start(&SLT.Pwm); 
	
	tcpip_adapter_init();
	my_start_wifi();
	
	init_server_tpcp(80, 5);
	init_espnow();
}


void app_main(void) {
	
	
	my_init_project();
	
	
	xRecvPassWifi = xSemaphoreCreateBinary();
	xTryConnectWifi = xSemaphoreCreateBinary();
	
	xBuffLoadf = xQueueCreate(30, sizeof(tcp_recv_t));
	
	xBuffSendf = xQueueCreate(10, sizeof(tcp_data_t)); 
	
	
	xTaskCreate(task_esp_now, "esp_now_task", 1024, NULL, 4, NULL);
	
//	xTaskCreate(esp_recv_inf_wifi, "esp_recv_inf_wifi", 1024, NULL, 5, NULL);
	
	xTaskCreate(task_tcp_file_bin, "esp_recv_file_bin", 2048, NULL, 4, NULL);
	
//	xTaskCreate(task_wifi_sta, "wifi_sta_task", 1024, NULL, 4, NULL);

	 
	
	while (1)
	{

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	
}

void task_esp_now() 
{
	esp_err_t ret; 
	SLT.espnow.can_send = true; 
	SLT.espnow.sent = false;
	
	void* data = data_esp_now;
	uint32_t len = len_test_data_esp_now;
	
	while (1)
	{
		if (SLT.is_gateway == false && SLT.espnow.gateway_added == false)
		{
			uint8_t request_add[10] = {'S', 'L', 'T', ADD_PEER}; 
			memcpy((uint8_t*)&request_add[4], SLT.espnow.my_addr, 6); 
			if (SLT.espnow.can_send  == true)
			{
				SLT.espnow.can_send  = false;
				ret = esp_now_send(SLT.gateway_addr, (uint8_t*)request_add, 10);
			
				if (ret != ESP_OK)
				{
					SLT.espnow.can_send  = true;
				}
			}
		}
		
		else
		{
			if (SLT.is_gateway == false || (SLT.is_gateway == true && SLT.espnow.tot_peer > 0))
			{
				if (SLT.espnow.can_send  == true)
				{
			
					if (SLT.espnow.sent == false)
					{
						/* retry send the last buffer */
					}
					else
					{
						/* send the next buffer */
						if (data != data_esp_now)
						{
							data = data_esp_now; 
							len = len_test_data_esp_now;
						}
						else
						{
							data = data_esp_now2; 
							len = len_test_data_esp_now2;
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
			
		}

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}


/* 
 *	store ssid and pass wifi then auto restart
 *	unlock by give semaphore in isr
 **/
void esp_recv_inf_wifi()
{ 
	gpio_set_level(LED_WIFI, 0);
	xSemaphoreTake(xRecvPassWifi, 0);
	while (1)
	{
		if (xSemaphoreTake(xRecvPassWifi, portMAX_DELAY) == pdTRUE)
		{
			gpio_set_level(LED_WIFI, 1);
			
			strcpy((char*)wifi_cred.ssid, tem_wifi_cred.ssid);
			strcpy((char*)wifi_cred.pass, tem_wifi_cred.pass);
			
			nvs_handle nvs;
			if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
				nvs_set_str(nvs, "ssid", wifi_cred.ssid);
				nvs_set_str(nvs, "pass", wifi_cred.pass);
				nvs_commit(nvs);
				nvs_close(nvs);
			}
		}
	}
}

/*
 *	Config info and try connect wifi STA
 **/
void task_wifi_sta() 
{
	xSemaphoreTake(xTryConnectWifi, 0);
	while (1)
	{
		if (xSemaphoreTake(xTryConnectWifi, portMAX_DELAY) == pdTRUE)
		{
			if (wifi_cred.retry_connect == MAX_RETRY_CONNECT)
			{
				strcpy((char*)tem_wifi_cred.ssid, wifi_cred.ssid);
				strcpy((char*)tem_wifi_cred.pass, wifi_cred.pass);
			}
			
			if (wifi_cred.retry_connect != MAX_RETRY_CONNECT)
			{
				strcpy((char*)tem_wifi_cred.ssid, tcp_wifi_cred.ssid);
				strcpy((char*)tem_wifi_cred.pass, tcp_wifi_cred.pass);
			}
			
			wifi_config_t sta_cfg = {0};
			strcpy((char*)sta_cfg.sta.ssid, tem_wifi_cred.ssid);
			strcpy((char*)sta_cfg.sta.password, tem_wifi_cred.pass);
			esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
			wifi_cred.retry_connect = 0;
			
			if (wifi_cred.is_connected == true) {
				esp_wifi_disconnect();
			}
			else 
				esp_wifi_connect();

		}
	}
}

/**
 *	@brief	x? lý yêu c?u c?a tcp v?i file
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
 *		stack ph?i ?? l?n ?? ch?y task này
 *		n?u file ch?a t?n t?i thì m? file v?i mode O_TRUNC tránh l?i
 *		n?u file t?n t?i thì không O_TRUNC tránh xóa file
 *		
 */

void task_tcp_file_bin()
{
	gpio_set_level(GPIO_NUM_15, 0);
	gpio_set_level(GPIO_NUM_0, 1);
	
	int fd = -100; 
	while (1)
	{
		if (xQueueReceive(xBuffLoadf, &SLT_server.recv.segment, portMAX_DELAY) == pdPASS)
		{	
			if (SLT_server.recv.segment.command == FORMAT)
			{
				if (SLT_server.recv.segment.data.content != NULL) 
				{
					free(SLT_server.recv.segment.data.content);
					SLT_server.recv.segment.data.content = NULL;
				}
				
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}
				
				esp_spiffs_format(NULL);
				spiffs_init(); 
			}
			
			else if (SLT_server.recv.segment.command == OPEN)
			{
				
				if (SLT_server.recv.segment.data.content != NULL) 
				{
					free(SLT_server.recv.segment.data.content);
					SLT_server.recv.segment.data.content = NULL;
				}
				
				if (fd < 0)
				{
					struct stat st;
					int ret = stat("/spiffs/data.bin", &st);
					fd = ret < 0 ?  open("/spiffs/data.bin", O_RDWR | O_CREAT | O_TRUNC, 0666) : open("/spiffs/data.bin", O_RDWR | O_CREAT, 0666);
					
				}
				

			}
			else if (SLT_server.recv.segment.command == CLOSE)
			{
				if (SLT_server.recv.segment.data.content != NULL) 
				{
					free(SLT_server.recv.segment.data.content);
					SLT_server.recv.segment.data.content = NULL;
				}
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}

			}			
			else if (SLT_server.recv.segment.command == DELETE)
			{
				if (SLT_server.recv.segment.data.content != NULL) 
				{
					free(SLT_server.recv.segment.data.content);
					SLT_server.recv.segment.data.content = NULL;
				}

				if (fd >= 0) 
				{
					close(fd);
					fd = -100;
				}
				unlink("/spiffs/data.bin");

			}
			
			else if (SLT_server.recv.segment.command == READ)
			{
				if (fd < 0)
				{
					if (SLT_server.recv.segment.data.content != NULL) 
					{
						free(SLT_server.recv.segment.data.content);
						SLT_server.recv.segment.data.content = NULL;
					}		
	//				gpio_set_level(GPIO_NUM_0, 0);
				}
				else
				{
	//				gpio_set_level(GPIO_NUM_0, 1);
					
					lseek(fd, SLT_server.recv.segment.pos_in_file, SEEK_SET); 
					uint32_t remaining = SLT_server.recv.segment.data.len;
					
					
					while (remaining > 0)
					{
						SLT_server.send.data.len = remaining <= 512 ? remaining : 512; 
						SLT_server.send.data.content = malloc(SLT_server.send.data.len); 
						
						if (SLT_server.send.data.content != NULL)
						{
							read(fd, SLT_server.send.data.content, SLT_server.send.data.len);
						
							xQueueSendToBack(xBuffSendf, &SLT_server.send, portMAX_DELAY);
							remaining = remaining - SLT_server.send.data.len;						
						}
						else
						{
							break;
						}
					}
					
					if (SLT_server.recv.segment.data.content != NULL) 
					{
						free(SLT_server.recv.segment.data.content);
						SLT_server.recv.segment.data.content = NULL;
					}
				}
				
				SLT_server.send.data.content = NULL;
				SLT_server.send.data.len = 0; 
				xQueueSendToBack(xBuffSendf, &SLT_server.send, portMAX_DELAY);
			}
			else if (SLT_server.recv.segment.command == WRITE)
			{
				
				if (fd < 0)
				{
					if (SLT_server.recv.segment.data.content != NULL)
					{
						free(SLT_server.recv.segment.data.content); 	
						SLT_server.recv.segment.data.content = NULL; 
					}	
//					gpio_set_level(GPIO_NUM_0, 0);
				}
				else
				{	
//					gpio_set_level(GPIO_NUM_0, 1);			
					if (SLT_server.recv.segment.data.content != NULL)
					{

					 
						if (SLT_server.recv.segment.pos_in_file == POS_CONTINUE)
						{
							lseek(fd, SLT_server.recv.current_pos_file, SEEK_SET);
						}
						else
						{
							lseek(fd, SLT_server.recv.segment.pos_in_file, SEEK_SET);
						}

						write(fd, &((uint8_t*)SLT_server.recv.segment.data.content)[SLT_server.recv.segment.pos_data], SLT_server.recv.segment.data.len);
						SLT_server.recv.current_pos_file = lseek(fd, 0, SEEK_CUR);
						free(SLT_server.recv.segment.data.content); 		
						SLT_server.recv.segment.data.content = NULL;
					
					}

				}
				
				int f = open("/spiffs/data.bin", O_RDONLY);
				if (f >= 0)
				{
				
					off_t size = lseek(f, 0, SEEK_END); 
				
					if (size > 45558 - 8)
					{
						char* tem = (char*)malloc(3);
					
						lseek(f, 45556 - 8, SEEK_SET);
				
						read(f, tem, 3);
						if (tem[0] == 65 && tem[1] == 66 && tem[2] == 67)
						{
	//						gpio_set_level(GPIO_NUM_15, 1);
						}
						else
						{
	//						gpio_set_level(GPIO_NUM_15, 0);				
						
						}
						free(tem);					
					}
					close(f);			
				}
			}
			else
			{
				if (SLT_server.recv.segment.data.content != NULL) 
				{
					free(SLT_server.recv.segment.data.content);
					SLT_server.recv.segment.data.content = NULL;
				}				
			}
			
			
		}
	}
}


