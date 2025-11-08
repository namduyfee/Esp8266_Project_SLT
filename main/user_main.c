#include "my_lib.h"

uint8_t data_esp_now [] = "hello from ESP8266";
uint8_t len_test_data_esp_now = sizeof(data_esp_now) / sizeof(data_esp_now[0]); 

uint8_t data_frame2 [] = "i am frame 2025";
uint8_t len_test_data_2 = sizeof(data_frame2) / sizeof(data_frame2[0]); 

void esp_now_task();

void app_main(void) {
	
	
	config_GPIO_PWM();
	config_espnow();
	config_Timer();
	
	xTaskCreate(esp_now_task, "esp_now_send_task", 4096, NULL, 5, NULL);
	
	while (1) {
	//	esp_now_send(NULL, data_esp_now, len_test_data_esp_now);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void esp_now_task() 
{
	while (1)
	{
		if (g_my_esp_now.start == 1)
			send_esp_now();
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

