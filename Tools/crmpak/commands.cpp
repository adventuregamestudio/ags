//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "commands.h"
#include <cinttypes>
#include "data/data_ext.h"
#include "data/room_utils.h"
#include "game/roomdata.h"
#include "game/room_file.h"
#include "game/room_file_blocks.h"
#include "gfx/image_file.h"
#include "script/cc_script.h"
#include "util/file.h"
#include "util/path.h"

using namespace AGS::Common;

// Default Allegro palette, to be used when exporting masks from non-8-bit rooms.
// This is necessary, because 16/32-bit rooms do not always contain valid palette,
// so mask colors will be not "visible".
// TODO: move elsewhere to let share among tools?
PALETTE default_palette =
{
    // FIXME: convert 64-unit RGBs to 256-unit RGBs!
    { 0,  0,  0,  0 },   { 0,  0,  42, 0 },   { 0,  42, 0,  0 },   { 0,  42, 42, 0 }, 
    { 42, 0,  0,  0 },   { 42, 0,  42, 0 },   { 42, 21, 0,  0 },   { 42, 42, 42, 0 }, 
    { 21, 21, 21, 0 },   { 21, 21, 63, 0 },   { 21, 63, 21, 0 },   { 21, 63, 63, 0 }, 
    { 63, 21, 21, 0 },   { 63, 21, 63, 0 },   { 63, 63, 21, 0 },   { 63, 63, 63, 0 }, 
    { 0,  0,  0,  0 },   { 5,  5,  5,  0 },   { 8,  8,  8,  0 },   { 11, 11, 11, 0 }, 
    { 14, 14, 14, 0 },   { 17, 17, 17, 0 },   { 20, 20, 20, 0 },   { 24, 24, 24, 0 }, 
    { 28, 28, 28, 0 },   { 32, 32, 32, 0 },   { 36, 36, 36, 0 },   { 40, 40, 40, 0 }, 
    { 45, 45, 45, 0 },   { 50, 50, 50, 0 },   { 56, 56, 56, 0 },   { 63, 63, 63, 0 }, 
    { 0,  0,  63, 0 },   { 16, 0,  63, 0 },   { 31, 0,  63, 0 },   { 47, 0,  63, 0 }, 
    { 63, 0,  63, 0 },   { 63, 0,  47, 0 },   { 63, 0,  31, 0 },   { 63, 0,  16, 0 }, 
    { 63, 0,  0,  0 },   { 63, 16, 0,  0 },   { 63, 31, 0,  0 },   { 63, 47, 0,  0 }, 
    { 63, 63, 0,  0 },   { 47, 63, 0,  0 },   { 31, 63, 0,  0 },   { 16, 63, 0,  0 }, 
    { 0,  63, 0,  0 },   { 0,  63, 16, 0 },   { 0,  63, 31, 0 },   { 0,  63, 47, 0 }, 
    { 0,  63, 63, 0 },   { 0,  47, 63, 0 },   { 0,  31, 63, 0 },   { 0,  16, 63, 0 }, 
    { 31, 31, 63, 0 },   { 39, 31, 63, 0 },   { 47, 31, 63, 0 },   { 55, 31, 63, 0 }, 
    { 63, 31, 63, 0 },   { 63, 31, 55, 0 },   { 63, 31, 47, 0 },   { 63, 31, 39, 0 }, 
    { 63, 31, 31, 0 },   { 63, 39, 31, 0 },   { 63, 47, 31, 0 },   { 63, 55, 31, 0 }, 
    { 63, 63, 31, 0 },   { 55, 63, 31, 0 },   { 47, 63, 31, 0 },   { 39, 63, 31, 0 }, 
    { 31, 63, 31, 0 },   { 31, 63, 39, 0 },   { 31, 63, 47, 0 },   { 31, 63, 55, 0 }, 
    { 31, 63, 63, 0 },   { 31, 55, 63, 0 },   { 31, 47, 63, 0 },   { 31, 39, 63, 0 }, 
    { 45, 45, 63, 0 },   { 49, 45, 63, 0 },   { 54, 45, 63, 0 },   { 58, 45, 63, 0 }, 
    { 63, 45, 63, 0 },   { 63, 45, 58, 0 },   { 63, 45, 54, 0 },   { 63, 45, 49, 0 }, 
    { 63, 45, 45, 0 },   { 63, 49, 45, 0 },   { 63, 54, 45, 0 },   { 63, 58, 45, 0 }, 
    { 63, 63, 45, 0 },   { 58, 63, 45, 0 },   { 54, 63, 45, 0 },   { 49, 63, 45, 0 }, 
    { 45, 63, 45, 0 },   { 45, 63, 49, 0 },   { 45, 63, 54, 0 },   { 45, 63, 58, 0 }, 
    { 45, 63, 63, 0 },   { 45, 58, 63, 0 },   { 45, 54, 63, 0 },   { 45, 49, 63, 0 }, 
    { 0,  0,  28, 0 },   { 7,  0,  28, 0 },   { 14, 0,  28, 0 },   { 21, 0,  28, 0 }, 
    { 28, 0,  28, 0 },   { 28, 0,  21, 0 },   { 28, 0,  14, 0 },   { 28, 0,  7,  0 }, 
    { 28, 0,  0,  0 },   { 28, 7,  0,  0 },   { 28, 14, 0,  0 },   { 28, 21, 0,  0 }, 
    { 28, 28, 0,  0 },   { 21, 28, 0,  0 },   { 14, 28, 0,  0 },   { 7,  28, 0,  0 }, 
    { 0,  28, 0,  0 },   { 0,  28, 7,  0 },   { 0,  28, 14, 0 },   { 0,  28, 21, 0 }, 
    { 0,  28, 28, 0 },   { 0,  21, 28, 0 },   { 0,  14, 28, 0 },   { 0,  7,  28, 0 }, 
    { 14, 14, 28, 0 },   { 17, 14, 28, 0 },   { 21, 14, 28, 0 },   { 24, 14, 28, 0 }, 
    { 28, 14, 28, 0 },   { 28, 14, 24, 0 },   { 28, 14, 21, 0 },   { 28, 14, 17, 0 }, 
    { 28, 14, 14, 0 },   { 28, 17, 14, 0 },   { 28, 21, 14, 0 },   { 28, 24, 14, 0 }, 
    { 28, 28, 14, 0 },   { 24, 28, 14, 0 },   { 21, 28, 14, 0 },   { 17, 28, 14, 0 }, 
    { 14, 28, 14, 0 },   { 14, 28, 17, 0 },   { 14, 28, 21, 0 },   { 14, 28, 24, 0 }, 
    { 14, 28, 28, 0 },   { 14, 24, 28, 0 },   { 14, 21, 28, 0 },   { 14, 17, 28, 0 }, 
    { 20, 20, 28, 0 },   { 22, 20, 28, 0 },   { 24, 20, 28, 0 },   { 26, 20, 28, 0 }, 
    { 28, 20, 28, 0 },   { 28, 20, 26, 0 },   { 28, 20, 24, 0 },   { 28, 20, 22, 0 }, 
    { 28, 20, 20, 0 },   { 28, 22, 20, 0 },   { 28, 24, 20, 0 },   { 28, 26, 20, 0 }, 
    { 28, 28, 20, 0 },   { 26, 28, 20, 0 },   { 24, 28, 20, 0 },   { 22, 28, 20, 0 }, 
    { 20, 28, 20, 0 },   { 20, 28, 22, 0 },   { 20, 28, 24, 0 },   { 20, 28, 26, 0 }, 
    { 20, 28, 28, 0 },   { 20, 26, 28, 0 },   { 20, 24, 28, 0 },   { 20, 22, 28, 0 }, 
    { 0,  0,  16, 0 },   { 4,  0,  16, 0 },   { 8,  0,  16, 0 },   { 12, 0,  16, 0 }, 
    { 16, 0,  16, 0 },   { 16, 0,  12, 0 },   { 16, 0,  8,  0 },   { 16, 0,  4,  0 }, 
    { 16, 0,  0,  0 },   { 16, 4,  0,  0 },   { 16, 8,  0,  0 },   { 16, 12, 0,  0 }, 
    { 16, 16, 0,  0 },   { 12, 16, 0,  0 },   { 8,  16, 0,  0 },   { 4,  16, 0,  0 }, 
    { 0,  16, 0,  0 },   { 0,  16, 4,  0 },   { 0,  16, 8,  0 },   { 0,  16, 12, 0 }, 
    { 0,  16, 16, 0 },   { 0,  12, 16, 0 },   { 0,  8,  16, 0 },   { 0,  4,  16, 0 }, 
    { 8,  8,  16, 0 },   { 10, 8,  16, 0 },   { 12, 8,  16, 0 },   { 14, 8,  16, 0 }, 
    { 16, 8,  16, 0 },   { 16, 8,  14, 0 },   { 16, 8,  12, 0 },   { 16, 8,  10, 0 }, 
    { 16, 8,  8,  0 },   { 16, 10, 8,  0 },   { 16, 12, 8,  0 },   { 16, 14, 8,  0 }, 
    { 16, 16, 8,  0 },   { 14, 16, 8,  0 },   { 12, 16, 8,  0 },   { 10, 16, 8,  0 }, 
    { 8,  16, 8,  0 },   { 8,  16, 10, 0 },   { 8,  16, 12, 0 },   { 8,  16, 14, 0 }, 
    { 8,  16, 16, 0 },   { 8,  14, 16, 0 },   { 8,  12, 16, 0 },   { 8,  10, 16, 0 }, 
    { 11, 11, 16, 0 },   { 12, 11, 16, 0 },   { 13, 11, 16, 0 },   { 15, 11, 16, 0 }, 
    { 16, 11, 16, 0 },   { 16, 11, 15, 0 },   { 16, 11, 13, 0 },   { 16, 11, 12, 0 }, 
    { 16, 11, 11, 0 },   { 16, 12, 11, 0 },   { 16, 13, 11, 0 },   { 16, 15, 11, 0 }, 
    { 16, 16, 11, 0 },   { 15, 16, 11, 0 },   { 13, 16, 11, 0 },   { 12, 16, 11, 0 }, 
    { 11, 16, 11, 0 },   { 11, 16, 12, 0 },   { 11, 16, 13, 0 },   { 11, 16, 15, 0 }, 
    { 11, 16, 16, 0 },   { 11, 15, 16, 0 },   { 11, 13, 16, 0 },   { 11, 12, 16, 0 }, 
    { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 }, 
    { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 63, 63, 63, 0 }
};

namespace CRMPak
{

const CstrArr<CRMPak::kNumContentTypes> ContentNames = 
    {{ "undefined", "ash", "back", "mask-hotspot", "mask-region", "mask-walkarea", "mask-walkbehind", "script", "script-text" }};
const CstrArr<CRMPak::kNumContentTypes> FriendlyContentNames = 
    {{ "undefined", "Room header", "Background %d", "Hotspot mask", "Region mask", "Walkable mask", "Walk-behind mask", "Compiled script", "Script text" }};

const CstrArr<kNumContentTypes> &GetContentNames()
{
    return ContentNames;
}

void Init()
{
    // Init Allegro RGB shifts; necessary for doing color conversions
    set_rgb_shifts(10, 5, 0, 11, 5, 0, 16, 8, 0, 16, 8, 0, 24);
}

HRoomFileError ReadOnlyScriptNames(RoomDataExt &room, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver)
{
    RoomReadOptions read_opts;
    read_opts.SkipImageData = true; // we read main block, which contains main bg and masks
    HRoomFileError err = ReadRoomData(&room, &room, std::move(in), data_ver,
        {{ kRoomFblk_Main, "" }, { kRoomFblk_ObjectScNames, "" }}, read_opts);
    // Must call UpdateRoomData, because certain script names from older room formats need to be converted
    if (err)
        err = UpdateRoomData(&room, data_ver, nullptr);
    if (!err)
        return err;
    return HRoomFileError::None();
}

HRoomFileError ReadOnlyCompiledScript(RoomDataExt &room, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver)
{
    return ReadRoomData(nullptr, &room, std::move(in), data_ver, {{ kRoomFblk_CompScript3, "" }}, {});
}

HRoomFileError ReadOnlyScriptText(RoomDataExt &room, std::unique_ptr<Stream> &&in, RoomFileVersion data_ver)
{
    return ReadRoomData(nullptr, &room, std::move(in), data_ver, {{ kRoomFblk_Script, "" }}, {});
}

bool LoadBlockList(std::vector<DataExtBlockInfo> &blk_infos, const String &filename)
{
    auto in = File::OpenFileRead(filename);
    if (!in)
    {
        printf("Error: failed to open room file for reading.\n");
        return false;
    }

    RoomDataSource src(filename, std::move(in));
    HError err = OpenRoomFile(src);
    if (err)
    {
        RoomBlockParser parser(std::move(src.InputStream), src.DataVersion);
        err = ReadExtBlockList(blk_infos, parser);
    }
    if (!err)
    {
        printf("Error: failed to read list of room blocks.\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return false;
    }
    return true;
}

bool LoadRoomFile(RoomDataExt &room, const String &filename, bool cmd_readonly, const std::vector<Content> &content)
{
    auto in = File::OpenFileRead(filename);
    if (!in)
    {
        printf("Error: failed to open room file for reading.\n");
        return false;
    }

    RoomDataSource src(filename, std::move(in));
    HRoomFileError err = OpenRoomFile(src);
    if (err)
    {
        // This is the basic attempt to optimize export performance by reducing amount
        // of loaded data. Currently done only for couple of the most common use cases.
        // But in theory this may be expanded further by utilizing RoomBlockReader class
        // and it's feature of providing a reading delegate (see room_file.cpp).
        if (cmd_readonly && content.size() == 1)
        {
            if (content[0].Type == CRMPak::kContent_Ash)
                err = ReadOnlyScriptNames(room, std::move(src.InputStream), src.DataVersion);
            else if (content[0].Type == CRMPak::kContent_ScriptCompiled3)
                err = ReadOnlyCompiledScript(room, std::move(src.InputStream), src.DataVersion);
            else if (content[0].Type == CRMPak::kContent_ScriptText)
                err = ReadOnlyScriptText(room, std::move(src.InputStream), src.DataVersion);
            else
                err = ReadRoomData(&room, std::move(src.InputStream), src.DataVersion);
        }
        else
        {
            err = ReadRoomData(&room, std::move(src.InputStream), src.DataVersion);
        }
    }
    if (!err)
    {
        printf("Error: failed to read room data.\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return false;
    }

    return true;
}

bool SaveRoomFile(const RoomDataExt &room, const String &filename, const String &compiled_with)
{
    auto out = File::CreateFile(filename);
    if (!out)
    {
        printf("Error: failed to open room file for writing.\n");
        return false;
    }

    HRoomFileError err = WriteRoomData(&room, out.get(), kRoomVersion_Current, compiled_with);
    if (!err)
    {
        printf("Error: failed to write room data.\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return false;
    }
    return true;
}

void PrintContentOptions(const std::vector<Content> &content, const char *operation)
{
    if (content.size() > 0)
    {
        printf("%s content:\n", operation);
        for (const auto &c : content)
        {
            if (c.Type == kContent_Background)
                printf("\t%s%d = %s\n", ContentNames[c.Type], c.Index, c.FileName.GetCStr());
            else
                printf("\t%s = %s\n", ContentNames[c.Type], c.FileName.GetCStr());
        }
    }
    else
    {
        printf("%s content: none\n", operation);
    }
}

bool LoadImageFile(PixelBuffer &out_px_buf, RGB *pal, const String &filename, int import_bpp = 0)
{
    auto in = File::OpenFileRead(filename);
    if (!in)
    {
        printf("Error: failed to open file for reading: %s\n", filename.GetCStr());
        return false;
    }
    auto px_buf = ImageFile::LoadImage(in.get(), Path::GetFileExtension(filename), nullptr, pal);
    if (!px_buf)
    {
        printf("Error: failed to read image from file: %s\n", filename.GetCStr());
        return false;
    }
    if (import_bpp == 0)
    {
        out_px_buf = px_buf;
    }
    else
    {
        if (!PixelOp::CopyConvert(px_buf, out_px_buf, ColorDepthToPixelFormat(import_bpp)))
        {
            printf("Error: failed to convert input image (%d bpp) to destination format (%d bpp)\n", px_buf.GetColorDepth(), import_bpp);
            return false;
        }
    }
    
    return true;
}

bool SaveImageFile(const BitmapData &bm_data, const RGB *pal, const String &filename)
{
    auto out = File::CreateFile(filename);
    if (!out)
    {
        printf("Error: failed to open file for writing: %s\n", filename.GetCStr());
        return false;
    }
    if (!ImageFile::SaveImage(bm_data, true /* opaque */, pal, out.get(), Path::GetFileExtension(filename)))
    {
        printf("Error: failed to write image to file: %s\n", filename.GetCStr());
        return false;
    }
    return true;
}

bool LoadTextFile(String &text, const String &filename)
{
    auto in = File::OpenFileRead(filename);
    if (!in)
    {
        printf("Error: failed to open file for reading: %s\n", filename.GetCStr());
        return false;
    }
    text = String::FromStream(in.get(), SIZE_MAX);
    return true;
}

bool SaveTextFile(const String &text, const String &filename)
{
    auto out = File::CreateFile(filename);
    if (!out)
    {
        printf("Error: failed to open file for writing: %s\n", filename.GetCStr());
        return false;
    }
    out->Write(text.GetCStr(), text.GetLength());
    return true;
}

bool LoadScriptFile(UScript &script, const String &filename)
{
    auto in = File::OpenFileRead(filename);
    if (!in)
    {
        printf("Error: failed to open file for reading: %s\n", filename.GetCStr());
        return false;
    }
    script.reset(ccScript::CreateFromStream(in.get()));
    return script != nullptr;
}

bool SaveScriptFile(const ccScript &script, const String &filename)
{
    auto out = File::CreateFile(filename);
    if (!out)
    {
        printf("Error: failed to open file for writing: %s\n", filename.GetCStr());
        return false;
    }
    script.Write(out.get());
    return true;
}

bool DoesContentExist(const RoomDataExt &room, const Content &c, bool allow_expand, bool test_instance)
{
    switch (c.Type)
    {
    case kContent_Background:
        return c.Index >= 0 && static_cast<uint32_t>(c.Index) < (allow_expand ? MAX_ROOM_BGFRAMES : room.BgFrameCount)
            && (!test_instance || room.BgFrames[c.Index].GraphicBuf);
    case kContent_Hotspot: return (!test_instance || room.HotspotMaskBuf);
    case kContent_Region: return (!test_instance || room.RegionMaskBuf);
    case kContent_WalkArea: return (!test_instance || room.WalkAreaMaskBuf);
    case kContent_WalkBehind: return (!test_instance || room.WalkBehindMaskBuf);
    case kContent_ScriptCompiled3: return (!test_instance || room.CompiledScript != nullptr);
    case kContent_ScriptText: return (!test_instance || !room.ScriptText.IsEmpty());
    // NOTE: generated content never exists in room data
    default: return false;
    }
}

bool IsContentGenerated(const Content &c)
{
    switch (c.Type)
    {
    case kContent_Ash: return true;
    default: return false;
    }
}

String GetFriendlyContentName(const Content &c)
{
    if (c.Type == kContent_Background)
        return String::FromFormat(FriendlyContentNames[c.Type], c.Index);
    return String::Wrapper(FriendlyContentNames[c.Type]);
}

bool Export_Ash(const RoomDataExt &room, const String &filename)
{
    DataUtil::RoomScriptNames names;
    for (const auto &h : room.Hotspots)
        if (!h.ScriptName.IsEmpty())
            names.HotspotNames.push_back(h.ScriptName);
    for (const auto &o : room.Objects)
        if (!o.ScriptName.IsEmpty())
            names.ObjectNames.push_back(o.ScriptName);
    return SaveTextFile(DataUtil::MakeRoomScriptHeader(names), filename);
}

void ExportContent(const RoomDataExt &room, const std::vector<Content> &content)
{
    const auto &pal_to_use = default_palette;
    for (const auto &c : content)
    {
        if (DoesContentExist(room, c, false, true) || IsContentGenerated(c))
        {
            bool result = false;
            switch (c.Type)
            {
            case kContent_Ash:
                result = Export_Ash(room, c.FileName);
                break;
            case kContent_Background:
                if (c.Index >= 0 && static_cast<uint32_t>(c.Index) < room.BgFrameCount)
                    result = SaveImageFile(room.BgFrames[c.Index].GraphicBuf, room.BgFrames[c.Index].Palette, c.FileName);
                break;
            case kContent_Hotspot:
                result = SaveImageFile(room.HotspotMaskBuf, pal_to_use, c.FileName);
                break;
            case kContent_Region:
                result = SaveImageFile(room.RegionMaskBuf, pal_to_use, c.FileName);
                break;
            case kContent_WalkArea:
                result = SaveImageFile(room.WalkAreaMaskBuf, pal_to_use, c.FileName);
                break;
            case kContent_WalkBehind:
                result = SaveImageFile(room.WalkBehindMaskBuf, pal_to_use, c.FileName);
                break;
            case kContent_ScriptCompiled3:
                result = SaveScriptFile(*room.CompiledScript, c.FileName);
                break;
            case kContent_ScriptText:
                result = SaveTextFile(room.ScriptText, c.FileName);
                break;
            default: break;
            }

            if (result)
                printf("+ %s: %s\n", GetFriendlyContentName(c).GetCStr(), c.FileName.GetCStr());
        }
        else
        {
            printf("Warning: %s does not exist\n", GetFriendlyContentName(c).GetCStr());
        }
    }
}

void ImportContent(RoomDataExt &room, const std::vector<Content> &content, int import_bpp)
{
    int old_room_bpp = room.BackgroundBPP;
    int new_room_bpp = old_room_bpp;
    for (const auto &c : content)
    {
        if (DoesContentExist(room, c, true /* allow new items */, false /* test only slot */))
        {
            bool result = false;
            switch (c.Type)
            {
            case kContent_Background:
                if (c.Index >= 0 && static_cast<uint32_t>(c.Index) < MAX_ROOM_BGFRAMES)
                {
                    if (static_cast<uint32_t>(c.Index) >= room.BgFrameCount)
                    {
                        room.BgFrameCount = c.Index + 1u;
                        room.BgFrames.resize(room.BgFrameCount);
                    }
                    result = LoadImageFile(room.BgFrames[c.Index].GraphicBuf, room.BgFrames[c.Index].Palette, c.FileName,
                        (c.Index == 0 && import_bpp > 0) ? import_bpp : room.BackgroundBPP * 8);
                }
                if (result && c.Index == 0)
                {
                    room.BackgroundBPP = room.BgFrames[0].GraphicBuf.GetBytesPerPixel();
                    new_room_bpp = room.BackgroundBPP;
                }
                break;
            case kContent_Hotspot:
                result = LoadImageFile(room.HotspotMaskBuf, room.Palette, c.FileName, 8);
                break;
            case kContent_Region:
                result = LoadImageFile(room.RegionMaskBuf, room.Palette, c.FileName, 8);
                break;
            case kContent_WalkArea:
                result = LoadImageFile(room.WalkAreaMaskBuf, room.Palette, c.FileName, 8);
                break;
            case kContent_WalkBehind:
                result = LoadImageFile(room.WalkBehindMaskBuf, room.Palette, c.FileName, 8);
                break;
            case kContent_ScriptCompiled3:
                result = LoadScriptFile(room.CompiledScript, c.FileName);
                break;
            case kContent_ScriptText:
                result = LoadTextFile(room.ScriptText, c.FileName);
                break;
            }

            if (result)
                printf("+ %s: %s\n", GetFriendlyContentName(c).GetCStr(), c.FileName.GetCStr());
        }
        else
        {
            printf("Warning: %s cannot be imported\n", GetFriendlyContentName(c).GetCStr());
        }
    }

    if ((old_room_bpp != new_room_bpp) && (room.BgFrameCount > 1))
    {
        printf("Primary background's color depth changed (%d-bits -> %d-bits);\n\twill convert %u existing secondary background frames to the new color depth.\n",
            old_room_bpp * 8, new_room_bpp * 8, room.BgFrameCount - 1);
        for (uint32_t i = 1; i < room.BgFrameCount; ++i)
        {
            PixelBuffer newbuf;
            PixelOp::CopyConvert(room.BgFrames[i].GraphicBuf, newbuf, ColorDepthToPixelFormat(room.BackgroundBPP * 8));
            room.BgFrames[i].GraphicBuf = std::move(newbuf);
        }
    }
}

void CutContent(RoomDataExt &room, const std::vector<Content> &content)
{
    for (const auto &c : content)
    {
        if (DoesContentExist(room, c, false, true))
        {
            switch (c.Type)
            {
            case kContent_Background:
                if (c.Index >= 0 && static_cast<uint32_t>(c.Index) < room.BgFrameCount)
                    room.BgFrames[c.Index].GraphicBuf = {};
                break;
            case kContent_Hotspot: room.HotspotMaskBuf = {}; break;
            case kContent_Region: room.RegionMaskBuf = {}; break;
            case kContent_WalkArea: room.WalkAreaMaskBuf = {}; break;
            case kContent_WalkBehind: room.WalkBehindMaskBuf = {}; break;
            case kContent_ScriptCompiled3: room.CompiledScript = nullptr; break;
            case kContent_ScriptText: room.ScriptText = {}; break;
            }

            printf("- %s\n", GetFriendlyContentName(c).GetCStr());
        }
        else
        {
            printf("Info: %s does not exist\n", GetFriendlyContentName(c).GetCStr());
        }
    }
}

int Command_Create(const String &dst_room, const std::vector<Content> &content, int import_bpp, const String &compiled_with, bool verbose)
{
    printf("Output room file: %s\n", dst_room.GetCStr());
    PrintContentOptions(content, "Import");

    RoomDataExt empty;
    ImportContent(empty, content, import_bpp);
    if (!SaveRoomFile(empty, dst_room, compiled_with))
        return -1;

    printf("Done.\n");
    return 0;
}

int Command_Cut(const String &src_room, const String &dst_room, const std::vector<Content> &content, const String &compiled_with, bool verbose)
{
    printf("Input room file: %s\n", src_room.GetCStr());
    printf("Output room file: %s\n", dst_room.GetCStr());
    PrintContentOptions(content, "Cut");

    RoomDataExt room;
    if (!LoadRoomFile(room, src_room, false, {}))
        return -1;

    CutContent(room, content);

    if (!SaveRoomFile(room, dst_room, compiled_with))
        return -1;

    printf("Done.\n");
    return 0;
}

int Command_Export(const String &src_room, const std::vector<Content> &content, bool verbose)
{
    printf("Input room file: %s\n", src_room.GetCStr());
    PrintContentOptions(content, "Export");

    RoomDataExt room;
    if (!LoadRoomFile(room, src_room, true, content))
        return -1;

    // Export content to files
    ExportContent(room, content);

    printf("Done.\n");
    return 0;
}

int Command_Import(const String &src_room, const String &dst_room, const std::vector<Content> &content, int import_bpp, const String &compiled_with, bool verbose)
{
    printf("Input room file: %s\n", src_room.GetCStr());
    printf("Output room file: %s\n", dst_room.GetCStr());
    PrintContentOptions(content, "Import");

    RoomDataExt room;
    if (!LoadRoomFile(room, src_room, false, {}))
        return -1;

    ImportContent(room, content, import_bpp);

    if (!SaveRoomFile(room, dst_room, compiled_with))
        return -1;

    printf("Done.\n");
    return 0;
}

int Command_List(const String &src_room)
{
    printf("Input room file: %s\n", src_room.GetCStr());

    std::vector<DataExtBlockInfo> blk_infos;
    if (!LoadBlockList(blk_infos, src_room))
        return -1;

    printf("Data blocks count: %zu\n", blk_infos.size());
    printf("------ Block ID ------|------- Offset -------|--- Size --\n");
    for (const auto &blk_info : blk_infos)
    {
        printf(" %-16s (%d) | %20" PRId64 " | %10zu\n",
            blk_info.ID.Name.GetCStr(), blk_info.ID.ID, blk_info.Offset, static_cast<size_t>(blk_info.Length));
    }
    printf("---------------------------------------------------------\n\n");

    RoomDataExt room;
    if (!LoadRoomFile(room, src_room, true, {}))
        return -1;

    printf("* Room backgrounds: %d\n", room.BgFrameCount);
    for (uint32_t i = 0; i < room.BgFrameCount; ++i)
    {
        if (room.BgFrames[i].GraphicBuf)
            printf("\tbackground %u: %dx%d %d-bit\n", i, room.BgFrames[i].GraphicBuf.GetWidth(), room.BgFrames[i].GraphicBuf.GetHeight(), room.BgFrames[i].GraphicBuf.GetColorDepth());
        else
            printf("\tbackground %u: no image\n", i);
    }

    if (room.HotspotMaskBuf)
    {
        printf("* Hotspot mask: %dx%d %d-bit\n", room.HotspotMaskBuf.GetWidth(), room.HotspotMaskBuf.GetHeight(), room.HotspotMaskBuf.GetColorDepth());
        printf("* Hotspots: %d\n", room.HotspotCount);
    }
    if (room.RegionMaskBuf)
    {
        printf("* Region mask: %dx%d %d-bit\n", room.RegionMaskBuf.GetWidth(), room.RegionMaskBuf.GetHeight(), room.RegionMaskBuf.GetColorDepth());
        printf("* Regions: %d\n", room.RegionCount);
    }
    if (room.WalkAreaMaskBuf)
    {
        printf("* Walkable mask: %dx%d %d-bit\n", room.WalkAreaMaskBuf.GetWidth(), room.WalkAreaMaskBuf.GetHeight(), room.WalkAreaMaskBuf.GetColorDepth());
        printf("* Walkable areas: %d\n", room.WalkAreaCount);
    }
    if (room.WalkBehindMaskBuf)
    {
        printf("* Walk-behind mask: %dx%d %d-bit\n", room.WalkBehindMaskBuf.GetWidth(), room.WalkBehindMaskBuf.GetHeight(), room.WalkBehindMaskBuf.GetColorDepth());
        printf("* Walk-behinds: %d\n", room.WalkBehindCount);
    }

    printf("* Objects: %zu\n", room.Objects.size());

    if (room.CompiledScript)
    {
        printf("* Compiled script:\n\tbytecode %zu bytes\n\tvariables %zu bytes\n\tstrings %zu bytes\n",
            room.CompiledScript->code.size(), room.CompiledScript->globaldata.size(), room.CompiledScript->strings.size());
    }

    if (!room.ScriptText.IsEmpty())
    {
        printf("* Script text: %zu bytes\n", room.ScriptText.GetLength() + 1);
    }

    printf("Done.\n");
    return 0;
}

} // namespace CRMPak
