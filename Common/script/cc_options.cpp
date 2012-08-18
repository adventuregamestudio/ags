
#include "cc_options.h"

int ccCompOptions = SCOPT_LEFTTORIGHT;

void ccSetOption(int optbit, int onoroff)
{
    if (onoroff)
        ccCompOptions |= optbit;
    else
        ccCompOptions &= ~optbit;
}

int ccGetOption(int optbit)
{
    if (ccCompOptions & optbit)
        return 1;

    return 0;
}
