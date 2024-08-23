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
#define SIRC_HEADER         2400

#define NEC_BIT_HIGH        560
#define NEC_BIT_LOW_ZERO    560
#define NEC_BIT_LOW_ONE     1690

#define SIRC_BIT_LOW        600
#define SIRC_BIT_HIGH_ZERO  600
#define SIRC_BIT_HIGH_ONE   1200

#define RC5_PULSE           890

extern uint16_t IR_duration[100];
extern int8_t IR_data[100];

extern uint32_t data_rec;
extern volatile int duration;
extern volatile int indx_ir;

typedef enum{
    UNKNOWN = 0,
    IR_NEC,
    IR_SIRC,
    IR_RC5
}ir_type;

typedef enum{
    SIRC_12_BIT=0,
    SIRC_15_BIT,
    SIRC_20_BIT
}sirc_type;

void task_send_ir(void *para);
void task_rec_ir(void *para);
void init_task_IR(void);



#endif /* _MY_IR_H */
