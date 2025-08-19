#pragma once

#include <stdint.h>
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t cmd;   // 命令字
    uint8_t* buffer;    // 音频数据
    uint32_t len;   // 音频数据长度
} ci1302_uart_frame_t;

#define UART_VERSION 0x0000

// 以下是CI1302的API
#define INVAILD_SPEAK                (0x12345666)
#define RECV_TTS_PLAY                (0x12345677)
#define RECV_MP3_PLAY                (0x12345688)
#define RECV_M4A_PLAY                (0x123456aa)
#define IDLE_STATUS_RECV_M4A_PLAY    (0x123456ab)
#define RECV_WAV_PLAY                (0x123456bb)
#define DEF_FILL                     (0x12345678)
#define WAKEUP_FILL_DATA             (0x12345678)

typedef struct {
    uint32_t magic;      /* Start data for frame. Let's define as 0x5a5aa5a5; */
    uint16_t checksum; /* checksum */
    uint16_t type;     /* Define command type. */
    uint16_t len;      /* Define data stream len. */
    uint16_t version;  /*版本信息*/
    uint32_t fill_data;  /*填充数据*/
} __attribute__((packed)) cias_standard_head_t;

typedef enum{
    //语音识别相关
    LOCAL_ASR_RESULT_NOTIFY             =  0x0101,  /*本地语音识别通知*/
    WAKE_UP                             =  0x0102,  /*唤醒*/
    VAD_END                             =  0x0103,  /* 云端VAD END */
    SKIP_INVAILD_SPEAK                  =  0x0104,  /* 跳过无效语音 */
    PCM_MIDDLE                          =  0x0105,  /*PCM数据中间包*/
    PCM_FINISH                          =  0x0106,  /*PCM数据结束包*/
    PCM_IDLE                            =  0x0107,  /*PCM数据空闲 */
    VAD_START                           =  0x0108,  /*vad start*/
    EXIT_WAKE_UP                        =  0x0109,  /*退出唤醒*/
    SET_VAD_SENSITIVITY                 =  0x010A,  /*设置VAD灵敏度*/
    VAD_START_BY_KEY                    =  0x010B,  /*手动按键触发vad start*/
    VAD_END_BY_KEY                      =  0x010C,  /*手动触发vad end*/
    SET_AUDIO_EXIT_WAKE_UP              =  0x010D,  /*设置语音退出唤醒，带参数1字节1-带退出提示音(默认) 0-不带退出唤醒提示音*/
	PCM_DENOISE_ENABLE                  =  0x010E,  /*上传音频是否带功能降噪，带参数1字节 1-带降噪(默认) 0-不带降噪*/
	SET_VAD_FILTER_FRAME                =  0x010F,  /*过滤vad start和end之间帧数(16ms一帧)，小于该设定值被判断为无效语音，语音芯片不上传, 带参数2字节*/
	SET_VAD_SENSITIVITY_ACTIVATE_LENTH  =  0x0110,  /*多长时间的静音产生vad end，带参数1字节*/
	SET_VAD_START_MAX_TIMEOUT           =  0x0111,  /*设置vad start 最大时间，超过设置时间则强制结束上传语音，单位秒，默认5S，带参数2字节*/
    SET_PLAY_VOICE_ID                   =  0x0112,  /*设置离线播放音频，带参数4字节(id号)*/
    SET_WAKE_UP_CONTINUE_TIME           =  0x0113,  /*设置持续唤醒时间，超过设置时间，语音芯片强制退出唤醒，带参数2字节(默认15S)*/
    SET_ENTER_WAKE_UP                   =  0x0114,  /*通过指令让语音芯片进入唤醒状态，不用等待唤醒词唤醒*/
    SET_INTERACTION_NULTI_RUOUND_ENABLE =  0x0115,  /*设置单轮还是多轮，带参数1字节，0-单轮 1-多轮(默认)*/
    UPLOAD_PLAY_FULL_DUPLEX_EANBLE      =  0x0116,  /*设置播放音频时是否上传录音，带参数1字节 0-不上传(默认) 1-上传*/
    SET_AUDIO_VOLUME                    =  0x0117,  /*设置语音芯片音量，带参数1字节，范围(1-7), 默认7*/
    SET_AUDIO_COMPRESS_TYPE             =  0x0118,  /*压缩类型，支持speex和opus压缩*/
    SET_VOLUME_MUTE_STATE               =  0X0119,   /*设置mute状态1-mute 0-非mute*/
    SET_AUDIO_START_RECORD              =  0x011A,   /*通过指令开始录音*/
    SET_AUDIO_STOP_RECORD               =  0x011B,    /*通过指令结束录音(和SET_AUDIO_START_RECORD必须配对使用)*/

    SET_CLOUD_ANS_TIMEOUT_EXIT_WAKEUP   =  0x011C,   /*设置云端回答超时退出唤醒，带参数2字节，单位秒，默认10S*/

    //网络播放相关
    NET_PLAY_START                      =  0x0201,   /*开始播放 */
    NET_PLAY_PAUSE                      =  0x0202,   /*播放暂停 */
    NET_PLAY_RESUME                     =  0x0203,   /*恢复播放 */
    NET_PLAY_STOP                       =  0x0204,   /*停止播放 */
    NET_PLAY_RESTART                    =  0x0205,   /*重播*/
    NET_PLAY_NEXT                       =  0x0206,   /*播放下一首 */
    NET_PLAY_LOCAL_TTS                  =  0x0207,   /*播放本地TTS */
    NET_PLAY_END                        =  0x0208,   /*播放结束*/
    NET_PLAY_RECONECT_URL               =  0x0209,   /*重新获取连接 */
    PLAY_DATA_GET                       =  0x020a,   /*获取后续播放数据 */
    PLAY_DATA_RECV                      =  0x020b,   /*接收播放数据 */
    PLAY_DATA_END                       =  0x020c,   /*播放数据接收完*/
    PLAY_TTS_END                        =  0x020d,   /*播放tts结束*/
    PLAY_EMPTY                          =  0x020e,   /*播放空指令 */
    PLAY_NEXT                           =  0x020f,   /*播放完上一首，主动播放下一首*/
    PLAYING_TTS                         =  0x0210,   /*当前正在播放TTS音频*/
    PLAY_RESUME_ERRO                    =  0x0211,   /*重播失败*/
    PLAY_LAST                           =  0x0212,   /*播放上一首*/
    PLAY_AUDIO_SIZE                     =  0x0213,   /*播放音频数据长度*/
    PLAY_AUDIO_TYPE                     =  0x0214,   /*播放数据类型*/
    SET_AUDIO_PLAY_MODE                 =  0x0215,    /*设置播放模式，带参数1字节，1-打断当前播报 0-不打断当前播报，顺序播放默认)*/
    VAD_START_STOP_PLAY                 =  0x0216,    /*设置全双工模式下，vad start是否停止当前播放，带参数1字节，1-停止播放 0-不停止播放)*/
    LOCAL_AUDIO_PLAY_START              =  0x0217,    /*本地播放音频开始*/
    LOCAL_AUDIO_PLAY_STOP               =  0x0218,    /*本地播放音频结束*/
    //IOT自定义协议
    QCLOUD_IOT_CMD                      =  0x0301,   /*云端IOT指令 */
    NET_VOLUME                          =  0x0302,   /*云端音量 */
    LOCAL_VOLUME                        =  0x0303,   /*本地音量 */
    VOLUME_INC                          =  0x0304,   /*增大音量 */
    VOLUME_DEC                          =  0x0305,   /*减小音量 */
    VOLUME_MAXI                         =  0x0306,   /*最大音量 */
    VOLUME_MINI                         =  0x0307,   /*最小音量 */
    CIAS_CJSON_DATA                     =  0x0308,   //云端iot的json格式数据
    IOT_VOLUME_MUTE                     =  0x0309,    // 云端音量静音
    IOT_VOLUME_UNMUTE                   =  0x030a,   // 云端音量取消静音
    IOT_QUITE_WAKE_UP_MODE              =  0x030b,   // 退出唤醒模式
    //网络相关
    ENTER_NET_CONFIG                    =  0x0401,   //进入配网模式
    NET_CONFIGING                       =  0x0402,   //配网中
    EXIT_NET_CONFIG                     =  0x0403,   //退出配网模式
    INIT_SMARTCONFIG                    =  0x0404,   //初始密码状态 出厂配置状态
    WIFI_DISCONNECTED                   =  0x0405,   //网络断开，带参数 3 字节(前2 个字节表示 id 号，第三个字节表示要不要打断当前播报 1-打断 0-不打断)
    WIFI_CONNECTED                      =  0x0406,   //网络连接成功
    GET_PROFILE                         =  0x0407,   //已获取鉴权文件
    NEED_PROFILE                        =  0x0408,   //需要鉴权文件，add by roy
    CLOUD_CONNECTED                     =  0x0409,   //云端已连接
    CLOUD_DISCONNECTED                  =  0x040a,   //云端已断开
    NET_CONFIG_SUCCESS                  =  0x040b,   //配网成功
    NET_CONFIG_FAIL                     =  0x040c,   //配网失败
    NET_CONFIG_CLEAN                    =  0x040d,   //配网清除

    //and by yjd
    CIAS_OTA_START                      =  0x0501,      //开始ota
    CIAS_OTA_DATA                       =  0x0502,      //ota 数据
    CIAS_OTA_SUCESS                     =  0x0503,      // OTA升级成功
    CIAS_FACTORY_START                  =  0x0504,      //生产测试
    CIAS_FACTORY_OK                     =  0x0505,      //生产测试成功
    CIAS_FACTORY_FAIL                   =  0x0506,      //生产测试失败
    CIAS_FACTORY_SELF_TEST_START        =  0x0507,      //自测试
    CIAS_IR_DATA                        =  0x0508,      //红外数据发送
    CIAS_IR_LOADING_DATA                =  0x0509,      //红外码库下载中
    CIAS_IR_LOAD_DATA_OVER              =  0x050a,      //红外码库下载完成
    CIAS_IR_LOAD_DATA_START             =  0x050b,      //红外下载码库开始
    CIAS_FACTORY_TEST_ENG_THR_SET       =  0x050c,   //音频通路设置检测播报音能量阈值设置，范围(0-100db)
    CIAS_FACTORY_TEST_ENG_GET           =  0x050d,       //生产测试结果返回
    CIAS_FACTORY_TEST_REAL_VAL_GET      =  0x050e,       //生产通路测试过程中实时值上传
    //语音系统相关
    CIAS_AUDIO_SYS_READY                =  0x0601,       //语音系统ready
    CIAS_AUDIO_SYS_ERR                  =  0x0602,       //语音系统异常
    CIAS_AUDIO_RST                      =  0x0603,        //复位语音芯片

    // 自我学习
    CWSL_UART_REGISTRATION_WAKE         = 0x0701,   // 自我学习唤醒
    CWSL_UART_REGISTRATION_WAKE_ING     = 0x0702,   // 自我学习唤醒中
    CWSL_UART_REGISTRATION_WAKE_END_SUCCESSFUL      = 0x0703,   // 自我学习唤醒结束成功
    CWSL_UART_REGISTRATION_WAKE_END_FAILED          = 0x0704,   // 自我学习唤醒结束失败
    CWSL_UART_REGISTRATION_WAKE_END_FAILED_REASON   = 0x0705,   // 自我学习唤醒结束失败原因
    CWSL_UART_DELETE_WAKEUP_WORD        = 0x0706,   // 删除唤醒词

    //指令执行状态相关
    CIAS_CMD_EXEC_STATE                    =  0x0801,     /*指令执行状态，返回数据部分3字节 前两字节表示语音芯片接收到指令的类型， 第3字节表示指令执行结果(0x01-执行成功 0x02-执行失败)*/
}ci1302_cmd_t;

void ci1302_protocol_init(uint8_t uart_num, uint8_t tx_pin, uint8_t rx_pin, uint32_t baudrate);

void ci1302_protocol_write_bytes_multi(uint16_t cmd, uint8_t frame_nums, ...);

void ci1302_protocol_write_bytes(uint16_t cmd, const uint8_t* frame, uint32_t len, uint16_t version, uint32_t fill_data);

void ci1302_protocol_write_byte(uint8_t cmd, uint8_t data);

int ci1302_protocol_recv_frame(ci1302_uart_frame_t* frame, uint32_t timeout_ms);

void ci1302_protocol_free_frame_buffer(ci1302_uart_frame_t* frame);

void ci1302_protocol_wait_write_done();

#ifdef __cplusplus
}
#endif
