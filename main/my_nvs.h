#ifndef ESP_NVS

#define ESP_NVS

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

#include "nvs.h"
#include "nvs_flash.h"

#define NVS_ESPNOW_NAMESP "espnow"	

#define NVS_NOW_MY_MODE "espnow_mode"
#define NVS_NOW_MY_ID "espnow_my_id"
#define NVS_NOW_GW_CODE "espnow_gw_code"

#define NVS_MASTER_EFFECT_NS "master_effect"
#define NVS_MODE_EFFECT "mode_effect"

#endif