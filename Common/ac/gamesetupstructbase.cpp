//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/characterinfo.h"
#include "ac/gamesetupstructbase.h"
#include "ac/game_version.h"
#include "ac/wordsdictionary.h"
#include "script/cc_script.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;


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
    gamename.ReadCount(in, LEGACY_GAME_NAME_LENGTH);
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
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hot cross color
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
    hotdot = in->ReadInt32();
    hotdotouter = in->ReadInt32();
    in->ReadArrayOfInt32(reserved, NUM_INTS_RESERVED);
    info.ExtensionOffset = static_cast<uint32_t>(in->ReadInt32());

    in->ReadArrayOfInt32(info.HasMessages.data(), NUM_LEGACY_GLOBALMES);
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
    gamename.WriteCount(out, LEGACY_GAME_NAME_LENGTH);
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
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    out->WriteInt32(uniqueid);
    out->WriteInt32(numgui);
    out->WriteInt32(numcursors);
    out->WriteInt32(0); // [DEPRECATED] resolution type
    out->WriteInt32(_gameResolution.Width);
    out->WriteInt32(_gameResolution.Height);
    out->WriteInt32(default_lipsync_frame);
    out->WriteInt32(invhotdotsprite);
    out->WriteInt32(hotdot);
    out->WriteInt32(hotdotouter);
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
    case kScriptAPI_v362: return "v3.6.2";
    case kScriptAPI_v363: return "v3.6.3";
    case kScriptAPI_v399: return "3.99.x";
    case kScriptAPI_v400: return "4.0.0-alpha8";
    case kScriptAPI_v400_07: return "4.0.0-alpha12";
    case kScriptAPI_v400_14: return "4.0.0-alpha18";
    case kScriptAPI_v400_16: return "4.0.0-alpha20";
    case kScriptAPI_v400_18: return "4.0.0-alpha22";
    case kScriptAPI_v400_24: return "4.0.0-alpha28";
    default: return "unknown";
    }
}
