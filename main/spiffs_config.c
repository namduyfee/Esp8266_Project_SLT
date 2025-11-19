#include "spiffs_config.h"

void spiffs_init(void)
{
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = true
	};
	esp_vfs_spiffs_register(&conf);
	
}

void spiffs_read_file(const char *path, char *data, uint32_t data_len)
{
	FILE *fd = fopen(path, "rb");
	if (!fd)
	{
		ESP_LOGI("SPIFFS", "Cannot open file");
		return;
	}
	ssize_t n = fread(data, 1, data_len, fd);
	ESP_LOGI("SPIFFS", "Read %d bytes", n);
	fclose(fd);
}



