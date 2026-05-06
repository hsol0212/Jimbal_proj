#ifndef _UART_H_
#define _UART_H_

#include "hw_def.h"
#include <stdbool.h>

bool     uartInit(void);
// area 매개변수 추가
void     uartSendTrackData(int cx, int cy, int area, bool detected);
uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t len);

#endif