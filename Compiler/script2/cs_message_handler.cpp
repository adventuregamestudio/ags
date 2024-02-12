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
