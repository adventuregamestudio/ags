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
#include <stdio.h>
#include "ac/scriptmodule.h"

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
    compiled.reset();
}