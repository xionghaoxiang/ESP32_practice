#include "my_wifi_audio.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <driver/i2s.h>

// 外部声明I2S配置，避免重复定义
extern i2s_config_t inmp441_i2s_config;
extern i2s_config_t max98357_i2s_config;
extern const i2s_pin_config_t inmp441_gpio_config;
extern const i2s_pin_config_t max98357_gpio_config;

// 全局WiFi音频实例
WifiAudio wifiAudio;

// 构造函数
WifiAudio::WifiAudio()
{
    // 为发送和接收分配缓冲区内存
    audio_tx_buffer = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    audio_rx_buffer = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!audio_tx_buffer)
    {
        audio_tx_buffer = (uint8_t *)malloc(buffer_size);
    }

    if (!audio_rx_buffer)
    {
        audio_rx_buffer = (uint8_t *)malloc(buffer_size);
    }

    // 检查内存分配是否成功
    if (!audio_tx_buffer || !audio_rx_buffer)
    {
        Serial.println("Failed to allocate audio buffers");
    }
}

// 析构函数
WifiAudio::~WifiAudio()
{
    stopAudioTransmission();

    if (audio_tx_buffer)
    {
        free(audio_tx_buffer);
        audio_tx_buffer = nullptr;
    }

    if (audio_rx_buffer)
    {
        free(audio_rx_buffer);
        audio_rx_buffer = nullptr;
    }
}

// 初始化WiFi音频功能
void WifiAudio::init()
{
    Serial.println("Initializing WiFi Audio Server on port 81...");

    // 设置WebSocket事件回调
    webSocketServer.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
                            { this->webSocketEvent(num, type, payload, length); });

    // 启动WebSocket服务器
    webSocketServer.begin();
    Serial.println("WiFi Audio Server started");
}

// WebSocket事件处理
void WifiAudio::webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("WebSocket Client [%u] Disconnected!\n", num);
        is_client_connected = false;
        stopAudioTransmission();
        break;

    case WStype_CONNECTED:
    {
        IPAddress ip = webSocketServer.remoteIP(num);
        Serial.printf("WebSocket Client [%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        is_client_connected = true;
        client_id = num;

        // 发送欢迎消息
        webSocketServer.sendTXT(num, "Connected to ESP32 WiFi Audio Server");
    }
    break;

    case WStype_TEXT:
        Serial.printf("WebSocket Client [%u] Received Text: %s\n", num, payload);

        // 处理控制命令
        if (strcmp((char *)payload, "START_AUDIO_TX") == 0)
        {
            startAudioTransmission();
        }
        else if (strcmp((char *)payload, "STOP_AUDIO_TX") == 0)
        {
            stopAudioTransmission();
        }
        else if (strcmp((char *)payload, "START_AUDIO_RX") == 0)
        {
            startAudioReception();
        }
        else if (strcmp((char *)payload, "STOP_AUDIO_RX") == 0)
        {
            stopAudioReception();
        }
        break;

    case WStype_BIN:
        // 接收音频数据
        // 添加更严格的检查，防止缓冲区溢出
        if (is_receiving_audio && audio_rx_buffer != nullptr && length <= buffer_size)
        {
            // 使用临界区保护内存拷贝操作
            portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
            portENTER_CRITICAL(&mux);
            memcpy(audio_rx_buffer, payload, length);
            portEXIT_CRITICAL(&mux);

            playReceivedAudio(audio_rx_buffer, length);
        }
        else
        {
            Serial.printf("Discarded binary data: receiving=%d, buffer=%p, length=%d, buffer_size=%d\n",
                          is_receiving_audio, audio_rx_buffer, length, buffer_size);
        }
        break;

    default:
        break;
    }
}

// 启动音频传输（从麦克风到WiFi客户端）
void WifiAudio::startAudioTransmission()
{
    if (!is_client_connected)
    {
        Serial.println("No client connected. Cannot start audio transmission.");
        return;
    }

    Serial.println("Starting audio transmission...");
    is_sending_audio = true;

    // 如果任务已经在运行，先删除它
    if (audio_tx_task_handle != nullptr)
    {
        vTaskDelete(audio_tx_task_handle);
        audio_tx_task_handle = nullptr;
    }

    // 启动音频采集任务，使用更高的优先级并分配更多堆栈空间
    BaseType_t result = xTaskCreatePinnedToCore(
        audioTransmissionTask,
        "AudioTX",
        4096,
        this,
        configMAX_PRIORITIES - 3, // 较高优先级但低于WiFi任务
        &audio_tx_task_handle,
        0);

    if (result != pdPASS)
    {
        Serial.println("Failed to create audio transmission task");
        is_sending_audio = false;
    }
}

// 停止音频传输
void WifiAudio::stopAudioTransmission()
{
    Serial.println("Stopping audio transmission...");
    is_sending_audio = false;

    // 等待任务结束
    unsigned long start_time = millis();
    while (audio_tx_task_handle != nullptr && (millis() - start_time) < 2000)
    {
        delay(10);
    }

    if (audio_tx_task_handle != nullptr)
    {
        Serial.println("Warning: Audio transmission task did not terminate gracefully");
        audio_tx_task_handle = nullptr;
    }
}

// 启动音频接收（从WiFi客户端到扬声器）
void WifiAudio::startAudioReception()
{
    if (!is_client_connected)
    {
        Serial.println("No client connected. Cannot start audio reception.");
        return;
    }

    Serial.println("Starting audio reception...");
    is_receiving_audio = true;
}

// 停止音频接收
void WifiAudio::stopAudioReception()
{
    Serial.println("Stopping audio reception...");
    is_receiving_audio = false;
}

// 处理WiFi音频命令
void WifiAudio::handleWifiAudioCommand(int command)
{
    switch (command)
    {
    case 24: // 启动音频传输（麦克风到客户端）
        i2s_stop(I2S_NUM_0);
        delay(10);
        i2s_set_sample_rates(I2S_NUM_0, 44100);
        delay(10);
        startAudioTransmission();
        break;

    case 25: // 停止音频传输
        stopAudioTransmission();
        i2s_stop(I2S_NUM_0);
        delay(10);
        i2s_set_sample_rates(I2S_NUM_0, 16000);
        delay(10);
        break;

    case 26: // 启动音频接收（客户端到扬声器）
        startAudioReception();
        break;

    case 27: // 停止音频接收
        stopAudioReception();
        break;

    case 28: // 启动双向音频传输
        startAudioTransmission();
        startAudioReception();
        break;

    case 29: // 停止所有音频传输
        stopAudioTransmission();
        stopAudioReception();
        break;

    default:
        Serial.printf("Unknown WiFi Audio command: %d\n", command);
        break;
    }
}

// WebSocket服务器循环处理
void WifiAudio::loop()
{
    webSocketServer.loop();
}

// 音频传输任务
void WifiAudio::audioTransmissionTask(void *parameter)
{
    WifiAudio *instance = (WifiAudio *)parameter;
    unsigned long last_yield_time = millis();

    while (instance->is_sending_audio)
    {
        // 检查客户端连接状态
        if (!instance->is_client_connected)
        {
            delay(10);
            continue;
        }

        // 从麦克风读取音频数据
        size_t bytes_read = 0;
        esp_err_t result = i2s_read(
            I2S_NUM_1,
            instance->audio_tx_buffer,
            instance->buffer_size,
            &bytes_read,
            20 / portTICK_PERIOD_MS); // 增加超时时间

        if (result == ESP_OK && bytes_read > 0 && instance->is_client_connected)
        {
            // 为了避免pbuf_free断言失败，我们分批发送数据
            // 将大数据块拆分为较小的块进行发送
            const size_t max_chunk_size = 512; // 增加块大小以减少发送次数
            size_t sent_bytes = 0;

            while (sent_bytes < bytes_read && instance->is_client_connected)
            {
                size_t chunk_size = min(max_chunk_size, bytes_read - sent_bytes);
                // 在每次发送前检查客户端连接状态
                if (!instance->is_client_connected)
                    break;

                // 添加短暂延迟以减轻网络压力
                delay(1);
                instance->webSocketServer.sendBIN(
                    instance->client_id,
                    instance->audio_tx_buffer + sent_bytes,
                    chunk_size);
                sent_bytes += chunk_size;

                // 在每次发送后短暂延迟
                delay(1);
            }
        }

        // 定期让出CPU时间给其他任务，特别是WiFi任务
        if ((millis() - last_yield_time) > 5)
        {
            yield();
            last_yield_time = millis();
        }

        // 添加额外的延迟以避免过度占用CPU
        delay(1);
    }

    // 清理任务句柄
    instance->audio_tx_task_handle = nullptr;
    vTaskDelete(NULL);
}

// 播放接收到的音频
void WifiAudio::playReceivedAudio(uint8_t *audio_data, size_t length)
{
    int16_t *in = (int16_t *)audio_data;
    size_t samples = length / 2;

    static int16_t stereo_buffer[4096]; // enough space

    for (size_t i = 0; i < samples; i++)
    {
        int16_t s = in[i];
        stereo_buffer[i * 2] = s;     // Left
        stereo_buffer[i * 2 + 1] = s; // Right
    }

    size_t bytes_to_write = samples * 4;

    size_t bytes_written;
    i2s_write(I2S_NUM_0, stereo_buffer, bytes_to_write, &bytes_written, portMAX_DELAY);
}