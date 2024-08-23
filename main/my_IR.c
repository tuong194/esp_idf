#include "my_IR.h"
#include "my_gpio.h"

uint16_t IR_duration[100];
int8_t IR_data[100];

uint32_t data_rec = 0;
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

uint32_t merge_data_ir(uint8_t device_addr, uint8_t cmd_ir, uint8_t extend, sirc_type type_sirc) //12,15 bit
{
    uint32_t data_send = 0;
    if (type_sirc == SIRC_20_BIT)
    {
        data_send = (extend<<12 & 0xFF000)| (device_addr << 7 & 0xF80) | cmd_ir;
    }else if (type_sirc == SIRC_15_BIT){
        data_send = (device_addr << 7 & 0x7F80) | cmd_ir;
    }else{
        data_send = (device_addr << 7 & 0xF80) | cmd_ir;
    }
    printf("send data : %lu", data_send);
    return data_send;
}

// SIRC
void SIRC_send_ir_signal(uint8_t device_addr, uint8_t cmd_ir, uint8_t extend, sirc_type type_sirc)
{
    uint8_t num_bit = 0;
    uint32_t data_send = 0;
    data_send = merge_data_ir(device_addr, cmd_ir,extend, type_sirc);
    send_pulse_high(SIRC_HEADER);
    send_pulse_low(600);
    if(type_sirc == SIRC_12_BIT){
        num_bit = 12;
    }else if (type_sirc == SIRC_15_BIT)
    {
        num_bit = 15;
    }else if (type_sirc == SIRC_20_BIT)
    {
        num_bit = 20;
    }
    
    
    for (int i = 0; i < num_bit; i++)
    {
        
        if (data_send & 0x01)
        {
            send_pulse_high(SIRC_BIT_HIGH_ONE);
        }
        else
        {
            send_pulse_high(SIRC_BIT_HIGH_ZERO);
        }
        send_pulse_low(SIRC_BIT_LOW);
        data_send >>= 1;
    }
}


// example NEC
void NEC_send_ir_signal(uint8_t adr, uint8_t cmd)
{
    uint32_t data_send = 0;;
    data_send = ((~cmd)<<24 & 0xFF000000)|(cmd<<16 &0xFF0000)|((~adr)<<8 & 0xFF00)|(adr);

    printf("NEC data: %lu\n", data_send);
    // Gửi Header
    send_pulse_high(NEC_HEADER_HIGH); // Xung HIGH 9000 us
    send_pulse_low(NEC_HEADER_LOW);   // Xung LOW 4500 us

    // Gửi Dữ Liệu
    for (int i = 0; i < 32; i++)
    {
        send_pulse_high(NEC_BIT_HIGH); // Xung HIGH 560 us

        if (data_send & 0x01)
        {
            send_pulse_low(NEC_BIT_LOW_ONE); // Xung LOW 1690 us cho bit 1
        }
        else
        {
            send_pulse_low(NEC_BIT_LOW_ZERO); // Xung LOW 560 us cho bit 0
        }
        data_send >>= 1;
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

sirc_type check_type_SIRC(void){
    if(IR_data[33] == 1){
        return SIRC_20_BIT;
    }else if (IR_data[27] == 1 && IR_data[33] != 1)
    {
        return SIRC_15_BIT;
    }else{
        return SIRC_12_BIT;
    }
    
}

ir_type check_type_IR(void)
{
    int count = 0;
    if (IR_duration[1] * 50 > NEC_HEADER_HIGH - 150 && IR_duration[1] * 50 < NEC_HEADER_HIGH + 150)
    {
        if (IR_duration[2] * 50 > NEC_HEADER_LOW - 150 && IR_duration[2] * 50 < NEC_HEADER_LOW + 150)
        {
            printf("chuan NEC!!! \n");
            return IR_NEC;
        }
    }
    else if (IR_duration[1] * 50 > SIRC_HEADER - 150 && IR_duration[1] * 50 < SIRC_HEADER + 150)
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

    return UNKNOWN;
}

void parse_data(void)
{
    int i = 0;
    ir_type check_type = check_type_IR();
    if (check_type == IR_NEC)
    {
        for (i = 66; i >= 4; i -= 2) // ktra xung LOW
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
        uint8_t addr = data_rec & 0xFF;
        uint8_t cmd = (data_rec>>16) & 0xFF;
        printf("address device: 0x%02X \n", addr); // 5 bit
        printf("cmd is: 0x%02X \n", cmd);       // 7 bit
    }
    else if (check_type == IR_SIRC)
    {
        uint8_t num_bit = 0;
        sirc_type check_sirc = check_type_SIRC();
        if (check_sirc == SIRC_12_BIT)
        {
            num_bit = 25;
        }else if (check_sirc == SIRC_15_BIT)
        {
            num_bit = 31;
        }else if (check_sirc == SIRC_20_BIT)
        {
            num_bit = 41;
        }
        
        for (i = num_bit; i >= 3; i -= 2)
        {   // check pulse
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
        uint8_t addr = 0;
        uint8_t cmd = 0;
        uint8_t extend = 0;
        if(check_sirc == SIRC_20_BIT){
            extend = data_rec >> 12 & 0xFF;
            addr = data_rec>>7 &0x1F;
            cmd = data_rec &0x7F;
        }else{
            addr = data_rec >> 7;
            cmd = data_rec & 0x7F;
            extend = 0;

        }
        printf("extend is: 0x%02X \n", extend); // 8 bit
        printf("address device: 0x%02X \n", addr); // 5 or 7 bit
        printf("cmd is: 0x%02X \n", cmd);       // 7 bit
    }
    else if (check_type == IR_RC5)
    {
        for (i = 4; i <= 28; i++)
        {
            if(i==4 && IR_duration[i + 1] * 50 > (RC5_PULSE - 150) && IR_duration[i + 1] * 50 < (RC5_PULSE + 150)) i += 1;
            if (IR_data[i] == 0 && IR_duration[i] > 0)
            {
                //printf("i=%d, bit1\n", i);
                data_rec <<= 1;
                data_rec |= 1;
            }
            else if (IR_data[i] == 1)
            {
                //printf("i=%d, bit0\n", i);
                data_rec <<= 1;
            }

            if (IR_duration[i + 1] * 50 > (RC5_PULSE - 150) && IR_duration[i + 1] * 50 < (RC5_PULSE + 150))
            {
                i++;
            }
        }
        uint8_t addr = data_rec >>6;
        uint8_t cmd = data_rec & 0x3F;
        printf("address device: 0x%02X \n", addr); // 5 bit
        printf("cmd is: 0x%02X \n", cmd);       // 6 bit
    }

    ESP_LOGI(TAG, "data receive: %lu\n", data_rec);
}

void task_send_ir(void *para)
{
    vTaskDelay(1000);
    while (1)
    {
        // NEC_send_ir_signal(0x03,0x31);                     // NEC
        // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(5000/portTICK_PERIOD_MS);
        SIRC_send_ir_signal(0x01, 0x15,0, SIRC_12_BIT);             // SIRC
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000/portTICK_PERIOD_MS);

        SIRC_send_ir_signal(0x5D, 0x7E,0, SIRC_15_BIT);             // SIRC
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000/portTICK_PERIOD_MS);

        SIRC_send_ir_signal(0x1B, 0x69, 0xA3, SIRC_20_BIT);             // SIRC
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000/portTICK_PERIOD_MS);        
        // RC5_send_ir_signal(0x05, 0x32);
        // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}

void task_rec_ir(void *para)
{

    while (1)
    {
        // for (int i = 0; i < 30; i++)
        // {
        //     printf("data receive IR_data[%d]: %01X, duration time: %u \n", i, IR_data[i], IR_duration[i]);
        //     vTaskDelay(10/portTICK_PERIOD_MS);
        // }
        printf("---------------------------------------------------\n");

        parse_data();
        data_rec = 0;
        memset(IR_data, -1, sizeof(IR_data));
        memset(IR_duration, 0, sizeof(IR_duration));

        vTaskDelay(4500/portTICK_PERIOD_MS);
    }
}
void init_task_IR(void)
{
    memset(IR_data, -1, sizeof(IR_data));
    xTaskCreate(task_send_ir, "send_task", 4096, NULL, 10, NULL);
    xTaskCreate(task_rec_ir, "rec_task", 4096, NULL, 10, NULL);
}