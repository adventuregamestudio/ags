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

#include "ac/characterinfo.h"
#include "ac/gamesetupstructbase.h"
#include "ac/game_version.h"
#include "ac/wordsdictionary.h"
#include "script/cc_script.h"
#include "util/stream.h"

using AGS::Common::Stream;

GameSetupStructBase::GameSetupStructBase()
    : numviews(0)
    , numcharacters(0)
    , playercharacter(-1)
    , numinvitems(0)
    , numdialog(0)
    , numdlgmessage(0)
    , numfonts(0)
    , color_depth(0)
    , target_win(0)
    , dialog_bullet(0)
    , hotdot(0)
    , hotdotouter(0)
    , uniqueid(0)
    , numgui(0)
    , numcursors(0)
    , default_lipsync_frame(0)
    , invhotdotsprite(0)
{
    memset(gamename, 0, sizeof(gamename));
    memset(options, 0, sizeof(options));
    memset(paluses, 0, sizeof(paluses));
    memset(defpal, 0, sizeof(defpal));
    memset(reserved, 0, sizeof(reserved));
}

GameSetupStructBase::~GameSetupStructBase()
{
    Free();
}

void GameSetupStructBase::Free()
{
    dict.reset();
    chars.clear();

    numcharacters = 0;
}

void GameSetupStructBase::SetGameResolution(Size game_res)
{
    _gameResolution = game_res;
    OnResolutionSet();
}

void GameSetupStructBase::OnResolutionSet()
{
    _relativeUIMult = 1; // NOTE: this is remains of old logic, currently unused.
}

void GameSetupStructBase::ReadFromFile(Stream *in, GameDataVersion game_ver, SerializeInfo &info)
{
    // NOTE: historically the struct was saved by dumping whole memory
    // into the file stream, which added padding from memory alignment;
    // here we mark the padding bytes, as they do not belong to actual data.
    in->Read(&gamename[0], GAME_NAME_LENGTH);
    in->ReadInt16(); // alignment padding to int32 (gamename: 50 -> 52 bytes)
    in->ReadArrayOfInt32(options, MAX_OPTIONS);
    in->Read(&paluses[0], sizeof(paluses));
    // colors are an array of chars
    in->Read(&defpal[0], sizeof(defpal));
    numviews = in->ReadInt32();
    numcharacters = in->ReadInt32();
    playercharacter = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    numinvitems = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    numdialog = in->ReadInt32();
    numdlgmessage = in->ReadInt32();
    numfonts = in->ReadInt32();
    color_depth = in->ReadInt32();
    target_win = in->ReadInt32();
    dialog_bullet = in->ReadInt32();
    hotdot = in->ReadInt16();
    hotdotouter = in->ReadInt16();
    uniqueid = in->ReadInt32();
    numgui = in->ReadInt32();
    numcursors = in->ReadInt32();
    GameResolutionType resolution_type = (GameResolutionType)in->ReadInt32();
    assert(resolution_type == kGameResolution_Custom);
    Size game_size;
    game_size.Width = in->ReadInt32();
    game_size.Height = in->ReadInt32();
    SetGameResolution(game_size);

    default_lipsync_frame = in->ReadInt32();
    invhotdotsprite = in->ReadInt32();
    in->ReadArrayOfInt32(reserved, NUM_INTS_RESERVED);
    in->ReadArrayOfInt32(&info.HasMessages.front(), NUM_LEGACY_GLOBALMES);

    info.HasWordsDict = in->ReadInt32() != 0;
    in->ReadInt32(); // globalscript (dummy 32-bit pointer value)
    in->ReadInt32(); // chars (dummy 32-bit pointer value)
    info.HasCCScript = in->ReadInt32() != 0;
}

void GameSetupStructBase::WriteToFile(Stream *out, const SerializeInfo &info) const
{
    // NOTE: historically the struct was saved by dumping whole memory
    // into the file stream, which added padding from memory alignment;
    // here we mark the padding bytes, as they do not belong to actual data.
    out->Write(&gamename[0], GAME_NAME_LENGTH);
    out->WriteInt16(0); // alignment padding to int32
    out->WriteArrayOfInt32(options, MAX_OPTIONS);
    out->Write(&paluses[0], sizeof(paluses));
    // colors are an array of chars
    out->Write(&defpal[0], sizeof(defpal));
    out->WriteInt32(numviews);
    out->WriteInt32(numcharacters);
    out->WriteInt32(playercharacter);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt16(numinvitems);
    out->WriteInt16(0); // alignment padding to int32
    out->WriteInt32(numdialog);
    out->WriteInt32(numdlgmessage);
    out->WriteInt32(numfonts);
    out->WriteInt32(color_depth);
    out->WriteInt32(target_win);
    out->WriteInt32(dialog_bullet);
    out->WriteInt16(hotdot);
    out->WriteInt16(hotdotouter);
    out->WriteInt32(uniqueid);
    out->WriteInt32(numgui);
    out->WriteInt32(numcursors);
    out->WriteInt32(0); // [DEPRECATED] resolution type
    out->WriteInt32(_gameResolution.Width);
    out->WriteInt32(_gameResolution.Height);
    out->WriteInt32(default_lipsync_frame);
    out->WriteInt32(invhotdotsprite);
    out->WriteArrayOfInt32(reserved, NUM_INTS_RESERVED);
    out->WriteByteCount(0, sizeof(int32_t) * NUM_LEGACY_GLOBALMES);
    out->WriteInt32(dict ? 1 : 0);
    out->WriteInt32(0); // globalscript (dummy 32-bit pointer value)
    out->WriteInt32(0); // chars  (dummy 32-bit pointer value)
    out->WriteInt32(info.HasCCScript ? 1 : 0);
}

const char *GetScriptAPIName(ScriptAPIVersion v)
{
    switch (v)
    {
    case kScriptAPI_v321: return "v3.2.1";
    case kScriptAPI_v330: return "v3.3.0";
    case kScriptAPI_v334: return "v3.3.4";
    case kScriptAPI_v335: return "v3.3.5";
    case kScriptAPI_v340: return "v3.4.0";
    case kScriptAPI_v341: return "v3.4.1";
    case kScriptAPI_v350: return "v3.5.0-alpha";
    case kScriptAPI_v3507: return "v3.5.0-final";
    case kScriptAPI_v360: return "v3.6.0-alpha";
    case kScriptAPI_v36026: return "v3.6.0-final";
    case kScriptAPI_v361: return "v3.6.1";
    case kScriptAPI_v399: return "3.99.x";
    default: return "unknown";
    }
}
