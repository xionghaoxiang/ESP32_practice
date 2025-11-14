#ifndef MY_SERIAL_H
#define MY_SERIAL_H

#include <Arduino.h>

class SerialCommunication {
private:
    HardwareSerial* serialPort;
    int baudRate;
    int rxPin;
    int txPin;
    bool useCustomPins;
    
public:
    // 构造函数，默认使用Serial (UART0)
    SerialCommunication(int baud = 115200);
    
    // 使用指定的串口初始化
    SerialCommunication(HardwareSerial& serial, int baud = 115200);
    
    // 使用指定串口和自定义引脚初始化
    SerialCommunication(HardwareSerial& serial, int rxPin, int txPin, int baud = 115200);
    
    // 初始化串口
    void begin();
    
    // 检查是否有数据可读
    bool available();
    
    // 读取字符串命令
    String readCommand();
    
    // 发送字符串数据
    void sendString(const String& data);
    
    // 发送整数数据
    void sendInt(int value);
    
    // 发送浮点数据
    void sendFloat(float value);
    
    // 发送行数据（带换行符）
    void sendLine(const String& data);
    
    // 发送原始数据
    void sendData(const uint8_t* data, size_t len);
    
    // 处理接收到的命令
    void processCommand(const String& command);
};

#endif // MY_SERIAL_H