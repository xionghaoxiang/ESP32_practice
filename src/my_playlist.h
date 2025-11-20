#ifndef MY_PLAYLIST_H
#define MY_PLAYLIST_H

#include <Arduino.h>
#include <vector>

#define MAX_URLS 50
#define MAX_URL_LEN 256

class PlaylistManager
{
private:
    std::vector<String> urls;
    int currentIndex = 0;

public:
    PlaylistManager() {}

    // 添加单个 URL
    bool addURL(const String &url)
    {
        if (urls.size() >= MAX_URLS)
        {
            Serial.println("Playlist is full!");
            return false;
        }
        if (url.length() > MAX_URL_LEN)
        {
            Serial.println("URL too long!");
            return false;
        }
        urls.push_back(url);
        Serial.printf("Added URL: %s (total: %d)\n", url.c_str(), urls.size());
        return true;
    }

    // 从 JSON 字符串解析并设置播放列表
    bool setFromJSON(const String &jsonStr)
    {
        urls.clear();
        currentIndex = 0;

        // 简单 JSON 解析：["url1","url2","url3"]
        int start = jsonStr.indexOf('[');
        int end = jsonStr.lastIndexOf(']');
        if (start < 0 || end < 0)
            return false;

        String content = jsonStr.substring(start + 1, end);
        int pos = 0;
        while (pos < content.length())
        {
            int q1 = content.indexOf('"', pos);
            if (q1 < 0)
                break;
            int q2 = content.indexOf('"', q1 + 1);
            if (q2 < 0)
                break;
            String url = content.substring(q1 + 1, q2);
            addURL(url);
            pos = q2 + 1;
        }
        return urls.size() > 0;
    }

    // 获取指定索引的 URL
    String getURL(int index)
    {
        if (index < 0 || index >= urls.size())
            return "";
        currentIndex = index;
        return urls[index];
    }

    // 获取当前 URL
    String getCurrentURL()
    {
        if (currentIndex < 0 || currentIndex >= urls.size())
            return "";
        return urls[currentIndex];
    }

    // 获取下一个 URL
    String getNextURL()
    {
        currentIndex = (currentIndex + 1) % urls.size();
        return urls[currentIndex];
    }

    // 获取上一个 URL
    String getPreviousURL()
    {
        currentIndex = (currentIndex - 1 + urls.size()) % urls.size();
        return urls[currentIndex];
    }

    // 获取播放列表大小
    int getSize()
    {
        return urls.size();
    }

    // 清空播放列表
    void clear()
    {
        urls.clear();
        currentIndex = 0;
        Serial.println("Playlist cleared");
    }

    // 打印所有 URL
    void printAll()
    {
        Serial.println("=== Current Playlist ===");
        for (int i = 0; i < urls.size(); i++)
        {
            Serial.printf("%d: %s\n", i, urls[i].c_str());
        }
        Serial.println("=======================");
    }
};

#endif