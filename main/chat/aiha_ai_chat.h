#pragma once

#include "ci1302.h"
#include "aiha_websocket.h"

/**
 * @brief 本地ASR检测回调函数
 * @param detect_id 检测ID
 */
void aiha_local_asr_detected_cb(uint16_t detect_id);

/**
 * @brief WebSocket音频接收回调函数
 * @param data 音频数据
 * @param size 音频数据大小
 * @param status 音频状态
 * @param format 音频格式
 */
void aiha_websocket_audio_recv_cb(const uint8_t* data, uint32_t size, allinone_audio_status_t status, aiha_audio_format_t format);

/**
 * @param quest 识别结果
 * @param answer_replace 回答替换，暂未实现
 * @return 是否继续处理
 */
bool aiha_audio_asr_finish(const char* quest, const char* answer_replace);
/**
 * @brief 处理AI聊天过程中的错误
 * @param error_code 错误代码，定义在allinone_error_code_t枚举中
 * @note 根据不同的错误代码执行相应的错误处理逻辑，如播放错误提示音、重置连接等
 */
void aiha_chat_deal_error(allinone_error_code_t error_code);

/**
 * @brief AI音频接收回调函数
 * @param status 音频状态，包含音频数据的处理状态信息
 * @param data 音频数据缓冲区指针
 * @param len 音频数据长度
 * @note 当ci1302芯片接收到音频数据时，会调用此函数进行音频数据的处理和转发
 */
void aiha_audio_recv_callback(ci1302_audio_status_t status, uint8_t* data, uint32_t len);

/**
 * @brief 启动AI聊天功能
 * @note 初始化AI聊天相关的组件，建立WebSocket连接，准备接收和处理语音数据
 */
void aiha_ai_chat_start();
