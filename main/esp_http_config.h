#ifndef ESP_HTTP_CONFIG

#define ESP_HTTP_CONFIG

#include "my_lib.h"

esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t save_post_handler(httpd_req_t *req);
void start_http_server(void);


#endif

