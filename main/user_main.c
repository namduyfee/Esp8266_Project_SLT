#include "my_lib.h"



void esp_now_task(); 
void esp_recv_inf_wifi();
void esp_recv_file_bin();

void wifi_sta_task();

uint8_t data_esp_now[] = "hello from ESP8266";
uint8_t len_test_data_esp_now = sizeof(data_esp_now) / sizeof(data_esp_now[0]); 

uint8_t data_frame2[] = "i am frame 2025";
uint8_t len_test_data_2 = sizeof(data_frame2) / sizeof(data_frame2[0]); 


SemaphoreHandle_t xRecvPassWifi; 
SemaphoreHandle_t xTryConnectWifi; 

QueueHandle_t xBuffLoadf;

void app_main(void) {
	
	xRecvPassWifi = xSemaphoreCreateBinary();
	xTryConnectWifi = xSemaphoreCreateBinary();
	
	xBuffLoadf = xQueueCreate(10, sizeof(BufItem_Typedf));
	
	nvs_flash_init();
	spiffs_init();
	config_GPIO_OUT();
	config_input_pullup_gpio();

	
	tcpip_adapter_init();
	
//	config_espnow();
	
	start_wifi();
	
	my_init_tcpip();
	
	start_pwm();
	
	/* init wifi, tcp ... then start timer */
	
//	xTaskCreate(esp_now_task, "esp_now_send_task", 2048, NULL, 4, NULL);
	
	xTaskCreate(esp_recv_inf_wifi, "esp_recv_inf_wifi", 1024, NULL, 5, NULL);
	xTaskCreate(esp_recv_file_bin, "esp_recv_file_bin", 2048, NULL, 4, NULL);
	
	xTaskCreate(wifi_sta_task, "wifi_sta_task", 1024, NULL, 4, NULL);
	
	vTaskDelay(pdMS_TO_TICKS(100));
	
	Pwm.duty[0] = 200;
	Pwm.duty[1] = 300;
	Pwm.duty[2] = 400;
	Pwm.duty[3] = 700;
	Pwm.duty[4] = 600;
	Pwm.duty[5] = 250;
	Pwm.duty[6] = 350;
	Pwm.duty[7] = 450;

	set_duties_pwm();
	
//	pwm_set_duty(0, 200);
//	pwm_set_duty(1, 300);
//	pwm_set_duty(2, 400);
//	pwm_set_duty(3, 600);
//	pwm_set_duty(4, 700);
//	pwm_set_duty(5, 800);
//	pwm_set_duty(6, 900);
//	pwm_set_duty(7, 750);
//	pwm_start();
	
//	vTaskDelay(pdMS_TO_TICKS(1000));
//	pwm_set_duty(0, 250);
//	pwm_start();
//	pwm_set_duty(1, 350);
//	pwm_start();
//	pwm_set_duty(2, 450);
//	pwm_start();
//	pwm_set_duty(3, 750);
//	pwm_start();

	
	
	while (1)
	{

		vTaskDelay(pdMS_TO_TICKS(1));

	}
	
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


/* 
 *	store ssid and pass wifi then auto restart
 *	unlock by give semaphore in isr
 **/
void esp_recv_inf_wifi()
{ 
	gpio_set_level(LED_WIFI, 0);
	xSemaphoreTake(xRecvPassWifi, 0);
	while (1)
	{
		if (xSemaphoreTake(xRecvPassWifi, portMAX_DELAY) == pdTRUE)
		{
			gpio_set_level(LED_WIFI, 1);
			
			strcpy((char*)wifi_cred.ssid, tem_wifi_cred.ssid);
			strcpy((char*)wifi_cred.pass, tem_wifi_cred.pass);
			
			nvs_handle nvs;
			if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
				nvs_set_str(nvs, "ssid", wifi_cred.ssid);
				nvs_set_str(nvs, "pass", wifi_cred.pass);
				nvs_commit(nvs);
				nvs_close(nvs);
			}
		}
	}
}

/*
 *	Config info and try connect wifi STA
 **/
void wifi_sta_task() 
{
	xSemaphoreTake(xTryConnectWifi, 0);
	while (1)
	{
		if (xSemaphoreTake(xTryConnectWifi, portMAX_DELAY) == pdTRUE)
		{
			if (wifi_cred.retry_connect == MAX_RETRY_CONNECT)
			{
				strcpy((char*)tem_wifi_cred.ssid, wifi_cred.ssid);
				strcpy((char*)tem_wifi_cred.pass, wifi_cred.pass);
			}
			
			if (wifi_cred.retry_connect != MAX_RETRY_CONNECT)
			{
				strcpy((char*)tem_wifi_cred.ssid, tcp_wifi_cred.ssid);
				strcpy((char*)tem_wifi_cred.pass, tcp_wifi_cred.pass);
			}
			
			wifi_config_t sta_cfg = {0};
			strcpy((char*)sta_cfg.sta.ssid, tem_wifi_cred.ssid);
			strcpy((char*)sta_cfg.sta.password, tem_wifi_cred.pass);
			esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
			wifi_cred.retry_connect = 0;
			
			if (wifi_cred.is_connected == true) {
				esp_wifi_disconnect();
			}
			else 
				esp_wifi_connect();

		}
	}
}

/*
 *	write file.bin from tcp use spiffs
 *	
 **/
void esp_recv_file_bin()
{
	char tem[3] = {0, 0, 0};
	
	BufItem_Typedf tm_buf;
	
	while (1)
	{
		if (xQueueReceive(xBuffLoadf, &tm_buf, portMAX_DELAY) == pdPASS)
		{
			int fd = open("/spiffs/upload.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
			
			if (fd < 0)
			{
		
			}
			else 
			{
				write(fd, tm_buf.payload, tm_buf.len);
				free(tm_buf.payload);
				close(fd);
			}
			int f = open("/spiffs/upload.bin", O_RDONLY, 0666);
			if (f >= 0)
			{
				read(f, tem, sizeof(tem));
		
				//	spiffs_read_file("/spiffs/hello.bin", tem, sizeof(tem));
				if (tem[0] == 66 && tem[1] == 66 && tem[2] == 66)
				{
					gpio_set_level(GPIO_NUM_2, 1); 
					gpio_set_level(GPIO_NUM_4, 1);
				}
				else
				{
					gpio_set_level(GPIO_NUM_2, 0); 
					gpio_set_level(GPIO_NUM_4, 0);				
					
				}
				close(f);			
			}
		}
	}
}


