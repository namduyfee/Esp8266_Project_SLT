
#include "my_callback.h"

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
	// r : send index frame and request my mcu send
	// s : data send to my mcu
	
	uint8_t tem_state = data[0];
	
	if (tem_state == 'r')
	{
		for (int i = 0; i < g_len_peer; i++)
		{
			if (is_same_macadrr(mac_addr, (*g_peer[i]).inf.peer_addr) == true)
			{
				(*g_peer[i]).total_request = len - 1;	// if data only has a byte (r/s) <=> (*g_peer[i]).total_request = 1 - 1; // not has request
				
				for (int j = 1; j < len; j++) {
					(*g_peer[i]).request[j - 1] = data[j];
				}
				break;
			}
		}
	}
	else if (tem_state == 's')
	{
		
		
	}
//	ESP_LOGI("ESPNOW", "Received data from " MACSTR ", len=%d", MAC2STR(mac_addr), len);
}

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	if (is_same_macadrr(mac_addr, g_peer_esp32.inf.peer_addr) == true)
	{
			
	}
	//	ESP_LOGI("ESPNOW", "Send to " MACSTR ", status=%d", MAC2STR(mac_addr), status);
//	state_esp_now_send = 1;
}
