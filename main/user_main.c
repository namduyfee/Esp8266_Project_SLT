#include "pwm_timer1.h"

void app_main(void) {
	
	
	config_GPIO_PWM();
	config_Timer();
	
	
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

