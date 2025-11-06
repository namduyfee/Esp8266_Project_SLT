#include "my_lib.h"

void app_main(void) {
	
	
	config_GPIO_PWM();
	config_espnow();
	config_Timer();
	uint8_t data_esp_now[] = "hello from ESP8266";
	while (1) {

		esp_now_send(g_peer_esp32.peer_addr, data_esp_now, sizeof(data_esp_now));
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

