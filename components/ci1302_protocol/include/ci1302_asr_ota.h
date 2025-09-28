#pragma once

#include <stdint.h>
#include "esp_err.h"

#include "ci1302_ota_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

void ci1302_asr_ota_init(const uint8_t* firmware_data, uint32_t firmware_size);

esp_err_t ci1302_asr_ota_get_firmware_data_cb(uint16_t packet_id, uint8_t* data, uint16_t data_len);

#ifdef __cplusplus
}
#endif
