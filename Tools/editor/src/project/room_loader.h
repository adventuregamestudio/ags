// AGS Editor ImGui - Room file loader using AGS Common room_file API
#pragma once

#include "project/game_data.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace AGS { namespace Common { class RoomStruct; class Bitmap; } }
struct SpriteInfo;

namespace AGSEditor
{

// Simplified room data for display in the editor
struct RoomData
{
    int number = 0;
    int width = 320;
    int height = 200;
    int mask_resolution = 1;
    int bg_frame_count = 1;
    int bg_bpp = 1; // background color depth in bytes

    struct HotspotData {
        int id = 0;
        std::string name;
        std::string script_name;
        std::string description;
        int walk_to_x = -1;
        int walk_to_y = -1;
        Interactions interactions;
        std::map<std::string, std::string> custom_properties;
    };
    std::vector<HotspotData> hotspots;

    struct ObjectData {
        int id = 0;
        std::string name;
        std::string script_name;
        int x = 0, y = 0;
        int sprite = 0;
        bool visible = true;
        int baseline = -1;     // -1 = use object's Y
        bool clickable = true;
        Interactions interactions;
        std::map<std::string, std::string> custom_properties;
        // Design-time properties (editor only, not saved to .crm)
        bool design_visible = true;
        bool design_locked = false;
    };
    std::vector<ObjectData> objects;

    struct WalkAreaData {
        int id = 0;
        int scaling_min = 100;
        int scaling_max = 100;
        bool continuous_scaling = false; // true when ScalingNear != NOT_VECTOR_SCALED
        int top = 0;      // Top Y bound for continuous scaling
        int bottom = 0;    // Bottom Y bound for continuous scaling
    };
    std::vector<WalkAreaData> walk_areas;

    struct WalkBehindData {
        int id = 0;
        int baseline = 0;
    };
    std::vector<WalkBehindData> walk_behinds;

    struct RegionData {
        int id = 0;
        int light_level = 0;
        int tint_r = 0, tint_g = 0, tint_b = 0;
        Interactions interactions;
    };
    std::vector<RegionData> regions;

    struct MessageData {
        int id = 0;
        std::string text;
        int display_as = 0;   // 0 = standard window, >=1 = character speech
        bool auto_remove = false;
    };

    int message_count = 0;
    std::vector<MessageData> messages;
    bool has_script = false;

    // Room-level interactions (e.g. room_Load, room_AfterFadeIn, etc.)
    Interactions interactions;

    // Custom properties (key = property name, value = property value string)
    std::map<std::string, std::string> custom_properties;

    // Edges
    int left_edge = 0, right_edge = 0, top_edge = 0, bottom_edge = 0;

    // Background bitmaps (loaded from room file)
    // These are shared_ptr<Bitmap> from the AGS RoomStruct
    std::vector<std::shared_ptr<AGS::Common::Bitmap>> bg_frames;

    // Mask bitmaps (8-bit indexed, size = room_size / mask_resolution)
    // Each pixel value = area ID. nullptr if mask not loaded.
    std::shared_ptr<AGS::Common::Bitmap> hotspot_mask;
    std::shared_ptr<AGS::Common::Bitmap> walkarea_mask;
    std::shared_ptr<AGS::Common::Bitmap> walkbehind_mask;
    std::shared_ptr<AGS::Common::Bitmap> region_mask;
};

// Loads room files from the game directory
class SpriteLoader;

class RoomLoader
{
public:
    RoomLoader();
    ~RoomLoader();

    // Set the game directory (needed to locate .crm files)
    void SetGameDir(const std::string& game_dir) { game_dir_ = game_dir; }
    void SetGameIsHires(bool hires) { game_is_hires_ = hires; }
    void SetSpriteLoader(SpriteLoader* loader) { sprite_loader_ = loader; }

    // Load a room by number; returns the loaded room data or nullptr on failure
    std::unique_ptr<RoomData> LoadRoom(int room_number);

    // Save room data back to file
    bool SaveRoom(int room_number, const RoomData& data);

    // Export a room's .crm file to an arbitrary path
    bool ExportRoom(int room_number, const std::string& output_path);

    // Import a .crm file as a new room, returns assigned room number or -1 on failure
    int ImportRoom(const std::string& crm_path);

    // Get the room filename for a given number
    std::string GetRoomFilename(int room_number) const;

private:
    std::string game_dir_;
    bool game_is_hires_ = false;
    SpriteLoader* sprite_loader_ = nullptr;
};

} // namespace AGSEditor
