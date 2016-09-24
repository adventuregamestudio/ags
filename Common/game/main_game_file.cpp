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

#include "core/asset.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "game/main_game_file.h"
#include "script/cc_error.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

const String MainGameSource::DefaultFilename_v3 = "game28.dta";
const String MainGameSource::DefaultFilename_v2 = "ac2game.dta";
const String MainGameSource::Signature = "Adventure Creator Game File v2";

MainGameSource::MainGameSource()
    : DataVersion(kGameVersion_Undefined)
{
}

String GetMainGameFileErrorText(MainGameFileError err)
{
    switch (err)
    {
    case kMGFErr_NoError:
        return "No error";
    case kMGFErr_FileNotFound:
        return "Main game file not found";
    case kMGFErr_NoStream:
        return "Failed to open input stream";
    case kMGFErr_SignatureFailed:
        return "Not an AGS main game file or unsupported format";
    case kMGFErr_FormatVersionTooOld:
        return "Format version is too old; this engine can only run games made with AGS 2.5 or later";
    case kMGFErr_FormatVersionNotSupported:
        return "Format version not supported";
    case kMGFErr_InvalidNativeResolution:
        return "Unable to determine native game resolution";
    case kMGFErr_TooManyFonts:
        return "Too many fonts for this engine to handle";
    case kMGFErr_NoGlobalScript:
        return "No global script in game";
    case kMGFErr_CreateGlobalScriptFailed:
        return String::FromFormat("Failed to load global script: %s", ccErrorString);
    case kMGFErr_CreateDialogScriptFailed:
        return String::FromFormat("Failed to load dialog script: %s", ccErrorString);
    case kMGFErr_CreateScriptModuleFailed:
        return String::FromFormat("Failed to load script module: %s", ccErrorString);
    case kMGFErr_NoFonts:
        return "No fonts specified to be used in this game";
    case kMGFErr_ScriptLinkFailed:
        return String::FromFormat("Script link failed: %s", ccErrorString);
    }
    return "Unknown error";
}

bool IsMainGameLibrary(const String &filename)
{
    // We must not only detect if the given file is a correct AGS data library,
    // we also have to assure that this library contains main game asset.
    // Library may contain some optional data (digital audio, speech etc), but
    // that is not what we want.
    AssetLibInfo lib;
    if (AssetManager::ReadDataFileTOC(filename, lib) != kAssetNoError)
        return false;
    for (size_t i = 0; i < lib.AssetInfos.size(); ++i)
    {
        if (lib.AssetInfos[i].FileName.CompareNoCase(MainGameSource::DefaultFilename_v3) == 0 ||
            lib.AssetInfos[i].FileName.CompareNoCase(MainGameSource::DefaultFilename_v2) == 0)
        {
            return true;
        }
    }
    return false;
}

MainGameFileError OpenMainGameFile(MainGameSource &src)
{
    // Cleanup source struct
    src = MainGameSource();
    // Try to find and open main game file
    String filename = MainGameSource::DefaultFilename_v3;
    PStream in(AssetManager::OpenAsset(filename));
    if (!in)
    {
        filename = MainGameSource::DefaultFilename_v2;
        in = PStream(AssetManager::OpenAsset(filename));
    }
    if (!in)
        return kMGFErr_FileNotFound;
    src.Filename = filename;
    // Check data signature
    String data_sig = String::FromStreamCount(in.get(), MainGameSource::Signature.GetLength());
    if (data_sig.Compare(MainGameSource::Signature))
        return kMGFErr_SignatureFailed;
    // Read data format version and requested engine version
    src.DataVersion = (GameDataVersion)in->ReadInt32();
    if (src.DataVersion < kGameVersion_250)
        return kMGFErr_FormatVersionTooOld;
    String version_str = StrUtil::ReadString(in.get());
    src.EngineVersion.SetFromString(version_str);
    if (src.DataVersion > kGameVersion_Current)
        return kMGFErr_FormatVersionNotSupported;
    // Everything is fine, return opened stream
    src.InputStream = in;
    // Remember loaded game data version
    // NOTE: this global variable is embedded in the code too much to get
    // rid of it too easily; the easy way is to set it whenever the main
    // game file is opened.
    loaded_game_file_version = src.DataVersion;
    return kMGFErr_NoError;
}

} // namespace Common
} // namespace AGS
