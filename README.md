# ESP32-S3 实时语音交互系统

## 项目概述

一个基于 **ESP32-S3** 的完整语音交互解决方案，集成了语音识别（STT）、自然语言处理（LLM）、语音合成（TTS）和蓝牙控制。支持实时音乐流播放和 TTS 语音中断，适用于车载语音助手、IoT 智能家居等场景。

**核心工作流**: 音频采集 → 百度语音识别 → 阿里通义千问 LLM → 百度语音合成 → 扬声器播放 + 蓝牙远程控制

---

## 功能特性

✅ **语音识别（STT）**
- 支持中文实时语音识别
- 使用百度语音识别 API（16kHz PCM）
- Token 自动刷新

✅ **自然语言处理（NLP）**  
- 集成阿里通义千问大模型
- 支持对话理解与生成

✅ **语音合成（TTS）**
- 百度 TTS API（MP3 格式，16kHz）
- 多语种支持（中英混合）
- 可配置语速和音色

✅ **音频系统**
- 双 I2S 端口配置：
  - I2S_NUM_1 RX：INMP441 麦克风输入（16kHz）
  - I2S_NUM_0 TX：MAX98357 扬声器输出（16kHz TTS / 44.1kHz 音乐）
- 自动样本率切换与恢复
- DMA 缓冲优化（8×128KB）

✅ **蓝牙控制**
- 串口蓝牙通信（通过 GPIO 11/12）
- 11 种 TTS 预制命令（车速警告、灯光状态等）
- 音乐播放控制（播放、暂停、音量调节、切歌等）

✅ **网络音乐流**
- HTTP/HTTPS 音乐流播放
- 支持 MP3 格式
- 自动暂停/恢复以便 TTS 中断
- 播放列表管理功能

✅ **STM32 串行通信**
- 支持与下位机 MCU 通信（UART，9600 波特率）

✅ **物理按键控制**
- 按键中断驱动的语音识别和语音聊天功能

---

## 硬件配置

### 开发板
- **MCU**: ESP32-S3-DevKitC-1（双核，384KB SRAM）
- **存储**: 16MB Flash（QIO 模式）+ 8MB PSRAM（外部 RAM）
- **分区表**: `default_16MB.csv`（SPIFFS 文件系统）

### 音频硬件接线

#### INMP441 麦克风（I2S RX）
| 引脚名 | ESP32 GPIO | 功能 |
|--------|-----------|------|
| WS     | GPIO 4    | 字选择时钟 |
| SCK    | GPIO 6    | 串行时钟 |
| SD     | GPIO 5    | 串行数据输入 |
| 3.3V   | 3V3       | 电源 |
| GND    | GND       | 地 |

#### MAX98357 扬声器放大器（I2S TX）
| 引脚名 | ESP32 GPIO | 功能 |
|--------|-----------|------|
| BCLK   | GPIO 17   | 位时钟 |
| LRC    | GPIO 18   | 帧时钟/通道选择 |
| DIN    | GPIO 16   | 串行数据输入 |
| GND    | GND       | 地 |
| SD     | 3.3V      | 使能（接高电平） |

#### 蓝牙和外设
| 功能        | GPIO  | 说明 |
|------------|-------|------|
| 按钮（语音输入） | GPIO 3 | 按下时触发 STT |
| 按钮（语音聊天） | GPIO 7 | 按下时触发语音聊天 |
| LED        | GPIO 9 | 状态指示灯 |
| STM32 RX   | GPIO 13 | 接收 MCU 数据 |
| STM32 TX   | GPIO 14 | 发送至 MCU |
| 蓝牙  RX   | GPIO 11 |
| 蓝牙  TX   | GPIO 12 |

---

## 软件环境

### 编译工具链
- **IDE**: VS Code + PlatformIO 扩展
- **框架**: Arduino（ESP32 v2.x 核心）
- **构建**: PlatformIO CLI

### 核心依赖库
```ini
esphome/ESP32-audioI2S@^2.3.0    # 音频流播放
bblanchon/ArduinoJson@^6.21.3    # JSON 解析
links2004/WebSockets@^2.7.0      # WebSocket 通信（可选）
plerup/EspSoftwareSerial@^8.2.0  # STM32 软串口
```

### 系统库
- `WiFi.h` - WiFi 连接
- `HTTPClient.h` - HTTPS API 调用
- `driver/i2s.h` - I2S 驱动
- `BLEDevice.h`, `BLEServer.h`, `BLEService.h` - 蓝牙

---

## 快速开始

### 1. 克隆和初始化

```bash
git clone https://github.com/your-repo/ESP32-S3-voice-assistant.git
cd ESP32-S3-voice-assistant
```

### 2. PlatformIO 环境配置

```bash
# 安装 PlatformIO CLI（如果未安装）
pip install platformio

# 下载依赖
pio run -e esp32s3 -t install

# 编译（不上传）
pio run -e esp32s3
```

### 3. 云服务凭证配置

#### A. 百度云 STT/TTS 凭证

1. 访问 [百度智能云控制台](https://console.bce.baidu.com/ai/)
2. 创建/选择项目 → 语音识别 + 语音合成服务
3. 复制 **API Key** 和 **Secret Key**
4. 编辑 `src/my_stt.h`:
   ```cpp
   const char *STT_CLIENT_ID = "your_baidu_stt_api_key";
   const char *STT_CLIENT_SECRET = "your_baidu_stt_secret_key";
   ```
5. 编辑 `src/my_tts.h`:
   ```cpp
   const char *TTS_CLIENT_ID = "your_baidu_tts_api_key";
   const char *TTS_CLIENT_SECRET = "your_baidu_tts_secret_key";
   ```

#### B. 阿里通义千问 API Key

1. 访问 [阿里云 DashScope](https://dashscope.aliyuncs.com/)
2. 创建 API Key（通义千问模型）
3. 编辑 `src/my_Qwen.h`:
   ```cpp
   const char* QWEN_API_KEY = "your_alibaba_qwen_api_key";
   ```

#### C. WiFi 配置

编辑 `src/my_wifi.h`:
```cpp
const char *WIFI_SSID = "your_wifi_name";
const char *WIFI_PASSWORD = "your_wifi_password";
```

### 4. 编译和上传

```bash
# 编译
pio run -e esp32s3

# 上传到开发板
pio run -e esp32s3 -t upload

# 打开串口监视器（查看日志）
pio run -e esp32s3 -t monitor
```

---

## 蓝牙命令参考

系统支持通过串口蓝牙接收多种命令。使用蓝牙串口应用（如 Serial Bluetooth Terminal）连接：

### TTS 预制命令
| 命令 | 对应文本 | 场景 |
|------|--------|------|
| `status1` | "您已超速，请减速慢行" | 超速警告 |
| `status2` | "请注意转弯" | 转弯提醒 |
| `status3` | "请注意倒车" | 倒车提醒 |
| `status4` | "电池电量低" | 电量提醒 |

### 音乐播放控制命令
| 命令 | 功能 |
|------|------|
| `play` | 播放音乐 |
| `stop` | 停止播放 |
| `volumeup` | 音量增加 |
| `volumedown` | 音量减小 |
| `next_song` | 下一首 |
| `previous_song` | 上一首 |
| `pause/resume` | 暂停/恢复 |

### 灯光控制命令
| 命令 | 功能 |
|------|------|
| `on` | 打开 LED 灯 |
| `off` | 关闭 LED 灯 |

### 播放列表管理命令
| 命令 | 功能 |
|------|------|
| `ADDURL:<url>` | 添加 URL 到播放列表 |
| `PLAYLIST:[<url1>,<url2>,...]` | 设置播放列表 |
| `PLAYLIST:<index>` | 播放指定索引的歌曲 |
| `LISTSHOW` | 显示播放列表 |
| `LISTCLEAR` | 清空播放列表 |

### 其他命令
| 命令 | 功能 |
|------|------|
| `velocity:<value>` | 发送给 STM32 的速度信息 |

---

## 工作流程详解

### 启动阶段
```
1. GPIO + Serial 初始化
   ↓
2. WiFi 连接 → DNS 查询
   ↓
3. 百度 Token 获取（STT + TTS） → OAuth 请求
   ↓
4. I2S 驱动安装（麦克风/扬声器）
   ↓
5. 按键中断配置
```

### 语音识别流程（按钮触发）
```
按下按钮 → GPIO 中断
   ↓
采集音频（16kHz, 16-bit PCM）
   ↓
发送至百度 STT API（分块上传）
   ↓
解析 JSON 响应 → 提取识别文本
   ↓
根据关键词执行相应操作（开灯、关灯、播放音乐等）
   ↓
将识别文本转换为语音并播放
```

### 语音聊天流程（按钮触发）
```
按下按钮 → GPIO 中断
   ↓
采集音频（16kHz, 16-bit PCM）
   ↓
发送至百度 STT API（分块上传）
   ↓
解析 JSON 响应 → 提取识别文本
   ↓
发送至阿里通义千问 LLM
   ↓
生成回复文本
   ↓
发送至百度 TTS API → 获取 MP3 字节流
   ↓
播放 TTS 音频
```

### 音乐播放 + TTS 中断
```
音乐流播放中（44.1kHz）
   ↓
接收蓝牙命令（如 status1）
   ↓
暂停音乐 → 音乐流保持连接
   ↓
切换 I2S 样本率至 16kHz
   ↓
播放 TTS 语音
   ↓
恢复音乐流 → 恢复 44.1kHz 样本率
   ↓
继续播放（从服务器当前位置恢复，非字节精确）
```

---

## 文件结构

```
ESP32_practice_compitition/
├── platformio.ini                # PlatformIO 构建配置
├── README.md                     # 本文档
├── src/
│   ├── main.cpp                  # 应用主程序
│   ├── my_inmp441_max98357.h     # I2S 驱动初始化（麦克风/扬声器）
│   ├── my_stt.h                  # 百度语音识别 API 包装
│   ├── my_tts.h                  # 百度语音合成 API 包装
│   ├── my_Qwen.h                 # 阿里通义千问 LLM 包装
│   ├── my_wifi.h                 # WiFi 连接管理
│   ├── my_playlist.h             # 播放列表管理
│   ├── UrlEncode.h/cpp           # URL 编码工具
├── include/                      # 头文件目录（保留）
├── lib/                          # 自定义库目录
└── test/                         # 单元测试（可选）
```

---

## 性能指标与限制

### 延迟
| 阶段 | 延迟 | 说明 |
|------|------|------|
| WiFi 连接 | ~3-5s | 首次连接或信号弱 |
| 百度 STT API | ~1-2s | 语音识别处理 |
| 通义千问 LLM | ~0.5-1.5s | 文本生成 |
| 百度 TTS API | ~2-3s | MP3 合成 |
| **总端到端** | **~7-10s** | 从按钮到播放回复 |

### 内存
| 资源 | 使用情况 | 说明 |
|------|---------|------|
| 堆内存（SRAM） | ~50-100 KB | 动态分配，API 缓冲 |
| PSRAM | ~2-4 MB | I2S DMA、MP3 流缓冲 |
| SPIFFS 分区 | ~2-4 MB | 临时音频文件存储 |

**关键优化**:
- PSRAM 启用后 HTTPS API 调用不再因内存溢出而失败
- 使用中断驱动的按键处理提高响应性

### 限制
⚠️ **音乐恢复不精确**: 暂停音乐后重新连接服务器，从当前时间位置恢复播放（非字节精确）  
⚠️ **TTS 语言**: 仅支持汉语和简单英文混合（百度 API 限制）  
⚠️ **并发限制**: WiFi/HTTPS 调用阻塞式（FreeRTOS 中被 yield 但不能多任务并行）  

---

## 故障排除

### 1. 编译错误：`undefined reference to 'i2s_driver_install'`
**原因**: 缺少 ESP32 I2S 驱动库  
**解决**:
```bash
pio run -e esp32s3 -t install
# 或在 platformio.ini 确认 framework = arduino
```

### 2. 运行时崩溃：`Guru Meditation Error: Core 1 panic'ed`
**原因**: 通常是 PSRAM 未启用导致内存溢出  
**解决**:  
编辑 `platformio.ini`:
```ini
[env:esp32s3]
board_build.psram = true
board_build.arduino.memory_type = qio_opi
```

### 3. TTS 播放速度过快
**原因**: 音乐播放改变了 I2S 样本率，未复原  
**解决**:  
`src/main.cpp` 中的所有 TTS 调用前已加入样本率恢复（无需手动修改）

### 4. WiFi 连接超时或 HTTPS 证书错误
**原因**: 网络不稳定或 CA 证书过期  
**解决**:
```cpp
// src/my_stt.h 中示例
http.setCACert(BAIDU_CERT);  // 使用最新证书链
```

### 5. 百度 Token 过期
**原因**: 访问令牌默认 30 天有效期  
**解决**:  
系统自动刷新；如仍失败，在百度控制台检查 API Key 是否有效

---

## 开发建议

### 调试技巧
1. **启用详细日志**:
   ```cpp
   // 在 main.cpp 开头
   #define DEBUG_LOG 1
   if (DEBUG_LOG) Serial.printf("[DEBUG] Free heap: %d\n", ESP.getFreeHeap());
   ```

2. **监视堆内存**:
   ```bash
   # 串口输出中查看
   pio run -e esp32s3 -t monitor --filters=esp32_exception_decoder
   ```

3. **检查 I2S 采样率**:
   ```cpp
   uint32_t sr;
   i2s_get_sample_rate(I2S_NUM_0, &sr);
   Serial.printf("I2S Sample Rate: %u Hz\n", sr);
   ```

### 扩展功能

**集成更多 TTS 命令**:  
编辑 `src/main.cpp` 的 `handleBluetoothCommand()` 函数，在 case 语句中添加新分支

**使用本地 STT 识别**（离线方案）:  
集成 Pocketsphinx 或类似引擎，替换百度 API 调用

**支持多语言 LLM**:  
在 `my_Qwen.h` 中修改 system prompt；或集成 OpenAI/Claude API

---

## 许可证

本项目遵循 MIT 许可证。详见 LICENSE 文件。

---

## 联系方式与参考

- **百度智能云**: https://ai.baidu.com/ （STT/TTS 文档）
- **阿里 DashScope**: https://dashscope.aliyuncs.com/doc/ （通义千问 API）
- **ESP32-S3 官方文档**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
- **PlatformIO 文档**: https://docs.platformio.org/