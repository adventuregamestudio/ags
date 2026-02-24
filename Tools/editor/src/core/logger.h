// AGS Editor ImGui - Unified Logger
// Routes log messages to both stderr/stdout console and the in-editor log pane.
// Usage: AGSEditor::Logger::Log("[Info] Something happened: %s", value);
//        AGSEditor::Logger::Error("Failed to load: %s", path);
//        AGSEditor::Logger::Warn("Deprecated setting used");
//        AGSEditor::Logger::Info("Project loaded successfully");
#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace AGSEditor
{

// Callback type for forwarding log messages to the UI LogPanel
using LogCallback = std::function<void(const char*)>;

class Logger
{
public:
    // Set the callback that forwards messages to the LogPanel.
    // Called once during EditorUI initialization.
    static void SetLogCallback(LogCallback cb)
    {
        std::lock_guard<std::mutex> lock(GetMutex());
        GetCallback() = std::move(cb);
    }

    // General log: writes formatted message to both stderr and LogPanel.
    // The message should include a tag like [Info], [Warn], [Error], [Compiler], etc.
    static void Log(const char* fmt, ...)
    {
        char buf[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        // Write to stderr
        fprintf(stderr, "%s\n", buf);

        // Forward to LogPanel if available
        std::lock_guard<std::mutex> lock(GetMutex());
        auto& cb = GetCallback();
        if (cb)
            cb(buf);
    }

    // Convenience: [Info] prefix
    static void Info(const char* fmt, ...)
    {
        char msg[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        char buf[2100];
        snprintf(buf, sizeof(buf), "[Info] %s", msg);
        Log("%s", buf);
    }

    // Convenience: [Warn] prefix
    static void Warn(const char* fmt, ...)
    {
        char msg[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        char buf[2100];
        snprintf(buf, sizeof(buf), "[Warn] %s", msg);
        Log("%s", buf);
    }

    // Convenience: [Error] prefix
    static void Error(const char* fmt, ...)
    {
        char msg[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        char buf[2100];
        snprintf(buf, sizeof(buf), "[Error] %s", msg);
        Log("%s", buf);
    }

private:
    static std::mutex& GetMutex()
    {
        static std::mutex mtx;
        return mtx;
    }

    static LogCallback& GetCallback()
    {
        static LogCallback cb;
        return cb;
    }
};

} // namespace AGSEditor
