#include "my_lib.h"

uint8_t data_esp_now [] = "xhello from ESP8266";
uint8_t len_test_data_esp_now = sizeof(data_esp_now) / sizeof(data_esp_now[0]); 

uint8_t data_frame2 [] = "i am frame 2";
uint8_t len_test_data_2 = sizeof(data_frame2) / sizeof(data_frame2[0]); 

void app_main(void) {
	
	
	config_GPIO_PWM();
	config_espnow();
	config_Timer();
	
//	send_to_all_peer(data_esp_nowp_now, len_test_data_esp_now);
	
	while (1) {
		check_send_request();
		vTaskDelay(pdMS_TO_TICKS(10)); 
	}
}
