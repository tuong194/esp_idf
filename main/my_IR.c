#include "my_IR.h"
#include "my_gpio.h"

uint16_t IR_duration[50];
int8_t IR_data[50];

uint16_t data_rec = 0;
volatile int duration = 0;
volatile int indx_ir = 0;

#define TAG  "IR"

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

void RC5_send_bit_one(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(RC5_PULSE);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(RC5_PULSE);
}

void RC5_send_bit_zero(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 512); // Tắt PWM
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(RC5_PULSE);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(RC5_PULSE);
}

uint16_t merge_data_ir(uint8_t device_addr, uint8_t cmd_ir)
{
    uint16_t data_send;
    data_send = device_addr << 7 | cmd_ir;
    return data_send;
}

// SIRC
void SIRC_send_ir_signal(uint8_t device_addr, uint8_t cmd_ir)
{
    uint16_t data_send;
    data_send = merge_data_ir(device_addr, cmd_ir);
    send_pulse_high(SIRC_HEADER);
    for (int i = 0; i < 12; i++)
    {
        send_pulse_low(SIRC_BIT_LOW);
        if (data_send & 0x01)
        {
            send_pulse_high(SIRC_BIT_HIGH_ONE);
        }
        else
        {
            send_pulse_high(SIRC_BIT_HIGH_ZERO);
        }
        data_send >>= 1;
    }
}

// example NEC
void NEC_send_ir_signal(uint8_t data)
{
    // Gửi Header
    send_pulse_high(NEC_HEADER_HIGH); // Xung HIGH 9000 us
    send_pulse_low(NEC_HEADER_LOW);   // Xung LOW 4500 us

    // Gửi Dữ Liệu
    for (int i = 0; i < 8; i++)
    {
        send_pulse_high(NEC_BIT_HIGH); // Xung HIGH 560 us

        if (data & 0x80)
        {
            send_pulse_low(NEC_BIT_LOW_ONE); // Xung LOW 1690 us cho bit 1
        }
        else
        {
            send_pulse_low(NEC_BIT_LOW_ZERO); // Xung LOW 560 us cho bit 0
        }
        data <<= 1;
    }

    // Gửi Kết Thúc
    send_pulse_high(560);
    // ets_delay_us(560); // Xung LOW 560 us
}

void RC5_send_ir_signal(uint8_t addr, uint8_t cmd) // 5 bit addr, 6 bit cmd
{
    uint16_t data_send = 0;
    data_send = addr << 6 | (cmd & 0x3F);
    printf("data send : %u\n", data_send);
    RC5_send_bit_one();
    RC5_send_bit_one();
    RC5_send_bit_zero();
    for (int i = 0; i < 11; i++)
    {
        if (data_send & 0x400)
        {
            RC5_send_bit_one();
        }
        else
        {
            RC5_send_bit_zero();
        }
        data_send <<= 1;
    }
}

uint8_t check_type_IR(void)
{
    int count = 0;
    if (IR_duration[1] * 50 > NEC_HEADER_HIGH - 500 && IR_duration[1] * 50 < NEC_HEADER_HIGH + 500)
    {
        if (IR_duration[2] * 50 > NEC_HEADER_LOW - 200 && IR_duration[2] * 50 < NEC_HEADER_LOW + 200)
        {
            printf("chuan NEC!!! \n");
            return IR_NEC;
        }
    }
    else if (IR_duration[1] * 50 > SIRC_HEADER - 300 && IR_duration[1] * 50 < SIRC_HEADER + 300)
    {
        printf("chuan SIRC!!! \n");
        return IR_SIRC;
    }
    else
    {
        for (int i = 1; i <= 28; i++)
        {
            if (IR_duration[i] * 50 > (RC5_PULSE - 150) && IR_duration[i] * 50 < (RC5_PULSE + 150))
            {
                count += 1;
            }
            else if (IR_duration[i] * 50 > (2 * RC5_PULSE - 150) && IR_duration[i] * 50 < (2 * RC5_PULSE + 150))
            {

                count += 2;
            }
        }
        printf("gia tri count: %d\n", count);
        if (count >= 26)
        {
            printf("chuan RC5!!! \n");
            return IR_RC5;
        }
    }

    return 0;
}

void parse_data(void)
{
    int i = 0;
    uint8_t check_type = check_type_IR();
    if (check_type == IR_NEC)
    {
        for (i = 4; i <= 18; i += 2) // ktra xung LOW
        {
            if (IR_duration[i] * 50 > NEC_BIT_LOW_ZERO - 100 && IR_duration[i] * 50 < NEC_BIT_LOW_ZERO + 100)
            {
                data_rec <<= 1;
            }
            else if (IR_duration[i] * 50 > NEC_BIT_LOW_ONE - 100 && IR_duration[i] * 50 < NEC_BIT_LOW_ONE + 100)
            {
                data_rec <<= 1;
                data_rec |= 1;
            }
        }
    }
    else if (check_type == IR_SIRC)
    {
        for (i = 25; i >= 3; i -= 2)
        {                                                                                                         // ktra xung HIGH
            if (IR_duration[i] * 50 > SIRC_BIT_HIGH_ZERO - 150 && IR_duration[i] * 50 < SIRC_BIT_HIGH_ZERO + 150) // bit 0
            {
                data_rec <<= 1;
            }
            else if (IR_duration[i] * 50 > SIRC_BIT_HIGH_ONE - 150 && IR_duration[i] * 50 < SIRC_BIT_HIGH_ONE + 150) // bit 1
            {
                data_rec <<= 1;
                data_rec |= 1;
            }
        }
        printf("address device: 0x%02X \n", data_rec >> 7); // 5 bit
        printf("cmd is: 0x%02X \n", data_rec & 0x7F);       // 7 bit
    }
    else if (check_type == IR_RC5)
    {
        for (i = 4; i <= 28; i++)
        {

            if (IR_data[i] == 0 && IR_duration[i] > 0)
            {
                printf("i=%d, bit1\n", i);
                data_rec <<= 1;
                data_rec |= 1;
            }
            else if (IR_data[i] == 1)
            {
                printf("i=%d, bit0\n", i);
                data_rec <<= 1;
            }

            if (IR_duration[i + 1] * 50 > (RC5_PULSE - 150) && IR_duration[i + 1] * 50 < (RC5_PULSE + 150))
            {
                i++;
            }
        }
        printf("address device: 0x%02X \n", data_rec >> 6); // 5 bit
        printf("cmd is: 0x%02X \n", data_rec & 0x3F);       // 6 bit
    }

    ESP_LOGI(TAG, "data receive: %u\n", data_rec);
}

void task_send_ir(void *para)
{
    vTaskDelay(1000);
    while (1)
    {
        NEC_send_ir_signal(130);                     // NEC
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000);
        SIRC_send_ir_signal(0x01, 0x15);             // SIRC
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000);
        RC5_send_ir_signal(0x10, 0x32);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000);
    }
}

void task_rec_ir(void *para)
{

    while (1)
    {
        for (int i = 0; i < 28; i++)
        {
            printf("data receive IR_data[%d]: %01X, duration time: %u \n", i, IR_data[i], IR_duration[i]);
            vTaskDelay(10);
        }
        printf("---------------------------------------------------\n");

        parse_data();
        data_rec = 0;
        memset(IR_data, -1, sizeof(IR_data));
        memset(IR_duration, 0, sizeof(IR_duration));

        vTaskDelay(4500);
    }
}
void init_task_IR(void)
{
    memset(IR_data, -1, sizeof(IR_data));
    xTaskCreate(task_send_ir, "send_task", 4096, NULL, 10, NULL);
    xTaskCreate(task_rec_ir, "rec_task", 4096, NULL, 10, NULL);
}