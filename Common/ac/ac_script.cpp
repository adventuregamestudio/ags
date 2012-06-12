
#include <stdio.h>
#include "ac_script.h"

int in_interaction_editor = 0;

void ScriptModule::init() { 
    name = NULL;
    author = NULL;
    version = NULL;
    description = NULL;
    script = NULL;
    scriptHeader = NULL;
    uniqueKey = 0;
    permissions = 0;
    weAreOwner = 1;
    compiled = NULL;
}