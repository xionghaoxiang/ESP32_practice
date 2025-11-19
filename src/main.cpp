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
#include "my_Qwen.h"  // 替换为通义千问
#include "Audio.h"

// LED1 引脚定义
#define LED1 9
// 添加日志宏保持代码简洁
#define LOG(msg) Serial.printf("[%5lu ms] %s\n", millis(), msg)
#define LOG_F(format, ...) Serial.printf("[%5lu ms] " format, millis(), ##__VA_ARGS__)
// 定义按键引脚
const int BUTTON_PIN = 3; // 可以根据实际情况修改引脚号
bool isListening = false; // 语音识别状态标志
Audio audio;
const char* streamUrls[] = {
    "http://music.163.com/song/media/outer/url?id=431551064.mp3",
    "http://music.163.com/song/media/outer/url?id=1980818176.mp3",
    "http://music.163.com/song/media/outer/url?id=28768892.mp3", // 示例第三首歌曲
    "http://music.163.com/song/media/outer/url?id=2628344945.mp3"// 可以继续添加更多音频流URL
};
const int streamCount = sizeof(streamUrls) / sizeof(streamUrls[0]); // 自动计算音频流数量
int currentStreamIndex = 0; // 当前播放的音频流索引
bool pinok;
int isplaying = 0;

// BLEServer *pServer = NULL;
// BLECharacteristic * pTxCharacteristic;
// bool deviceConnected = false;
// bool oldDeviceConnected = false;
// uint8_t txValue = 0;

// 添加状态标志用于处理蓝牙命令
volatile int bluetoothCommand = 0; // 0=无命令, 1=status1, 2=status2, 3=status3

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
void handleBluetoothCommand(int command) {
    Serial.printf("Handling Bluetooth command: %d, Free heap: %d\n", command, ESP.getFreeHeap());
    
    switch(command) {
       
        case 1: {
            int audioLen = 0;
            String audioData = sendToTTS("您已超速请减速慢行", &audioLen);
            audio.setVolume(15);
            if (audioLen > 0) {
                // 播放合成的音频
                size_t bytes_written = 0;
                esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
                delay(audioLen / 64);
            }
            break;
        }
        case 2: {
            int audioLen = 0;
            String audioData = sendToTTS("请注意转弯", &audioLen);
            if (audioLen > 0) {
                // 播放合成的音频
                size_t bytes_written = 0;
                esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
                delay(audioLen / 64);  
            }
            break;
        }
        case 3: {
            int audioLen = 0;
            String audioData = sendToTTS("请注意倒车", &audioLen);
            if (audioLen > 0) {
                // 播放合成的音频
                size_t bytes_written = 0;
                esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
                delay(audioLen / 64); 
            }
            break;
        }
        case 4:{
             int audioLen = 0;
            String audioData = sendToTTS("电池电量低", &audioLen);
            if (audioLen > 0) {
                // 播放合成的音频
                size_t bytes_written = 0;
                esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
                delay(audioLen / 64); 
            }
            }
                break;
        case 5:{
        LOG_F("Connecting to stream: %s", streamUrls[currentStreamIndex]);
        audio.setVolume(10); 
        audio.connecttohost(streamUrls[currentStreamIndex]); 
        isplaying = 1;
        LOG("Initialization complete!");
            }
                break;
        default:
            break;
    }
    
//     Serial.printf("Finished handling Bluetooth command, Free heap: %d\n", ESP.getFreeHeap());
// }
}
 void setup() {
    //电脑串口
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 Started");
    Serial.printf("startup FreeHeap: %d, PSRAM size: %d, FreePsram (approx): %d\n", 
    ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getPsramSize() - 1000000 + ESP.getFreeHeap());
    
    
    Serial2.begin(9600,SERIAL_8N1, 11, 12);
    //STM32串口
    Serial1.begin(9600,SERIAL_8N1, 13, 14); // 可选：用于与另一个设备通信

    // 设置 LED 引脚为输出
    pinMode(LED1, OUTPUT);           
    digitalWrite(LED1, LOW);
    inmp441_setup();
    //max98357_setup();

    
    audio.setPinout(17, 18, 16);
    audio.setVolume(10); 
    // 初始化按键引脚
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    wifi_setup();
    stt_token = stt_gainToken();
    tts_token = tts_gainToken();

    
    Serial.println("setup complete");
    Serial.println("Press button to start voice recognition...");
    
    
   
    // // 初始化 BLE
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

void loop() {
    
    if (isplaying == 1)
      { audio.loop();} 

    // 处理来自蓝牙的串口通信
    if (Serial2.available() > 0)
    {
        String receivedData = Serial2.readStringUntil('\n');
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
       
        else if(receivedData == "stop")
        {
            audio.stopSong();
            isplaying = 0;
        }
        else if(receivedData == "volumeup")
        {
            int currentVolume = audio.getVolume();
            if (currentVolume < 21) {
                audio.setVolume(currentVolume + 1);
            }
        }
        else if(receivedData == "volumedown")
        {
            int currentVolume = audio.getVolume();
            if (currentVolume > 0) {
                audio.setVolume(currentVolume - 1);
            }
        }
        else if(receivedData == "next_song")
        {
            // 停止当前播放
            audio.stopSong();
            
            // 切换到下一个音频流（循环切换）
            currentStreamIndex = (currentStreamIndex + 1) % streamCount;
            
            // 播放下一个音频流
            audio.setVolume(10); 
            audio.connecttohost(streamUrls[currentStreamIndex]);
            isplaying = 1;
            
            Serial.printf("Switched to stream %d: %s\n", currentStreamIndex, streamUrls[currentStreamIndex]);
        }
            else if(receivedData == "previous_song")
        {
            // 停止当前播放
            audio.stopSong();
            
            // 切换到上一个音频流（循环切换）
            currentStreamIndex = (currentStreamIndex - 1 + streamCount) % streamCount;
            
            // 播放上一个音频流
            audio.setVolume(10); 
            audio.connecttohost(streamUrls[currentStreamIndex]);
            isplaying = 1;
            
            Serial.printf("Switched to stream %d: %s\n", currentStreamIndex, streamUrls[currentStreamIndex]);
        }
  
        else
        {
        
        }
        
        Serial2.printf("Received: %s \r\n", receivedData);
    }


    //处理STM32串口通信
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
        Serial1.printf("Received: %s \r\n", receivedData);
    }

    // 处理蓝牙命令
    if (bluetoothCommand != 0) {
        handleBluetoothCommand(bluetoothCommand);
        bluetoothCommand = 0; // 重置命令
    }


    
    // 可以在这里添加向STM32发送数据的代码
    

    



    if (digitalRead(BUTTON_PIN) == LOW) {
        // 按键按下，开始语音识别
        if (!isListening) {
            isListening = true;
            Serial.println("Button pressed, starting voice recognition...");
            
            // 采集音频数据到adc_data数组
            size_t bytes_read = 0;
            esp_err_t result = i2s_read(I2S_NUM_1, &adc_data, sizeof(adc_data), &bytes_read, portMAX_DELAY);
            Serial.printf("Read %d bytes from I2S\n", bytes_read);
            
            // 检查是否读取到数据
            if (bytes_read > 0) {
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
                else if(recognizedText.indexOf("播放音乐") != -1)
                {
                    bluetoothCommand = 5;
                }
                else if(recognizedText.indexOf("停止播放") != -1)
                {
                    audio.stopSong();
                    isplaying = 0;
                }
                
                int audioLen = 0;



                String audioData = sendToTTS(recognizedText, &audioLen);
                Serial.println("TTS conversion completed");
                if (audioLen > 0)
                {
                 // 播放合成的音频
                size_t bytes_written = 0;
                esp_err_t writeResult = i2s_write(I2S_NUM_0, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
                delay(audioLen / 64);

                Serial.printf("Wrote %d bytes to I2S for playback\n", bytes_written);
                } 

            
            
            //     // 将识别到的文字发送给大模型
            //     if (recognizedText.length() > 0 && recognizedText != "") {
            //         Serial.println("Sending text to Qwen: " + recognizedText);  // 修改为通义千问
            //         String answer = getQwenAnswer(recognizedText);  // 调用通义千问
            //         Serial.println("Qwen answer: " + answer);  // 修改为通义千问
                    
            //         // 将通义千问的回答转换为语音
            //         if (answer.length() > 0 && answer != "<error>") {
            //             Serial.println("Converting answer to speech...");
            //             int audioLen = 0;
            //             String audioData = sendToTTS(answer, &audioLen);
            //             Serial.println("TTS conversion completed");
            //             if (audioLen > 0) {
            //                 // 播放合成的音频
            //                 size_t bytes_written = 0;
            //                 esp_err_t writeResult = i2s_write(I2S_NUM_1, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
            //                 Serial.printf("Wrote %d bytes to I2S for playback\n", bytes_written);
            //             } else {
            //                 Serial.println("No audio data received from TTS");
            //             }
            //         }
            //     } else {
            //         Serial.println("No valid text recognized, skipping Qwen call");  // 修改为通义千问
            //     }
            // } else {
                Serial.println("No audio data captured");
            }
            
            isListening = false;
        }
        
        // 简单的按键去抖动处理
        delay(500);
    }
}