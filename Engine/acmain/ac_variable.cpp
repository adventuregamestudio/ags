
#include "acmain/ac_maindefines.h"
#include "acmain/ac_variable.h"

InteractionVariable *get_interaction_variable (int varindx) {

    if ((varindx >= LOCAL_VARIABLE_OFFSET) && (varindx < LOCAL_VARIABLE_OFFSET + thisroom.numLocalVars))
        return &thisroom.localvars[varindx - LOCAL_VARIABLE_OFFSET];

    if ((varindx < 0) || (varindx >= numGlobalVars))
        quit("!invalid interaction variable specified");

    return &globalvars[varindx];
}

InteractionVariable *FindGraphicalVariable(const char *varName) {
    int ii;
    for (ii = 0; ii < numGlobalVars; ii++) {
        if (stricmp (globalvars[ii].name, varName) == 0)
            return &globalvars[ii];
    }
    for (ii = 0; ii < thisroom.numLocalVars; ii++) {
        if (stricmp (thisroom.localvars[ii].name, varName) == 0)
            return &thisroom.localvars[ii];
    }
    return NULL;
}

int GetGraphicalVariable (const char *varName) {
    InteractionVariable *theVar = FindGraphicalVariable(varName);
    if (theVar == NULL) {
        char quitmessage[120];
        sprintf (quitmessage, "!GetGraphicalVariable: interaction variable '%s' not found", varName);
        quit(quitmessage);
        return 0;
    }
    return theVar->value;
}

void SetGraphicalVariable (const char *varName, int p_value) {
    InteractionVariable *theVar = FindGraphicalVariable(varName);
    if (theVar == NULL) {
        char quitmessage[120];
        sprintf (quitmessage, "!SetGraphicalVariable: interaction variable '%s' not found", varName);
        quit(quitmessage);
    }
    else
        theVar->value = p_value;
}
