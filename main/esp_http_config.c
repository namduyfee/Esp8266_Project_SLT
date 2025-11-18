
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

static const char *html_upload_form =
	"<!DOCTYPE html><html><body>"
	"<h2>Upload file .bin cho ESP</h2>"
	"<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\">"
	"<label for=\"file\">enter file .bin:</label>"
	"<input type=\"file\" id=\"file\" name=\"file\" accept=\".bin\"><br><br>"
	"<input type=\"submit\" value=\"Upload\">"
	"</form>""</body>""</html>";

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

static httpd_uri_t uri_upload = {
	.uri = "/upload",
	.method = HTTP_POST,
	.handler = upload_post_handler
};

esp_err_t upload_post_handler(httpd_req_t *req)
{
	
	
	return ESP_OK;
}

esp_err_t root_get_handler(httpd_req_t *req)
{
	wifi_mode_t wifi_mode;
	esp_wifi_get_mode(&wifi_mode);
	
	if (wifi_mode == WIFI_MODE_AP)
		httpd_resp_send(req, html_form, strlen(html_form));
	else if(wifi_mode == WIFI_MODE_STA)
		httpd_resp_send(req, html_upload_form, strlen(html_upload_form));
	return ESP_OK; 
	
}

esp_err_t save_post_handler(httpd_req_t *req)
{
	char buf[512] = {0};
	int len_read;
	
	if ((sizeof(buf)) <= req->content_len)
		len_read = sizeof(buf) - 1;
	else 
		len_read = req->content_len;

	int ret = httpd_req_recv(req, buf, len_read);
	if (ret <= 0) {
				
	}

	buf[len_read] = '\0';	
	// Parse ssid và pass
	sscanf(buf, "ssid=%31[^&]&pass=%63s", wifi_cred.ssid, wifi_cred.pass);			
	const char *resp = "Saved! Rebooting...";
	httpd_resp_send(req, resp, strlen(resp));
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xRecvPassWifi, &xHigherPriorityTaskWoken); 
	if (xHigherPriorityTaskWoken)
		portYIELD_FROM_ISR();

	return ESP_OK;
}

void start_http_server(void)
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_start(&server, &config);
	httpd_register_uri_handler(server, &uri_root);
	httpd_register_uri_handler(server, &uri_save);
	httpd_register_uri_handler(server, &uri_upload);
}

