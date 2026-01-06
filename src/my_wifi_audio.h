#ifndef MY_WIFI_AUDIO_H
#define MY_WIFI_AUDIO_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <driver/i2s.h>



// WiFi音频传输类
class WifiAudio {
private:
    WebSocketsServer webSocketServer = WebSocketsServer(81);
    bool is_client_connected = false;
    uint8_t client_id = 0;
    
    // 音频缓冲区 - 使用较大的缓冲区以适应更高采样率
    uint8_t *audio_tx_buffer = nullptr;
    uint8_t *audio_rx_buffer = nullptr;
    // 增加缓冲区大小以适应44.1kHz采样率
    static constexpr size_t buffer_size = 2048;
    
    // 音频传输状态
    bool is_sending_audio = false;
    bool is_receiving_audio = false;
    
    // 任务句柄
    TaskHandle_t audio_tx_task_handle = nullptr;

public:
    WifiAudio();
    ~WifiAudio();

    // 初始化WiFi音频功能
    void init();
    
    // WebSocket事件处理
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

    // 启动音频传输（从麦克风到WiFi客户端）
    void startAudioTransmission();
    
    // 停止音频传输
    void stopAudioTransmission();
    
    // 启动音频接收（从WiFi客户端到扬声器）
    void startAudioReception();
    
    // 停止音频接收
    void stopAudioReception();
    
    // 处理WiFi音频命令
    void handleWifiAudioCommand(int command);
    
    // WebSocket服务器循环处理
    void loop();

private:
    // 音频传输任务
    static void audioTransmissionTask(void * parameter);
    
    // 播放接收到的音频
    void playReceivedAudio(uint8_t* audio_data, size_t length);
};

// 全局WiFi音频实例
extern WifiAudio wifiAudio;

#endif