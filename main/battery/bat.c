#include <stdio.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#if CONFIG_IDF_TARGET_ESP32C2
#include "esp_adc/adc_oneshot.h"
#else
#include "esp_adc/adc_continuous.h"
#endif

#include "qmsd_board_pin.h"
#include "qmsd_utils.h"

#define TAG "BATTERY"

#if CONFIG_IDF_TARGET_ESP32C2
#define SOC_ADC_DIGI_RESULT_BYTES 4
#endif

#define ADC_READ_LENGTH 20 * SOC_ADC_DIGI_RESULT_BYTES
#define BAT_DIV_COEFF 1.33  // 分压比: 100K/(33K+100K) = 0.75, 所以还原系数 = 1/0.75 = 1.33
#define BAT_ADC_ATTEN_DB ADC_ATTEN_DB_12
#define BAT_ADC_VOLT_COMPENSATION 0  // ESP32-C2的补偿电压，需要根据实际情况调整

// 充电状态判断阈值
#define USB_VOLTAGE_THRESHOLD 4200  // USB供电电压阈值(mV) - ADC原始值
#define CHARGE_FULL_VOLTAGE 3700    // 充电完成电压阈值(mV)
#define CHARGE_DETECTION_TIME 5     // 充电检测稳定时间(秒)

static adc_cali_handle_t adc1_cali_chan0_handle = NULL;
static adc_channel_t g_bat_adc_channel = 0;
#if CONFIG_IDF_TARGET_ESP32C2
static adc_oneshot_unit_handle_t adc_oneshot_handle = NULL;
#else
static adc_continuous_handle_t adc_cont_handle = NULL;
#endif
static uint32_t g_bat_voltage = 0;
static uint8_t g_bat_charge_full = 0;
static uint8_t g_bat_percent = 0;
static uint8_t g_usb_valid = 0;

// 电池状态事件回调
typedef void (*bat_event_callback_t)(uint8_t event_type, uint32_t data);
static bat_event_callback_t g_bat_event_callback = NULL;

// 电池事件类型定义
#define BAT_EVENT_LOW_POWER 1       // 低电量警告
#define BAT_EVENT_CRITICAL 2        // 极低电量
#define BAT_EVENT_USB_PLUG 3        // USB插入
#define BAT_EVENT_USB_UNPLUG 4      // USB拔出
#define BAT_EVENT_CHARGE_FULL 5     // 充电完成
#define BAT_EVENT_PERCENT_CHANGE 6  // 电量百分比变化

/**
 * @brief Initialize the ADC calibration.
 *
 * @param unit ADC unit.
 * @param channel ADC channel.
 * @param atten ADC attenuation.
 * @param out_handle Output handle for the calibration.
 * @return true if calibration is successful, false otherwise.
 */
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t* out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    // Create calibration scheme - 根据芯片类型选择校准方法
#if CONFIG_IDF_TARGET_ESP32C2
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting (ESP32-C2)");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
#else
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
#endif
    if (ret == ESP_OK) {
        calibrated = true;
    }

    *out_handle = handle;  // Output the calibration handle
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

/**
 * @brief Calculate the battery percentage based on the voltage.
 *
 * @param volt_mv Battery voltage in millivolts.
 * @return uint8_t Battery percentage (0-100).
 */
uint8_t bat_volt_calculate_percent(uint32_t volt_mv) {
    double y;
    double x = (double)volt_mv / 1000.0;
    if (x < 3.45) {
        y = 0;
    } else if (x >= 3.4 && x <= 3.59) {
        y = 0.0882 * x * x - 0.4418 * x + 0.5476;
    } else if (x > 3.59 && x <= 4.049) {
        y = -5.5508 * x * x * x + 64.741 * x * x - 249.57 * x + 318.5;
    } else if (x > 4.049 && x <= 4.2) {
        y = -2.8846 * x * x + 24.51 * x - 51.049;
    } else {
        y = 1;
    }

    if (y < 0) {
        y = 0;
    };
    // 将y值映射到0到100的范围
    uint8_t y_mapped = y * 100;
    if (y_mapped > 100) {
        y_mapped = 100;
    }
    return y_mapped;
}


// c202 1v0版本的硬件只能测到3.790v，再高就没了
/**
 * @brief Calculate battery percentage for 3000mAh 3.7V battery with 3.2V-3.7V range
 * 针对3000mAh电池，电压范围3.2V-3.7V的电量计算函数
 *
 * @param volt_mv Battery voltage in millivolts (3200-3700mV)
 * @return uint8_t Battery percentage (0-100)
 */
// uint8_t bat_volt_calculate_percent(uint32_t volt_mv) {
//     double voltage = (double)volt_mv / 1000.0;  // 转换为V
//     double percentage;

//     // 针对3.2V-3.7V范围的3000mAh锂电池
//     if (voltage <= 3.2) {
//         // 3.2V以下，电池耗尽
//         percentage = 0;
//     } else if (voltage <= 3.3) {
//         // 3.2V-3.3V: 0%-10% (低电量警告区)
//         percentage = (voltage - 3.2) / (3.3 - 3.2) * 10.0;
//     } else if (voltage <= 3.4) {
//         // 3.3V-3.4V: 10%-25% (低电量区)
//         percentage = 10.0 + (voltage - 3.3) / (3.4 - 3.3) * 15.0;
//     } else if (voltage <= 3.5) {
//         // 3.4V-3.5V: 25%-50% (正常使用区)
//         percentage = 25.0 + (voltage - 3.4) / (3.5 - 3.4) * 25.0;
//     } else if (voltage <= 3.6) {
//         // 3.5V-3.6V: 50%-75% (良好电量区)
//         percentage = 50.0 + (voltage - 3.5) / (3.6 - 3.5) * 25.0;
//     } else if (voltage <= 3.7) {
//         // 3.6V-3.7V: 75%-100% (高电量区)
//         percentage = 75.0 + (voltage - 3.6) / (3.7 - 3.6) * 25.0;
//     } else {
//         // 超过3.7V，认为是100%
//         percentage = 100.0;
//     }

//     // 边界检查
//     if (percentage < 0) {
//         percentage = 0;
//     } else if (percentage > 100) {
//         percentage = 100;
//     }

//     return (uint8_t)percentage;
// }

/**
 * @brief Read the battery voltage once.
 *
 * @return uint32_t Battery voltage in millivolts.
 */
static uint32_t bat_adc_volt_read_single() {
#if CONFIG_IDF_TARGET_ESP32C2
    // ESP32-C2使用oneshot模式
    int adc_raw = 0;
    int voltage = 0;

    // 读取多次并求平均值以提高精度
    uint32_t total_raw = 0;
    const int read_count = 10;

    for (int i = 0; i < read_count; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_oneshot_handle, g_bat_adc_channel, &adc_raw));
        total_raw += adc_raw;
        vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延时
    }

    uint32_t avg_raw = total_raw / read_count;
    adc_cali_raw_to_voltage(adc1_cali_chan0_handle, avg_raw, &voltage);

    // 调试信息：打印原始ADC值和校准后的电压
    static uint32_t debug_count = 0;
    if (debug_count++ % 10 == 0) {  // 每10次打印一次调试信息
        ESP_LOGI(TAG, "ADC调试(ESP32-C2): 原始值=%lu, 校准电压=%dmV, 分压系数=%.1f, 补偿=%dmV", avg_raw, voltage, BAT_DIV_COEFF, BAT_ADC_VOLT_COMPENSATION);
    }

    voltage = BAT_DIV_COEFF * (voltage + BAT_ADC_VOLT_COMPENSATION);
    return voltage;
#else
    // ESP32-C3等使用continuous模式
    uint8_t result[ADC_READ_LENGTH];
    uint32_t ret_num = 0;

    adc_continuous_flush_pool(adc_cont_handle);
    adc_continuous_start(adc_cont_handle);
    adc_continuous_read(adc_cont_handle, result, ADC_READ_LENGTH, &ret_num, 1000);
    adc_continuous_stop(adc_cont_handle);

    uint32_t value_total = 0;
    int voltage = 0;
    for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
        adc_digi_output_data_t* p = (adc_digi_output_data_t*)&result[i];
        uint32_t data = p->type2.data;
        value_total += data;
    }
    value_total = value_total / (ret_num / SOC_ADC_DIGI_RESULT_BYTES);
    adc_cali_raw_to_voltage(adc1_cali_chan0_handle, value_total, &voltage);

    // 调试信息：打印原始ADC值和校准后的电压
    // static uint32_t debug_count = 0;
    // if (debug_count++ % 10 == 0) {  // 每10次打印一次调试信息
    //     ESP_LOGI(TAG, "ADC调试: 原始值=%lu, 校准电压=%dmV, 分压系数=%.1f, 补偿=%dmV", value_total, voltage, BAT_DIV_COEFF, BAT_ADC_VOLT_COMPENSATION);
    // }

    voltage = BAT_DIV_COEFF * (voltage + BAT_ADC_VOLT_COMPENSATION);
    return voltage;
#endif
}

/**
 * @brief Compare two uint32_t values.
 *
 * @param a Pointer to the first value.
 * @param b Pointer to the second value.
 * @return int Comparison result.
 */
static int compare_uint32(const void* a, const void* b) {
    // 强制转换为 uint32_t 指针后比较
    uint32_t val_a = *(uint32_t*)a;
    uint32_t val_b = *(uint32_t*)b;

    if (val_a < val_b) {
        return -1;
    } else if (val_a > val_b) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Calculate the average voltage from a list of voltages.
 *
 * @param volt_list List of voltages.
 * @param num Number of voltages.
 * @return uint32_t Average voltage.
 */
static uint32_t bat_take_average(uint32_t* volt_list, uint32_t num) {
    uint32_t volt_new[num];
    memcpy(volt_new, volt_list, num * sizeof(uint32_t));
    qsort(volt_new, num, sizeof(uint32_t), compare_uint32);
    uint32_t volt_total = 0;
    for (uint32_t i = 3; i < num - 3; i++) {
        volt_total += volt_new[i];
    }
    return volt_total / (num - 6);
}

/**
 * @brief Task to update the battery status periodically.
 *
 * @param param Task parameter (unused).
 */
static void bat_update_task(void* param) {
    uint8_t bat_average_num = 12;
    uint32_t bat_average[bat_average_num];
    uint8_t bat_ave_pos = 0;
    uint8_t last_percent = 0;

    // 状态记忆，避免重复事件通知
    uint8_t low_power_notified = 0;        // 是否已通知低电量
    uint8_t critical_power_notified = 0;   // 是否已通知极低电量

    // 避免开机的时候因为1阶滤波的迟滞, 读取10次电压进平均值
    for (uint8_t i = 0; i < bat_average_num; i++) {
        uint32_t voltage = bat_adc_volt_read_single();
        bat_average[i] = voltage;
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        uint32_t voltage = bat_adc_volt_read_single();

        // 简化电池状态处理：仅基于电压
        g_bat_charge_full = 0;  // 简化：不检测充电状态

        bat_average[bat_ave_pos] = voltage;
        bat_ave_pos = (bat_ave_pos == (bat_average_num - 1)) ? 0 : bat_ave_pos + 1;
        uint32_t bat_ave = bat_take_average(bat_average, bat_average_num);

        // 简化：直接使用平均电压
        g_bat_voltage = bat_ave;
        g_bat_percent = bat_volt_calculate_percent(g_bat_voltage);
        // extern uint8_t bat_percent;
        // g_bat_percent = bat_percent;

        // 电量变化事件
        if (g_bat_percent != last_percent && g_bat_event_callback) {
            g_bat_event_callback(BAT_EVENT_PERCENT_CHANGE, g_bat_percent);
        }

        // 低电量警告事件 - 只在首次进入低电量时触发，避免重复通知
        if (g_bat_percent <= 20 && !low_power_notified && g_bat_event_callback) {
            g_bat_event_callback(BAT_EVENT_LOW_POWER, g_bat_percent);
            low_power_notified = 1;
            ESP_LOGI(TAG, "低电量警告事件已触发 (%d%%)", g_bat_percent);
        }

        // 电量恢复到20%以上时，重置低电量通知标志
        if (g_bat_percent > 20) {
            low_power_notified = 0;
        }

        // 极低电量事件 - 只在首次进入极低电量时触发，避免重复通知
        if (g_bat_percent <= 5 && !critical_power_notified && g_bat_event_callback) {
            g_bat_event_callback(BAT_EVENT_CRITICAL, g_bat_percent);
            critical_power_notified = 1;
            ESP_LOGI(TAG, "极低电量警告事件已触发 (%d%%)", g_bat_percent);
        }

        // 电量恢复到5%以上时，重置极低电量通知标志
        if (g_bat_percent > 5) {
            critical_power_notified = 0;
        }

        last_percent = g_bat_percent;

        // ESP_LOGI(TAG, "电池电压: %" PRIu32 "mV, 电量: %d%%", g_bat_voltage, g_bat_percent);
    }
}

/**
 * @brief Initialize the battery monitoring system.
 *
 * @param adc_pin ADC pin number for battery voltage measurement.
 */
void bat_init(int adc_pin) {
    ESP_LOGI(TAG, "初始化电池管理系统，ADC引脚: GPIO%d", adc_pin);

    // GPIO0 对应 ADC1_CH0，确保通道配置正确
    if (adc_pin == 0) {
        g_bat_adc_channel = ADC_CHANNEL_0;  // ADC1_CH0
    } else {
        ESP_LOGE(TAG, "不支持的ADC引脚: GPIO%d", adc_pin);
        return;
    }

#if CONFIG_IDF_TARGET_ESP32C2
    // ESP32-C2使用oneshot模式初始化
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_oneshot_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = BAT_ADC_ATTEN_DB,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_oneshot_handle, g_bat_adc_channel, &config));
#else
    // ESP32-C3等使用continuous模式初始化
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = ADC_READ_LENGTH,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_cont_handle));
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 1 * 1000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    adc_digi_pattern_config_t adc_pattern = {
        .atten = BAT_ADC_ATTEN_DB,
        .channel = g_bat_adc_channel,
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };
    dig_cfg.pattern_num = 1;
    dig_cfg.adc_pattern = &adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(adc_cont_handle, &dig_cfg));
#endif

    adc_calibration_init(ADC_UNIT_1, g_bat_adc_channel, BAT_ADC_ATTEN_DB, &adc1_cali_chan0_handle);
    qmsd_thread_create(bat_update_task, "bat", 4 * 1024, NULL, 4, NULL, 1, 1);
}

/**
 * @brief Get the current battery percentage.
 *
 * @return uint8_t Battery percentage (0-100).
 */
uint8_t bat_get_percent() {
    return g_bat_percent;
}

/**
 * @brief Check if USB power is valid.
 *
 * @return uint8_t 1 if USB power is valid, 0 otherwise.
 */
uint8_t bat_get_usb_valid() {
    // 简化：硬件无法检测USB状态，返回0
    return 0;
}

/**
 * @brief Get the current battery voltage in millivolts.
 *
 * @return uint32_t Battery voltage in millivolts.
 */
uint32_t bat_get_volt_mv() {
    return g_bat_voltage;
}

/**
 * @brief Check if the battery is currently charging.
 *
 * @return uint8_t 1 if the battery is charging, 0 otherwise.
 */
uint8_t bat_in_charge() {
    // 简化：硬件无法检测充电状态，返回0
    return 0;
}

/**
 * @brief 设置电池事件回调函数
 *
 * @param callback 回调函数指针
 */
void bat_set_event_callback(void (*callback)(uint8_t event_type, uint32_t data)) {
    g_bat_event_callback = callback;
}

/**
 * @brief 检查电池是否充满
 *
 * @return uint8_t 1表示充满，0表示未充满
 */
uint8_t bat_is_full() {
    return g_bat_charge_full;
}

/**
 * @brief 获取电池状态信息字符串
 *
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 */
void bat_get_status_string(char* buf, size_t buf_size) {
    const char* status = "未知";
    if (g_usb_valid) {
        status = g_bat_charge_full ? "充电完成" : "充电中";
    } else {
        if (g_bat_percent > 20) {
            status = "正常";
        } else if (g_bat_percent > 5) {
            status = "电量低";
        } else {
            status = "电量极低";
        }
    }

    snprintf(buf, buf_size, "电量:%d%% 电压:%" PRIu32 "mV 状态:%s", g_bat_percent, g_bat_voltage, status);
}

