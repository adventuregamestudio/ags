#define USE_CLIB
#include "wgt2allg.h"
#include "ac/ac_common.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/rundefines.h"
#include "ac/translation.h"
#include "ac/tree_map.h"
#include "misc.h"

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

    FILE *language_file = clibfopen(transFileLoc, "rb");
    free(transFileLoc);

    if (language_file == NULL) 
    {
        if (lang != NULL)
        {
            // Just in case they're running in Debug, try compiled folder
            sprintf(transFileName, "Compiled\\%s.tra", lang);
            language_file = clibfopen(transFileName, "rb");
        }
        if (language_file == NULL)
            return false;
    }
    // in case it's inside a library file, record the offset
    lang_offs_start = ftell(language_file);

    char transsig[16];
    fread(transsig, 15, 1, language_file);
    if (strcmp(transsig, "AGSTranslation") != 0) {
        fclose(language_file);
        return false;
    }

    if (transtree != NULL)
    {
        close_translation();
    }
    transtree = new TreeMap();

    while (!feof (language_file)) {
        int blockType = getw(language_file);
        if (blockType == -1)
            break;
        // MACPORT FIX 9/6/5: remove warning
        /* int blockSize = */ getw(language_file);

        if (blockType == 1) {
            char original[STD_BUFFER_SIZE], translation[STD_BUFFER_SIZE];
            while (1) {
                read_string_decrypt (language_file, original);
                read_string_decrypt (language_file, translation);
                if ((strlen (original) < 1) && (strlen(translation) < 1))
                    break;
                if (feof (language_file))
                    quit("!Language file is corrupt");
                transtree->addText (original, translation);
            }

        }
        else if (blockType == 2) {
            int uidfrom;
            char wasgamename[100];
            fread (&uidfrom, 4, 1, language_file);
            read_string_decrypt (language_file, wasgamename);
            if ((uidfrom != game.uniqueid) || (strcmp (wasgamename, game.gamename) != 0)) {
                char quitmess[250];
                sprintf(quitmess,
                    "!The translation file you have selected is not compatible with this game. "
                    "The translation is designed for '%s'. Make sure the translation was compiled by the original game author.",
                    wasgamename);
                quit(quitmess);
            }
        }
        else if (blockType == 3) {
            // game settings
            int temp = getw(language_file);
            // normal font
            if (temp >= 0)
                SetNormalFont (temp);
            temp = getw(language_file);
            // speech font
            if (temp >= 0)
                SetSpeechFont (temp);
            temp = getw(language_file);
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
            quit("Unknown block type in translation file.");
    }

    fclose (language_file);

    if (transtree->text == NULL)
        quit("!The selected translation file was empty. The translation source may have been translated incorrectly or you may have generated a blank file.");

    return true;
}
