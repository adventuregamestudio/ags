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
#include "ac/common.h"
#include "ac/display.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/translation.h"
#include "core/types.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin_evts.h"
#include "plugin/plugin_engine.h"
#include "util/memory.h"

using namespace AGS::Common::Memory;

extern GameState play;
extern AGSPlatformDriver *platform;

const char *get_translation (const char *text) {
    if (text == nullptr)
        quit("!Null string supplied to CheckForTranslations");

    source_text_length = GetTextDisplayLength(text);

#if AGS_PLATFORM_64BIT
    // check if a plugin wants to translate it - if so, return that
    // TODO: plugin API is currently strictly 32-bit, so this may break on 64-bit systems
    char *plResult = Int32ToPtr<char>(pl_run_plugin_hooks(AGSE_TRANSLATETEXT, PtrToInt32(text)));
    if (plResult) {
        return plResult;
    }
#endif

    const auto &transtree = get_translation_tree();
    const auto it = transtree.find(String::Wrapper(text));
    if (it != transtree.end())
        return it->second.GetCStr();
    // return the original text
    return text;
}

int IsTranslationAvailable () {
    if (get_translation_tree().size() > 0)
        return 1;
    return 0;
}

int GetTranslationName (char* buffer) {
    VALIDATE_STRING (buffer);
    snprintf(buffer, MAX_MAXSTRLEN, "%s", get_translation_name().GetCStr());
    return IsTranslationAvailable();
}
