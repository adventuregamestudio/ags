//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "util/memory.h"

using namespace AGS::Common::Memory;

extern AGSPlatformDriver *platform;

const char *get_translation (const char *text) {
    if (text == nullptr)
        quit("!Null string supplied to CheckForTranslations");

    source_text_length = GetTextDisplayLength(text);
    if (text[0] == 0)
        return ""; // don't try translating an empty line

    // check if a plugin wants to translate it - if so, return that
    const char *pl_result = reinterpret_cast<const char*>(
        pl_run_plugin_hooks(kPluginEvt_TranslateText, reinterpret_cast<intptr_t>(text)));
    if (pl_result)
        return pl_result;

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
