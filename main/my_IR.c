#include "my_IR.h"
#include "my_gpio.h"

uint16_t IR_duration[500];
int8_t IR_data[500];

uint32_t data_rec = 0;
volatile int duration = 0;
volatile int indx_ir = 0;

#define TAG "IR"

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

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(RC5_PULSE);
}

uint32_t merge_data_ir(uint8_t device_addr, uint8_t cmd_ir, uint8_t extend, sirc_type type_sirc) // 12,15 bit
{
    uint32_t data_send = 0;
    if (type_sirc == SIRC_20_BIT)
    {
        data_send = (extend << 12 & 0xFF000) | (device_addr << 7 & 0xF80) | cmd_ir;
    }
    else if (type_sirc == SIRC_15_BIT)
    {
        data_send = (device_addr << 7 & 0x7F80) | cmd_ir;
    }
    else
    {
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
    data_send = merge_data_ir(device_addr, cmd_ir, extend, type_sirc);
    send_pulse_high(SIRC_HEADER);
    send_pulse_low(600);
    if (type_sirc == SIRC_12_BIT)
    {
        num_bit = 12;
    }
    else if (type_sirc == SIRC_15_BIT)
    {
        num_bit = 15;
    }
    else if (type_sirc == SIRC_20_BIT)
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
    uint32_t data_send = 0;

    data_send = ((~cmd) << 24 & 0xFF000000) | (cmd << 16 & 0xFF0000) | ((~adr) << 8 & 0xFF00) | (adr);

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

void JVC_send_ir_signal(uint8_t addr, uint8_t cmd)
{
    uint16_t data_send = 0;
    data_send = cmd << 8 | addr;
    send_pulse_high(JVC_HEADER_MARK);
    send_pulse_low(JVC_HEADER_SPACE);
    for (int i = 0; i < 16; i++)
    {
        send_pulse_high(JVC_BIT_HIGH);

        if (data_send & 0x01)
        {
            send_pulse_low(JVC_BIT_LOW_ONE);
        }
        else
        {
            send_pulse_low(JVC_BIT_LOW_ZERO);
        }
        data_send >>= 1;
    }
    send_pulse_high(JVC_BIT_HIGH);
}

void SAMSUNG_send_ir_signal(uint8_t addr, uint8_t data)
{
    uint32_t data_send = 0;
    data_send = (addr << 24 & 0xFF000000) | (addr << 16 & 0xFF0000) | (data << 8 & 0xFF00) | (~data & 0xFF);

    send_pulse_high(SAMSUNG_HEADER_MARK);
    send_pulse_low(SAMSUNG_HEADER_SPACE);
    for (int i = 0; i < 32; i++)
    {
        send_pulse_high(SAMSUNG_BIT_HIGH);

        if (data_send & 0x80000000)
        {
            send_pulse_low(SAMSUNG_BIT_LOW_ONE);
        }
        else
        {
            send_pulse_low(SAMSUNG_BIT_LOW_ZERO);
        }
        data_send <<= 1;
    }
    send_pulse_high(SAMSUNG_BIT_HIGH);
}

void LG_send_ir_signal(uint8_t addr, uint8_t cmd) // 24 bit
{
    uint32_t data_send = 0;
    data_send = (addr << 16 & 0xFF0000) | ((cmd) << 8 & 0xFF00) | (~cmd & 0xFF);
    printf("LG send: %lu", data_send);
    send_pulse_high(LG_HEADER_MARK);
    send_pulse_low(LG_HEADER_SPACE);
    for (int i = 0; i < 24; i++)
    {
        send_pulse_high(LG_BIT_HIGH);

        if (data_send & 0x01)
        {
            send_pulse_low(LG_BIT_LOW_ONE);
        }
        else
        {
            send_pulse_low(LG_BIT_LOW_ZERO);
        }
        data_send >>= 1;
    }
    send_pulse_high(LG_BIT_HIGH);
}

void SHARP_send_ir_signal(uint8_t addr, uint8_t cmd){
    int i = 0;
    uint16_t data_send = 0, data_send2 = 0;
    data_send = cmd<<5 | addr;
    data_send2 = (~cmd <<5 & 0x1FE0) | addr;

    for ( i = 0; i < 13; i++)
    {
        send_pulse_high(SHARP_MARK);
        if (data_send & 0x01)
        {
            send_pulse_low(SHARP_SPACE_BIT_ONE);
        }else{
            send_pulse_low(SHARP_SPACE_BIT_ZERO);
        }
        data_send>>=1;
        
    }
    send_pulse_high(SHARP_MARK);
    send_pulse_low(SHARP_SPACE_BIT_ONE);
    send_pulse_high(SHARP_MARK);
    send_pulse_low(SHARP_SPACE_BIT_ZERO);
    send_pulse_high(SHARP_MARK);
    send_pulse_low(46000);
    
    for ( i = 0; i < 13; i++)
    {
        send_pulse_high(SHARP_MARK);
        if (data_send2 & 0x01)
        {
            send_pulse_low(SHARP_SPACE_BIT_ONE);
        }else{
            send_pulse_low(SHARP_SPACE_BIT_ZERO);
        }
        data_send2>>=1;
    }
    send_pulse_high(SHARP_MARK);
    send_pulse_low(SHARP_SPACE_BIT_ZERO);
    send_pulse_high(SHARP_MARK);
    send_pulse_low(SHARP_SPACE_BIT_ONE);
    send_pulse_high(SHARP_MARK);
}



sirc_type check_type_SIRC(void)
{
    if (IR_data[33] == 1)
    {
        printf("SIRC 20 bit\n");
        return SIRC_20_BIT;
    }
    else if (IR_data[27] == 1 && IR_data[33] != 1)
    {
        printf("SIRC 15 bit\n");
        return SIRC_15_BIT;
    }
    else
    {
        printf("SIRC 12 bit\n");
        return SIRC_12_BIT;
    }
}

bool check_casper_AC(void)
{
    int i =0;
    uint8_t data_check = 0;
    for ( i = 4; i <= 18; i += 2)
    {
        //printf("0x%02X ",IR_duration[i]);
        if (IR_duration[i] * 50 > NEC_BIT_LOW_ZERO - 150 && IR_duration[i] * 50 < NEC_BIT_LOW_ZERO + 150)
        {
            //printf("0 ");
            data_check <<= 1;
        }
        else if (IR_duration[i] * 50 > NEC_BIT_LOW_ONE - 150 && IR_duration[i] * 50 < NEC_BIT_LOW_ONE + 150)
        {
            //printf("1 ");
            data_check <<= 1;
            data_check |= 1;
        }
    }
    //printf("\ndata check casper AC: 0x%02X\n", data_check);
    if (data_check == 0xC3)
    {
        return true;
    }
    return false;
}

ir_type check_type_IR(void)
{
    int count = 0;
    if (IR_duration[1] * 50 > NEC_HEADER_HIGH - 150 && IR_duration[1] * 50 < NEC_HEADER_HIGH + 150)
    {
        if (IR_duration[2] * 50 > NEC_HEADER_LOW - 100 && IR_duration[2] * 50 < NEC_HEADER_LOW + 100)
        {
            bool check = check_casper_AC();
            if (check)
            {
                printf("\nchuan CASPER AC!!! \n");
                return IR_CASPER_AC;
            }
            else
            {
                printf("\nchuan NEC!!! \n");
                return IR_NEC;
            }
        }
        // else if (IR_duration[2] * 50 > LG_HEADER_SPACE - 100 && IR_duration[2] * 50 < LG_HEADER_SPACE + 100)
        // {
        //     printf("chuan LG!!! \n");
        //     return IR_LG;
        // }
    }
    else if (IR_duration[1] * 50 > SIRC_HEADER - 150 && IR_duration[1] * 50 < SIRC_HEADER + 150)
    {
        printf("\nchuan SIRC!!! \n");
        return IR_SIRC;
    }
    else if (IR_duration[1] * 50 > JVC_HEADER_MARK - 150 && IR_duration[1] * 50 < JVC_HEADER_MARK + 150)
    {
        if (IR_duration[2] * 50 > JVC_HEADER_SPACE - 150 && IR_duration[2] * 50 < JVC_HEADER_SPACE + 150)
        {
            printf("\nchuan JVC!!! \n");
            return IR_JVC;
        }
    }
    else if (IR_duration[1] * 50 > SAMSUNG_HEADER_MARK - 150 && IR_duration[1] * 50 < SAMSUNG_HEADER_MARK + 150)
    {
        if (IR_duration[2] * 50 > SAMSUNG_HEADER_SPACE - 150 && IR_duration[2] * 50 < SAMSUNG_HEADER_SPACE + 150)
        {
            printf("\nchuan SAMSUNG!!! \n");
            return IR_SAMSUNG;
        }
    }

    else if (IR_duration[1] * 50 > (RC5_PULSE - 150) && IR_duration[1] * 50 < (RC5_PULSE + 150))
    {
        if (IR_duration[3] * 50 > (2 * RC5_PULSE - 150) && IR_duration[3] * 50 < (2 * RC5_PULSE + 150))
        {
            printf("\nchuan RC5!!! \n");
            return IR_RC5;
        }
    }
    else if (IR_duration[1] * 50 > MITSUBISHI_HEADER_HIGH - 150 && IR_duration[1] * 50 < MITSUBISHI_HEADER_HIGH + 150)
    {
        if (IR_duration[2] * 50 > MITSUBISHI_HEADER_LOW - 150 && IR_duration[2] * 50 < MITSUBISHI_HEADER_LOW + 150)
        {
            printf("\nchuan MITSUBISHI !!! \n");
            return IR_MITSUBISHI_AC;
        }
    }
    else if (IR_duration[1] * 50 > PANASONIC_HEADER_HIGH - 100 && IR_duration[1] * 50 < PANASONIC_HEADER_HIGH + 100)
    {
        if (IR_duration[2] * 50 > PANASONIC_HEADER_LOW - 100 && IR_duration[2] * 50 < PANASONIC_HEADER_LOW + 100)
        {
            printf("\nchuan PANASONIC AC !!! \n");
            return IR_PANASONIC_AC;
        }
    }
    else if (IR_duration[1] * 50 > MIDEA_HEADER_HIGH - 100 && IR_duration[1] * 50 < MIDEA_HEADER_HIGH + 100)
    {
        if (IR_duration[2] * 50 > MIDEA_HEADER_LOW - 100 && IR_duration[2] * 50 < MIDEA_HEADER_LOW + 100)
        {
            printf("\nchuan MIDEA AC !!! \n");
            return IR_MIDEA_AC;
        }
    }
    else
    {
        for (int i = 1; i <= 31; i+=2)
        {
            if (IR_duration[i] * 50 > (SHARP_MARK - 150) && IR_duration[i] * 50 < (SHARP_MARK + 150))
            {
                count += 1;
            }
        }
        // printf("gia tri count: %d\n", count);
        if (count == 16)
        {
            printf("\nchuan SHARP!!! \n");
            return IR_SHARP;
        }
    }

    return UNKNOWN;
}

void parse_data_NEC(void)
{
    int i = 0;
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
    uint8_t cmd = (data_rec >> 16) & 0xFF;
    printf("address device: 0x%02X \n", addr); // 5 bit
    printf("cmd is: 0x%02X \n", cmd);          // 7 bit
}

void parse_data_SIRC(void)
{
    int i = 0;
    uint8_t num_bit = 0;
    sirc_type check_sirc = check_type_SIRC();
    if (check_sirc == SIRC_12_BIT)
    {
        num_bit = 25;
    }
    else if (check_sirc == SIRC_15_BIT)
    {
        num_bit = 31;
    }
    else if (check_sirc == SIRC_20_BIT)
    {
        num_bit = 41;
    }

    for (i = num_bit; i >= 3; i -= 2)
    {                                                                                                         // check pulse
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
    if (check_sirc == SIRC_20_BIT)
    {
        extend = data_rec >> 12 & 0xFF;
        addr = data_rec >> 7 & 0x1F;
        cmd = data_rec & 0x7F;
    }
    else
    {
        addr = data_rec >> 7;
        cmd = data_rec & 0x7F;
        extend = 0;
    }
    printf("extend is: 0x%02X \n", extend);    // 8 bit
    printf("address device: 0x%02X \n", addr); // 5 or 7 bit
    printf("cmd is: 0x%02X \n", cmd);          // 7 bit
}

void parse_data_RC5(void)
{
    int i = 0;
    for (i = 4; i <= 28; i++)
    {
        if (i == 4 && IR_duration[i + 1] * 50 > (RC5_PULSE - 150) && IR_duration[i + 1] * 50 < (RC5_PULSE + 150))
            i += 1;
        if (IR_data[i] == 0 && IR_duration[i] > 0)
        {
            // printf("i=%d, bit1\n", i);
            data_rec <<= 1;
            data_rec |= 1;
        }
        else if (IR_data[i] == 1)
        {
            // printf("i=%d, bit0\n", i);
            data_rec <<= 1;
        }

        if (IR_duration[i + 1] * 50 > (RC5_PULSE - 150) && IR_duration[i + 1] * 50 < (RC5_PULSE + 150))
        {
            i++;
        }
    }
    uint8_t addr = data_rec >> 6;
    uint8_t cmd = data_rec & 0x3F;
    printf("address device: 0x%02X \n", addr); // 5 bit
    printf("cmd is: 0x%02X \n", cmd);          // 6 bit
}

void parse_data_JVC(void)
{
    int i = 0;
    for (i = 34; i >= 4; i -= 2) // ktra xung LOW
    {
        if (IR_duration[i] * 50 > JVC_BIT_LOW_ZERO - 100 && IR_duration[i] * 50 < JVC_BIT_LOW_ZERO + 100)
        {
            data_rec <<= 1;
        }
        else if (IR_duration[i] * 50 > JVC_BIT_LOW_ONE - 100 && IR_duration[i] * 50 < JVC_BIT_LOW_ONE + 100)
        {
            data_rec <<= 1;
            data_rec |= 1;
        }
    }
    uint8_t cmd = data_rec >> 8 & 0xFF;
    uint8_t adr = data_rec & 0xFF;
    printf("cmd is: 0x%02X \n", cmd); // 8 bit
    printf("adr is: 0x%02X \n", adr); // 8 bit
}

void parse_data_SAMSUNG(void)
{
    int i = 0;
    for (i = 4; i <= 66; i += 2) // ktra xung LOW
    {
        if (IR_duration[i] * 50 > SAMSUNG_BIT_LOW_ZERO - 150 && IR_duration[i] * 50 < SAMSUNG_BIT_LOW_ZERO + 150)
        {
            data_rec <<= 1;
        }
        else if (IR_duration[i] * 50 > SAMSUNG_BIT_LOW_ONE - 150 && IR_duration[i] * 50 < SAMSUNG_BIT_LOW_ONE + 150)
        {
            data_rec <<= 1;
            data_rec |= 1;
        }
    }
    uint8_t addr = data_rec >> 24 & 0xFF;
    uint8_t data = data_rec >> 8 & 0xFF;
    printf("addr is: 0x%02X \n", addr); // 8 bit
    printf("data is: 0x%02X \n", data); // 8 bit
}

void parse_data_LG(void)
{
    int i = 0;
    for (i = 50; i >= 4; i -= 2) // ktra xung LOW
    {
        if (IR_duration[i] * 50 > LG_BIT_LOW_ZERO - 100 && IR_duration[i] * 50 < LG_BIT_LOW_ZERO + 100)
        {
            data_rec <<= 1;
        }
        else if (IR_duration[i] * 50 > LG_BIT_LOW_ONE - 100 && IR_duration[i] * 50 < LG_BIT_LOW_ONE + 100)
        {
            data_rec <<= 1;
            data_rec |= 1;
        }
    }
    uint8_t data = data_rec >> 16 & 0xFF;
    uint8_t addr = data_rec >> 8 & 0xFF;
    printf("data: 0x%02X \n", data);    // 8 bit
    printf("addr is: 0x%02X \n", addr); // 8 bit
}

void parse_data_SHARP(void)
{
    int i = 0;
    if (IR_duration[28] * 50 > SHARP_SPACE_BIT_ONE - 150 && IR_duration[28] * 50 < SHARP_SPACE_BIT_ONE + 150)
    {
        if (IR_duration[30] * 50 > SHARP_SPACE_BIT_ZERO - 150 && IR_duration[30] * 50 < SHARP_SPACE_BIT_ZERO + 150)
        {
            for (i = 26; i >= 2; i -= 2) // ktra xung H
            {
                if (IR_duration[i] * 50 > SHARP_SPACE_BIT_ZERO - 150 && IR_duration[i] * 50 < SHARP_SPACE_BIT_ZERO + 150)
                {
                    data_rec <<= 1;
                }
                else if (IR_duration[i] * 50 > SHARP_SPACE_BIT_ONE - 150 && IR_duration[i] * 50 < SHARP_SPACE_BIT_ONE + 150)
                {
                    data_rec <<= 1;
                    data_rec |= 1;
                }
            }
            uint8_t cmd = data_rec >> 5 & 0xFF;
            uint8_t addr = data_rec & 0x1F;
            printf("cmd is: 0x%02X \n", cmd);   // 8 bit
            printf("addr is: 0x%02X \n", addr); // 5 bit
        }
    }
}

uint8_t get_byte(uint8_t num_bit, uint16_t bit0, uint16_t bit1)
{
    int i = 0;
    uint8_t data = 0;
    for (i = (num_bit + 7) * 2 + 4; i >= num_bit*2+4; i -= 2)
    {
        if (IR_duration[i] * 50 > bit0 - 150 && IR_duration[i] * 50 < bit0 + 150)
        {
            data <<= 1;
        }
        else if (IR_duration[i] * 50 > bit1 - 150 && IR_duration[i] * 50 < bit1 + 150)
        {
            data <<= 1;
            data |= 1;
        }
    }
    return data;
}
uint8_t data_Casper_AC(get_data_Casper_AC get_data)
{
    uint8_t data = 0;
    uint8_t num_bit = 0;
    switch (get_data)
    {
    case CAS_TEMP_SW:
        num_bit = 8;//20;
        break;
    case CAS_SW_HOR:
        num_bit = 16;//36;
        break;
    case CAS_FAN:
        num_bit = 32;//68;
        break;
    case CAS_MODE:
        num_bit = 42;//100;
        break;
    case CAS_ON_OFF:
        num_bit = 72;//148;
        break;
    case CAS_ID_BUTTON:
        num_bit = 88;//180;
        break;
    default:
        break;
    }
    data = get_byte(num_bit, NEC_BIT_LOW_ZERO, NEC_BIT_LOW_ONE);
    return data;
}

uint8_t data_Mitsubishi_AC(get_data_Mitsubishi_AC get_data){
    uint8_t data = 0;
    uint8_t num_bit = 0;
    switch (get_data)
    {
    case MIT_SWING:
        num_bit = 40;//84;  // byte 6 trong frame truyen di
        break;
    case MIT_FAN:
        num_bit = 56;//116;
        break;
    case MIT_TEMP_MODE:
        num_bit = 72;//148;
        break;    
    default:
        break;
    }
    data = get_byte(num_bit, MITSUBISHI_BIT_LOW_ZERO, MITSUBISHI_BIT_LOW_ONE);
    return data;
}

void parse_data_CASPER_AC(void)
{
    Casper_AC_para casper_data ={
        .temp_swing = data_Casper_AC(CAS_TEMP_SW),
        .swing_hor = data_Casper_AC(CAS_SW_HOR),
        .fan = data_Casper_AC(CAS_FAN),
        .mode = data_Casper_AC(CAS_MODE),
        .on_off = data_Casper_AC(CAS_ON_OFF),
        .id_button = data_Casper_AC(CAS_ID_BUTTON),
    };

    printf("temp_swing is: 0x%02X \n",casper_data.temp_swing);
    printf("swing_hor is: 0x%02X \n", casper_data.swing_hor);
    printf("fan is: 0x%02X \n", casper_data.fan);
    printf("mode is: 0x%02X \n", casper_data.mode);
    printf("on_off is: 0x%02X \n", casper_data.on_off);
    printf("id_button is: 0x%02X \n", casper_data.id_button);

    //memset(&casper_data, 0 , sizeof(casper_data));
}

void parse_data_MISUBISHI_AC(void){
    Mitsubishi_AC_para mitsu_data = {
        .Swing = data_Mitsubishi_AC(MIT_SWING),
        .fan = data_Mitsubishi_AC(MIT_FAN),
        .temp_mode = data_Mitsubishi_AC(MIT_TEMP_MODE),
    };
    printf("swing is: 0x%02X \n",mitsu_data.Swing);
    printf("fan is: 0x%02X \n", mitsu_data.fan);
    printf("mode temp is: 0x%02X \n", mitsu_data.temp_mode);
}

void parse_data_PANASONIC_AC(void){
    int i = 0;
    uint8_t data_r = 0;
    uint8_t num_bit = 0;
    for (i = 0; i < 12; i++)
    {
        num_bit = i*8;
        data_r = get_byte(num_bit,PANASONIC_BIT_LOW_ZERO, PANASONIC_BIT_LOW_ONE);
        printf("data[%d] is 0x%02X \n",i,data_r);
    }
}

void parse_data_MIDEA_AC(void){
    int i = 0;
    uint8_t data_r = 0;
    uint8_t num_bit = 0;
    for (i = 0; i < 6; i++)
    {
        num_bit = i*8;
        data_r = get_byte(num_bit,MIDEA_BIT_LOW_ZERO, MIDEA_BIT_LOW_ONE);
        printf("data[%d] is 0x%02X \n",i,data_r);
    }
}


void parse_data(void)
{

    ir_type check_type = check_type_IR();
    switch (check_type)
    {
    case IR_NEC:
        parse_data_NEC();
        break;
    case IR_SIRC:
        parse_data_SIRC();
        break;
    case IR_RC5:
        parse_data_RC5();
        break;
    case IR_JVC:
        parse_data_JVC();
        break;
    case IR_SAMSUNG:
        parse_data_SAMSUNG();
        break;
    case IR_SHARP:
        parse_data_SHARP();
        break;
        /*----------------------------------------------------------------------------------------*/
    case IR_CASPER_AC:
        parse_data_CASPER_AC();
        break;
    case IR_MITSUBISHI_AC:
        parse_data_MISUBISHI_AC();
        break;
    case IR_PANASONIC_AC:
        parse_data_PANASONIC_AC();
        break;
    case IR_MIDEA_AC:
        parse_data_MIDEA_AC();
        break;
    default:
        break;
    }
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

        // SIRC_send_ir_signal(0x01, 0x15, 0, SIRC_12_BIT); // SIRC
        // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0);     // Tắt PWM
        // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(5000 / portTICK_PERIOD_MS);

        // RC5_send_ir_signal(0x05, 0x32);
        // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(5000/portTICK_PERIOD_MS);

        // JVC_send_ir_signal(0x05, 0x32);
        // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(5000/portTICK_PERIOD_MS);

        // SAMSUNG_send_ir_signal(0x05, 0x32);
        // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(5000/portTICK_PERIOD_MS);

        // LG_send_ir_signal(0x05, 0x32);
        // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(5000/portTICK_PERIOD_MS);

        SHARP_send_ir_signal(0x01,0x16);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0); // Tắt PWM
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}

void task_rec_ir(void *para)
{

    while (1)
    {
        // for (int i = 0; i < 5; i++)
        // {
        //     printf("data receive IR_data[%d]: %01X, duration time: %u \n", i, IR_data[i], IR_duration[i]);
        //     vTaskDelay(10/portTICK_PERIOD_MS);
        // }
        // printf("---------------------------------------------------\n");

        parse_data();

        data_rec = 0;
        memset(IR_data, -1, sizeof(IR_data));
        memset(IR_duration, 0, sizeof(IR_duration));

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void init_task_IR(void)
{
    memset(IR_data, -1, sizeof(IR_data));
    //xTaskCreate(task_send_ir, "send_task", 4096, NULL, 10, NULL);
    xTaskCreate(task_rec_ir, "rec_task", 4096, NULL, 10, NULL);
}