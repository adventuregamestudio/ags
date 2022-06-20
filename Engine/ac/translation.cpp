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
#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/runtime_defines.h"
#include "ac/translation.h"
#include "ac/wordsdictionary.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/tra_file.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern GameState play;

String trans_name;
String trans_filename;
Translation trans;


void close_translation () {
    trans = Translation();
    trans_name = "";
    trans_filename = "";

    // Return back to default game's encoding
    if (game.options[OPT_GAMETEXTENCODING] == 65001) // utf-8 codepage number
        set_uformat(U_UTF8);
    else
        set_uformat(U_ASCII);
}

bool init_translation(const String &lang, const String &fallback_lang)
{
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
        close_translation();
        Debug::Printf(kDbgMsg_Error, "Failed to read translation file: %s:\n%s",
            trans_filename.GetCStr(),
            err->FullMessage().GetCStr());
        if (!fallback_lang.IsEmpty())
        {
            Debug::Printf("Fallback to translation: %s", fallback_lang.GetCStr());
            init_translation(fallback_lang, "");
        }
        return false;
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
    // dictionary keys won't match. With that assumption we must convert
    // dictionary keys into ASCII using provided locale hint.
    int game_codepage = game.options[OPT_GAMETEXTENCODING];
    if ((get_uformat() == U_UTF8) && (game_codepage != 65001))
    {
        String key_enc = (game_codepage > 0) ?
            String::FromFormat(".%d", game_codepage) :
            trans.StrOptions["gameencoding"];
        if (!key_enc.IsEmpty())
        {
            StringMap conv_map;
            std::vector<char> ascii; // ascii buffer
            for (const auto &item : trans.Dict)
            {
                ascii.resize(item.first.GetLength() + 1); // ascii len will be <= utf-8 len
                StrUtil::ConvertUtf8ToAscii(item.first.GetCStr(), key_enc.GetCStr(), &ascii[0], ascii.size());
                conv_map.insert(std::make_pair(&ascii[0], item.second));
            }
            trans.Dict = conv_map;
        }
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
