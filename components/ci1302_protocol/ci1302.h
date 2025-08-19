#pragma once

#include <stdint.h>
#include "ci1302_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// 常量定义
#define CI1302_VAD_FRAME_MS          40      // VAD帧时长，单位ms
#define CI1302_VAD_MAX_TIMEOUT_S    15      // VAD最大超时时间，单位s
#define CI1302_VAD_SENSITIVITY_MAX   60     // VAD灵敏度最大值
#define CI1302_VAD_SENSITIVITY_MIN   45     // VAD灵敏度最小值
#define CI1302_VAD_SENSITIVITY_DEFAULT 53
#define CI1302_MIC_GAIN_MAX          32      // 麦克风增益最大值
typedef enum {
    CI1302_FORMAT_PCM = 0,   // only spk support
    CI1302_FORMAT_SPEEX = 1, // not support now
    CI1302_FORMAT_OPUS = 2,
    CI1302_FORMAT_MP3 = 3, // not support now
} __attribute__((packed)) ci1302_audio_format_t;

// 确保枚举大小为1字节的编译时检查
_Static_assert(sizeof(ci1302_audio_format_t) == 1, "ci1302_audio_format_t must be 1 byte");


typedef enum {
    CI1302_AUDIO_START = 0x00,
    CI1302_AUDIO_RUNNING = 0x01,
    CI1302_AUDIO_END = 0x02,
    CI1302_AUDIO_FULL_FRAME = 0x03,
    CI1302_AUDIO_WAKEUP = 0x04,
    CI1302_AUDIO_SLEEP = 0x05,
    CI1302_AUDIO_IDLE = 0xFF,
} ci1302_audio_status_t;


typedef void (*ci1302_audio_recv_cb_t)(ci1302_audio_status_t status, uint8_t* data, uint32_t len);

/**
 * @brief 初始化CI1302音频芯片
 * @param uart_num UART端口号
 * @param tx_pin 发送引脚
 * @param rx_pin 接收引脚
 * @param flow_ctrl_pin 流控引脚，-1表示不使用流控
 * @param rst_pin 复位引脚
 * @param baudrate 波特率
 */
void ci1302_init(uint8_t uart_num, uint8_t tx_pin, uint8_t rx_pin, int8_t flow_ctrl_pin, uint8_t rst_pin, uint32_t baudrate);

/**
 * @brief 复位CI1302
 */
void ci1302_reset();

/**
 * @brief 请求获取CI1302芯片版本信息
 */
void ci1302_req_version();

/**
 * @brief 设置扬声器音量
 */
void ci1302_set_volume(uint8_t volume);

/**
 * @brief 设置上传播放全双工使能
 * @param is_enable 是否使能，0x00 关闭，0x01 开启
 */
void ci1302_set_upload_while_playing(uint8_t is_enable);

/**
 * @brief 当1302开始请求数据时就返回1，表示可写入，用于流控
 */
uint8_t ci1302_flow_get_write_enable(uint32_t need_empty_size);

/**
 * @brief 开始音频写入
 */
void ci1302_audio_write_start();

/**
 * @brief 停止音频写入
 */
void ci1302_audio_write_stop();

/**
 * @brief 写入音频数据
 * @param data 音频数据缓冲区指针
 * @param len 数据长度
 */
void ci1302_write_audio_data(uint8_t* data, uint32_t len);

/**
 * @brief 设置音频接收回调函数
 * @param callback 回调函数指针
 */
void ci1302_set_audio_recv_callback(ci1302_audio_recv_cb_t callback);

/**
 * @brief 设置vad允许停顿时间
 * @param fix_interval 超时时间，单位为s
 */
void ci1302_vad_interval_time_cfg(uint8_t fix_interval);

/**
 * @brief 配置VAD超时时间
 * @param timeout_s 超时时间，单位秒（实现按秒传参）
 */
void ci1302_vad_timeout_cfg(uint16_t timeout_s);

/**
 * @brief 进入唤醒模式
 */
void ci1302_into_wakeup_mode(uint8_t is_notify);

/**
 * @brief 进入睡眠模式
 */
void ci1302_into_sleep_mode(uint8_t is_notify);

/**
 * @brief 配置睡眠超时时间
 * @param sleep_timeout_sec 睡眠超时时间，单位秒
 */
void ci1302_sleep_timeout_cfg(uint16_t sleep_timeout_sec);


/**
 * @brief 配置VAD灵敏度
 * @param sensitivity 灵敏度值（45~60，数值越大灵敏度越低）
 */
void ci1302_vad_sensitivity_cfg(uint8_t sensitivity);

/**
 * @brief 等待芯片启动完成
 * @param timeout_ms 超时时间，单位毫秒
 * @return true 启动成功，false 启动超时
 */
bool ci1302_wait_startup(uint32_t timeout_ms);

/**
 * @brief 获取唤醒保持时间
 * @return 唤醒保持时间，单位毫秒
 */
uint32_t ci1302_get_wakeup_keep_ms();


/**
 * @brief 退出聊天模式
 * @param is_exit true:需要退出聊天，休眠时间改为1s，false:确定休眠后，休眠时间改为30s
 */
void ci1302_exit_chat_mode(bool is_exit);

/**
 * @brief 获取是否在唤醒模式
 * @return true 在唤醒模式，false 不在唤醒模式
 */
bool ci1302_in_wakeup();

bool ci1302_audio_in_playing();

void ci1302_audio_in_playing_reset();



#ifdef __cplusplus
}
#endif