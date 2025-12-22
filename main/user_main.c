#include "my_lib.h"

#define MIN_DELAY 10

void esp_now_task(); 
void esp_recv_inf_wifi();
void esp_recv_file_bin();

void wifi_sta_task();



SemaphoreHandle_t xRecvPassWifi; 
SemaphoreHandle_t xTryConnectWifi; 

QueueHandle_t xBuffLoadf;

void app_main(void) {
	
	xRecvPassWifi = xSemaphoreCreateBinary();
	xTryConnectWifi = xSemaphoreCreateBinary();
	
	xBuffLoadf = xQueueCreate(10, sizeof(data_t));
	
	nvs_flash_init();
	spiffs_init();
	config_GPIO_OUT();
	config_input_pullup_gpio();

	
	tcpip_adapter_init();
	
//	config_espnow();
	
	start_wifi();
 
//	start_pwm();
	
	init_server_tpcp(80, 5);
	
//	xTaskCreate(esp_now_task, "esp_now_send_task", 1024, NULL, 4, NULL);
	
//	xTaskCreate(esp_recv_inf_wifi, "esp_recv_inf_wifi", 1024, NULL, 5, NULL);
	xTaskCreate(esp_recv_file_bin, "esp_recv_file_bin", 1024, NULL, 4, NULL);
	
//	xTaskCreate(wifi_sta_task, "wifi_sta_task", 1024, NULL, 4, NULL);


	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(MIN_DELAY));
	}
	
}

void esp_now_task() 
{
//	esp_err_t ret; 
	g_my_esp_now.can_send = true; 
	while (1)
	{
//		if (g_my_esp_now.can_send == true)
//		{
//
//			ret = esp_now_send(g_peer_esp8266.inf.peer_addr, data_esp_now, len_test_data_esp_now);
//			while (ret != ESP_OK)
//			{
//				ret = esp_now_send(g_peer_esp8266.inf.peer_addr, data_esp_now, len_test_data_esp_now);
//				vTaskDelay(pdMS_TO_TICKS(10));
//			}
//			g_my_esp_now.can_send = false;
//		}
		vTaskDelay(pdMS_TO_TICKS(10));
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
void wifi_sta_task() 
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

/*
 *	write file.bin from tcp use spiffs
 *	
 **/
void esp_recv_file_bin()
{
	gpio_set_level(GPIO_NUM_15, 0);
	
	while (1)
	{
		if (xQueueReceive(xBuffLoadf, &SLT_server.recv.segment, portMAX_DELAY) == pdPASS)
		{		
			
			int fd = open("/spiffs/data.bin", O_RDWR | O_CREAT, 0666);
			
			if (fd < 0)
			{
				if (SLT_server.recv.segment.content != NULL)
				{
					free(SLT_server.recv.segment.content); 	
//					gpio_set_level(GPIO_NUM_15, 1);
				}	
			}
			else
			{	
				 
				uint8_t processed = 0;				
				if (SLT_server.recv.segment.content != NULL)
				{
					/* code 0
					 
					if (SLT_server.recv.segment.pos_in_file == POS_CONTINUE)
					{
						lseek(fd, (off_t)SLT_server.recv.current_pos_file, SEEK_SET);
					}
					else
					{
						lseek(fd, (off_t)SLT_server.recv.segment.pos_in_file, SEEK_SET);
					}

					write(fd, &((uint8_t*)SLT_server.recv.segment.content)[SLT_server.recv.segment.pos_data], SLT_server.recv.segment.len);
					SLT_server.recv.current_pos_file = lseek(fd, 0, SEEK_CUR);
					free(SLT_server.recv.segment.content); 		
					
					*/
					
					
					/* code 1
					

					free(SLT_server.recv.segment.content); 	
					uint8_t a = 66; 
					lseek(fd, 0, SEEK_SET); 
					write(fd, &a, 1);
					*/

				
//					/* code 2
		
					
					int fd_tm = open("/spiffs/tm.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
					
					if (fd_tm < 0) {
						free(SLT_server.recv.segment.content);
						continue;
					}
					
					uint32_t pos_write; 
					
					if (SLT_server.recv.segment.pos_in_file == POS_CONTINUE)
					{
						pos_write = SLT_server.recv.current_pos_file; 
					}
					else
					{
						pos_write = SLT_server.recv.segment.pos_in_file;
					}		
					if (pos_write != 0)
					{
						uint8_t* tem = (uint8_t*)(malloc(pos_write < 512 ? pos_write : 512 ));
						
						lseek(fd, 0, SEEK_SET);
						lseek(fd_tm, 0, SEEK_SET);
						
						uint32_t remaining = pos_write; 
						
						while (remaining > 0)
						{
							uint32_t read_num = read(fd, tem, remaining < 512 ? remaining : 512);
							if (read_num > 0)
							{
								write(fd_tm, tem, read_num);
								remaining = remaining - read_num; 
							}
							else 
								break; 
						}
						free(tem); 
						
					}
					
					lseek(fd_tm, pos_write, SEEK_SET);	
					write(fd_tm, &((uint8_t*)SLT_server.recv.segment.content)[SLT_server.recv.segment.pos_data], SLT_server.recv.segment.len);
					
					SLT_server.recv.current_pos_file = lseek(fd_tm, 0, SEEK_CUR); 
					
					pos_write = lseek(fd_tm, 0, SEEK_CUR); 
					
					off_t file_size = lseek(fd, 0, SEEK_END);
					
					if (pos_write < file_size)
					{
						uint8_t* tem = (uint8_t*)(malloc(file_size - pos_write < 512 ? file_size - pos_write : 512));
						
						lseek(fd, pos_write, SEEK_SET);
						lseek(fd_tm, pos_write, SEEK_SET);
						
						uint32_t remaining = file_size - pos_write; 
						
						while (remaining > 0)
						{
							uint32_t read_num = read(fd, tem, remaining < 512 ? remaining : 512);
							
							if (read_num > 0)
							{
								write(fd_tm, tem, read_num);
								remaining = remaining - read_num; 
							}
							else 
								break; 
						}
						free(tem);						
					}
					
					free(SLT_server.recv.segment.content); 
					
					close(fd_tm);
					close(fd);
					
					unlink("/spiffs/data.bin"); 
					
					int ret = rename("/spiffs/tm.bin", "/spiffs/data.bin");
					
					if (ret == 0) {
						// complete
						
					}
					else {
						
					}
					processed = 1; 
//					*/
				}	
				if (processed == 0)
					close(fd);
//				close(fd);
			}
			int f = open("/spiffs/data.bin", O_RDONLY);
			if (f >= 0)
			{
				
				off_t size = lseek(f, 0, SEEK_END); 
				
				if (size > 2502 - 7)
				{
					char* tem = (char*)malloc(3);
					
					lseek(f, 2500 - 7, SEEK_SET);
					
					read(f, tem, 3);
					if (tem[0] == 65 && tem[1] == 66 && tem[2] == 67)
					{
						gpio_set_level(GPIO_NUM_15, 1);
					}
					else
					{
						gpio_set_level(GPIO_NUM_15, 0);				
						
					}
					free(tem);					
				}
				close(f);			
			}
			
			
				
		}
	}
}


