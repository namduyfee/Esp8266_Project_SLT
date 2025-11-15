#include "my_lib.h"

uint8_t data_esp_now[] = "hello from ESP8266";
uint8_t len_test_data_esp_now = sizeof(data_esp_now) / sizeof(data_esp_now[0]); 

uint8_t data_frame2[] = "i am frame 2025";
uint8_t len_test_data_2 = sizeof(data_frame2) / sizeof(data_frame2[0]); 

void esp_now_task();


void app_main(void) {
	
	
	config_GPIO_PWM();
	config_espnow();
//	config_Timer();
//	g_dutis[PWM_PIN_GPIO2] = 70;
	gpio_set_level(PWM_PIN_GPIO2, 0); 
	gpio_set_level(PWM_PIN_GPIO4, 0);
	
	xTaskCreate(esp_now_task, "esp_now_send_task", 2048, NULL, 4, NULL);
	nvs_open()
	while (1) {
			
		vTaskDelay(pdMS_TO_TICKS(100));
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
