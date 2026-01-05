
#include "my_callback.h"

#include "my_pwm.h"
#include "esp_now_config.h"
#include "my_lib.h"


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
