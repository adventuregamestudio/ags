
#include <stdio.h>
#include <string.h>
#include "cc_error.h"

int ccError = 0;
int ccErrorLine = 0;
char ccErrorString[400];
char ccErrorCallStack[400];
bool ccErrorIsUserError = false;
const char *ccCurScriptName = "";

const char* ccGetSectionNameAtOffs(ccScript *scri, long offs) {

    int i;
    for (i = 0; i < scri->numSections; i++) {
        if (scri->sectionOffsets[i] < offs)
            continue;
        break;
    }

    // if no sections in script, return unknown
    if (i == 0)
        return "(unknown section)";

    return scri->sectionNames[i - 1];
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
