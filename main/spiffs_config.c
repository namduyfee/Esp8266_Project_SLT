#include "spiffs_config.h"

void spiffs_init(void)
{
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 7,
		.format_if_mount_failed = true
	};
	esp_vfs_spiffs_register(&conf);
	
}
void spiffs_write_file(const char *path, const char *data, uint32_t data_len)
{
	int fd = open(path, O_WRONLY | O_CREAT, 0666);
	ssize_t  written = write(fd, data, data_len);
	if (written != data_len)
	{
		ESP_LOGE("SPIFFS", "Write error");
	}
	close(fd);
	
}

void spiffs_read_file(const char *path, char *data, uint32_t data_len)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		ESP_LOGI("SPIFFS", "Cannot open file");
		return;
	}
	ssize_t n = read(fd, data, data_len);
	ESP_LOGI("SPIFFS", "Read %d bytes", n);
	close(fd);
}



