
#include "wifi_config.h"
#include "esp_now_config.h"
#include "my_lib.h"

extern esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

void init_wifi(void)
{
	esp_event_loop_init(wifi_event_handler, NULL);
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
}

void my_start_wifi(wifi_t* wifi)
{
	init_wifi();
	
	esp_wifi_get_mac(ESP_IF_WIFI_STA, wifi->sta_macaddr); 
	
	struct stat st;
	int ret = stat("/spiffs/gateway.bin", &st);
	int fd = ret < 0 ?  open("/spiffs/gateway.bin", O_RDWR | O_CREAT | O_TRUNC, 0666) : 
						open("/spiffs/gateway.bin", O_RDWR | O_CREAT, 0666);
	if (fd >= 0)
	{
		uint32_t len = lseek(fd, 0, SEEK_END);
	
		if (len >= POS_ADDR_GATEWAY + 6)
		{
			lseek(fd, POS_ADDR_GATEWAY, SEEK_SET);
			read(fd, wifi->gateway_addr, 6);
			
			if (is_same_macadrr(wifi->gateway_addr, wifi->sta_macaddr))
			{
				wifi->is_gateway = true; 

				esp_wifi_set_mode(WIFI_MODE_APSTA);
				wifi_config_t ap_config = {
					.ap = {
					.ssid = "ESP_SLT",
					.ssid_len = 0,
					.max_connection = 4,
					.password = "12345678",
					.channel = CONFIG_ESPNOW_CHANNEL, 
					.authmode = WIFI_AUTH_WPA_WPA2_PSK		
					}	
				};
				esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);		
				esp_wifi_start();
				close(fd);
				return;
			}
		}
		close(fd);
	}
	
	wifi->is_gateway = false;
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_start();
	esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, 0);
	
}
