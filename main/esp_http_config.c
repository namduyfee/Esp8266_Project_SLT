
#include "esp_http_config.h"

static httpd_handle_t server = NULL;
static const char *html_form =
    "<!DOCTYPE html><html><body>"
    "<h2>ESP8266 Wi-Fi Config</h2>"
    "<form action=\"/save\" method=\"post\">"
    "SSID: <input name=\"ssid\" type=\"text\"><br>"
    "PASS: <input name=\"pass\" type=\"password\"><br>"
    "<input type=\"submit\" value=\"Save\">"
    "</form></body></html>";

static httpd_uri_t uri_root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = root_get_handler
};

static httpd_uri_t uri_save = {
	.uri = "/save",
	.method = HTTP_POST,
	.handler = save_post_handler
};


esp_err_t root_get_handler(httpd_req_t *req)
{
	httpd_resp_send(req, html_form, strlen(html_form));
	return ESP_OK; 
	
}

esp_err_t save_post_handler(httpd_req_t *req)
{
	char buf[64];           // buffer t?m ?? ??c t?ng ph?n
	int remaining = req->content_len;
	char body[128] = {0};   // l?u toàn b? body an toàn
	int idx = 0;

	while (remaining > 0 && idx < sizeof(body) - 1) {
		int to_read = remaining;
		if (to_read > sizeof(buf))
			to_read = sizeof(buf);

		int ret = httpd_req_recv(req, buf, to_read);
		if (ret <= 0) {
			return ESP_FAIL;
		}

		// Copy vào body
		memcpy(body + idx, buf, ret);
		idx += ret;
		remaining -= ret;
	}

	body[idx] = '\0'; // k?t thúc chu?i


	// Parse ssid và pass
	sscanf(body, "ssid=%31[^&]&pass=%63s", wifi_cred.ssid, wifi_cred.pass);

	// Save vào NVS
	nvs_handle nvs;
	if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
		nvs_set_str(nvs, "ssid", wifi_cred.ssid);
		nvs_set_str(nvs, "pass", wifi_cred.pass);
		nvs_commit(nvs);
		nvs_close(nvs);
	}

	// G?i response
	const char *resp = "Saved! Rebooting...";
	httpd_resp_send(req, resp, strlen(resp));

	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();

	return ESP_OK;
}

void start_http_server(void)
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_start(&server, &config);
	httpd_register_uri_handler(server, &uri_root);
	httpd_register_uri_handler(server, &uri_save);
}

