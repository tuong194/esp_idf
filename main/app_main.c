/*  WiFi softAP Example

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "esp_http_server.h"

#include "lwip/err.h"
#include "lwip/sys.h"

char *ssid = "RD_SMART";
char *pass = "Led@RD@99";

static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;
static httpd_handle_t start_webserver(void);
void smart_config_wifi(void);
httpd_handle_t server = NULL;

#define ESP_WIFI_AP_SSID "esp32c3"	// CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_AP_PASS "hoilamgi" // CONFIG_ESP_WIFI_PASSWORD
#define ESP_WIFI_CHANNEL 1			// CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONNECTION 4		// CONFIG_ESP_MAX_STA_CONN

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY  5//CONFIG_ESP_MAXIMUM_RETRY // 5





static const char *TAG = "smartConfig";

static void event_handler(void *arg, esp_event_base_t event_base,
						  int32_t event_id, void *event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		esp_wifi_connect();
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
		{
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}
		else
		{
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG, "connect to the AP fail");
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;

		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
							   int32_t event_id, void *event_data)
{
	if (event_id == WIFI_EVENT_AP_STACONNECTED)
	{
		wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
		ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
				 MAC2STR(event->mac), event->aid);
	}
	else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
	{
		wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
		ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
				 MAC2STR(event->mac), event->aid, event->reason);
	}
}

void wifi_softAP_start(void)
{


	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&wifi_event_handler,
														NULL,
														NULL));
	wifi_config_t wifi_config = {
		.ap = {
			.ssid = ESP_WIFI_AP_SSID,
			.ssid_len = strlen(ESP_WIFI_AP_SSID),
			.channel = ESP_WIFI_CHANNEL,
			.password = ESP_WIFI_AP_PASS,
			.max_connection = MAX_STA_CONNECTION,
			.authmode = WIFI_AUTH_WPA2_PSK}};
	if (strlen(ESP_WIFI_AP_PASS) == 0)
	{
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s ",
			 ESP_WIFI_AP_SSID, ESP_WIFI_AP_PASS);
}

/*------------------------------------------------------------------------------------------*/

esp_err_t get_handler(httpd_req_t *req)
{
	const char *resp_str = "<!DOCTYPE html>"
						   "<html>"
						   "<body>"
						   "<label>SSID</label>"
						   "<input type='text' id='ssid'><br><br>"
						   "<label>PASS</label>"
						   "<input type='text' id='pass'><br><br>"
						   "<button onclick='myFunc()'>SEND</button>"
						   "<script>"
						   "var xhttp1 = new XMLHttpRequest();"
						   "function myFunc(){"
						   "var ssid = document.getElementById('ssid').value;"
						   "var pass = document.getElementById('pass').value;"
						   "var data = ssid + '/' + pass;"
						   "xhttp1.open('POST', '/post', true);"
						   "xhttp1.send(data);"
						   "}"
						   "</script>"
						   "</body>"
						   "</html>";
	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

httpd_uri_t uri_get = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = get_handler,
	.user_ctx = NULL};

esp_err_t http_post_handler(httpd_req_t *req)
{
	char content[100];
	size_t recv_size = MIN(req->content_len, sizeof(content) - 1);

	int ret = httpd_req_recv(req, content, recv_size);
	if (ret <= 0)
	{
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}
	content[ret] = '\0';
	ESP_LOGI(TAG, "receive data : %s", content);

	ssid = strtok(content, "/");
	pass = strtok(NULL, "/");

	// ESP_LOGI(TAG, "SSID: %s", ssid);
	// ESP_LOGI(TAG, "PASS: %s", pass);

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
	
	smart_config_wifi();
	return ESP_OK;
}

httpd_uri_t uri_post = {
	.uri = "/post",
	.method = HTTP_POST,
	.handler = http_post_handler,
	.user_ctx = NULL};

static httpd_handle_t start_webserver(void)
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_post);
	}
	return server;
}

void smart_config_wifi(void)
{
	ESP_LOGI(TAG, "config wifi station");
	
	ESP_ERROR_CHECK(esp_wifi_stop());
 
   
     esp_netif_create_default_wifi_sta();
	// wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	// ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = "",
		}
	};
	memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
	memcpy(wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

	ESP_LOGI(TAG, "SSID: %s", wifi_config.sta.ssid);
	ESP_LOGI(TAG, "PASS: %s", wifi_config.sta.password);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	// ESP_ERROR_CHECK(httpd_stop(server));
	

	s_wifi_event_group = xEventGroupCreate();
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
										   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
										   pdTRUE,
										   pdFALSE,
										   portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */

	if (bits & WIFI_CONNECTED_BIT)
	{
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
				 ssid, pass);
		httpd_stop(server);
		ESP_LOGI(TAG, "webserver stop");
		// vTaskDelete(NULL);
		
		 
	}
	else if (bits & WIFI_FAIL_BIT)
	{
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
				 ssid, pass);
	}
	else
	{
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}
}

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	wifi_softAP_start();
	start_webserver();
}
