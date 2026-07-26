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
    {{ "undefined", "back", "mask-hotspot", "mask-region", "mask-walkarea", "mask-walkbehind", "script", "script-text" }};
const CstrArr<CRMPak::kNumContentTypes> FriendlyContentNames = 
    {{ "undefined", "Background %d", "Hotspot mask", "Region mask", "Walkable mask", "Walk-behind mask", "Compiled script", "Script text" }};

const CstrArr<kNumContentTypes> &GetContentNames()
{
    return ContentNames;
}

void Init()
{
    // Init Allegro RGB shifts; necessary for doing color conversions
    set_rgb_shifts(10, 5, 0, 11, 5, 0, 16, 8, 0, 16, 8, 0, 24);
}

bool LoadRoomFile(RoomData &room, const String &filename)
{
    auto in = File::OpenFileRead(filename);
    if (!in)
    {
        printf("Error: failed to open room file for reading.\n");
        return false;
    }

    // TODO: we may probably do a simpler parsing which only checks and records
    // available data instead of loading whole data into memory.
    RoomDataSource src(filename, std::move(in));
    HRoomFileError err = OpenRoomFile(src);
    if (err)
        err = ReadRoomData(&room, std::move(src.InputStream), src.DataVersion);
    if (!err)
    {
        printf("Error: failed to read room data.\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return false;
    }
    return true;
}

bool SaveRoomFile(const RoomData &room, const String &filename)
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

bool DoesContentExist(const RoomData &room, const Content &c, bool test_instance)
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
    case kContent_ScriptText: return false;// TODO: must read special data block!
    default: return false;
    }
}

String GetFriendlyContentName(const Content &c)
{
    if (c.Type == kContent_Background)
        return String::FromFormat(FriendlyContentNames[c.Type], c.Index);
    return String::Wrapper(FriendlyContentNames[c.Type]);
}

void ExportContent(const RoomData &room, const std::vector<Content> &content)
{
    for (const auto &c : content)
    {
        if (DoesContentExist(room, c, true))
        {
            bool result = false;
            switch (c.Type)
            {
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
                // TODO: must read special data block!
                break;
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

void ImportContent(RoomData &room, const std::vector<Content> &content)
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
                    result = LoadImageFile(room.BgFrames[c.Index].GraphicBuf, room.BgFrames[c.Index].Palette, c.FileName);
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
                // TODO: must read special data block!
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

void CutContent(RoomData &room, const std::vector<Content> &content)
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
            case kContent_ScriptText:
                // TODO: must read special data block!
                // Actually, the new room won't save this at all, so it gets cut anyway?
                // which means that we might have to allow to keep it on other operations.
                break;
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

    RoomData empty;
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

    RoomData room;
    if (!LoadRoomFile(room, src_room))
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

    RoomData room;
    if (!LoadRoomFile(room, src_room))
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

    RoomData room;
    if (!LoadRoomFile(room, src_room))
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

    RoomData room;
    if (!LoadRoomFile(room, src_room))
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
