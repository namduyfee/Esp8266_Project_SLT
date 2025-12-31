#ifndef SPIFFS_CONFIG
#define SPIFFS_CONFIG

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_spiffs.h"

void spiffs_init(void);
void spiffs_read_file(const char *path, char *data, uint32_t data_len);


#endif