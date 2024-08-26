#ifndef _MY_IR_H
#define _MY_IR_H


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"



#define NEC_HEADER_HIGH     9000
#define NEC_HEADER_LOW      4500
#define NEC_BIT_HIGH        560
#define NEC_BIT_LOW_ZERO    560
#define NEC_BIT_LOW_ONE     1690

#define SIRC_HEADER         2400
#define SIRC_BIT_LOW        600
#define SIRC_BIT_HIGH_ZERO  600
#define SIRC_BIT_HIGH_ONE   1200

#define RC5_PULSE           890

#define JVC_HEADER_MARK  8440
#define JVC_HEADER_SPACE 4220
#define JVC_BIT_HIGH     527
#define JVC_BIT_LOW_ZERO 527
#define JVC_BIT_LOW_ONE  1583

#define SAMSUNG_HEADER_MARK 4500
#define SAMSUNG_HEADER_SPACE 4500
#define SAMSUNG_BIT_HIGH     560
#define SAMSUNG_BIT_LOW_ZERO 560
#define SAMSUNG_BIT_LOW_ONE  1690

#define LG_HEADER_MARK 9000 // ???????????
#define LG_HEADER_SPACE 4200
#define LG_BIT_HIGH     500
#define LG_BIT_LOW_ZERO 550
#define LG_BIT_LOW_ONE  1580

#define SHARP_MARK     320
#define SHARP_SPACE_BIT_ONE 1800
#define SHARP_SPACE_BIT_ZERO 800

#define MITSUBISHI_HEADER_HIGH 3160
#define MITSUBISHI_HEADER_LOW 1600
#define MITSUBISHI_BIT_HIGH 360
#define MITSUBISHI_BIT_LOW_ZERO 420
#define MITSUBISHI_BIT_LOW_ONE 1170


extern uint16_t IR_duration[500];
extern int8_t IR_data[500];

extern uint32_t data_rec;
extern volatile int duration;
extern volatile int indx_ir;

typedef enum{
    UNKNOWN = 0,
    IR_NEC,
    IR_SIRC,
    IR_RC5,
    IR_JVC,
    IR_SAMSUNG,
    IR_LG,
    IR_SHARP,
    IR_CASPER_AC,   //NEC
    IR_MITSUBISHI_AC
}ir_type;

typedef enum{
    SIRC_12_BIT=0,
    SIRC_15_BIT,
    SIRC_20_BIT
}sirc_type;

typedef enum{
    CAS_TEMP_SW =0,
    CAS_SW_HOR,
    CAS_FAN,
    CAS_MODE,
    CAS_ON_OFF,
    CAS_ID_BUTTON
}get_data_Casper_AC;
typedef struct 
{
    uint8_t temp_swing;
    uint8_t swing_hor;
    uint8_t fan;
    uint8_t mode;
    uint8_t on_off;
    uint8_t id_button;
}Casper_AC_para;

typedef enum{
    MIT_SWING =0,
    MIT_FAN,
    MIT_TEMP_MODE
}get_data_Mitsubishi_AC;
typedef struct 
{
    uint8_t Swing;
    uint8_t fan;
    uint8_t temp_mode;
}Mitsubishi_AC_para;


void task_send_ir(void *para);
void task_rec_ir(void *para);
void init_task_IR(void);



#endif /* _MY_IR_H */
