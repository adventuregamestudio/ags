
#include "cs_parser_common.h"

AGS::MessageHandler::Entry::Entry(enum Severity sev, std::string const & section, size_t lineno, std::string const msg)
    : Severity(sev)
    , Section(section)
    , Lineno(lineno)
    , Message(msg)
{
}

AGS::MessageHandler::Entry AGS::MessageHandler::_noError = { AGS::MessageHandler::kSV_Error, "", 0u, "((no error))" };

AGS::MessageHandler::Entry const &AGS::MessageHandler::GetError(void) const
{
    if (_entries.empty() || kSV_Error != _entries.back().Severity)
        return _noError;
    return _entries.back();
}
