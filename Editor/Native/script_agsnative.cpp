//
// Implementation for script specific to AGS.Native library
//

#include <stdio.h>
#include "script/cc_error.h"

extern int currentline; // in script/script_common

void cc_error_at_line(char *buffer, const char *error_msg)
{
    sprintf(ccErrorString, "Error (line %d): %s", currentline, error_msg);
}
