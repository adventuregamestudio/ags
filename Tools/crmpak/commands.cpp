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
#include "data/room_utils.h"
#include "game/roomdata.h"
#include "game/room_file.h"
#include "gfx/image_file.h"
#include "script/cc_script.h"
#include "util/file.h"
#include "util/path.h"

using namespace AGS::Common;

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
        err = UpdateRoomData(&room, data_ver, false, nullptr);
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

bool SaveRoomFile(const RoomDataExt &room, const String &filename)
{
    auto out = File::CreateFile(filename);
    if (!out)
    {
        printf("Error: failed to open room file for writing.\n");
        return false;
    }

    HRoomFileError err = WriteRoomData(&room, out.get(), kRoomVersion_Current);
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

bool LoadImageFile(PixelBuffer &out_px_buf, RGB *pal, const String &filename)
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
        printf("Error: failed to write image to file: %s\n", filename.GetCStr());
        return false;
    }
    out_px_buf = px_buf;
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

bool LoadScriptFile(PScript &script, const String &filename)
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

bool DoesContentExist(const RoomDataExt &room, const Content &c, bool test_instance)
{
    switch (c.Type)
    {
    case kContent_Background:
        return c.Index >= 0 && static_cast<uint32_t>(c.Index) < room.BgFrameCount && (!test_instance || room.BgFrames[c.Index].GraphicBuf);
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
    for (const auto &c : content)
    {
        if (DoesContentExist(room, c, true) || IsContentGenerated(c))
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
                result = SaveImageFile(room.HotspotMaskBuf, room.Palette, c.FileName);
                break;
            case kContent_Region:
                result = SaveImageFile(room.RegionMaskBuf, room.Palette, c.FileName);
                break;
            case kContent_WalkArea:
                result = SaveImageFile(room.WalkAreaMaskBuf, room.Palette, c.FileName);
                break;
            case kContent_WalkBehind:
                result = SaveImageFile(room.WalkBehindMaskBuf, room.Palette, c.FileName);
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

void ImportContent(RoomDataExt &room, const std::vector<Content> &content)
{
    for (const auto &c : content)
    {
        if (DoesContentExist(room, c, false /* test only slot */))
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
                    result = LoadImageFile(room.BgFrames[c.Index].GraphicBuf, room.BgFrames[c.Index].Palette, c.FileName);
                }
                if (result && c.Index == 0)
                    room.BackgroundBPP = room.BgFrames[c.Index].GraphicBuf.GetBytesPerPixel();
                break;
            case kContent_Hotspot:
                result = LoadImageFile(room.HotspotMaskBuf, room.Palette, c.FileName);
                break;
            case kContent_Region:
                result = LoadImageFile(room.RegionMaskBuf, room.Palette, c.FileName);
                break;
            case kContent_WalkArea:
                result = LoadImageFile(room.WalkAreaMaskBuf, room.Palette, c.FileName);
                break;
            case kContent_WalkBehind:
                result = LoadImageFile(room.WalkBehindMaskBuf, room.Palette, c.FileName);
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
}

void CutContent(RoomDataExt &room, const std::vector<Content> &content)
{
    for (const auto &c : content)
    {
        if (DoesContentExist(room, c, true))
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

int Command_Create(const String &dst_room, const std::vector<Content> &content, bool verbose)
{
    printf("Output room file: %s\n", dst_room.GetCStr());
    PrintContentOptions(content, "Import");

    RoomDataExt empty;
    ImportContent(empty, content);
    if (!SaveRoomFile(empty, dst_room))
        return -1;

    printf("Done.\n");
    return 0;
}

int Command_Cut(const String &src_room, const String &dst_room, const std::vector<Content> &content, bool verbose)
{
    printf("Input room file: %s\n", src_room.GetCStr());
    printf("Output room file: %s\n", dst_room.GetCStr());
    PrintContentOptions(content, "Cut");

    RoomDataExt room;
    if (!LoadRoomFile(room, src_room, false, {}))
        return -1;

    CutContent(room, content);

    if (!SaveRoomFile(room, dst_room))
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

int Command_Import(const String &src_room, const String &dst_room, const std::vector<Content> &content, bool verbose)
{
    printf("Input room file: %s\n", src_room.GetCStr());
    printf("Output room file: %s\n", dst_room.GetCStr());
    PrintContentOptions(content, "Import");

    RoomDataExt room;
    if (!LoadRoomFile(room, src_room, false, {}))
        return -1;

    ImportContent(room, content);

    if (!SaveRoomFile(room, dst_room))
        return -1;

    printf("Done.\n");
    return 0;
}

int Command_List(const String &src_room)
{
    printf("Input room file: %s\n", src_room.GetCStr());

    RoomDataExt room;
    if (!LoadRoomFile(room, src_room, false, {}))
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

    printf("Done.\n");
    return 0;
}

} // namespace CRMPak
