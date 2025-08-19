#pragma once

#include <stdint.h>
#include <stddef.h>

// 电池事件类型定义
#define BAT_EVENT_LOW_POWER     1    // 低电量警告 (<=20%)
#define BAT_EVENT_CRITICAL      2    // 极低电量 (<=5%)
#define BAT_EVENT_USB_PLUG      3    // USB插入
#define BAT_EVENT_USB_UNPLUG    4    // USB拔出
#define BAT_EVENT_CHARGE_FULL   5    // 充电完成
#define BAT_EVENT_PERCENT_CHANGE 6   // 电量百分比变化

/**
 * @brief Initialize the battery monitoring system.
 * 仅需要ADC引脚，通过电压检测充电状态和电量
 *
 * @param adc_pin ADC pin number for battery voltage measurement.
 */
void bat_init(int adc_pin);

/**
 * @brief Get the current battery percentage.
 *
 * @return uint8_t Battery percentage (0-100).
 */
uint8_t bat_get_percent(void);

/**
 * @brief Check if USB power is valid.
 *
 * @return uint8_t 1 if USB power is valid, 0 otherwise.
 */
uint8_t bat_get_usb_valid(void);

/**
 * @brief Get the current battery voltage in millivolts.
 *
 * @return uint32_t Battery voltage in millivolts.
 */
uint32_t bat_get_volt_mv(void);

/**
 * @brief Check if the battery is currently charging.
 *
 * @return uint8_t 1 if the battery is charging, 0 otherwise.
 */
uint8_t bat_in_charge(void);

/**
 * @brief 设置电池事件回调函数
 *
 * @param callback 回调函数指针，参数：event_type(事件类型), data(事件数据)
 */
void bat_set_event_callback(void (*callback)(uint8_t event_type, uint32_t data));

/**
 * @brief 获取电池健康度百分比
 *
 * @return uint32_t 电池健康度 (0-100)
 */
uint32_t bat_get_health_percent(void);

/**
 * @brief 获取电池充放电循环次数
 *
 * @return uint32_t 循环次数
 */
uint32_t bat_get_cycle_count(void);

/**
 * @brief 检查电池是否充满
 *
 * @return uint8_t 1表示充满，0表示未充满
 */
uint8_t bat_is_full(void);

/**
 * @brief 获取电池状态信息字符串
 *
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 */
void bat_get_status_string(char* buf, size_t buf_size);
