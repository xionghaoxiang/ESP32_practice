#include <Arduino.h>
#include <driver/i2s.h>
#include "base64.h"
#include "HTTPClient.h"
#include "cJSON.h"
#include <ArduinoJson.h>
 
// 1、修改百度语言技术的用户信息：https://console.bce.baidu.com/ai/?fromai=1#/ai/speech/app/list
const int STT_DEV_PID = 1537; //选填，输入法模型 1737-英语 1537-普通话(近场识别模型) 1936-普通话远程识别 1837-四川话 
const char *STT_CUID = "8C:BF:EA:1A:F2:B8"; //用户唯一标识，用来区分用户，计算UV值。建议填写能区分用户的机器 MAC 地址或 IMEI 码，长度为60字符以内。
const char *STT_CLIENT_ID = "xenO6P1P2HqUKjq0MNbEP3LX"; //API Key
const char *STT_CLIENT_SECRET = "9UW81Yiregz4HG1YtyN0Be2AkkNKA3VT"; //Secret Key
 
String stt_token;
 
const int adc_data_len = 1024*16*2;
const int data_json_len = adc_data_len * 2 * 1.4;
uint16_t adc_data[adc_data_len];
char data_json[data_json_len];
 
String stt_gainToken() {
  HTTPClient stt_http;
  String token;
  String url = String("https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=") + STT_CLIENT_ID + "&client_secret=" + STT_CLIENT_SECRET;
 
  stt_http.begin(url);
  int httpCode = stt_http.GET();
 
  if (httpCode > 0) {
    String payload = stt_http.getString();
    Serial.println("STT Token response: " + payload); // 添加调试信息
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    // 检查是否有错误信息
    if (doc.containsKey("error")) {
      String error = doc["error"];
      Serial.println("STT Token error: " + error);
      if (doc.containsKey("error_description")) {
        String errorDesc = doc["error_description"];
        Serial.println("STT Token error description: " + errorDesc);
      }
      stt_http.end();
      return token; // 返回空字符串
    }
    
    token = doc["access_token"].as<String>();
    Serial.println("stt:" + token);
  } else {
    Serial.println("Error on HTTP request for token, HTTP code: " + String(httpCode));
    Serial.println("Error: " + stt_http.errorToString(httpCode));
  }
  stt_http.end();
  return token;
}
 
void stt_assembleJson() {
  memset(data_json, '\0', data_json_len * sizeof(char));
  strcat(data_json, "{");
  strcat(data_json, "\"format\":\"pcm\",");
  strcat(data_json, "\"rate\":16000,");
  strcat(data_json, "\"dev_pid\":1537,");
  strcat(data_json, "\"channel\":1,");
  strcat(data_json, "\"cuid\":\""); strcat(data_json, STT_CUID); strcat(data_json, "\",");
  strcat(data_json, "\"token\":\""); strcat(data_json, stt_token.c_str()); strcat(data_json, "\",");
  sprintf(data_json + strlen(data_json), "\"len\":%d,", adc_data_len * 2);
  strcat(data_json, "\"speech\":\"");
  strcat(data_json, base64::encode((uint8_t *)adc_data, adc_data_len * sizeof(uint16_t)).c_str());
  //int tmp = base64::decode((char *)adc_data, adc_data_len, data_json);
  strcat(data_json, "\"");
  strcat(data_json, "}");
  //Serial.println(data_json);
}
 
 
String sendToSTT() {
  // 检查令牌是否有效，如果无效则重新获取
  if (stt_token == NULL || stt_token.length() == 0) {
    Serial.println("STT token is invalid, trying to get a new one...");
    stt_token = stt_gainToken();
    if (stt_token == NULL || stt_token.length() == 0) {
      Serial.println("Failed to get STT token");
      return String("获取访问令牌失败!");
    }
  }
  
  stt_assembleJson();
  HTTPClient http_client_stt;
  http_client_stt.begin("http://vop.baidu.com/server_api");//短语音识别请求地址: 标准版http://vop.baidu.com/server_api, 极速版https://vop.baidu.com/pro_api
  http_client_stt.addHeader("Content-Type", "application/json");

  int httpCode = http_client_stt.POST(data_json);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String response = http_client_stt.getString();
      Serial.println("语音识别完整响应: " + response);
      
      // 检查响应中是否包含错误信息
      DynamicJsonDocument respDoc(1024);
      deserializeJson(respDoc, response);
      if (respDoc.containsKey("err_no") && respDoc["err_no"] != 0) {
        int errNum = respDoc["err_no"];
        String errMsg = respDoc["err_msg"] | "Unknown error";
       
        // 如果是令牌相关错误，则尝试重新获取令牌
        if (errNum == 3302) { // Access token invalid or no longer valid
          Serial.println("Access token expired, trying to get a new one...");
          stt_token = stt_gainToken();
          if (stt_token != NULL && stt_token.length() > 0) {
            // 重新尝试一次识别
            stt_assembleJson();
            httpCode = http_client_stt.POST(data_json);
            if (httpCode == HTTP_CODE_OK) {
              response = http_client_stt.getString();
              Serial.println("语音识别完整响应: " + response);
              http_client_stt.end();
              
              // 解析识别结果
              DynamicJsonDocument resultDoc(1024);
              DeserializationError error = deserializeJson(resultDoc, response);
              if (!error && resultDoc.containsKey("result")) {
                JsonArray resultArray = resultDoc["result"];
                if (resultArray.size() > 0) {
                  return resultArray[0].as<String>();
                }
              }
              return "";
            }
          }
        }
        
        http_client_stt.end();
        return String("识别失败: ") + errMsg;
      }
      
      // 正常解析识别结果
      if (respDoc.containsKey("result")) {
        JsonArray resultArray = respDoc["result"];
        if (resultArray.size() > 0) {
          String result = resultArray[0].as<String>();
          http_client_stt.end();
          return result;
        }
      }
      
      http_client_stt.end();
      return "";
    }
  } else {
    Serial.printf("[HTTP] POST failed, error: %s\n", http_client_stt.errorToString(httpCode).c_str());
    http_client_stt.end();
    return String("响应失败请重新获取!");
  }
  
  http_client_stt.end();
  return "";
}
 
void stt_setup()
{
  stt_token = stt_gainToken();
  //Serial.println(stt_token.c_str());
}