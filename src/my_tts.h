#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>
#include "cJSON.h"
#include <esp_heap_caps.h>

// 1、修改百度语言技术的用户信息：https://console.bce.baidu.com/ai/?fromai=1#/ai/speech/app/list
const char *TTS_CUID = "8C:BF:EA:1A:F2:B8";                         // 用户唯一标识，用来区分用户，计算UV值。建议填写能区分用户的机器 MAC 地址或 IMEI 码，长度为60字符以内。
const char *TTS_CLIENT_ID = "xenO6P1P2HqUKjq0MNbEP3LX";             // API Key
const char *TTS_CLIENT_SECRET = "9UW81Yiregz4HG1YtyN0Be2AkkNKA3VT"; // Secret Key

String tts_token;

// 百度语音合成的API URL
const char *tts_url = "http://tsn.baidu.com/text2audio"; // 使用HTTP而不是HTTPS

// 使用静态HTTPClient对象以减少内存分配
static HTTPClient *tts_http = nullptr;

typedef struct
{
  int per; // 发音人编号
  int spd; // 语速
  int pit; // 音调
  int vol; // 音量
} TTSConfig;

TTSConfig currentTtsConfig = {3, 1, 3, 5};
extern TTSConfig currentTtsConfig;

String tts_gainToken()
{
  HTTPClient http;
  String token;
  String url = String("http://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=") + TTS_CLIENT_ID + "&client_secret=" + TTS_CLIENT_SECRET;

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    token = doc["access_token"].as<String>();
    Serial.println("tts:" + token);
  }
  else
  {
    Serial.println("Error on HTTP request for token");
  }
  http.end();
  return token;
}

String sendToTTS(String InputText, int *len)
{
  // 检查是否有足够内存
  Serial.printf("Before TTS request - Free heap: %d\n", ESP.getFreeHeap());

  InputText = urlEncode(InputText); // tex字段2次urlencode
  InputText = urlEncode(InputText); // 百度为了更好地兼容，支持1次及2次urlencode， 其中2次urlencode可以覆盖全部的特殊字符。因而推荐传递tex 参数时做2次urlencode编码

  // 创建或重用HTTPClient对象
  if (tts_http == nullptr)
  {
    tts_http = new HTTPClient;
    if (tts_http)
    {
      tts_http->begin(tts_url);                                                 // 初始化HTTP请求
      tts_http->addHeader("Content-Type", "application/x-www-form-urlencoded"); // 根据API要求添加HTTP头  application/x-www-form-urlencoded
    }
    else
    {
      Serial.println("Failed to create HTTPClient object for TTS");
      *len = 0;
      return "";
    }
  }

  String payload = String("tex=") + InputText.c_str() + String("&tok=") + tts_token.c_str() + String("&cuid=") + TTS_CUID + String("&ctp=1&lan=zh&spd=0&pit=5&vol=1&per=5&aue=4");
  // Serial.println(payload);

  String outputText;
  int httpCode = tts_http->POST(payload); // 发送POST请求
  if (httpCode == HTTP_CODE_OK)
  {
    String response = tts_http->getString(); // 获取响应体
    // Serial.println(response);
    *len = tts_http->getSize();
    Serial.println(*len);
    // 注意：这里不清除连接，以便重用
    return response;
  }
  else
  {
    Serial.printf("Error in the HTTP request, code: %d\n", httpCode);
    outputText = String("Error in the HTTP request");
  }

  // 注意：这里不清除连接，以便重用
  return outputText;
}

void tts_setup()
{
  tts_token = tts_gainToken();
  // Serial.println(tts_token.c_str());
}
String getTTSUrl(String InputText)
{
  InputText = urlEncode(InputText);
  InputText = urlEncode(InputText);

  String url = String(tts_url) + "?tex=" + InputText.c_str() + "&tok=" + tts_token.c_str() + "&cuid=" + TTS_CUID + "&ctp=1&lan=zh&spd=5&pit=5&vol=1&per=5&aue=4";
  return url;
}

String sendToTTSWithConfig(String InputText, int *len, const TTSConfig &config)
{
  Serial.printf("Before TTS request - Free heap: %d\n", ESP.getFreeHeap());

  InputText = urlEncode(InputText);
  InputText = urlEncode(InputText);

  if (tts_http == nullptr)
  {
    tts_http = new HTTPClient;
    if (tts_http)
    {
      tts_http->begin(tts_url);
      tts_http->addHeader("Content-Type", "application/x-www-form-urlencoded");
    }
    else
    {
      Serial.println("Failed to create HTTPClient object for TTS");
      *len = 0;
      return "";
    }
  }

  String payload = String("tex=") + InputText.c_str() +
                   "&tok=" + tts_token.c_str() +
                   "&cuid=" + TTS_CUID +
                   "&ctp=1&lan=zh" +
                   "&spd=" + String(config.spd) +
                   "&pit=" + String(config.pit) +
                   "&vol=" + String(config.vol) +
                   "&per=" + String(config.per) +
                   "&aue=4";

  String outputText;
  int httpCode = tts_http->POST(payload);
  if (httpCode == HTTP_CODE_OK)
  {
    String response = tts_http->getString();
    *len = tts_http->getSize();
    Serial.println(*len);
    return response;
  }
  else
  {
    Serial.printf("Error in the HTTP request, code: %d\n", httpCode);
    outputText = String("Error in the HTTP request");
  }

  return outputText;
}