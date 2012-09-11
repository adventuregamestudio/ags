#define USE_CLIB
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/runtime_defines.h"
#include "ac/translation.h"
#include "ac/tree_map.h"
#include "util/misc.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

extern GameSetup usetup;
extern GameSetupStruct game;
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

    CDataStream *language_file = clibfopen(transFileLoc);
    free(transFileLoc);

    if (language_file == NULL) 
    {
        if (lang != NULL)
        {
            // Just in case they're running in Debug, try compiled folder
            sprintf(transFileName, "Compiled\\%s.tra", lang);
            language_file = clibfopen(transFileName);
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
            if ((uidfrom != game.uniqueid) || (strcmp (wasgamename, game.gamename) != 0)) {
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
                game.options[OPT_RIGHTLEFTWRITE] = 0;
            }
            else if (temp == 2) {
                play.text_align = SCALIGN_RIGHT;
                game.options[OPT_RIGHTLEFTWRITE] = 1;
            }
        }
        else
            quit("Unknown IBitmap *type in translation file.");
    }

    delete language_file;

    if (transtree->text == NULL)
        quit("!The selected translation file was empty. The translation source may have been translated incorrectly or you may have generated a blank file.");

    return true;
}
