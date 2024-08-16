/* LEDC (LED Controller) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/queue.h"
#include "esp_task_wdt.h"

static const char *TAG = "IR";

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (5) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT // Set duty resolution to 13 bits

#define LEDC_FREQUENCY (38000) // Frequency in Hertz. Set frequency at 4 kHz

#define ESP_INTR_FLAG_DEFAULT 0

#define IR_PIN 18
#define TIMER_INTERVAL_MS 1000 // 1s
#define TIMER_SCALE TIMER_SRC_CLK_DEFAULT/160

void rx_ir(void *arg);

uint8_t IR_data[100];
int IR_duration[100];

static int last_level;
static int new_level =5;
static bool check_get_tick;
int indx_ir = 0;

static  int count = 0;

static void timer_callback(TimerHandle_t xTimer)
{

    if (new_level != last_level)
    {
        count++;
    }

    if (count >= 10)
    {
        ESP_LOGI(TAG, "Time Out");
        last_level = new_level;
        indx_ir = 0;
        check_get_tick = 1;
        count = 0;
    }
}

static QueueHandle_t gpio_evt_queue = NULL;
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static bool IRAM_ATTR itr_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    
        count++;
        //printf("running");
    
    
    
    return true;
}

void config_timer(void)
{
    TimerHandle_t timer = xTimerCreate(
        "MyTimer",
        pdMS_TO_TICKS(TIMER_INTERVAL_MS),
        pdTRUE,
        0,
        timer_callback);
    xTimerStart(timer, 0);
}

void config_timer_us(uint64_t time_us)
{
   gptimer_handle_t gptimer = NULL;

    // Cấu hình GPTimer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,  
        .direction = GPTIMER_COUNT_UP,       
        .resolution_hz = 1000000,            // f=  1 MHz (1 tick = 1 us)
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Cấu hình alarm
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = time_us,    // Ngắt mỗi 500 000 us
        .reload_count = 0,                
        .flags.auto_reload_on_alarm = true,  // Tự động tải lại khi ngắt
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Đăng ký callback
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &(gptimer_event_callbacks_t){
        .on_alarm = itr_timer_cb,
    }, NULL));

    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    // Bật timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));
        ESP_LOGI(TAG,"done");
}

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at ... kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_OUTPUT_IO,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void gpio_set_pin_input(gpio_num_t GPIO_NUM, gpio_int_type_t INTR_TYPE)
{
    gpio_config_t gpio_config_pin = {
        .pin_bit_mask = 1ULL << GPIO_NUM,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = INTR_TYPE};
    gpio_config(&gpio_config_pin);
}
void send_pulse_high(uint16_t duration_us)
{

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(duration_us);
}

void send_pulse_low(uint16_t duration_us)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(duration_us);
}

// example NEC
void send_ir_signal(uint8_t data)
{
    // Gửi Header
    send_pulse_high(9000); // Xung HIGH 9000 us
    send_pulse_low(4500);  // Xung LOW 4500 us

    // Gửi Dữ Liệu
    for (int i = 0; i < 8; i++)
    {
        send_pulse_high(560); // Xung HIGH 560 us

        if (data & 0x80)
        {
            send_pulse_low(1690); // Xung LOW 1690 us cho bit 1
        }
        else
        {
            send_pulse_low(560); // Xung LOW 560 us cho bit 0
        }
        data <<= 1;
    }

    // Gửi Kết Thúc
    send_pulse_high(560);
    // ets_delay_us(560); // Xung LOW 560 us
}

void task_send_ir(void *para)
{
    vTaskDelay(1000);
    while (1)
    {
        send_ir_signal(130);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000);
    }
}

void rx_ir1(void *para)
{
    last_level = 1;

    uint64_t last_time = 0;
    uint64_t new_time = 0;
    check_get_tick = 1;

    while (1)
    {
        new_level = gpio_get_level(IR_PIN);

        if (new_level != last_level)
        {
            if (check_get_tick)
            {
                last_time = esp_timer_get_time();
                check_get_tick = 0;
            }
        }
        else
        {
            if (!check_get_tick)
            {
                new_time = esp_timer_get_time();

                // int duration_data = 0;
                int duration_data = (esp_timer_get_time() - last_time);

                IR_data[indx_ir] = new_level;
                IR_duration[indx_ir] = duration_data;

                ESP_LOGI(TAG, "IR_data[%d] is: %d, duration is: %d", indx_ir, new_level, duration_data);
                indx_ir++;
                last_level = !last_level;
                check_get_tick = 1;
                count = 0;
            }
        }
        // vTaskDelay(1);
    }
}

void app_main(void)
{
    // esp_task_wdt_config_t config = {
    //     .timeout_ms = 5000,                    // Timeout
    //     .idle_core_mask = (1 << 0) | (1 << 1), // Theo dõi các task IDLE trên lõi 0 và lõi 1
    //     .trigger_panic = true                  // Kích hoạt panic khi timeout xảy ra
    // };
    // esp_task_wdt_init(&config);
    // // config_timer();
    // example_ledc_init();

    // gpio_set_pin_input(IR_PIN, GPIO_INTR_DISABLE);
    // gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // gpio_isr_handler_add(IR_PIN, gpio_isr_handler, (void *)IR_PIN);

    // xTaskCreate(task_send_ir, "send_task", 4096, NULL, 10, NULL);
    // xTaskCreate(rx_ir, "receive_task", 4096, NULL, 10, NULL);

    config_timer_us(1000);

    while (1)
    {
        ESP_LOGI(TAG, "gia tri count: %d", count);
        vTaskDelay(1000);
    }
}

/*
NEC
PWM 50% -> HIGH -> IRPIN = 0
PWM 0   -> LOW  -> IRPIN = 1
*/
void rx_ir(void *arg)
{
    esp_task_wdt_add(NULL);

    unsigned long start_time = 0;
    unsigned long duration_time = 0;
    bool header_receive = false;
    uint8_t data_rec = 0;
    while (1)
    {

        while (gpio_get_level(IR_PIN) == 1)
        {

            vTaskDelay(1);
        }
        start_time = esp_timer_get_time(); // start 9ms Header muc HIGH
        while (gpio_get_level(IR_PIN) == 0)
        {

            vTaskDelay(1);
        }
        duration_time = esp_timer_get_time() - start_time;
        if (duration_time < 10000 && duration_time > 8000)
        { // Header HIGH = 9000us
            header_receive = true;
            ESP_LOGI(TAG, "header true!");
            ESP_LOGI(TAG, "time header high: %lu", duration_time);
        }
        if (header_receive)
        {                                      // co ban tin
            start_time = esp_timer_get_time(); // start 4,5 ms Header muc LOW
            while (gpio_get_level(IR_PIN) == 1)
            {
                esp_task_wdt_reset();
                // vTaskDelay(1);
            }
            duration_time = esp_timer_get_time() - start_time;
            ESP_LOGI(TAG, "time header low: %lu", duration_time);
            if (duration_time > 3500 && duration_time < 5500)
            {
                ESP_LOGI(TAG, "check data");
                // Heder LOW = 4500us
                for (uint8_t i = 0; i < 8; i++)
                {
                    // muc HIGH
                    while (gpio_get_level(IR_PIN) == 0)
                    {
                        esp_task_wdt_reset();
                        // vTaskDelay(1);
                    }
                    // start muc LOW
                    start_time = esp_timer_get_time();
                    while (gpio_get_level(IR_PIN) == 1)
                    {
                        esp_task_wdt_reset();
                        // vTaskDelay(1);
                    }
                    duration_time = esp_timer_get_time() - start_time;
                    if (duration_time > 460 && duration_time < 660)
                    { // bit 0
                        data_rec <<= 1;
                    }
                    else if (duration_time > 1587 && duration_time < 1787) // bit 1
                    {
                        data_rec <<= 1;
                        data_rec |= 1;
                    }
                }

                if (data_rec != 0)
                {
                    ESP_LOGI(TAG, "Received NEC code: 0x%02X", data_rec);
                    // printf("data receive: 0x%02X", data_rec);
                }
            }
        }
        ESP_LOGI(TAG, "finish receive data");
        // vTaskDelete(NULL);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}