// AGS - Simple stderr logger for the build pipeline.
#pragma once

#include <cstdio>
#include <cstdarg>

namespace AGSBuild
{

class Logger
{
public:
    static void Log(const char* fmt, ...)
    {
        char buf[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        fprintf(stderr, "%s\n", buf);
    }

    static void Info(const char* fmt, ...)
    {
        char msg[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);
        Log("[Info] %s", msg);
    }

    static void Warn(const char* fmt, ...)
    {
        char msg[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);
        Log("[Warn] %s", msg);
    }

    static void Error(const char* fmt, ...)
    {
        char msg[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);
        Log("[Error] %s", msg);
    }
};

} // namespace AGSBuild
