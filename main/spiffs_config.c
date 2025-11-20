#include "spiffs_config.h"

void spiffs_init(void)
{

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = true
	};
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		printf("SPIFFS mount failed: %s\n", esp_err_to_name(ret));
		if (ret == ESP_ERR_NOT_FOUND)
		{
			
			gpio_set_level(GPIO_NUM_2, 1);
			gpio_set_level(GPIO_NUM_4, 0);
		}
		if (ret == ESP_ERR_NO_MEM)
		{
			gpio_set_level(GPIO_NUM_2, 0);
			gpio_set_level(GPIO_NUM_4, 1);
		}
		if (ret == ESP_ERR_INVALID_SIZE)
		{
	//		gpio_set_level(GPIO_NUM_2, 1);
	//		gpio_set_level(GPIO_NUM_4, 1);
		}
	}
}

void spiffs_read_file(const char *path, char *data, uint32_t data_len)
{
	FILE *fd = fopen(path, "r");
	if (!fd)
	{
		ESP_LOGI("SPIFFS", "Cannot open file");
		return;
	}
	ssize_t n = fread(data, 1, data_len, fd);
	ESP_LOGI("SPIFFS", "Read %d bytes", n);
	fclose(fd);
}



