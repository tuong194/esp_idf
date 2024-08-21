/* LEDC (LED Controller) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "driver/gptimer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/queue.h"

#include "my_gpio.h"
#include "my_IR.h"
#include "wifi_STA.h"


#define TAG_MAIN "main"

static int level_gpio;
static bool flag_check_itr = 1;

static QueueHandle_t gpio_evt_queue = NULL;
static void IRAM_ATTR gpio_isr_handler(void *arg) // send data from ISR to QUEUE
{
   level_gpio = !level_gpio;
   flag_check_itr = 1;
   IR_data[indx_ir + 1] = !level_gpio;
   IR_duration[indx_ir] = duration;
   duration = 0;
   indx_ir++;

   uint32_t gpio_num = (uint32_t)arg;
   xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static bool IRAM_ATTR itr_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
   if (flag_check_itr)
   {
      duration++;
      if (duration > 230)
      {
         duration = 0;
         flag_check_itr = 0;
         indx_ir = 0;
      }
   }

   return true;
}

void config_timer_us(uint64_t time_us)
{
   gptimer_handle_t gptimer = NULL;

   // Cấu hình GPTimer
   gptimer_config_t timer_config = {
       .clk_src = GPTIMER_CLK_SRC_DEFAULT,
       .direction = GPTIMER_COUNT_UP,
       .resolution_hz = 1000000, // f=  1 MHz (1 tick = 1 us)
   };
   ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

   // Cấu hình alarm
   gptimer_alarm_config_t alarm_config = {
       .alarm_count = time_us, // Ngắt mỗi 50 us
       .reload_count = 0,
       .flags.auto_reload_on_alarm = true, // Tự động tải lại khi ngắt
   };
   ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

   // Đăng ký callback
   ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &(gptimer_event_callbacks_t){
                                                                 .on_alarm = itr_timer_cb,
                                                             },
                                                    NULL));

   ESP_ERROR_CHECK(gptimer_enable(gptimer));

   // Bật timer
   ESP_ERROR_CHECK(gptimer_start(gptimer));
   ESP_LOGI(TAG_MAIN, "done");
}

void init_main(void){
   config_timer_us(50);
   ledc_pwm_init();
   gpio_set_pin_input(IR_PIN, GPIO_INTR_ANYEDGE); // rising and falling
   gpio_evt_queue = xQueueCreate(30, sizeof(uint32_t));
   gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
   gpio_isr_handler_add(IR_PIN, gpio_isr_handler, (void *)IR_PIN);
   level_gpio = gpio_get_level(IR_PIN);
   
}

void app_main(void)
{
   //init_main();
   init_wifi();
   //init_task_IR();
}
