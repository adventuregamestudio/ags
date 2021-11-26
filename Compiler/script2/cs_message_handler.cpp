
#include "cs_message_handler.h"

AGS::MessageHandler::Entry::Entry(enum Severity sev, std::string const &section, size_t lineno, std::string const &msg)
    : Severity(sev)
    , Section(section)
    , Lineno(lineno)
    , Message(msg)
{
}

AGS::MessageHandler::Entry AGS::MessageHandler::_noError = { AGS::MessageHandler::kSV_UserError, "", 0u, "((no error))" };

bool AGS::MessageHandler::HasError() const
{
    if (_entries.empty())
        return false;
    Severity const sev = _entries.back().Severity;
    return kSV_UserError == sev || kSV_InternalError == sev;
}
