//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "diagnostics.h"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace AGSBuild
{
namespace Diag
{

static bool IsLetterOrSpace(char c)
{
    return c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool StartsWithNoCase(const std::string &s, size_t pos, const char *prefix)
{
    const size_t len = strlen(prefix);
    if (s.size() < pos + len)
        return false;
    for (size_t i = 0; i < len; ++i)
    {
        char c = s[pos + i];
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
        if (c != prefix[i])
            return false;
    }
    return true;
}

bool ParseDiagnosticLine(const std::string &line, Diagnostic &diag)
{
    const size_t pos = line.find_first_not_of(" \t");
    if (pos == std::string::npos)
        return false;
    Severity level;
    size_t prefix_len;
    if (StartsWithNoCase(line, pos, "error:"))
    {
        level = Severity::Error;
        prefix_len = 6;
    }
    else if (StartsWithNoCase(line, pos, "warning:"))
    {
        level = Severity::Warning;
        prefix_len = 8;
    }
    else if (StartsWithNoCase(line, pos, "info:"))
    {
        level = Severity::Info;
        prefix_len = 5;
    }
    else
    {
        return false;
    }

    const size_t start = line.find_first_not_of(" \t", pos + prefix_len);
    const size_t end = line.find_last_not_of(" \t\r\n");
    const std::string rest =
        (start == std::string::npos || end < start) ? std::string() : line.substr(start, end - start + 1);

    diag = Diagnostic();
    diag.Level = level;
    diag.Message = rest;

    // "<what> at <file>, line <N> : <message>", where <what> is a few plain words, or nothing
    size_t at_pos = std::string::npos;
    size_t file_pos = 0;
    if (rest.compare(0, 3, "at ") == 0)
    {
        at_pos = 0;
        file_pos = 3;
    }
    else
    {
        const size_t found = rest.find(" at ");
        if (found != std::string::npos)
        {
            at_pos = found;
            file_pos = found + 4;
        }
    }
    if (at_pos != std::string::npos && std::all_of(rest.begin(), rest.begin() + at_pos, IsLetterOrSpace))
    {
        const size_t line_pos = rest.find(", line ", file_pos);
        if (line_pos != std::string::npos)
        {
            const size_t digits = line_pos + 7;
            size_t digits_end = digits;
            while (digits_end < rest.size() && rest[digits_end] >= '0' && rest[digits_end] <= '9')
                digits_end++;
            if (digits_end > digits && digits_end - digits < 9 && rest.compare(digits_end, 3, " : ") == 0)
            {
                diag.File = rest.substr(file_pos, line_pos - file_pos);
                diag.Line = std::stoi(rest.substr(digits, digits_end - digits));
                diag.Message = rest.substr(digits_end + 3);
            }
        }
    }
    return true;
}

std::vector<Diagnostic> ExtractDiagnostics(const std::string &text)
{
    std::vector<Diagnostic> list;
    size_t pos = 0;
    while (pos < text.size())
    {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos)
            eol = text.size();
        Diagnostic diag;
        if (ParseDiagnosticLine(text.substr(pos, eol - pos), diag))
            list.push_back(std::move(diag));
        pos = eol + 1;
    }
    return list;
}

// Length of the valid UTF-8 sequence at s[i], or 0
static size_t Utf8SequenceLength(const std::string &s, size_t i)
{
    const unsigned char c = s[i];
    size_t len;
    if (c < 0x80)
        return 1;
    else if (c >= 0xC2 && c <= 0xDF)
        len = 2;
    else if (c >= 0xE0 && c <= 0xEF)
        len = 3;
    else if (c >= 0xF0 && c <= 0xF4)
        len = 4;
    else
        return 0;
    if (i + len > s.size())
        return 0;
    for (size_t k = 1; k < len; ++k)
    {
        if ((static_cast<unsigned char>(s[i + k]) & 0xC0) != 0x80)
            return 0;
    }
    return len;
}

std::string JsonString(const std::string &s)
{
    std::string out = "\"";
    for (size_t i = 0; i < s.size();)
    {
        const unsigned char c = s[i];
        if (c >= 0x80)
        {
            const size_t len = Utf8SequenceLength(s, i);
            if (len == 0)
            {
                out += '?';
                i++;
            }
            else
            {
                out.append(s, i, len);
                i += len;
            }
            continue;
        }

        if (c == '"' || c == '\\')
        {
            out += '\\';
            out += static_cast<char>(c);
        }
        else if (c == '\n')
        {
            out += "\\n";
        }
        else if (c == '\r')
        {
            out += "\\r";
        }
        else if (c == '\t')
        {
            out += "\\t";
        }
        else if (c < 0x20)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
        }
        else
        {
            out += static_cast<char>(c);
        }
        i++;
    }
    out += '"';
    return out;
}

std::string ToJson(const Diagnostic &diag)
{
    const char *severity =
        diag.Level == Severity::Error ? "error" : (diag.Level == Severity::Warning ? "warning" : "info");
    std::string json = std::string("{\"event\":\"diagnostic\",\"severity\":\"") + severity +
                       "\",\"tool\":" + JsonString(diag.Tool) + ",\"task\":" + JsonString(diag.Task);
    if (!diag.File.empty())
        json += ",\"file\":" + JsonString(diag.File);
    if (diag.Line > 0)
        json += ",\"line\":" + std::to_string(diag.Line);
    json += ",\"message\":" + JsonString(diag.Message) + "}";
    return json;
}

static bool JsonOutput = false;
static std::mutex OutputMutex;
static std::atomic<int> Errors{0};
static std::atomic<int> Warnings{0};
static std::atomic<bool> CancelFlag{false};

void SetJsonOutput(bool on)
{
    JsonOutput = on;
}

bool IsJsonOutput()
{
    return JsonOutput;
}

void Log(const char *fmt, ...)
{
    std::lock_guard<std::mutex> lk(OutputMutex);
    va_list args;
    va_start(args, fmt);
    vfprintf(JsonOutput ? stderr : stdout, fmt, args);
    va_end(args);
}

void EmitEvent(const std::string &json_line)
{
    if (!JsonOutput)
        return;
    std::lock_guard<std::mutex> lk(OutputMutex);
    fprintf(stdout, "%s\n", json_line.c_str());
    fflush(stdout);
}

void EmitStart(size_t tasks)
{
    EmitEvent("{\"event\":\"start\",\"tasks\":" + std::to_string(tasks) + "}");
}

void EmitTask(const std::string &task, const char *state)
{
    EmitEvent("{\"event\":\"task\",\"task\":" + JsonString(task) + ",\"state\":\"" + state + "\"}");
}

void EmitProgress(size_t done, size_t total)
{
    EmitEvent("{\"event\":\"progress\",\"done\":" + std::to_string(done) + ",\"total\":" + std::to_string(total) + "}");
}

void EmitDiagnostic(const Diagnostic &diag)
{
    if (diag.Level == Severity::Error)
        Errors++;
    else if (diag.Level == Severity::Warning)
        Warnings++;
    EmitEvent(ToJson(diag));
}

void EmitEnd(const char *result)
{
    EmitEvent(std::string("{\"event\":\"end\",\"result\":\"") + result + "\",\"errors\":" +
              std::to_string(ErrorCount()) + ",\"warnings\":" + std::to_string(WarningCount()) + "}");
}

int ErrorCount()
{
    return Errors;
}

int WarningCount()
{
    return Warnings;
}

void RequestCancel()
{
    CancelFlag = true;
}

bool IsCancelRequested()
{
    return CancelFlag;
}

void ResetCancel()
{
    CancelFlag = false;
}

static void OnSignal(int)
{
    CancelFlag = true; // a lock-free atomic is safe to set here
}

void InstallCancelHandlers()
{
    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);
#ifdef SIGBREAK
    std::signal(SIGBREAK, OnSignal);
#endif
    if (JsonOutput)
    {
        // The editor has no signal to send to a hidden console process: it writes "cancel" instead.
        // The thread may sit in the read until the process exits.
        std::thread(
            []()
            {
                std::string line;
                while (std::getline(std::cin, line))
                {
                    if (line == "cancel" || line == "cancel\r")
                    {
                        RequestCancel();
                        break;
                    }
                }
            })
            .detach();
    }
}

} // namespace Diag
} // namespace AGSBuild
