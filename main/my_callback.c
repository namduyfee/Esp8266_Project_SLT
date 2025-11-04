
#include "my_callback.h"

extern uint32_t g_gpio_pwm_channel[];
extern const uint32_t g_pwm_channel_len;
void timer_cb(void* arg) {
	
	g_cnt_pwm = (g_cnt_pwm + 1) % RELOAD_DATA_PWM;
	
	for (uint32_t i = 0; i < g_pwm_channel_len; i++)
	{
		if (g_cnt_pwm < g_dutis[i])
		{
			gpio_set_level(g_gpio_pwm_channel[i], 1);
		}
		else
		{
			gpio_set_level(g_gpio_pwm_channel[i], 0);
		}
	}
	
}
