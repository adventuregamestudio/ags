//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __CS_MESSAGE_HANDLER_H
#define __CS_MESSAGE_HANDLER_H

#include <string>
#include <vector>

namespace AGS
{

class MessageHandler
{
public:
    enum Severity
    {
        kSV_None,
        kSV_Info,
        kSV_Warning,
        kSV_UserError,
        kSV_InternalError,
    };

    struct Entry
    {
        MessageHandler::Severity Severity = kSV_UserError;
        std::string Section = "";
        size_t Lineno = 0u;
        std::string Message = "";

        Entry() = default;
        Entry(enum Severity sev, std::string const &section, size_t lineno, std::string const &msg);
    };

    typedef std::vector<Entry> MessagesType;

private:
    MessagesType _entries;
    static Entry _noError;

public:
    inline void AddMessage(Severity sev, std::string const &sec, size_t line, std::string const &msg)
        { _entries.emplace_back(sev, sec, line, msg); }
    inline MessagesType GetMessages() const { return _entries; }
    inline void Clear() { _entries.clear(); }
    bool HasError() const;
    size_t WarningsCount() const;
    Entry const &GetError() const { return HasError() ? _entries.back() : _noError; }
};

} // namespace AGS

#endif
