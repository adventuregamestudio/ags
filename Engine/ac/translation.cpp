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
#include <cstdio>
#include <locale.h>
#include <wchar.h>
#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/runtime_defines.h"
#include "ac/translation.h"
#include "ac/wordsdictionary.h"
#include "debug/out.h"
#include "game/tra_file.h"
#include "util/misc.h"
#include "util/stream.h"
#include "core/assetmanager.h"

using namespace AGS::Common;

extern GameSetup usetup;
extern GameSetupStruct game;
extern GameState play;

String trans_name;
String trans_filename;
Translation trans;

// TODO: this is a test, later consider using C++ conversion methods
// that do not change global locale state (but they are more confusing)
std::vector<wchar_t> wcsbuf; // widechar buffer
std::vector<char> mbbuf;  // utf8 buffer
const char *convert_utf8_to_ascii(const char *mbstr, const char *loc_name)
{
    // First convert utf-8 string into widestring;
    size_t len = strlen(mbstr);
    wcsbuf.resize(len + 1); // the actual length in glyths will be <= len
    // use uconvert, because libc funcs are locale dependent and complicate things
    uconvert(mbstr, U_UTF8, (char*)&wcsbuf[0], U_UNICODE, wcsbuf.size() * sizeof(wchar_t));
    // Then convert widestring to single-byte string using specified locale
    mbbuf.resize(wcsbuf.size() * 4);
    setlocale(LC_CTYPE, loc_name);
    wcstombs(&mbbuf[0], &wcsbuf[0], mbbuf.size());
    setlocale(LC_CTYPE, "");
    return &mbbuf[0];
}


void close_translation () {
    trans = Translation();
    trans_name = "";
    trans_filename = "";

    // Return back to default game's encoding
    set_uformat(U_ASCII);
}

bool init_translation (const String &lang, const String &fallback_lang, bool quit_on_error) {

    if (lang.IsEmpty())
        return false;
    trans_name = lang;
    trans_filename = String::FromFormat("%s.tra", lang.GetCStr());

    std::unique_ptr<Stream> in(AssetMgr->OpenAsset(trans_filename));
    if (in == nullptr)
    {
        Debug::Printf(kDbgMsg_Error, "Cannot open translation: %s", trans_filename.GetCStr());
        return false;
    }

    trans = Translation();

    // First test if the translation is meant for this game
    HError err = TestTraGameID(game.uniqueid, game.gamename, in.get());
    if (err)
    {
        // If successful, then read translation data fully
        in.reset(AssetMgr->OpenAsset(trans_filename));
        err = ReadTraData(trans, in.get());
    }

    // Process errors
    if (!err)
    {
        String err_msg = String::FromFormat("Failed to read translation file: %s:\n%s",
            trans_filename.GetCStr(),
            err->FullMessage().GetCStr());
        close_translation();
        if (quit_on_error)
        {
            quitprintf("!%s", err_msg.GetCStr());
        }
        else
        {
            Debug::Printf(kDbgMsg_Error, err_msg);
            if (!fallback_lang.IsEmpty())
            {
                Debug::Printf("Fallback to translation: %s", fallback_lang.GetCStr());
                init_translation(fallback_lang, "", false);
            }
            return false;
        }
    }

    // Translation read successfully
    // Configure new game settings
    if (trans.NormalFont >= 0)
        SetNormalFont(trans.NormalFont);
    if (trans.SpeechFont >= 0)
        SetSpeechFont(trans.SpeechFont);
    if (trans.RightToLeft == 1)
    {
        play.text_align = kHAlignLeft;
        game.options[OPT_RIGHTLEFTWRITE] = 0;
    }
    else if (trans.RightToLeft == 2)
    {
        play.text_align = kHAlignRight;
        game.options[OPT_RIGHTLEFTWRITE] = 1;
    }
    // Setup a text encoding mode depending on the translation data hint
    String encoding = trans.StrOptions["encoding"];
    if (encoding.CompareNoCase("utf-8") == 0)
        set_uformat(U_UTF8);
    else
        set_uformat(U_ASCII);

    // Mixed encoding support: 
    // original text unfortunately may contain extended ASCII chars (> 127);
    // if translation is UTF-8 but game is extended ASCII, then the translation
    // dictionary keys won't match.
    // With that assumption we must convert dictionary keys into ASCII using
    // provided locale hint.
    String key_enc = trans.StrOptions["gameencoding"];
    if (!key_enc.IsEmpty())
    {
        StringMap conv_map;
        for (const auto &item : trans.Dict)
        {
            String key = convert_utf8_to_ascii(item.first.GetCStr(), key_enc.GetCStr());
            conv_map.insert(std::make_pair(key, item.second));
        }
        trans.Dict = conv_map;
    }

    Debug::Printf("Translation initialized: %s", trans_filename.GetCStr());
    return true;
}

String get_translation_name()
{
    return trans_name;
}

String get_translation_path()
{
    return trans_filename;
}

const StringMap& get_translation_tree()
{
    return trans.Dict;
}
