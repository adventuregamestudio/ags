// AGS Editor ImGui - Room file loader implementation
#include "room_loader.h"
#include "sprite_loader.h"
#include "ac/common_defines.h"
#include "ac/gamestructdefines.h"
#include "game/room_file.h"
#include "game/room_version.h"
#include "game/roomstruct.h"
#include "game/interactions.h"
#include "util/file.h"
#include "util/string.h"

#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

namespace AGSEditor
{

// Helper: extract event handler function names from AGS InteractionEvents
static Interactions ExtractInteractions(
    const AGS::Common::UInteractionEvents& events, int expected_count)
{
    Interactions result;
    result.handler_functions.resize(expected_count);
    if (events)
    {
        result.script_module = events->ScriptModule.GetCStr();
        for (size_t i = 0; i < events->Events.size() && (int)i < expected_count; i++)
        {
            result.handler_functions[i] = events->Events[i].FunctionName.GetCStr();
        }
    }
    return result;
}

// Helper: write event handler function names back to AGS InteractionEvents
static void ApplyInteractions(
    AGS::Common::UInteractionEvents& events, const Interactions& data)
{
    if (!events)
        events = std::make_unique<AGS::Common::InteractionEvents>();
    events->ScriptModule = data.script_module.c_str();
    events->Events.clear();
    for (const auto& fn : data.handler_functions)
    {
        events->Events.emplace_back(AGS::Common::String(fn.c_str()));
    }
}

RoomLoader::RoomLoader() = default;
RoomLoader::~RoomLoader() = default;

std::string RoomLoader::GetRoomFilename(int room_number) const
{
    char buf[64];
    snprintf(buf, sizeof(buf), "room%d.crm", room_number);
    return (fs::path(game_dir_) / buf).string();
}

std::unique_ptr<RoomData> RoomLoader::LoadRoom(int room_number)
{
    using namespace AGS::Common;

    std::string filepath = GetRoomFilename(room_number);

    // Open the room file
    RoomDataSource src;
    HRoomFileError err = OpenRoomFile(String(filepath.c_str()), src);
    if (!err)
    {
        fprintf(stderr, "[RoomLoader] Cannot open room file '%s': %s\n",
                filepath.c_str(), err->FullMessage().GetCStr());
        return nullptr;
    }

    // Read room data
    RoomStruct room;
    HRoomFileError read_err = ReadRoomData(&room, std::move(src.InputStream), src.DataVersion);
    if (!read_err)
    {
        fprintf(stderr, "[RoomLoader] Failed to read room data: %s\n",
                read_err->FullMessage().GetCStr());
        return nullptr;
    }

    // Apply updates - build sprite info from sprite loader if available
    std::vector<SpriteInfo> sprinfos;
    if (sprite_loader_)
    {
        int count = sprite_loader_->GetSpriteCount();
        sprinfos.resize(count);
        for (int i = 0; i < count; i++)
        {
            const SpriteMetrics* m = sprite_loader_->GetMetrics(i);
            if (m && m->exists)
            {
                sprinfos[i].Width = m->width;
                sprinfos[i].Height = m->height;
            }
        }
    }
    HRoomFileError upd_err = UpdateRoomData(&room, src.DataVersion, game_is_hires_, sprinfos);
    if (!upd_err)
    {
        fprintf(stderr, "[RoomLoader] Warning during room update: %s\n",
                upd_err->FullMessage().GetCStr());
    }

    // Convert to our RoomData structure
    auto data = std::make_unique<RoomData>();
    data->number = room_number;
    data->width = room.Width;
    data->height = room.Height;
    data->mask_resolution = room.MaskResolution;
    data->bg_frame_count = room.BgFrameCount;
    data->bg_bpp = room.BackgroundBPP;

    // Edges
    data->left_edge = room.Edges.Left;
    data->right_edge = room.Edges.Right;
    data->top_edge = room.Edges.Top;
    data->bottom_edge = room.Edges.Bottom;

    // Hotspots
    for (uint32_t i = 0; i < room.HotspotCount && i < MAX_ROOM_HOTSPOTS; i++)
    {
        RoomData::HotspotData hs;
        hs.id = (int)i;
        hs.name = room.Hotspots[i].Name.GetCStr();
        hs.script_name = room.Hotspots[i].ScriptName.GetCStr();
        hs.walk_to_x = room.Hotspots[i].WalkTo.X;
        hs.walk_to_y = room.Hotspots[i].WalkTo.Y;
        hs.interactions = ExtractInteractions(room.Hotspots[i].EventHandlers, 10);
        for (const auto& kv : room.Hotspots[i].Properties)
            hs.custom_properties[kv.first.GetCStr()] = kv.second.GetCStr();
        data->hotspots.push_back(hs);
    }

    // Objects
    // Objects
    for (size_t i = 0; i < room.Objects.size(); i++)
    {
        RoomData::ObjectData obj;
        obj.id = (int)i;
        obj.name = room.Objects[i].Name.GetCStr();
        obj.script_name = room.Objects[i].ScriptName.GetCStr();
        obj.x = room.Objects[i].X;
        obj.y = room.Objects[i].Y;
        obj.sprite = room.Objects[i].Sprite;
        obj.visible = room.Objects[i].IsOn;
        obj.baseline = room.Objects[i].Baseline;
        obj.clickable = !(room.Objects[i].Flags & OBJF_NOINTERACT);
        obj.interactions = ExtractInteractions(room.Objects[i].EventHandlers, 8);
        for (const auto& kv : room.Objects[i].Properties)
            obj.custom_properties[kv.first.GetCStr()] = kv.second.GetCStr();
        data->objects.push_back(obj);
    }

    // Walk areas
    for (uint32_t i = 0; i < room.WalkAreaCount && i < MAX_WALK_AREAS; i++)
    {
        RoomData::WalkAreaData wa;
        wa.id = (int)i;
        wa.scaling_min = room.WalkAreas[i].ScalingNear;
        wa.scaling_max = room.WalkAreas[i].ScalingFar;
        wa.continuous_scaling = (room.WalkAreas[i].ScalingNear != NOT_VECTOR_SCALED);
        wa.top = room.WalkAreas[i].Top;
        wa.bottom = room.WalkAreas[i].Bottom;
        data->walk_areas.push_back(wa);
    }

    // Walk behinds
    for (uint32_t i = 0; i < room.WalkBehindCount && i < MAX_WALK_BEHINDS; i++)
    {
        RoomData::WalkBehindData wb;
        wb.id = (int)i;
        wb.baseline = room.WalkBehinds[i].Baseline;
        data->walk_behinds.push_back(wb);
    }

    // Regions
    for (uint32_t i = 0; i < room.RegionCount && i < MAX_ROOM_REGIONS; i++)
    {
        RoomData::RegionData reg;
        reg.id = (int)i;
        reg.light_level = room.Regions[i].Light;
        int tint = room.Regions[i].Tint;
        reg.tint_r = (tint >> 16) & 0xFF;
        reg.tint_g = (tint >> 8) & 0xFF;
        reg.tint_b = tint & 0xFF;
        reg.interactions = ExtractInteractions(room.Regions[i].EventHandlers, 3);
        data->regions.push_back(reg);
    }

    data->message_count = room.MessageCount;
    for (uint32_t i = 0; i < room.MessageCount && i < MAX_MESSAGES; i++)
    {
        RoomData::MessageData msg;
        msg.id = (int)i;
        msg.text = room.Messages[i].GetCStr();
        msg.display_as = room.MessageInfos[i].DisplayAs;
        msg.auto_remove = (room.MessageInfos[i].Flags & 0x02) != 0; // MSG_TIMELIMIT
        data->messages.push_back(msg);
    }
    data->has_script = (room.CompiledScript != nullptr);

    // Room-level interactions
    data->interactions = ExtractInteractions(room.EventHandlers, 10);

    // Load room-level custom properties
    for (const auto& kv : room.Properties)
        data->custom_properties[kv.first.GetCStr()] = kv.second.GetCStr();

    // Preserve background frame bitmaps
    for (uint32_t i = 0; i < room.BgFrameCount && i < MAX_ROOM_BGFRAMES; i++)
    {
        if (room.BgFrames[i].Graphic)
            data->bg_frames.push_back(room.BgFrames[i].Graphic);
    }

    // Preserve mask bitmaps (8-bit indexed, area ID per pixel)
    if (room.HotspotMask)
        data->hotspot_mask = room.HotspotMask;
    if (room.WalkAreaMask)
        data->walkarea_mask = room.WalkAreaMask;
    if (room.WalkBehindMask)
        data->walkbehind_mask = room.WalkBehindMask;
    if (room.RegionMask)
        data->region_mask = room.RegionMask;

    return data;
}

bool RoomLoader::SaveRoom(int room_number, const RoomData& data)
{
    using namespace AGS::Common;

    std::string filepath = GetRoomFilename(room_number);

    // First, load the original room to preserve data we don't edit
    // (compiled scripts, interaction data, messages, masks, etc.)
    RoomStruct room;
    bool have_original = false;
    {
        RoomDataSource src;
        HRoomFileError err = OpenRoomFile(String(filepath.c_str()), src);
        if (err)
        {
            HRoomFileError read_err = ReadRoomData(&room, std::move(src.InputStream), src.DataVersion);
            if (read_err)
                have_original = true;
        }
    }

    if (!have_original)
    {
        // Initialize a new room with default values
        room.Width = data.width;
        room.Height = data.height;
        room.MaskResolution = data.mask_resolution > 0 ? data.mask_resolution : 1;
        room.BgFrameCount = 1;
        room.BackgroundBPP = 1;
    }

    // Apply our editable data back to the RoomStruct
    room.Width = data.width;
    room.Height = data.height;

    // Edges
    room.Edges.Left = data.left_edge;
    room.Edges.Right = data.right_edge;
    room.Edges.Top = data.top_edge;
    room.Edges.Bottom = data.bottom_edge;

    // Hotspots
    for (size_t i = 0; i < data.hotspots.size() && i < MAX_ROOM_HOTSPOTS; i++)
    {
        room.Hotspots[i].Name = data.hotspots[i].name.c_str();
        room.Hotspots[i].WalkTo.X = data.hotspots[i].walk_to_x;
        room.Hotspots[i].WalkTo.Y = data.hotspots[i].walk_to_y;
        room.Hotspots[i].ScriptName = data.hotspots[i].script_name.c_str();
        ApplyInteractions(room.Hotspots[i].EventHandlers, data.hotspots[i].interactions);
        room.Hotspots[i].Properties.clear();
        for (const auto& kv : data.hotspots[i].custom_properties)
            room.Hotspots[i].Properties[kv.first.c_str()] = kv.second.c_str();
    }
    room.HotspotCount = std::min((uint32_t)data.hotspots.size(), (uint32_t)MAX_ROOM_HOTSPOTS);

    // Objects
    room.Objects.resize(data.objects.size());
    for (size_t i = 0; i < data.objects.size(); i++)
    {
        room.Objects[i].Name = data.objects[i].name.c_str();
        room.Objects[i].ScriptName = data.objects[i].script_name.c_str();
        room.Objects[i].X = data.objects[i].x;
        room.Objects[i].Baseline = data.objects[i].baseline;
        room.Objects[i].Y = data.objects[i].y;
        room.Objects[i].Sprite = data.objects[i].sprite;
        room.Objects[i].IsOn = data.objects[i].visible;
        // Update clickable flag
        if (data.objects[i].clickable)
            room.Objects[i].Flags &= ~OBJF_NOINTERACT;
        else
            room.Objects[i].Flags |= OBJF_NOINTERACT;
        ApplyInteractions(room.Objects[i].EventHandlers, data.objects[i].interactions);
        room.Objects[i].Properties.clear();
        for (const auto& kv : data.objects[i].custom_properties)
            room.Objects[i].Properties[kv.first.c_str()] = kv.second.c_str();
    }

    // Walk areas
    for (size_t i = 0; i < data.walk_areas.size() && i < MAX_WALK_AREAS; i++)
    {
        room.WalkAreas[i].ScalingNear = data.walk_areas[i].scaling_min;
        room.WalkAreas[i].ScalingFar = data.walk_areas[i].scaling_max;
    }
    room.WalkAreaCount = std::min((uint32_t)data.walk_areas.size(), (uint32_t)MAX_WALK_AREAS);

    // Walk behinds
    for (size_t i = 0; i < data.walk_behinds.size() && i < MAX_WALK_BEHINDS; i++)
    {
        room.WalkBehinds[i].Baseline = data.walk_behinds[i].baseline;
    }
    room.WalkBehindCount = std::min((uint32_t)data.walk_behinds.size(), (uint32_t)MAX_WALK_BEHINDS);

    // Regions
    for (size_t i = 0; i < data.regions.size() && i < MAX_ROOM_REGIONS; i++)
    {
        room.Regions[i].Light = data.regions[i].light_level;
        room.Regions[i].Tint = (data.regions[i].tint_r << 16) |
                               (data.regions[i].tint_g << 8) |
                               data.regions[i].tint_b;
        ApplyInteractions(room.Regions[i].EventHandlers, data.regions[i].interactions);
    }
    room.RegionCount = std::min((uint32_t)data.regions.size(), (uint32_t)MAX_ROOM_REGIONS);

    // Room-level interactions
    ApplyInteractions(room.EventHandlers, data.interactions);

    // Room-level custom properties
    room.Properties.clear();
    for (const auto& kv : data.custom_properties)
        room.Properties[kv.first.c_str()] = kv.second.c_str();

    // Apply mask bitmaps back to the room
    if (data.hotspot_mask)
        room.HotspotMask = data.hotspot_mask;
    if (data.walkarea_mask)
        room.WalkAreaMask = data.walkarea_mask;
    if (data.walkbehind_mask)
        room.WalkBehindMask = data.walkbehind_mask;
    if (data.region_mask)
        room.RegionMask = data.region_mask;

    // Apply background frame bitmaps back to the room
    room.BgFrameCount = std::min((uint32_t)data.bg_frames.size(), (uint32_t)MAX_ROOM_BGFRAMES);
    if (room.BgFrameCount == 0) room.BgFrameCount = 1;
    room.BackgroundBPP = data.bg_bpp;
    for (uint32_t i = 0; i < room.BgFrameCount; i++)
    {
        if (i < data.bg_frames.size())
            room.BgFrames[i].Graphic = data.bg_frames[i];
        room.BgFrames[i].IsPaletteShared = (i > 0);
    }
    for (uint32_t i = room.BgFrameCount; i < MAX_ROOM_BGFRAMES; i++)
        room.BgFrames[i].Graphic.reset();

    // Write room data to file
    std::unique_ptr<Stream> out(File::CreateFile(filepath.c_str()));
    if (!out)
    {
        fprintf(stderr, "[RoomLoader] Cannot create room file: %s\n", filepath.c_str());
        return false;
    }

    HRoomFileError write_err = WriteRoomData(&room, out.get(), kRoomVersion_Current);
    if (!write_err)
    {
        fprintf(stderr, "[RoomLoader] Failed to write room data: %s\n",
                write_err->FullMessage().GetCStr());
        return false;
    }

    fprintf(stderr, "[RoomLoader] Saved room %d to %s\n", room_number, filepath.c_str());
    return true;
}

bool RoomLoader::ExportRoom(int room_number, const std::string& output_path)
{
    // Simply copy the .crm file to the target location
    std::string source = GetRoomFilename(room_number);
    if (!fs::exists(source))
    {
        fprintf(stderr, "[RoomLoader] Room file not found: %s\n", source.c_str());
        return false;
    }

    std::error_code ec;
    fs::copy_file(source, output_path, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        fprintf(stderr, "[RoomLoader] Failed to export room: %s\n", ec.message().c_str());
        return false;
    }

    // Also copy companion script files if they exist
    std::string proj_dir = game_dir_;
    char asc_name[64], ash_name[64];
    snprintf(asc_name, sizeof(asc_name), "room%d.asc", room_number);
    snprintf(ash_name, sizeof(ash_name), "room%d.ash", room_number);

    fs::path output_dir = fs::path(output_path).parent_path();
    std::string asc_src = proj_dir + "/" + asc_name;
    std::string ash_src = proj_dir + "/" + ash_name;

    if (fs::exists(asc_src))
        fs::copy_file(asc_src, output_dir / asc_name, fs::copy_options::overwrite_existing, ec);
    if (fs::exists(ash_src))
        fs::copy_file(ash_src, output_dir / ash_name, fs::copy_options::overwrite_existing, ec);

    fprintf(stderr, "[RoomLoader] Exported room %d to %s\n", room_number, output_path.c_str());
    return true;
}

int RoomLoader::ImportRoom(const std::string& crm_path)
{
    if (!fs::exists(crm_path))
    {
        fprintf(stderr, "[RoomLoader] File not found: %s\n", crm_path.c_str());
        return -1;
    }

    // Try to extract room number from filename (e.g. room5.crm -> 5)
    std::string stem = fs::path(crm_path).stem().string();
    int room_number = -1;
    if (stem.size() > 4 && stem.substr(0, 4) == "room")
    {
        try { room_number = std::stoi(stem.substr(4)); } catch (...) {}
    }

    // Find a free room number if extraction failed or room already exists
    if (room_number < 0 || fs::exists(GetRoomFilename(room_number)))
    {
        // Find first available room number starting from 1
        for (int i = 1; i < 1000; i++)
        {
            if (!fs::exists(GetRoomFilename(i)))
            {
                room_number = i;
                break;
            }
        }
    }

    if (room_number < 0)
    {
        fprintf(stderr, "[RoomLoader] Could not find available room slot.\n");
        return -1;
    }

    // Copy .crm to game directory
    std::string dest = GetRoomFilename(room_number);
    std::error_code ec;
    fs::copy_file(crm_path, dest, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        fprintf(stderr, "[RoomLoader] Failed to copy room file: %s\n", ec.message().c_str());
        return -1;
    }

    // Copy companion script files if they exist alongside the source
    fs::path source_dir = fs::path(crm_path).parent_path();
    std::string src_stem = fs::path(crm_path).stem().string();

    // Try copying scripts with original name, or room{N} naming
    std::vector<std::string> script_stems = { src_stem };
    if (src_stem != "room" + std::to_string(room_number))
        script_stems.push_back("room" + std::to_string(room_number));

    for (const auto& ss : script_stems)
    {
        std::string asc_src = (source_dir / (ss + ".asc")).string();
        std::string ash_src = (source_dir / (ss + ".ash")).string();

        char asc_dst[128], ash_dst[128];
        snprintf(asc_dst, sizeof(asc_dst), "%s/room%d.asc", game_dir_.c_str(), room_number);
        snprintf(ash_dst, sizeof(ash_dst), "%s/room%d.ash", game_dir_.c_str(), room_number);

        if (fs::exists(asc_src))
        {
            fs::copy_file(asc_src, asc_dst, fs::copy_options::overwrite_existing, ec);
            if (!ec) fprintf(stderr, "[RoomLoader] Copied script: %s\n", asc_src.c_str());
        }
        if (fs::exists(ash_src))
        {
            fs::copy_file(ash_src, ash_dst, fs::copy_options::overwrite_existing, ec);
            if (!ec) fprintf(stderr, "[RoomLoader] Copied header: %s\n", ash_src.c_str());
        }

        if (fs::exists(asc_src) || fs::exists(ash_src)) break; // Found scripts, stop searching
    }

    fprintf(stderr, "[RoomLoader] Imported room as room%d from %s\n", room_number, crm_path.c_str());
    return room_number;
}

} // namespace AGSEditor
