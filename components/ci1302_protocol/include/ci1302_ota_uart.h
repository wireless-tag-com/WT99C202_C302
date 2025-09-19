#pragma once

#include <stdint.h>
#include "ci1302_protocol.h"
#include "ci1302_ota_uart_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t version;
    uint32_t addr;
    uint32_t size;
    uint32_t crc;
    uint8_t status;
} __attribute__((packed)) partition_single_t;

typedef struct
{
    uint32_t manu_facturer_id;    //32Bit
    uint32_t product_id[2];      //64Bit (MAC Address)
//    
    uint32_t hard_ware_name[16];        //String
    uint32_t hard_ware_version;         //Vm.n.x.y
    uint32_t soft_ware_name[16];        //String,Exporting to Packet file name
    uint32_t soft_ware_version;         //Vm.n.x.y
//    
    uint32_t bootloader_version; //Vm.n.x.y
    char ChipName[9];
    uint8_t FirmwareFormatVer;
    uint8_t reserve[4];
//   
    partition_single_t user_code1;
    partition_single_t user_code2;
    partition_single_t asr_cmd_mode;
    partition_single_t dnn_model;
    partition_single_t voice;
    partition_single_t user_file;

    uint32_t nv_data_offset;
    uint32_t nv_data_size;

    uint16_t partition_table_checksum;
} __attribute__((packed)) partition_table_t; 

// V3协议配置
#define OTA_INVALID_HEAD_LENGTH            0x8000                 // OTA时只需要从0x8000开始取数据，前面是boot等无效信息
#define OTA_V3_UART_BAUDRATE               921600                 // 最大3000000
#define OTA_PACK_LENGTH                    4096                   // 每包有效数据长度

// V3协议消息头尾定义
#define MSG_HEAD_HIGH                      0xA5
#define MSG_HEAD_LOW                       0x0F
#define MSG_TAIL                           0xFF

// V3协议消息类型
#define MSG_TYPE_OTA_VERSION           0xA0
#define MSG_TYPE_OTA_START             0xA1
#define MSG_TYPE_OTA_DATA              0xA2                // 数据传输消息类型
#define MSG_TYPE_OTA_FIRMWARE          0xA2                // 与MSG_TYPE_OTA_DATA相同，保持兼容性
#define MSG_TYPE_OTA_FINISH            0xA3
#define MSG_TYPE_OTA_REQUEST           0xA4

typedef esp_err_t (*ci1302_ota_get_firmware_data_cb_t)(uint16_t packet_id, uint8_t* data, uint16_t data_len);

/**
 * @brief 开始OTA升级
 * @param firmware_data 固件数据指针
 * @param firmware_size 固件大小
 * @return true 成功, false 失败
 */
bool ci1302_ota_v3_start_update(const uint8_t* firmware_data, uint32_t firmware_size);

///////////////////////V3协议///////////////////////
void ci1302_uart_ota_init(uint8_t uart_num, uint8_t tx_pin, uint8_t rx_pin, uint32_t baudrate);

void ci1302_uart_enter_ota_mode(void);

void ci1302_uart_send_ota_start(uint8_t* version, uint32_t firmware_size);

void ci1302_uart_write_ota_data(uint16_t packet_id, const uint8_t* data, uint16_t data_len);

// 获取从固件中提取的信息
bool ci1302_ota_get_extracted_firmware_info(uint8_t version[3], uint32_t *package_count);

void ci1302_ota_v3_data_check_version(void);

void ci1302_ota_v3_send_firmware_data(const uint8_t* data, uint16_t data_len);

void ci1302_ota_v3_data_finish(void);

bool ci1302_ota_v3_verify_ota_ack(uint8_t *data, const uint8_t recv_msg_type);

void ci1302_ota_uart_set_firmware_data_cb(ci1302_ota_get_firmware_data_cb_t cb);

#ifdef __cplusplus
}
#endif