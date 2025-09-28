# 📱 CI1302 OTA升级说明

## 🎯 1. 功能简述

**WT99C202/WT99C302** 中，语音芯片CI1302默认配置：
- **默认唤醒词**：`你好，小明`
- **v1.0.2版本新增命令词**：
  - `打开空调`
  - `关闭空调`
  - `二十七度`
- **功能特性**：联网状态下识别到命令词会播报`收到本地命令词`

通过本操作文档，您可以**自定义替换唤醒词及命令词**。

> ⚠️ **注意**：当前只支持更换中文，语言模型V00927生成的唤醒词和命令词


---

## 📋 2. 前提需要

在开始操作前，请确保您已具备以下条件：

✅ **必备技能**：
1. 掌握本SDK（ESP32C2/ESP32C3）的编译烧录流程
2. 设备能够正常运行

✅ **可选技能**：
1. 掌握CI1302的烧录方法（如遇1302固件不能正常启动时需要）

---

## 🔧 3. 具体步骤

### 步骤1：生成新的bin文件
参考`ci1302_bin_generator\readme.md`中的步骤说明，生成新唤醒词的`asr.bin`及`user_file.bin`

### 步骤2：替换文件
用上述两个bin文件替换`asr_ota_file`目录下的两个同名文件

### 步骤3：清除缓存
运行命令：
```bash
idf.py fullclean
```

### 步骤4：编译固件
运行命令：
```bash
idf.py build
```
脚本检测固件有变化时，会重新打包，log如下：
```
原始分区表:
pat->hard_ware_name: 0.2.0
pat->soft_ware_version: 0.2.0
pat->user_code1.version: 0.0.100, size: 276620, crc: 0x9737
pat->user_code2.version: 0.0.100, size: 276620, crc: 0x9737
pat->asr_cmd_mode.version: 0.0.100, size: 8965, crc: 0x1c4a
pat->dnn_model.version: 0.0.100, size: 1218472, crc: 0x00fe
pat->voice.version: 0.0.100, size: 1379, crc: 0xd3a2
pat->user_file.version: 0.0.100, size: 623, crc: 0x5364
原始校验和: 0x1c34

重新打包后的分区表:
pat->hard_ware_name: 0.2.0
pat->soft_ware_version: 0.2.0
pat->user_code1.version: 0.0.100, size: 276620, crc: 0x9737
pat->user_code2.version: 0.0.100, size: 276620, crc: 0x9737
pat->asr_cmd_mode.version: 0.0.100, size: 8965, crc: 0x1c4a
pat->dnn_model.version: 0.0.100, size: 1218472, crc: 0x00fe
pat->voice.version: 0.0.100, size: 1379, crc: 0xd3a2
pat->user_file.version: 0.0.100, size: 7855, crc: 0xffb7
新校验和: 0x1d8f

重新打包的分区表已保存到: */WT99C202_C302/main/asr_ota_file/tools/../../asr_ota_file/asr_by_gen.bin
```
### 步骤5：烧录并验证
将新固件烧录至ESP32C2或ESP32C3，上电后会自动检查CI1302是否需要更新：

**📝 无需更新时**的log显示：
```
I (1273) CI1302_UART: partitions CRC same, no need to ota
```

**🔄 需要更新时**，log显示如下并自动进入OTA模式开始升级：
```
E (1249) CI1302_UART: partitions CRC error
W (1249) CI1302_UART: 1302中: ASR: 8cd4, USER_FILE: ca64
W (1254) CI1302_UART: c2中:   ASR: 1c4a, USER_FILE: ffb7
```
**🚀 OTA更新流程**log如下，完成后自动重启，之后可以尝试使用新的唤醒词和命令词来交互测试OTA是否成功：
```
W (314) MAIN: ASR module found update, start ota ci1302
I (315) uart: queue free spaces: 10
I (319) qmsd_utils: The ci1302_protocol_ota_uart_frame_task task allocate stack on internal memory
I (332) UART_RECEIVE: UART接收任务已启动
I (339) qmsd_utils: The ci1302_ota_uart_frame_deal_task task allocate stack on internal memory
I (839) CI1302_OTA_UART: 发送版本查询命令，消息类型: 0xA0
I (1349) CI1302_OTA_UART: 发送版本查询命令，消息类型: 0xA0
I (1355) ci1302_protocol: In <- 0xa0, len 6
I (1355) ci1302_protocol: 00 00 02 00 00 02
I (1355) CI1302_OTA_UART: 收到版本信息, 版本号: 0.0.2
I (2050) ci1302_protocol: In <- 0xa1, len 0
I (2050) CI1302_OTA_UART: 收到OTA开始命令, 开始发送固件数据
I (2050) CI1302_ASR_OTA: request partition table
I (2104) ci1302_protocol: In <- 0xa2, len 5
I (2104) ci1302_protocol: 01 00 00 00 01
I (2104) CI1302_OTA_UART: OTA写入成功, 当前包序号: 0, 下一包序号: 1
I (2110) CI1302_ASR_OTA: request partition table
I (3944) ci1302_protocol: In <- 0xa2, len 5
I (3944) ci1302_protocol: 01 00 01 00 a2
I (3944) CI1302_OTA_UART: OTA写入成功, 当前包序号: 1, 下一包序号: 162
I (3950) CI1302_ASR_OTA: request asr model, offset id 2
I (4010) ci1302_protocol: In <- 0xa2, len 5
I (4010) ci1302_protocol: 01 00 a2 00 a3
I (4010) CI1302_OTA_UART: OTA写入成功, 当前包序号: 162, 下一包序号: 163
I (4016) CI1302_ASR_OTA: request asr model, offset id 3
I (4075) ci1302_protocol: In <- 0xa2, len 5
I (4075) ci1302_protocol: 01 00 a3 00 a4
I (4076) CI1302_OTA_UART: OTA写入成功, 当前包序号: 163, 下一包序号: 164
I (4081) CI1302_ASR_OTA: request asr model, offset id 4
I (4138) ci1302_protocol: In <- 0xa2, len 5
I (4138) ci1302_protocol: 01 00 a4 00 a5
I (4138) CI1302_OTA_UART: OTA写入成功, 当前包序号: 164, 下一包序号: 165
I (4191) ci1302_protocol: In <- 0xa2, len 5
I (4191) ci1302_protocol: 01 00 a5 00 a6
I (4192) CI1302_OTA_UART: OTA写入成功, 当前包序号: 165, 下一包序号: 166
I (4245) ci1302_protocol: In <- 0xa2, len 5
I (4245) ci1302_protocol: 01 00 a6 00 a7
I (4245) CI1302_OTA_UART: OTA写入成功, 当前包序号: 166, 下一包序号: 167
I (4299) ci1302_protocol: In <- 0xa2, len 5
I (4299) ci1302_protocol: 01 00 a7 00 a8
I (4299) CI1302_OTA_UART: OTA写入成功, 当前包序号: 167, 下一包序号: 168
I (4352) ci1302_protocol: In <- 0xa2, len 5
I (4352) ci1302_protocol: 01 00 a8 00 a9
I (4353) CI1302_OTA_UART: OTA写入成功, 当前包序号: 168, 下一包序号: 169
I (4406) ci1302_protocol: In <- 0xa2, len 5
I (4406) ci1302_protocol: 01 00 a9 00 aa
I (4406) CI1302_OTA_UART: OTA写入成功, 当前包序号: 169, 下一包序号: 170
I (4460) ci1302_protocol: In <- 0xa2, len 5
I (4460) ci1302_protocol: 01 00 aa 00 ab
I (4460) CI1302_OTA_UART: OTA写入成功, 当前包序号: 170, 下一包序号: 171
I (4513) ci1302_protocol: In <- 0xa2, len 5
I (4514) ci1302_protocol: 01 00 ab 00 ac
I (4514) CI1302_OTA_UART: OTA写入成功, 当前包序号: 171, 下一包序号: 172
I (4567) ci1302_protocol: In <- 0xa2, len 5
I (4567) ci1302_protocol: 01 00 ac 00 ad
I (4568) CI1302_OTA_UART: OTA写入成功, 当前包序号: 172, 下一包序号: 173
I (4621) ci1302_protocol: In <- 0xa2, len 5
I (4621) ci1302_protocol: 01 00 ad 00 ae
I (4621) CI1302_OTA_UART: OTA写入成功, 当前包序号: 173, 下一包序号: 174
I (4675) ci1302_protocol: In <- 0xa2, len 5
I (4675) ci1302_protocol: 01 00 ae 00 af
I (4675) CI1302_OTA_UART: OTA写入成功, 当前包序号: 174, 下一包序号: 175
I (4728) ci1302_protocol: In <- 0xa2, len 5
I (4728) ci1302_protocol: 01 00 af 00 b0
I (4729) CI1302_OTA_UART: OTA写入成功, 当前包序号: 175, 下一包序号: 176
I (4782) ci1302_protocol: In <- 0xa2, len 5
I (4782) ci1302_protocol: 01 00 b0 00 b1
I (4782) CI1302_OTA_UART: OTA写入成功, 当前包序号: 176, 下一包序号: 177
I (4836) ci1302_protocol: In <- 0xa2, len 5
I (4836) ci1302_protocol: 01 00 b1 00 b2
I (4836) CI1302_OTA_UART: OTA写入成功, 当前包序号: 177, 下一包序号: 178
I (4889) ci1302_protocol: In <- 0xa2, len 5
I (4890) ci1302_protocol: 01 00 b2 00 b3
I (4890) CI1302_OTA_UART: OTA写入成功, 当前包序号: 178, 下一包序号: 179
I (4943) ci1302_protocol: In <- 0xa2, len 5
I (4943) ci1302_protocol: 01 00 b3 00 b4
I (4943) CI1302_OTA_UART: OTA写入成功, 当前包序号: 179, 下一包序号: 180
I (4997) ci1302_protocol: In <- 0xa2, len 5
I (4997) ci1302_protocol: 01 00 b4 00 b5
I (4997) CI1302_OTA_UART: OTA写入成功, 当前包序号: 180, 下一包序号: 181
I (6035) ci1302_protocol: In <- 0xa2, len 5
I (6035) ci1302_protocol: 01 00 b5 01 e1
I (6036) CI1302_OTA_UART: OTA写入成功, 当前包序号: 181, 下一包序号: 481
I (6041) CI1302_ASR_OTA: request user file, offset id 5
I (6100) ci1302_protocol: In <- 0xa2, len 5
I (6101) ci1302_protocol: 01 01 e1 01 e2
I (6101) CI1302_OTA_UART: OTA写入成功, 当前包序号: 481, 下一包序号: 482
I (6107) CI1302_ASR_OTA: request user file, offset id 6
I (6983) ci1302_protocol: In <- 0xa2, len 5
I (6983) ci1302_protocol: 01 01 e2 02 00
I (6983) CI1302_OTA_UART: OTA 发送升级完成
I (6991) ci1302_protocol: In <- 0xa3, len 1
I (6991) ci1302_protocol: 01
I (6994) CI1302_OTA_UART: OTA升级 成功
```


---

## ❓ 4. 常见问题

| 🔍 问题现象 | 💡 解决方案 |
|:-----------|:-----------|
| 更换固件后没有进行OTA | 重新编译烧录  |
| 运行卡在`ci1302 startup wait start` | 给设备重新上电或给1302烧录默认固件`ci1302_firmware.bin` |


---

> 📚 **相关文档**：更多详细信息请参考 [ci1302_bin_generator/readme.md](ci1302_bin_generator/readme.md)
