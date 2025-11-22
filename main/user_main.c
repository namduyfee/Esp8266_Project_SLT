#include "my_lib.h"


void esp_now_task();
void esp_reset_wifi(); 
void esp_recv_inf_wifi();
void esp_recv_file_bin();

uint8_t data_esp_now[] = "hello from ESP8266";
uint8_t len_test_data_esp_now = sizeof(data_esp_now) / sizeof(data_esp_now[0]); 

uint8_t data_frame2[] = "i am frame 2025";
uint8_t len_test_data_2 = sizeof(data_frame2) / sizeof(data_frame2[0]); 


SemaphoreHandle_t xRecvPassWifi; 

void app_main(void) {
	
	xRecvPassWifi = xSemaphoreCreateBinary();
	nvs_flash_init();
	config_GPIO_OUT();
	config_input_pullup_gpio();
	config_GPIO_PWM();
	gpio_set_level(GPIO_NUM_2, 0); 
	gpio_set_level(GPIO_NUM_4, 0);
	
//	config_espnow();
//	config_Timer();
	spiffs_init();
	start_wifi();
	

	
//	xTaskCreate(esp_now_task, "esp_now_send_task", 2048, NULL, 4, NULL);
	
	xTaskCreate(esp_reset_wifi, "esp_reset_wifi", 1024, NULL, 4, NULL);
	xTaskCreate(esp_recv_inf_wifi, "esp_recv_inf_wifi", 1024, NULL, 5, NULL);
	xTaskCreate(esp_recv_file_bin, "esp_recv_file_bin", 2048, NULL, 4, NULL);
	
	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
}

void esp_now_task() 
{
	esp_err_t ret; 
	g_my_esp_now.can_send = true; 
	while (1)
	{
		if (g_my_esp_now.can_send == true)
		{

			ret = esp_now_send(g_peer_esp8266.inf.peer_addr, data_esp_now, len_test_data_esp_now);
			while (ret != ESP_OK)
			{
				ret = esp_now_send(g_peer_esp8266.inf.peer_addr, data_esp_now, len_test_data_esp_now);
				vTaskDelay(pdMS_TO_TICKS(10));
			}
			g_my_esp_now.can_send = false;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/*
 *	use a button to reset ssid and pass wifi then auto restart
 **/
void esp_reset_wifi()
{
	gpio_set_level(LED_WIFI, 0);
	while (1)
	{
		
		if (STATE_RESET_WIFI_BUT == IS_RESET_WIFI)   
		{
			int count = 0;

			while (STATE_RESET_WIFI_BUT == IS_RESET_WIFI)
			{
				int state = gpio_get_level(LED_WIFI);
				gpio_set_level(LED_WIFI, !state);
				vTaskDelay(pdMS_TO_TICKS(100));
				count++;
				
				if (count >= 30)   
					break;
			}
			gpio_set_level(LED_WIFI, 0);
			if (count >= 30)
			{
				gpio_set_level(LED_WIFI, 1);
				// XÓA NVS
				nvs_handle nvs;
				if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK)
				{
					nvs_erase_all(nvs);
					nvs_commit(nvs);
					nvs_close(nvs);
				}

				vTaskDelay(pdMS_TO_TICKS(1000));
				esp_restart();
			}
		}
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

			wifi_config_t sta_cfg = {0};
			strcpy((char*)sta_cfg.sta.ssid, wifi_cred.ssid);
			strcpy((char*)sta_cfg.sta.password, wifi_cred.pass);
			esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
			esp_wifi_connect();
			gpio_set_level(LED_WIFI, 0);
			
			vTaskDelay(pdMS_TO_TICKS(3000));
			if (wifi_cred.is_connect == true)
			{
				gpio_set_level(LED_WIFI, 1);
				nvs_handle nvs;
				if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
					nvs_set_str(nvs, "ssid", wifi_cred.ssid);
					nvs_set_str(nvs, "pass", wifi_cred.pass);
					nvs_commit(nvs);
					nvs_close(nvs);
				}
				vTaskDelay(pdMS_TO_TICKS(1000));
				esp_restart();
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void esp_recv_file_bin()
{
	int tem[3] = {0, 0, 0};
	
	while (1)
	{
		int f = open("/spiffs/upload.bin", O_RDONLY, 0666);
		
		if (f >= 0)
		{
			read(f, tem, 3);
		
			//	spiffs_read_file("/spiffs/hello.bin", tem, sizeof(tem));
			if (tem[0] == 1 && tem[1] == 0 && tem[2] == 0)
			{
				gpio_set_level(GPIO_NUM_2, 1); 
				gpio_set_level(GPIO_NUM_4, 1);
			}
			close(f);			
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

