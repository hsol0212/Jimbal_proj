#include "uart.h"
#include "usart.h"

// 두 개의 UART 핸들을 모두 가져옵니다.
extern UART_HandleTypeDef huart6; // Board B 통신용
extern UART_HandleTypeDef huart2; // PC 테라텀 디버그용

// FreeRTOS 멀티태스킹 충돌 방지용 뮤텍스
static osMutexId_t uart6_tx_mutex = NULL;
static osMutexId_t uart2_tx_mutex = NULL;

bool uartInit(void) {
    if (uart6_tx_mutex == NULL) {
        uart6_tx_mutex = osMutexNew(NULL);
    }
    if (uart2_tx_mutex == NULL) {
        uart2_tx_mutex = osMutexNew(NULL);
    }
    return true;
}

// [핵심] Board B로 추적 좌표 패킷 전송 (무조건 USART6 사용)
// 패킷: [0x02][cx_H][cx_L][cy_H][cy_L][detected][0x03] = 7바이트
void uartSendTrackData(int cx, int cy, int area, bool detected) {
    uint8_t packet[9];
    packet[0] = 0x02;                    // STX
    packet[1] = (cx >> 8) & 0xFF;
    packet[2] =  cx & 0xFF;
    packet[3] = (cy >> 8) & 0xFF;
    packet[4] =  cy & 0xFF;
    
    // Area 2바이트 추가
    packet[5] = (area >> 8) & 0xFF;
    packet[6] =  area & 0xFF;
    
    packet[7] = detected ? 0x01 : 0x00;
    packet[8] = 0x03;                    // ETX

    if (uart6_tx_mutex == NULL) return;
    
    osMutexAcquire(uart6_tx_mutex, osWaitForever);
    HAL_UART_Transmit(&huart6, packet, 9, 100); // 길이 7 -> 9로 수정!
    osMutexRelease(uart6_tx_mutex);
}

// [유틸리티] 채널(ch) 번호에 따라 원하는 UART로 데이터를 쏘는 함수
// ch == 1 : PC 테라텀 (USART2) 
// ch == 2 : Board B (USART6)
uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t len) {
    UART_HandleTypeDef *p_huart;
    osMutexId_t target_mutex;

    // 채널에 따라 목적지(UART)와 자물쇠(Mutex)를 선택
    if (ch == 1) {
        p_huart = &huart2;
        target_mutex = uart2_tx_mutex;
    } else if (ch == 2) {
        p_huart = &huart6;
        target_mutex = uart6_tx_mutex;
    } else {
        return 0; // 잘못된 채널
    }

    if (target_mutex == NULL) return 0;

    osMutexAcquire(target_mutex, osWaitForever);
    HAL_UART_Transmit(p_huart, p_data, len, 200);
    osMutexRelease(target_mutex);

    return len;
}