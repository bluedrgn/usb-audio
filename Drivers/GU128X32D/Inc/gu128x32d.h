#pragma once

#include <memory.h>
#include <stdint.h>
#include <stdbool.h>
#include <stm32f4xx_ll_usart.h>

#define GU128X32D_SCR_W (128)
#define GU128X32D_SCR_H (32)
#define GU128X32D_FB_SIZE (GU128X32D_SCR_W * (GU128X32D_SCR_H / 8))

#define GU128X32D_DOUBLEBUFFER

typedef struct {
    UART_HandleTypeDef* huart;
    uint8_t *FB;
    #ifdef GU128X32D_DOUBLEBUFFER
    uint8_t *FB2;
    #endif
    volatile uint8_t _flush_state;
} gu128x32d_instance_t;

void gu128x32d_init(gu128x32d_instance_t *display);
void gu128x32d_flush(gu128x32d_instance_t *display);
bool gu128x32d_poll_flush_complete(gu128x32d_instance_t *display);
void gu128x32d_flush_complete_callback(gu128x32d_instance_t *display);
