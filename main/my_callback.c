
#include "my_callback.h"

void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) 
{
	
	if (is_same_macadrr(mac_addr, g_peer_esp8266.inf.sta.peer_addr) == true)
	{
		if (data[0] == 'S' && data[1] == 'L' && data[2] == 'T')
		{
			set_duty_pwm(0, 800);
		}			
	}

//	ESP_LOGI("ESPNOW", "Received data from " MACSTR ", len=%d", MAC2STR(mac_addr), len);
}

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	g_my_esp_now.can_send = true;
	if (status == ESP_NOW_SEND_SUCCESS) {
		set_duty_pwm(1, 500);
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
	case SYSTEM_EVENT_AP_START: {
		
		break;
		}
		
//	case SYSTEM_EVENT_STA_START: {
////		if (wifi_cred.last_available == true) {
//////			esp_wifi_connect();
////		}
////		
//		break;
//	}
//        
//	case SYSTEM_EVENT_STA_GOT_IP: {
//		
//		wifi_cred.last_available = true;
//		wifi_cred.is_connected = true;
//		
//		xSemaphoreGive(xRecvPassWifi);
//		
//		break;
//	}
//    
//	case SYSTEM_EVENT_STA_DISCONNECTED: {
//		
//		wifi_cred.is_connected = false;
//
//		if (wifi_cred.retry_connect  < MAX_RETRY_CONNECT)
//		{
//			esp_wifi_connect(); // reconnect
//			wifi_cred.retry_connect++;
//		}
//		else if(wifi_cred.retry_connect == MAX_RETRY_CONNECT)
//		{
//				
//			if (wifi_cred.last_available == true)
//			{
//				xSemaphoreGive(xTryConnectWifi);
//			}
//		}
//		
//		break;
//	}
        
	default:
		break;
	}
            
	return ESP_OK;
}
