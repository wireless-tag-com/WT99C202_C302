#pragma once

#include <stdint.h>
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t msg_type;  /* 消息类型; */
    uint8_t* buffer;  // 数据
    uint16_t len;     // 数据长度
} ci1302_uart_ota_frame_t;

typedef struct {
    uint8_t head0;          // 包头高字节
    uint8_t head1;          // 包头低字节
    uint8_t len0;           // 长度高字节
    uint8_t len1;           // 长度低字节
    uint8_t msg_type;       // 消息类型
    uint8_t crc0;           // CRC高字节
    uint8_t crc1;           // CRC低字节
    uint8_t tail;           // 包尾
} __attribute__((packed)) cias_standard_ota_head_t;
// V3协议数据包结构
#pragma pack(1)


void ci1302_protocol_ota_init(uint8_t uart_num, uint8_t tx_pin, uint8_t rx_pin, uint32_t baudrate);

/**
 * @brief 发送一帧数据, 使用协议发送一帧数据
 * @param msg_type 消息类型
 * @param data 数据
 * @param len 数据长度
 */
void ci1302_protocol_ota_write_bytes(uint8_t msg_type, const uint8_t* data, uint16_t len);

void ci1302_protocol_ota_write_bytes_multi(uint8_t msg_type, uint8_t frame_nums, ...);

int ci1302_protocol_ota_recv_frame(ci1302_uart_ota_frame_t* frame, uint32_t timeout_ms);

void ci1302_protocol_ota_free_frame_buffer(ci1302_uart_ota_frame_t* frame);

#ifdef __cplusplus
}
#endif
