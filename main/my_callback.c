
#include "my_callback.h"

extern uint32_t g_gpio_pwm_channel[];
extern const uint32_t g_pwm_channel_len;

void timer_cb(void* arg) {
	

	uint32_t tem_mask = GPIO_REG_READ(GPIO_OUT_ADDRESS);
	
	g_cnt_pwm = (g_cnt_pwm + 1) % RELOAD_DATA_PWM;
	
	for (uint32_t i = 0; i < g_pwm_channel_len; i++)
	{
		if (g_cnt_pwm < g_dutis[g_gpio_pwm_channel[i]])
			tem_mask |= (1 << g_gpio_pwm_channel[i]);
		else
			tem_mask &= (~(1 << g_gpio_pwm_channel[i]));
	}
	GPIO_REG_WRITE(GPIO_OUT_ADDRESS, tem_mask);
}


void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) 
{
//	gpio_set_level(PWM_PIN_GPIO2, 1);
//	ESP_LOGI("ESPNOW", "Received data from " MACSTR ", len=%d", MAC2STR(mac_addr), len);
}

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
//	ESP_LOGI("ESPNOW", "Send to " MACSTR ", status=%d", MAC2STR(mac_addr), status);
}