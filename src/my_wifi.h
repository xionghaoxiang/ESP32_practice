#include <WiFi.h>

void wifi_setup()
{
  WiFi.disconnect(true);
  // 3、填写您的wifi账号密码
  WiFi.begin("xhx", "xhx1958105721");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    vTaskDelay(200);
  }
  Serial2.println("\n-- wifi connect success! --");
  Serial2.print("ESP32 IP Address: ");
  Serial2.println(WiFi.localIP());
}