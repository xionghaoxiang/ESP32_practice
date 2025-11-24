// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <BLEUtils.h>
// #include <BLE2902.h>
#include <Arduino.h>
#include "UrlEncode.h"
#include "my_wifi.h"
#include "my_stt.h"
#include "my_tts.h"
#include "my_inmp441_max98357.h"
#include "my_Qwen.h" // 通义千问
#include "Audio.h"
#include "my_playlist.h"
// LED1 引脚定义
#define LED1 9
#define LOG(msg) Serial.printf("[%5lu ms] %s\n", millis(), msg)
#define LOG_F(format, ...) Serial.printf("[%5lu ms] " format, millis(), ##__VA_ARGS__)
// 定义按键引脚
const int BUTTON_PIN = 3;   // 3号引脚作为按键控制语音输入
const int BUTTON_PIN_2 = 7; // 7号引脚作为语音聊天
bool isListening = false;   // 语音识别状态标志
bool isChatting = false;    // 语音聊天状态标志
Audio audio;
PlaylistManager playlist;
const char *streamUrls[] = {
    "http://music.163.com/song/media/outer/url?id=431551064.mp3",
    "http://music.163.com/song/media/outer/url?id=2692390309.mp3",
    "http://music.163.com/song/media/outer/url?id=35847388.mp3",
    "http://music.163.com/song/media/outer/url?id=3315244030.mp3",
    "http://music.163.com/song/media/outer/url?id=28285910.mp3",
    "http://music.163.com/song/media/outer/url?id=1330348068.mp3",
    "http://music.163.com/song/media/outer/url?id=2697656415.mp3",
    "http://music.163.com/song/media/outer/url?id=1392908905.mp3" // 可以继续添加更多音频流URL
};
const int streamCount = sizeof(streamUrls) / sizeof(streamUrls[0]); // 自动计算音频流数量
int currentStreamIndex = 0;                                         // 当前播放的音频流索引
bool pinok;
int isplaying = 0;
volatile int bluetoothCommand = 0;
volatile bool button1Pressed = false;
volatile bool button2Pressed = false;

void IRAM_ATTR button1ISR()
{
    button1Pressed = true;
}

void IRAM_ATTR button2ISR()
{
    button2Pressed = true;
}
void handleVoiceRecognition()
{
    // 按键按下，开始语音识别
    if (!isListening)
    {
        isListening = true;
        Serial2.println("Button pressed, starting voice recognition...");

        // 采集音频数据到adc_data数组
        size_t bytes_read = 0;
        esp_err_t result = i2s_read(I2S_NUM_1, &adc_data, sizeof(adc_data), &bytes_read, portMAX_DELAY);
        Serial2.printf("Read %d bytes from I2S\n", bytes_read);

        // 检查是否读取到数据
        if (bytes_read > 0)
        {
            // 执行语音识别流程
            stt_setup();
            stt_assembleJson();
            String recognizedText = sendToSTT(); // 直接获取识别结果
            if (recognizedText.indexOf("开灯") != -1)
            {
                digitalWrite(LED1, HIGH);
            }
            else if (recognizedText.indexOf("关灯") != -1)
            {
                digitalWrite(LED1, LOW);
            }
            else if (recognizedText.indexOf("播放音乐") != -1)
            {
                bluetoothCommand = 5;
            }
            else if (recognizedText.indexOf("停止播放") != -1)
            {
                bluetoothCommand = 6;
            }
            else if (recognizedText.indexOf("播放下一首") != -1)
            {
                bluetoothCommand = 9;
            }
            else if (recognizedText.indexOf("播放上一首") != -1)
            {
                bluetoothCommand = 10;
            }
            else if (recognizedText.indexOf("音量增大") != -1)
            {
                bluetoothCommand = 7;
            }
            else if (recognizedText.indexOf("音量减小") != -1)
            {
                bluetoothCommand = 8;
            }
            bool wasPlaying = (isplaying == 1);

            // 优先使用库的 pauseResume 切换暂停（无缝）
            if (wasPlaying)
            {
                audio.pauseResume(); // 暂停流媒体（若库实现了）
                isplaying = 0;
            }

            int audioLen = 0;
            String audioData = sendToTTSWithConfig(recognizedText, &audioLen, currentTtsConfig);
            Serial2.println("TTS conversion completed");
            if (audioLen > 0)
            {
                // 播放 TTS 前：停止 I2S 并恢复为 16kHz
                i2s_stop(I2S_NUM_0);
                delay(10);
                i2s_set_sample_rates(I2S_NUM_0, 16000);
                delay(10);
                size_t bytes_written = 0;
                esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);

                uint32_t playbackMs = (audioLen * 1000) / (16000 * 2);
                delay(playbackMs + 10);
                Serial2.println("TTS playback completed");
            }
            if (wasPlaying)
            {
                i2s_stop(I2S_NUM_0);
                delay(10);
                i2s_set_sample_rates(I2S_NUM_0, 44100); // 音乐通常是 44.1kHz
                delay(10);
                audio.pauseResume(); // 再次调用恢复
                isplaying = 1;
            }
        }
        else
            Serial2.println("No audio data captured");
        isListening = false;
    }
    button1Pressed = false;
}

void handleVoiceChat()
{
    if (!isChatting)
    {
        isChatting = true; // 这里可以添加语音聊天的处理逻辑
    }
    bool wasPlaying = (isplaying == 1);
    // 优先使用库的 pauseResume 切换暂停（无缝）
    if (wasPlaying)
    {
        audio.pauseResume(); // 暂停流媒体（若库实现了）
        isplaying = 0;
    }
    Serial2.println("Button2 pressed, starting voice recognition...");
    // 采集音频数据到adc_data数组
    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_NUM_1, &adc_data, sizeof(adc_data), &bytes_read, portMAX_DELAY);
    Serial2.printf("Read %d bytes from I2S\n", bytes_read);
    // 检查是否读取到数据
    if (bytes_read > 0)
    {
        // 执行语音识别流程
        stt_setup();
        stt_assembleJson();
        String recognizedText = sendToSTT(); // 直接获取识别结果

        if (recognizedText.length() > 0 && recognizedText != "")
        {
            Serial2.println("Sending text to Qwen: " + recognizedText); // 修改为通义千问
            String answer = getQwenAnswer(recognizedText);              // 调用通义千问
            Serial2.println("Qwen answer: " + answer);                  // 修改为通义千问
            // 将通义千问的回答转换为语音
            if (answer.length() > 0 && answer != "<error>")
            {
                Serial2.println("Converting answer to speech...");
                int audioLen = 0;
                String audioData = sendToTTSWithConfig(answer, &audioLen, currentTtsConfig);

                Serial2.println("TTS conversion completed");
                if (audioLen > 0)
                {
                    // 播放 TTS 前：停止 I2S 并恢复为 16kHz
                    i2s_stop(I2S_NUM_0);
                    delay(10);
                    i2s_set_sample_rates(I2S_NUM_0, 16000);
                    delay(10);
                    size_t bytes_written = 0;
                    esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
                    uint32_t playbackMs = (audioLen * 1000) / (16000 * 2);
                    delay(playbackMs + 10);
                    Serial2.println("TTS playback completed");
                    if (wasPlaying)
                    {
                        i2s_stop(I2S_NUM_0);
                        delay(10);
                        i2s_set_sample_rates(I2S_NUM_0, 44100); // 音乐通常是 44.1kHz
                        delay(10);
                        audio.pauseResume(); // 再次调用恢复
                        isplaying = 1;
                    }
                }
            }
            else

                Serial.println("No valid text recognized, skipping Qwen call"); // 修改为通义千问
        }
    }
    else
        Serial.println("No audio data captured");
    button2Pressed = false;
}

// BLEServer *pServer = NULL;
// BLECharacteristic * pTxCharacteristic;
// bool deviceConnected = false;
// bool oldDeviceConnected = false;
// uint8_t txValue = 0;
// 添加状态标志用于处理蓝牙命令
// // BLE 服务 UUID 和特征 UUID
// #define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
// #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
// // BLE 服务器回调
// class MyServerCallbacks: public BLEServerCallbacks {
//     void onConnect(BLEServer* pServer) { deviceConnected = true; };
//     void onDisconnect(BLEServer* pServer) { deviceConnected = false; };
// };
// // BLE 特征回调
// class MyCallbacks: public BLECharacteristicCallbacks {
//     void onWrite(BLECharacteristic *pCharacteristic) {
//         std::string rxValue = pCharacteristic->getValue();
//         if (rxValue.length() > 0) {
//             Serial.println("*********");
//             Serial.print("Received Value: ");
//             for (int i = 0; i < rxValue.length(); i++)
//                 Serial.print(rxValue[i]);
//             Serial.println();
//             Serial.println("*********");
//             // 根据接收值控制 LED
//             if(rxValue[0]=='o' && rxValue[1]=='n')
//                 digitalWrite(LED1, HIGH);  // 点亮 LED
//             else if(rxValue[0]=='o' && rxValue[1]=='f' && rxValue[2]=='f')
//                 digitalWrite(LED1, LOW);   // 熄灭 LED
//             // 设置命令标志而不是直接处理
//             else if(rxValue[0]=='s' && rxValue[1]=='t' && rxValue[2]=='a' && rxValue[3]=='t' && rxValue[4]=='u' && rxValue[5]=='s'&& rxValue[6]=='1'&& rxValue[7]=='\n') {
//                 bluetoothCommand = 1; // 设置status1命令标志
//             }
//             else if(rxValue[0]=='s' && rxValue[1]=='t' && rxValue[2]=='a' && rxValue[3]=='t' && rxValue[4]=='u' && rxValue[5]=='s'&& rxValue[6]=='2'&& rxValue[7]=='\n') {
//                 bluetoothCommand = 2; // 设置status2命令标志
//             }
//             else if(rxValue[0]=='s' && rxValue[1]=='t' && rxValue[2]=='a' && rxValue[3]=='t' && rxValue[4]=='u' && rxValue[5]=='s'&& rxValue[6]=='3'&& rxValue[7]=='\n') {
//                 bluetoothCommand = 3; // 设置status3命令标志
//             }
//         }
//     }
// };
// // 处理蓝牙命令的函数
void handleBluetoothCommand(int command)
{
    Serial.printf("Handling Bluetooth command: %d, Free heap: %d\n", command, ESP.getFreeHeap());
    switch (command)
    {

    case 1:
    {
        bool wasPlaying = (isplaying == 1);

        // 优先使用库的 pauseResume 切换暂停（无缝）
        if (wasPlaying)
        {
            audio.pauseResume(); // 暂停流媒体（若库实现了）
            isplaying = 0;
        }

        int audioLen = 0;
        String audioData = sendToTTSWithConfig("您已超速请减速慢行", &audioLen, currentTtsConfig);
        audio.setVolume(15);
        if (audioLen > 0)
        {
            // 播放 TTS（保持你已有的 I2S 恢复/播放逻辑）
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 16000);
            i2s_zero_dma_buffer(I2S_NUM_0);
            i2s_start(I2S_NUM_0);
            delay(10);

            size_t bytes_written = 0;
            esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
            uint32_t playbackMs = (audioLen * 1000) / (16000UL * 2UL);
            delay(playbackMs + 20);
        }

        // 恢复播放（如果使用 pauseResume）
        if (wasPlaying)
        {
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 44100); // 音乐通常是 44.1kHz
            delay(10);
            audio.pauseResume(); // 再次调用恢复
            isplaying = 1;
        }
    }
    break;
    case 2:
    {

        bool wasPlaying = (isplaying == 1);

        // 优先使用库的 pauseResume 切换暂停（无缝）
        if (wasPlaying)
        {
            audio.pauseResume(); // 暂停流媒体（若库实现了）
            isplaying = 0;
        }
        int audioLen = 0;
        String audioData = sendToTTSWithConfig("请注意转弯", &audioLen, currentTtsConfig);
        if (audioLen > 0)
        {
            // 播放 TTS 前：停止 I2S 并恢复为 16kHz
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 16000);
            delay(10);

            size_t bytes_written = 0;
            esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
            uint32_t playbackMs = (audioLen * 1000) / (16000UL * 2UL);
            delay(playbackMs + 20);
        }
        if (wasPlaying)
        {
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 44100); // 音乐通常是 44.1kHz
            delay(10);
            audio.pauseResume(); // 再次调用恢复
            isplaying = 1;
        }
    }
    break;
    case 3:
    {
        // 停止音乐播放
        bool wasPlaying = (isplaying == 1);

        // 优先使用库的 pauseResume 切换暂停（无缝）
        if (wasPlaying)
        {
            audio.pauseResume(); // 暂停流媒体（若库实现了）
            isplaying = 0;
        }

        int audioLen = 0;
        String audioData = sendToTTSWithConfig("请注意倒车", &audioLen, currentTtsConfig);
        if (audioLen > 0)
        {
            // 播放 TTS 前：停止 I2S 并恢复为 16kHz
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 16000);
            delay(10);

            size_t bytes_written = 0;
            esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
            uint32_t playbackMs = (audioLen * 1000) / (16000UL * 2UL);
            delay(playbackMs + 20);
        }
        if (wasPlaying)
        {
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 44100); // 音乐通常是 44.1kHz
            delay(10);
            audio.pauseResume(); // 再次调用恢复
            isplaying = 1;
        }
    }
    break;
    case 4:
    {
        bool wasPlaying = (isplaying == 1);

        // 优先使用库的 pauseResume 切换暂停（无缝）
        if (wasPlaying)
        {
            audio.pauseResume(); //
            isplaying = 0;
        }

        int audioLen = 0;
        String audioData = sendToTTSWithConfig("电池电量低", &audioLen, currentTtsConfig);
        if (audioLen > 0)
        {
            // 播放 TTS 前：停止 I2S 并恢复为 16kHz
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 16000);
            delay(10);

            size_t bytes_written = 0;
            esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
            uint32_t playbackMs = (audioLen * 1000) / (16000UL * 2UL);
            delay(playbackMs + 20);
        }
        if (wasPlaying)
        {
            i2s_stop(I2S_NUM_0);
            delay(10);
            i2s_set_sample_rates(I2S_NUM_0, 44100); // 音乐通常是 44.1kHz
            delay(10);
            audio.pauseResume(); // 再次调用恢复
            isplaying = 1;
        }
    }
    break;
    case 5:
    {
        // 播放音乐前：停止 I2S 并切换为音乐采样率 44.1kHz
        i2s_stop(I2S_NUM_0);
        delay(10);
        i2s_set_sample_rates(I2S_NUM_0, 44100); // 音乐通常是 44.1kHz
        delay(10);
        LOG_F("Connecting to stream: %s", streamUrls[currentStreamIndex]);
        audio.setVolume(10);
        audio.connecttohost(streamUrls[currentStreamIndex]);
        isplaying = 1;
        LOG("Music playback initialized!");
    }
    break;
    case 6:
    {
        // 停止音乐播放
        if (isplaying == 1)
        {
            audio.stopSong();
            isplaying = 0;
        }
    }
    break;
    case 7:
    {
        int currentVolume = audio.getVolume();
        if (currentVolume < 21)
        {
            audio.setVolume(currentVolume + 1);
            Serial2.printf("Volume: %d\n", audio.getVolume());
        }
    }
    break;
    case 8:
    {
        int currentVolume = audio.getVolume();
        if (currentVolume > 0)
        {
            audio.setVolume(currentVolume - 1);
            Serial2.printf("Volume: %d\n", audio.getVolume());
        }
    }
    break;
    case 9:
    {
        // 停止当前播放
        audio.stopSong();
        // 切换到下一个音频流（循环切换）
        currentStreamIndex = (currentStreamIndex + 1) % streamCount;

        // 播放下一个音频流

        audio.connecttohost(streamUrls[currentStreamIndex]);
        isplaying = 1;

        Serial.printf("Switched to stream %d: %s\n", currentStreamIndex, streamUrls[currentStreamIndex]);
    }
    break;
    case 10:
    {
        // 停止当前播放
        audio.stopSong();

        // 切换到上一个音频流（循环切换）
        currentStreamIndex = (currentStreamIndex - 1 + streamCount) % streamCount;

        // 播放上一个音频流

        audio.connecttohost(streamUrls[currentStreamIndex]);
        isplaying = 1;

        Serial.printf("Switched to stream %d: %s\n", currentStreamIndex, streamUrls[currentStreamIndex]);
    }
    break;
    case 11:
    {
        audio.pauseResume(); // 暂停
    }
    break;
    default:
        break;
    }
}
void setup()
{
    // 电脑串口
    Serial.begin(115200);
    delay(1000);

    // 蓝牙串口
    Serial2.begin(9600, SERIAL_8N1, 11, 12);
    // STM32串口
    Serial1.begin(115200, SERIAL_8N1, 13, 14); // 可选：用于与另一个设备通信

    Serial2.println("ESP32 Started");
    Serial2.printf("startup FreeHeap: %d, PSRAM size: %d, FreePsram (approx): %d\n", ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getPsramSize() - 1000000 + ESP.getFreeHeap());
    // 设置 LED 引脚为输出
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, LOW);

    inmp441_setup();
    // max98357_setup();

    audio.setPinout(17, 18, 16);
    audio.setVolume(10);
    // 初始化按键引脚
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN_2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button1ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_2), button2ISR, FALLING);
    wifi_setup();
    stt_token = stt_gainToken();
    tts_token = tts_gainToken();

    Serial2.println("setup complete");
    Serial2.println("Press button to start voice recognition...");

    // 初始化 BLE
    // BLEDevice::init("ESP32test");
    // pServer = BLEDevice::createServer();
    // pServer->setCallbacks(new MyServerCallbacks());
    // // 创建 BLE 服务
    // BLEService *pService = pServer->createService(SERVICE_UUID);
    // // 创建 BLE 特征
    // pTxCharacteristic = pService->createCharacteristic(
    //     CHARACTERISTIC_UUID_TX,
    //     BLECharacteristic::PROPERTY_NOTIFY
    // );
    // pTxCharacteristic->addDescriptor(new BLE2902());
    // BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
    //     CHARACTERISTIC_UUID_RX,
    //     BLECharacteristic::PROPERTY_WRITE
    // );
    // pRxCharacteristic->setCallbacks(new MyCallbacks());
    // // 启动服务
    // pService->start();
    // // 开始广播
    // pServer->getAdvertising()->start();
    // Serial.println("BLE Started, Waiting for client connection");
}
void loop()
{
    // 播放音乐循环
    if (isplaying == 1)
    {
        audio.loop();
    }
    // 处理来自蓝牙的串口通信
    if (Serial2.available() > 0)
    {
        String receivedData = Serial2.readStringUntil('\n');
        receivedData.trim();
        if (receivedData == "status1")
        {
            bluetoothCommand = 1;
        }
        else if (receivedData == "status2")
        {
            bluetoothCommand = 2;
        }
        else if (receivedData == "status3")
        {
            bluetoothCommand = 3;
        }
        else if (receivedData == "status4")
        {
            bluetoothCommand = 4;
        }
        else if (receivedData == "on")
        {
            digitalWrite(LED1, HIGH);
        }
        else if (receivedData == "off")
        {
            digitalWrite(LED1, LOW);
        }
        else if (receivedData == "play")
        {
            bluetoothCommand = 5;
        }
        else if (receivedData == "stop")
        {
            bluetoothCommand = 6;
        }
        else if (receivedData == "volumeup")
        {
            bluetoothCommand = 7;
        }
        else if (receivedData == "volumedown")
        {
            bluetoothCommand = 8;
        }
        else if (receivedData == "next_song")
        {
            bluetoothCommand = 9;
        }
        else if (receivedData == "previous_song")
        {
            bluetoothCommand = 10;
        }
        else if (receivedData.indexOf("velocity") != -1)
        {
            Serial1.printf("%s\r\n", receivedData.c_str());
        }
        else if (receivedData == "pause/resume")
        {
            bluetoothCommand = 11;
        }
        // 处理播放列表命令
        // ...existing code...
        else if (receivedData.startsWith("ADDURL:"))
        {
            String url = receivedData.substring(7); // 去掉 "ADDURL:"
            playlist.addURL(url);
            Serial2.printf("OK: Added URL\r\n");
        }
        else if (receivedData.startsWith("PLAYLIST:"))
        {
            String arg = receivedData.substring(9); // 去掉 "PLAYLIST:"
            arg.trim();
            if (arg.startsWith("["))
            {
                // 传入 JSON 列表，设置播放列表
                if (playlist.setFromJSON(arg))
                {
                    Serial2.printf("OK: Playlist set (%d items)\r\n", playlist.getSize());
                }
                else
                {
                    Serial2.printf("ERROR: Invalid JSON\r\n");
                }
            }
            else if (arg.length() > 0 && isDigit(arg.charAt(0)))
            {
                // 传入索引，按索引播放
                int index = arg.toInt();
                String url = playlist.getURL(index);
                if (url.length() > 0)
                {
                    // 停止当前播放
                    if (isplaying == 1)
                    {
                        audio.stopSong();
                    }
                    // 播放列表中的音乐
                    i2s_stop(I2S_NUM_0);
                    delay(10);
                    i2s_set_sample_rates(I2S_NUM_0, 44100);
                    delay(10);
                    audio.setVolume(10);
                    audio.connecttohost(url.c_str());
                    isplaying = 1;
                    Serial2.printf("OK: Playing from playlist index %d\r\n", index);
                }
                else
                {
                    Serial2.printf("ERROR: Invalid index\r\n");
                }
            }
            else
            {
                Serial2.printf("ERROR: PLAYLIST argument invalid\r\n");
            }
        }

        else if (receivedData == "LISTSHOW")
        {
            playlist.printAll();
            Serial2.printf("OK: Playlist printed to Serial\r\n");
        }
        else if (receivedData == "LISTCLEAR")
        {
            playlist.clear();
            Serial2.printf("OK: Playlist cleared\r\n");
        }

        else if (receivedData.indexOf("A=") != -1)
        {
            int pos = receivedData.indexOf("A=");
            String numStr = receivedData.substring(pos + 2);
            numStr.trim();
            // 简单数字检测：允许负号
            if (numStr.length() > 0 && (isDigit(numStr.charAt(0)) || (numStr.charAt(0) == '-' && numStr.length() > 1 && isDigit(numStr.charAt(1)))))
            {
                int value = numStr.toInt(); // 转为 int
                Serial1.write(value);
            }
            else
            {
                // 非法数字时仍发送原始串或发送错误提示
                Serial1.printf("ERR: invalid number '%s'\r\n", numStr.c_str());
            }
        }
        else if (receivedData.startsWith("SET_PER:"))
        {
            int perValue = receivedData.substring(8).toInt();
            currentTtsConfig.per = perValue;
            Serial2.println("发音人已设置为: " + String(perValue));
        }
        // 命令格式示例："SET_SPD:7" 设置语速为7
        else if (receivedData.startsWith("SET_SPD:"))
        {
            int spdValue = receivedData.substring(8).toInt();
            currentTtsConfig.spd = spdValue;
            Serial2.println("语速已设置为: " + String(spdValue));
        }
        // 命令格式示例："SET_PIT:6" 设置音调为6
        else if (receivedData.startsWith("SET_PIT:"))
        {
            int pitValue = receivedData.substring(8).toInt();
            currentTtsConfig.pit = pitValue;
            Serial2.println("音调已设置为: " + String(pitValue));
        }
        // 命令格式示例："SET_VOL:9" 设置音量为9
        else if (receivedData.startsWith("SET_VOL:"))
        {
            int volValue = receivedData.substring(8).toInt();
            currentTtsConfig.vol = volValue;
            Serial2.println("音量已设置为: " + String(volValue));
        }
        Serial2.printf("Received: %s \r\n", receivedData);
    }
    // 处理STM32串口通信
    if (Serial1.available() > 0)
    {
        String receivedData = Serial1.readStringUntil('\n');
        if (receivedData == "status1")
        {
            bluetoothCommand = 1;
        }
        else if (receivedData == "status2")
        {
            bluetoothCommand = 2;
        }
        else if (receivedData == "status3")
        {
            bluetoothCommand = 3;
        }
        else if (receivedData == "status4")
        {
            bluetoothCommand = 4;
        }
        Serial2.printf("Received: %s \r\n", receivedData);
    }
    // 处理蓝牙命令
    if (bluetoothCommand != 0)
    {
        handleBluetoothCommand(bluetoothCommand);
        bluetoothCommand = 0; // 重置命令
    }
    // 检测按键按下以启动语音识别

    if (button1Pressed)
    {
        button1Pressed = false;
        // 执行语音识别逻辑
        handleVoiceRecognition();
    }

    if (button2Pressed)
    {
        button2Pressed = false;
        // 执行语音聊天逻辑
        handleVoiceChat();
    }
}