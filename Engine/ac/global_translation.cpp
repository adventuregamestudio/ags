//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include <string.h>
#include "ac/common.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/tree_map.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"

extern GameState play;
extern AGSPlatformDriver *platform;
extern int source_text_length;
extern TreeMap *transtree;
extern char transFileName[MAX_PATH];

char *get_translation (const char *text) {
    if (text == NULL)
        quit("!Null string supplied to CheckForTranslations");

    source_text_length = strlen(text);
    if ((text[0] == '&') && (play.unfactor_speech_from_textlength != 0)) {
        // if there's an "&12 text" type line, remove "&12 " from the source
        // length
        int j = 0;
        while ((text[j] != ' ') && (text[j] != 0))
            j++;
        j++;
        source_text_length -= j;
    }

    // check if a plugin wants to translate it - if so, return that
    char *plResult = (char*)platform->RunPluginHooks(AGSE_TRANSLATETEXT, (long)text);
    if (plResult) {

        //  64bit: This is a wonky way to detect a valid pointer
        //  if (((int)plResult >= -1) && ((int)plResult < 10000))
        //    quit("!Plugin did not return a string for text translation");

        return plResult;
    }

    if (transtree != NULL) {
        // translate the text using the translation file
        char * transl = transtree->findValue (text);
        if (transl != NULL)
            return transl;
    }
    // return the original text
    return (char*)text;
}

int IsTranslationAvailable () {
    if (transtree != NULL)
        return 1;
    return 0;
}

int GetTranslationName (char* buffer) {
    VALIDATE_STRING (buffer);
    const char *copyFrom = transFileName;

    while (strchr(copyFrom, '\\') != NULL)
    {
        copyFrom = strchr(copyFrom, '\\') + 1;
    }
    while (strchr(copyFrom, '/') != NULL)
    {
        copyFrom = strchr(copyFrom, '/') + 1;
    }

    strcpy (buffer, copyFrom);
    // remove the ".tra" from the end of the filename
    if (strstr (buffer, ".tra") != NULL)
        strstr (buffer, ".tra")[0] = 0;

    return IsTranslationAvailable();
}
