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
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "IR";

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (5) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT // Set duty resolution to 13 bits

#define LEDC_FREQUENCY (38000) // Frequency in Hertz. Set frequency at 4 kHz

#define IR_PIN 18
#define TIMER_INTERVAL_MS 1000 // 1s

void rx_ir();

uint8_t IR_data[100];
// int IR_duration[200];

static int last_level;
static int new_level;
static bool check_get_tick;
int indx_ir = 0;

static int count = 0;
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

    // configTICK_RATE_HZ
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

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
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
                // last_time = esp_timer_get_time();
                check_get_tick = 0;
            }
        }
        else
        {
            if (!check_get_tick)
            {
                // new_time = esp_timer_get_time();

                int duration_data = 0;
                // int duration_data =(int)(new_time - last_time) / 10;

                IR_data[indx_ir] = new_level;
                // IR_duration[indx_ir] = duration_data;

                ESP_LOGI(TAG, "IR_data[%d] is: %d, duration is: %d", indx_ir, new_level, duration_data);
                indx_ir++;
                last_level = !last_level;
                check_get_tick = 1;
                count = 0;
            }
        }
    }
}

void task_test(void *para){
    int x= 2;
    while (1)
    {
        while(x>1){
            //esp_rom_delay_us(10000);
            vTaskDelay(10/portTICK_PERIOD_MS);
        }
        //vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
}
void app_main(void)
{

    example_ledc_init();
    gpio_set_pin_input(IR_PIN, GPIO_INTR_DISABLE);
   //xTaskCreate(rx_ir, "receive_task", 4096, NULL, 10, NULL);

    xTaskCreate(task_test, "receive_task", 4096, NULL, 10, NULL);
    rx_ir();
    // while (1)
    // {
    //     send_ir_signal(130);
    //     ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0);
    //     ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    //   //  xTaskCreate(rx_ir, "receive_task", 4096, NULL, 3, NULL);
    
    //     vTaskDelay(5000 / portTICK_PERIOD_MS);
    // }

    // config_timer();

     
}






/*
NEC
PWM 50% -> HIGH -> IRPIN = 0
PWM 0   -> LOW  -> IRPIN = 1
*/
void rx_ir()
{
    
    unsigned long start_time = 0;
    unsigned long duration_time = 0;
    bool header_receive = false;
    uint8_t data_rec = 0;
    while (1)
    {

        while (gpio_get_level(IR_PIN) == 1)
        {
            esp_rom_delay_us(10000);
            //vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        start_time = esp_timer_get_time(); // start 9ms Header muc HIGH
        while (gpio_get_level(IR_PIN) == 0)
        {
           esp_rom_delay_us(10000);
            //vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        duration_time = esp_timer_get_time() - start_time;
        if (duration_time < 10000 && duration_time > 8000)
        { // Header HIGH = 9000us
            header_receive = true;
        }
        if (header_receive)
        {                                      // co ban tin
            start_time = esp_timer_get_time(); // start 4,5 ms Header muc LOW
            while (gpio_get_level(IR_PIN) == 1)
            {
                esp_rom_delay_us(10000);
                //vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            duration_time = esp_timer_get_time() - start_time;
            if (duration_time > 3500 && duration_time < 5500)
            {
                // Heder LOW = 4500us
                for (uint8_t i = 0; i < 8; i++)
                {
                    // muc HIGH
                    while (gpio_get_level(IR_PIN) == 0)
                    {
                        esp_rom_delay_us(10000);
                       // vTaskDelay(10 / portTICK_PERIOD_MS);
                    }
                    // start muc LOW
                    start_time = esp_timer_get_time();
                    while (gpio_get_level(IR_PIN) == 1)
                    {
                        esp_rom_delay_us(10000);
                        //vTaskDelay(10 / portTICK_PERIOD_MS);
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
                    ESP_LOGI(TAG, "Received NEC code: ");
                    printf("data receive: 0x%02X", data_rec);
                }
            }
        }
       // vTaskDelete(NULL);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}