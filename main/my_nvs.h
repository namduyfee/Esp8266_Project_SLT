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

#define NVS_ESPNOW_NAMESP "espnow_namespace"

#define NVS_GW_PEER_INF "gateway_peer_espnow"		/**! key in NVS_ESPNOW_NAMESP */

#define NVS_ADDED_PEERS_INF "added_peer_espnow"		/**! key in NVS_ESPNOW_NAMESP */

#define NVS_COUNT_PEER "count_peer_espnow"			/**! key in NVS_ESPNOW_NAMESP */
#endif