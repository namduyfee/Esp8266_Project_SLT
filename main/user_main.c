#include "my_lib.h"


void esp_now_task();
void esp_reset_wifi(); 
void esp_read_http();


uint8_t data_esp_now[] = "hello from ESP8266";
uint8_t len_test_data_esp_now = sizeof(data_esp_now) / sizeof(data_esp_now[0]); 

uint8_t data_frame2[] = "i am frame 2025";
uint8_t len_test_data_2 = sizeof(data_frame2) / sizeof(data_frame2[0]); 

QueueHandle_t http_queue;


void app_main(void) {
	
	config_input_pullup_gpio();
	
	config_GPIO_PWM();
//	config_espnow();
//	config_Timer();
	
	start_wifi();
	
	gpio_set_level(GPIO_NUM_2, 0); 
	gpio_set_level(GPIO_NUM_4, 0);
	http_queue = xQueueCreate(3, sizeof(httpd_req_t*));
//	xTaskCreate(esp_now_task, "esp_now_send_task", 2048, NULL, 4, NULL);
	
	xTaskCreate(esp_reset_wifi, "esp_reset_wifi", 2048, NULL, 4, NULL);
	xTaskCreate(esp_read_http, "esp_read_http", 4096, NULL, 5, NULL);
	
}

void esp_now_task() 
{
	esp_err_t ret; 
	g_my_esp_now.can_send = true; 
	while (1)
	{
		if (g_my_esp_now.can_send == true)
		{

			ret = esp_now_send(g_peer_esp8266.inf.peer_addr, data_esp_now, len_test_data_esp_now);
			while (ret != ESP_OK)
			{
				ret = esp_now_send(g_peer_esp8266.inf.peer_addr, data_esp_now, len_test_data_esp_now);
				vTaskDelay(pdMS_TO_TICKS(10));
			}
			g_my_esp_now.can_send = false;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void esp_reset_wifi()
{
	while (1)
	{

		for (int i = 0; i < 100; i++)
		{
			if (RESET_WIFI_BUT != IS_RESET_WIFI)
				goto NOT_RESET;
			vTaskDelay(pdMS_TO_TICKS(30));
		}
		nvs_handle nvs;
		if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK)
		{
			nvs_erase_all(nvs);
			nvs_commit(nvs);
			nvs_close(nvs);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
		esp_restart();
		
		
		NOT_RESET:
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void esp_read_http()
{
	
	httpd_req_t *req;
	
	while (1)
	{
		if (xQueueReceive(http_queue, &req, portMAX_DELAY) == pdPASS)
		{
			char buf[128];           
			int remaining = req->content_len;
			char body[512] = {0};   
			int idx = 0;

			while (remaining > 0 && idx < sizeof(body) - 1) {
				int to_read = remaining;
				if (to_read > sizeof(buf))
					to_read = sizeof(buf);

				int ret = httpd_req_recv(req, buf, to_read);
				if (ret <= 0) {
				
				}

				// Copy vào body
				memcpy(body + idx, buf, ret);
				idx += ret;
				remaining -= ret;
			}

			body[idx] = '\0'; // k?t thúc chu?i			
							// G?i response
			const char *resp = "Saved! Rebooting...";
			httpd_resp_send(req, resp, strlen(resp));
			wifi_mode_t wifi_mode;
			esp_wifi_get_mode(&wifi_mode);
			
			if (wifi_mode == WIFI_MODE_AP)
			{
				// Parse ssid và pass
				sscanf(body, "ssid=%31[^&]&pass=%63s", wifi_cred.ssid, wifi_cred.pass);

				// Save vào NVS
				nvs_handle nvs;
				if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
					nvs_set_str(nvs, "ssid", wifi_cred.ssid);
					nvs_set_str(nvs, "pass", wifi_cred.pass);
					nvs_commit(nvs);
					nvs_close(nvs);
				}



				vTaskDelay(2000 / portTICK_PERIOD_MS);
				esp_restart();
			}
			
		}
		vTaskDelay(pdMS_TO_TICKS(1));
		
	}
	
}

