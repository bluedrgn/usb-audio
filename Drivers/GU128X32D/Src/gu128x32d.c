
#include "gu128x32d.h"
#include "main.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_uart.h"
#include <stdint.h>

#define PAGE_HEIGHT (8)

static gu128x32d_instance_t *__display = NULL;

void gu128x32d_init(gu128x32d_instance_t *D) {
  const uint8_t init_sequence[] = {
    0x1b, 0x40, /* initialize display */
    0x1f, 0x43, 0, /* cursor off */
    0x0c, /* display cleatr */
    0x1f, 0x28, 0x67, 0x03, 0, /* fixed character width 1 */
    0x1f, 0x28, 0x67, 0x40, 1, 1, /* font magnification = 1,1 */
    0x1f, 0x28, 0x77, 0x01, 0 /* current window = Base */
  };

  HAL_UART_Transmit(D->huart, init_sequence, sizeof(init_sequence), 100);
}

static void _flush(gu128x32d_instance_t *D) {
  static uint8_t *buf_tmp;

  if (D->_flush_state == 0) {
    buf_tmp = D->FB;
    #ifdef GU128X32D_DOUBLEBUFFER
    /* swap buffers */
    D->FB = D->FB2;
    D->FB2 = buf_tmp;
    #endif
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
  }

  if (D->_flush_state >= 2) {
    /* flush complete */
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
    D->_flush_state = 0;
    gu128x32d_flush_complete_callback(D);
    return;
  }

  if (HAL_UART_GetState(D->huart) != HAL_UART_STATE_READY) {
    Error_Handler();
  }

  if ((D->_flush_state & 1) == 0) {
    static uint8_t command[] = {
      0x1f, 0x24, 0, 0, 0, 0, /* cursor set 0,0 */
      0x1f, 0x28, 0x66, 0x11, GU128X32D_SCR_W, 0,
      GU128X32D_SCR_H/PAGE_HEIGHT, 0, 1 /* Real-time bit image display */
    };
    HAL_UART_Transmit_DMA(D->huart, command, sizeof(command));
  }
  else {
    HAL_UART_Transmit_DMA(D->huart, buf_tmp, GU128X32D_FB_SIZE);
  }

  D->_flush_state++;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == __display->huart) {
    _flush(__display);
  }
}

void gu128x32d_flush(gu128x32d_instance_t *D) {
  while (!gu128x32d_poll_flush_complete(D)) {
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON,
                           PWR_SLEEPENTRY_WFE_NO_EVT_CLEAR);
  }
  __display = D;
  _flush(D);
}

bool gu128x32d_poll_flush_complete(gu128x32d_instance_t *D) {
  return (D->_flush_state == 0) ? true : false;
}

__weak void gu128x32d_flush_complete_callback(gu128x32d_instance_t *D) {}
