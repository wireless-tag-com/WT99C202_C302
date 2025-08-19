#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "driver/uart.h"
#include "esp_littlefs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "audio_player_user.h"
#include "qmsd_board_pin.h"
#include "qmsd_button.h"
#include "qmsd_network.h"
#include "qmsd_ota.h"
#include "qmsd_utils.h"
#include "qmsd_wifi_sta.h"
#include "storage_nvs.h"

#include "aiha_ai_chat.h"
#include "aiha_http_common.h"
#include "aiha_websocket.h"
#include "audio_hardware.h"
#include "aiha_audio_http.h"
#include "chat_asr_ctrl.h"
#include "chat_notify.h"
#include "ci1302.h"
#include "bat_status.h"

#define TAG "MAIN"


bool g_ecrypt_index = 0;

uint8_t vol_status = 40;
int volume_hope = -1;
btn_handle_t btn;


void littlefs_init(void) {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "res",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void btn_single_callback_cb(btn_handle_t handle, void* user_data) {
    ESP_LOGI(TAG, "single click");
    if (qmsd_wifi_sta_get_status() == STA_NOT_CONNECTED) {
        aiha_chat_deal_error(ALLINONE_ERROR_CODE_HTTP_ERROR);
        return;
    }
    if (aiha_websocket_is_connected() == false || qmsd_wifi_sta_get_status() != STA_CONNECTED) {
        return;
    }
    if (ci1302_in_wakeup()) {
        aiha_chat_deal_error(ALLINONE_ERROR_CODE_USER_EXIT);
    }
}

void btn_repeat_callback_cb(btn_handle_t handle, void* user_data) {
    uint8_t repeat_count = qmsd_button_get_repeat(handle);
    // 长按5次重启
    if (repeat_count > 5) {
        storage_nvs_erase_key("wifiCfg");
        esp_restart();
    }
    ESP_LOGE(TAG, "btn repeat %d", repeat_count);
}

void aiha_tts_cb(const char* url, void* user_data) {
    ESP_LOGI(TAG, "tts url: %s", url);
    audio_player_play_url(url, 1);
}

// 音乐播放状态回调：控制全双工开关
void duplex_mode_callback(bool is_enable) {
    ESP_LOGI(TAG, "Duplex mode: %s", is_enable ? "ON" : "OFF");
    // 音乐模式：关闭全双工（仅保留唤醒词）
    // 非音乐模式：开启全双工（正常对话）
    ci1302_set_upload_while_playing(is_enable ? 0x01 : 0x00);
}

// 音频播放结束回调：清除音乐播放标志
void audio_stop_callback(void) {
    ESP_LOGI(TAG, "Audio playback stopped, clearing music flag");
    aiha_websocket_set_music_playing(false);
}

void app_main(void) {
    printf("   ___    __  __   ____    ____  \n");
    printf("  / _ \\  |  \\/  | / ___|  |  _ \\ \n");
    printf(" | | | | | |\\/| | \\___ \\  | | | |\n");
    printf(" | |_| | | |  | |  ___) | | |_| |\n");
    printf("  \\__\\_\\ |_|  |_| |____/  |____/  \n");

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("CI1302", ESP_LOG_INFO);
    esp_log_level_set("aiha.allinOne", ESP_LOG_INFO);
    esp_log_level_set("HTTP_STREAM", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_THREAD", ESP_LOG_ERROR);
    esp_log_level_set("i2s_std", ESP_LOG_DEBUG);

    esp_log_level_set("ci1302_protocol", ESP_LOG_ERROR);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    printf("QMSD Start, version: " SOFT_VERSION "\n");
    ci1302_init(UART_NUM_1, EXT_UART_TXD_PIN, EXT_UART_RXD_PIN, EXT_UART_STA_PIN, EXT_AUDIO_RST_PIN, 921600);
    ci1302_set_audio_recv_callback(aiha_audio_recv_callback);
    ESP_LOGI(TAG, "ci1302 startup wait start");
    ci1302_wait_startup(portMAX_DELAY);
    ESP_LOGI(TAG, "ci1302 startup wait done");

    qmsd_button_config_t btn_config = QMSD_BUTTON_DEFAULT_CONFIG;
    btn_config.debounce_ticks = 2;
    btn_config.short_ticks = 400 / btn_config.ticks_interval_ms;
    btn_config.update_task.en = 0;
    qmsd_button_init(&btn_config);

    aiha_http_set_production_id("C38006");
    aiha_request_tts_set_cb(aiha_tts_cb);
    aiha_websocket_set_music_status_callback(duplex_mode_callback);
    audio_player_set_stop_callback(audio_stop_callback);
    audio_player_mp3_hardware_player_enable();

    storage_nvs_init();
    littlefs_init();
    audio_hardware_init();
    audio_player_init();
    chat_notify_init();


    btn = qmsd_button_create_gpio(KEY_0_PIN, 0, NULL);
    qmsd_button_register_cb(btn, BUTTON_PRESS_REPEAT, btn_repeat_callback_cb);
    qmsd_button_register_cb(btn, BUTTON_SINGLE_CLICK, btn_single_callback_cb);
    qmsd_button_start(btn);

    uint8_t* volume_index = NULL;
    uint32_t lenv = sizeof(uint8_t);
    if (storage_nvs_read_blob("volume", (void**)&volume_index, &lenv) == ESP_OK) {
        vol_status = *volume_index;
    } else {
        vol_status = 50;
    }
    printf("vol_status: %d \n", vol_status);

    audio_hardware_set_volume(vol_status);

    chat_notify_audio_play(NOTIFY_STARTUP, NULL);
    qmsd_network_start(aiha_ai_chat_start);

    battery_manage_init();

    int volume_diff_count = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        char input = '\0';
        scanf("%c", &input);
        if (input == 'm') {
            qmsd_debug_heap_print(MALLOC_CAP_INTERNAL, 0);
        } else if (input == 'd') {
            qmsd_debug_task_print(0);
        } else if (input == 'p') {
            ESP_LOGI(TAG, "play");
            audio_player_play_url(MP3_URL_FROM_FILE "/littlefs/ota_failed.mp3", 1);
            // 自动拼接 从"file:/" "/spiffs/wifi_failed.mp3"=>"file:/spiffs/wifi_failed.mp3"
        } else if (input == 'e') {
            ESP_LOGE(TAG, "stop");
            audio_player_stop_speak();  // 清除mp3流
            ESP_LOGE(TAG, "stop finish");
        } else if (input == 'c') {
            ci1302_into_sleep_mode(1);
        } else if (input == 'a') {
            ESP_LOGE(TAG, "audio_player_get_remaining_size: %d", audio_player_get_remaining_size());
        } else if (input == 'r') {
            ci1302_reset();
        } else if (input == 's') {
            audio_hardware_add_volume(17);
        } else if (input == 'l') {
            audio_hardware_add_volume(-17);
        } else if (input == 'f') {
            ci1302_set_upload_while_playing(1);
        } else if (input == 'g') {
            ci1302_set_upload_while_playing(0);
        } else if (input == 'h') {
            ci1302_vad_timeout_cfg(10);
        } else if (input == 'j') {
            ci1302_into_wakeup_mode(1);
        } else if (input == 'k') {
            //
        } else if (input == 'v') {
            ci1302_sleep_timeout_cfg(5);
        } else if (input == 'b') {
            //
        }

        if (vol_status != audio_hardware_get_volume()) {
            volume_diff_count += 1;
        } else {
            volume_diff_count = 0;
        }

        if (volume_diff_count > 100) {
            vol_status = audio_hardware_get_volume();
            storage_nvs_write_blob("volume", &vol_status, sizeof(vol_status));
            ESP_LOGI(TAG, "write to flash new volume: %d", vol_status);
            volume_diff_count = 0;
        }

        qmsd_button_update();
    }
}
