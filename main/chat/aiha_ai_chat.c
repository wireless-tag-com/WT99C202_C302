#include "aiha_ai_chat.h"
#include "aiha_audio_http.h"
#include "aiha_http_common.h"
#include "aiha_websocket.h"
#include "audio_player_user.h"
#include "chat_asr_ctrl.h"
#include "chat_notify.h"
#include "ci1302.h"
#include "qmsd_ota.h"
#include "qmsd_utils.h"
#include "qmsd_wifi_sta.h"

#define TAG "aiha.ai_chat"

static bool use_exit_chat_mode = false;

// 这个函数不能阻塞，否则会导致音频数据丢失
void aiha_local_asr_detected_cb(uint16_t detect_id) {
    ESP_LOGW(TAG, "Local ASR detected, detect_id: %d", detect_id);
    aiha_websocket_req_stop_all_async();
    aiha_request_tts_async("收到本地命令词");
}

void aiha_websocket_audio_recv_cb(const uint8_t* data, uint32_t size, allinone_audio_status_t status, aiha_audio_format_t format) {
    if (status == ALLINONE_AUDIO_STATUS_START) {
        if (format == AIHA_AUDIO_FORMAT_MP3) {
            audio_player_play_url(MP3_URL_FROM_RAW, 1);
        } else if (format == AIHA_AUDIO_FORMAT_OPUS) {
            audio_player_play_url(OPUS_URL_FROM_RAW, 1);
        }
        audio_player_wait_stream_pipeline_running();
    } else if (status == ALLINONE_AUDIO_STATUS_END) {
        audio_player_raw_write_finish();
    } else if (status == ALLINONE_AUDIO_STATUS_PROCESSING) {
        if (format == AIHA_AUDIO_FORMAT_MP3) {
            audio_player_raw_mp3_write((char*)data, size);
        } else if (format == AIHA_AUDIO_FORMAT_OPUS) {
            audio_player_opus_write((char*)data, size);
        }
    }
}

bool aiha_audio_asr_finish(const char* quest, const char* answer_replace) {
    char* data_answer = qmsd_malloc(256);
    if (data_answer == NULL) {
        return false;
    }
    data_answer[0] = '\0';
    bool ret = chat_asr_ctrl_deal_asr_result(quest, data_answer);
    if (ret) {
        aiha_websocket_req_stop_all_async();
        aiha_request_tts_async(data_answer);
    }
    qmsd_free(data_answer);
    return true;
}

void aiha_chat_deal_error(allinone_error_code_t error_code) {
    if (error_code == ALLINONE_ERROR_CODE_ASR_ERROR) {
        // chat_notify_audio_play(NOTIFY_CHAT_NOT_UNDERSTAND, NULL);
    } else if (error_code == ALLINONE_ERROR_CODE_HTTP_ERROR) {
        if (qmsd_wifi_sta_get_status() == STA_NOT_CONNECTED) {
            chat_notify_audio_play(NOTIFY_NOT_BIND, NULL);
        } else {
            chat_notify_audio_play(NOTIFY_WIFI_SERVER_ERROR, NULL);
        }
    } else if (error_code == ALLINONE_ERROR_CODE_USER_EXIT || error_code == ALLINONE_ERROR_CODE_NO_MONEY) {
        aiha_websocket_req_stop_all_async();
        ci1302_exit_chat_mode(true);
        use_exit_chat_mode = true;
    }
}

void aiha_audio_recv_callback(ci1302_audio_status_t status, uint8_t* data, uint32_t len) {
    uint8_t ota_percent = 0;
    uint8_t ota_status = 0;
    qmsd_ota_get_status(&ota_percent, &ota_status);
    if (ota_status > 0) {
        if (status == CI1302_AUDIO_WAKEUP) {
            if (ota_status == 2) {
                chat_notify_audio_play(NOTIFY_OTA_FAILED, NULL);
            }
            ci1302_exit_chat_mode(true);
            use_exit_chat_mode = true;
        }
        return;
    }

    if (aiha_websocket_is_connected() == false || qmsd_wifi_sta_get_status() != STA_CONNECTED) {
        if (status == CI1302_AUDIO_WAKEUP) {
            aiha_chat_deal_error(ALLINONE_ERROR_CODE_HTTP_ERROR);
        } else if (status == CI1302_AUDIO_SLEEP) {
            if (use_exit_chat_mode) {
                ci1302_exit_chat_mode(false);
                use_exit_chat_mode = false;
            }
        }
        return;
    }

    // 唤醒打断处理 - 无论是否在播放音乐都要处理
    if (status == CI1302_AUDIO_WAKEUP) {
        // 如果正在播放音乐，停止播放
        if (aiha_websocket_is_music_playing()) {
            ESP_LOGW(TAG, "wakeup interrupt during music playing, stop music");
            audio_player_stop_speak();
            aiha_websocket_set_music_playing(false);
        }
        aiha_websocket_set_wakeup();
        uint8_t random_num = rand() % 3;
        if (random_num == 0) {
            chat_notify_audio_play(NOTIFY_CHAT_WAKEUP_VC, NULL);
        } else if (random_num == 1) {
            chat_notify_audio_play(NOTIFY_CHAT_WAKEUP, NULL);
        } else {
            chat_notify_audio_play(NOTIFY_CHAT_WAKEUP_VC2, NULL);
        }

        return;
    }

    // 睡眠处理
    if (status == CI1302_AUDIO_SLEEP) {
        chat_notify_audio_play(NOTIFY_CHAT_EXIT, NULL);
        aiha_websocket_req_stop_all_async();
        if (use_exit_chat_mode) {
            ci1302_exit_chat_mode(false);
            use_exit_chat_mode = false;
        }
        return;
    }

    // 如果正在播放音乐，则忽略VAD相关的音频处理（但不忽略唤醒）
    if (aiha_websocket_is_music_playing()) {
        ESP_LOGD(TAG, "music playing, ignore VAD audio processing, status: %d", status);
        return;
    }

    // 正常的VAD音频处理
    // 忽略唤醒词，有可能唤醒词的音频在wakeup事件后200ms才发送
    if (status == CI1302_AUDIO_START && ci1302_get_wakeup_keep_ms() > 500) {
        audio_player_stop_speak();
        aiha_websocket_audio_upload_start();
    } else if (status == CI1302_AUDIO_END) {
        aiha_websocket_audio_upload_end();
    } else if (status == CI1302_AUDIO_RUNNING) {
        aiha_websocket_audio_upload_data(data, len);
    }

    if (status == CI1302_AUDIO_FULL_FRAME) {
        ESP_LOGI(TAG, "audio full frame");
    }

    ESP_LOGD(TAG, "audio recv callback, status: %d", status);
}

void aiha_ai_chat_start() {
    chat_asr_ctrl_init();
    srand(esp_timer_get_time());
    ci1302_set_local_asr_detected_cb(aiha_local_asr_detected_cb);

    aiha_websocket_handle_t websocket_handle = {
        .audio_upload_format = AIHA_AUDIO_FORMAT_OPUS,
        .audio_spk_format = AIHA_AUDIO_FORMAT_MP3,
        .vad_type = 2,
        .audio_recv_cb = aiha_websocket_audio_recv_cb,
        .text_recv_cb = NULL,
        .asr_finish_cb = aiha_audio_asr_finish,
        .error_cb = aiha_chat_deal_error,
    };
    strcpy(websocket_handle.production_id, aiha_get_production_id());

    aiha_websocket_init(&websocket_handle);
    aiha_websocket_connect();
}
