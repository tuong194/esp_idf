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
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "esp_http_server.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

	 If you'd rather not, just change the below entries to strings with
	 the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define ESP_WIFI_AP_SSID "esp32c3"	// CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_AP_PASS "hoilamgi" // CONFIG_ESP_WIFI_PASSWORD
#define ESP_WIFI_CHANNEL 1					// CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONNECTION 4				// CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "softAP";

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
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

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
					.authmode = WIFI_AUTH_WPA2_PSK}
	};
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
static esp_err_t http_post_handler(httpd_req_t *req){
	
}

static const httpd_uri_t http_post = {
	.uri = "/post",
	.method = HTTP_POST,
	.handler = http_post_handler,
	.user_ctx= NULL
};



void app_main(void)
{

	// ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
	wifi_softAP_start();

}
