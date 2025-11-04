#include "pwm_timer1.h"


/********* global variables ***********/

uint32_t g_gpio_pwm_channel [] = {GPIO_PWM_CHANNEL0, GPIO_PWM_CHANNEL1, GPIO_PWM_CHANNEL2, GPIO_PWM_CHANNEL3, GPIO_PWM_CHANNEL4};
volatile uint32_t g_dutis[MAX_CHANNEL] = {0};

volatile uint32_t g_cnt_pwm = 0;


//Chỗ này em nên dùng 1 câu lệnh để set tất cả GPIO cùng lúc.
//Vì nếu như thế này khoảng cách bật tắt giữa các kênh có khoảng lệch
void timer_cb(void* arg) {
	
	g_cnt_pwm = (g_cnt_pwm + 1) % RELOAD_DATA_PWM;
	
	for (uint32_t i = 0; i < (sizeof(g_gpio_pwm_channel) / sizeof(uint32_t)); i++)
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

uint32_t precent_to_duty(uint32_t precent)
{
	if (precent == 0)
		return 0;
	
	return (uint32_t)((RELOAD_DATA_PWM * precent) / 100); 
}
void config_GPIO_PWM()
{
	uint32_t tem_pin_bit_mask = 0;
	
	for (uint32_t i = 0; i < (sizeof(g_gpio_pwm_channel) / sizeof(uint32_t)); i++) {
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
	for (uint32_t i = 0; i < (sizeof(g_gpio_pwm_channel) / sizeof(uint32_t)); i++)
	{
		g_dutis[i] = DEFAUL_DUTY; 
	}
	
}

