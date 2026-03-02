
#include "my_callback.h"

#include "my_pwm.h"
#include "esp_now_config.h"
#include "my_lib.h"


esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
		
	case SYSTEM_EVENT_AP_START: {
		xSemaphoreGive(xWifiAPStart); 
		break;
		}
		
	case SYSTEM_EVENT_STA_START: {
		xSemaphoreGive(xWifiSTAStart); 
		break;
		}
		
//	case SYSTEM_EVENT_STA_GOT_IP: {	
//		break;
//	}
//    
//	case SYSTEM_EVENT_STA_DISCONNECTED: {
//		break;
//	}
        
	default:
		break;
	}
            
	return ESP_OK;
}
