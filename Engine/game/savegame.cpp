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

#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/richgamemedia.h"
#include "ac/gamesetup.h"
#include "gfx/bitmap.h"
#include "game/savegame.h"
#include "main/main.h"
#include "main/version.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "util/alignedstream.h"
#include "util/file.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace Common;
using namespace Engine;

// function is currently implemented in game.cpp
AGS::Engine::SavegameError restore_game_data(Stream *in, SavegameVersion svg_version);
extern GameSetupStruct game;


namespace AGS
{
namespace Engine
{

const String SavegameSource::Signature = "Adventure Game Studio saved game";


String GetSavegameErrorText(SavegameError err)
{
    switch (err)
    {
    case kSvgErr_NoError:
        return "No error";
    case kSvgErr_FileNotFound:
        return "File not found";
    case kSvgErr_SignatureFailed:
        return "Not an AGS saved game or unsupported format";
    case kSvgErr_FormatVersionNotSupported:
        return "Save format version not supported";
    case kSvgErr_IncompatibleEngine:
        return "Saved game was written by incompatible interpreter";
    case kSvgErr_DifferentColorDepth:
        return "Saved with different colour depth";
    }
    return "Unknown error";
}

Bitmap *RestoreSaveImage(Stream *in)
{
    if (in->ReadInt32())
        return read_serialized_bitmap(in);
    return NULL;
}

void SkipSaveImage(Stream *in)
{
    if (in->ReadInt32())
        skip_serialized_bitmap(in);
}

SavegameError OpenSavegameBase(const String &filename, SavegameSource *src, SavegameDescription *desc, SavegameDescElem elems)
{
    AStream in(File::OpenFileRead(filename));
    if (!in.get())
        return kSvgErr_FileNotFound;

    // Skip MS Windows Vista rich media header
    RICH_GAME_MEDIA_HEADER rich_media_header;
    rich_media_header.ReadFromFile(in.get());

    // Check saved game signature
    String svg_sig = String::FromStreamCount(in.get(), SavegameSource::Signature.GetLength());
    if (svg_sig.Compare(SavegameSource::Signature))
        return kSvgErr_SignatureFailed;

    String desc_text;
    if (desc && elems == kSvgDesc_UserText)
        desc_text.Read(in.get());
    else
        for (; in->ReadByte(); ); // skip until null terminator
    SavegameVersion svg_ver = (SavegameVersion)in->ReadInt32();

    // Check saved game format version
    if (svg_ver < kSvgVersion_LowestSupported ||
        svg_ver > kSvgVersion_Current)
    {
        return kSvgErr_FormatVersionNotSupported;
    }

    ABitmap image;
    if (desc && elems == kSvgDesc_UserImage)
        image.reset(RestoreSaveImage(in.get()));
    else
        SkipSaveImage(in.get());

    String version_str = String::FromStream(in.get());
    Version eng_version(version_str);
    if (eng_version > EngineVersion ||
        eng_version < SavedgameLowestBackwardCompatVersion)
    {
        // Engine version is either non-forward or non-backward compatible
        return kSvgErr_IncompatibleEngine;
    }
    String main_file;
    if (desc && elems == kSvgDesc_EnvInfo)
        main_file.Read(in.get());
    else
        for (; in->ReadByte(); ); // skip until null terminator

    if (src)
    {
        src->Filename = filename;
        src->Version = svg_ver;
        src->InputStream.reset(in.release());
    }
    if (desc)
    {
        if (elems == kSvgDesc_EnvInfo)
        {
            desc->EngineVersion = eng_version;
            desc->MainDataFilename = main_file;
        }
        if (elems == kSvgDesc_UserText)
            desc->UserText = desc_text;
        if (elems == kSvgDesc_UserImage)
            desc->UserImage.reset(image.release());
    }
    return kSvgErr_NoError;
}

SavegameError OpenSavegame(const String &filename, SavegameSource &src, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, &src, &desc, elems);
}

SavegameError OpenSavegame(const String &filename, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, NULL, &desc, elems);
}

SavegameError RestoreGameState(Stream *in, SavegameVersion svg_version)
{
    return restore_game_data(in, svg_version);
}

void WriteSaveImage(Stream *out, const Bitmap *screenshot)
{
    // store the screenshot at the start to make it easily accesible
    out->WriteInt32((screenshot == NULL) ? 0 : 1);

    if (screenshot)
        serialize_bitmap(screenshot, out);
}

Stream *StartSavegame(const String &filename, const String &desc, const Bitmap *image)
{
    Stream *out = Common::File::CreateFile(filename);
    if (!out)
        return NULL;

    // Initialize and write Vista header
    RICH_GAME_MEDIA_HEADER vistaHeader;
    memset(&vistaHeader, 0, sizeof(RICH_GAME_MEDIA_HEADER));
    memcpy(&vistaHeader.dwMagicNumber, RM_MAGICNUMBER, sizeof(int));
    vistaHeader.dwHeaderVersion = 1;
    vistaHeader.dwHeaderSize = sizeof(RICH_GAME_MEDIA_HEADER);
    vistaHeader.dwThumbnailOffsetHigherDword = 0;
    vistaHeader.dwThumbnailOffsetLowerDword = 0;
    vistaHeader.dwThumbnailSize = 0;
    convert_guid_from_text_to_binary(game.guid, &vistaHeader.guidGameId[0]);
    uconvert(game.gamename, U_ASCII, (char*)&vistaHeader.szGameName[0], U_UNICODE, RM_MAXLENGTH);
    uconvert(desc, U_ASCII, (char*)&vistaHeader.szSaveName[0], U_UNICODE, RM_MAXLENGTH);
    vistaHeader.szLevelName[0] = 0;
    vistaHeader.szComments[0] = 0;

    // MS Windows Vista rich media header
    vistaHeader.WriteToFile(out);

    // Savegame signature
    out->Write(SavegameSource::Signature, SavegameSource::Signature.GetLength());
    // Description
    StrUtil::WriteCStr(desc, out);

    platform->RunPluginHooks(AGSE_PRESAVEGAME, 0);
    out->WriteInt32(kSvgVersion_Current);
    WriteSaveImage(out, image);

    // Write lowest forward-compatible version string, so that
    // earlier versions could load savedgames made by current engine
    String compat_version;
    if (SavedgameLowestForwardCompatVersion <= Version::LastOldFormatVersion)
        compat_version = SavedgameLowestForwardCompatVersion.BackwardCompatibleString;
    else
        compat_version = SavedgameLowestForwardCompatVersion.LongString;
    StrUtil::WriteCStr(compat_version, out);
    StrUtil::WriteCStr(usetup.main_data_filename, out);
    return out;
}

} // namespace Engine
} // namespace AGS
