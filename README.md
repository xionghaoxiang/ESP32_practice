# ESP32 语音助手

这是一个基于 ESP32 的语音助手项目，可以实现语音识别、自然语言处理和语音合成的功能。用户可以通过按下按钮说话，系统会识别语音内容，将其发送给通义千问大模型处理，并将回答转换为语音播放出来。

## 功能特点

- 语音识别：使用百度语音识别 API 将语音转换为文本
- 自然语言处理：使用通义千问大模型处理文本
- 语音合成：使用百度语音合成 API 将文本转换为语音
- 音频播放：通过 MAX98357 音频放大器播放语音
- 按钮控制：通过按钮触发语音识别流程

## 硬件要求

- ESP32 开发板
- INMP441 麦克风模块（I2S 接口）
- MAX98357 音频放大器模块（I2S 接口）
- 按钮开关
- 扬声器

## 软件依赖

- Arduino IDE 或 PlatformIO
- ESP32 开发环境
- 相关库文件：
  - WiFi.h
  - HTTPClient.h
  - ArduinoJson.h
  - driver/i2s.h

## 硬件连接

### INMP441 麦克风模块
| INMP441 引脚 | ESP32 引脚 |
|-------------|-----------|
| WS          | GPIO 4    |
| SCK         | GPIO 6    |
| SD          | GPIO 5    |

### MAX98357 音频放大器模块
| MAX98357 引脚 | ESP32 引脚 |
|--------------|-----------|
| LRC          | GPIO 18   |
| BCLK         | GPIO 17   |
| DIN          | GPIO 16   |

### 按钮开关
| 按钮引脚 | ESP32 引脚 |
|---------|-----------|
| 一端    | GPIO 3    |
| 另一端  | GND       |

## 配置说明

在使用本项目前，需要进行以下配置：

### 1. WiFi 配置
在 [my_wifi.h](src/my_wifi.h) 文件中修改 WiFi 账号和密码：
```cpp
WiFi.begin("your_wifi_ssid", "your_wifi_password");
```

### 2. 百度语音识别配置
在 [my_stt.h](src/my_stt.h) 文件中修改百度语音识别的 API Key 和 Secret Key：
```cpp
const char *STT_CLIENT_ID = "your_baidu_stt_api_key";
const char *STT_CLIENT_SECRET = "your_baidu_stt_secret_key";
```

### 3. 百度语音合成配置
在 [my_tts.h](src/my_tts.h) 文件中修改百度语音合成的 API Key 和 Secret Key：
```cpp
const char *TTS_CLIENT_ID = "your_baidu_tts_api_key";
const char *TTS_CLIENT_SECRET = "your_baidu_tts_secret_key";
```

### 4. 通义千问配置
在 [my_Qwen.h](src/my_Qwen.h) 文件中修改通义千问的 API Key：
```cpp
const char* QWEN_API_KEY = "your_qwen_api_key";
```

## 工作流程

1. 系统启动后连接 WiFi
2. 初始化各模块并获取百度语音服务的访问令牌
3. 等待用户按下按钮
4. 按下按钮后，通过 INMP441 麦克风采集音频数据
5. 将音频数据发送到百度语音识别服务，转换为文本
6. 将识别的文本发送到通义千问大模型获取回答
7. 将回答文本通过百度语音合成服务转换为音频
8. 通过 MAX98357 音频放大器播放回答音频

## 文件说明

- [main.cpp](src/main.cpp) - 主程序文件
- [my_wifi.h](src/my_wifi.h) - WiFi 连接配置
- [my_stt.h](src/my_stt.h) - 语音识别模块（百度语音识别）
- [my_tts.h](src/my_tts.h) - 语音合成模块（百度语音合成）
- [my_Qwen.h](src/my_Qwen.h) - 大模型处理模块（通义千问）
- [my_inmp441_max98357.h](src/my_inmp441_max98357.h) - 音频硬件配置
- [UrlEncode.h](src/UrlEncode.h) - URL 编码工具

## 使用方法

1. 按下按钮并说话
2. 等待系统处理并播放回答

## 注意事项

- 确保网络连接稳定
- 百度语音服务和通义千问服务需要注册并获取相应的 API Key
- 根据实际硬件连接修改引脚配置
- 确保 ESP32 有足够的内存运行程序