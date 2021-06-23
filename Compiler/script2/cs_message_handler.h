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
        kSV_Error,
    };

    struct Entry
    {
        Severity Severity = kSV_Error;
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
    bool HasError() const { return !_entries.empty() && kSV_Error == _entries.back().Severity; }
    Entry const &GetError() const { return HasError() ? _entries.back() : _noError; }
};

} // namespace AGS

#endif
