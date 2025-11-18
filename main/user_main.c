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
	
	config_input_pullup_gpio();
	config_GPIO_PWM();
//	config_espnow();
//	config_Timer();
//	spiffs_init();
	start_wifi();

	gpio_set_level(GPIO_NUM_2, 0); 
	gpio_set_level(GPIO_NUM_4, 0);
	
//	xTaskCreate(esp_now_task, "esp_now_send_task", 2048, NULL, 4, NULL);
	
//	xTaskCreate(esp_reset_wifi, "esp_reset_wifi", 2048, NULL, 4, NULL);
//	xTaskCreate(esp_recv_inf_wifi, "esp_recv_inf_wifi", 2048, NULL, 4, NULL);
//	xTaskCreate(esp_recv_file_bin, "esp_recv_file_bin", 1024, NULL, 5, NULL);
	
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
	while (1)
	{

		for (int i = 0; i < 100; i++)
		{
			if (RESET_WIFI_BUT != IS_RESET_WIFI)
				goto NOT_RESET;
			vTaskDelay(pdMS_TO_TICKS(30));
		}
		nvs_handle nvs;
		if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK)
		{
			nvs_erase_all(nvs);
			nvs_commit(nvs);
			nvs_close(nvs);
		}
		vTaskDelay(pdMS_TO_TICKS(2000));
		esp_restart();
		
		
		NOT_RESET:
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

/* 
 *	store ssid and pass wifi then auto restart
 *	unlock by give semaphore in isr
 **/
void esp_recv_inf_wifi()
{ 
	xSemaphoreTake(xRecvPassWifi, 0);
	while (1)
	{
		if (xSemaphoreTake(xRecvPassWifi, portMAX_DELAY) == pdTRUE)
		{
			nvs_handle nvs;
			if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
				nvs_set_str(nvs, "ssid", wifi_cred.ssid);
				nvs_set_str(nvs, "pass", wifi_cred.pass);
				nvs_commit(nvs);
				nvs_close(nvs);
			}
			vTaskDelay(pdMS_TO_TICKS(2000));
			esp_restart();
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void esp_recv_file_bin()
{
	char tem[3] = {0, 0, 0};

	while (1)
	{
		spiffs_read_file("/spiffs/data.bin", tem, 3);
		if (tem[0] == 11 && tem[1] == 20 && tem[2] == 41)
		{
			gpio_set_level(GPIO_NUM_2, 1); 
			gpio_set_level(GPIO_NUM_4, 1);
		}
			
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

