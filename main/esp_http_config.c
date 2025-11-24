
#include "esp_http_config.h"

static httpd_handle_t server = NULL;
static const char *html_form =
    "<!DOCTYPE html><html><body>"
    "<h2>ESP8266 Wi-Fi Config</h2>"

    "<form action=\"/wifi/save\" method=\"post\">"
        "SSID: <input name=\"ssid\" type=\"text\"><br>"
        "PASS: <input name=\"pass\" type=\"password\"><br>""<br>"
	
        "<input type=\"submit\" value=\"Save\">"
    "</form>"

    "<br>"

    "<form action=\"/wifi/info\" method=\"get\">"
        "<input type=\"submit\" value=\"Info\">"
    "</form>"

    "</body></html>";

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
static httpd_uri_t uri_wifi = {
	.uri = "/wifi",
	.method = HTTP_GET,
	.handler = wifi_get_handler
};

static httpd_uri_t uri_wifi_info = {
	.uri = "/wifi/info",
	.method = HTTP_GET,
	.handler = wifi_get_handler
};

static httpd_uri_t uri_wifi_save = {
	.uri = "/wifi/save",
	.method = HTTP_POST,
	.handler = save_post_handler
};

static httpd_uri_t uri_upload = {
	.uri = "/upload",
	.method = HTTP_POST,
	.handler = upload_post_handler
};


void start_http_server(void)
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_start(&server, &config);
	httpd_register_uri_handler(server, &uri_wifi);
	httpd_register_uri_handler(server, &uri_root);
	httpd_register_uri_handler(server, &uri_wifi_save);
	httpd_register_uri_handler(server, &uri_wifi_info);
	httpd_register_uri_handler(server, &uri_upload);
}


esp_err_t wifi_get_handler(httpd_req_t *req)
{
	httpd_resp_send_chunk(req, html_form, strlen(html_form));
	
	wifi_ap_record_t ap_info;
	
	esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
	
	if (err == ESP_OK)
	{
		httpd_resp_send_chunk(req, "\n", 1);
		httpd_resp_send_chunk(req, "SSID :  ", strlen("SSID :  "));
		httpd_resp_send_chunk(req, (char*)ap_info.ssid, strlen((char*)ap_info.ssid));
		

	}
	else
	{
		const char *resp = "Not connected to any Wi-Fi";
		httpd_resp_send_chunk(req, resp, strlen(resp));
	}
	
	return ESP_OK; 
}
esp_err_t root_get_handler(httpd_req_t *req)
{
	httpd_resp_send(req, html_upload_form, strlen(html_upload_form));
	return ESP_OK; 
}

esp_err_t save_post_handler(httpd_req_t *req)
{
	// only post ssid and pass, so all contens will < 512 byte
	char buf[512] = {0};
	int len_read = ((sizeof(buf)) <= req->content_len) ? sizeof(buf) - 1 : req->content_len;
	int ret = httpd_req_recv(req, buf, len_read);
	if (ret <= 0) {
				
	}

	buf[len_read] = '\0';	
	// Parse ssid và pass
	sscanf(buf, "ssid=%31[^&]&pass=%63s", wifi_cred.ssid, wifi_cred.pass);		
	
	wifi_cred.is_call_discnt = true;
	esp_wifi_disconnect();
	
	httpd_resp_send(req, html_form, strlen(html_form));
	
	vTaskDelay(pdMS_TO_TICKS(1));
	
	wifi_config_t sta_cfg = {0};
	strcpy((char*)sta_cfg.sta.ssid, wifi_cred.ssid);
	strcpy((char*)sta_cfg.sta.password, wifi_cred.pass);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg);
	esp_wifi_connect();

	return ESP_OK;
}

esp_err_t upload_post_handler(httpd_req_t *req)
{
	char buf[512];
	int remaining = req->content_len;
	int ret;
	int fd = open("/spiffs/upload.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	
	if (fd < 0)
	{
		httpd_resp_send_500(req);
		return ESP_FAIL;

	}

	bool header_skipped = false;

	while (remaining > 0)
	{
		int to_read = remaining < sizeof(buf) ? remaining : sizeof(buf);
		ret = httpd_req_recv(req, buf, to_read);
		if (ret <= 0) {
			close(fd);
			return ESP_FAIL;
		}

		remaining = remaining - ret;

		if (header_skipped == false)
		{
			/*
			 * end of header: "\r\n\r\n" 
			 * header alway the first buf before all data raw. header often < 512 byte (size buf[]) so header will not be cut into next buf
			 * so no need to process "\r\n\r\n" in many cases
			 */
			char *body = strstr(buf, "\r\n\r\n");
			if (body != NULL)
			{
				body += 4; // skip \r\n\r\n
				int body_len = (buf + ret) - body;

				if (body_len > 0)
					write(fd, body, body_len);

				header_skipped = true;
				continue;
			}
		}
		else
		{
			write(fd, buf, ret);
		}
	}
	close(fd);
	if (header_skipped == true)
	{
		
		const char *resp = "stored!";
		httpd_resp_send(req, resp, strlen(resp));
	}
	else
	{
		const char *resp = "faild stored!";
		httpd_resp_send(req, resp, strlen(resp));
	}
	return ESP_OK;
}

