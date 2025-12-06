import websocket
import threading
import time
import pyaudio
import numpy as np
import sys

# 音频参数
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100
CHUNK = 1024

class WifiAudioClient:
    def __init__(self, host):
        self.host = host
        self.ws = None
        self.audio = pyaudio.PyAudio()
        self.stream_in = None
        self.stream_out = None
        self.is_sending = False
        self.is_receiving = False
        
    def connect(self):
        """连接到ESP32的WebSocket服务器"""
        try:
            self.ws = websocket.WebSocketApp(
                f"ws://{self.host}:81/",
                on_message=self.on_message,
                on_error=self.on_error,
                on_close=self.on_close,
                on_open=self.on_open
            )
            
            # 在单独线程中运行WebSocket
            self.wst = threading.Thread(target=self.ws.run_forever)
            self.wst.daemon = True
            self.wst.start()
            
            # 等待连接建立
            time.sleep(2)
            
            if self.ws.sock and self.ws.sock.connected:
                print(f"成功连接到ESP32 WiFi音频服务器 {self.host}:81")
                return True
            else:
                print("连接失败")
                return False
                
        except Exception as e:
            print(f"连接错误: {e}")
            return False
    
    def on_open(self, ws):
        print("WebSocket连接已打开")
        
    def on_message(self, ws, message):
        """处理从ESP32接收到的消息"""
        if isinstance(message, str):
            print(f"收到文本消息: {message}")
        else:
            # 接收到音频数据
            if self.is_receiving and self.stream_out:
                try:
                    self.stream_out.write(message)
                except Exception as e:
                    print(f"播放音频时出错: {e}")
    
    def on_error(self, ws, error):
        print(f"WebSocket错误: {error}")
        # 不要在这里调用stop_all，避免递归调用
        # 连接关闭会触发on_close回调
    
    def on_close(self, ws, close_status_code, close_msg):
        print("WebSocket连接已关闭")
        self.stop_all()
    
    def start_audio_tx(self):
        """开始音频发送（麦克风到ESP32）"""
        if not self.ws or not hasattr(self.ws, 'sock') or not self.ws.sock or not self.ws.sock.connected:
            print("请先连接到服务器")
            return
            
        print("开始音频发送...")
        try:
            self.ws.send("START_AUDIO_TX")
        except Exception as e:
            print(f"无法发送控制命令: {e}")
            return
        
        # 打开音频输入流
        try:
            self.stream_in = self.audio.open(
                format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                input=True,
                frames_per_buffer=CHUNK
            )
        except Exception as e:
            print(f"无法打开音频输入流: {e}")
            return
        
        self.is_sending = True
        
        # 在单独线程中发送音频数据
        self.send_thread = threading.Thread(target=self._send_audio)
        self.send_thread.daemon = True
        self.send_thread.start()
    
    def stop_audio_tx(self):
        """停止音频发送"""
        print("停止音频发送...")
        self.is_sending = False
        if self.ws and hasattr(self.ws, 'sock') and self.ws.sock:
            try:
                if self.ws.sock.connected:
                    self.ws.send("STOP_AUDIO_TX")
            except:
                pass  # 忽略发送停止命令时的任何错误
        if self.stream_in:
            try:
                self.stream_in.stop_stream()
                self.stream_in.close()
            except:
                pass
            self.stream_in = None
    
    def start_audio_rx(self):
        """开始音频接收（ESP32到扬声器）"""
        if not self.ws or not hasattr(self.ws, 'sock') or not self.ws.sock or not self.ws.sock.connected:
            print("请先连接到服务器")
            return
            
        print("开始音频接收...")
        try:
            self.ws.send("START_AUDIO_RX")
        except Exception as e:
            print(f"无法发送控制命令: {e}")
            return
        
        # 打开音频输出流
        try:
            self.stream_out = self.audio.open(
                format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                output=True,
                frames_per_buffer=CHUNK
            )
        except Exception as e:
            print(f"无法打开音频输出流: {e}")
            return
        
        self.is_receiving = True
    
    def stop_audio_rx(self):
        """停止音频接收"""
        print("停止音频接收...")
        self.is_receiving = False
        if self.ws and hasattr(self.ws, 'sock') and self.ws.sock:
            try:
                if self.ws.sock.connected:
                    self.ws.send("STOP_AUDIO_RX")
            except:
                pass  # 忽略发送停止命令时的任何错误
        if self.stream_out:
            try:
                self.stream_out.stop_stream()
                self.stream_out.close()
            except:
                pass
            self.stream_out = None
    
    def start_bidirectional(self):
        """开始双向音频传输"""
        self.start_audio_tx()
        self.start_audio_rx()
    
    def stop_all(self):
        """停止所有音频传输"""
        # 防止重复调用导致的问题
        if hasattr(self, '_stopping'):
            return
        self._stopping = True
        self.stop_audio_tx()
        self.stop_audio_rx()
        delattr(self, '_stopping')
    
    def _send_audio(self):
        """发送音频数据的线程函数"""
        while self.is_sending:
            try:
                # 更严格的连接检查
                if not self.ws or not hasattr(self.ws, 'sock') or not self.ws.sock or not self.ws.sock.connected:
                    print("WebSocket连接已断开，停止发送音频数据")
                    self.is_sending = False
                    break
                    
                if self.stream_in and self.stream_in.is_active():
                    data = self.stream_in.read(CHUNK)
                    # 再次检查连接状态，因为读取数据可能需要时间
                    if not self.ws.sock.connected:
                        print("WebSocket连接已断开，停止发送音频数据")
                        self.is_sending = False
                        break
                    self.ws.send(data, opcode=websocket.ABNF.OPCODE_BINARY)
                else:
                    # 如果音频流不活跃，短暂休眠以避免忙等待
                    time.sleep(0.01)
            except websocket.WebSocketConnectionClosedException:
                print("WebSocket连接已关闭，停止发送音频数据")
                self.is_sending = False
                break
            except OSError as e:
                if "WinError 10054" in str(e):
                    print("远程主机强迫关闭了一个现有的连接，停止发送音频数据")
                else:
                    print(f"网络错误: {e}")
                self.is_sending = False
                break
            except Exception as e:
                if self.is_sending:  # 只有在我们仍然试图发送时才打印错误
                    print(f"发送音频数据时出错: {e}")
                self.is_sending = False
                break

    def close(self):
        """关闭连接"""
        self.stop_all()
        if self.ws:
            self.ws.close()
        if self.audio:
            self.audio.terminate()

def print_help():
    print("\nWiFi音频测试客户端")
    print("=" * 30)
    print("命令:")
    print("  connect <IP>     - 连接到ESP32 (例如: connect 192.168.1.100)")
    print("  start_tx         - 开始音频发送 (麦克风→ESP32)")
    print("  stop_tx          - 停止音频发送")
    print("  start_rx         - 开始音频接收 (ESP32→扬声器)")
    print("  stop_rx          - 停止音频接收")
    print("  start_both       - 开始双向音频传输")
    print("  stop_both        - 停止所有音频传输")
    print("  quit             - 退出程序")
    print("  help             - 显示此帮助信息")
    print("=" * 30)

def main():
    client = WifiAudioClient("")
    print_help()
    
    while True:
        try:
            command = input("\n请输入命令: ").strip().split()
            if not command:
                continue
                
            cmd = command[0].lower()
            
            if cmd == "connect":
                if len(command) < 2:
                    print("请提供ESP32的IP地址")
                    continue
                    
                client = WifiAudioClient(command[1])
                if client.connect():
                    print("连接成功!")
                else:
                    print("连接失败!")
                    
            elif cmd == "start_tx":
                client.start_audio_tx()
                
            elif cmd == "stop_tx":
                client.stop_audio_tx()
                
            elif cmd == "start_rx":
                client.start_audio_rx()
                
            elif cmd == "stop_rx":
                client.stop_audio_rx()
                
            elif cmd == "start_both":
                client.start_bidirectional()
                
            elif cmd == "stop_both":
                client.stop_all()
                
            elif cmd == "quit":
                break
                
            elif cmd == "help":
                print_help()
                
            else:
                print("未知命令，请输入 'help' 查看可用命令")
                
        except KeyboardInterrupt:
            print("\n程序被用户中断")
            break
        except Exception as e:
            print(f"发生错误: {e}")
    
    client.close()
    print("程序已退出")

if __name__ == "__main__":
    main()