#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// 通义千问API配置
// 请确保API Key具有访问通义千问服务的权限
// 获取方式：登录阿里云控制台 -> DashScope -> API-KEY -> 创建我的API-KEY
const char* QWEN_API_KEY = "sk-6d02eabb93dd46c785f5e1020b930913	";  // 替换为您的DashScope API Key

String Qwen_accessToken;  // 通义千问不需要单独的访问令牌，直接使用API Key

String getQwenAnswer(String inputText) {
  // 检查API Key是否已配置
  if (strcmp(QWEN_API_KEY, "YOUR_DASHSCOPE_API_KEY") == 0) {
    Serial.println("===============================================");
    Serial.println("ERROR: 请先配置您的通义千问API Key!");
    Serial.println("请在my_Qwen.h文件中修改以下内容:");
    Serial.println("const char* QWEN_API_KEY = \"YOUR_DASHSCOPE_API_KEY\";");
    Serial.println("");
    Serial.println("获取API Key的步骤:");
    Serial.println("1. 访问阿里云DashScope平台: https://dashscope.console.aliyun.com");
    Serial.println("2. 创建API Key");
    Serial.println("3. 复制API Key并替换上述占位符");
    Serial.println("===============================================");
    return "<error>";
  }
  
  HTTPClient http;
  http.setTimeout(30000); // 30秒超时
  
  // 通义千问API端点
  String apiUrl = "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation";
  
  // 使用安全连接
  WiFiClientSecure client;
  client.setInsecure(); // 在生产环境中应该使用证书验证
  
  http.begin(client, apiUrl);
  http.addHeader("Authorization", "Bearer " + String(QWEN_API_KEY));
  http.addHeader("Content-Type", "application/json");
  // 不启用SSE流式输出，获取标准JSON响应
  
  // 构建请求参数
  String payload = "{\"model\": \"qwen-turbo\", \"input\": {\"messages\": [{\"role\": \"user\", \"content\": \"" + inputText + "，请用200字以内回答。\"}]}, \"parameters\": {\"max_tokens\": 1500}}";
  
  
  int httpResponseCode = http.POST(payload);
  if (httpResponseCode == 200) {
    String response = http.getString();
    http.end();
    

    // 解析JSON响应
    DynamicJsonDocument jsonDoc(2048);
    DeserializationError error = deserializeJson(jsonDoc, response);
    if (error) {
      Serial.println("JSON解析失败: " + String(error.c_str()));
      // 尝试处理SSE格式的响应
      // 查找最后一个包含完整结果的data行
      int lastDataIndex = response.lastIndexOf("data:");
      if (lastDataIndex != -1) {
        String lastDataLine = response.substring(lastDataIndex + 5);
        // 截取到行尾
        int nextLineIndex = lastDataLine.indexOf("\n");
        if (nextLineIndex != -1) {
          lastDataLine = lastDataLine.substring(0, nextLineIndex);
        }
        lastDataLine.trim();
        
        Serial.println("尝试解析最后的SSE数据: " + lastDataLine);
        DeserializationError sseError = deserializeJson(jsonDoc, lastDataLine);
        if (sseError) {
          Serial.println("SSE数据解析也失败: " + String(sseError.c_str()));
          return "<error>";
        }
      } else {
        return "<error>";
      }
    }
    
    if (jsonDoc.containsKey("output") && jsonDoc["output"].containsKey("text")) {
      String outputText = jsonDoc["output"]["text"];
      return outputText;
    } else if (jsonDoc.containsKey("message")) {
      String errorMsg = jsonDoc["message"];
      Serial.println("通义千问错误信息: " + errorMsg);
      return "<error: " + errorMsg + ">";
    } else {
      Serial.println("无法解析响应内容");
      return "<error>";
    }
  } else {
    String response = http.getString();
    http.end();
    Serial.printf("通义千问HTTP错误 %i: %s\n", httpResponseCode, response.c_str());
    return "<error>";
  }
  
  http.end();
  return "<error>";
}

void Qwen_setup() {
  Serial.println("初始化通义千问...");
  
  // 测试API Key是否有效
  String answer = getQwenAnswer("你好，通义千问");
  Serial.println("<测试回答: " + answer + ">");
}