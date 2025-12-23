#include "spiffs_config.h"

void spiffs_init(void)
{
//	esp_spiffs_format(NULL);
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = true
	};
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	
	if (ret != ESP_OK)
	{
		printf("SPIFFS mount failed: %s\n", esp_err_to_name(ret));
		if (ret == ESP_ERR_NOT_FOUND)
		{
			
		}
		else if (ret == ESP_ERR_NO_MEM)
		{
			
		}
		else if (ret == ESP_ERR_INVALID_SIZE)
		{
		
		}
	}
	else
	{
		
	}
	
}

void spiffs_read_file(const char *path, char *data, uint32_t data_len)
{ 
	int fd = open(path, O_RDONLY, 0666);
	if (fd < 0)
	{
		return;
	}
	read(fd, data, data_len);
	close(fd);
	
}



