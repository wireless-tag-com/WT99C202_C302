/**
 * @file bat_status.c
 * @brief 电池管理系统状态监控和事件处理
 *
 * 展示如何在现有项目中无缝集成电池管理功能
 *
 * ⚠️  重要提醒：格式化 uint32_t 类型时必须使用 PRIu32 宏！
 *     正确：ESP_LOGI(TAG, "电压: %" PRIu32 "mV", voltage);
 *     错误：ESP_LOGI(TAG, "电压: %dmV", voltage);  // 编译错误！
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

// 原有的头文件
#include "qmsd_board_pin.h"
#include "qmsd_utils.h"
// ... 其他原有头文件

// 新增：电池管理头文件
#include "bat.h"
#include "chat_notify.h"  // 通知播放功能

#define TAG "BATTERY"
#define BAT_STATUS_CHECK_INTERVAL 10000

// 电池状态全局变量（可选）
static uint8_t g_battery_percent = 0;
static uint8_t g_battery_low_warning = 0;
static uint8_t system_fully_started = 0;       // 系统是否完全启动标志

/**
 * @brief 电池事件回调处理函数
 * 整合到您的系统事件处理中
 */
void system_battery_event_handler(uint8_t event_type, uint32_t data) {
    switch (event_type) {
        case BAT_EVENT_LOW_POWER:
            ESP_LOGW(TAG, "🔋 系统低电量警告！剩余: %d%%", (int)data);
            g_battery_low_warning = 1;

            // 低电量处理逻辑：
            // 1. 降低系统功耗（降低CPU频率、关闭不必要外设）
            // 2. 暂停非关键任务
            // 3. 显示低电量指示灯
            // 4. 播放低电量提示音（仅在系统完全启动后）
            if (system_fully_started) {
                chat_notify_audio_play(NOTIFY_BAT_LOW_POWER, NULL);
            }
            break;

        case BAT_EVENT_CRITICAL:
            ESP_LOGE(TAG, "🚨 电池电量极低！剩余: %d%% - 即将关机", (int)data);

            // 紧急处理逻辑：
            // 1. 保存重要数据到NVS
            // 2. 停止所有非关键任务
            // 3. 准备系统关机
            if (system_fully_started) {
                chat_notify_audio_play(NOTIFY_BAT_CRITICAL, NULL);
            }
            // save_critical_data_to_nvs();
            // prepare_system_shutdown();
            break;

        case BAT_EVENT_USB_PLUG:
            ESP_LOGI(TAG, "🔌 USB电源已连接, 切换到外部供电");
            g_battery_low_warning = 0;  // 清除低电量警告

            // USB插入处理：
            // 1. 恢复正常功耗模式
            // 2. 重启被暂停的任务
            // 3. 指示充电状态（仅在系统完全启动后播放音频）
            if (system_fully_started) {
                chat_notify_audio_play(NOTIFY_BAT_USB_PLUG, NULL);
            }
            // restore_normal_power_mode();
            break;

        case BAT_EVENT_USB_UNPLUG:
            ESP_LOGI(TAG, "🔌 USB电源已断开, 切换到电池供电");


            // USB拔出处理：
            // 1. 根据当前电量调整功耗策略
            // 2. 开始电池供电模式
            uint8_t current_percent = bat_get_percent();
            if (current_percent <= 20) {
                // 进入省电模式
                // enter_power_saving_mode();
            }
            break;

        case BAT_EVENT_CHARGE_FULL:
            ESP_LOGI(TAG, "✅ 电池充电完成");
            // 充电完成提示（仅在系统完全启动后播放音频）
            if (system_fully_started) {
                chat_notify_audio_play(NOTIFY_BAT_CHARGE_FULL, NULL);
            }
            break;

        case BAT_EVENT_PERCENT_CHANGE:
            g_battery_percent = (uint8_t)data;
            ESP_LOGD(TAG, "📊 电池电量更新: %d%%", g_battery_percent);

            // 可以在这里更新UI显示、LED指示等
            // update_battery_ui_display(g_battery_percent);
            break;

        default:
            ESP_LOGW(TAG, "未知电池事件: %d", event_type);
            break;
    }
}

/**
 * @brief 获取电池信息的便捷函数
 * 可以被其他模块调用
 */
void get_battery_info(uint8_t* percent, uint32_t* voltage_mv, bool* is_charging, bool* usb_connected) {
    if (percent) *percent = bat_get_percent();
    if (voltage_mv) *voltage_mv = bat_get_volt_mv();
    if (is_charging) *is_charging = bat_in_charge();
    if (usb_connected) *usb_connected = bat_get_usb_valid();
}

/**
 * @brief 检查是否需要进入低功耗模式
 * 可以集成到现有的功耗管理中
 */
bool should_enter_power_saving_mode(void) {
    uint8_t percent = bat_get_percent();
    bool usb_connected = bat_get_usb_valid();

    // 电量低于20%且不在充电时, 建议进入省电模式
    return (percent <= 20 && !usb_connected);
}

/**
 * @brief 播放当前电池状态的TTS语音提示
 * 包含电量百分比和充电状态
 */
void battery_speak_status(void) {
    uint8_t percent = bat_get_percent();
    uint32_t voltage = bat_get_volt_mv();
    uint8_t is_charging = bat_in_charge();
    uint8_t usb_valid = bat_get_usb_valid();

    char tts_text[128];

    if (usb_valid) {
        if (is_charging) {
            snprintf(tts_text, sizeof(tts_text),
                    "当前电量百分之%d, 正在充电中, 电压%" PRIu32 "毫伏",
                    percent, voltage);
        } else {
            snprintf(tts_text, sizeof(tts_text),
                    "当前电量百分之%d, 充电已完成, 电压%" PRIu32 "毫伏",
                    percent, voltage);
        }
    } else {
        snprintf(tts_text, sizeof(tts_text),
                "当前电量百分之%d, 使用电池供电, 电压%" PRIu32 "毫伏",
                percent, voltage);
    }

    ESP_LOGE(TAG, "播放电池状态: %s", tts_text);
    // aiha_request_tts_async(tts_text);
}

/**
 * @brief 电池状态定期检查任务
 * 可以集成到现有的系统监控任务中
 */
void battery_status_check_task(void* param) {
    uint32_t check_count = 0;

    ESP_LOGI(TAG, "电池监控任务开始运行...");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(BAT_STATUS_CHECK_INTERVAL));
        check_count++;

        // 简化的电池状态报告，避免大缓冲区
        if (check_count % 6 == 0) {  // 每60秒报告一次
            uint8_t percent = bat_get_percent();
            uint32_t voltage = bat_get_volt_mv();
            uint8_t is_charging = bat_in_charge();
            uint8_t usb_valid = bat_get_usb_valid();

            ESP_LOGI(TAG, "🔋 电池: %d%%, %lumV, 充电:%d, USB:%d",
                     percent, voltage, is_charging, usb_valid);
        }
    }

    vTaskDelete(NULL);
}

/**
 * @brief 电池状态管理初始化，有延时，注意不要阻塞主函数运行
 */
void battery_manage_init(void) {
    // 新增：初始化电池管理系统
    ESP_LOGI(TAG, "初始化电池管理系统...");

    // 使用默认BAT_ADC_PIN 初始化
    bat_init(BAT_ADC_PIN);

    // 等待电池系统稳定，避免启动时读数不准确
    ESP_LOGI(TAG, "等待电池系统稳定...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 获取初始电池状态
    uint8_t initial_percent = bat_get_percent();
    uint32_t initial_voltage = bat_get_volt_mv();
    uint8_t usb_connected = bat_get_usb_valid();

    ESP_LOGI(TAG, "电池管理系统就绪 - 电量: %d%%, 电压: %" PRIu32 "mV, USB: %s",
             initial_percent, initial_voltage, usb_connected ? "已连接" : "未连接");

    // 根据初始状态设置系统行为
    if (initial_percent <= 10 && !usb_connected) {
        ESP_LOGW(TAG, "启动时电量低, 建议立即充电");
        // 极低电量时暂时不播放音频，避免网络未就绪时崩溃
    }

    // 设置电池事件回调
    bat_set_event_callback(system_battery_event_handler);

    // 延迟创建电池状态检查任务，确保系统完全启动后再开始监控
    // ESP_LOGI(TAG, "将在5秒后启动电池监控任务...");
    // vTaskDelay(pdMS_TO_TICKS(5000));

    // 创建电池状态检查任务
    xTaskCreate(battery_status_check_task, "bat_check", 5 * 1024, NULL, 3, NULL);

    // 标记系统完全启动，可以安全播放音频
    system_fully_started = 1;
    ESP_LOGI(TAG, "电池监控任务已启动，系统完全就绪");
}
