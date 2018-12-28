//
// Implementation for script specific to AGS.Native library
//

#include <utility>
#include "util/string.h"

using namespace AGS::Common;

extern int currentline; // in script/script_common

std::pair<String, String> cc_error_at_line(const char *error_msg)
{
    return std::make_pair(String::FromFormat("Error (line %d): %s", currentline, error_msg), String());
}

String cc_error_without_line(const char *error_msg)
{
    return String::FromFormat("Error (line unknown): %s", error_msg);
}
