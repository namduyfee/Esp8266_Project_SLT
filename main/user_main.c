#include "pwm_timer1.h"


extern uint8_t esp32_macadrr[6];


void app_main(void) {
	
	
	config_GPIO_PWM();
	espnow_init();
//	config_Timer();

	gpio_set_level(PWM_PIN_GPIO2, 0);
	
	
//	uint8_t data_send_esp32[] = "hello esp32";
	

	while (1) {
		
//		esp_now_send(esp32_macadrr, data_send_esp32, sizeof(data_send_esp32));

		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

