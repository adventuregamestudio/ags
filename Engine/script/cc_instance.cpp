
#include <stdio.h>
#include <string.h>
#include "cc_instance.h"

ccInstance *current_instance;

ccInstance *ccGetCurrentInstance()
{
    return current_instance;
}

void ccGetCallStack(ccInstance *inst, char *buffer, int maxLines) {

    if (inst == NULL) {
        // not in a script, no call stack
        buffer[0] = 0;
        return;
    }

    sprintf(buffer, "in \"%s\", line %d\n", ccGetSectionNameAtOffs(inst->runningInst->instanceof, inst->pc), inst->line_number);

    char lineBuffer[300];
    int linesDone = 0;
    for (int j = inst->callStackSize - 1; (j >= 0) && (linesDone < maxLines); j--, linesDone++) {
        sprintf(lineBuffer, "from \"%s\", line %d\n", ccGetSectionNameAtOffs(inst->callStackCodeInst[j]->instanceof, inst->callStackAddr[j]), inst->callStackLineNumber[j]);
        strcat(buffer, lineBuffer);
        if (linesDone == maxLines - 1)
            strcat(buffer, "(and more...)\n");
    }

}
