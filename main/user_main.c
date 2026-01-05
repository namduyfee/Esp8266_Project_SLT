#include "my_lib.h"
#include "string.h"

#define MIN_DELAY 10

void task_esp_now(); 
void esp_recv_inf_wifi();
void task_tcp_file_bin(); 
void task_select_master(); 
void task_wifi_sta(); 

Object SLT = {
	.Pwm = {
		.gpio_channel = {
		GPIO_CHANNEL_0,
		GPIO_CHANNEL_1,
		GPIO_CHANNEL_2,
		GPIO_CHANNEL_3,
		GPIO_CHANNEL_4,
		GPIO_CHANNEL_5,
		GPIO_CHANNEL_6,
		GPIO_CHANNEL_7
		},
		.duty = {0, 0, 0, 0, 0, 0, 0, 0},
		.num_channel_en = 0
	},
	
	.espnow = {
		.can_send	= true,
		.p_peer		= NULL,
		.sent		= false,
		.tot_peer   = 0
	}
		
};

QueueHandle_t xBuffLoadf;
QueueHandle_t xBuffSendf;

void my_init_project(void)
{
	
	nvs_flash_init();
	spiffs_init();
	config_GPIO_OUT();
	config_input_pullup_gpio();
	
	my_pwm_start(&SLT.Pwm); 
	
	tcpip_adapter_init();
	my_start_wifi();
	
	init_server_tpcp(80, 5);
	init_espnow();
}


void app_main(void) {
	
	
	my_init_project();
	
	xBuffLoadf = xQueueCreate(30, sizeof(tcp_recv_t));
	
	xBuffSendf = xQueueCreate(10, sizeof(tcp_data_t)); 
	
	
	xTaskCreate(task_esp_now, "esp_now_task", 1024, NULL, 4, NULL);
	
	xTaskCreate(task_tcp_file_bin, "esp_recv_file_bin", 2048, NULL, 4, NULL);
	
	xTaskCreate(task_select_master, "task_select_master", 1024, NULL, 4, NULL);
		 
	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	
}

void task_select_master()
{
	
	while (1)
	{
		if(gpio_get_level(BUT_SEL_MASTER) == PRES_SEL_MASTER)   
		{
			int count = 0;

			while (gpio_get_level(BUT_SEL_MASTER) == PRES_SEL_MASTER)
			{
				vTaskDelay(pdMS_TO_TICKS(10));
				count++;

				if (count >= 100)   
					break;
			}

			if (count >= 100)
			{
				/* handler when button is pressed */
				
			}
		}		
		
	}
	
	
}
/**
 *	@brief	
 */
void task_esp_now() 
{
	uint8_t data_esp_now[5] = {'S', 'L', 'T'}; data_esp_now[3] = ESPNOW_WRITE; data_esp_now[4] = 'a'; 
	uint32_t len_test_data_esp_now = sizeof(data_esp_now) / sizeof(data_esp_now[0]);

	uint8_t data_esp_now2[5] = {'S', 'L', 'T'}; data_esp_now2[3] = ESPNOW_WRITE; data_esp_now2[4] = 'b';
	uint32_t len_test_data_esp_now2 = sizeof(data_esp_now2) / sizeof(data_esp_now2[0]);
	
	esp_err_t ret; 
	SLT.espnow.can_send = true; 
	SLT.espnow.sent = false;
	
	void* data = data_esp_now;
	uint32_t len = len_test_data_esp_now;
	uint8_t broadcard[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
	while (1)
	{

		if (SLT.espnow.can_send  == true)
		{
			
			if (SLT.espnow.sent == false)
			{
				/* retry send the last buffer */
			}
			else
			{
				/* send the next buffer */
				if (data != data_esp_now)
				{
					data = data_esp_now; 
					len = len_test_data_esp_now;
				}
				else
				{
					data = data_esp_now2; 
					len = len_test_data_esp_now2;
				}
				SLT.espnow.sent = false; 

			}
			SLT.espnow.can_send  = false;
			
			ret = esp_now_send(broadcard, (uint8_t*)data, len);
			if (ret != ESP_OK)
			{
				SLT.espnow.can_send  = true;
			}
		}				
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/**
 *	@brief	handler request from tcp
 *	
 *	@details	
 *		- format
 *		- open
 *		- close
 *		- delete
 *		- read
 *		- write
 *		
 *	@note
 *		
 */

void task_tcp_file_bin()
{
	gpio_set_level(GPIO_NUM_15, 0);
	gpio_set_level(GPIO_NUM_0, 1);
	
	int fd = -100; 
	while (1)
	{
		if (xQueueReceive(xBuffLoadf, &SLT.server.recv.segment, portMAX_DELAY) == pdPASS)
		{	
			if (SLT.server.recv.segment.command == FORMAT)
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}
				
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}
				
				esp_spiffs_format(NULL);
				spiffs_init(); 
			}
			
			else if (SLT.server.recv.segment.command == OPEN)
			{
				
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}
				
				if (fd < 0)
				{
					struct stat st;
					int ret = stat("/spiffs/data.bin", &st);
					fd = ret < 0 ?  open("/spiffs/data.bin", O_RDWR | O_CREAT | O_TRUNC, 0666) : open("/spiffs/data.bin", O_RDWR | O_CREAT, 0666);
					
				}
				

			}
			else if (SLT.server.recv.segment.command == CLOSE)
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}
				if (fd >= 0)
				{
					close(fd);
					fd = -100;					
				}

			}			
			else if (SLT.server.recv.segment.command == DELETE)
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}

				if (fd >= 0) 
				{
					close(fd);
					fd = -100;
				}
				unlink("/spiffs/data.bin");

			}
			
			else if (SLT.server.recv.segment.command == READ)
			{
				if (fd < 0)
				{
					if (SLT.server.recv.segment.data.content != NULL) 
					{
						free(SLT.server.recv.segment.data.content);
						SLT.server.recv.segment.data.content = NULL;
					}		
				}
				else
				{
					
					lseek(fd, SLT.server.recv.segment.pos_in_file, SEEK_SET); 
					uint32_t remaining = SLT.server.recv.segment.data.len;
					
					
					while (remaining > 0)
					{
						SLT.server.send.data.len = remaining <= 512 ? remaining : 512; 
						SLT.server.send.data.content = malloc(SLT.server.send.data.len); 
						
						if (SLT.server.send.data.content != NULL)
						{
							read(fd, SLT.server.send.data.content, SLT.server.send.data.len);
						
							xQueueSendToBack(xBuffSendf, &SLT.server.send, portMAX_DELAY);
							remaining = remaining - SLT.server.send.data.len;						
						}
						else
						{
							break;
						}
					}
					
					if (SLT.server.recv.segment.data.content != NULL) 
					{
						free(SLT.server.recv.segment.data.content);
						SLT.server.recv.segment.data.content = NULL;
					}
				}
				
				SLT.server.send.data.content = NULL;
				SLT.server.send.data.len = 0; 
				xQueueSendToBack(xBuffSendf, &SLT.server.send, portMAX_DELAY);
			}
			else if (SLT.server.recv.segment.command == WRITE)
			{
				
				if (fd < 0)
				{
					if (SLT.server.recv.segment.data.content != NULL)
					{
						free(SLT.server.recv.segment.data.content); 	
						SLT.server.recv.segment.data.content = NULL; 
					}	
				}
				else
				{				
					if (SLT.server.recv.segment.data.content != NULL)
					{

					 
						if (SLT.server.recv.segment.pos_in_file == POS_CONTINUE)
						{
							lseek(fd, SLT.server.recv.current_pos_file, SEEK_SET);
						}
						else
						{
							lseek(fd, SLT.server.recv.segment.pos_in_file, SEEK_SET);
						}

						write(fd, &((uint8_t*)SLT.server.recv.segment.data.content)[SLT.server.recv.segment.pos_data], SLT.server.recv.segment.data.len);
						SLT.server.recv.current_pos_file = lseek(fd, 0, SEEK_CUR);
						free(SLT.server.recv.segment.data.content); 		
						SLT.server.recv.segment.data.content = NULL;
					
					}

				}
			}
			else
			{
				if (SLT.server.recv.segment.data.content != NULL) 
				{
					free(SLT.server.recv.segment.data.content);
					SLT.server.recv.segment.data.content = NULL;
				}				
			}
			
			
		}
	}
}


