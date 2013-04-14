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

#define USE_CLIB
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/runtime_defines.h"
#include "ac/translation.h"
#include "ac/tree_map.h"
#include "ac/wordsdictionary.h"
#include "game/game_objects.h"
#include "util/misc.h"
#include "util/stream.h"
#include "core/assetmanager.h"

using AGS::Common::Stream;

extern GameSetup usetup;
extern GameState play;
extern char transFileName[MAX_PATH];


TreeMap *transtree = NULL;
long lang_offs_start = 0;
char transFileName[MAX_PATH] = "\0";

void close_translation () {
    if (transtree != NULL) {
        delete transtree;
        transtree = NULL;
    }
}

bool init_translation (const char *lang) {
    char *transFileLoc;

    if (lang == NULL) {
        sprintf(transFileName, "default.tra");
    }
    else {
        sprintf(transFileName, "%s.tra", lang);
    }

    transFileLoc = ci_find_file(usetup.data_files_dir, transFileName);

    Stream *language_file = Common::AssetManager::OpenAsset(transFileLoc);
    free(transFileLoc);

    if (language_file == NULL) 
    {
        if (lang != NULL)
        {
            // Just in case they're running in Debug, try compiled folder
            sprintf(transFileName, "Compiled\\%s.tra", lang);
            language_file = Common::AssetManager::OpenAsset(transFileName);
        }
        if (language_file == NULL)
            return false;
    }
    // in case it's inside a library file, record the offset
    lang_offs_start = language_file->GetPosition();

    char transsig[16];
    language_file->Read(transsig, 15);
    if (strcmp(transsig, "AGSTranslation") != 0) {
        delete language_file;
        return false;
    }

    if (transtree != NULL)
    {
        close_translation();
    }
    transtree = new TreeMap();

    while (!language_file->EOS()) {
        int blockType = language_file->ReadInt32();
        if (blockType == -1)
            break;
        // MACPORT FIX 9/6/5: remove warning
        /* int blockSize = */ language_file->ReadInt32();

        if (blockType == 1) {
            char original[STD_BUFFER_SIZE], translation[STD_BUFFER_SIZE];
            while (1) {
                read_string_decrypt (language_file, original);
                read_string_decrypt (language_file, translation);
                if ((strlen (original) < 1) && (strlen(translation) < 1))
                    break;
                if (language_file->EOS())
                    quit("!Language file is corrupt");
                transtree->addText (original, translation);
            }

        }
        else if (blockType == 2) {
            int uidfrom;
            char wasgamename[100];
            uidfrom = language_file->ReadInt32();
            read_string_decrypt (language_file, wasgamename);
            if ((uidfrom != game.UniqueId) || (strcmp (wasgamename, game.GameName) != 0)) {
                char quitmess[250];
                sprintf(quitmess,
                    "!The translation file you have selected is not compatible with this game. "
                    "The translation is designed for '%s'. Make sure the translation was compiled by the original game author.",
                    wasgamename);
                delete language_file;
                quit(quitmess);
            }
        }
        else if (blockType == 3) {
            // game settings
            int temp = language_file->ReadInt32();
            // normal font
            if (temp >= 0)
                SetNormalFont (temp);
            temp = language_file->ReadInt32();
            // speech font
            if (temp >= 0)
                SetSpeechFont (temp);
            temp = language_file->ReadInt32();
            // text direction
            if (temp == 1) {
                play.text_align = SCALIGN_LEFT;
                game.Options[OPT_RIGHTLEFTWRITE] = 0;
            }
            else if (temp == 2) {
                play.text_align = SCALIGN_RIGHT;
                game.Options[OPT_RIGHTLEFTWRITE] = 1;
            }
        }
        else
            quit("Unknown block type in translation file.");
    }

    delete language_file;

    if (transtree->text == NULL)
        quit("!The selected translation file was empty. The translation source may have been translated incorrectly or you may have generated a blank file.");

    return true;
}
