#include <Arduino.h>
#include "UrlEncode.h"
#include "my_wifi.h"
#include "my_stt.h"
#include "my_tts.h"
#include "my_inmp441_max98357.h"
#include "my_Qwen.h"  // 替换为通义千问

// 定义按键引脚
const int BUTTON_PIN = 3; // 可以根据实际情况修改引脚号
bool isListening = false; // 语音识别状态标志

void setup() {
  Serial.begin(115200);
  
  // 初始化按键引脚
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  wifi_setup();
  stt_token = stt_gainToken();
  tts_token = tts_gainToken();
  // ErnieBot_accessToken = ErnieBotGainToken();  // 移除文心一言初始化
  inmp441_max98357_setup();
  Serial.println("setup complete");
  Serial.println("Press button to start voice recognition...");
  Qwen_setup();  // 初始化通义千问
}

void loop() {
  // 检查按键状态
  if (digitalRead(BUTTON_PIN) == LOW) {
    // 按键按下，开始语音识别
    if (!isListening) {
      isListening = true;
      Serial.println("Button pressed, starting voice recognition...");
      
      // 采集音频数据到adc_data数组
      size_t bytes_read = 0;
      esp_err_t result = i2s_read(I2S_NUM_0, &adc_data, sizeof(adc_data), &bytes_read, portMAX_DELAY);
      Serial.printf("Read %d bytes from I2S\n", bytes_read);
      
      // 检查是否读取到数据
      if (bytes_read > 0) {
        // 执行语音识别流程
        stt_setup();
        stt_assembleJson();
        String recognizedText = sendToSTT(); // 直接获取识别结果
        
        // 将识别到的文字发送给大模型
        if (recognizedText.length() > 0) {
          Serial.println("Sending text to Qwen: " + recognizedText);  // 修改为通义千问
          String answer = getQwenAnswer(recognizedText);  // 调用通义千问
          Serial.println("Qwen answer: " + answer);  // 修改为通义千问
          

          // 将通义千问的回答转换为语音
          if (answer.length() > 0 && answer != "<error>") {
            Serial.println("Converting answer to speech...");
            int audioLen = 0;
            String audioData = sendToTTS(answer, &audioLen);
            Serial.println("TTS conversion completed");
            if (audioLen > 0) {
              // 播放合成的音频
              size_t bytes_written = 0;
              esp_err_t writeResult = i2s_write(I2S_NUM_1, audioData.c_str(), audioLen, &bytes_written, portMAX_DELAY);
              Serial.printf("Wrote %d bytes to I2S for playback\n", bytes_written);
            } else {
              Serial.println("No audio data received from TTS");
            }
          }
        } else {
          Serial.println("No valid text recognized, skipping Qwen call");  // 修改为通义千问
        }
      } else {
        Serial.println("No audio data captured");
      }
      
      isListening = false;
    }
    
    // 简单的按键去抖动处理
    delay(500);
  }
  
  // 正常的音频循环处理
  inmp441_max98357_loop();
}