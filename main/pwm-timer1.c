#include "pwm_timer1.h"


/********* global variables ***********/

uint32_t g_gpio_pwm_channel[] = {PWM_PIN_GPIO14, PWM_PIN_GPIO2, PWM_PIN_GPIO4, PWM_PIN_GPIO5, PWM_PIN_GPIO13};
const uint32_t g_pwm_channel_len = sizeof(g_gpio_pwm_channel) / sizeof(uint32_t);
	
volatile uint32_t g_dutis[TOTAL_GPIO_MCU] = {0};

volatile uint32_t g_cnt_pwm = 0;


uint32_t precent_to_duty(uint32_t precent)
{
	if (precent == 0)
		return 0;
	
	return (uint32_t)((RELOAD_DATA_PWM * precent) / 100); 
}
void config_GPIO_PWM()
{
	uint32_t tem_pin_bit_mask = 0;
	
	for (uint32_t i = 0; i < g_pwm_channel_len; i++) {
		tem_pin_bit_mask |= (1UL << g_gpio_pwm_channel[i]);
	}
	
	gpio_config_t io_conf = {
			
		.pin_bit_mask = tem_pin_bit_mask,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&io_conf);
}

void config_Timer()
{
	// hw_timer_init is must first config
	
	hw_timer_init(timer_cb, NULL);
	hw_timer_set_clkdiv(TIMER_CLKDIV_1);
	hw_timer_set_load_data(800 - 1);
	hw_timer_set_reload(true);
	hw_timer_set_intr_type(TIMER_EDGE_INT);
	hw_timer_enable(true);
	
	for (uint32_t i = 0; i < g_pwm_channel_len; i++)
	{
		g_dutis[g_gpio_pwm_channel[i]] = DEFAUL_DUTY; 
	}
	
}



