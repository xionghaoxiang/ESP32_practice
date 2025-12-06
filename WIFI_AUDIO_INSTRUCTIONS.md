# WiFi音频功能使用说明

## 功能简介

本项目新增了基于WebSocket的WiFi音频传输功能，允许ESP32通过WiFi与手机或其他设备进行实时音频通信。该功能支持双向音频传输：

1. **音频发送（TX）**：ESP32通过麦克风(INMP441)采集音频并通过WebSocket发送到客户端
2. **音频接收（RX）**：ESP32通过WebSocket接收客户端发送的音频数据并通过扬声器(MAX98357)播放

## 工作原理

1. ESP32启动后会开启一个WebSocket服务器，监听81端口
2. 客户端（如手机）连接到ESP32的WebSocket服务器
3. 通过发送控制命令启动或停止音频传输：
   - `START_AUDIO_TX`：启动音频发送（麦克风→WiFi客户端）
   - `STOP_AUDIO_TX`：停止音频发送
   - `START_AUDIO_RX`：启动音频接收（WiFi客户端→扬声器）
   - `STOP_AUDIO_RX`：停止音频接收

## 控制方式

有两种方式控制WiFi音频功能：

### 1. 通过串口蓝牙命令

通过串口2（蓝牙串口）发送以下命令：

- `start_tx` - 启动音频发送（麦克风→WiFi客户端）
- `stop_tx` - 停止音频发送
- `start_rx` - 启动音频接收（WiFi客户端→扬声器）
- `stop_rx` - 停止音频接收
- `start_both` - 同时启动音频发送和接收
- `stop_both` - 停止所有音频传输

### 2. 通过Python客户端

项目提供了[wifi_audio_test.py](file:///d:/study/computer_practice/ESP32_practice_compitition/wifi_audio_test.py)测试客户端，可以直接运行并与ESP32进行音频通信。

## 使用方法

### 准备工作

1. 确保ESP32已连接到WiFi网络
2. 确认ESP32的IP地址（可通过串口监视器查看）
3. 确保手机与ESP32在同一WiFi网络下

### 使用Python客户端

1. 运行[wifi_audio_test.py](file:///d:/study/computer_practice/ESP32_practice_compitition/wifi_audio_test.py)：
   ```
   python wifi_audio_test.py
   ```

2. 连接到ESP32：
   ```
   connect ESP32_IP_ADDRESS
   ```
   例如：`connect 192.168.1.100`

3. 发送命令控制音频传输：
   - `start_tx` - 开始发送音频（ESP32麦克风→电脑）
   - `start_rx` - 开始接收音频（电脑→ESP32扬声器）
   - `start_both` - 双向音频传输

### 使用串口蓝牙控制

通过蓝牙串口向ESP32发送以下命令：

- `start_tx` - 启动音频发送
- `stop_tx` - 停止音频发送
- `start_rx` - 启动音频接收
- `stop_rx` - 停止音频接收
- `start_both` - 同时启动音频发送和接收
- `stop_both` - 停止所有音频传输

## 技术细节

### 音频参数

- 采样率：44.1kHz (CD质量)
- 位深度：16位
- 声道：单声道
- 缓冲区大小：2048字节
- WebSocket端口：81

### 内存管理

- 使用PSRAM存储音频缓冲区（如果可用）
- 若无PSRAM，则使用普通内存
- 缓冲区大小经过优化，平衡了延迟和内存使用
- 采用分块传输策略，避免网络堆栈内存问题

### 性能优化

- 使用独立的任务处理音频采集和发送
- 定期调用[yield()](file:///D:/Program%20Files/Python311/Lib/site-packages/jedi/third_party/typeshed/stdlib/builtins.pyi#L907-L908)以保证WiFi任务正常运行
- 采用合适的任务优先级以确保音频和网络任务协调工作
- 数据分块传输以避免网络堆栈内存溢出
- 增加DMA缓冲区数量和大小以提高稳定性

## 故障排除

1. **无法连接到WebSocket服务器**
   - 确认ESP32和客户端在同一网络
   - 检查ESP32的IP地址是否正确
   - 确认ESP32已成功连接WiFi

2. **没有声音**
   - 检查硬件连接是否正确
   - 确认音频设备（麦克风/扬声器）工作正常
   - 查看串口输出确认命令是否被正确接收

3. **音频断断续续**
   - 网络不稳定，尝试靠近路由器
   - 系统负载过高，避免同时运行过多任务
   - 尝试降低采样率或减小缓冲区大小

4. **系统重启或崩溃**
   - 这可能是由于内存管理问题引起的，已经通过分块传输进行了优化
   - 如果问题持续存在，请减小缓冲区大小

## API参考

### WebSocket协议

- 文本消息：控制命令（如`START_AUDIO_TX`）
- 二进制消息：音频数据

### 控制命令

| 命令 | 功能 |
|------|------|
| START_AUDIO_TX | 启动音频发送 |
| STOP_AUDIO_TX | 停止音频发送 |
| START_AUDIO_RX | 启动音频接收 |
| STOP_AUDIO_RX | 停止音频接收 |

## 扩展开发

您可以基于此功能开发更多应用：

1. **语音对讲机**：实现两个ESP32设备之间的语音通话
2. **远程监听**：将ESP32部署在远程位置，通过WiFi监听环境声音
3. **语音转发**：将语音实时转发到网络服务器进行处理

只需按照现有代码结构扩展即可。