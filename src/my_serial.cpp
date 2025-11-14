#include "my_serial.h"
#include <Arduino.h>

// 默认构造函数，使用 Serial (UART0)
SerialCommunication::SerialCommunication(int baud) : serialPort(&Serial), baudRate(baud), rxPin(-1), txPin(-1), useCustomPins(false) {}

// 使用指定串口的构造函数
SerialCommunication::SerialCommunication(HardwareSerial& serial, int baud) : serialPort(&serial), baudRate(baud), rxPin(-1), txPin(-1), useCustomPins(false) {}

// 使用指定串口和自定义引脚的构造函数
SerialCommunication::SerialCommunication(HardwareSerial& serial, int rxPin, int txPin, int baud) : 
    serialPort(&serial), baudRate(baud), rxPin(rxPin), txPin(txPin), useCustomPins(true) {}

// 初始化串口
void SerialCommunication::begin() {
    if (useCustomPins) {
        // 使用自定义引脚初始化串口
        Serial.println("Initializing Serial with custom pins: RX=" + String(rxPin) + ", TX=" + String(txPin) + ", Baud=" + String(baudRate));
        serialPort->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    } else {
        // 使用默认引脚初始化串口
        Serial.println("Initializing Serial with default pins, Baud=" + String(baudRate));
        serialPort->begin(baudRate);
    }
    
    // 等待串口准备就绪
    delay(100);
}

// 检查是否有数据可读
bool SerialCommunication::available() {
    return serialPort->available();
}

// 读取字符串命令
String SerialCommunication::readCommand() {
    String command = "";
    
    // 读取所有当前可用的数据
    while (serialPort->available()) {
        char c = serialPort->read();
        command += c;
        delay(2);  // 短暂延迟以允许更多字符到达
    }
    
    // 打印调试信息
    if (command.length() > 0) {
        Serial.print("Raw data length: ");
        Serial.println(command.length());
        Serial.print("Raw data ASCII: ");
        for (int i = 0; i < command.length(); i++) {
            Serial.print((int)command.charAt(i));
            Serial.print(" ");
        }
        Serial.println();
    }
    
    return command;
}

// 发送字符串数据
void SerialCommunication::sendString(const String& data) {
    serialPort->print(data);
}

// 发送整数数据
void SerialCommunication::sendInt(int value) {
    serialPort->print(value);
}

// 发送浮点数据
void SerialCommunication::sendFloat(float value) {
    serialPort->print(value);
}

// 发送行数据（带换行符）
void SerialCommunication::sendLine(const String& data) {
    serialPort->println(data);
}

// 发送原始数据
void SerialCommunication::sendData(const uint8_t* data, size_t len) {
    serialPort->write(data, len);
}

// 处理接收到的命令
void SerialCommunication::processCommand(const String& command) {
    // 去除首尾空格
    String cmd = command;
    cmd.trim();
    
    // 转换为小写以便比较
    cmd.toLowerCase();
    
    // 显示接收到的任何非空命令（包括"error"）
    if (cmd.length() > 0) {
        Serial.println("Received from STM32: '" + cmd + "' (length: " + cmd.length() + ")");
    }
    
    // 根据命令执行相应操作
    if (cmd == "help") {
        sendLine("Available commands:");
        sendLine("help - Show this help");
        sendLine("led on - Turn on LED");
        sendLine("led off - Turn off LED");
        sendLine("status - Show device status");
        sendLine("version - Show firmware version");
    } 
    else if (cmd == "led on") {
        digitalWrite(9, HIGH);
        sendLine("LED turned ON");
    } 
    else if (cmd == "led off") {
        digitalWrite(9, LOW);
        sendLine("LED turned OFF");
    } 
    else if (cmd == "status") {
        sendLine("Device Status:");
        sendLine("- LED: " + String(digitalRead(9) ? "ON" : "OFF"));
        sendLine("- Firmware: 1.0");
    } 
    else if (cmd == "version") {
        sendLine("Firmware Version: 1.0");
    } 
    else if (cmd.length() > 0) {
        sendLine("Unknown command: " + command);
        sendLine("Type 'help' for available commands");
    }
}