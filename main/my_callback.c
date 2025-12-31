
#include "my_callback.h"

#include "my_pwm.h"
#include "esp_now_config.h"
#include "my_lib.h"

void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) 
{
	if (data[0] == 'S' && data[1] == 'L' && data[2] == 'T')
	{
		if (data[3] == ADD_PEER)
		{
			espnow_add_peer((uint8_t*)&data[4]); 
		}
		
		else if (data[3] == ESPNOW_WRITE)
		{
			if (data[4] == 'a')
			{
				set_duty_pwm(&SLT.Pwm, 0, 300);
			}	
			else if (data[4] == 'b')
			{
				set_duty_pwm(&SLT.Pwm, 0, 800);
			}
		}
	}
}

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
	
	if (status == ESP_NOW_SEND_SUCCESS) {
		
		if (SLT.espnow.gateway_added == false && is_same_macadrr(mac_addr, SLT.gateway_addr))
		{
			SLT.espnow.gateway_added = true;
		}
		else 
			SLT.espnow.sent = true; 
		set_duty_pwm(&SLT.Pwm, 1, 550);
	}
	/*
	if (is_same_macadrr(mac_addr, g_peer_esp32.inf.peer_addr) == true)
	{
			
	}
	*/
	
	SLT.espnow.can_send = true;
}


esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
		
	case SYSTEM_EVENT_AP_START: {
		
		break;
		}
		
	case SYSTEM_EVENT_STA_START: {

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
