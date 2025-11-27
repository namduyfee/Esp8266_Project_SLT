
#include "my_callback.h"

volatile uint32_t cnt1 = 0;
volatile uint32_t cnt2 = 0;
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
//	if(is_same_macadrr(mac_addr, g_peer_esp32.inf.peer_addr) == true) // && 
	// && data[0] == 's' && data[1] == 't' && data[2] == 'a' && data[3] == 'r' && data[4] == 't'
//	gpio_set_level(PWM_PIN_GPIO2, 1);
	if(cnt1 > 80)
		gpio_set_level(GPIO_NUM_4, 1);
	if (is_same_macadrr(mac_addr, g_peer_esp8266.inf.peer_addr) == true && data[0] == 'h' && data[1] == 'e' && data[2] == 'l' && data[3] == 'l' && data[4] == 'o') {
		g_my_esp_now.start = 1; 
		if (cnt1 < 100)
			cnt1++;
	}
//	ESP_LOGI("ESPNOW", "Received data from " MACSTR ", len=%d", MAC2STR(mac_addr), len);
}

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	if(cnt2 > 80)
		gpio_set_level(GPIO_NUM_2, 1);
	g_my_esp_now.can_send = true;
	if (status == ESP_NOW_SEND_SUCCESS) {
		if (cnt2 < 100)
			cnt2++;
	}
	/*
	if (is_same_macadrr(mac_addr, g_peer_esp32.inf.peer_addr) == true)
	{
			
	}
	*/
	
	//	ESP_LOGI("ESPNOW", "Send to " MACSTR ", status=%d", MAC2STR(mac_addr), status);
//	state_esp_now_send = 1;
}


esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START: {
		if (wifi_cred.is_start_empty == false) {
			esp_wifi_connect();
		}
		
		break;
	}
        
	case SYSTEM_EVENT_STA_GOT_IP: {
		
		wifi_cred.is_connected = true;
		
		strcpy((char*)wifi_cred.ssid, tem_wifi_cred.ssid);
		strcpy((char*)wifi_cred.pass, tem_wifi_cred.pass);
		
		
		xSemaphoreGive(xRecvPassWifi);
		
		break;
	}
    
	case SYSTEM_EVENT_STA_DISCONNECTED: {

		if (wifi_cred.retry_connect  < MAX_RETRY_CONNECT)
		{
			esp_wifi_connect(); // reconnect
			wifi_cred.retry_connect++;
		}
		else
		{
				
			wifi_cred.retry_connect = 0;
				
			if (wifi_cred.is_start_empty == false)
			{
					
				strcpy((char*)tem_wifi_cred.ssid, wifi_cred.ssid);
				strcpy((char*)tem_wifi_cred.pass, wifi_cred.pass);
				wifi_config_t sta_cfg = {0};
				strcpy((char*)sta_cfg.sta.ssid, tem_wifi_cred.ssid);
				strcpy((char*)sta_cfg.sta.password, tem_wifi_cred.pass);
				esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
				esp_wifi_connect();
					
			}
		}
	

		wifi_cred.is_connected = false;
		
		break;
	}
        
	default:
		break;
	}
            
	return ESP_OK;
}
