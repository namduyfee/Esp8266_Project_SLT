#include "pwm_timer1.h"


/********* global variables ***********/

uint32_t g_gpio_pwm_channel[] = {GPIO_NUM_14, GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_13, GPIO_NUM_5};
const uint32_t g_pwm_channel_len = sizeof(g_gpio_pwm_channel) / sizeof(g_gpio_pwm_channel[0]);
	
volatile uint32_t g_dutis[TOTAL_GPIO_MCU] = {0};

volatile uint32_t g_cnt_pwm = 0;

void config_Timer(void)
{
	// hw_timer_init is must first config
	
	hw_timer_init(timer_cb, NULL);
	hw_timer_set_clkdiv(TIMER_CLKDIV_16);
	hw_timer_set_load_data(2000);
	hw_timer_set_reload(true);
	hw_timer_set_intr_type(TIMER_EDGE_INT);
	hw_timer_enable(true);
	
	for (uint32_t i = 0; i < g_pwm_channel_len; i++)
	{
		g_dutis[g_gpio_pwm_channel[i]] = DEFAUL_DUTY; 
	}
	
}



