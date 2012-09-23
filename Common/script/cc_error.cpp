
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cc_error.h"
#include "script/script_common.h"  // current_line

extern void cc_error_at_line(char *buffer, const char *error_msg);

int ccError = 0;
int ccErrorLine = 0;
char ccErrorString[400];
char ccErrorCallStack[400];
bool ccErrorIsUserError = false;
const char *ccCurScriptName = "";

void cc_error(char *descr, ...)
{
    ccErrorCallStack[0] = 0;
    ccErrorIsUserError = false;
    if (descr[0] == '!')
    {
        ccErrorIsUserError = true;
        descr++;
    }

    char displbuf[1000];
    va_list ap;

    va_start(ap, descr);
    vsprintf(displbuf, descr, ap);
    va_end(ap);

    if (currentline > 0) {
        // [IKM] Implementation is project-specific
        cc_error_at_line(ccErrorString, displbuf);
    }
    else
        sprintf(ccErrorString, "Runtime error: %s", displbuf);

    ccError = 1;
    ccErrorLine = currentline;
}
