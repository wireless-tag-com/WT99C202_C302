#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 初始化并维护电池管理系统
 * 包含GPIO设置和电池监控系统启动
 */
void battery_manage_init(void);

/**
 * @brief 获取电池信息的便捷函数
 * @param percent 电量百分比输出指针 (可为NULL)
 * @param voltage_mv 电压输出指针 (mV) (可为NULL)
 * @param is_charging 充电状态输出指针 (可为NULL)
 * @param usb_connected USB连接状态输出指针 (可为NULL)
 */
void get_battery_info(uint8_t* percent, uint32_t* voltage_mv, bool* is_charging, bool* usb_connected);

/**
 * @brief 检查是否需要进入低功耗模式
 * @return true 建议进入省电模式, false 正常运行
 */
bool should_enter_power_saving_mode(void);

/**
 * @brief 播放当前电池状态的TTS语音提示
 * 包含电量百分比、充电状态和电压信息
 */
void battery_speak_status(void);

/**
 * @brief 电池关机函数
 * 通过设置PW_KEEP_PIN为低电平来关闭设备电源
 */
void battery_shutdown(void);

